#define _POSIX_C_SOURCE 200809L

#include "benchmark_runner.h"
#include "runner_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

typedef struct {
    int prepare_calls;
    int transform_calls;
    int inject_calls;
    int build_calls;
    int verify_calls;
    int cleanup_calls;
    BenchmarkAdapterOutcome transform_outcome;
    BenchmarkAdapterOutcome prepare_outcome;
    BenchmarkAdapterOutcome inject_outcome;
    BenchmarkAdapterOutcome build_outcome;
    BenchmarkAdapterOutcome verify_outcome;
    const char *output_text;
    const char *expected_names[4];
    size_t expected_count;
    const char *output_names[4];
    size_t output_count;
    unsigned int hooks_with_timeout;
    RunnerStringVector seen_flags;
} FakeAdapter;

typedef struct {
    int calls;
    int formatter_calls;
    int diff_calls;
    int diff_code;
    int fail_run;
    int timed_out;
    int timeout;
} FakeProcess;

static int write_text(const char *path, const char *text)
{
    return runner_write_file(path, text, strlen(text));
}

static int make_parent(const char *path)
{
    char *copy = strdup(path);
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

static int fake_expected(void *opaque,
                         const BenchmarkCaseContext *case_context,
                         RunnerStringVector *names,
                         char *error,
                         size_t error_size)
{
    FakeAdapter *fake = opaque;
    size_t index;
    (void)case_context;
    (void)error;
    (void)error_size;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    if (fake->expected_count == 0) {
        return runner_string_vector_push(names, "demo.c");
    }
    for (index = 0; index < fake->expected_count; ++index) {
        if (runner_string_vector_push(names, fake->expected_names[index]) != 0) {
            return -1;
        }
    }
    return 0;
}

static BenchmarkAdapterOutcome fake_prepare(
    void *opaque, const BenchmarkCaseContext *case_context,
    char *error, size_t error_size)
{
    FakeAdapter *fake = opaque;
    (void)case_context;
    (void)error;
    (void)error_size;
    ++fake->prepare_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    if (fake->prepare_outcome != BENCHMARK_ADAPTER_OK) {
        snprintf(error, error_size, "prepare failed");
    }
    return fake->prepare_outcome;
}

static BenchmarkAdapterOutcome fake_transform(
    void *opaque, const BenchmarkCaseContext *case_context,
    const RunnerStringVector *flags, RunnerStringVector *outputs,
    char *error, size_t error_size)
{
    FakeAdapter *fake = opaque;
    char *output;
    size_t index;
    ++fake->transform_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    for (index = 0; index < flags->length; ++index) {
        if (runner_string_vector_push(&fake->seen_flags, flags->items[index]) != 0) {
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
    }
    if (fake->transform_outcome != BENCHMARK_ADAPTER_OK) {
        snprintf(error, error_size, "transform failed");
        return fake->transform_outcome;
    }
    for (index = 0; index < (fake->output_count ? fake->output_count : 1);
         ++index) {
        const char *name =
            fake->output_count ? fake->output_names[index] : "demo.c";
        output = runner_path_join(case_context->work_dir, name);
        if (output == NULL || make_parent(output) != 0 ||
            write_text(output, fake->output_text ? fake->output_text : "out\n") !=
                0 ||
            runner_string_vector_push(outputs, output) != 0) {
            snprintf(error, error_size, "cannot create output");
            free(output);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        free(output);
    }
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome fake_inject(
    void *opaque, const BenchmarkCaseContext *case_context,
    const RunnerStringVector *outputs, char *error, size_t error_size)
{
    FakeAdapter *fake = opaque;
    (void)case_context;
    (void)outputs;
    (void)error;
    (void)error_size;
    ++fake->inject_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    if (fake->inject_outcome != BENCHMARK_ADAPTER_OK) {
        snprintf(error, error_size, "inject failed");
    }
    return fake->inject_outcome;
}

static BenchmarkAdapterOutcome fake_build(
    void *opaque, const BenchmarkCaseContext *case_context,
    char *error, size_t error_size)
{
    FakeAdapter *fake = opaque;
    (void)case_context;
    ++fake->build_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    if (fake->build_outcome != BENCHMARK_ADAPTER_OK) {
        snprintf(error, error_size, "build failed");
    }
    return fake->build_outcome;
}

static BenchmarkAdapterOutcome fake_verify(
    void *opaque, const BenchmarkCaseContext *case_context,
    char *error, size_t error_size)
{
    FakeAdapter *fake = opaque;
    (void)case_context;
    (void)error;
    (void)error_size;
    ++fake->verify_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
    if (fake->verify_outcome != BENCHMARK_ADAPTER_OK) {
        snprintf(error, error_size, "verify failed");
    }
    return fake->verify_outcome;
}

static void fake_cleanup(void *opaque, const BenchmarkCaseContext *case_context)
{
    FakeAdapter *fake = opaque;
    ++fake->cleanup_calls;
    fake->hooks_with_timeout += case_context->timeout_seconds == 17;
}

static const BenchmarkAdapterVTable fake_vtable = {
    fake_expected, fake_prepare, fake_transform, fake_inject,
    fake_build, fake_verify, fake_cleanup
};

static int fake_process_run(const ProcessSpec *spec,
                            ProcessResult *result,
                            void *opaque)
{
    FakeProcess *fake = opaque;
    const char *tool = runner_path_basename(spec->argv[0]);
    memset(result, 0, sizeof(*result));
    result->exited = 1;
    result->exit_code = 0;
    result->stdout_data = strdup("");
    result->stderr_data = strdup("");
    ++fake->calls;
    fake->timeout = spec->timeout_seconds;
    if (fake->fail_run) {
        free(result->stdout_data);
        free(result->stderr_data);
        result->stdout_data = NULL;
        result->stderr_data = NULL;
        return -1;
    }
    if (strstr(tool, "format") != NULL) {
        ++fake->formatter_calls;
    } else if (strstr(tool, "diff") != NULL) {
        ++fake->diff_calls;
        result->exit_code = fake->diff_code;
    }
    if (fake->timed_out) {
        result->timed_out = 1;
    }
    return result->stdout_data != NULL && result->stderr_data != NULL ? 0 : -1;
}

static void init_config(BenchmarkConfig *config,
                        BenchmarkSuite *suite,
                        BenchmarkKernel *kernel,
                        BenchmarkProfile *profile,
                        const char *root)
{
    static char *suite_profiles[] = {"parallel"};
    static char *flags[] = {"-ompGen=1", "-alias=3"};
    memset(config, 0, sizeof(*config));
    memset(suite, 0, sizeof(*suite));
    memset(kernel, 0, sizeof(*kernel));
    memset(profile, 0, sizeof(*profile));
    runner_key_value_map_init(&config->paths);
    runner_key_value_map_set(&config->paths, "CETUS", root);
    runner_key_value_map_set(&config->paths, "CLANG_FORMAT", "fake-format");
    runner_key_value_map_set(&config->paths, "DIFF", "fake-diff");
    runner_key_value_map_set(&config->paths, "ROOT", root);
    profile->name = "parallel";
    profile->flags.items = flags;
    profile->flags.length = 2;
    suite->id = "suite";
    suite->is_default = 1;
    suite->root_env = "ROOT";
    suite->profiles.items = suite_profiles;
    suite->profiles.length = 1;
    suite->kernels = kernel;
    suite->kernel_count = 1;
    kernel->name = "demo";
    config->profiles = profile;
    config->profile_count = 1;
    config->suites = suite;
    config->suite_count = 1;
}

static BenchmarkRunnerOptions make_options(const char *project_root,
                                            FakeProcess *process)
{
    BenchmarkRunnerOptions options;
    benchmark_runner_options_init(&options);
    options.project_root = project_root;
    options.timeout_seconds = 17;
    options.process_run = fake_process_run;
    options.process_context = process;
    return options;
}

static int test_selection_and_summary(void)
{
    BenchmarkConfig config;
    BenchmarkSuite suites[2] = {0};
    BenchmarkKernel kernels[2] = {0};
    BenchmarkProfile profile = {0};
    RunnerStringVector suite_profiles = {0};
    BenchmarkSelection selection;
    BenchmarkSummary summary = {0};
    char error[256];

    init_config(&config, &suites[0], &kernels[0], &profile, "/tmp");
    suites[1] = suites[0];
    kernels[1] = kernels[0];
    suites[1].id = "other";
    suites[1].is_default = 0;
    suites[1].kernels = &kernels[1];
    kernels[1].name = "second";
    config.suites = suites;
    config.suite_count = 2;
    benchmark_selection_init(&selection);
    CHECK(benchmark_runner_select(
              &config, NULL, NULL, NULL, &selection, error, sizeof(error)) == 0);
    CHECK(selection.length == 1);
    CHECK(strcmp(selection.items[0].suite->id, "suite") == 0);
    runner_string_vector_init(&suite_profiles);
    CHECK(runner_string_vector_push(&suite_profiles, "other") == 0);
    CHECK(benchmark_runner_select(&config, &suite_profiles, "sec", "parallel",
                                  &selection, error, sizeof(error)) == 0);
    CHECK(selection.length == 1);
    CHECK(strcmp(selection.items[0].kernel->name, "second") == 0);
    selection.items[0].result.status = BENCHMARK_STATUS_XPASS;
    benchmark_summary_add(&summary, &selection.items[0].result);
    CHECK(summary.xpass == 1 && benchmark_summary_exit_code(&summary) == 1);
    benchmark_selection_free(&selection);
    runner_string_vector_free(&suite_profiles);
    runner_key_value_map_free(&config.paths);
    return 0;
}

static int test_lifecycle_compare_and_skip_run(void)
{
    char root[] = "/tmp/cetus-benchmark-runner-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *gt_dir;
    char *gt_file;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    gt_file = runner_path_join(gt_dir, "demo.c");
    CHECK(gt_dir && gt_file && runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(gt_file, "tracked ground truth\n") == 0);
    fake.output_text = "transformed\n";
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    options.skip_run = 1;
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_PASS);
    CHECK(result.stage == BENCHMARK_STAGE_GROUND_TRUTH);
    CHECK(fake.prepare_calls == 1 && fake.transform_calls == 1);
    CHECK(fake.inject_calls == 0 && fake.cleanup_calls == 1);
    CHECK(process.formatter_calls == 2 && process.diff_calls == 1);
    CHECK(process.timeout == 17);
    CHECK(fake.seen_flags.length == 2);
    CHECK(runner_file_exists(
        "/tmp/cetus-benchmark-runner-this-path-must-not-exist") == 0);
    benchmark_case_result_free(&result);
    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(gt_file);
    free(gt_dir);
    return 0;
}

static int test_failures_and_xfail_mapping(void)
{
    char root[] = "/tmp/cetus-benchmark-xfail-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *gt_dir;
    char *gt_file;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    kernel.xfail = "known";
    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    gt_file = runner_path_join(gt_dir, "demo.c");
    CHECK(gt_dir && gt_file && runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(gt_file, "gt\n") == 0);
    fake.output_text = "out\n";
    fake.transform_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XFAIL);
    CHECK(result.stage == BENCHMARK_STAGE_CETUS);
    benchmark_case_result_free(&result);

    fake.transform_outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    benchmark_case_result_free(&result);

    fake.transform_outcome = BENCHMARK_ADAPTER_OK;
    process.diff_code = 2;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "diff tool failed") != NULL);
    benchmark_case_result_free(&result);

    process.diff_code = 0;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XPASS);
    benchmark_case_result_free(&result);

    options.timeout_seconds = 0;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) != 0);
    CHECK(strstr(error, "timeout") != NULL);

    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(gt_file);
    free(gt_dir);
    return 0;
}

