#define _POSIX_C_SOURCE 200809L

#include "benchmark_config.h"
#include "cJSON.h"

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
    if (value == NULL) {
        return NULL;
    }
    length = strlen(value) + 1;
    copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, value, length);
    }
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

static void command_init(BenchmarkCommand *command)
{
    memset(command, 0, sizeof(*command));
    runner_string_vector_init(&command->argv);
}

static void command_free(BenchmarkCommand *command)
{
    free(command->shell);
    runner_string_vector_free(&command->argv);
    command_init(command);
}

static void verify_init(BenchmarkVerify *verify)
{
    memset(verify, 0, sizeof(*verify));
    command_init(&verify->run);
    command_init(&verify->baseline);
}

static void verify_free(BenchmarkVerify *verify)
{
    free(verify->pattern);
    command_free(&verify->run);
    command_free(&verify->baseline);
    verify_init(verify);
}

static void generic_init(BenchmarkGenericData *generic)
{
    memset(generic, 0, sizeof(*generic));
    runner_string_vector_init(&generic->copy);
    runner_string_vector_init(&generic->cetus_inputs);
    runner_key_value_map_init(&generic->inject_map);
    command_init(&generic->compile);
    command_init(&generic->run);
    verify_init(&generic->verify);
}

static void generic_free(BenchmarkGenericData *generic)
{
    free(generic->preprocessor);
    runner_string_vector_free(&generic->copy);
    runner_string_vector_free(&generic->cetus_inputs);
    runner_key_value_map_free(&generic->inject_map);
    command_free(&generic->compile);
    command_free(&generic->run);
    verify_free(&generic->verify);
    generic_init(generic);
}

static void kernel_init(BenchmarkKernel *kernel)
{
    memset(kernel, 0, sizeof(*kernel));
    runner_string_vector_init(&kernel->headers);
    runner_string_vector_init(&kernel->sources);
    runner_key_value_map_init(&kernel->inject_map);
    runner_key_value_map_init(&kernel->xfail_profiles);
    runner_string_vector_init(&kernel->cetus_sources);
    generic_init(&kernel->generic);
}

static void kernel_free(BenchmarkKernel *kernel)
{
    free(kernel->name);
    free(kernel->rel_path);
    free(kernel->bench);
    free(kernel->binary);
    runner_string_vector_free(&kernel->headers);
    runner_string_vector_free(&kernel->sources);
    runner_key_value_map_free(&kernel->inject_map);
    free(kernel->xfail);
    runner_key_value_map_free(&kernel->xfail_profiles);
    free(kernel->root_env);
    free(kernel->src_file);
    free(kernel->header_file);
    runner_string_vector_free(&kernel->cetus_sources);
    generic_free(&kernel->generic);
    kernel_init(kernel);
}

static void suite_init(BenchmarkSuite *suite)
{
    memset(suite, 0, sizeof(*suite));
    runner_string_vector_init(&suite->profiles);
    runner_key_value_map_init(&suite->xfail_profiles);
    generic_init(&suite->generic);
}

static void suite_free(BenchmarkSuite *suite)
{
    size_t index;
    free(suite->id);
    free(suite->adapter_name);
    free(suite->description);
    free(suite->root_env);
    free(suite->dataset);
    free(suite->npb_class);
    runner_string_vector_free(&suite->profiles);
    free(suite->xfail);
    runner_key_value_map_free(&suite->xfail_profiles);
    generic_free(&suite->generic);
    for (index = 0; index < suite->kernel_count; ++index) {
        kernel_free(&suite->kernels[index]);
    }
    free(suite->kernels);
    suite_init(suite);
}

void benchmark_config_init(BenchmarkConfig *config)
{
    memset(config, 0, sizeof(*config));
    runner_key_value_map_init(&config->paths);
}

void benchmark_config_free(BenchmarkConfig *config)
{
    size_t index;
    runner_key_value_map_free(&config->paths);
    for (index = 0; index < config->profile_count; ++index) {
        free(config->profiles[index].name);
        free(config->profiles[index].description);
        runner_string_vector_free(&config->profiles[index].flags);
    }
    free(config->profiles);
    for (index = 0; index < config->suite_count; ++index) {
        suite_free(&config->suites[index]);
    }
    free(config->suites);
    benchmark_config_init(config);
}

const char *benchmark_config_path(const BenchmarkConfig *config,
                                  const char *name)
{
    return runner_key_value_map_get(&config->paths, name);
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
    if (item == NULL) {
        return required
                   ? fail_field(error, error_size, source, field, "is required")
                   : 0;
    }
    if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
        return fail_field(
            error, error_size, source, field, "must be a non-empty string");
    }
    *destination = copy_string(item->valuestring);
    if (*destination == NULL) {
        return fail(error, error_size, "%s: out of memory", source);
    }
    return 0;
}

