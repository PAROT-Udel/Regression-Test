#define _POSIX_C_SOURCE 200809L

#include "benchmark_adapters.h"
#include "benchmark_config.h"
#include "benchmark_runner.h"
#include "runner_common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(cond)                                                            \
    do {                                                                        \
        if (!(cond)) {                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            return 1;                                                           \
        }                                                                       \
    } while (0)

/* ------------------------------------------------------------------ */
/*  helpers                                                           */
/* ------------------------------------------------------------------ */

static int write_text(const char *path, const char *text)
{
    return runner_write_file(path, text, strlen(text));
}

static int make_parent(const char *path)
{
    char *copy = strdup(path);
    char *slash;
    int result;
    if (copy == NULL)
        return -1;
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

static int write_fixture(const char *base, const char *rel, const char *text)
{
    char *path = runner_path_join(base, rel);
    int result;
    if (path == NULL || make_parent(path) != 0) {
        free(path);
        return -1;
    }
    result = write_text(path, text);
    free(path);
    return result;
}

static int file_exists_at(const char *base, const char *rel)
{
    char *path = runner_path_join(base, rel);
    int exists;
    if (path == NULL)
        return 0;
    exists = runner_file_exists(path);
    free(path);
    return exists;
}

static char *read_at(const char *base, const char *rel)
{
    char *path = runner_path_join(base, rel);
    char *contents = NULL;
    if (path == NULL)
        return NULL;
    runner_read_file(path, &contents, NULL);
    free(path);
    return contents;
}

/* ------------------------------------------------------------------ */
/*  fake process runner for adapter tests                             */
/* ------------------------------------------------------------------ */

typedef struct {
    int cetus_calls;
    int gcc_calls;
    int make_calls;
    int binary_calls;
    int diff_calls;
    int cetus_exit;
    int gcc_exit;
    int make_exit;
    int binary_exit;
    int diff_exit;
    int binary_fail_on_call;
    const char *binary_stdout;
    const char *binary_stderr;
    const char *cetus_output_content;
    int create_npbparams;
    char last_cetus_argv[4096];
    char last_gcc_argv[4096];
    char last_make_argv[4096];
    char last_binary_argv[4096];
} AdapterFakeProcess;

static void adapter_fake_init(AdapterFakeProcess *f)
{
    memset(f, 0, sizeof(*f));
    f->cetus_output_content = "/* cetus transformed */\n";
    f->binary_stdout = "";
    f->binary_stderr = "";
}

static void join_argv(const char *const *argv, char *buf, size_t bufsz)
{
    buf[0] = '\0';
    for (size_t i = 0; argv[i] != NULL; ++i) {
        if (i > 0)
            strncat(buf, " ", bufsz - strlen(buf) - 1);
        strncat(buf, argv[i], bufsz - strlen(buf) - 1);
    }
}

static int adapter_fake_run(const ProcessSpec *spec,
                            ProcessResult *result,
                            void *opaque)
{
    AdapterFakeProcess *f = opaque;
    const char *tool;
    memset(result, 0, sizeof(*result));
    result->exited = 1;
    result->exit_code = 0;
    result->stdout_data = strdup("");
    result->stderr_data = strdup("");
    if (result->stdout_data == NULL || result->stderr_data == NULL)
        return -1;

    tool = spec->argv[0];
    {
        const char *slash = strrchr(tool, '/');
        if (slash)
            tool = slash + 1;
    }

    if (strstr(tool, "cetus") != NULL) {
        ++f->cetus_calls;
        join_argv(spec->argv, f->last_cetus_argv, sizeof(f->last_cetus_argv));
        result->exit_code = f->cetus_exit;
        if (result->exit_code == 0) {
            /* find -outdir=... and source file to create output */
            const char *outdir = NULL;
            const char *src_name = NULL;
            for (int i = 0; spec->argv[i]; ++i) {
                if (strncmp(spec->argv[i], "-outdir=", 8) == 0)
                    outdir = spec->argv[i] + 8;
                if (spec->argv[i][0] != '-')
                    src_name = spec->argv[i];
            }
            if (outdir && src_name) {
                char *dir = strdup(outdir);
                char *out;
                if (dir) {
                    runner_mkdir_recursive(dir, 0755);
                    out = runner_path_join(dir, src_name);
                    if (out) {
                        write_text(out, f->cetus_output_content);
                        free(out);
                    }
                    free(dir);
                }
            }
        }
    } else if (strstr(tool, "gcc") != NULL || strstr(tool, "cc") != NULL) {
        ++f->gcc_calls;
        join_argv(spec->argv, f->last_gcc_argv, sizeof(f->last_gcc_argv));
        result->exit_code = f->gcc_exit;
        if (result->exit_code == 0) {
            /* find -o <binary> and create a dummy file */
            for (int i = 0; spec->argv[i]; ++i) {
                if (strcmp(spec->argv[i], "-o") == 0 && spec->argv[i + 1]) {
                    write_text(spec->argv[i + 1], "#!/bin/true\n");
                    chmod(spec->argv[i + 1], 0755);
                    break;
                }
            }
        }
    } else if (strcmp(tool, "make") == 0) {
        ++f->make_calls;
        join_argv(spec->argv, f->last_make_argv, sizeof(f->last_make_argv));
        result->exit_code = f->make_exit;
        if (f->create_npbparams && spec->cwd) {
            /* find bench dir from make args and create npbparams.h */
            for (int i = 1; spec->argv[i]; ++i) {
                const char *a = spec->argv[i];
                if (a[0] != '-' && strchr(a, '=') == NULL && islower((unsigned char)a[0])) {
                    char *benchdir = runner_path_join(spec->cwd, a);
                    if (benchdir == NULL) {
                        /* try uppercase first letter */
                        size_t len = strlen(a);
                        char *upper = malloc(len + 1);
                        if (upper) {
                            memcpy(upper, a, len + 1);
                            upper[0] = toupper((unsigned char)upper[0]);
                            benchdir = runner_path_join(spec->cwd, upper);
                            free(upper);
                        }
                    }
                    if (benchdir) {
                        char *params = runner_path_join(benchdir, "npbparams.h");
                        if (params) {
                            write_text(params, "/* generated npbparams.h */\n");
                            free(params);
                        }
                        free(benchdir);
                    }
                    break;
                }
            }
        }
    } else if (strstr(tool, "diff") != NULL) {
        ++f->diff_calls;
        result->exit_code = f->diff_exit;
    } else {
        ++f->binary_calls;
        join_argv(spec->argv, f->last_binary_argv,
                  sizeof(f->last_binary_argv));
        if (f->binary_fail_on_call > 0 &&
            f->binary_calls == f->binary_fail_on_call)
            result->exit_code = 1;
        else
            result->exit_code = f->binary_exit;
        free(result->stdout_data);
        free(result->stderr_data);
        result->stdout_data = strdup(f->binary_stdout ? f->binary_stdout : "");
        result->stderr_data = strdup(f->binary_stderr ? f->binary_stderr : "");
        if (!result->stdout_data || !result->stderr_data)
            return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test fixture builders                                             */
/* ------------------------------------------------------------------ */

static int make_polybench_root(const char *root)
{
    return write_fixture(root, "utilities/polybench.c",
                         "/* polybench.c */\n") == 0 &&
           write_fixture(root, "utilities/polybench.h",
                         "/* polybench.h */\n") == 0 &&
           write_fixture(root, "linear-algebra/blas/gemm/gemm.c",
                         "/* gemm source */\n") == 0 &&
           write_fixture(root, "linear-algebra/blas/gemm/gemm.h",
                         "/* gemm header */\n") == 0
               ? 0
               : -1;
}

static int make_npb_root(const char *root)
{
    return write_fixture(root, "Makefile", "# NPB root Makefile\n") == 0 &&
           write_fixture(root, "config/make.def", "# make.def\n") == 0 &&
           write_fixture(root, "sys/setparams.c", "/* setparams */\n") == 0 &&
           write_fixture(root, "sys/Makefile", "# sys Makefile\n") == 0 &&
           write_fixture(root, "common/c_print_results.c",
                         "/* c_print_results */\n") == 0 &&
           write_fixture(root, "common/c_timers.c", "/* c_timers */\n") == 0 &&
           write_fixture(root, "common/wtime.c", "/* wtime */\n") == 0 &&
           write_fixture(root, "common/wtime.h", "/* wtime.h */\n") == 0 &&
           write_fixture(root, "common/randdp.c", "/* randdp */\n") == 0 &&
           write_fixture(root, "common/type.h", "/* type.h */\n") == 0 &&
           write_fixture(root, "CG/Makefile", "# CG Makefile\n") == 0 &&
           write_fixture(root, "CG/cg.c", "/* cg source */\n") == 0 &&
           write_fixture(root, "CG/globals.h", "/* globals */\n") == 0 &&
           write_fixture(root, "CG/rose_junk.c",
                         "/* experiment junk */\n") == 0
               ? 0
               : -1;
}

static int make_generic_root(const char *root)
{
    return write_fixture(root, "src/demo.c", "/* demo source */\n") == 0 &&
           write_fixture(root, "include/demo.h", "/* demo header */\n") == 0
               ? 0
               : -1;
}

/* ------------------------------------------------------------------ */
/*  build BenchmarkCaseContext for adapter testing                     */
/* ------------------------------------------------------------------ */

static void make_case_context(BenchmarkCaseContext *ctx,
                              const BenchmarkConfig *config,
                              const BenchmarkCase *test_case,
                              const char *project_root,
                              const char *work_dir,
                              const char *gt_dir,
                              AdapterFakeProcess *proc)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->config = config;
    ctx->test_case = test_case;
    ctx->project_root = project_root;
    ctx->work_dir = work_dir;
    ctx->ground_truth_dir = gt_dir;
    ctx->cetus = "fake-cetus";
    ctx->timeout_seconds = 120;
    ctx->process_run = adapter_fake_run;
    ctx->process_context = proc;
}

/* ------------------------------------------------------------------ */
/*  test: factory creates adapters                                    */
/* ------------------------------------------------------------------ */

static int test_factory_creates_adapters(void)
{
    BenchmarkAdapterInstance inst;
    char error[256];

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &inst,
                                   error, sizeof(error)) == 0);
    CHECK(inst.vtable == &polybench_adapter_vtable);
    benchmark_adapter_destroy(&inst);
    CHECK(inst.vtable == NULL);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &inst,
                                   error, sizeof(error)) == 0);
    CHECK(inst.vtable == &npb_adapter_vtable);
    benchmark_adapter_destroy(&inst);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &inst,
                                   error, sizeof(error)) == 0);
    CHECK(inst.vtable == &generic_adapter_vtable);
    benchmark_adapter_destroy(&inst);

    CHECK(benchmark_adapter_create((BenchmarkAdapter)99, &inst,
                                   error, sizeof(error)) != 0);
    CHECK(strstr(error, "unknown") != NULL);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench expected_gt_names                                 */
