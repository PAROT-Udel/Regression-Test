#define _POSIX_C_SOURCE 200809L

#include "kernel_config.h"
#include "cJSON.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *value)
{
    size_t length;
    char *copy;
    if (value == NULL)
        return NULL;
    length = strlen(value) + 1;
    copy = malloc(length);
    if (copy != NULL)
        memcpy(copy, value, length);
    return copy;
}

static int fail(char *error, size_t size, const char *format, ...)
{
    va_list arguments;
    if (error != NULL && size != 0) {
        va_start(arguments, format);
        vsnprintf(error, size, format, arguments);
        va_end(arguments);
    }
    return -1;
}

static int fail_field(char *error,
                      size_t size,
                      const char *source,
                      const char *field,
                      const char *message)
{
    return fail(error, size, "%s: field '%s' %s", source, field, message);
}

static int eq_ci(const char *left, const char *right)
{
    if (left == NULL || right == NULL)
        return 0;
    while (*left && *right) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        left++;
        right++;
    }
    return *left == *right;
}

static int safe_name(const char *value)
{
    const unsigned char *cursor;
    if (value == NULL || value[0] == '\0' || strcmp(value, ".") == 0 ||
        strcmp(value, "..") == 0)
        return 0;
    for (cursor = (const unsigned char *)value; *cursor != '\0'; ++cursor) {
        if (!(('a' <= *cursor && *cursor <= 'z') ||
              ('A' <= *cursor && *cursor <= 'Z') ||
              ('0' <= *cursor && *cursor <= '9') || *cursor == '-' ||
              *cursor == '_' || *cursor == '.'))
            return 0;
    }
    return strstr(value, "..") == NULL;
}

static int safe_relative_path(const char *path)
{
    const char *cursor;
    if (path == NULL || path[0] == '\0')
        return 0;
    if (path[0] == '/' || path[0] == '\\')
        return 0;
    if (((path[0] >= 'A' && path[0] <= 'Z') ||
         (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':')
        return 0;
    cursor = path;
    while (*cursor != '\0') {
        if (cursor == path || cursor[-1] == '/' || cursor[-1] == '\\') {
            if (cursor[0] == '.' && cursor[1] == '.' &&
                (cursor[2] == '/' || cursor[2] == '\\' || cursor[2] == '\0'))
                return 0;
        }
        ++cursor;
    }
    return 1;
}

static int json_string(cJSON *object,
                       const char *field,
                       int required,
                       char **destination,
                       const char *source,
                       char *error,
                       size_t error_size)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, field);
    if (item == NULL)
        return required ? fail_field(error, error_size, source, field, "is required")
                        : 0;
    if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
        return fail_field(
            error, error_size, source, field, "must be a non-empty string");
    }
    *destination = copy_string(item->valuestring);
    if (*destination == NULL)
        return fail(error, error_size, "%s: out of memory", source);
    return 0;
}

static int json_strings(cJSON *object,
                        const char *field,
                        int required,
                        int allow_empty,
                        RunnerStringVector *destination,
                        const char *source,
                        char *error,
                        size_t error_size)
{
    cJSON *array = cJSON_GetObjectItemCaseSensitive(object, field);
    cJSON *item;
    if (array == NULL) {
        return required ? fail_field(error, error_size, source, field, "is required")
                        : 0;
    }
    if (!cJSON_IsArray(array))
        return fail_field(error, error_size, source, field, "must be an array");
    if (!allow_empty && cJSON_GetArraySize(array) == 0) {
        return fail_field(
            error, error_size, source, field, "must not be empty");
    }
    cJSON_ArrayForEach(item, array) {
        if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
            return fail_field(error,
                              error_size,
                              source,
                              field,
                              "must contain only non-empty strings");
        }
        if (runner_string_vector_push(destination, item->valuestring) != 0)
            return fail(error, error_size, "%s: out of memory", source);
    }
    return 0;
}

static int parse_transform(const char *text,
                           TransformationType *type,
                           const char *source,
                           char *error,
                           size_t error_size)
{
    if (strcmp(text, "tiling") == 0)
        *type = TRANSFORM_TILING;
    else if (strcmp(text, "subsub_analysis") == 0)
        *type = TRANSFORM_SUBSUB_ANALYSIS;
    else if (strcmp(text, "parallelization") == 0)
        *type = TRANSFORM_PARALLELIZATION;
    else if (strcmp(text, "privatization") == 0)
        *type = TRANSFORM_PRIVATIZATION;
    else if (strcmp(text, "reduction") == 0)
        *type = TRANSFORM_REDUCTION;
    else if (strcmp(text, "no_transformation") == 0)
        *type = TRANSFORM_NONE;
    else {
        return fail_field(
            error, error_size, source, "transform", "is not a known transform");
    }
    return 0;
}