static int json_optional_bool(cJSON *object,
                              const char *field,
                              int *destination,
                              const char *source,
                              char *error,
                              size_t error_size)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, field);
    if (item == NULL) {
        return 0;
    }
    if (!cJSON_IsBool(item)) {
        return fail_field(error, error_size, source, field, "must be a boolean");
    }
    *destination = cJSON_IsTrue(item);
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
        return required
                   ? fail_field(error, error_size, source, field, "is required")
                   : 0;
    }
    if (!cJSON_IsArray(array)) {
        return fail_field(error, error_size, source, field, "must be an array");
    }
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
        if (runner_string_vector_push(destination, item->valuestring) != 0) {
            return fail(error, error_size, "%s: out of memory", source);
        }
    }
    return 0;
}

static int json_string_map(cJSON *object,
                           const char *field,
                           RunnerKeyValueMap *destination,
                           const char *source,
                           char *error,
                           size_t error_size)
{
    cJSON *map = cJSON_GetObjectItemCaseSensitive(object, field);
    cJSON *item;
    if (map == NULL) {
        return 0;
    }
    if (!cJSON_IsObject(map)) {
        return fail_field(error, error_size, source, field, "must be an object");
    }
    cJSON_ArrayForEach(item, map) {
        if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
            return fail_field(error,
                              error_size,
                              source,
                              field,
                              "values must be non-empty strings");
        }
        if (runner_key_value_map_set(
                destination, item->string, item->valuestring) != 0) {
            return fail(error, error_size, "%s: out of memory", source);
        }
    }
    return 0;
}

static int parse_command(cJSON *object,
                         const char *field,
                         BenchmarkCommand *command,
                         int *present,
                         const char *source,
                         char *error,
                         size_t error_size)
{
    cJSON *value = cJSON_GetObjectItemCaseSensitive(object, field);
    cJSON *item;
    if (value == NULL) {
        return 0;
    }
    *present = 1;
    if (cJSON_IsString(value) && value->valuestring[0] != '\0') {
        command->type = BENCHMARK_COMMAND_SHELL;
        command->shell = copy_string(value->valuestring);
        if (command->shell == NULL) {
            return fail(error, error_size, "%s: out of memory", source);
        }
        return 0;
    }
    if (!cJSON_IsArray(value) || cJSON_GetArraySize(value) == 0) {
        return fail_field(error,
                          error_size,
                          source,
                          field,
                          "must be a non-empty string or string array");
    }
    command->type = BENCHMARK_COMMAND_ARGV;
    cJSON_ArrayForEach(item, value) {
        if (!cJSON_IsString(item) || item->valuestring[0] == '\0' ||
            runner_string_vector_push(&command->argv, item->valuestring) != 0) {
            return fail_field(error,
                              error_size,
                              source,
                              field,
                              "must contain only non-empty strings");
        }
    }
    return 0;
}

static int parse_verify(cJSON *object,
                        BenchmarkVerify *verify,
                        int *present,
                        const char *source,
                        char *error,
                        size_t error_size)
{
    cJSON *value = cJSON_GetObjectItemCaseSensitive(object, "verify");
    cJSON *type;
    cJSON *expect;
    int run_present = 0;
    if (value == NULL) {
        return 0;
    }
    *present = 1;
    verify->present = 1;
    if (!cJSON_IsObject(value)) {
        return fail_field(
            error, error_size, source, "verify", "must be an object");
    }
    type = cJSON_GetObjectItemCaseSensitive(value, "type");
    if (type == NULL || !cJSON_IsString(type)) {
        return fail_field(error,
                          error_size,
                          source,
                          "verify.type",
                          "must be a string");
    }
    if (strcmp(type->valuestring, "exit_code") == 0) {
        verify->type = BENCHMARK_VERIFY_EXIT_CODE;
    } else if (strcmp(type->valuestring, "regex") == 0) {
        verify->type = BENCHMARK_VERIFY_REGEX;
    } else if (strcmp(type->valuestring, "dump_diff") == 0) {
        verify->type = BENCHMARK_VERIFY_DUMP_DIFF;
    } else {
        return fail_field(error,
                          error_size,
                          source,
                          "verify.type",
                          "must be exit_code, regex, or dump_diff");
    }
    expect = cJSON_GetObjectItemCaseSensitive(value, "expect");
    if (expect != NULL) {
        if (!cJSON_IsNumber(expect) || expect->valuedouble != expect->valueint) {
            return fail_field(error,
                              error_size,
                              source,
                              "verify.expect",
                              "must be an integer");
        }
        verify->expect = expect->valueint;
    }
    if (json_string(value,
                    "pattern",
                    verify->type == BENCHMARK_VERIFY_REGEX,
                    &verify->pattern,
                    source,
                    error,
                    error_size) != 0 ||
        parse_command(value,
                      "run",
                      &verify->run,
                      &run_present,
                      source,
                      error,
                      error_size) != 0) {
        return -1;
    }
    if (verify->type == BENCHMARK_VERIFY_DUMP_DIFF) {
        int baseline_present = 0;
        cJSON *stream;
        if (parse_command(value,
                          "baseline",
                          &verify->baseline,
                          &baseline_present,
                          source,
                          error,
                          error_size) != 0) {
            return -1;
        }
        if (!baseline_present || !run_present) {
            return fail_field(error,
                              error_size,
                              source,
                              "verify",
                              "dump_diff requires both 'run' and 'baseline'");
        }
        stream = cJSON_GetObjectItemCaseSensitive(value, "stream");
        if (stream != NULL) {
            if (!cJSON_IsString(stream)) {
                return fail_field(error,
                                  error_size,
                                  source,
                                  "verify.stream",
                                  "must be a string");
            }
            if (strcmp(stream->valuestring, "stdout") == 0) {
                verify->dump_stream = BENCHMARK_DUMP_STREAM_STDOUT;
            } else if (strcmp(stream->valuestring, "stderr") == 0) {
                verify->dump_stream = BENCHMARK_DUMP_STREAM_STDERR;
            } else {
                return fail_field(error,
                                  error_size,
                                  source,
                                  "verify.stream",
                                  "must be 'stdout' or 'stderr'");
            }
        }
    }
    return 0;
}