/* ------------------------------------------------------------------ */

static int test_polybench_expected_gt_names(void)
{
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector names;
    AdapterFakeProcess proc;
    char error[256];

    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, "/tmp", "/tmp/work",
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);
    runner_string_vector_init(&names);
    CHECK(adapter.vtable->expected_gt_names(adapter.context, &ctx, &names,
                                            error, sizeof(error)) == 0);
    CHECK(names.length == 1);
    CHECK(strcmp(names.items[0], "gemm.c") == 0);
    runner_string_vector_free(&names);
    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench prepare creates correct layout                    */
/* ------------------------------------------------------------------ */

static int test_polybench_prepare(void)
{
    char project[] = "/tmp/cetus-adapter-pb-prep-XXXXXX";
    char *pb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    pb_root = runner_path_join(project, "polybench_root");
    work = runner_path_join(project, "work");
    CHECK(pb_root && work);
    CHECK(runner_mkdir_recursive(pb_root, 0755) == 0);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);
    CHECK(make_polybench_root(pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "POLYBENCH_ROOT",
                                   pb_root) == 0);

    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    suite.dataset = "MINI_DATASET";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(file_exists_at(work, "original/gemm.c"));
    CHECK(file_exists_at(work, "kernel/gemm.c"));
    CHECK(file_exists_at(work, "kernel/gemm.h"));
    CHECK(file_exists_at(work, "baseline/gemm.c"));
    CHECK(file_exists_at(work, "baseline/gemm.h"));
    CHECK(file_exists_at(work, "utilities/polybench.c"));
    CHECK(file_exists_at(work, "utilities/polybench.h"));

    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(pb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench cetus_transform builds correct argv               */
/* ------------------------------------------------------------------ */

static int test_polybench_cetus_transform(void)
{
    char project[] = "/tmp/cetus-adapter-pb-xform-XXXXXX";
    char *pb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector flags;
    RunnerStringVector outputs;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    pb_root = runner_path_join(project, "polybench_root");
    work = runner_path_join(project, "work");
    CHECK(pb_root && work);
    CHECK(make_polybench_root(pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "POLYBENCH_ROOT",
                                   pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "CPP", "test-cpp") == 0);

    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    suite.dataset = "MINI_DATASET";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);
    ctx.cetus = "/path/to/cetus";

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);
    /* prepare first to create directories */
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);

    runner_string_vector_init(&flags);
    CHECK(runner_string_vector_push(&flags, "-ompGen=1") == 0);
    runner_string_vector_init(&outputs);
    out = adapter.vtable->cetus_transform(adapter.context, &ctx, &flags,
                                          &outputs, error, sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(proc.cetus_calls == 1);
    CHECK(outputs.length == 1);
    CHECK(strstr(outputs.items[0], "gemm.c") != NULL);
    /* verify argv contains preprocessor with dataset and dump arrays */
    CHECK(strstr(proc.last_cetus_argv, "-preprocessor=") != NULL);
    CHECK(strstr(proc.last_cetus_argv, "MINI_DATASET") != NULL);
    CHECK(strstr(proc.last_cetus_argv, "POLYBENCH_DUMP_ARRAYS") != NULL);
    CHECK(strstr(proc.last_cetus_argv, "-ompGen=1") != NULL);

    runner_string_vector_free(&outputs);
    runner_string_vector_free(&flags);
    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(pb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench build produces correct gcc commands                */
/* ------------------------------------------------------------------ */

static int test_polybench_build(void)
{
    char project[] = "/tmp/cetus-adapter-pb-build-XXXXXX";
    char *pb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    pb_root = runner_path_join(project, "polybench_root");
    work = runner_path_join(project, "work");
    CHECK(pb_root && work);
    CHECK(make_polybench_root(pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "POLYBENCH_ROOT",
                                   pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "GCC", "test-gcc") == 0);

    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    suite.dataset = "MINI_DATASET";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    out = adapter.vtable->build(adapter.context, &ctx, error,
                                sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(proc.gcc_calls == 2);
    CHECK(strstr(proc.last_gcc_argv, "-O2") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "-fopenmp") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "-std=c11") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "MINI_DATASET") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "POLYBENCH_DUMP_ARRAYS") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "polybench.c") != NULL);
    CHECK(strstr(proc.last_gcc_argv, "-lm") != NULL);

    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(pb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench verify runs dumps and diffs                       */
/* ------------------------------------------------------------------ */

static int test_polybench_verify(void)
{
    char project[] = "/tmp/cetus-adapter-pb-verify-XXXXXX";
    char *pb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    proc.binary_stderr = "==BEGIN_DUMP\n1.0 2.0\n==END_DUMP\n";
    runner_key_value_map_init(&config.paths);
    pb_root = runner_path_join(project, "polybench_root");
    work = runner_path_join(project, "work");
    CHECK(pb_root && work);
    CHECK(make_polybench_root(pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "POLYBENCH_ROOT",
                                   pb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "GCC", "test-gcc") == 0);
    CHECK(runner_key_value_map_set(&config.paths, "DIFF", "test-diff") == 0);

    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    suite.dataset = "MINI_DATASET";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    /* build needs to create dummy binaries */
    out = adapter.vtable->build(adapter.context, &ctx, error,
                                sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    /* verify */
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(proc.binary_calls == 2);
    CHECK(proc.diff_calls == 1);
    /* check dump files were written */
    CHECK(file_exists_at(work, "dumps/baseline.txt"));
    CHECK(file_exists_at(work, "dumps/transformed.txt"));

    /* verify failure when diff reports mismatch */
    proc.diff_exit = 1;
    proc.binary_calls = 0;
    proc.diff_calls = 0;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
    CHECK(strstr(error, "mismatch") != NULL);

    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(pb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: npb expected_gt_names returns sources                       */
/* ------------------------------------------------------------------ */

static int test_npb_expected_gt_names(void)
{
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector names;
    AdapterFakeProcess proc;
    char error[256];

    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    suite.adapter = BENCHMARK_ADAPTER_NPB;
    suite.root_env = "NPB_SER_ROOT";
    runner_string_vector_init(&kernel.sources);
    runner_string_vector_init(&kernel.cetus_sources);
    CHECK(runner_string_vector_push(&kernel.sources, "cg.c") == 0);
    kernel.name = "cg";
    kernel.bench = "CG";
    kernel.binary = "cg";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, "/tmp", "/tmp/work",
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &adapter,
                                   error, sizeof(error)) == 0);
    runner_string_vector_init(&names);
    CHECK(adapter.vtable->expected_gt_names(adapter.context, &ctx, &names,
                                            error, sizeof(error)) == 0);
    CHECK(names.length == 1);
    CHECK(strcmp(names.items[0], "cg.c") == 0);
    runner_string_vector_free(&names);
    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.sources);
    runner_string_vector_free(&kernel.cetus_sources);
    runner_key_value_map_free(&config.paths);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: npb prepare builds sandbox and filters junk                 */
/* ------------------------------------------------------------------ */

static int test_npb_prepare(void)
{
    char project[] = "/tmp/cetus-adapter-npb-prep-XXXXXX";
    char *npb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    proc.create_npbparams = 1;
    runner_key_value_map_init(&config.paths);
    npb_root = runner_path_join(project, "npb_root");
    work = runner_path_join(project, "work");
    CHECK(npb_root && work);
    CHECK(runner_mkdir_recursive(npb_root, 0755) == 0);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);
    CHECK(make_npb_root(npb_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "NPB_SER_ROOT",
                                   npb_root) == 0);

    suite.adapter = BENCHMARK_ADAPTER_NPB;
    suite.root_env = "NPB_SER_ROOT";
    suite.npb_class = "S";
    runner_string_vector_init(&kernel.sources);
    runner_string_vector_init(&kernel.cetus_sources);
    runner_string_vector_init(&kernel.headers);
    CHECK(runner_string_vector_push(&kernel.sources, "cg.c") == 0);
    CHECK(runner_string_vector_push(&kernel.headers, "globals.h") == 0);
    kernel.name = "cg";
    kernel.bench = "CG";
    kernel.binary = "cg";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &adapter,
                                   error, sizeof(error)) == 0);
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    /* sandbox should have root Makefile */
    CHECK(file_exists_at(work, "sandbox/Makefile"));
    /* config dir */
    CHECK(file_exists_at(work, "sandbox/config/make.def"));
    /* common dir - canonical support copied */
    CHECK(file_exists_at(work, "sandbox/common/c_print_results.c"));
    CHECK(file_exists_at(work, "sandbox/common/type.h"));
    /* bench dir with source and header */
    CHECK(file_exists_at(work, "sandbox/CG/cg.c"));
    CHECK(file_exists_at(work, "sandbox/CG/globals.h"));
    CHECK(file_exists_at(work, "sandbox/CG/Makefile"));
    /* experiment junk filtered out */
    CHECK(!file_exists_at(work, "sandbox/CG/rose_junk.c"));
    /* originals snapshotted */
    CHECK(file_exists_at(work, "original/cg.c"));

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.sources);
    runner_string_vector_free(&kernel.cetus_sources);
    runner_string_vector_free(&kernel.headers);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(npb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: npb inject removes stale .o files                           */
/* ------------------------------------------------------------------ */

static int test_npb_inject(void)
{
    char project[] = "/tmp/cetus-adapter-npb-inject-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector outputs;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;
    char *obj_path;
    char *common_obj;
    char *output_file;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);
    /* create sandbox bench dir with a stale .o file */
    CHECK(write_fixture(work, "sandbox/CG/cg.c", "/* old */\n") == 0);
    CHECK(write_fixture(work, "sandbox/CG/cg.o", "stale object\n") == 0);
    CHECK(write_fixture(work, "sandbox/common/c_print_results.o",
                        "stale\n") == 0);
    /* create a transformed output */
    CHECK(write_fixture(work, "cetus_output/cg.c",
                        "/* transformed */\n") == 0);

    suite.adapter = BENCHMARK_ADAPTER_NPB;
    suite.root_env = "NPB_SER_ROOT";
    runner_string_vector_init(&kernel.sources);
    runner_string_vector_init(&kernel.cetus_sources);
    CHECK(runner_string_vector_push(&kernel.sources, "cg.c") == 0);
    kernel.name = "cg";
    kernel.bench = "CG";
    kernel.binary = "cg";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &adapter,
                                   error, sizeof(error)) == 0);
    runner_string_vector_init(&outputs);
    output_file = runner_path_join(work, "cetus_output/cg.c");
    CHECK(output_file != NULL);
    CHECK(runner_string_vector_push(&outputs, output_file) == 0);
    out = adapter.vtable->inject(adapter.context, &ctx, &outputs, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    /* transformed source should be in bench dir */
    {
        char *content = read_at(work, "sandbox/CG/cg.c");
        CHECK(content != NULL);
        CHECK(strstr(content, "transformed") != NULL);
        free(content);
    }
    /* stale .o should be removed */
    obj_path = runner_path_join(work, "sandbox/CG/cg.o");
    CHECK(obj_path != NULL);
    CHECK(!runner_file_exists(obj_path));
    free(obj_path);
    common_obj = runner_path_join(work, "sandbox/common/c_print_results.o");
    CHECK(common_obj != NULL);
    CHECK(!runner_file_exists(common_obj));
    free(common_obj);

    runner_string_vector_free(&outputs);
    free(output_file);
    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.sources);
    runner_string_vector_free(&kernel.cetus_sources);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: npb verify accepts verification patterns                    */