static int parse_expected(const char *text,
                          ExpectedOutcome *outcome,
                          const char *source,
                          char *error,
                          size_t error_size)
{
    if (strcmp(text, "success_transformed") == 0)
        *outcome = EXPECT_SUCCESS_TRANSFORMED;
    else if (strcmp(text, "success_no_change") == 0)
        *outcome = EXPECT_SUCCESS_NO_CHANGE;
    else if (strcmp(text, "expect_failure") == 0)
        *outcome = EXPECT_FAILURE;
    else {
        return fail_field(
            error, error_size, source, "expected", "is not a known outcome");
    }
    return 0;
}

static int join_flags(const RunnerStringVector *base,
                      const RunnerStringVector *extra,
                      char **joined,
                      const char *source,
                      char *error,
                      size_t error_size)
{
    RunnerString text;
    size_t index;
    runner_string_init(&text);
    for (index = 0; index < base->length; ++index) {
        if (text.length > 0 && runner_string_append(&text, " ") != 0)
            goto oom;
        if (runner_string_append(&text, base->items[index]) != 0)
            goto oom;
    }
    for (index = 0; index < extra->length; ++index) {
        if (text.length > 0 && runner_string_append(&text, " ") != 0)
            goto oom;
        if (runner_string_append(&text, extra->items[index]) != 0)
            goto oom;
    }
    *joined = runner_string_take(&text);
    if (*joined == NULL)
        goto oom;
    return 0;
oom:
    runner_string_free(&text);
    return fail(error, error_size, "%s: out of memory", source);
}

static int name_taken(const KernelConfig *config, const char *name)
{
    size_t index;
    for (index = 0; index < config->case_count; ++index) {
        if (eq_ci(config->cases[index].name, name))
            return 1;
    }
    return 0;
}

static int suite_taken(const KernelConfig *config, const char *name)
{
    size_t index;
    size_t alias;
    for (index = 0; index < config->suite_count; ++index) {
        KernelSuiteInfo *suite = &config->suites[index];
        if (eq_ci(suite->id, name))
            return 1;
        for (alias = 0; alias < suite->aliases.length; ++alias) {
            if (eq_ci(suite->aliases.items[alias], name))
                return 1;
        }
    }
    return 0;
}

static void suite_info_clear(KernelSuiteInfo *suite)
{
    free(suite->id);
    free(suite->description);
    runner_string_vector_free(&suite->aliases);
    memset(suite, 0, sizeof(*suite));
}

