#define _POSIX_C_SOURCE 200809L

#include "benchmark_adapters.h"
#include "benchmark_config.h"
#include "benchmark_runner.h"
#include "runner_common.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *npb_common_always[] = {
    "print_results.c", "c_print_results.c", "c_timers.c",
    "wtime.c",         "c_wtime.c",         "wtime.h",
    "timers.h",        "type.h",            "randdp.c",
    "randdp.h",        "randi8.c",          "print_results.h"};

static const char *resolve_root(const BenchmarkCaseContext *ctx)
{
    const BenchmarkCase *tc = ctx->test_case;
    const char *env = tc->kernel->root_env != NULL ? tc->kernel->root_env
                                                   : tc->suite->root_env;
    return benchmark_config_path(ctx->config, env);
}

static const char *resolve_class(const BenchmarkCaseContext *ctx)
{
    const char *cls = ctx->test_case->suite->npb_class;
    return (cls != NULL && cls[0] != '\0') ? cls : "S";
}

static int run_tool(const BenchmarkCaseContext *ctx,
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
    spec.timeout_seconds = (unsigned int)ctx->timeout_seconds;
    rc = ctx->process_run(&spec, result, ctx->process_context);
    benchmark_log_process(ctx->log_path, label, argv, rc == 0 ? result : NULL);
    return rc;
}

static int contains_ci(const char *haystack, const char *needle)
{
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    size_t i, j;
    for (i = 0; i + nlen <= hlen; ++i) {
        int match = 1;
        for (j = 0; j < nlen; ++j) {
            if (tolower((unsigned char)haystack[i + j]) !=
                tolower((unsigned char)needle[j])) {
                match = 0;
                break;
            }
        }
        if (match)
            return 1;
    }
    return 0;
}

static int is_experiment_junk(const char *name)
{
    return contains_ci(name, "rose_") || contains_ci(name, "llm") ||
           contains_ci(name, "carv") || contains_ci(name, "cetus");
}

static int in_string_set(const char *name, const char *const *set,
                         size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(name, set[i]) == 0)
            return 1;
    }
    return 0;
}

static int has_ext(const char *name, const char *ext)
{
    size_t nlen = strlen(name);
    size_t elen = strlen(ext);
    return nlen > elen && strcmp(name + nlen - elen, ext) == 0;
}

static int in_vector(const RunnerStringVector *v, const char *name)
{
    size_t i;
    for (i = 0; i < v->length; ++i) {
        if (strcmp(v->items[i], name) == 0)
            return 1;
    }
    return 0;
}

/* Copy config/ files with .def, .template, .h extensions or known names */
static int copy_config_dir(const char *src, const char *dst)
{
    DIR *dir;
    struct dirent *entry;
    static const char *allow_names[] = {"make.def", "make.def.template",
                                        "suite.def", "suite.def.template"};
    if (!runner_file_exists(src))
        return 0;
    dir = opendir(src);
    if (dir == NULL)
        return -1;
    if (runner_mkdir_recursive(dst, 0755) != 0) {
        closedir(dir);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        if (has_ext(entry->d_name, ".def") ||
            has_ext(entry->d_name, ".template") ||
            has_ext(entry->d_name, ".h") ||
            in_string_set(entry->d_name, allow_names, 4)) {
            char *s = runner_path_join(src, entry->d_name);
            char *d = runner_path_join(dst, entry->d_name);
            int rc;
            if (s == NULL || d == NULL) {
                free(s);
                free(d);
                closedir(dir);
                return -1;
            }
            rc = runner_copy_file(s, d);
            free(s);
            free(d);
            if (rc != 0) {
                closedir(dir);
                return -1;
            }
        }
    }
    closedir(dir);
    return 0;
}

/* Copy sys/ directory filtering *.o, *.x, .git */
static int npb_sys_filter(const char *relative, int is_dir, void *context)
{
    const char *base;
    (void)context;
    base = runner_path_basename(relative);
    if (is_dir)
        return strcmp(base, ".git") != 0;
    return !has_ext(base, ".o") && !has_ext(base, ".x");
}