/* ------------------------------------------------------------------ */

static int test_npb_verify(void)
{
    char project[] = "/tmp/cetus-adapter-npb-verify-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);
    /* create fake binary in bin/ */
    CHECK(write_fixture(work, "sandbox/bin/cg.S.x", "#!/bin/true\n") == 0);
    {
        char *binpath = runner_path_join(work, "sandbox/bin/cg.S.x");
        CHECK(binpath);
        chmod(binpath, 0755);
        free(binpath);
    }

    suite.adapter = BENCHMARK_ADAPTER_NPB;
    suite.root_env = "NPB_SER_ROOT";
    suite.npb_class = "S";
    runner_string_vector_init(&kernel.sources);
    runner_string_vector_init(&kernel.cetus_sources);
    CHECK(runner_string_vector_push(&kernel.sources, "cg.c") == 0);
    kernel.name = "cg";
    kernel.bench = "CG";
    kernel.binary = "cg";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &adapter,
                                   error, sizeof(error)) == 0);

    /* "Verification Successful" phrase */
    proc.binary_stdout = " Verification Successful\n";
    error[0] = '\0';
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(strstr(error, "Verification Successful") != NULL);

    /* "verification = successful" form */
    proc.binary_stdout = " Verification = SUCCESSFUL\n";
    proc.binary_calls = 0;
    error[0] = '\0';
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(strstr(error, "Verification = SUCCESSFUL") != NULL ||
          strstr(error, "Verification = Successful") != NULL ||
          strstr(error, "SUCCESSFUL") != NULL);

    /* missing verification string */
    proc.binary_stdout = "No verification info here\n";
    proc.binary_calls = 0;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
    CHECK(strstr(error, "verification") != NULL ||
          strstr(error, "Verification") != NULL);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.sources);
    runner_string_vector_free(&kernel.cetus_sources);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic adapter full lifecycle                              */