static int parse_suite_file(const char *path,
                            KernelConfig *config,
                            char *error,
                            size_t error_size)
{
    char *contents = NULL;
    size_t length = 0;
    cJSON *root = NULL;
    cJSON *kernels = NULL;
    cJSON *item;
    KernelSuiteInfo suite;
    KernelCase *built = NULL;
    size_t built_count = 0;
    char *suite_transform = NULL;
    char *suite_expected = NULL;
    RunnerStringVector suite_flags;
    TransformationType transform = TRANSFORM_UNKNOWN;
    ExpectedOutcome expected = EXPECT_SUCCESS_TRANSFORMED;
    int result = -1;

    memset(&suite, 0, sizeof(suite));
    runner_string_vector_init(&suite.aliases);
    runner_string_vector_init(&suite_flags);
    if (runner_read_file(path, &contents, &length) != 0)
        return fail(error, error_size, "%s: cannot read file", path);
    root = cJSON_Parse(contents);
    if (root == NULL || !cJSON_IsObject(root)) {
        fail(error, error_size, "%s: invalid JSON", path);
        goto cleanup;
    }
    if (json_string(root, "id", 1, &suite.id, path, error, error_size) != 0 ||
        json_string(root, "description", 0, &suite.description, path, error, error_size) != 0 ||
        json_string(root, "transform", 1, &suite_transform, path, error, error_size) != 0 ||
        json_strings(root, "flags", 1, 0, &suite_flags, path, error, error_size) != 0 ||
        json_strings(root, "aliases", 0, 1, &suite.aliases, path, error, error_size) != 0)
        goto cleanup;
    if (!safe_name(suite.id)) {
        fail_field(error, error_size, path, "id", "must be a plain name");
        goto cleanup;
    }
    if (suite_taken(config, suite.id)) {
        fail(error, error_size, "%s: duplicate suite name '%s'", path, suite.id);
        goto cleanup;
    }
    if (parse_transform(suite_transform, &transform, path, error, error_size) != 0)
        goto cleanup;
    if (cJSON_GetObjectItemCaseSensitive(root, "expected") != NULL) {
        if (json_string(root, "expected", 1, &suite_expected, path, error, error_size) != 0 ||
            parse_expected(suite_expected, &expected, path, error, error_size) != 0)
            goto cleanup;
    }
    {
        size_t alias;
        for (alias = 0; alias < suite.aliases.length; ++alias) {
            const char *name = suite.aliases.items[alias];
            size_t prior;
            if (!safe_name(name) || eq_ci(name, suite.id) || suite_taken(config, name)) {
                fail(error, error_size, "%s: duplicate suite name '%s'", path, name);
                goto cleanup;
            }
            for (prior = 0; prior < alias; ++prior) {
                if (eq_ci(suite.aliases.items[prior], name)) {
                    fail(error, error_size, "%s: duplicate suite name '%s'", path, name);
                    goto cleanup;
                }
            }
        }
    }
    kernels = cJSON_GetObjectItemCaseSensitive(root, "kernels");
    if (kernels == NULL) {
        fail_field(error, error_size, path, "kernels", "is required");
        goto cleanup;
    }
    if (!cJSON_IsArray(kernels) || cJSON_GetArraySize(kernels) == 0) {
        fail_field(error, error_size, path, "kernels", "must not be empty");
        goto cleanup;
    }
    built = calloc((size_t)cJSON_GetArraySize(kernels), sizeof(*built));
    if (built == NULL) {
        fail(error, error_size, "%s: out of memory", path);
        goto cleanup;
    }
    cJSON_ArrayForEach(item, kernels) {
        KernelCase *kernel = &built[built_count];
        char *kernel_transform = NULL;
        char *kernel_expected = NULL;
        RunnerStringVector kernel_flags;
        runner_string_vector_init(&kernel_flags);
        if (!cJSON_IsObject(item)) {
            fail_field(error, error_size, path, "kernels", "must contain objects");
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        kernel->transform = transform;
        kernel->expected = expected;
        if (json_string(item, "name", 1, &kernel->name, path, error, error_size) != 0 ||
            json_string(item, "input", 1, &kernel->input, path, error, error_size) != 0 ||
            json_strings(item, "flags", 0, 1, &kernel_flags, path, error, error_size) != 0) {
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        if (cJSON_GetObjectItemCaseSensitive(item, "transform") != NULL) {
            if (json_string(item, "transform", 1, &kernel_transform, path, error, error_size) != 0 ||
                parse_transform(kernel_transform, &kernel->transform, path, error, error_size) != 0) {
                free(kernel_transform);
                runner_string_vector_free(&kernel_flags);
                goto cleanup;
            }
        }
        if (cJSON_GetObjectItemCaseSensitive(item, "expected") != NULL) {
            if (json_string(item, "expected", 1, &kernel_expected, path, error, error_size) != 0 ||
                parse_expected(kernel_expected, &kernel->expected, path, error, error_size) != 0) {
                free(kernel_transform);
                free(kernel_expected);
                runner_string_vector_free(&kernel_flags);
                goto cleanup;
            }
        }
        free(kernel_transform);
        free(kernel_expected);
        if (!safe_name(kernel->name)) {
            fail_field(error, error_size, path, "name", "must be a plain name");
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        if (!safe_relative_path(kernel->input)) {
            fail_field(error, error_size, path, "input", "must be a safe relative path");
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        if (name_taken(config, kernel->name)) {
            fail(error, error_size, "%s: duplicate kernel name '%s'", path, kernel->name);
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        {
            size_t prior;
            for (prior = 0; prior < built_count; ++prior) {
                if (eq_ci(built[prior].name, kernel->name)) {
                    fail(error,
                         error_size,
                         "%s: duplicate kernel name '%s'",
                         path,
                         kernel->name);
                    runner_string_vector_free(&kernel_flags);
                    goto cleanup;
                }
            }
        }
        kernel->suite = copy_string(suite.id);
        if (kernel->suite == NULL ||
            join_flags(&suite_flags, &kernel_flags, &kernel->flags, path, error, error_size) != 0) {
            runner_string_vector_free(&kernel_flags);
            goto cleanup;
        }
        runner_string_vector_free(&kernel_flags);
        built_count++;
    }

    {
        KernelSuiteInfo *grown_suites =
            realloc(config->suites, (config->suite_count + 1) * sizeof(*grown_suites));
        KernelCase *grown_cases;
        if (grown_suites == NULL) {
            fail(error, error_size, "%s: out of memory", path);
            goto cleanup;
        }
        config->suites = grown_suites;
        grown_cases =
            realloc(config->cases, (config->case_count + built_count) * sizeof(*grown_cases));
        if (grown_cases == NULL) {
            fail(error, error_size, "%s: out of memory", path);
            goto cleanup;
        }
        config->cases = grown_cases;
        if (suite.description == NULL)
            suite.description = copy_string("");
        config->suites[config->suite_count] = suite;
        memset(&suite, 0, sizeof(suite));
        memcpy(config->cases + config->case_count, built, built_count * sizeof(*built));
        config->suite_count++;
        config->case_count += built_count;
        free(built);
        built = NULL;
        built_count = 0;
        result = 0;
    }

cleanup:
    if (built != NULL) {
        size_t index;
        for (index = 0; index < built_count; ++index) {
            free(built[index].suite);
            free(built[index].name);
            free(built[index].input);
            free(built[index].flags);
        }
        /* The in-progress kernel may have partial fields past built_count. */
        if (built_count < (size_t)cJSON_GetArraySize(kernels) || kernels == NULL) {
            free(built[built_count].suite);
            free(built[built_count].name);
            free(built[built_count].input);
            free(built[built_count].flags);
        }
        free(built);
    }
    suite_info_clear(&suite);
    runner_string_vector_free(&suite_flags);
    free(suite_transform);
    free(suite_expected);
    cJSON_Delete(root);
    free(contents);
    return result;
}

static int compare_strings(const void *left, const void *right)
{
    const char *const *a = left;
    const char *const *b = right;
    return strcmp(*a, *b);
}

static int discover_files(const char *directory,
                          RunnerStringVector *files,
                          char *error,
                          size_t error_size)
{
    DIR *stream = opendir(directory);
    struct dirent *entry;
    if (stream == NULL) {
        return fail(error,
                    error_size,
                    "%s: cannot open kernel directory: %s",
                    directory,
                    strerror(errno));
    }
    while ((entry = readdir(stream)) != NULL) {
        size_t length = strlen(entry->d_name);
        if (entry->d_name[0] != '.' && length > 5 &&
            strcmp(entry->d_name + length - 5, ".json") == 0 &&
            runner_string_vector_push(files, entry->d_name) != 0) {
            closedir(stream);
            return fail(error, error_size, "%s: out of memory", directory);
        }
    }
    closedir(stream);
    if (files->length == 0) {
        return fail(error, error_size, "%s: no kernel JSON files found", directory);
    }
    qsort(files->items, files->length, sizeof(*files->items), compare_strings);
    return 0;
}

void kernel_config_init(KernelConfig *config)
{
    memset(config, 0, sizeof(*config));
}

void kernel_config_free(KernelConfig *config)
{
    size_t index;
    if (config == NULL)
        return;
    for (index = 0; index < config->suite_count; ++index) {
        free(config->suites[index].id);
        free(config->suites[index].description);
        runner_string_vector_free(&config->suites[index].aliases);
    }
    free(config->suites);
    for (index = 0; index < config->case_count; ++index) {
        free(config->cases[index].suite);
        free(config->cases[index].name);
        free(config->cases[index].input);
        free(config->cases[index].flags);
    }
    free(config->cases);
    kernel_config_init(config);
}

int kernel_config_load(const char *directory,
                       KernelConfig *config,
                       char *error,
                       size_t error_size)
{
    RunnerStringVector files;
    size_t index;
    if (error != NULL && error_size != 0)
        error[0] = '\0';
    kernel_config_free(config);
    runner_string_vector_init(&files);
    if (discover_files(directory, &files, error, error_size) != 0) {
        runner_string_vector_free(&files);
        return -1;
    }
    for (index = 0; index < files.length; ++index) {
        char *path = runner_path_join(directory, files.items[index]);
        if (path == NULL ||
            parse_suite_file(path, config, error, error_size) != 0) {
            free(path);
            runner_string_vector_free(&files);
            kernel_config_free(config);
            return -1;
        }
        free(path);
    }
    runner_string_vector_free(&files);
    return 0;
}

int kernel_config_matches_suite(const KernelConfig *config,
                                const KernelCase *kernel,
                                const char *suite_name)
{
    size_t index;
    if (config == NULL || kernel == NULL || kernel->suite == NULL || suite_name == NULL)
        return 0;
    for (index = 0; index < config->suite_count; ++index) {
        KernelSuiteInfo *suite = &config->suites[index];
        size_t alias;
        int named = eq_ci(suite->id, suite_name);
        for (alias = 0; !named && alias < suite->aliases.length; ++alias)
            named = eq_ci(suite->aliases.items[alias], suite_name);
        if (named)
            return eq_ci(kernel->suite, suite->id);
    }
    return eq_ci(kernel->suite, suite_name);
}