static int test_missing_gt_and_generate_preserves_stale(void)
{
    char root[] = "/tmp/cetus-benchmark-generate-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *gt_dir;
    char *stale;
    char *generated;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    fake.output_text = "generated\n";
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_GROUND_TRUTH);
    CHECK(fake.prepare_calls == 0);
    benchmark_case_result_free(&result);

    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    stale = runner_path_join(gt_dir, "stale.c");
    generated = runner_path_join(gt_dir, "demo.c");
    CHECK(gt_dir && stale && generated &&
          runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(stale, "keep\n") == 0);
    options.generate = 1;
    options.skip_run = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_PASS);
    CHECK(runner_file_exists(stale) && runner_file_exists(generated));
    benchmark_case_result_free(&result);

    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(generated);
    free(stale);
    free(gt_dir);
    return 0;
}

static int test_output_set_validation_and_partial_gt(void)
{
    char root[] = "/tmp/cetus-benchmark-output-set-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *gt_dir;
    char *gt_file;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    fake.expected_names[0] = "a.c";
    fake.expected_names[1] = "b.c";
    fake.expected_count = 2;
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    gt_file = runner_path_join(gt_dir, "a.c");
    CHECK(gt_dir && gt_file && runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(gt_file, "a\n") == 0);

    /* Without xfail, partial GT triggers the pre-check failure. */
    kernel.xfail = NULL;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_GROUND_TRUTH);
    CHECK(strstr(result.message, "missing ground truth") != NULL);
    CHECK(fake.prepare_calls == 0);
    benchmark_case_result_free(&result);

    /* With xfail, the pre-check is skipped so Cetus can run and xfail. */
    kernel.xfail = "must not mask setup";
    options.generate = 1;
    options.skip_run = 1;
    fake.output_names[0] = "a.c";
    fake.output_count = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "missing adapter output") != NULL);
    benchmark_case_result_free(&result);

    fake.output_names[0] = "a.c";
    fake.output_names[1] = "b.c";
    fake.output_names[2] = "extra.c";
    fake.output_count = 3;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "unexpected adapter output") != NULL);
    benchmark_case_result_free(&result);

    fake.output_names[0] = "a.c";
    fake.output_names[1] = "a.c";
    fake.output_count = 2;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "duplicate adapter output") != NULL);
    benchmark_case_result_free(&result);

    fake.output_names[0] = "one/a.c";
    fake.output_names[1] = "two/a.c";
    fake.output_count = 2;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "basename collision") != NULL);
    benchmark_case_result_free(&result);

    fake.expected_names[1] = "a.c";
    fake.output_names[0] = "a.c";
    fake.output_count = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(strstr(result.message, "duplicate expected output") != NULL);
    benchmark_case_result_free(&result);

    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(gt_file);
    free(gt_dir);
    return 0;
}