/* ------------------------------------------------------------------ */

static int test_generic_lifecycle(void)
{
    char project[] = "/tmp/cetus-adapter-gen-XXXXXX";
    char *gen_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector flags;
    RunnerStringVector outputs;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    gen_root = runner_path_join(project, "generic_root");
    work = runner_path_join(project, "work");
    CHECK(gen_root && work);
    CHECK(make_generic_root(gen_root) == 0);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "CUSTOM_ROOT",
                                   gen_root) == 0);
    CHECK(runner_key_value_map_set(&config.paths, "GCC", "test-gcc") == 0);

    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "CUSTOM_ROOT";
    suite.dataset = "MINI";
    memset(&suite.generic, 0, sizeof(suite.generic));
    runner_string_vector_init(&suite.generic.copy);
    runner_string_vector_init(&suite.generic.cetus_inputs);
    runner_key_value_map_init(&suite.generic.inject_map);
    runner_string_vector_init(&suite.generic.compile.argv);
    runner_string_vector_init(&suite.generic.run.argv);
    runner_string_vector_init(&suite.generic.verify.run.argv);
    runner_string_vector_init(&suite.generic.verify.baseline.argv);

    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_copy = 1;
    CHECK(runner_string_vector_push(&kernel.generic.copy, "src/demo.c") == 0);
    CHECK(runner_string_vector_push(&kernel.generic.copy,
                                    "include/demo.h") == 0);
    kernel.generic.has_cetus_inputs = 1;
    CHECK(runner_string_vector_push(&kernel.generic.cetus_inputs,
                                    "src/demo.c") == 0);
    kernel.generic.has_inject_map = 1;
    CHECK(runner_key_value_map_set(&kernel.generic.inject_map, "demo.c",
                                   "src/demo.c") == 0);
    runner_key_value_map_init(&kernel.inject_map);
    CHECK(runner_key_value_map_set(&kernel.inject_map, "demo.c",
                                   "src/demo.c") == 0);
    kernel.generic.has_compile = 1;
    kernel.generic.compile.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&kernel.generic.compile.argv);
    CHECK(runner_string_vector_push(&kernel.generic.compile.argv,
                                    "{gcc}") == 0);
    CHECK(runner_string_vector_push(&kernel.generic.compile.argv,
                                    "src/demo.c") == 0);
    CHECK(runner_string_vector_push(&kernel.generic.compile.argv,
                                    "-o") == 0);
    CHECK(runner_string_vector_push(&kernel.generic.compile.argv,
                                    "demo.exe") == 0);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_EXIT_CODE;
    kernel.generic.verify.run.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    CHECK(runner_string_vector_push(&kernel.generic.verify.run.argv,
                                    "./demo.exe") == 0);

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    /* expected gt names */
    {
        RunnerStringVector names;
        runner_string_vector_init(&names);
        CHECK(adapter.vtable->expected_gt_names(adapter.context, &ctx,
                                                &names, error,
                                                sizeof(error)) == 0);
        CHECK(names.length == 1);
        CHECK(strcmp(names.items[0], "demo.c") == 0);
        runner_string_vector_free(&names);
    }

    /* prepare */
    out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                  sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(file_exists_at(work, "sandbox/src/demo.c"));
    CHECK(file_exists_at(work, "sandbox/include/demo.h"));
    CHECK(file_exists_at(work, "original/demo.c"));

    /* cetus_transform */
    runner_string_vector_init(&flags);
    CHECK(runner_string_vector_push(&flags, "-ompGen=1") == 0);
    runner_string_vector_init(&outputs);
    out = adapter.vtable->cetus_transform(adapter.context, &ctx, &flags,
                                          &outputs, error, sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(outputs.length == 1);
    CHECK(proc.cetus_calls == 1);

    /* inject */
    out = adapter.vtable->inject(adapter.context, &ctx, &outputs, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    {
        char *content = read_at(work, "sandbox/src/demo.c");
        CHECK(content != NULL);
        CHECK(strstr(content, "cetus transformed") != NULL);
        free(content);
    }

    /* build */
    out = adapter.vtable->build(adapter.context, &ctx, error,
                                sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);

    /* verify (exit_code) */
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);

    runner_string_vector_free(&outputs);
    runner_string_vector_free(&flags);
    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_string_vector_free(&kernel.generic.compile.argv);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&suite.generic.copy);
    runner_string_vector_free(&suite.generic.cetus_inputs);
    runner_key_value_map_free(&suite.generic.inject_map);
    runner_key_value_map_free(&kernel.inject_map);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(gen_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic verify regex                                        */
/* ------------------------------------------------------------------ */

static int test_generic_verify_regex(void)
{
    char project[] = "/tmp/cetus-adapter-gen-regex-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "ROOT";
    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_REGEX;
    kernel.generic.verify.pattern = strdup("TEST.*PASS");
    kernel.generic.verify.run.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    CHECK(runner_string_vector_push(&kernel.generic.verify.run.argv,
                                    "./test.exe") == 0);

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    /* need sandbox dir for cwd */
    CHECK(write_fixture(work, "sandbox/.keep", "") == 0);
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    /* match */
    proc.binary_stdout = "running... TEST 42 PASSED\n";
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);

    /* no match */
    proc.binary_stdout = "FAIL: no match\n";
    proc.binary_calls = 0;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);

    benchmark_adapter_destroy(&adapter);
    free(kernel.generic.verify.pattern);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic verify dump_diff                                    */
