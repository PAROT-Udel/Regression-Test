#ifndef RUNNER_COMMON_H
#define RUNNER_COMMON_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} RunnerString;

void runner_string_init(RunnerString *string);
int runner_string_append(RunnerString *string, const char *text);
int runner_string_append_n(RunnerString *string, const char *text, size_t length);
char *runner_string_take(RunnerString *string);
void runner_string_free(RunnerString *string);

typedef struct {
    char **items;
    size_t length;
    size_t capacity;
} RunnerStringVector;

void runner_string_vector_init(RunnerStringVector *vector);
int runner_string_vector_push(RunnerStringVector *vector, const char *value);
void runner_string_vector_free(RunnerStringVector *vector);

char *runner_path_join(const char *left, const char *right);
const char *runner_path_basename(const char *path);
int runner_mkdir_recursive(const char *path, mode_t mode);
int runner_file_exists(const char *path);
int runner_copy_file(const char *source, const char *destination);

typedef int (*RunnerCopyFilter)(
    const char *relative_path, int is_directory, void *context);

int runner_copy_tree(const char *source,
                     const char *destination,
                     RunnerCopyFilter filter,
                     void *context);
int runner_remove_recursive(const char *path);
int runner_read_file(const char *path, char **contents, size_t *length);
int runner_write_file(const char *path, const void *contents, size_t length);

typedef struct {
    char *key;
    char *value;
} RunnerKeyValue;

typedef struct {
    RunnerKeyValue *items;
    size_t length;
    size_t capacity;
} RunnerKeyValueMap;

void runner_key_value_map_init(RunnerKeyValueMap *map);
int runner_key_value_map_set(
    RunnerKeyValueMap *map, const char *key, const char *value);
const char *runner_key_value_map_get(
    const RunnerKeyValueMap *map, const char *key);
int runner_parse_env_text(const char *text, RunnerKeyValueMap *map);
int runner_parse_env_file(const char *path, RunnerKeyValueMap *map);
void runner_key_value_map_free(RunnerKeyValueMap *map);

char *runner_template_replace(
    const char *input, const char *placeholder, const char *replacement);

#endif