static int test_rejects_unsafe_identifiers_before_delete(void)
{
    char root[] = "/tmp/cetus-benchmark-safe-path-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *sentinel;
    char error[256];
    const char *unsafe[] = {
        "", ".", "..", "../escape", "a/b", "a\\b", "/absolute", "\\absolute",
        "C:drive"
    };
    size_t index;

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    sentinel = runner_path_join(root, "sentinel.txt");
    CHECK(sentinel && write_text(sentinel, "safe\n") == 0);
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    for (index = 0; index < sizeof(unsafe) / sizeof(unsafe[0]); ++index) {
        kernel.name = (char *)unsafe[index];
        CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                        &result, error, sizeof(error)) != 0);
        CHECK(strstr(error, "unsafe") != NULL);
        CHECK(runner_file_exists(sentinel));
    }
    kernel.name = "demo";
    suite.id = "../outside";
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) != 0);
    CHECK(runner_file_exists(sentinel));
    suite.id = "suite";
    profile.name = "..";
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) != 0);
    CHECK(runner_file_exists(sentinel));

    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(sentinel);
    return 0;
}

static int test_timeout_override_hooks_and_unique_logs(void)
{
    char root[] = "/tmp/cetus-benchmark-contract-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult first;
    BenchmarkCaseResult second;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    RunnerStringVector overrides;
    char *gt_dir;
    char *gt_file;
    char *contents = NULL;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    gt_file = runner_path_join(gt_dir, "demo.c");
    CHECK(gt_dir && gt_file && runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(gt_file, "tracked\n") == 0);
    runner_string_vector_init(&overrides);
    CHECK(runner_string_vector_push(&overrides, "-override=1") == 0);
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    options.override_flags = &overrides;
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &first, error, sizeof(error)) == 0);
    CHECK(first.status == BENCHMARK_STATUS_PASS);
    CHECK(fake.seen_flags.length == 1);
    CHECK(strcmp(fake.seen_flags.items[0], "-override=1") == 0);
    CHECK(fake.hooks_with_timeout == 7);
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &second, error, sizeof(error)) == 0);
    CHECK(strcmp(first.log_path, second.log_path) != 0);
    CHECK(runner_file_exists(first.log_path) && runner_file_exists(second.log_path));
    CHECK(runner_read_file(gt_file, &contents, NULL) == 0);
    CHECK(strcmp(contents, "tracked\n") == 0);
    free(contents);
    benchmark_case_result_free(&second);
    benchmark_case_result_free(&first);

    options.timeout_seconds = -1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &first, error, sizeof(error)) != 0);
    CHECK(strstr(error, "timeout") != NULL);

    runner_string_vector_free(&fake.seen_flags);
    runner_string_vector_free(&overrides);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(gt_file);
    free(gt_dir);
    return 0;
}