/* Copy common/ with canonical support files, filtering experiment junk */
static int copy_common_dir(const char *src, const char *dst)
{
    DIR *dir;
    struct dirent *entry;
    size_t common_count =
        sizeof(npb_common_always) / sizeof(npb_common_always[0]);

    if (!runner_file_exists(src))
        return 0;
    dir = opendir(src);
    if (dir == NULL)
        return -1;
    if (runner_mkdir_recursive(dst, 0755) != 0) {
        closedir(dir);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        struct stat st;
        char *s;
        char *d;
        int rc;
        if (name[0] == '.')
            continue;
        s = runner_path_join(src, name);
        if (s == NULL) {
            closedir(dir);
            return -1;
        }
        if (stat(s, &st) != 0 || !S_ISREG(st.st_mode)) {
            free(s);
            continue;
        }
        if (!has_ext(name, ".h") && !has_ext(name, ".c")) {
            free(s);
            continue;
        }
        if (is_experiment_junk(name)) {
            free(s);
            continue;
        }
        if (!in_string_set(name, npb_common_always, common_count) &&
            !has_ext(name, ".h")) {
            free(s);
            continue;
        }
        d = runner_path_join(dst, name);
        if (d == NULL) {
            free(s);
            closedir(dir);
            return -1;
        }
        rc = runner_copy_file(s, d);
        free(s);
        free(d);
        if (rc != 0) {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

/* Copy bench directory: Makefile, listed sources, headers, .h, .incl */
static int copy_bench_dir(const char *src, const char *dst,
                          const BenchmarkKernel *kernel)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(src);
    if (dir == NULL)
        return -1;
    if (runner_mkdir_recursive(dst, 0755) != 0) {
        closedir(dir);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        struct stat st;
        char *s;
        char *d;
        int rc;
        int needed;
        if (name[0] == '.')
            continue;
        s = runner_path_join(src, name);
        if (s == NULL) {
            closedir(dir);
            return -1;
        }
        if (stat(s, &st) != 0 || !S_ISREG(st.st_mode)) {
            free(s);
            continue;
        }
        needed = strcmp(name, "Makefile") == 0 ||
                 in_vector(&kernel->sources, name) ||
                 in_vector(&kernel->headers, name) ||
                 has_ext(name, ".h") || has_ext(name, ".incl");
        if (!needed) {
            free(s);
            continue;
        }
        if (is_experiment_junk(name) &&
            !in_vector(&kernel->sources, name) &&
            !in_vector(&kernel->headers, name)) {
            free(s);
            continue;
        }
        d = runner_path_join(dst, name);
        if (d == NULL) {
            free(s);
            closedir(dir);
            return -1;
        }
        rc = runner_copy_file(s, d);
        free(s);
        free(d);
        if (rc != 0) {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

/* Remove all .o files in a directory */
static void remove_objects(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    struct dirent *entry;
    if (dir == NULL)
        return;
    while ((entry = readdir(dir)) != NULL) {
        if (has_ext(entry->d_name, ".o")) {
            char *path = runner_path_join(dir_path, entry->d_name);
            if (path != NULL) {
                unlink(path);
                free(path);
            }
        }
    }
    closedir(dir);
}

/* ------------------------------------------------------------------ */
/*  expected_gt_names                                                 */
/* ------------------------------------------------------------------ */

static const RunnerStringVector *resolve_cetus_sources(
    const BenchmarkKernel *kernel)
{
    if (kernel->cetus_sources.length > 0)
        return &kernel->cetus_sources;
    return &kernel->sources;
}

static int npb_expected_gt_names(void *context,
                                 const BenchmarkCaseContext *ctx,
                                 RunnerStringVector *names,
                                 char *error,
                                 size_t error_size)
{
    const RunnerStringVector *sources =
        resolve_cetus_sources(ctx->test_case->kernel);
    size_t i;
    (void)context;
    for (i = 0; i < sources->length; ++i) {
        if (runner_string_vector_push(names, sources->items[i]) != 0) {
            snprintf(error, error_size, "out of memory");
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  prepare                                                           */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome npb_prepare(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const char *root = resolve_root(ctx);
    const char *npb_class = resolve_class(ctx);
    const BenchmarkKernel *kernel = ctx->test_case->kernel;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *work_original = runner_path_join(ctx->work_dir, "original");
    char *work_cetus = runner_path_join(ctx->work_dir, "cetus_output");
    char *root_mk = NULL;
    char *sandbox_mk = NULL;
    char *root_config = NULL;
    char *sandbox_config = NULL;
    char *root_sys = NULL;
    char *sandbox_sys = NULL;
    char *root_common = NULL;
    char *sandbox_common = NULL;
    char *root_bench = NULL;
    char *sandbox_bench = NULL;
    char *sandbox_bin = NULL;
    size_t i;
    BenchmarkAdapterOutcome outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;

    (void)context;
    if (sandbox == NULL || work_original == NULL || work_cetus == NULL)
        goto oom;

    if (runner_file_exists(sandbox))
        runner_remove_recursive(sandbox);
    if (runner_mkdir_recursive(sandbox, 0755) != 0 ||
        runner_mkdir_recursive(work_original, 0755) != 0 ||
        runner_mkdir_recursive(work_cetus, 0755) != 0)
        goto infra;

    /* copy root Makefile */
    root_mk = runner_path_join(root, "Makefile");
    sandbox_mk = runner_path_join(sandbox, "Makefile");
    if (root_mk == NULL || sandbox_mk == NULL)
        goto oom;
    if (runner_file_exists(root_mk) &&
        runner_copy_file(root_mk, sandbox_mk) != 0)
        goto infra;

    /* copy config/ */
    root_config = runner_path_join(root, "config");
    sandbox_config = runner_path_join(sandbox, "config");
    if (root_config == NULL || sandbox_config == NULL)
        goto oom;
    if (copy_config_dir(root_config, sandbox_config) != 0) {
        snprintf(error, error_size, "cannot copy NPB config directory");
        goto done;
    }

    /* copy sys/ */
    root_sys = runner_path_join(root, "sys");
    sandbox_sys = runner_path_join(sandbox, "sys");
    if (root_sys == NULL || sandbox_sys == NULL)
        goto oom;
    if (runner_file_exists(root_sys) &&
        runner_copy_tree(root_sys, sandbox_sys, npb_sys_filter, NULL) != 0) {
        snprintf(error, error_size, "cannot copy NPB sys directory");
        goto done;
    }

    /* copy common/ */
    root_common = runner_path_join(root, "common");
    sandbox_common = runner_path_join(sandbox, "common");
    if (root_common == NULL || sandbox_common == NULL)
        goto oom;
    if (copy_common_dir(root_common, sandbox_common) != 0) {
        snprintf(error, error_size, "cannot copy NPB common directory");
        goto done;
    }

    /* copy bench directory */
    root_bench = runner_path_join(root, kernel->bench);
    sandbox_bench = runner_path_join(sandbox, kernel->bench);
    if (root_bench == NULL || sandbox_bench == NULL)
        goto oom;
    if (copy_bench_dir(root_bench, sandbox_bench, kernel) != 0) {
        snprintf(error, error_size, "cannot copy NPB bench directory %s",
                 kernel->bench);
        goto done;
    }

    /* verify all required sources exist */
    for (i = 0; i < kernel->sources.length; ++i) {
        char *path = runner_path_join(sandbox_bench, kernel->sources.items[i]);
        if (path == NULL)
            goto oom;
        if (!runner_file_exists(path)) {
            snprintf(error, error_size, "missing NPB source: %s",
                     kernel->sources.items[i]);
            free(path);
            goto done;
        }
        free(path);
    }

    /* create bin/ */
    sandbox_bin = runner_path_join(sandbox, "bin");
    if (sandbox_bin == NULL)
        goto oom;
    runner_mkdir_recursive(sandbox_bin, 0755);

    /* snapshot originals */
    for (i = 0; i < kernel->sources.length; ++i) {
        char *src = runner_path_join(sandbox_bench, kernel->sources.items[i]);
        char *dst = runner_path_join(work_original, kernel->sources.items[i]);
        if (src == NULL || dst == NULL) {
            free(src);
            free(dst);
            goto oom;
        }
        if (runner_copy_file(src, dst) != 0) {
            snprintf(error, error_size, "cannot snapshot source %s",
                     kernel->sources.items[i]);
            free(src);
            free(dst);
            goto done;
        }
        free(src);
        free(dst);
    }

    /* generate npbparams.h via make (allow failure) */
    {
        const char *argv[5];
        ProcessResult result;
        argv[0] = "make";
        argv[1] = kernel->binary;
        {
            char class_arg[32];
            snprintf(class_arg, sizeof(class_arg), "CLASS=%s", npb_class);
            argv[2] = class_arg;
            argv[3] = NULL;
            memset(&result, 0, sizeof(result));
            run_tool(ctx, "make-params", argv, sandbox, &result);
            process_result_free(&result);
        }
    }

    /* check npbparams.h; if missing, try building setparams */
    {
        char *params = runner_path_join(sandbox_bench, "npbparams.h");
        if (params == NULL)
            goto oom;
        if (!runner_file_exists(params)) {
            char *sys_dir = runner_path_join(sandbox, "sys");
            if (sys_dir != NULL) {
                const char *make_argv[2] = {"make", NULL};
                ProcessResult make_result;
                memset(&make_result, 0, sizeof(make_result));
                run_tool(ctx, "make-setparams", make_argv, sys_dir, &make_result);
                process_result_free(&make_result);

                {
                    char *setparams = runner_path_join(sys_dir, "setparams");
                    if (setparams != NULL && runner_file_exists(setparams)) {
                        const char *sp_argv[4];
                        ProcessResult sp_result;
                        sp_argv[0] = setparams;
                        sp_argv[1] = kernel->binary;
                        sp_argv[2] = npb_class;
                        sp_argv[3] = NULL;
                        memset(&sp_result, 0, sizeof(sp_result));
                        run_tool(ctx, "setparams", sp_argv, sandbox_bench, &sp_result);
                        process_result_free(&sp_result);
                    }
                    free(setparams);
                }
                free(sys_dir);
            }
        }
        free(params);
    }

    outcome = BENCHMARK_ADAPTER_OK;
    goto done;

oom:
    snprintf(error, error_size, "out of memory in NPB prepare");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    goto done;
infra:
    snprintf(error, error_size, "cannot create work directories");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
done:
    free(sandbox_bin);
    free(sandbox_bench);
    free(root_bench);
    free(sandbox_common);
    free(root_common);
    free(sandbox_sys);
    free(root_sys);
    free(sandbox_config);
    free(root_config);
    free(sandbox_mk);
    free(root_mk);
    free(work_cetus);
    free(work_original);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  cetus_transform                                                   */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome npb_cetus_transform(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *flags,
    RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    const BenchmarkKernel *kernel = ctx->test_case->kernel;
    const RunnerStringVector *sources = resolve_cetus_sources(kernel);
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *sandbox_bench = NULL;
    char *sandbox_common = NULL;
    char *work_original = runner_path_join(ctx->work_dir, "original");
    char *work_cetus = runner_path_join(ctx->work_dir, "cetus_output");
    RunnerString prep;
    const char *cpp;
    BenchmarkAdapterOutcome outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    size_t i;

    (void)context;
    runner_string_init(&prep);

    if (sandbox == NULL || work_original == NULL || work_cetus == NULL)
        goto oom;

    sandbox_bench = runner_path_join(sandbox, kernel->bench);
    sandbox_common = runner_path_join(sandbox, "common");
    if (sandbox_bench == NULL || sandbox_common == NULL)
        goto oom;

    cpp = benchmark_config_path(ctx->config, "CPP");
    if (cpp == NULL || cpp[0] == '\0')
        cpp = "cpp";
    if (runner_string_append(&prep, cpp) != 0 ||
        runner_string_append(&prep, " -C -I. -I") != 0 ||
        runner_string_append(&prep, sandbox_bench) != 0 ||
        runner_string_append(&prep, " -I") != 0 ||
        runner_string_append(&prep, sandbox_common) != 0)
        goto oom;

    for (i = 0; i < sources->length; ++i) {
        const char *src_name = sources->items[i];
        char cetus_run_name[256];
        char *outdir = NULL;
        char *outdir_output = NULL;
        char *staged = NULL;
        char *orig_src = NULL;
        char *generated = NULL;
        char *dest = NULL;
        const char **argv = NULL;
        size_t argc;
        ProcessResult result;
        size_t idx;

        /* replace dots for unique dir name */
        {
            size_t j;
            snprintf(cetus_run_name, sizeof(cetus_run_name), "%s", src_name);
            for (j = 0; cetus_run_name[j]; ++j) {
                if (cetus_run_name[j] == '.')
                    cetus_run_name[j] = '_';
            }
        }

        {
            char *cetus_run = runner_path_join(ctx->work_dir, "cetus_run");
            if (cetus_run == NULL)
                goto oom;
            outdir = runner_path_join(cetus_run, cetus_run_name);
            free(cetus_run);
        }
        if (outdir == NULL)
            goto oom;
        if (runner_file_exists(outdir))
            runner_remove_recursive(outdir);
        if (runner_mkdir_recursive(outdir, 0755) != 0) {
            free(outdir);
            goto infra;
        }

        orig_src = runner_path_join(work_original, src_name);
        staged = runner_path_join(outdir, src_name);
        outdir_output = runner_path_join(outdir, "cetus_output");
        if (orig_src == NULL || staged == NULL || outdir_output == NULL) {
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto oom;
        }

        if (runner_copy_file(orig_src, staged) != 0) {
            snprintf(error, error_size, "cannot stage %s", src_name);
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto done;
        }

        argc = 3 + flags->length + 1 + 1;
        argv = calloc(argc, sizeof(*argv));
        if (argv == NULL) {
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto oom;
        }
        {
            RunnerString outdir_arg;
            RunnerString prep_arg;
            runner_string_init(&outdir_arg);
            runner_string_init(&prep_arg);
            if (runner_string_append(&outdir_arg, "-outdir=") != 0 ||
                runner_string_append(&outdir_arg, outdir_output) != 0 ||
                runner_string_append(&prep_arg, "-preprocessor=") != 0 ||
                runner_string_append(&prep_arg, prep.data) != 0) {
                runner_string_free(&outdir_arg);
                runner_string_free(&prep_arg);
                free(argv);
                free(orig_src);
                free(staged);
                free(outdir_output);
                free(outdir);
                goto oom;
            }
            idx = 0;
            argv[idx++] = ctx->cetus;
            argv[idx++] = runner_string_take(&outdir_arg);
            argv[idx++] = runner_string_take(&prep_arg);
            for (size_t fi = 0; fi < flags->length; ++fi)
                argv[idx++] = flags->items[fi];
            argv[idx++] = src_name;
            argv[idx] = NULL;
        }

        memset(&result, 0, sizeof(result));
        if (run_tool(ctx, "cetus", argv, outdir, &result) != 0) {
            snprintf(error, error_size, "cannot execute Cetus for %s",
                     src_name);
            free((char *)argv[1]);
            free((char *)argv[2]);
            free(argv);
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto done;
        }
        if (result.timed_out || !result.exited || result.exit_code != 0) {
            snprintf(error, error_size,
                     "Cetus failed on %s (rc=%d)", src_name,
                     result.exit_code);
            outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
            process_result_free(&result);
            free((char *)argv[1]);
            free((char *)argv[2]);
            free(argv);
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto done;
        }
        process_result_free(&result);

        generated = runner_path_join(outdir_output, src_name);
        dest = runner_path_join(work_cetus, src_name);
        if (generated == NULL || dest == NULL ||
            !runner_file_exists(generated)) {
            if (generated != NULL && !runner_file_exists(generated))
                snprintf(error, error_size,
                         "Cetus produced no output for %s", src_name);
            else
                snprintf(error, error_size, "out of memory");
            outcome = generated != NULL && !runner_file_exists(generated)
                          ? BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE
                          : BENCHMARK_ADAPTER_INFRA_FAILURE;
            free(generated);
            free(dest);
            free((char *)argv[1]);
            free((char *)argv[2]);
            free(argv);
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto done;
        }
        if (runner_copy_file(generated, dest) != 0 ||
            runner_string_vector_push(outputs, dest) != 0) {
            snprintf(error, error_size, "cannot collect output for %s",
                     src_name);
            free(generated);
            free(dest);
            free((char *)argv[1]);
            free((char *)argv[2]);
            free(argv);
            free(orig_src);
            free(staged);
            free(outdir_output);
            free(outdir);
            goto done;
        }
        free(generated);
        free(dest);
        free((char *)argv[1]);
        free((char *)argv[2]);
        free(argv);
        free(orig_src);
        free(staged);
        free(outdir_output);
        free(outdir);
    }
    outcome = BENCHMARK_ADAPTER_OK;
    goto done;

oom:
    snprintf(error, error_size, "out of memory in NPB cetus_transform");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    goto done;
infra:
    snprintf(error, error_size, "cannot create staging directories");
    outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
done:
    runner_string_free(&prep);
    free(sandbox_common);
    free(sandbox_bench);
    free(work_cetus);
    free(work_original);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  inject                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome npb_inject(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    const BenchmarkKernel *kernel = ctx->test_case->kernel;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *sandbox_bench = NULL;
    char *sandbox_common = NULL;
    size_t i;

    (void)context;
    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    sandbox_bench = runner_path_join(sandbox, kernel->bench);
    sandbox_common = runner_path_join(sandbox, "common");
    if (sandbox_bench == NULL || sandbox_common == NULL) {
        free(sandbox_bench);
        free(sandbox_common);
        free(sandbox);
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    for (i = 0; i < outputs->length; ++i) {
        const char *name = runner_path_basename(outputs->items[i]);
        char *dst = runner_path_join(sandbox_bench, name);
        if (dst == NULL || runner_copy_file(outputs->items[i], dst) != 0) {
            snprintf(error, error_size, "cannot inject %s", name);
            free(dst);
            free(sandbox_common);
            free(sandbox_bench);
            free(sandbox);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        free(dst);
    }

    remove_objects(sandbox_bench);
    remove_objects(sandbox_common);

    free(sandbox_common);
    free(sandbox_bench);
    free(sandbox);
    return BENCHMARK_ADAPTER_OK;
}

/* ------------------------------------------------------------------ */
/*  build                                                             */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome npb_build(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const BenchmarkKernel *kernel = ctx->test_case->kernel;
    const char *npb_class = resolve_class(ctx);
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *bindir = NULL;
    char class_arg[32];
    char bindir_arg[1024];
    const char *cflags =
        "-g -Wall -O2 -fopenmp -mcmodel=large "
        "-include ../common/type.h -Wno-unknown-pragmas";
    const char *clink = "-O2 -fopenmp -mcmodel=large";
    char cflags_arg[512];
    char clink_arg[256];
    const char *argv[7];
    ProcessResult result;
    BenchmarkAdapterOutcome outcome;

    (void)context;
    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    bindir = runner_path_join(sandbox, "bin");
    if (bindir == NULL) {
        free(sandbox);
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    runner_mkdir_recursive(bindir, 0755);

    snprintf(class_arg, sizeof(class_arg), "CLASS=%s", npb_class);
    snprintf(bindir_arg, sizeof(bindir_arg), "BINDIR=%s", bindir);
    snprintf(cflags_arg, sizeof(cflags_arg), "CFLAGS=%s", cflags);
    snprintf(clink_arg, sizeof(clink_arg), "CLINKFLAGS=%s", clink);

    argv[0] = "make";
    argv[1] = kernel->binary;
    argv[2] = class_arg;
    argv[3] = bindir_arg;
    argv[4] = cflags_arg;
    argv[5] = clink_arg;
    argv[6] = NULL;

    memset(&result, 0, sizeof(result));
    if (run_tool(ctx, "make", argv, sandbox, &result) != 0) {
        snprintf(error, error_size, "cannot execute make");
        free(bindir);
        free(sandbox);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (result.timed_out || !result.exited || result.exit_code != 0) {
        snprintf(error, error_size, "make failed (rc=%d)",
                 result.exit_code);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    } else {
        outcome = BENCHMARK_ADAPTER_OK;
    }
    process_result_free(&result);
    free(bindir);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  verify                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome npb_verify(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const BenchmarkKernel *kernel = ctx->test_case->kernel;
    const char *npb_class = resolve_class(ctx);
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *bindir = NULL;
    char binary_name[256];
    char *binary = NULL;
    char *alt_binary = NULL;
    const char *bin_path;
    const char *argv[2];
    ProcessResult result;
    char *combined = NULL;
    regex_t regex1;
    regex_t regex2;
    int compiled1 = 0;
    int compiled2 = 0;
    BenchmarkAdapterOutcome outcome;

    (void)context;
    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    bindir = runner_path_join(sandbox, "bin");
    if (bindir == NULL) {
        free(sandbox);
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    snprintf(binary_name, sizeof(binary_name), "%s.%s.x",
             kernel->binary, npb_class);
    binary = runner_path_join(bindir, binary_name);
    if (binary == NULL) {
        free(bindir);
        free(sandbox);
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    bin_path = binary;
    if (!runner_file_exists(binary)) {
        char *bench_dir = runner_path_join(sandbox, kernel->bench);
        if (bench_dir != NULL) {
            alt_binary = runner_path_join(bench_dir, binary_name);
            free(bench_dir);
        }
        if (alt_binary != NULL && runner_file_exists(alt_binary)) {
            bin_path = alt_binary;
        } else {
            snprintf(error, error_size, "binary not found: %s", binary);
            free(alt_binary);
            free(binary);
            free(bindir);
            free(sandbox);
            return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        }
    }

    argv[0] = bin_path;
    argv[1] = NULL;
    memset(&result, 0, sizeof(result));
    if (run_tool(ctx, "npb-run", argv, sandbox, &result) != 0) {
        snprintf(error, error_size, "cannot execute NPB binary");
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        goto cleanup;
    }
    if (result.timed_out || !result.exited || result.exit_code != 0) {
        snprintf(error, error_size, "NPB binary failed (rc=%d)",
                 result.exit_code);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
        goto cleanup;
    }

    /* combine stdout + stderr */
    {
        RunnerString combo;
        runner_string_init(&combo);
        if (result.stdout_data)
            runner_string_append(&combo, result.stdout_data);
        if (result.stderr_data)
            runner_string_append(&combo, result.stderr_data);
        combined = runner_string_take(&combo);
    }

    /* write run output */
    {
        char *out_path = runner_path_join(ctx->work_dir, "run_output.txt");
        if (out_path != NULL && combined != NULL)
            runner_write_file(out_path, combined, strlen(combined));
        free(out_path);
    }

    /* check verification pattern (required for PASS) */
    if (regcomp(&regex1, "verification[[:space:]]*=[[:space:]]*successful",
                REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
        snprintf(error, error_size, "cannot compile verification regex");
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        goto cleanup;
    }
    compiled1 = 1;
    if (regcomp(&regex2, "verification[[:space:]]+successful",
                REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
        snprintf(error, error_size, "cannot compile verification regex");
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        goto cleanup;
    }
    compiled2 = 1;

    if (combined != NULL &&
        (regexec(&regex1, combined, 0, NULL, 0) == 0 ||
         regexec(&regex2, combined, 0, NULL, 0) == 0)) {
        /* Capture the matching program line for the SUCCESS message. */
        const char *line = combined;
        const char *end;
        size_t len;
        char captured[256];
        error[0] = '\0';
        while (*line) {
            end = line;
            while (*end && *end != '\n' && *end != '\r')
                ++end;
            len = (size_t)(end - line);
            if (len > 0 && len < sizeof(captured)) {
                char lower[256];
                size_t i;
                memcpy(captured, line, len);
                captured[len] = '\0';
                for (i = 0; i <= len; ++i) {
                    lower[i] = (char)tolower((unsigned char)captured[i]);
                }
                if (strstr(lower, "verification") != NULL &&
                    strstr(lower, "successful") != NULL) {
                    /* Trim leading whitespace for display. */
                    const char *trim = captured;
                    while (*trim == ' ' || *trim == '\t')
                        ++trim;
                    snprintf(error, error_size, "%s", trim);
                    break;
                }
            }
            line = (*end) ? end + 1 : end;
        }
        if (error[0] == '\0')
            snprintf(error, error_size, "Verification Successful");
        outcome = BENCHMARK_ADAPTER_OK;
    } else {
        snprintf(error, error_size,
                 "NPB verification string not found in output");
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }

cleanup:
    if (compiled2)
        regfree(&regex2);
    if (compiled1)
        regfree(&regex1);
    free(combined);
    process_result_free(&result);
    free(alt_binary);
    free(binary);
    free(bindir);
    free(sandbox);
    return outcome;
}

static void npb_cleanup(void *context, const BenchmarkCaseContext *ctx)
{
    (void)context;
    (void)ctx;
}

const BenchmarkAdapterVTable npb_adapter_vtable = {
    npb_expected_gt_names, npb_prepare,   npb_cetus_transform,
    npb_inject,            npb_build,     npb_verify,
    npb_cleanup};
