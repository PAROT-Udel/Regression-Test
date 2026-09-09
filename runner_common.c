#define _POSIX_C_SOURCE 200809L

#include "runner_common.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *duplicate_string(const char *value)
{
    size_t length = strlen(value) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, value, length);
    }
    return copy;
}

static int reserve_bytes(void **allocation,
                         size_t *capacity,
                         size_t needed,
                         size_t item_size)
{
    size_t next = *capacity == 0 ? 8 : *capacity;
    void *resized;

    if (needed <= *capacity) {
        return 0;
    }
    while (next < needed) {
        if (next > SIZE_MAX / 2) {
            errno = ENOMEM;
            return -1;
        }
        next *= 2;
    }
    if (item_size != 0 && next > SIZE_MAX / item_size) {
        errno = ENOMEM;
        return -1;
    }
    resized = realloc(*allocation, next * item_size);
    if (resized == NULL) {
        return -1;
    }
    *allocation = resized;
    *capacity = next;
    return 0;
}

void runner_string_init(RunnerString *string)
{
    string->data = NULL;
    string->length = 0;
    string->capacity = 0;
}

int runner_string_append_n(RunnerString *string, const char *text, size_t length)
{
    if (length > SIZE_MAX - string->length - 1 ||
        reserve_bytes((void **)&string->data,
                      &string->capacity,
                      string->length + length + 1,
                      sizeof(*string->data)) != 0) {
        return -1;
    }
    memcpy(string->data + string->length, text, length);
    string->length += length;
    string->data[string->length] = '\0';
    return 0;
}

int runner_string_append(RunnerString *string, const char *text)
{
    return runner_string_append_n(string, text, strlen(text));
}

char *runner_string_take(RunnerString *string)
{
    char *result = string->data;
    if (result == NULL) {
        result = duplicate_string("");
    }
    string->data = NULL;
    string->length = 0;
    string->capacity = 0;
    return result;
}

void runner_string_free(RunnerString *string)
{
    free(string->data);
    runner_string_init(string);
}

void runner_string_vector_init(RunnerStringVector *vector)
{
    vector->items = NULL;
    vector->length = 0;
    vector->capacity = 0;
}

int runner_string_vector_push(RunnerStringVector *vector, const char *value)
{
    char *copy = duplicate_string(value);
    if (copy == NULL) {
        return -1;
    }
    if (reserve_bytes((void **)&vector->items,
                      &vector->capacity,
                      vector->length + 1,
                      sizeof(*vector->items)) != 0) {
        free(copy);
        return -1;
    }
    vector->items[vector->length++] = copy;
    return 0;
}

void runner_string_vector_free(RunnerStringVector *vector)
{
    size_t index;
    for (index = 0; index < vector->length; ++index) {
        free(vector->items[index]);
    }
    free(vector->items);
    runner_string_vector_init(vector);
}

char *runner_path_join(const char *left, const char *right)
{
    size_t left_length = strlen(left);
    size_t right_start = 0;
    int separator;
    char *result;

    while (left_length > 1 && left[left_length - 1] == '/') {
        --left_length;
    }
    while (right[right_start] == '/') {
        ++right_start;
    }
    separator = left_length != 0 && left[left_length - 1] != '/' &&
                right[right_start] != '\0';
    result = malloc(left_length + (size_t)separator +
                    strlen(right + right_start) + 1);
    if (result == NULL) {
        return NULL;
    }
    memcpy(result, left, left_length);
    if (separator) {
        result[left_length++] = '/';
    }
    strcpy(result + left_length, right + right_start);
    return result;
}

const char *runner_path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash == NULL ? path : slash + 1;
}

static int make_directory(const char *path, mode_t mode)
{
    struct stat status;
    if (mkdir(path, mode) == 0) {
        return 0;
    }
    if (errno != EEXIST) {
        return -1;
    }
    if (stat(path, &status) == 0 && S_ISDIR(status.st_mode)) {
        return 0;
    }
    errno = ENOTDIR;
    return -1;
}