static int test_stage_and_tool_failure_mapping(void)
{
    char root[] = "/tmp/cetus-benchmark-stages-XXXXXX";
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char *gt_dir;
    char *gt_file;
    char error[256];

    CHECK(mkdtemp(root) != NULL);
    init_config(&config, &suite, &kernel, &profile, root);
    kernel.xfail = "known functional failure";
    gt_dir = runner_path_join(root,
        "ground_truth/benchmarks/suite/parallel/demo");
    gt_file = runner_path_join(gt_dir, "demo.c");
    CHECK(gt_dir && gt_file && runner_mkdir_recursive(gt_dir, 0755) == 0);
    CHECK(write_text(gt_file, "tracked\n") == 0);
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options(root, &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;

    fake.prepare_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_PREPARE);
    benchmark_case_result_free(&result);
    fake.prepare_outcome = BENCHMARK_ADAPTER_OK;

    fake.transform_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XFAIL);
    CHECK(result.stage == BENCHMARK_STAGE_CETUS);
    benchmark_case_result_free(&result);
    fake.transform_outcome = BENCHMARK_ADAPTER_OK;

    fake.inject_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_INJECT);
    benchmark_case_result_free(&result);
    fake.inject_outcome = BENCHMARK_ADAPTER_OK;

    fake.build_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XFAIL);
    CHECK(result.stage == BENCHMARK_STAGE_BUILD);
    benchmark_case_result_free(&result);
    fake.build_outcome = BENCHMARK_ADAPTER_OK;

    fake.verify_outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XFAIL);
    CHECK(result.stage == BENCHMARK_STAGE_VERIFY);
    benchmark_case_result_free(&result);
    fake.verify_outcome = BENCHMARK_ADAPTER_OK;

    process.diff_code = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_XFAIL);
    CHECK(result.stage == BENCHMARK_STAGE_GROUND_TRUTH);
    benchmark_case_result_free(&result);
    process.diff_code = 0;

    process.fail_run = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_FORMAT);
    benchmark_case_result_free(&result);
    process.fail_run = 0;

    process.timed_out = 1;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) == 0);
    CHECK(result.status == BENCHMARK_STATUS_FAIL);
    CHECK(result.stage == BENCHMARK_STAGE_FORMAT);
    benchmark_case_result_free(&result);

    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    CHECK(runner_remove_recursive(root) == 0);
    free(gt_file);
    free(gt_dir);
    return 0;
}