/* ------------------------------------------------------------------ */

static int test_generic_verify_dump_diff(void)
{
    char project[] = "/tmp/cetus-adapter-gen-dump-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    CHECK(runner_key_value_map_set(&config.paths, "DIFF", "test-diff") == 0);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "ROOT";
    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_DUMP_DIFF;
    kernel.generic.verify.dump_stream = BENCHMARK_DUMP_STREAM_STDERR;
    kernel.generic.verify.run.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    CHECK(runner_string_vector_push(&kernel.generic.verify.run.argv,
                                    "./transformed.exe") == 0);
    kernel.generic.verify.baseline.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    CHECK(runner_string_vector_push(&kernel.generic.verify.baseline.argv,
                                    "./baseline.exe") == 0);

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(write_fixture(work, "sandbox/.keep", "") == 0);
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    proc.binary_stderr = "dump output line 1\n";
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(proc.binary_calls == 2);
    CHECK(proc.diff_calls == 1);
    CHECK(file_exists_at(work, "dumps/baseline.txt"));
    CHECK(file_exists_at(work, "dumps/transformed.txt"));

    /* diff mismatch */
    proc.diff_exit = 1;
    proc.binary_calls = 0;
    proc.diff_calls = 0;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&kernel.generic.verify.baseline.argv);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: dump_diff config parsing                                    */
/* ------------------------------------------------------------------ */

