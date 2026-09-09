#define _POSIX_C_SOURCE 200809L

#include "benchmark_adapters.h"
#include "benchmark_config.h"
#include "benchmark_runner.h"
#include "runner_common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *make_filename(const char *rel_path, const char *ext)
{
    const char *base = runner_path_basename(rel_path);
    size_t base_len = strlen(base);
    size_t ext_len = strlen(ext);
    char *result = malloc(base_len + ext_len + 1);
    if (result != NULL) {
        memcpy(result, base, base_len);
        memcpy(result + base_len, ext, ext_len + 1);
    }
    return result;
}

static const char *resolve_root(const BenchmarkCaseContext *ctx)
{
    const BenchmarkCase *tc = ctx->test_case;
    const char *env = tc->kernel->root_env != NULL ? tc->kernel->root_env
                                                   : tc->suite->root_env;
    return benchmark_config_path(ctx->config, env);
}

static const char *resolve_dataset(const BenchmarkCaseContext *ctx)
{
    const char *ds = ctx->test_case->suite->dataset;
    return (ds != NULL && ds[0] != '\0') ? ds : "MINI_DATASET";
}

static int run_tool(const BenchmarkCaseContext *ctx,
                    const char *label,
                    const char *const *argv,
                    const char *cwd,
                    const RunnerEnvOverride *env,
                    size_t env_count,
                    ProcessResult *result)
{
    ProcessSpec spec;
    int rc;
    memset(&spec, 0, sizeof(spec));
    memset(result, 0, sizeof(*result));
    spec.argv = argv;
    spec.cwd = cwd;
    spec.environment = env;
    spec.environment_count = env_count;
    spec.timeout_seconds = (unsigned int)ctx->timeout_seconds;
    rc = ctx->process_run(&spec, result, ctx->process_context);
    benchmark_log_process(ctx->log_path, label, argv, rc == 0 ? result : NULL);
    return rc;
}

/* ------------------------------------------------------------------ */
/*  expected_gt_names                                                 */
/* ------------------------------------------------------------------ */

static const char *resolve_src_file(const BenchmarkKernel *kernel)
{
    return kernel->src_file;
}

static const char *resolve_header_file(const BenchmarkKernel *kernel)
{
    return kernel->header_file;
}