static int parse_generic(cJSON *object,
                         BenchmarkGenericData *generic,
                         const char *source,
                         char *error,
                         size_t error_size)
{
    cJSON *item;
    if ((item = cJSON_GetObjectItemCaseSensitive(object, "preprocessor")) != NULL) {
        generic->has_preprocessor = 1;
        if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
            return fail_field(error,
                              error_size,
                              source,
                              "preprocessor",
                              "must be a non-empty string");
        }
        generic->preprocessor = copy_string(item->valuestring);
        if (generic->preprocessor == NULL) {
            return fail(error, error_size, "%s: out of memory", source);
        }
    }
    if ((item = cJSON_GetObjectItemCaseSensitive(object, "copy")) != NULL) {
        generic->has_copy = 1;
        if (json_strings(object,
                         "copy",
                         1,
                         1,
                         &generic->copy,
                         source,
                         error,
                         error_size) != 0) {
            return -1;
        }
    }
    if ((item = cJSON_GetObjectItemCaseSensitive(object, "cetus_inputs")) != NULL) {
        generic->has_cetus_inputs = 1;
        if (json_strings(object,
                         "cetus_inputs",
                         1,
                         1,
                         &generic->cetus_inputs,
                         source,
                         error,
                         error_size) != 0) {
            return -1;
        }
    }
    if ((item = cJSON_GetObjectItemCaseSensitive(object, "inject_map")) != NULL) {
        generic->has_inject_map = 1;
        if (json_string_map(object,
                            "inject_map",
                            &generic->inject_map,
                            source,
                            error,
                            error_size) != 0) {
            return -1;
        }
    }
    if (parse_command(object,
                      "compile",
                      &generic->compile,
                      &generic->has_compile,
                      source,
                      error,
                      error_size) != 0 ||
        parse_command(object,
                      "run",
                      &generic->run,
                      &generic->has_run,
                      source,
                      error,
                      error_size) != 0 ||
        parse_verify(object,
                     &generic->verify,
                     &generic->has_verify,
                     source,
                     error,
                     error_size) != 0) {
        return -1;
    }
    return 0;
}

static cJSON *read_json(const char *path,
                        char **contents,
                        char *error,
                        size_t error_size)
{
    cJSON *json;
    const char *parse_end = NULL;
    if (runner_read_file(path, contents, NULL) != 0) {
        fail(error, error_size, "%s: cannot read: %s", path, strerror(errno));
        return NULL;
    }
    json = cJSON_ParseWithOpts(*contents, &parse_end, 1);
    if (json == NULL) {
        size_t offset = parse_end == NULL ? 0 : (size_t)(parse_end - *contents);
        fail(error, error_size, "%s: invalid JSON near byte %zu", path, offset);
    }
    return json;
}