int runner_mkdir_recursive(const char *path, mode_t mode)
{
    char *copy;
    char *cursor;

    if (path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    copy = duplicate_string(path);
    if (copy == NULL) {
        return -1;
    }
    for (cursor = copy + 1; *cursor != '\0'; ++cursor) {
        if (*cursor == '/') {
            *cursor = '\0';
            if (make_directory(copy, mode) != 0) {
                free(copy);
                return -1;
            }
            *cursor = '/';
        }
    }
    if (make_directory(copy, mode) != 0) {
        free(copy);
        return -1;
    }
    free(copy);
    return 0;
}

int runner_file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int copy_descriptor(int source, int destination)
{
    char buffer[65536];
    ssize_t count;

    while ((count = read(source, buffer, sizeof(buffer))) > 0) {
        ssize_t offset = 0;
        while (offset < count) {
            ssize_t written = write(
                destination, buffer + offset, (size_t)(count - offset));
            if (written < 0) {
                return -1;
            }
            offset += written;
        }
    }
    return count < 0 ? -1 : 0;
}

int runner_copy_file(const char *source, const char *destination)
{
    struct stat status;
    int input = -1;
    int output = -1;
    int result = -1;

    if (stat(source, &status) != 0 ||
        (input = open(source, O_RDONLY)) < 0 ||
        (output = open(destination, O_WRONLY | O_CREAT | O_TRUNC,
                       status.st_mode & 0777)) < 0) {
        goto cleanup;
    }
    result = copy_descriptor(input, output);
cleanup:
    if (output >= 0 && close(output) != 0) {
        result = -1;
    }
    if (input >= 0) {
        close(input);
    }
    return result;
}

static int copy_tree_entry(const char *source,
                           const char *destination,
                           const char *relative,
                           RunnerCopyFilter filter,
                           void *context)
{
    struct stat status;
    char *source_path = relative[0] ? runner_path_join(source, relative)
                                    : duplicate_string(source);
    char *destination_path =
        relative[0] ? runner_path_join(destination, relative)
                    : duplicate_string(destination);
    DIR *directory = NULL;
    struct dirent *entry;
    int result = -1;

    if (source_path == NULL || destination_path == NULL ||
        lstat(source_path, &status) != 0) {
        goto cleanup;
    }
    if (S_ISDIR(status.st_mode)) {
        if (relative[0] && filter != NULL &&
            !filter(relative, 1, context)) {
            result = 0;
            goto cleanup;
        }
        if (runner_mkdir_recursive(destination_path, status.st_mode & 0777) != 0 ||
            (directory = opendir(source_path)) == NULL) {
            goto cleanup;
        }
        result = 0;
        while ((entry = readdir(directory)) != NULL) {
            char *child;
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            child = relative[0] ? runner_path_join(relative, entry->d_name)
                                : duplicate_string(entry->d_name);
            if (child == NULL ||
                copy_tree_entry(
                    source, destination, child, filter, context) != 0) {
                free(child);
                result = -1;
                break;
            }
            free(child);
        }
    } else if (S_ISREG(status.st_mode)) {
        if (filter == NULL || filter(relative, 0, context)) {
            result = runner_copy_file(source_path, destination_path);
        } else {
            result = 0;
        }
    } else {
        errno = ENOTSUP;
    }
cleanup:
    if (directory != NULL) {
        closedir(directory);
    }
    free(destination_path);
    free(source_path);
    return result;
}

int runner_copy_tree(const char *source,
                     const char *destination,
                     RunnerCopyFilter filter,
                     void *context)
{
    return copy_tree_entry(source, destination, "", filter, context);
}

int runner_remove_recursive(const char *path)
{
    struct stat status;

    if (lstat(path, &status) != 0) {
        return errno == ENOENT ? 0 : -1;
    }
    if (S_ISDIR(status.st_mode)) {
        DIR *directory = opendir(path);
        struct dirent *entry;
        int result = 0;
        if (directory == NULL) {
            return -1;
        }
        while ((entry = readdir(directory)) != NULL) {
            char *child;
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            child = runner_path_join(path, entry->d_name);
            if (child == NULL || runner_remove_recursive(child) != 0) {
                result = -1;
            }
            free(child);
            if (result != 0) {
                break;
            }
        }
        closedir(directory);
        return result == 0 ? rmdir(path) : -1;
    }
    return unlink(path);
}

int runner_read_file(const char *path, char **contents, size_t *length)
{
    FILE *file = fopen(path, "rb");
    RunnerString output;
    char buffer[8192];
    size_t count;
    size_t output_length;

    *contents = NULL;
    if (length != NULL) {
        *length = 0;
    }
    if (file == NULL) {
        return -1;
    }
    runner_string_init(&output);
    while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (runner_string_append_n(&output, buffer, count) != 0) {
            fclose(file);
            runner_string_free(&output);
            return -1;
        }
    }
    if (ferror(file) || fclose(file) != 0) {
        runner_string_free(&output);
        return -1;
    }
    output_length = output.length;
    *contents = runner_string_take(&output);
    if (*contents == NULL) {
        return -1;
    }
    if (length != NULL) {
        *length = output_length;
    }
    return 0;
}