static int polybench_expected_gt_names(void *context,
                                       const BenchmarkCaseContext *ctx,
                                       RunnerStringVector *names,
                                       char *error,
                                       size_t error_size)
{
    const char *override = resolve_src_file(ctx->test_case->kernel);
    char *src;
    int rc;
    (void)context;
    if (override != NULL) {
        src = strdup(override);
    } else {
        src = make_filename(ctx->test_case->kernel->rel_path, ".c");
    }
    if (src == NULL) {
        snprintf(error, error_size, "out of memory");
        return -1;
    }
    rc = runner_string_vector_push(names, src);
    free(src);
    if (rc != 0) {
        snprintf(error, error_size, "out of memory");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  prepare                                                           */
/* ------------------------------------------------------------------ */

static int ensure_and_copy(const char *src_dir, const char *src_name,
                           const char *dst_dir, const char *dst_name)
{
    char *src = runner_path_join(src_dir, src_name);
    char *dst_d = runner_path_join(dst_dir, dst_name ? dst_name : src_name);
    int result;
    if (src == NULL || dst_d == NULL) {
        free(src);
        free(dst_d);
        return -1;
    }
    if (runner_mkdir_recursive(dst_dir, 0755) != 0) {
        free(src);
        free(dst_d);
        return -1;
    }
    result = runner_copy_file(src, dst_d);
    free(src);
    free(dst_d);
    return result;
}

static BenchmarkAdapterOutcome polybench_prepare(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const char *root = resolve_root(ctx);
    const char *rel_path = ctx->test_case->kernel->rel_path;
    const char *src_override = resolve_src_file(ctx->test_case->kernel);
    const char *hdr_override = resolve_header_file(ctx->test_case->kernel);
    char *src_file = src_override ? strdup(src_override)
                                  : make_filename(rel_path, ".c");
    char *hdr_file = hdr_override ? strdup(hdr_override)
                                  : make_filename(rel_path, ".h");
    char *ksrc = NULL;
    char *util_src = NULL;
    char *work_original = NULL;
    char *work_kernel = NULL;
    char *work_baseline = NULL;
    char *work_utilities = NULL;
    char *work_cetus_output = NULL;
    char *work_bin = NULL;
    char *work_dumps = NULL;
    char *src_path = NULL;
    char *hdr_path = NULL;
    int has_header;

    (void)context;
    if (src_file == NULL || hdr_file == NULL)
        goto oom;

    ksrc = runner_path_join(root, rel_path);
    util_src = runner_path_join(root, "utilities");
    work_original = runner_path_join(ctx->work_dir, "original");
    work_kernel = runner_path_join(ctx->work_dir, "kernel");
    work_baseline = runner_path_join(ctx->work_dir, "baseline");
    work_utilities = runner_path_join(ctx->work_dir, "utilities");
    work_cetus_output = runner_path_join(ctx->work_dir, "cetus_output");
    work_bin = runner_path_join(ctx->work_dir, "bin");
    work_dumps = runner_path_join(ctx->work_dir, "dumps");

    if (ksrc == NULL || util_src == NULL || work_original == NULL ||
        work_kernel == NULL || work_baseline == NULL ||
        work_utilities == NULL || work_cetus_output == NULL ||
        work_bin == NULL || work_dumps == NULL)
        goto oom;

    src_path = runner_path_join(ksrc, src_file);
    hdr_path = runner_path_join(ksrc, hdr_file);
    if (src_path == NULL || hdr_path == NULL)
        goto oom;

    if (!runner_file_exists(src_path)) {
        snprintf(error, error_size, "missing PolyBench source: %s", src_path);
        goto fail;
    }
    has_header = runner_file_exists(hdr_path);

    if (runner_mkdir_recursive(work_original, 0755) != 0 ||
        runner_mkdir_recursive(work_kernel, 0755) != 0 ||
        runner_mkdir_recursive(work_baseline, 0755) != 0 ||
        runner_mkdir_recursive(work_utilities, 0755) != 0 ||
        runner_mkdir_recursive(work_cetus_output, 0755) != 0 ||
        runner_mkdir_recursive(work_bin, 0755) != 0 ||
        runner_mkdir_recursive(work_dumps, 0755) != 0)
        goto infra;

    /* copy utilities */
    if (ensure_and_copy(util_src, "polybench.c", work_utilities, NULL) != 0 ||
        ensure_and_copy(util_src, "polybench.h", work_utilities, NULL) != 0) {
        snprintf(error, error_size, "cannot copy PolyBench utilities");
        goto fail;
    }

    /* copy source to original, kernel, baseline */
    if (ensure_and_copy(ksrc, src_file, work_original, NULL) != 0 ||
        ensure_and_copy(ksrc, src_file, work_kernel, NULL) != 0 ||
        ensure_and_copy(ksrc, src_file, work_baseline, NULL) != 0) {
        snprintf(error, error_size, "cannot copy source %s", src_file);
        goto fail;
    }

    if (has_header) {
        if (ensure_and_copy(ksrc, hdr_file, work_kernel, NULL) != 0 ||
            ensure_and_copy(ksrc, hdr_file, work_baseline, NULL) != 0) {
            snprintf(error, error_size, "cannot copy header %s", hdr_file);
            goto fail;
        }
    }

    goto ok;

oom:
    snprintf(error, error_size, "out of memory in polybench prepare");
fail:
    free(hdr_path); free(src_path);
    free(work_dumps); free(work_bin); free(work_cetus_output);
    free(work_utilities); free(work_baseline); free(work_kernel);
    free(work_original); free(util_src); free(ksrc);
    free(hdr_file); free(src_file);
    return BENCHMARK_ADAPTER_INFRA_FAILURE;
infra:
    snprintf(error, error_size, "cannot create work directories");
    free(hdr_path); free(src_path);
    free(work_dumps); free(work_bin); free(work_cetus_output);
    free(work_utilities); free(work_baseline); free(work_kernel);
    free(work_original); free(util_src); free(ksrc);
    free(hdr_file); free(src_file);
    return BENCHMARK_ADAPTER_INFRA_FAILURE;
ok:
    free(hdr_path); free(src_path);
    free(work_dumps); free(work_bin); free(work_cetus_output);
    free(work_utilities); free(work_baseline); free(work_kernel);
    free(work_original); free(util_src); free(ksrc);
    free(hdr_file); free(src_file);
    return BENCHMARK_ADAPTER_OK;
}

/* ------------------------------------------------------------------ */
/*  cetus_transform                                                   */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome polybench_cetus_transform(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *flags,
    RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    const char *dataset = resolve_dataset(ctx);
    const char *src_override = resolve_src_file(ctx->test_case->kernel);
    char *src_file = src_override ? strdup(src_override)
                                  : make_filename(ctx->test_case->kernel->rel_path, ".c");
    char *outdir_base = runner_path_join(ctx->work_dir, "cetus_run");
    char *outdir_output = NULL;
    char *staged = NULL;
    char *original = NULL;
    char *generated = NULL;
    char *dest = NULL;
    char *work_kernel = runner_path_join(ctx->work_dir, "kernel");
    char *work_util = runner_path_join(ctx->work_dir, "utilities");
    RunnerString prep;
    const char *cpp;
    const char **argv = NULL;
    size_t argc;
    ProcessResult result;
    BenchmarkAdapterOutcome outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;

    (void)context;
    runner_string_init(&prep);
    memset(&result, 0, sizeof(result));

    if (src_file == NULL || outdir_base == NULL || work_kernel == NULL ||
        work_util == NULL)
        goto oom;

    /* build preprocessor string */
    cpp = benchmark_config_path(ctx->config, "CPP");
    if (cpp == NULL || cpp[0] == '\0')
        cpp = "cpp";
    if (runner_string_append(&prep, cpp) != 0 ||
        runner_string_append(&prep, " -C -I. -I") != 0 ||
        runner_string_append(&prep, work_kernel) != 0 ||
        runner_string_append(&prep, " -I") != 0 ||
        runner_string_append(&prep, work_util) != 0 ||
        runner_string_append(&prep, " -D") != 0 ||
        runner_string_append(&prep, dataset) != 0 ||
        runner_string_append(&prep, " -DPOLYBENCH_DUMP_ARRAYS") != 0)
        goto oom;

    /* set up staging directory */
    if (runner_file_exists(outdir_base))
        runner_remove_recursive(outdir_base);
    if (runner_mkdir_recursive(outdir_base, 0755) != 0)
        goto infra;

    outdir_output = runner_path_join(outdir_base, "cetus_output");
    original = runner_path_join(ctx->work_dir, "original");
    staged = runner_path_join(outdir_base, src_file);
    if (outdir_output == NULL || original == NULL || staged == NULL)
        goto oom;

    {
        char *orig_src = runner_path_join(original, src_file);
        if (orig_src == NULL)
            goto oom;
        if (runner_copy_file(orig_src, staged) != 0) {
            snprintf(error, error_size, "cannot stage source %s", src_file);
            free(orig_src);
            goto done;
        }
        free(orig_src);
    }

    /* build argv: cetus -outdir=... -preprocessor=... flags... source */
    argc = 3 + flags->length + 1 + 1; /* cetus, -outdir, -preprocessor, flags, src, NULL */
    argv = calloc(argc, sizeof(*argv));
    if (argv == NULL)
        goto oom;
    {
        RunnerString outdir_arg;
        RunnerString prep_arg;
        size_t idx = 0;
        runner_string_init(&outdir_arg);
        runner_string_init(&prep_arg);
        if (runner_string_append(&outdir_arg, "-outdir=") != 0 ||
            runner_string_append(&outdir_arg, outdir_output) != 0 ||
            runner_string_append(&prep_arg, "-preprocessor=") != 0 ||
            runner_string_append(&prep_arg, prep.data) != 0) {
            runner_string_free(&outdir_arg);
            runner_string_free(&prep_arg);
            goto oom;
        }
        argv[idx++] = ctx->cetus;
        argv[idx++] = runner_string_take(&outdir_arg);
        argv[idx++] = runner_string_take(&prep_arg);
        for (size_t i = 0; i < flags->length; ++i)
            argv[idx++] = flags->items[i];
        argv[idx++] = src_file;
        argv[idx] = NULL;
    }

    if (run_tool(ctx, "cetus", argv, outdir_base, NULL, 0, &result) != 0) {
        snprintf(error, error_size, "cannot execute Cetus");
        goto done;
    }
    if (result.timed_out) {
        snprintf(error, error_size, "Cetus timed out");
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        goto done;
    }
    if (!result.exited || result.exit_code != 0) {
        snprintf(error, error_size, "Cetus failed (rc=%d)",
                 result.exit_code);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        goto done;
    }

    generated = runner_path_join(outdir_output, src_file);
    if (generated == NULL)
        goto oom;
    if (!runner_file_exists(generated)) {
        snprintf(error, error_size, "Cetus produced no output: %s", generated);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        goto done;
    }
    dest = runner_path_join(ctx->work_dir, "cetus_output");
    if (dest == NULL)
        goto oom;
    {
        char *final = runner_path_join(dest, src_file);
        if (final == NULL)
            goto oom;
        if (runner_copy_file(generated, final) != 0) {
            snprintf(error, error_size, "cannot copy Cetus output");
            free(final);
            goto done;
        }
        if (runner_string_vector_push(outputs, final) != 0) {
            free(final);
            goto oom;
        }
        free(final);
    }
    outcome = BENCHMARK_ADAPTER_OK;
    goto done;

oom:
    snprintf(error, error_size, "out of memory in polybench cetus_transform");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    goto done;
infra:
    snprintf(error, error_size, "cannot create staging directories");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
done:
    process_result_free(&result);
    if (argv != NULL) {
        free((char *)argv[1]); /* outdir arg */
        free((char *)argv[2]); /* preprocessor arg */
        free(argv);
    }
    runner_string_free(&prep);
    free(dest);
    free(generated);
    free(staged);
    free(original);
    free(outdir_output);
    free(outdir_base);
    free(work_util);
    free(work_kernel);
    free(src_file);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  inject                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome polybench_inject(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    char *work_kernel = runner_path_join(ctx->work_dir, "kernel");
    size_t i;
    (void)context;
    if (work_kernel == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    for (i = 0; i < outputs->length; ++i) {
        const char *name = runner_path_basename(outputs->items[i]);
        char *dst = runner_path_join(work_kernel, name);
        if (dst == NULL || runner_copy_file(outputs->items[i], dst) != 0) {
            snprintf(error, error_size, "cannot inject %s", name);
            free(dst);
            free(work_kernel);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        free(dst);
    }
    free(work_kernel);
    return BENCHMARK_ADAPTER_OK;
}

/* ------------------------------------------------------------------ */
/*  build                                                             */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome polybench_compile_one(
    const BenchmarkCaseContext *ctx,
    const char *src_dir,
    const char *src_file,
    const char *bin_path,
    const char *dataset,
    char *error,
    size_t error_size)
{
    const char *gcc = benchmark_config_path(ctx->config, "GCC");
    char *src_path = runner_path_join(src_dir, src_file);
    char *util_pb = runner_path_join(ctx->work_dir, "utilities/polybench.c");
    const char *argv[20];
    ProcessResult result;
    int idx = 0;
    BenchmarkAdapterOutcome outcome;

    if (gcc == NULL || gcc[0] == '\0')
        gcc = "gcc";

    if (src_path == NULL || util_pb == NULL) {
        snprintf(error, error_size, "out of memory");
        free(src_path);
        free(util_pb);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    {
        char *ds_flag = malloc(strlen(dataset) + 3);
        char *incdir_src = malloc(strlen(src_dir) + 3);
        char *util_dir = runner_path_join(ctx->work_dir, "utilities");
        char *incdir_util = NULL;
        if (ds_flag == NULL || incdir_src == NULL || util_dir == NULL) {
            free(ds_flag);
            free(incdir_src);
            free(util_dir);
            free(src_path);
            free(util_pb);
            snprintf(error, error_size, "out of memory");
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        sprintf(ds_flag, "-D%s", dataset);
        sprintf(incdir_src, "-I%s", src_dir);
        incdir_util = malloc(strlen(util_dir) + 3);
        if (incdir_util == NULL) {
            free(ds_flag);
            free(incdir_src);
            free(util_dir);
            free(src_path);
            free(util_pb);
            snprintf(error, error_size, "out of memory");
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        sprintf(incdir_util, "-I%s", util_dir);

        argv[idx++] = gcc;
        argv[idx++] = "-O2";
        argv[idx++] = "-fopenmp";
        argv[idx++] = "-std=c11";
        argv[idx++] = ds_flag;
        argv[idx++] = "-DPOLYBENCH_DUMP_ARRAYS";
        argv[idx++] = incdir_src;
        argv[idx++] = incdir_util;
        argv[idx++] = src_path;
        argv[idx++] = util_pb;
        argv[idx++] = "-lm";
        argv[idx++] = "-o";
        argv[idx++] = bin_path;
        argv[idx] = NULL;

        memset(&result, 0, sizeof(result));
        if (run_tool(ctx, "gcc", argv, ctx->work_dir, NULL, 0, &result) != 0) {
            snprintf(error, error_size, "cannot execute gcc");
            outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        } else if (result.timed_out || !result.exited ||
                   result.exit_code != 0) {
            snprintf(error, error_size, "gcc failed for %s (rc=%d)",
                     src_file, result.exit_code);
            outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        } else {
            outcome = BENCHMARK_ADAPTER_OK;
        }
        process_result_free(&result);
        free(incdir_util);
        free(util_dir);
        free(incdir_src);
        free(ds_flag);
    }

    free(util_pb);
    free(src_path);
    return outcome;
}

static BenchmarkAdapterOutcome polybench_build(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const char *dataset = resolve_dataset(ctx);
    const char *src_override = resolve_src_file(ctx->test_case->kernel);
    char *src_file = src_override ? strdup(src_override)
                                  : make_filename(ctx->test_case->kernel->rel_path, ".c");
    char *work_kernel = runner_path_join(ctx->work_dir, "kernel");
    char *work_baseline = runner_path_join(ctx->work_dir, "baseline");
    char *bin_transformed = NULL;
    char *bin_baseline = NULL;
    BenchmarkAdapterOutcome outcome;

    (void)context;
    if (src_file == NULL || work_kernel == NULL || work_baseline == NULL) {
        snprintf(error, error_size, "out of memory");
        free(src_file);
        free(work_kernel);
        free(work_baseline);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    {
        char *name = ctx->test_case->kernel->name;
        char *bin_dir = runner_path_join(ctx->work_dir, "bin");
        char xform_name[256];
        char base_name[256];
        if (bin_dir == NULL) {
            free(src_file);
            free(work_kernel);
            free(work_baseline);
            snprintf(error, error_size, "out of memory");
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        snprintf(xform_name, sizeof(xform_name), "%s.transformed", name);
        snprintf(base_name, sizeof(base_name), "%s.baseline", name);
        bin_transformed = runner_path_join(bin_dir, xform_name);
        bin_baseline = runner_path_join(bin_dir, base_name);
        free(bin_dir);
    }

    if (bin_transformed == NULL || bin_baseline == NULL) {
        snprintf(error, error_size, "out of memory");
        free(bin_transformed);
        free(bin_baseline);
        free(src_file);
        free(work_kernel);
        free(work_baseline);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    outcome = polybench_compile_one(ctx, work_kernel, src_file,
                                    bin_transformed, dataset, error,
                                    error_size);
    if (outcome != BENCHMARK_ADAPTER_OK) {
        free(bin_transformed);
        free(bin_baseline);
        free(src_file);
        free(work_kernel);
        free(work_baseline);
        return outcome;
    }

    outcome = polybench_compile_one(ctx, work_baseline, src_file,
                                    bin_baseline, dataset, error,
                                    error_size);
    free(bin_transformed);
    free(bin_baseline);
    free(src_file);
    free(work_kernel);
    free(work_baseline);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  verify                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome run_dump(const BenchmarkCaseContext *ctx,
                                        const char *binary,
                                        const char *dump_path,
                                        char *error,
                                        size_t error_size)
{
    static const RunnerEnvOverride env[] = {
        {"OMP_NUM_THREADS", "1"},
        {"OMP_DYNAMIC", "false"}};
    const char *argv[2];
    ProcessResult result;

    argv[0] = binary;
    argv[1] = NULL;
    memset(&result, 0, sizeof(result));
    if (run_tool(ctx, "polybench-run", argv, ctx->work_dir, env, 2, &result) != 0) {
        snprintf(error, error_size, "cannot execute %s",
                 runner_path_basename(binary));
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (result.timed_out || !result.exited || result.exit_code != 0) {
        snprintf(error, error_size, "binary %s failed (rc=%d)",
                 runner_path_basename(binary), result.exit_code);
        process_result_free(&result);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    /* write stderr dump to file */
    if (result.stderr_data != NULL) {
        runner_write_file(dump_path, result.stderr_data,
                          strlen(result.stderr_data));
    } else {
        runner_write_file(dump_path, "", 0);
    }
    process_result_free(&result);
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome polybench_verify(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    char *bin_dir = runner_path_join(ctx->work_dir, "bin");
    char *dump_dir = runner_path_join(ctx->work_dir, "dumps");
    char *base_dump = NULL;
    char *xform_dump = NULL;
    char *bin_base = NULL;
    char *bin_xform = NULL;
    const char *diff_tool;
    const char *diff_argv[5];
    ProcessResult result;
    BenchmarkAdapterOutcome outcome;
    char xform_name[256];
    char base_name[256];

    (void)context;
    snprintf(xform_name, sizeof(xform_name), "%s.transformed",
             ctx->test_case->kernel->name);
    snprintf(base_name, sizeof(base_name), "%s.baseline",
             ctx->test_case->kernel->name);

    if (bin_dir == NULL || dump_dir == NULL) {
        snprintf(error, error_size, "out of memory");
        free(bin_dir);
        free(dump_dir);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    bin_base = runner_path_join(bin_dir, base_name);
    bin_xform = runner_path_join(bin_dir, xform_name);
    base_dump = runner_path_join(dump_dir, "baseline.txt");
    xform_dump = runner_path_join(dump_dir, "transformed.txt");
    if (bin_base == NULL || bin_xform == NULL || base_dump == NULL ||
        xform_dump == NULL) {
        snprintf(error, error_size, "out of memory");
        goto cleanup_fail;
    }

    runner_mkdir_recursive(dump_dir, 0755);

    outcome = run_dump(ctx, bin_base, base_dump, error, error_size);
    if (outcome != BENCHMARK_ADAPTER_OK)
        goto cleanup;

    outcome = run_dump(ctx, bin_xform, xform_dump, error, error_size);
    if (outcome != BENCHMARK_ADAPTER_OK)
        goto cleanup;

    diff_tool = benchmark_config_path(ctx->config, "DIFF");
    if (diff_tool == NULL || diff_tool[0] == '\0')
        diff_tool = "diff";

    diff_argv[0] = diff_tool;
    diff_argv[1] = "-wB";
    diff_argv[2] = base_dump;
    diff_argv[3] = xform_dump;
    diff_argv[4] = NULL;

    memset(&result, 0, sizeof(result));
    if (run_tool(ctx, "dump-diff", diff_argv, NULL, NULL, 0, &result) != 0) {
        snprintf(error, error_size, "cannot execute diff");
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        goto cleanup;
    }
    if (result.timed_out || !result.exited || result.exit_code > 1) {
        snprintf(error, error_size, "diff tool error");
        process_result_free(&result);
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        goto cleanup;
    }
    if (result.exit_code == 1) {
        snprintf(error, error_size,
                 "PolyBench dump mismatch (baseline vs transformed)");
        process_result_free(&result);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        goto cleanup;
    }
    process_result_free(&result);
    outcome = BENCHMARK_ADAPTER_OK;
    goto cleanup;

cleanup_fail:
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
cleanup:
    free(xform_dump);
    free(base_dump);
    free(bin_xform);
    free(bin_base);
    free(dump_dir);
    free(bin_dir);
    return outcome;
}

static void polybench_cleanup(void *context,
                              const BenchmarkCaseContext *ctx)
{
    (void)context;
    (void)ctx;
}

const BenchmarkAdapterVTable polybench_adapter_vtable = {
    polybench_expected_gt_names,
    polybench_prepare,
    polybench_cetus_transform,
    polybench_inject,
    polybench_build,
    polybench_verify,
    polybench_cleanup};
