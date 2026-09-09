#define _POSIX_C_SOURCE 200809L

#include "benchmark_runner.h"
#include "runner_common.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char *copy_string(const char *value)
{
    size_t length = strlen(value) + 1;
    char *copy = malloc(length);
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

static int default_process_run(const ProcessSpec *spec,
                               ProcessResult *result,
                               void *context)
{
    (void)context;
    return process_run(spec, result);
}

void benchmark_runner_options_init(BenchmarkRunnerOptions *options)
{
    memset(options, 0, sizeof(*options));
    options->project_root = ".";
    options->timeout_seconds = 600;
    options->process_run = default_process_run;
}

int benchmark_runner_parse_timeout(const char *text,
                                   int *timeout_seconds,
                                   char *error,
                                   size_t error_size)
{
    char *end;
    long value;
    if (text == NULL || timeout_seconds == NULL || text[0] == '\0') {
        return fail(error, error_size, "timeout must be a positive integer");
    }
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value <= 0 || value > INT_MAX) {
        return fail(error, error_size, "timeout must be a positive integer");
    }
    *timeout_seconds = (int)value;
    return 0;
}

void benchmark_case_result_free(BenchmarkCaseResult *result)
{
    free(result->message);
    free(result->log_path);
    memset(result, 0, sizeof(*result));
}

void benchmark_selection_init(BenchmarkSelection *selection)
{
    memset(selection, 0, sizeof(*selection));
}

void benchmark_selection_free(BenchmarkSelection *selection)
{
    size_t index;
    for (index = 0; index < selection->length; ++index) {
        benchmark_case_result_free(&selection->items[index].result);
    }
    free(selection->items);
    benchmark_selection_init(selection);
}

static const BenchmarkProfile *find_profile(const BenchmarkConfig *config,
                                            const char *name)
{
    size_t index;
    for (index = 0; index < config->profile_count; ++index) {
        if (strcmp(config->profiles[index].name, name) == 0) {
            return &config->profiles[index];
        }
    }
    return NULL;
}

static int suite_requested(const RunnerStringVector *suite_ids, const char *id)
{
    size_t index;
    if (suite_ids == NULL || suite_ids->length == 0) {
        return 0;
    }
    for (index = 0; index < suite_ids->length; ++index) {
        if (strcmp(suite_ids->items[index], id) == 0) {
            return 1;
        }
    }
    return 0;
}

static int selection_push(BenchmarkSelection *selection,
                          const BenchmarkSuite *suite,
                          const BenchmarkKernel *kernel,
                          const BenchmarkProfile *profile)
{
    BenchmarkSelectionItem *items;
    size_t capacity;
    if (selection->length == selection->capacity) {
        capacity = selection->capacity == 0 ? 16 : selection->capacity * 2;
        if (capacity < selection->capacity ||
            capacity > SIZE_MAX / sizeof(*selection->items)) {
            errno = ENOMEM;
            return -1;
        }
        items = realloc(selection->items, capacity * sizeof(*items));
        if (items == NULL) {
            return -1;
        }
        selection->items = items;
        selection->capacity = capacity;
    }
    memset(&selection->items[selection->length], 0,
           sizeof(selection->items[selection->length]));
    selection->items[selection->length].suite = suite;
    selection->items[selection->length].kernel = kernel;
    selection->items[selection->length].profile = profile;
    ++selection->length;
    return 0;
}