int runner_write_file(const char *path, const void *contents, size_t length)
{
    FILE *file = fopen(path, "wb");
    int result;
    if (file == NULL) {
        return -1;
    }
    result = fwrite(contents, 1, length, file) == length ? 0 : -1;
    if (fclose(file) != 0) {
        result = -1;
    }
    return result;
}

void runner_key_value_map_init(RunnerKeyValueMap *map)
{
    map->items = NULL;
    map->length = 0;
    map->capacity = 0;
}

int runner_key_value_map_set(
    RunnerKeyValueMap *map, const char *key, const char *value)
{
    size_t index;
    char *key_copy;
    char *value_copy = duplicate_string(value);
    if (value_copy == NULL) {
        return -1;
    }
    for (index = 0; index < map->length; ++index) {
        if (strcmp(map->items[index].key, key) == 0) {
            free(map->items[index].value);
            map->items[index].value = value_copy;
            return 0;
        }
    }
    key_copy = duplicate_string(key);
    if (key_copy == NULL ||
        reserve_bytes((void **)&map->items,
                      &map->capacity,
                      map->length + 1,
                      sizeof(*map->items)) != 0) {
        free(key_copy);
        free(value_copy);
        return -1;
    }
    map->items[map->length].key = key_copy;
    map->items[map->length].value = value_copy;
    ++map->length;
    return 0;
}

const char *runner_key_value_map_get(
    const RunnerKeyValueMap *map, const char *key)
{
    size_t index;
    for (index = 0; index < map->length; ++index) {
        if (strcmp(map->items[index].key, key) == 0) {
            return map->items[index].value;
        }
    }
    return NULL;
}

static char *trim(char *value)
{
    char *end;
    while (*value == ' ' || *value == '\t') {
        ++value;
    }
    end = value + strlen(value);
    while (end > value &&
           (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
        --end;
    }
    *end = '\0';
    return value;
}

int runner_parse_env_text(const char *text, RunnerKeyValueMap *map)
{
    char *copy = duplicate_string(text);
    char *line;
    char *save = NULL;
    int result = 0;

    if (copy == NULL) {
        return -1;
    }
    for (line = strtok_r(copy, "\n", &save);
         line != NULL;
         line = strtok_r(NULL, "\n", &save)) {
        char *key = trim(line);
        char *equals;
        char *value;
        size_t value_length;
        if (*key == '\0' || *key == '#') {
            continue;
        }
        equals = strchr(key, '=');
        if (equals == NULL) {
            errno = EINVAL;
            result = -1;
            break;
        }
        *equals = '\0';
        key = trim(key);
        value = trim(equals + 1);
        value_length = strlen(value);
        if (value_length >= 2 &&
            ((value[0] == '"' && value[value_length - 1] == '"') ||
             (value[0] == '\'' && value[value_length - 1] == '\''))) {
            value[value_length - 1] = '\0';
            ++value;
        }
        if (*key == '\0' || runner_key_value_map_set(map, key, value) != 0) {
            result = -1;
            break;
        }
    }
    free(copy);
    return result;
}

int runner_parse_env_file(const char *path, RunnerKeyValueMap *map)
{
    char *contents;
    int result;
    if (runner_read_file(path, &contents, NULL) != 0) {
        return -1;
    }
    result = runner_parse_env_text(contents, map);
    free(contents);
    return result;
}

void runner_key_value_map_free(RunnerKeyValueMap *map)
{
    size_t index;
    for (index = 0; index < map->length; ++index) {
        free(map->items[index].key);
        free(map->items[index].value);
    }
    free(map->items);
    runner_key_value_map_init(map);
}

char *runner_template_replace(
    const char *input, const char *placeholder, const char *replacement)
{
    RunnerString output;
    const char *cursor = input;
    const char *match;
    size_t placeholder_length = strlen(placeholder);

    if (placeholder_length == 0) {
        errno = EINVAL;
        return NULL;
    }
    runner_string_init(&output);
    while ((match = strstr(cursor, placeholder)) != NULL) {
        if (runner_string_append_n(&output, cursor, (size_t)(match - cursor)) != 0 ||
            runner_string_append(&output, replacement) != 0) {
            runner_string_free(&output);
            return NULL;
        }
        cursor = match + placeholder_length;
    }
    if (runner_string_append(&output, cursor) != 0) {
        runner_string_free(&output);
        return NULL;
    }
    return runner_string_take(&output);
}