static int test_dump_diff_config_parsing(void)
{
    char root[] = "/tmp/cetus-adapter-ddcfg-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];
    const BenchmarkVerify *v;

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "dd.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);
    CHECK(write_text(suite_file,
                     "{\"id\":\"dd\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"generic\":{\"verify\":{"
                     "\"type\":\"dump_diff\","
                     "\"run\":[\"./xform.exe\"],"
                     "\"baseline\":[\"./base.exe\"],"
                     "\"stream\":\"stdout\"}},"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);

    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suite_count == 1);
    v = &config.suites[0].generic.verify;
    CHECK(v->type == BENCHMARK_VERIFY_DUMP_DIFF);
    CHECK(v->baseline.type == BENCHMARK_COMMAND_ARGV);
    CHECK(v->baseline.argv.length == 1);
    CHECK(strcmp(v->baseline.argv.items[0], "./base.exe") == 0);
    CHECK(v->run.type == BENCHMARK_COMMAND_ARGV);
    CHECK(v->run.argv.length == 1);
    CHECK(strcmp(v->run.argv.items[0], "./xform.exe") == 0);
    CHECK(v->dump_stream == BENCHMARK_DUMP_STREAM_STDOUT);

    benchmark_config_free(&config);
    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: dump_diff config validation rejects missing fields          */
/* ------------------------------------------------------------------ */

static int test_dump_diff_config_validation(void)
{
    char root[] = "/tmp/cetus-adapter-ddval-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "dd.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* missing baseline */
    CHECK(write_text(suite_file,
                     "{\"id\":\"dd\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"generic\":{\"verify\":{"
                     "\"type\":\"dump_diff\","
                     "\"run\":[\"./xform.exe\"]}},"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "dump_diff") != NULL ||
          strstr(error, "baseline") != NULL);
    benchmark_config_free(&config);

    /* invalid stream value */
    CHECK(write_text(suite_file,
                     "{\"id\":\"dd\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"generic\":{\"verify\":{"
                     "\"type\":\"dump_diff\","
                     "\"run\":[\"./x.exe\"],"
                     "\"baseline\":[\"./b.exe\"],"
                     "\"stream\":\"file\"}},"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "stream") != NULL);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: dump_diff treats baseline nonzero exit as failure (F1)      */
/* ------------------------------------------------------------------ */

static int test_dump_diff_baseline_exit_failure(void)
{
    char project[] = "/tmp/cetus-adapter-ddf1-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    CHECK(runner_key_value_map_set(&config.paths, "DIFF", "test-diff") == 0);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "ROOT";
    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_DUMP_DIFF;
    kernel.generic.verify.dump_stream = BENCHMARK_DUMP_STREAM_STDERR;
    kernel.generic.verify.run.type = BENCHMARK_COMMAND_ARGV;
    CHECK(runner_string_vector_push(&kernel.generic.verify.run.argv,
                                    "./transformed.exe") == 0);
    kernel.generic.verify.baseline.type = BENCHMARK_COMMAND_ARGV;
    CHECK(runner_string_vector_push(&kernel.generic.verify.baseline.argv,
                                    "./baseline.exe") == 0);

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(write_fixture(work, "sandbox/.keep", "") == 0);
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    /* baseline exits nonzero → functional failure, no diff */
    proc.binary_exit = 1;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
    CHECK(strstr(error, "baseline") != NULL);
    CHECK(proc.diff_calls == 0);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&kernel.generic.verify.baseline.argv);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: dump_diff treats transformed nonzero exit as failure (F1)   */
/* ------------------------------------------------------------------ */

static int test_dump_diff_transformed_exit_failure(void)
{
    char project[] = "/tmp/cetus-adapter-ddf2-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    CHECK(runner_key_value_map_set(&config.paths, "DIFF", "test-diff") == 0);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "ROOT";
    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_DUMP_DIFF;
    kernel.generic.verify.dump_stream = BENCHMARK_DUMP_STREAM_STDERR;
    kernel.generic.verify.run.type = BENCHMARK_COMMAND_ARGV;
    CHECK(runner_string_vector_push(&kernel.generic.verify.run.argv,
                                    "./transformed.exe") == 0);
    kernel.generic.verify.baseline.type = BENCHMARK_COMMAND_ARGV;
    CHECK(runner_string_vector_push(&kernel.generic.verify.baseline.argv,
                                    "./baseline.exe") == 0);

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(write_fixture(work, "sandbox/.keep", "") == 0);
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    /* baseline succeeds (call 1), transformed fails (call 2) */
    proc.binary_fail_on_call = 2;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);
    CHECK(strstr(error, "transformed") != NULL);
    CHECK(proc.diff_calls == 0);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&kernel.generic.verify.baseline.argv);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic config rejects shell compile command (F2)           */
/* ------------------------------------------------------------------ */

static int test_generic_config_rejects_shell_compile(void)
{
    char root[] = "/tmp/cetus-adapter-shell-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "shell.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* compile as string (shell command) should be rejected */
    CHECK(write_text(suite_file,
                     "{\"id\":\"sh\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"generic\":{\"compile\":\"gcc -O2 demo.c\","
                     "\"verify\":{\"type\":\"exit_code\","
                     "\"run\":[\"./demo\"]}},"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"demo.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "argv array") != NULL ||
          strstr(error, "shell") != NULL);
    benchmark_config_free(&config);

    /* array compile should still work */
    CHECK(write_text(suite_file,
                     "{\"id\":\"sh\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"generic\":{\"compile\":[\"gcc\",\"demo.c\"],"
                     "\"verify\":{\"type\":\"exit_code\","
                     "\"run\":[\"./demo\"]}},"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"demo.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic verify exit_code uses suite run fallback (F2)       */
/* ------------------------------------------------------------------ */

static int test_generic_verify_exit_code_run_fallback(void)
{
    char project[] = "/tmp/cetus-adapter-runfb-XXXXXX";
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    AdapterFakeProcess proc;
    char error[256];
    BenchmarkAdapterOutcome out;

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    work = runner_path_join(project, "work");
    CHECK(work);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    /* suite has a run command; kernel verify has exit_code but NO run */
    suite.adapter = BENCHMARK_ADAPTER_GENERIC;
    suite.root_env = "ROOT";
    memset(&suite.generic, 0, sizeof(suite.generic));
    runner_string_vector_init(&suite.generic.copy);
    runner_string_vector_init(&suite.generic.cetus_inputs);
    runner_key_value_map_init(&suite.generic.inject_map);
    runner_string_vector_init(&suite.generic.compile.argv);
    runner_string_vector_init(&suite.generic.verify.run.argv);
    runner_string_vector_init(&suite.generic.verify.baseline.argv);
    suite.generic.has_run = 1;
    suite.generic.run.type = BENCHMARK_COMMAND_ARGV;
    runner_string_vector_init(&suite.generic.run.argv);
    CHECK(runner_string_vector_push(&suite.generic.run.argv,
                                    "./suite_app.exe") == 0);

    kernel.name = "demo";
    memset(&kernel.generic, 0, sizeof(kernel.generic));
    runner_string_vector_init(&kernel.generic.copy);
    runner_string_vector_init(&kernel.generic.cetus_inputs);
    runner_key_value_map_init(&kernel.generic.inject_map);
    runner_string_vector_init(&kernel.generic.compile.argv);
    runner_string_vector_init(&kernel.generic.run.argv);
    runner_string_vector_init(&kernel.generic.verify.run.argv);
    runner_string_vector_init(&kernel.generic.verify.baseline.argv);
    kernel.generic.has_verify = 1;
    kernel.generic.verify.present = 1;
    kernel.generic.verify.type = BENCHMARK_VERIFY_EXIT_CODE;
    /* verify.run is left as NONE — should fall back to suite run */

    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    CHECK(write_fixture(work, "sandbox/.keep", "") == 0);
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_GENERIC, &adapter,
                                   error, sizeof(error)) == 0);

    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_OK);
    CHECK(proc.binary_calls == 1); /* fallback run was actually executed */

    /* verify nonzero exit is caught */
    proc.binary_exit = 1;
    proc.binary_calls = 0;
    out = adapter.vtable->verify(adapter.context, &ctx, error,
                                 sizeof(error));
    CHECK(out == BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&suite.generic.run.argv);
    runner_string_vector_free(&suite.generic.copy);
    runner_string_vector_free(&suite.generic.cetus_inputs);
    runner_key_value_map_free(&suite.generic.inject_map);
    runner_string_vector_free(&suite.generic.verify.run.argv);
    runner_string_vector_free(&suite.generic.verify.baseline.argv);
    runner_string_vector_free(&kernel.generic.copy);
    runner_string_vector_free(&kernel.generic.cetus_inputs);
    runner_key_value_map_free(&kernel.generic.inject_map);
    runner_string_vector_free(&kernel.generic.verify.run.argv);
    runner_string_vector_free(&kernel.generic.verify.baseline.argv);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic config rejects traversal paths (F3)                 */
/* ------------------------------------------------------------------ */

static int test_generic_config_rejects_traversal_paths(void)
{
    char root[] = "/tmp/cetus-adapter-trav-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "bad.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* copy with traversal */
    CHECK(write_text(suite_file,
                     "{\"id\":\"bad\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"copy\":[\"../escape\"],"
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "safe relative path") != NULL ||
          strstr(error, "traversal") != NULL);
    benchmark_config_free(&config);

    /* cetus_inputs with traversal */
    CHECK(write_text(suite_file,
                     "{\"id\":\"bad\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"copy\":[\"safe.c\"],"
                     "\"cetus_inputs\":[\"../evil.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "safe relative path") != NULL);
    benchmark_config_free(&config);

    /* inject_map value with traversal */
    CHECK(write_text(suite_file,
                     "{\"id\":\"bad\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"cetus_inputs\":[\"k.c\"],"
                     "\"inject_map\":{\"k.c\":\"../../out.c\"}}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "safe relative path") != NULL);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: generic config rejects absolute paths (F3)                  */
/* ------------------------------------------------------------------ */

static int test_generic_config_rejects_absolute_paths(void)
{
    char root[] = "/tmp/cetus-adapter-abs-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "bad.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* absolute path in copy */
    CHECK(write_text(suite_file,
                     "{\"id\":\"bad\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"copy\":[\"/etc/passwd\"],"
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "safe relative path") != NULL);
    benchmark_config_free(&config);

    /* drive-prefixed path */
    CHECK(write_text(suite_file,
                     "{\"id\":\"bad\",\"adapter\":\"generic\","
                     "\"root_env\":\"ROOT\",\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"k\","
                     "\"copy\":[\"C:demo.c\"],"
                     "\"cetus_inputs\":[\"k.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "safe relative path") != NULL);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: safe relative path unit tests (F3)                          */
/* ------------------------------------------------------------------ */

static int test_safe_relative_path_unit(void)
{
    CHECK(benchmark_safe_relative_path("src/demo.c") == 1);
    CHECK(benchmark_safe_relative_path("demo.c") == 1);
    CHECK(benchmark_safe_relative_path("a/b/c") == 1);
    CHECK(benchmark_safe_relative_path("./src/demo.c") == 1);
    CHECK(benchmark_safe_relative_path("") == 0);
    CHECK(benchmark_safe_relative_path(NULL) == 0);
    CHECK(benchmark_safe_relative_path("/absolute") == 0);
    CHECK(benchmark_safe_relative_path("C:drive") == 0);
    CHECK(benchmark_safe_relative_path("d:drive") == 0);
    CHECK(benchmark_safe_relative_path("..") == 0);
    CHECK(benchmark_safe_relative_path("../escape") == 0);
    CHECK(benchmark_safe_relative_path("a/../escape") == 0);
    CHECK(benchmark_safe_relative_path("a/b/../../escape") == 0);
    CHECK(benchmark_safe_relative_path("\\backslash") == 0);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench src_file/header override (F4)                     */
/* ------------------------------------------------------------------ */

static int test_polybench_src_file_override(void)
{
    char project[] = "/tmp/cetus-adapter-pbsrc-XXXXXX";
    char *pb_root = NULL;
    char *work = NULL;
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector names;
    AdapterFakeProcess proc;
    char error[256];

    CHECK(mkdtemp(project) != NULL);
    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    pb_root = runner_path_join(project, "polybench_root");
    work = runner_path_join(project, "work");
    CHECK(pb_root && work);
    /* create polybench root with custom source name */
    CHECK(write_fixture(pb_root, "utilities/polybench.c",
                        "/* polybench.c */\n") == 0);
    CHECK(write_fixture(pb_root, "utilities/polybench.h",
                        "/* polybench.h */\n") == 0);
    CHECK(write_fixture(pb_root, "linear-algebra/blas/gemm/custom_gemm.c",
                        "/* custom source */\n") == 0);
    CHECK(write_fixture(pb_root, "linear-algebra/blas/gemm/custom_gemm.h",
                        "/* custom header */\n") == 0);
    CHECK(runner_key_value_map_set(&config.paths, "POLYBENCH_ROOT",
                                   pb_root) == 0);
    CHECK(runner_mkdir_recursive(work, 0755) == 0);

    suite.adapter = BENCHMARK_ADAPTER_POLYBENCH;
    suite.root_env = "POLYBENCH_ROOT";
    suite.dataset = "MINI_DATASET";
    kernel.name = "gemm";
    kernel.rel_path = "linear-algebra/blas/gemm";
    kernel.src_file = "custom_gemm.c";
    kernel.header_file = "custom_gemm.h";
    runner_string_vector_init(&kernel.cetus_sources);
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, project, work,
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_POLYBENCH, &adapter,
                                   error, sizeof(error)) == 0);

    /* expected GT names should use override */
    runner_string_vector_init(&names);
    CHECK(adapter.vtable->expected_gt_names(adapter.context, &ctx, &names,
                                            error, sizeof(error)) == 0);
    CHECK(names.length == 1);
    CHECK(strcmp(names.items[0], "custom_gemm.c") == 0);
    runner_string_vector_free(&names);

    /* prepare should copy custom files */
    {
        BenchmarkAdapterOutcome out;
        out = adapter.vtable->prepare(adapter.context, &ctx, error,
                                      sizeof(error));
        CHECK(out == BENCHMARK_ADAPTER_OK);
        CHECK(file_exists_at(work, "original/custom_gemm.c"));
        CHECK(file_exists_at(work, "kernel/custom_gemm.c"));
        CHECK(file_exists_at(work, "kernel/custom_gemm.h"));
        CHECK(file_exists_at(work, "baseline/custom_gemm.c"));
        CHECK(file_exists_at(work, "baseline/custom_gemm.h"));
    }

    benchmark_adapter_destroy(&adapter);
    runner_key_value_map_free(&config.paths);
    runner_remove_recursive(project);
    free(work);
    free(pb_root);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench config parses src_file/header (F4)                */
/* ------------------------------------------------------------------ */

static int test_polybench_config_parses_src_file(void)
{
    char root[] = "/tmp/cetus-adapter-pbcfg-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "pb.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* kernel with src_file and header overrides */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\","
                     "\"src_file\":\"custom_gemm.c\","
                     "\"header\":\"custom_gemm.h\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suite_count == 1);
    CHECK(config.suites[0].kernel_count == 1);
    CHECK(config.suites[0].kernels[0].src_file != NULL);
    CHECK(strcmp(config.suites[0].kernels[0].src_file, "custom_gemm.c") ==
          0);
    CHECK(config.suites[0].kernels[0].header_file != NULL);
    CHECK(strcmp(config.suites[0].kernels[0].header_file,
                "custom_gemm.h") == 0);
    benchmark_config_free(&config);

    /* kernel without overrides: fields should be NULL */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suites[0].kernels[0].src_file == NULL);
    CHECK(config.suites[0].kernels[0].header_file == NULL);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: NPB cetus_sources override (F4)                            */
/* ------------------------------------------------------------------ */

static int test_npb_cetus_sources_override(void)
{
    BenchmarkAdapterInstance adapter;
    BenchmarkSuite suite = {0};
    BenchmarkKernel kernel = {0};
    BenchmarkProfile profile = {0};
    BenchmarkConfig config = {0};
    BenchmarkCase test_case;
    BenchmarkCaseContext ctx;
    RunnerStringVector names;
    AdapterFakeProcess proc;
    char error[256];

    adapter_fake_init(&proc);
    runner_key_value_map_init(&config.paths);
    suite.adapter = BENCHMARK_ADAPTER_NPB;
    suite.root_env = "NPB_SER_ROOT";
    runner_string_vector_init(&kernel.sources);
    CHECK(runner_string_vector_push(&kernel.sources, "cg.c") == 0);
    CHECK(runner_string_vector_push(&kernel.sources, "helper.c") == 0);
    runner_string_vector_init(&kernel.cetus_sources);
    CHECK(runner_string_vector_push(&kernel.cetus_sources, "cg.c") == 0);
    kernel.name = "cg";
    kernel.bench = "CG";
    kernel.binary = "cg";
    profile.name = "parallel";
    test_case.suite = &suite;
    test_case.kernel = &kernel;
    test_case.profile = &profile;
    make_case_context(&ctx, &config, &test_case, "/tmp", "/tmp/work",
                      "/tmp/gt", &proc);

    CHECK(benchmark_adapter_create(BENCHMARK_ADAPTER_NPB, &adapter,
                                   error, sizeof(error)) == 0);

    /* expected GT should use cetus_sources (only "cg.c"), not all sources */
    runner_string_vector_init(&names);
    CHECK(adapter.vtable->expected_gt_names(adapter.context, &ctx, &names,
                                            error, sizeof(error)) == 0);
    CHECK(names.length == 1);
    CHECK(strcmp(names.items[0], "cg.c") == 0);
    runner_string_vector_free(&names);

    benchmark_adapter_destroy(&adapter);
    runner_string_vector_free(&kernel.sources);
    runner_string_vector_free(&kernel.cetus_sources);
    runner_key_value_map_free(&config.paths);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: NPB config parses cetus_sources (F4)                       */
/* ------------------------------------------------------------------ */

static int test_npb_config_parses_cetus_sources(void)
{
    char root[] = "/tmp/cetus-adapter-npbcfg-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "npb.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* kernel with cetus_sources override */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\",\"helper.c\"],"
                     "\"cetus_sources\":[\"cg.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suites[0].kernels[0].cetus_sources.length == 1);
    CHECK(strcmp(config.suites[0].kernels[0].cetus_sources.items[0],
                "cg.c") == 0);
    /* sources still has both */
    CHECK(config.suites[0].kernels[0].sources.length == 2);
    benchmark_config_free(&config);

    /* without cetus_sources: field should be empty */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suites[0].kernels[0].cetus_sources.length == 0);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: polybench config rejects traversal in src_file/header (F5)  */
/* ------------------------------------------------------------------ */

static int test_polybench_config_rejects_bad_paths(void)
{
    char root[] = "/tmp/cetus-adapter-pbpath-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "pb.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* traversal in src_file */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\","
                     "\"src_file\":\"../evil.c\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "src_file") != NULL);
    benchmark_config_free(&config);

    /* nested path in src_file (should fail — basename only) */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\","
                     "\"src_file\":\"sub/gemm.c\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "src_file") != NULL);
    benchmark_config_free(&config);

    /* traversal in header */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\","
                     "\"header\":\"../../etc/passwd\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "header") != NULL);
    benchmark_config_free(&config);

    /* traversal in rel_path */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"../../escape\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "rel_path") != NULL);
    benchmark_config_free(&config);

    /* valid basename src_file and header should pass */
    CHECK(write_text(suite_file,
                     "{\"id\":\"pb\",\"adapter\":\"polybench\","
                     "\"root_env\":\"POLYBENCH_ROOT\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"gemm\","
                     "\"rel_path\":\"linear-algebra/blas/gemm\","
                     "\"src_file\":\"custom_gemm.c\","
                     "\"header\":\"custom_gemm.h\"}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  test: NPB config rejects traversal in sources/headers/etc (F5)    */
/* ------------------------------------------------------------------ */

static int test_npb_config_rejects_bad_paths(void)
{
    char root[] = "/tmp/cetus-adapter-npbpath-XXXXXX";
    char *suites;
    char *profiles;
    char *suite_file;
    BenchmarkConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    suite_file = runner_path_join(suites, "npb.json");
    CHECK(suites && profiles && suite_file);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles,
                     "{\"parallel\":{\"flags\":[\"-ompGen=1\"]}}") == 0);

    /* traversal in sources */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"../evil.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "sources") != NULL);
    benchmark_config_free(&config);

    /* traversal in headers */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\"],"
                     "\"headers\":[\"../evil.h\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "headers") != NULL);
    benchmark_config_free(&config);

    /* traversal in cetus_sources */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\"],"
                     "\"cetus_sources\":[\"../evil.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "cetus_sources") != NULL);
    benchmark_config_free(&config);

    /* traversal in bench */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"../CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "bench") != NULL);
    benchmark_config_free(&config);

    /* absolute path in binary */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"/bin/evil\","
                     "\"sources\":[\"cg.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) != 0);
    CHECK(strstr(error, "binary") != NULL);
    benchmark_config_free(&config);

    /* valid NPB kernel should still pass */
    CHECK(write_text(suite_file,
                     "{\"id\":\"npb\",\"adapter\":\"npb\","
                     "\"root_env\":\"NPB_SER_ROOT\",\"class\":\"S\","
                     "\"profiles\":[\"parallel\"],"
                     "\"kernels\":[{\"name\":\"cg\","
                     "\"bench\":\"CG\",\"binary\":\"cg\","
                     "\"sources\":[\"cg.c\"],"
                     "\"headers\":[\"globals.h\"],"
                     "\"cetus_sources\":[\"cg.c\"]}]}") == 0);
    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    benchmark_config_free(&config);

    runner_remove_recursive(root);
    free(suite_file);
    free(profiles);
    free(suites);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */

int main(void)
{
    CHECK(test_factory_creates_adapters() == 0);
    CHECK(test_polybench_expected_gt_names() == 0);
    CHECK(test_polybench_prepare() == 0);
    CHECK(test_polybench_cetus_transform() == 0);
    CHECK(test_polybench_build() == 0);
    CHECK(test_polybench_verify() == 0);
    CHECK(test_npb_expected_gt_names() == 0);
    CHECK(test_npb_prepare() == 0);
    CHECK(test_npb_inject() == 0);
    CHECK(test_npb_verify() == 0);
    CHECK(test_generic_lifecycle() == 0);
    CHECK(test_generic_verify_regex() == 0);
    CHECK(test_generic_verify_dump_diff() == 0);
    CHECK(test_dump_diff_config_parsing() == 0);
    CHECK(test_dump_diff_config_validation() == 0);
    /* F1: dump_diff baseline/transformed exit checks */
    CHECK(test_dump_diff_baseline_exit_failure() == 0);
    CHECK(test_dump_diff_transformed_exit_failure() == 0);
    /* F2: reject shell commands; verify run fallback */
    CHECK(test_generic_config_rejects_shell_compile() == 0);
    CHECK(test_generic_verify_exit_code_run_fallback() == 0);
    /* F3: path validation */
    CHECK(test_safe_relative_path_unit() == 0);
    CHECK(test_generic_config_rejects_traversal_paths() == 0);
    CHECK(test_generic_config_rejects_absolute_paths() == 0);
    /* F4: polybench/npb overrides */
    CHECK(test_polybench_src_file_override() == 0);
    CHECK(test_polybench_config_parses_src_file() == 0);
    CHECK(test_npb_cetus_sources_override() == 0);
    CHECK(test_npb_config_parses_cetus_sources() == 0);
    /* F5: polybench/npb path safety */
    CHECK(test_polybench_config_rejects_bad_paths() == 0);
    CHECK(test_npb_config_rejects_bad_paths() == 0);
    puts("all benchmark adapter tests passed");
    return 0;
}