static int parse_profiles(const char *path,
                          BenchmarkConfig *config,
                          char *error,
                          size_t error_size)
{
    char *contents = NULL;
    cJSON *root = read_json(path, &contents, error, error_size);
    cJSON *item;
    size_t index = 0;
    if (root == NULL) {
        free(contents);
        return -1;
    }
    if (!cJSON_IsObject(root) || cJSON_GetArraySize(root) == 0) {
        cJSON_Delete(root);
        free(contents);
        return fail_field(
            error, error_size, path, "profiles", "must be a non-empty object");
    }
    config->profile_count = (size_t)cJSON_GetArraySize(root);
    config->profiles = calloc(config->profile_count, sizeof(*config->profiles));
    if (config->profiles == NULL) {
        cJSON_Delete(root);
        free(contents);
        return fail(error, error_size, "%s: out of memory", path);
    }
    cJSON_ArrayForEach(item, root) {
        BenchmarkProfile *profile = &config->profiles[index++];
        if (!cJSON_IsObject(item) || item->string == NULL ||
            item->string[0] == '\0') {
            cJSON_Delete(root);
            free(contents);
            return fail_field(error,
                              error_size,
                              path,
                              "profiles",
                              "entries must be named objects");
        }
        runner_string_vector_init(&profile->flags);
        profile->name = copy_string(item->string);
        if (profile->name == NULL ||
            json_string(item,
                        "description",
                        0,
                        &profile->description,
                        path,
                        error,
                        error_size) != 0 ||
            json_strings(item,
                         "flags",
                         1,
                         1,
                         &profile->flags,
                         path,
                         error,
                         error_size) != 0) {
            cJSON_Delete(root);
            free(contents);
            return -1;
        }
    }
    cJSON_Delete(root);
    free(contents);
    return 0;
}