int benchmark_runner_select(const BenchmarkConfig *config,
                            const RunnerStringVector *suite_ids,
                            const char *kernel_filter,
                            const char *profile_filter,
                            BenchmarkSelection *selection,
                            char *error,
                            size_t error_size)
{
    size_t suite_index;
    size_t kernel_index;
    size_t profile_index;

    if (config == NULL || selection == NULL) {
        return fail(error, error_size, "invalid selection arguments");
    }
    benchmark_selection_free(selection);
    for (suite_index = 0; suite_index < config->suite_count; ++suite_index) {
        const BenchmarkSuite *suite = &config->suites[suite_index];
        int selected = suite_ids != NULL && suite_ids->length != 0
                           ? suite_requested(suite_ids, suite->id)
                           : suite->is_default;
        if (!selected) {
            continue;
        }
        for (kernel_index = 0; kernel_index < suite->kernel_count;
             ++kernel_index) {
            const BenchmarkKernel *kernel = &suite->kernels[kernel_index];
            if (kernel_filter != NULL &&
                strstr(kernel->name, kernel_filter) == NULL) {
                continue;
            }
            for (profile_index = 0; profile_index < suite->profiles.length;
                 ++profile_index) {
                const char *name = suite->profiles.items[profile_index];
                const BenchmarkProfile *profile;
                if (profile_filter != NULL &&
                    strcmp(profile_filter, name) != 0) {
                    continue;
                }
                profile = find_profile(config, name);
                if (profile == NULL ||
                    selection_push(selection, suite, kernel, profile) != 0) {
                    benchmark_selection_free(selection);
                    return fail(error, error_size,
                                profile == NULL ? "unknown profile '%s'"
                                                : "out of memory selecting cases",
                                name);
                }
            }
        }
    }
    if (suite_ids != NULL) {
        for (suite_index = 0; suite_index < suite_ids->length; ++suite_index) {
            size_t known;
            for (known = 0; known < config->suite_count; ++known) {
                if (strcmp(suite_ids->items[suite_index],
                           config->suites[known].id) == 0) {
                    break;
                }
            }
            if (known == config->suite_count) {
                benchmark_selection_free(selection);
                return fail(error, error_size, "unknown suite '%s'",
                            suite_ids->items[suite_index]);
            }
        }
    }
    return 0;
}

static char *join_four(const char *a,
                       const char *b,
                       const char *c,
                       const char *d)
{
    char *ab = runner_path_join(a, b);
    char *abc = ab == NULL ? NULL : runner_path_join(ab, c);
    char *result = abc == NULL ? NULL : runner_path_join(abc, d);
    free(abc);
    free(ab);
    return result;
}

static int safe_component(const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;
    if (value == NULL || value[0] == '\0' || strcmp(value, ".") == 0 ||
        strcmp(value, "..") == 0) {
        return 0;
    }
    for (; *cursor != '\0'; ++cursor) {
        if (!(('a' <= *cursor && *cursor <= 'z') ||
              ('A' <= *cursor && *cursor <= 'Z') ||
              ('0' <= *cursor && *cursor <= '9') || *cursor == '-' ||
              *cursor == '_' || *cursor == '.')) {
            return 0;
        }
    }
    return strstr(value, "..") == NULL;
}

static int validate_case_identifiers(const BenchmarkCase *test_case,
                                     char *error,
                                     size_t error_size)
{
    if (test_case->suite == NULL || test_case->kernel == NULL ||
        test_case->profile == NULL ||
        !safe_component(test_case->suite->id) ||
        !safe_component(test_case->profile->name) ||
        !safe_component(test_case->kernel->name)) {
        return fail(error, error_size,
                    "unsafe suite, profile, or kernel identifier");
    }
    return 0;
}

static int ensure_parent(const char *path)
{
    char *copy = copy_string(path);
    char *slash;
    int result;
    if (copy == NULL) {
        return -1;
    }
    slash = strrchr(copy, '/');
    if (slash == NULL) {
        free(copy);
        return 0;
    }
    *slash = '\0';
    result = runner_mkdir_recursive(copy, 0755);
    free(copy);
    return result;
}

static const char *xfail_reason(const BenchmarkCase *test_case)
{
    const char *reason =
        runner_key_value_map_get(&test_case->kernel->xfail_profiles,
                                 test_case->profile->name);
    if (reason != NULL) {
        return reason;
    }
    if (test_case->kernel->xfail != NULL) {
        return test_case->kernel->xfail;
    }
    reason = runner_key_value_map_get(&test_case->suite->xfail_profiles,
                                      test_case->profile->name);
    return reason != NULL ? reason : test_case->suite->xfail;
}