static int test_log_initialization_failure(void)
{
    BenchmarkConfig config;
    BenchmarkSuite suite;
    BenchmarkKernel kernel;
    BenchmarkProfile profile;
    BenchmarkCase test_case;
    BenchmarkCaseResult result;
    BenchmarkRunnerOptions options;
    BenchmarkAdapterInstance adapter;
    FakeAdapter fake = {0};
    FakeProcess process = {0};
    char error[256];

    init_config(&config, &suite, &kernel, &profile, "/tmp");
    runner_string_vector_init(&fake.seen_flags);
    adapter.vtable = &fake_vtable;
    adapter.context = &fake;
    options = make_options("/proc/cetus-runner-forbidden", &process);
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(benchmark_runner_run_case(&config, &test_case, &adapter, &options,
                                    &result, error, sizeof(error)) != 0);
    CHECK(strstr(error, "log") != NULL);
    CHECK(fake.prepare_calls == 0);
    runner_string_vector_free(&fake.seen_flags);
    runner_key_value_map_free(&config.paths);
    return 0;
}

static int test_timeout_parser_rejects_nonpositive(void)
{
    int timeout = 0;
    char error[256];
    CHECK(benchmark_runner_parse_timeout("600", &timeout,
                                         error, sizeof(error)) == 0);
    CHECK(timeout == 600);
    CHECK(benchmark_runner_parse_timeout("0", &timeout,
                                         error, sizeof(error)) != 0);
    CHECK(benchmark_runner_parse_timeout("-1", &timeout,
                                         error, sizeof(error)) != 0);
    CHECK(benchmark_runner_parse_timeout("abc", &timeout,
                                         error, sizeof(error)) != 0);
    CHECK(benchmark_runner_parse_timeout("999999999999999999999", &timeout,
                                         error, sizeof(error)) != 0);
    return 0;
}

int main(void)
{
    CHECK(test_selection_and_summary() == 0);
    CHECK(test_lifecycle_compare_and_skip_run() == 0);
    CHECK(test_failures_and_xfail_mapping() == 0);
    CHECK(test_missing_gt_and_generate_preserves_stale() == 0);
    CHECK(test_output_set_validation_and_partial_gt() == 0);
    CHECK(test_rejects_unsafe_identifiers_before_delete() == 0);
    CHECK(test_timeout_override_hooks_and_unique_logs() == 0);
    CHECK(test_stage_and_tool_failure_mapping() == 0);
    CHECK(test_log_initialization_failure() == 0);
    CHECK(test_timeout_parser_rejects_nonpositive() == 0);
    puts("all benchmark runner tests passed");
    return 0;
}