static int profile_exists(const BenchmarkConfig *config, const char *name)
{
    size_t index;
    for (index = 0; index < config->profile_count; ++index) {
        if (strcmp(config->profiles[index].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int validate_xfail_profiles(const BenchmarkConfig *config,
                                   const RunnerKeyValueMap *xfail_profiles,
                                   const char *source,
                                   char *error,
                                   size_t error_size)
{
    size_t index;
    for (index = 0; index < xfail_profiles->length; ++index) {
        if (!profile_exists(config, xfail_profiles->items[index].key)) {
            return fail(
                error,
                error_size,
                "%s: field 'xfail_profiles' references unknown profile '%s'",
                source,
                xfail_profiles->items[index].key);
        }
    }
    return 0;
}

int benchmark_safe_relative_path(const char *path)
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

static int is_safe_basename(const char *name)
{
    if (name == NULL || name[0] == '\0')
        return 0;
    if (strchr(name, '/') != NULL || strchr(name, '\\') != NULL)
        return 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;
    return 1;
}

static int validate_string_vector_safe(const RunnerStringVector *vec,
                                       const char *field,
                                       const char *source,
                                       char *error,
                                       size_t error_size)
{
    size_t i;
    for (i = 0; i < vec->length; ++i) {
        if (!benchmark_safe_relative_path(vec->items[i]))
            return fail(error, error_size,
                        "%s: '%s' entry '%s' is not a safe relative path",
                        source, field, vec->items[i]);
    }
    return 0;
}

static int validate_generic_command_no_shell(const BenchmarkCommand *cmd,
                                             const char *field,
                                             const char *source,
                                             char *error,
                                             size_t error_size)
{
    if (cmd->type == BENCHMARK_COMMAND_SHELL)
        return fail(error, error_size,
                    "%s: field '%s' must be an argv array, not a shell "
                    "string; generic adapter does not support shell execution",
                    source, field);
    return 0;
}

static int validate_generic_block(const BenchmarkGenericData *generic,
                                  const char *source,
                                  char *error,
                                  size_t error_size)
{
    size_t i;
    if (validate_generic_command_no_shell(&generic->compile, "compile",
                                          source, error, error_size) != 0 ||
        validate_generic_command_no_shell(&generic->run, "run",
                                          source, error, error_size) != 0 ||
        validate_generic_command_no_shell(&generic->verify.run, "verify.run",
                                          source, error, error_size) != 0 ||
        validate_generic_command_no_shell(&generic->verify.baseline,
                                          "verify.baseline",
                                          source, error, error_size) != 0)
        return -1;
    for (i = 0; i < generic->copy.length; ++i) {
        if (!benchmark_safe_relative_path(generic->copy.items[i]))
            return fail(error, error_size,
                        "%s: 'copy' entry '%s' is not a safe relative path "
                        "(no absolute, drive-prefixed, empty, or '..' "
                        "traversal paths allowed)",
                        source, generic->copy.items[i]);
    }
    for (i = 0; i < generic->cetus_inputs.length; ++i) {
        if (!benchmark_safe_relative_path(generic->cetus_inputs.items[i]))
            return fail(error, error_size,
                        "%s: 'cetus_inputs' entry '%s' is not a safe "
                        "relative path",
                        source, generic->cetus_inputs.items[i]);
    }
    for (i = 0; i < generic->inject_map.length; ++i) {
        if (!benchmark_safe_relative_path(
                generic->inject_map.items[i].key))
            return fail(error, error_size,
                        "%s: 'inject_map' key '%s' is not a safe "
                        "relative path",
                        source, generic->inject_map.items[i].key);
        if (!benchmark_safe_relative_path(
                generic->inject_map.items[i].value))
            return fail(error, error_size,
                        "%s: 'inject_map' value '%s' is not a safe "
                        "relative path",
                        source, generic->inject_map.items[i].value);
    }
    return 0;
}

void benchmark_generic_resolve(const BenchmarkSuite *suite,
                               const BenchmarkKernel *kernel,
                               BenchmarkGenericView *view)
{
    const BenchmarkGenericData *base = &suite->generic;
    const BenchmarkGenericData *override = &kernel->generic;
    view->preprocessor = override->has_preprocessor ? override->preprocessor
                                                    : base->preprocessor;
    view->copy = override->has_copy ? &override->copy : &base->copy;
    view->cetus_inputs = override->has_cetus_inputs
                             ? &override->cetus_inputs
                             : &base->cetus_inputs;
    view->inject_map = override->has_inject_map ? &override->inject_map
                                                : &base->inject_map;
    view->compile = override->has_compile ? &override->compile : &base->compile;
    view->run = override->has_run ? &override->run : &base->run;
    view->verify = override->has_verify ? &override->verify : &base->verify;
}

static int parse_kernel(cJSON *object,
                        BenchmarkSuite *suite,
                        BenchmarkKernel *kernel,
                        const char *source,
                        char *error,
                        size_t error_size)
{
    BenchmarkGenericView resolved;
    if (!cJSON_IsObject(object)) {
        return fail_field(
            error, error_size, source, "kernels[]", "must be an object");
    }
    kernel_init(kernel);
    if (json_string(object,
                    "name",
                    1,
                    &kernel->name,
                    source,
                    error,
                    error_size) != 0 ||
        json_string(object,
                    "xfail",
                    0,
                    &kernel->xfail,
                    source,
                    error,
                    error_size) != 0 ||
        json_string(object,
                    "root_env",
                    0,
                    &kernel->root_env,
                    source,
                    error,
                    error_size) != 0 ||
        json_string_map(object,
                        "xfail_profiles",
                        &kernel->xfail_profiles,
                        source,
                        error,
                        error_size) != 0) {
        return -1;
    }
    if (suite->adapter == BENCHMARK_ADAPTER_POLYBENCH) {
        if (json_string(object,
                        "rel_path",
                        1,
                        &kernel->rel_path,
                        source,
                        error,
                        error_size) != 0 ||
            json_string(object,
                        "src_file",
                        0,
                        &kernel->src_file,
                        source,
                        error,
                        error_size) != 0 ||
            json_string(object,
                        "header",
                        0,
                        &kernel->header_file,
                        source,
                        error,
                        error_size) != 0)
            return -1;
        if (!benchmark_safe_relative_path(kernel->rel_path))
            return fail_field(error, error_size, source, "rel_path",
                              "is not a safe relative path");
        if (kernel->src_file != NULL && !is_safe_basename(kernel->src_file))
            return fail_field(error, error_size, source, "src_file",
                              "must be a plain filename (no path separators "
                              "or traversal)");
        if (kernel->header_file != NULL &&
            !is_safe_basename(kernel->header_file))
            return fail_field(error, error_size, source, "header",
                              "must be a plain filename (no path separators "
                              "or traversal)");
        return 0;
    }
    if (suite->adapter == BENCHMARK_ADAPTER_NPB) {
        if (json_string(object,
                        "bench",
                        1,
                        &kernel->bench,
                        source,
                        error,
                        error_size) != 0 ||
            json_string(object,
                        "binary",
                        1,
                        &kernel->binary,
                        source,
                        error,
                        error_size) != 0 ||
            json_strings(object,
                         "headers",
                         0,
                         1,
                         &kernel->headers,
                         source,
                         error,
                         error_size) != 0 ||
            json_strings(object,
                         "sources",
                         1,
                         0,
                         &kernel->sources,
                         source,
                         error,
                         error_size) != 0 ||
            json_strings(object,
                         "cetus_sources",
                         0,
                         1,
                         &kernel->cetus_sources,
                         source,
                         error,
                         error_size) != 0) {
            return -1;
        }
        if (!benchmark_safe_relative_path(kernel->bench))
            return fail_field(error, error_size, source, "bench",
                              "is not a safe relative path");
        if (!benchmark_safe_relative_path(kernel->binary))
            return fail_field(error, error_size, source, "binary",
                              "is not a safe relative path");
        if (validate_string_vector_safe(&kernel->sources, "sources",
                                        source, error, error_size) != 0 ||
            validate_string_vector_safe(&kernel->headers, "headers",
                                        source, error, error_size) != 0 ||
            validate_string_vector_safe(&kernel->cetus_sources,
                                        "cetus_sources",
                                        source, error, error_size) != 0)
            return -1;
        return 0;
    }
    if (parse_generic(object, &kernel->generic, source, error, error_size) != 0 ||
        (kernel->generic.has_inject_map &&
         json_string_map(object,
                         "inject_map",
                         &kernel->inject_map,
                         source,
                         error,
                         error_size) != 0)) {
        return -1;
    }
    if (validate_generic_block(&kernel->generic, source, error,
                               error_size) != 0)
        return -1;
    benchmark_generic_resolve(suite, kernel, &resolved);
    if (resolved.cetus_inputs == NULL || resolved.cetus_inputs->length == 0) {
        return fail_field(
            error, error_size, source, "cetus_inputs", "must not be empty");
    }
    return 0;
}

static int parse_suite(const char *path,
                       BenchmarkConfig *config,
                       BenchmarkSuite *suite,
                       char *error,
                       size_t error_size)
{
    char *contents = NULL;
    cJSON *root = read_json(path, &contents, error, error_size);
    cJSON *adapter;
    cJSON *generic;
    cJSON *kernels;
    cJSON *item;
    size_t index = 0;
    size_t prior;
    if (root == NULL) {
        free(contents);
        return -1;
    }
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        free(contents);
        return fail(error, error_size, "%s: root must be an object", path);
    }
    suite_init(suite);
    if (json_string(root,
                    "id",
                    1,
                    &suite->id,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "adapter",
                    1,
                    &suite->adapter_name,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "description",
                    0,
                    &suite->description,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "root_env",
                    1,
                    &suite->root_env,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "dataset",
                    0,
                    &suite->dataset,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "class",
                    0,
                    &suite->npb_class,
                    path,
                    error,
                    error_size) != 0 ||
        json_string(root,
                    "xfail",
                    0,
                    &suite->xfail,
                    path,
                    error,
                    error_size) != 0 ||
        json_string_map(root,
                        "xfail_profiles",
                        &suite->xfail_profiles,
                        path,
                        error,
                        error_size) != 0 ||
        json_optional_bool(root,
                           "default",
                           &suite->is_default,
                           path,
                           error,
                           error_size) != 0 ||
        json_strings(root,
                     "profiles",
                     1,
                     0,
                     &suite->profiles,
                     path,
                     error,
                     error_size) != 0) {
        cJSON_Delete(root);
        free(contents);
        return -1;
    }
    adapter = cJSON_GetObjectItemCaseSensitive(root, "adapter");
    if (strcmp(adapter->valuestring, "polybench") == 0) {
        suite->adapter = BENCHMARK_ADAPTER_POLYBENCH;
    } else if (strcmp(adapter->valuestring, "npb") == 0) {
        suite->adapter = BENCHMARK_ADAPTER_NPB;
    } else if (strcmp(adapter->valuestring, "generic") == 0) {
        suite->adapter = BENCHMARK_ADAPTER_GENERIC;
    } else {
        cJSON_Delete(root);
        free(contents);
        return fail_field(error,
                          error_size,
                          path,
                          "adapter",
                          "must be polybench, npb, or generic");
    }
    for (index = 0; index < suite->profiles.length; ++index) {
        if (!profile_exists(config, suite->profiles.items[index])) {
            cJSON_Delete(root);
            free(contents);
            return fail(error,
                        error_size,
                        "%s: field 'profiles' references unknown profile '%s'",
                        path,
                        suite->profiles.items[index]);
        }
    }
    if (validate_xfail_profiles(config,
                                &suite->xfail_profiles,
                                path,
                                error,
                                error_size) != 0) {
        cJSON_Delete(root);
        free(contents);
        return -1;
    }
    generic = cJSON_GetObjectItemCaseSensitive(root, "generic");
    if (generic != NULL) {
        if (!cJSON_IsObject(generic)) {
            cJSON_Delete(root);
            free(contents);
            return fail_field(
                error, error_size, path, "generic", "must be an object");
        }
        if (parse_generic(generic,
                          &suite->generic,
                          path,
                          error,
                          error_size) != 0) {
            cJSON_Delete(root);
            free(contents);
            return -1;
        }
        if (suite->adapter == BENCHMARK_ADAPTER_GENERIC &&
            validate_generic_block(&suite->generic, path, error,
                                   error_size) != 0) {
            cJSON_Delete(root);
            free(contents);
            return -1;
        }
    }
    kernels = cJSON_GetObjectItemCaseSensitive(root, "kernels");
    if (kernels == NULL || !cJSON_IsArray(kernels) ||
        cJSON_GetArraySize(kernels) == 0) {
        cJSON_Delete(root);
        free(contents);
        return fail_field(
            error, error_size, path, "kernels", "must be a non-empty array");
    }
    suite->kernel_count = (size_t)cJSON_GetArraySize(kernels);
    suite->kernels = calloc(suite->kernel_count, sizeof(*suite->kernels));
    if (suite->kernels == NULL) {
        cJSON_Delete(root);
        free(contents);
        return fail(error, error_size, "%s: out of memory", path);
    }
    index = 0;
    cJSON_ArrayForEach(item, kernels) {
        if (parse_kernel(item,
                         suite,
                         &suite->kernels[index],
                         path,
                         error,
                         error_size) != 0) {
            cJSON_Delete(root);
            free(contents);
            return -1;
        }
        if (validate_xfail_profiles(config,
                                    &suite->kernels[index].xfail_profiles,
                                    path,
                                    error,
                                    error_size) != 0) {
            cJSON_Delete(root);
            free(contents);
            return -1;
        }
        for (prior = 0; prior < index; ++prior) {
            if (strcmp(suite->kernels[prior].name,
                       suite->kernels[index].name) == 0) {
                cJSON_Delete(root);
                free(contents);
                return fail(error,
                            error_size,
                            "%s: duplicate kernel name '%s' in suite '%s'",
                            path,
                            suite->kernels[index].name,
                            suite->id);
            }
        }
        ++index;
    }
    cJSON_Delete(root);
    free(contents);
    return 0;
}

static int compare_strings(const void *left, const void *right)
{
    const char *const *a = left;
    const char *const *b = right;
    return strcmp(*a, *b);
}

static int has_json_suffix(const char *name)
{
    size_t length = strlen(name);
    return length > 5 && strcmp(name + length - 5, ".json") == 0;
}

static int discover_suites(const char *directory,
                           RunnerStringVector *files,
                           char *error,
                           size_t error_size)
{
    DIR *stream = opendir(directory);
    struct dirent *entry;
    if (stream == NULL) {
        return fail(error,
                    error_size,
                    "%s: cannot open suite directory: %s",
                    directory,
                    strerror(errno));
    }
    while ((entry = readdir(stream)) != NULL) {
        if (entry->d_name[0] != '.' && has_json_suffix(entry->d_name) &&
            runner_string_vector_push(files, entry->d_name) != 0) {
            closedir(stream);
            return fail(error, error_size, "%s: out of memory", directory);
        }
    }
    closedir(stream);
    if (files->length == 0) {
        return fail(error, error_size, "%s: no suite JSON files found", directory);
    }
    qsort(files->items, files->length, sizeof(*files->items), compare_strings);
    return 0;
}

static int load_environment(BenchmarkConfig *config,
                            const char *directory,
                            char *error,
                            size_t error_size)
{
    static const char *known[] = {"CETUS", "CLANG_FORMAT", "GCC", "DIFF", "CPP"};
    char *example = runner_path_join(directory, "paths.example.env");
    char *local = runner_path_join(directory, "paths.env");
    size_t index;
    const char *value;
    if (example == NULL || local == NULL) {
        free(local);
        free(example);
        return fail(error, error_size, "%s: out of memory", directory);
    }
    if (runner_file_exists(example) &&
        runner_parse_env_file(example, &config->paths) != 0) {
        fail(error, error_size, "%s: invalid environment file", example);
        free(local);
        free(example);
        return -1;
    }
    if (runner_file_exists(local) &&
        runner_parse_env_file(local, &config->paths) != 0) {
        fail(error, error_size, "%s: invalid environment file", local);
        free(local);
        free(example);
        return -1;
    }
    for (index = 0; index < sizeof(known) / sizeof(known[0]); ++index) {
        value = getenv(known[index]);
        if (value != NULL &&
            runner_key_value_map_set(&config->paths, known[index], value) != 0) {
            free(local);
            free(example);
            return fail(error, error_size, "%s: out of memory", directory);
        }
    }
    free(local);
    free(example);
    return 0;
}

static int apply_suite_root_environment(BenchmarkConfig *config,
                                        char *error,
                                        size_t error_size)
{
    size_t suite_index;
    size_t kernel_index;
    const char *name;
    const char *value;
    for (suite_index = 0; suite_index < config->suite_count; ++suite_index) {
        BenchmarkSuite *suite = &config->suites[suite_index];
        name = suite->root_env;
        value = getenv(name);
        if (value != NULL &&
            runner_key_value_map_set(&config->paths, name, value) != 0) {
            return fail(error, error_size, "out of memory applying environment");
        }
        for (kernel_index = 0; kernel_index < suite->kernel_count; ++kernel_index) {
            name = suite->kernels[kernel_index].root_env;
            if (name == NULL) {
                continue;
            }
            value = getenv(name);
            if (value != NULL &&
                runner_key_value_map_set(&config->paths, name, value) != 0) {
                return fail(
                    error, error_size, "out of memory applying environment");
            }
        }
    }
    return 0;
}

int benchmark_config_load(const char *config_directory,
                          BenchmarkConfig *config,
                          char *error,
                          size_t error_size)
{
    RunnerStringVector files;
    char *profiles_path = NULL;
    char *suites_path = NULL;
    size_t index;
    size_t prior;
    int result = -1;

    if (error != NULL && error_size != 0) {
        error[0] = '\0';
    }
    benchmark_config_free(config);
    runner_string_vector_init(&files);
    if (load_environment(config, config_directory, error, error_size) != 0) {
        goto cleanup;
    }
    profiles_path = runner_path_join(config_directory, "profiles.json");
    suites_path = runner_path_join(config_directory, "suites");
    if (profiles_path == NULL || suites_path == NULL) {
        fail(error, error_size, "%s: out of memory", config_directory);
        goto cleanup;
    }
    if (parse_profiles(profiles_path, config, error, error_size) != 0 ||
        discover_suites(suites_path, &files, error, error_size) != 0) {
        goto cleanup;
    }
    config->suite_count = files.length;
    config->suites = calloc(config->suite_count, sizeof(*config->suites));
    if (config->suites == NULL) {
        fail(error, error_size, "%s: out of memory", config_directory);
        goto cleanup;
    }
    for (index = 0; index < files.length; ++index) {
        char *path = runner_path_join(suites_path, files.items[index]);
        if (path == NULL ||
            parse_suite(path,
                        config,
                        &config->suites[index],
                        error,
                        error_size) != 0) {
            free(path);
            goto cleanup;
        }
        free(path);
        for (prior = 0; prior < index; ++prior) {
            if (strcmp(config->suites[prior].id, config->suites[index].id) == 0) {
                fail(error,
                     error_size,
                     "%s/%s: duplicate suite id '%s'",
                     suites_path,
                     files.items[index],
                     config->suites[index].id);
                goto cleanup;
            }
        }
    }
    if (apply_suite_root_environment(config, error, error_size) != 0) {
        goto cleanup;
    }
    result = 0;
cleanup:
    runner_string_vector_free(&files);
    free(suites_path);
    free(profiles_path);
    if (result != 0) {
        benchmark_config_free(config);
    }
    return result;
}

static const char *template_value(const BenchmarkTemplateValues *values,
                                  const char *name,
                                  size_t length)
{
#define MATCH(field)                                                            \
    (length == sizeof(#field) - 1 && strncmp(name, #field, length) == 0)       \
        ? values->field                                                         \
        : NULL
    const char *value;
    if ((value = MATCH(root)) != NULL || (value = MATCH(sandbox)) != NULL ||
        (value = MATCH(cpp)) != NULL || (value = MATCH(dataset)) != NULL ||
        (value = MATCH(gcc)) != NULL || (value = MATCH(name)) != NULL ||
        (value = MATCH(work)) != NULL) {
        return value;
    }
    return NULL;
#undef MATCH
}

int benchmark_expand_template(const char *input,
                              const BenchmarkTemplateValues *values,
                              char **output,
                              char *error,
                              size_t error_size)
{
    RunnerString result;
    const char *cursor = input;
    *output = NULL;
    runner_string_init(&result);
    while (*cursor != '\0') {
        const char *close;
        const char *value;
        if (cursor[0] == '{' && cursor[1] == '{') {
            if (runner_string_append_n(&result, "{", 1) != 0) {
                goto memory_error;
            }
            cursor += 2;
            continue;
        }
        if (cursor[0] == '}' && cursor[1] == '}') {
            if (runner_string_append_n(&result, "}", 1) != 0) {
                goto memory_error;
            }
            cursor += 2;
            continue;
        }
        if (*cursor == '}') {
            runner_string_free(&result);
            return fail(error,
                        error_size,
                        "invalid template: unmatched '}' near '%.24s'",
                        cursor);
        }
        if (*cursor != '{') {
            if (runner_string_append_n(&result, cursor++, 1) != 0) {
                goto memory_error;
            }
            continue;
        }
        close = strchr(cursor + 1, '}');
        if (close == NULL || close == cursor + 1) {
            runner_string_free(&result);
            return fail(error,
                        error_size,
                        "invalid template placeholder near '%.24s'",
                        cursor);
        }
        value = template_value(
            values, cursor + 1, (size_t)(close - cursor - 1));
        if (value == NULL) {
            int status = fail(error,
                              error_size,
                              "unknown placeholder '%.*s'",
                              (int)(close - cursor - 1),
                              cursor + 1);
            runner_string_free(&result);
            return status;
        }
        if (runner_string_append(&result, value) != 0) {
            goto memory_error;
        }
        cursor = close + 1;
    }
    *output = runner_string_take(&result);
    if (*output == NULL) {
        goto memory_error;
    }
    return 0;
memory_error:
    runner_string_free(&result);
    return fail(error, error_size, "out of memory expanding template");
}