static int create_log_paths(const char *project_root,
                            const BenchmarkCase *test_case,
                            char **unique_path,
                            char **all_path,
                            char **failed_path)
{
    static unsigned long sequence;
    struct timespec now;
    char name[256];
    char *logs = runner_path_join(project_root, "logs/benchmarks");
    unsigned int attempt;
    int descriptor = -1;
    FILE *file;
    if (logs == NULL || runner_mkdir_recursive(logs, 0755) != 0 ||
        clock_gettime(CLOCK_REALTIME, &now) != 0) {
        free(logs);
        return -1;
    }
    for (attempt = 0; attempt < 100; ++attempt) {
        ++sequence;
        snprintf(name, sizeof(name), "run_%ld_%ld_%lu_%s_%s_%s.log",
                 (long)now.tv_sec, (long)getpid(), sequence,
                 test_case->suite->id, test_case->profile->name,
                 test_case->kernel->name);
        *unique_path = runner_path_join(logs, name);
        if (*unique_path == NULL) {
            break;
        }
        descriptor = open(*unique_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (descriptor >= 0) {
            if (close(descriptor) != 0) {
                unlink(*unique_path);
                free(*unique_path);
                *unique_path = NULL;
            }
            break;
        }
        free(*unique_path);
        *unique_path = NULL;
        if (errno != EEXIST) {
            break;
        }
    }
    *all_path = runner_path_join(logs, "all_tests.log");
    *failed_path = runner_path_join(logs, "failed_tests.log");
    free(logs);
    if (*unique_path == NULL || *all_path == NULL || *failed_path == NULL) {
        free(*unique_path);
        free(*all_path);
        free(*failed_path);
        *unique_path = NULL;
        *all_path = NULL;
        *failed_path = NULL;
        return -1;
    }
    file = fopen(*all_path, "a");
    if (file == NULL) {
        return -1;
    }
    if (fflush(file) != 0) {
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        return -1;
    }
    file = fopen(*failed_path, "a");
    if (file == NULL) {
        return -1;
    }
    if (fflush(file) != 0) {
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        return -1;
    }
    return 0;
}

static int append_log(const char *path, const char *line)
{
    FILE *file = fopen(path, "a");
    int result;
    if (file == NULL) {
        return -1;
    }
    result = fprintf(file, "%s\n", line) >= 0 && fflush(file) == 0 ? 0 : -1;
    if (fclose(file) != 0) {
        result = -1;
    }
    return result;
}

static void set_result(BenchmarkCaseResult *result,
                       BenchmarkStatus status,
                       BenchmarkStage stage,
                       const char *message,
                       const char *xfail,
                       int maskable)
{
    RunnerString combined;
    result->status = status;
    result->stage = stage;
    if (xfail != NULL && maskable && status == BENCHMARK_STATUS_FAIL) {
        result->status = BENCHMARK_STATUS_XFAIL;
    } else if (xfail != NULL && stage == BENCHMARK_STAGE_DONE &&
               status == BENCHMARK_STATUS_PASS) {
        result->status = BENCHMARK_STATUS_XPASS;
    }
    runner_string_init(&combined);
    if (result->status == BENCHMARK_STATUS_XFAIL) {
        runner_string_append(&combined, xfail);
        if (message != NULL && message[0] != '\0') {
            runner_string_append(&combined, " | ");
        }
    } else if (result->status == BENCHMARK_STATUS_XPASS) {
        runner_string_append(&combined, "unexpected pass (marked xfail: ");
        runner_string_append(&combined, xfail);
        runner_string_append(&combined, ")");
        message = NULL;
    }
    if (message != NULL) {
        runner_string_append(&combined, message);
    }
    result->message = runner_string_take(&combined);
}

static int run_tool(const BenchmarkRunnerOptions *options,
                    const char *log_path,
                    const char *label,
                    const char *const *argv,
                    const char *cwd,
                    ProcessResult *result)
{
    ProcessSpec spec;
    int rc;
    memset(&spec, 0, sizeof(spec));
    memset(result, 0, sizeof(*result));
    spec.argv = argv;
    spec.cwd = cwd;
    spec.timeout_seconds = (unsigned int)options->timeout_seconds;
    rc = options->process_run(&spec, result, options->process_context);
    benchmark_log_process(log_path, label, argv, rc == 0 ? result : NULL);
    return rc;
}

static int format_file(const BenchmarkConfig *config,
                       const BenchmarkRunnerOptions *options,
                       const char *log_path,
                       const char *path,
                       char *message,
                       size_t message_size)
{
    const char *formatter = benchmark_config_path(config, "CLANG_FORMAT");
    const char *argv[4];
    ProcessResult process;
    int run_result;
    if (formatter == NULL || formatter[0] == '\0') {
        formatter = "clang-format";
    }
    argv[0] = formatter;
    argv[1] = "-i";
    argv[2] = path;
    argv[3] = NULL;
    run_result = run_tool(options, log_path, "clang-format", argv, NULL,
                          &process);
    if (run_result != 0) {
        snprintf(message, message_size, "cannot execute clang-format");
        return -1;
    }
    if (process.timed_out || !process.exited || process.exit_code != 0) {
        snprintf(message, message_size, "clang-format failed for %s",
                 runner_path_basename(path));
        process_result_free(&process);
        return -1;
    }
    process_result_free(&process);
    return 0;
}

static int compare_outputs(const BenchmarkConfig *config,
                           const BenchmarkRunnerOptions *options,
                           const BenchmarkCaseContext *context,
                           const RunnerStringVector *outputs,
                           char *message,
                           size_t message_size,
                           int *mismatch)
{
    const char *diff = benchmark_config_path(config, "DIFF");
    char *temporary_dir = runner_path_join(context->work_dir, ".formatted_gt");
    size_t index;
    *mismatch = 0;
    if (temporary_dir == NULL ||
        runner_mkdir_recursive(temporary_dir, 0755) != 0) {
        free(temporary_dir);
        snprintf(message, message_size, "cannot create temporary GT directory");
        return -1;
    }
    if (diff == NULL || diff[0] == '\0') {
        diff = "diff";
    }
    for (index = 0; index < outputs->length; ++index) {
        const char *name = runner_path_basename(outputs->items[index]);
        char *expected = runner_path_join(context->ground_truth_dir, name);
        char *temporary = runner_path_join(temporary_dir, name);
        const char *argv[5];
        ProcessResult process;
        int run_result;
        if (expected == NULL || temporary == NULL ||
            !runner_file_exists(expected)) {
            snprintf(message, message_size, "missing ground truth: %s",
                     expected != NULL ? expected : name);
            free(temporary);
            free(expected);
            free(temporary_dir);
            return -1;
        }
        if (runner_copy_file(expected, temporary) != 0 ||
            format_file(config, options, context->log_path, temporary, message,
                        message_size) != 0) {
            free(temporary);
            free(expected);
            free(temporary_dir);
            return -1;
        }
        argv[0] = diff;
        argv[1] = "-wB";
        argv[2] = outputs->items[index];
        argv[3] = temporary;
        argv[4] = NULL;
        run_result = run_tool(options, context->log_path, "diff", argv, NULL,
                              &process);
        if (run_result != 0) {
            snprintf(message, message_size, "cannot execute diff tool");
            free(temporary);
            free(expected);
            free(temporary_dir);
            return -1;
        }
        if (process.timed_out || !process.exited || process.exit_code > 1) {
            snprintf(message, message_size, "diff tool failed for %s", name);
            process_result_free(&process);
            free(temporary);
            free(expected);
            free(temporary_dir);
            return -1;
        }
        if (process.exit_code == 1) {
            snprintf(message, message_size, "ground-truth mismatch: %s", name);
            *mismatch = 1;
            process_result_free(&process);
            free(temporary);
            free(expected);
            free(temporary_dir);
            return 0;
        }
        process_result_free(&process);
        free(temporary);
        free(expected);
    }
    free(temporary_dir);
    return 0;
}

static int validate_expected_names(const RunnerStringVector *names,
                                   char *message,
                                   size_t message_size)
{
    size_t index;
    if (names->length == 0) {
        snprintf(message, message_size, "adapter expected output set is empty");
        return -1;
    }
    for (index = 0; index < names->length; ++index) {
        size_t prior;
        if (!safe_component(names->items[index])) {
            snprintf(message, message_size, "unsafe expected output name: %s",
                     names->items[index]);
            return -1;
        }
        for (prior = 0; prior < index; ++prior) {
            if (strcmp(names->items[prior], names->items[index]) == 0) {
                snprintf(message, message_size,
                         "duplicate expected output: %s", names->items[index]);
                return -1;
            }
        }
    }
    return 0;
}

static int missing_ground_truth(const RunnerStringVector *names,
                                const char *ground_truth_dir,
                                char *message,
                                size_t message_size)
{
    size_t index;
    size_t missing = 0;
    const char *first = NULL;
    for (index = 0; index < names->length; ++index) {
        char *path = runner_path_join(ground_truth_dir, names->items[index]);
        if (path == NULL || !runner_file_exists(path)) {
            ++missing;
            if (first == NULL) {
                first = names->items[index];
            }
        }
        free(path);
    }
    if (missing != 0) {
        snprintf(message, message_size,
                 missing == names->length
                     ? "missing all ground-truth files (first: %s)"
                     : "missing ground truth: %s (%zu of %zu missing)",
                 first, missing, names->length);
        return 1;
    }
    return 0;
}

static int validate_and_order_outputs(const RunnerStringVector *expected,
                                      RunnerStringVector *outputs,
                                      char *message,
                                      size_t message_size)
{
    RunnerStringVector ordered;
    size_t output_index;
    size_t expected_index;
    runner_string_vector_init(&ordered);
    for (output_index = 0; output_index < outputs->length; ++output_index) {
        const char *name = runner_path_basename(outputs->items[output_index]);
        size_t prior;
        int expected_match = 0;
        if (!safe_component(name)) {
            snprintf(message, message_size, "unsafe adapter output name: %s",
                     name);
            goto failure;
        }
        for (prior = 0; prior < output_index; ++prior) {
            if (strcmp(name,
                       runner_path_basename(outputs->items[prior])) == 0) {
                snprintf(message, message_size,
                         strcmp(outputs->items[output_index],
                                outputs->items[prior]) == 0
                             ? "duplicate adapter output: %s"
                             : "adapter output basename collision: %s",
                         name);
                goto failure;
            }
        }
        for (expected_index = 0; expected_index < expected->length;
             ++expected_index) {
            if (strcmp(name, expected->items[expected_index]) == 0) {
                expected_match = 1;
                break;
            }
        }
        if (!expected_match) {
            snprintf(message, message_size, "unexpected adapter output: %s",
                     name);
            goto failure;
        }
    }
    for (expected_index = 0; expected_index < expected->length;
         ++expected_index) {
        const char *match = NULL;
        for (output_index = 0; output_index < outputs->length; ++output_index) {
            if (strcmp(expected->items[expected_index],
                       runner_path_basename(outputs->items[output_index])) == 0) {
                match = outputs->items[output_index];
                break;
            }
        }
        if (match == NULL) {
            snprintf(message, message_size, "missing adapter output: %s",
                     expected->items[expected_index]);
            goto failure;
        }
        if (runner_string_vector_push(&ordered, match) != 0) {
            snprintf(message, message_size, "out of memory ordering outputs");
            goto failure;
        }
    }
    runner_string_vector_free(outputs);
    *outputs = ordered;
    return 0;
failure:
    runner_string_vector_free(&ordered);
    return -1;
}

static int generate_outputs(const RunnerStringVector *outputs,
                            const char *ground_truth_dir,
                            char *message,
                            size_t message_size)
{
    size_t index;
    if (runner_mkdir_recursive(ground_truth_dir, 0755) != 0) {
        snprintf(message, message_size, "cannot create ground-truth directory");
        return -1;
    }
    for (index = 0; index < outputs->length; ++index) {
        char *destination =
            runner_path_join(ground_truth_dir,
                             runner_path_basename(outputs->items[index]));
        if (destination == NULL || ensure_parent(destination) != 0 ||
            runner_copy_file(outputs->items[index], destination) != 0) {
            snprintf(message, message_size, "cannot generate ground truth");
            free(destination);
            return -1;
        }
        free(destination);
    }
    return 0;
}

static BenchmarkStatus outcome_status(BenchmarkAdapterOutcome outcome)
{
    return outcome == BENCHMARK_ADAPTER_SKIP ? BENCHMARK_STATUS_SKIP
                                             : BENCHMARK_STATUS_FAIL;
}

int benchmark_runner_run_case(const BenchmarkConfig *config,
                              const BenchmarkCase *test_case,
                              const BenchmarkAdapterInstance *adapter,
                              const BenchmarkRunnerOptions *options,
                              BenchmarkCaseResult *result,
                              char *error,
                              size_t error_size)
{
    BenchmarkCaseContext context;
    RunnerStringVector expected_names;
    RunnerStringVector outputs;
    const RunnerStringVector *flags;
    BenchmarkAdapterOutcome adapter_result;
    const char *root;
    const char *cetus;
    const char *xfail;
    char *work_base = NULL;
    char *work_suite = NULL;
    char *work_profile = NULL;
    char *ground_base = NULL;
    char *all_log = NULL;
    char *failed_log = NULL;
    char message[2048] = "";
    int mismatch = 0;
    int final = 0;

    memset(&context, 0, sizeof(context));
    if (error != NULL && error_size != 0) {
        error[0] = '\0';
    }
    if (config == NULL || test_case == NULL || adapter == NULL ||
        adapter->vtable == NULL || options == NULL || result == NULL ||
        options->process_run == NULL || options->project_root == NULL) {
        return fail(error, error_size, "invalid benchmark runner arguments");
    }
    if (options->timeout_seconds <= 0) {
        return fail(error, error_size, "timeout must be greater than zero");
    }
    if (validate_case_identifiers(test_case, error, error_size) != 0) {
        return -1;
    }
    memset(result, 0, sizeof(*result));
    runner_string_vector_init(&expected_names);
    runner_string_vector_init(&outputs);
    work_base = runner_path_join(options->project_root, "work/benchmarks");
    work_suite = work_base == NULL ? NULL
                                  : runner_path_join(work_base,
                                                     test_case->suite->id);
    work_profile = work_suite == NULL
                       ? NULL
                       : runner_path_join(work_suite, test_case->profile->name);
    context.work_dir = work_profile == NULL
                           ? NULL
                           : runner_path_join(work_profile,
                                              test_case->kernel->name);
    ground_base =
        runner_path_join(options->project_root, "ground_truth/benchmarks");
    context.ground_truth_dir =
        ground_base == NULL
            ? NULL
            : join_four(ground_base, test_case->suite->id,
                        test_case->profile->name, test_case->kernel->name);
    if (context.work_dir == NULL || context.ground_truth_dir == NULL ||
        create_log_paths(options->project_root, test_case, &result->log_path,
                         &all_log, &failed_log) != 0) {
        fail(error, error_size,
             "cannot initialize required benchmark logs or case paths");
        goto cleanup;
    }
    context.config = config;
    context.test_case = test_case;
    context.project_root = options->project_root;
    context.log_path = result->log_path;
    context.timeout_seconds = options->timeout_seconds;
    context.process_run = options->process_run;
    context.process_context = options->process_context;
    cetus = benchmark_config_path(config, "CETUS");
    context.cetus = cetus;
    xfail = xfail_reason(test_case);
    if (runner_remove_recursive(context.work_dir) != 0 ||
        runner_mkdir_recursive(context.work_dir, 0755) != 0) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_SETUP,
                   "cannot reset work directory", xfail, 0);
        goto finish;
    }
    root = benchmark_config_path(config,
                                 test_case->kernel->root_env != NULL
                                     ? test_case->kernel->root_env
                                     : test_case->suite->root_env);
    if (root == NULL || root[0] == '\0' || !runner_file_exists(root)) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_SETUP,
                   "benchmark root is not configured or does not exist",
                   xfail, 0);
        goto finish;
    }
    if (cetus == NULL || cetus[0] == '\0' || !runner_file_exists(cetus)) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_SETUP,
                   "CETUS is not configured or does not exist", xfail, 0);
        goto finish;
    }
    if (adapter->vtable->expected_gt_names == NULL ||
        adapter->vtable->expected_gt_names(adapter->context, &context,
                                            &expected_names, message,
                                            sizeof(message)) != 0) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_SETUP,
                   message[0] ? message : "cannot determine expected outputs",
                   xfail, 0);
        goto finish;
    }
    if (validate_expected_names(&expected_names, message, sizeof(message)) != 0) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_SETUP,
                   message, xfail, 0);
        goto finish;
    }
    if (!options->generate && !options->skip_ground_truth &&
        xfail == NULL &&
        missing_ground_truth(&expected_names, context.ground_truth_dir,
                             message, sizeof(message))) {
        set_result(result, BENCHMARK_STATUS_FAIL,
                   BENCHMARK_STAGE_GROUND_TRUTH,
                   message, xfail, 0);
        goto finish;
    }
    adapter_result = BENCHMARK_ADAPTER_INFRA_FAILURE;
    if (adapter->vtable->prepare == NULL ||
        (adapter_result = adapter->vtable->prepare(
             adapter->context, &context, message, sizeof(message))) !=
            BENCHMARK_ADAPTER_OK) {
        set_result(result, outcome_status(adapter_result),
                   BENCHMARK_STAGE_PREPARE,
                   message[0] ? message : "adapter prepare failed", xfail, 0);
        goto finish;
    }
    flags = options->override_flags != NULL ? options->override_flags
                                            : &test_case->profile->flags;
    adapter_result = BENCHMARK_ADAPTER_INFRA_FAILURE;
    if (adapter->vtable->cetus_transform == NULL ||
        (adapter_result = adapter->vtable->cetus_transform(
             adapter->context, &context, flags, &outputs, message,
             sizeof(message))) != BENCHMARK_ADAPTER_OK) {
        set_result(result, outcome_status(adapter_result), BENCHMARK_STAGE_CETUS,
                   message[0] ? message : "Cetus transform failed", xfail,
                   adapter_result == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
        goto finish;
    }
    if (validate_and_order_outputs(&expected_names, &outputs, message,
                                   sizeof(message)) != 0) {
        set_result(result, BENCHMARK_STATUS_FAIL, BENCHMARK_STAGE_CETUS,
                   message, xfail, 0);
        goto finish;
    }
    {
        size_t index;
        for (index = 0; index < outputs.length; ++index) {
            if (format_file(config, options, context.log_path,
                            outputs.items[index], message,
                            sizeof(message)) != 0) {
                set_result(result, BENCHMARK_STATUS_FAIL,
                           BENCHMARK_STAGE_FORMAT, message, xfail, 0);
                goto finish;
            }
        }
    }
    if (options->generate) {
        if (generate_outputs(&outputs, context.ground_truth_dir, message,
                             sizeof(message)) != 0) {
            set_result(result, BENCHMARK_STATUS_FAIL,
                       BENCHMARK_STAGE_GROUND_TRUTH, message, xfail, 0);
            goto finish;
        }
    } else if (!options->skip_ground_truth) {
        if (compare_outputs(config, options, &context, &outputs, message,
                            sizeof(message), &mismatch) != 0) {
            set_result(result, BENCHMARK_STATUS_FAIL,
                       BENCHMARK_STAGE_GROUND_TRUTH, message, xfail, 0);
            goto finish;
        }
        if (mismatch) {
            set_result(result, BENCHMARK_STATUS_FAIL,
                       BENCHMARK_STAGE_GROUND_TRUTH, message, xfail, 1);
            goto finish;
        }
    }
    if (options->skip_run) {
        set_result(result, BENCHMARK_STATUS_PASS,
                   BENCHMARK_STAGE_GROUND_TRUTH, "", xfail, 0);
        goto finish;
    }
    adapter_result = BENCHMARK_ADAPTER_INFRA_FAILURE;
    if (adapter->vtable->inject == NULL ||
        (adapter_result = adapter->vtable->inject(
             adapter->context, &context, &outputs, message,
             sizeof(message))) != BENCHMARK_ADAPTER_OK) {
        set_result(result, outcome_status(adapter_result), BENCHMARK_STAGE_INJECT,
                   message[0] ? message : "adapter injection failed", xfail, 0);
        goto finish;
    }
    adapter_result = BENCHMARK_ADAPTER_INFRA_FAILURE;
    if (adapter->vtable->build == NULL ||
        (adapter_result = adapter->vtable->build(
             adapter->context, &context, message, sizeof(message))) !=
            BENCHMARK_ADAPTER_OK) {
        set_result(result, outcome_status(adapter_result), BENCHMARK_STAGE_BUILD,
                   message[0] ? message : "build failed", xfail,
                   adapter_result == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
        goto finish;
    }
    adapter_result = BENCHMARK_ADAPTER_INFRA_FAILURE;
    if (adapter->vtable->verify == NULL ||
        (adapter_result = adapter->vtable->verify(
             adapter->context, &context, message, sizeof(message))) !=
            BENCHMARK_ADAPTER_OK) {
        set_result(result, outcome_status(adapter_result),
                   BENCHMARK_STAGE_VERIFY,
                   message[0] ? message : "verification failed", xfail,
                   adapter_result == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
        goto finish;
    }
    set_result(result, BENCHMARK_STATUS_PASS, BENCHMARK_STAGE_DONE,
               message[0] ? message : "", xfail, 0);

finish:
    if (adapter->vtable->cleanup != NULL) {
        adapter->vtable->cleanup(adapter->context, &context);
    }
    {
        char line[4096];
        snprintf(line, sizeof(line), "[%s] %s/%s profile=%s (%s: %s)",
                 benchmark_status_name(result->status), test_case->suite->id,
                 test_case->kernel->name, test_case->profile->name,
                 benchmark_stage_name(result->stage),
                 result->message != NULL ? result->message : "");
        int log_failed = append_log(result->log_path, line) != 0 ||
                         append_log(all_log, line) != 0;
        if (result->status == BENCHMARK_STATUS_FAIL ||
            result->status == BENCHMARK_STATUS_XPASS) {
            log_failed |= append_log(failed_log, line) != 0;
        }
        if (log_failed) {
            free(result->message);
            result->message = copy_string("required benchmark log write failed");
            result->status = BENCHMARK_STATUS_FAIL;
            result->stage = BENCHMARK_STAGE_SETUP;
        }
    }
    final = 1;
cleanup:
    if (!final) {
        benchmark_case_result_free(result);
    }
    runner_string_vector_free(&outputs);
    runner_string_vector_free(&expected_names);
    free(failed_log);
    free(all_log);
    free((char *)context.ground_truth_dir);
    free(ground_base);
    free((char *)context.work_dir);
    free(work_profile);
    free(work_suite);
    free(work_base);
    return final ? 0 : -1;
}

void benchmark_summary_add(BenchmarkSummary *summary,
                           const BenchmarkCaseResult *result)
{
    ++summary->total;
    switch (result->status) {
    case BENCHMARK_STATUS_PASS:
        ++summary->pass;
        break;
    case BENCHMARK_STATUS_FAIL:
        ++summary->fail;
        break;
    case BENCHMARK_STATUS_XFAIL:
        ++summary->xfail;
        break;
    case BENCHMARK_STATUS_XPASS:
        ++summary->xpass;
        break;
    case BENCHMARK_STATUS_SKIP:
        ++summary->skip;
        break;
    }
}

int benchmark_summary_exit_code(const BenchmarkSummary *summary)
{
    return summary->fail == 0 && summary->xpass == 0 ? 0 : 1;
}

const char *benchmark_status_name(BenchmarkStatus status)
{
    static const char *names[] = {"PASS", "FAIL", "XFAIL", "XPASS", "SKIP"};
    return status >= BENCHMARK_STATUS_PASS && status <= BENCHMARK_STATUS_SKIP
               ? names[status]
               : "UNKNOWN";
}

const char *benchmark_stage_name(BenchmarkStage stage)
{
    static const char *names[] = {
        "selection", "setup", "ground_truth", "prepare", "cetus",
        "format", "inject", "build", "verify", "done"};
    return stage >= BENCHMARK_STAGE_SELECTION && stage <= BENCHMARK_STAGE_DONE
               ? names[stage]
               : "unknown";
}

void benchmark_log_process(const char *log_path,
                           const char *label,
                           const char *const *argv,
                           const ProcessResult *result)
{
    FILE *file;
    size_t i;
    if (log_path == NULL)
        return;
    file = fopen(log_path, "a");
    if (file == NULL)
        return;
    fprintf(file, "--- %s ---\ncommand:", label);
    for (i = 0; argv[i] != NULL; ++i)
        fprintf(file, " %s", argv[i]);
    fprintf(file, "\n");
    if (result == NULL) {
        fprintf(file, "(process could not be started)\n");
    } else {
        if (result->timed_out)
            fprintf(file, "TIMED OUT\n");
        if (result->signaled)
            fprintf(file, "signal=%d\n", result->term_signal);
        if (result->exited)
            fprintf(file, "exit_code=%d\n", result->exit_code);
        if (result->stderr_data != NULL && result->stderr_data[0] != '\0')
            fprintf(file, "stderr:\n%s\n", result->stderr_data);
        if (result->stdout_data != NULL && result->stdout_data[0] != '\0')
            fprintf(file, "stdout:\n%s\n", result->stdout_data);
    }
    fclose(file);
}
