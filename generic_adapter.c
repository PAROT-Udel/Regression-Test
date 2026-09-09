#define _POSIX_C_SOURCE 200809L

#include "benchmark_adapters.h"
#include "benchmark_config.h"
#include "benchmark_runner.h"
#include "runner_common.h"

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *resolve_root(const BenchmarkCaseContext *ctx)
{
    const BenchmarkCase *tc = ctx->test_case;
    const char *env = tc->kernel->root_env != NULL ? tc->kernel->root_env
                                                   : tc->suite->root_env;
    return benchmark_config_path(ctx->config, env);
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

static void fill_template_values(const BenchmarkCaseContext *ctx,
                                 BenchmarkTemplateValues *values)
{
    memset(values, 0, sizeof(*values));
    values->root = resolve_root(ctx);
    values->cpp = benchmark_config_path(ctx->config, "CPP");
    if (values->cpp == NULL || values->cpp[0] == '\0')
        values->cpp = "cpp";
    values->gcc = benchmark_config_path(ctx->config, "GCC");
    if (values->gcc == NULL || values->gcc[0] == '\0')
        values->gcc = "gcc";
    values->dataset = ctx->test_case->suite->dataset;
    if (values->dataset == NULL)
        values->dataset = "";
    values->name = ctx->test_case->kernel->name;
    values->work = ctx->work_dir;
    /* sandbox = work_dir/sandbox */
    values->sandbox = NULL; /* set separately after path_join */
}

/* Build expanded argv from a BenchmarkCommand using template values */
static int expand_command_argv(const BenchmarkCommand *cmd,
                               const BenchmarkTemplateValues *values,
                               const char ***out_argv,
                               size_t *out_argc,
                               char *error,
                               size_t error_size)
{
    size_t count = cmd->argv.length;
    const char **argv;
    size_t i;

    argv = calloc(count + 1, sizeof(*argv));
    if (argv == NULL) {
        snprintf(error, error_size, "out of memory");
        return -1;
    }
    for (i = 0; i < count; ++i) {
        char *expanded = NULL;
        if (benchmark_expand_template(cmd->argv.items[i], values, &expanded,
                                      error, error_size) != 0) {
            while (i > 0)
                free((char *)argv[--i]);
            free(argv);
            return -1;
        }
        argv[i] = expanded;
    }
    argv[count] = NULL;
    *out_argv = argv;
    *out_argc = count;
    return 0;
}

static void free_expanded_argv(const char **argv, size_t argc)
{
    size_t i;
    if (argv == NULL)
        return;
    for (i = 0; i < argc; ++i)
        free((char *)argv[i]);
    free(argv);
}

static int has_ext(const char *name, const char *ext)
{
    size_t nlen = strlen(name);
    size_t elen = strlen(ext);
    return nlen > elen && strcmp(name + nlen - elen, ext) == 0;
}

/* Copy filter for generic adapter: skip build artifacts */
static int generic_copy_filter(const char *relative, int is_dir,
                               void *context)
{
    const char *base;
    (void)context;
    base = runner_path_basename(relative);
    if (is_dir)
        return strcmp(base, ".git") != 0;
    return !has_ext(base, ".o") && !has_ext(base, ".x");
}

/* ------------------------------------------------------------------ */
/*  expected_gt_names                                                 */
/* ------------------------------------------------------------------ */

static int generic_expected_gt_names(void *context,
                                     const BenchmarkCaseContext *ctx,
                                     RunnerStringVector *names,
                                     char *error,
                                     size_t error_size)
{
    BenchmarkGenericView view;
    size_t i;
    (void)context;
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);
    if (view.cetus_inputs == NULL || view.cetus_inputs->length == 0) {
        snprintf(error, error_size, "generic adapter: no cetus_inputs");
        return -1;
    }
    for (i = 0; i < view.cetus_inputs->length; ++i) {
        const char *base =
            runner_path_basename(view.cetus_inputs->items[i]);
        if (runner_string_vector_push(names, base) != 0) {
            snprintf(error, error_size, "out of memory");
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  prepare                                                           */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome generic_prepare(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    const char *root = resolve_root(ctx);
    BenchmarkGenericView view;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *work_original = runner_path_join(ctx->work_dir, "original");
    char *work_cetus = runner_path_join(ctx->work_dir, "cetus_output");
    size_t i;
    BenchmarkAdapterOutcome outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;

    (void)context;
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);

    if (sandbox == NULL || work_original == NULL || work_cetus == NULL) {
        snprintf(error, error_size, "out of memory");
        goto done;
    }
    if (runner_file_exists(sandbox))
        runner_remove_recursive(sandbox);
    if (runner_mkdir_recursive(sandbox, 0755) != 0 ||
        runner_mkdir_recursive(work_original, 0755) != 0 ||
        runner_mkdir_recursive(work_cetus, 0755) != 0) {
        snprintf(error, error_size, "cannot create work directories");
        goto done;
    }

    /* copy configured files/dirs from root to sandbox */
    if (view.copy != NULL) {
        for (i = 0; i < view.copy->length; ++i) {
            char *src = runner_path_join(root, view.copy->items[i]);
            char *dst = runner_path_join(sandbox, view.copy->items[i]);
            struct stat st;

            if (src == NULL || dst == NULL) {
                free(src);
                free(dst);
                snprintf(error, error_size, "out of memory");
                goto done;
            }

            if (stat(src, &st) != 0) {
                snprintf(error, error_size, "copy path missing: %s", src);
                free(src);
                free(dst);
                goto done;
            }

            if (S_ISDIR(st.st_mode)) {
                if (runner_copy_tree(src, dst, generic_copy_filter,
                                     NULL) != 0) {
                    snprintf(error, error_size, "cannot copy directory %s",
                             view.copy->items[i]);
                    free(src);
                    free(dst);
                    goto done;
                }
            } else {
                /* ensure parent exists */
                char *parent = strdup(dst);
                char *slash;
                if (parent != NULL) {
                    slash = strrchr(parent, '/');
                    if (slash != NULL) {
                        *slash = '\0';
                        runner_mkdir_recursive(parent, 0755);
                    }
                    free(parent);
                }
                if (runner_copy_file(src, dst) != 0) {
                    snprintf(error, error_size, "cannot copy %s",
                             view.copy->items[i]);
                    free(src);
                    free(dst);
                    goto done;
                }
            }
            free(src);
            free(dst);
        }
    }

    /* snapshot cetus inputs to work/original */
    if (view.cetus_inputs != NULL) {
        for (i = 0; i < view.cetus_inputs->length; ++i) {
            const char *rel = view.cetus_inputs->items[i];
            const char *base = runner_path_basename(rel);
            char *src = runner_path_join(sandbox, rel);
            char *dst = runner_path_join(work_original, base);
            if (src == NULL || dst == NULL) {
                free(src);
                free(dst);
                snprintf(error, error_size, "out of memory");
                goto done;
            }
            if (!runner_file_exists(src)) {
                snprintf(error, error_size,
                         "cetus input missing after copy: %s", rel);
                free(src);
                free(dst);
                goto done;
            }
            if (runner_copy_file(src, dst) != 0) {
                snprintf(error, error_size,
                         "cannot snapshot cetus input %s", rel);
                free(src);
                free(dst);
                goto done;
            }
            free(src);
            free(dst);
        }
    }
    outcome = BENCHMARK_ADAPTER_OK;

done:
    free(work_cetus);
    free(work_original);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  cetus_transform                                                   */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome generic_cetus_transform(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *flags,
    RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    BenchmarkGenericView view;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    char *work_original = runner_path_join(ctx->work_dir, "original");
    char *work_cetus = runner_path_join(ctx->work_dir, "cetus_output");
    BenchmarkTemplateValues values;
    RunnerString prep;
    BenchmarkAdapterOutcome outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    size_t i;

    (void)context;
    runner_string_init(&prep);
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);
    fill_template_values(ctx, &values);
    values.sandbox = sandbox;

    if (sandbox == NULL || work_original == NULL || work_cetus == NULL) {
        snprintf(error, error_size, "out of memory");
        goto done;
    }

    /* build preprocessor string from template */
    if (view.preprocessor != NULL) {
        char *expanded = NULL;
        if (benchmark_expand_template(view.preprocessor, &values, &expanded,
                                      error, error_size) != 0) {
            goto done;
        }
        runner_string_append(&prep, expanded);
        free(expanded);
    } else {
        runner_string_append(&prep, values.cpp);
        runner_string_append(&prep, " -C -I. -I");
        runner_string_append(&prep, sandbox);
    }

    for (i = 0; view.cetus_inputs != NULL &&
                i < view.cetus_inputs->length;
         ++i) {
        const char *rel = view.cetus_inputs->items[i];
        const char *src_name = runner_path_basename(rel);
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
        size_t j;

        snprintf(cetus_run_name, sizeof(cetus_run_name), "%s", src_name);
        for (j = 0; cetus_run_name[j]; ++j) {
            if (cetus_run_name[j] == '.')
                cetus_run_name[j] = '_';
        }

        {
            char *cr = runner_path_join(ctx->work_dir, "cetus_run");
            if (cr == NULL)
                goto oom_inner;
            outdir = runner_path_join(cr, cetus_run_name);
            free(cr);
        }
        if (outdir == NULL)
            goto oom_inner;
        if (runner_file_exists(outdir))
            runner_remove_recursive(outdir);
        if (runner_mkdir_recursive(outdir, 0755) != 0) {
            snprintf(error, error_size, "cannot create staging directory");
            free(outdir);
            goto done;
        }

        orig_src = runner_path_join(work_original, src_name);
        staged = runner_path_join(outdir, src_name);
        outdir_output = runner_path_join(outdir, "cetus_output");
        if (orig_src == NULL || staged == NULL || outdir_output == NULL)
            goto oom_inner;
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
        if (argv == NULL)
            goto oom_inner;
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
                goto oom_inner;
            }
            idx = 0;
            argv[idx++] = ctx->cetus;
            argv[idx++] = runner_string_take(&outdir_arg);
            argv[idx++] = runner_string_take(&prep_arg);
            for (j = 0; j < flags->length; ++j)
                argv[idx++] = flags->items[j];
            argv[idx++] = src_name;
            argv[idx] = NULL;
        }

        memset(&result, 0, sizeof(result));
        if (run_tool(ctx, "cetus", argv, outdir, NULL, 0, &result) != 0) {
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
            if (generated && !runner_file_exists(generated)) {
                snprintf(error, error_size,
                         "Cetus produced no output for %s", src_name);
                outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
            } else {
                snprintf(error, error_size, "out of memory");
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
            goto done;
        }
        if (runner_copy_file(generated, dest) != 0 ||
            runner_string_vector_push(outputs, dest) != 0) {
            snprintf(error, error_size, "cannot collect output %s",
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
        continue;

    oom_inner:
        snprintf(error, error_size, "out of memory");
        free(outdir);
        free(orig_src);
        free(staged);
        free(outdir_output);
        goto done;
    }
    outcome = BENCHMARK_ADAPTER_OK;

done:
    runner_string_free(&prep);
    free(work_cetus);
    free(work_original);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  inject                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome generic_inject(
    void *context,
    const BenchmarkCaseContext *ctx,
    const RunnerStringVector *outputs,
    char *error,
    size_t error_size)
{
    BenchmarkGenericView view;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    size_t i;

    (void)context;
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);

    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    for (i = 0; i < outputs->length; ++i) {
        const char *name = runner_path_basename(outputs->items[i]);
        char *dst = NULL;

        if (view.inject_map != NULL && view.inject_map->length > 0) {
            const char *mapped =
                runner_key_value_map_get(view.inject_map, name);
            if (mapped != NULL) {
                dst = runner_path_join(sandbox, mapped);
            } else {
                snprintf(error, error_size,
                         "no inject_map entry for %s", name);
                free(sandbox);
                return BENCHMARK_ADAPTER_INFRA_FAILURE;
            }
        } else {
            /* no inject_map: reject ambiguous basename */
            snprintf(error, error_size,
                     "generic adapter requires inject_map for %s", name);
            free(sandbox);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }

        if (dst == NULL) {
            snprintf(error, error_size, "out of memory");
            free(sandbox);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }

        /* ensure parent directory exists */
        {
            char *parent = strdup(dst);
            char *slash;
            if (parent != NULL) {
                slash = strrchr(parent, '/');
                if (slash != NULL) {
                    *slash = '\0';
                    runner_mkdir_recursive(parent, 0755);
                }
                free(parent);
            }
        }

        if (runner_copy_file(outputs->items[i], dst) != 0) {
            snprintf(error, error_size, "cannot inject %s", name);
            free(dst);
            free(sandbox);
            return BENCHMARK_ADAPTER_INFRA_FAILURE;
        }
        free(dst);
    }
    free(sandbox);
    return BENCHMARK_ADAPTER_OK;
}

/* ------------------------------------------------------------------ */
/*  build                                                             */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome generic_build(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    BenchmarkGenericView view;
    BenchmarkTemplateValues values;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    const char **argv = NULL;
    size_t argc = 0;
    ProcessResult result;
    BenchmarkAdapterOutcome outcome;

    (void)context;
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);
    fill_template_values(ctx, &values);
    values.sandbox = sandbox;

    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    if (view.compile == NULL || view.compile->type == BENCHMARK_COMMAND_NONE) {
        free(sandbox);
        return BENCHMARK_ADAPTER_OK;
    }

    if (view.compile->type != BENCHMARK_COMMAND_ARGV) {
        snprintf(error, error_size,
                 "generic adapter requires argv compile command");
        free(sandbox);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    if (expand_command_argv(view.compile, &values, &argv, &argc, error,
                            error_size) != 0) {
        free(sandbox);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    memset(&result, 0, sizeof(result));
    if (run_tool(ctx, "compile", argv, sandbox, NULL, 0, &result) != 0) {
        snprintf(error, error_size, "cannot execute compile command");
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
    } else if (result.timed_out || !result.exited ||
               result.exit_code != 0) {
        snprintf(error, error_size, "compile failed (rc=%d)",
                 result.exit_code);
        outcome = BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    } else {
        outcome = BENCHMARK_ADAPTER_OK;
    }
    process_result_free(&result);
    free_expanded_argv(argv, argc);
    free(sandbox);
    return outcome;
}

/* ------------------------------------------------------------------ */
/*  verify                                                            */
/* ------------------------------------------------------------------ */

static BenchmarkAdapterOutcome run_verify_command(
    const BenchmarkCaseContext *ctx,
    const BenchmarkCommand *cmd,
    const BenchmarkTemplateValues *values,
    const char *sandbox,
    ProcessResult *result,
    char *error,
    size_t error_size)
{
    const char **argv = NULL;
    size_t argc = 0;

    if (cmd->type != BENCHMARK_COMMAND_ARGV) {
        snprintf(error, error_size,
                 "generic adapter requires argv for verify commands");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (expand_command_argv(cmd, values, &argv, &argc, error,
                            error_size) != 0)
        return BENCHMARK_ADAPTER_INFRA_FAILURE;

    memset(result, 0, sizeof(*result));
    if (run_tool(ctx, "verify-run", argv, sandbox, NULL, 0, result) != 0) {
        snprintf(error, error_size, "cannot execute verify command");
        free_expanded_argv(argv, argc);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    free_expanded_argv(argv, argc);
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome generic_verify_exit_code(
    const BenchmarkCaseContext *ctx,
    const BenchmarkVerify *v,
    const BenchmarkCommand *effective_run,
    const BenchmarkTemplateValues *values,
    const char *sandbox,
    char *error,
    size_t error_size)
{
    ProcessResult result;
    BenchmarkAdapterOutcome rc;

    if (effective_run->type == BENCHMARK_COMMAND_NONE) {
        return BENCHMARK_ADAPTER_OK;
    }
    rc = run_verify_command(ctx, effective_run, values, sandbox, &result,
                            error, error_size);
    if (rc != BENCHMARK_ADAPTER_OK)
        return rc;

    if (result.timed_out || !result.exited) {
        snprintf(error, error_size, "verify command timed out or crashed");
        process_result_free(&result);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    if (result.exit_code != v->expect) {
        snprintf(error, error_size,
                 "exit code %d, expected %d: %.512s", result.exit_code,
                 v->expect,
                 result.stderr_data ? result.stderr_data : "");
        process_result_free(&result);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    process_result_free(&result);
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome generic_verify_regex(
    const BenchmarkCaseContext *ctx,
    const BenchmarkVerify *v,
    const BenchmarkCommand *effective_run,
    const BenchmarkTemplateValues *values,
    const char *sandbox,
    char *error,
    size_t error_size)
{
    ProcessResult result;
    BenchmarkAdapterOutcome rc;
    RunnerString combined;
    char *output;
    regex_t regex;

    rc = run_verify_command(ctx, effective_run, values, sandbox, &result,
                            error, error_size);
    if (rc != BENCHMARK_ADAPTER_OK)
        return rc;

    runner_string_init(&combined);
    if (result.stdout_data)
        runner_string_append(&combined, result.stdout_data);
    if (result.stderr_data)
        runner_string_append(&combined, result.stderr_data);
    output = runner_string_take(&combined);
    process_result_free(&result);

    if (v->pattern == NULL || v->pattern[0] == '\0') {
        snprintf(error, error_size, "verify regex: no pattern");
        free(output);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (regcomp(&regex, v->pattern,
                REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB) != 0) {
        snprintf(error, error_size, "cannot compile regex: %s", v->pattern);
        free(output);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (output == NULL || regexec(&regex, output, 0, NULL, 0) != 0) {
        snprintf(error, error_size,
                 "regex '%s' not found in output", v->pattern);
        regfree(&regex);
        free(output);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    regfree(&regex);
    free(output);
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome generic_verify_dump_diff(
    const BenchmarkCaseContext *ctx,
    const BenchmarkVerify *v,
    const BenchmarkTemplateValues *values,
    const char *sandbox,
    char *error,
    size_t error_size)
{
    ProcessResult base_result;
    ProcessResult xform_result;
    BenchmarkAdapterOutcome rc;
    char *dump_dir = runner_path_join(ctx->work_dir, "dumps");
    char *base_dump = NULL;
    char *xform_dump = NULL;
    const char *diff_tool;
    const char *diff_argv[5];
    ProcessResult diff_result;
    const char *base_data;
    const char *xform_data;

    if (dump_dir == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    runner_mkdir_recursive(dump_dir, 0755);
    base_dump = runner_path_join(dump_dir, "baseline.txt");
    xform_dump = runner_path_join(dump_dir, "transformed.txt");
    if (base_dump == NULL || xform_dump == NULL) {
        snprintf(error, error_size, "out of memory");
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    /* run baseline */
    rc = run_verify_command(ctx, &v->baseline, values, sandbox,
                            &base_result, error, error_size);
    if (rc != BENCHMARK_ADAPTER_OK) {
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return rc;
    }
    if (base_result.timed_out || !base_result.exited ||
        base_result.exit_code != 0) {
        snprintf(error, error_size,
                 "baseline command failed (rc=%d): %.512s",
                 base_result.exit_code,
                 base_result.stderr_data ? base_result.stderr_data : "");
        process_result_free(&base_result);
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    base_data = v->dump_stream == BENCHMARK_DUMP_STREAM_STDOUT
                    ? base_result.stdout_data
                    : base_result.stderr_data;
    runner_write_file(base_dump, base_data ? base_data : "",
                      base_data ? strlen(base_data) : 0);

    /* run transformed */
    rc = run_verify_command(ctx, &v->run, values, sandbox, &xform_result,
                            error, error_size);
    if (rc != BENCHMARK_ADAPTER_OK) {
        process_result_free(&base_result);
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return rc;
    }
    if (xform_result.timed_out || !xform_result.exited ||
        xform_result.exit_code != 0) {
        snprintf(error, error_size,
                 "transformed command failed (rc=%d): %.512s",
                 xform_result.exit_code,
                 xform_result.stderr_data ? xform_result.stderr_data : "");
        process_result_free(&base_result);
        process_result_free(&xform_result);
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    xform_data = v->dump_stream == BENCHMARK_DUMP_STREAM_STDOUT
                     ? xform_result.stdout_data
                     : xform_result.stderr_data;
    runner_write_file(xform_dump, xform_data ? xform_data : "",
                      xform_data ? strlen(xform_data) : 0);
    process_result_free(&base_result);
    process_result_free(&xform_result);

    /* diff */
    diff_tool = benchmark_config_path(ctx->config, "DIFF");
    if (diff_tool == NULL || diff_tool[0] == '\0')
        diff_tool = "diff";
    diff_argv[0] = diff_tool;
    diff_argv[1] = "-wB";
    diff_argv[2] = base_dump;
    diff_argv[3] = xform_dump;
    diff_argv[4] = NULL;

    memset(&diff_result, 0, sizeof(diff_result));
    if (run_tool(ctx, "dump-diff", diff_argv, NULL, NULL, 0, &diff_result) != 0) {
        snprintf(error, error_size, "cannot execute diff");
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (diff_result.timed_out || !diff_result.exited ||
        diff_result.exit_code > 1) {
        snprintf(error, error_size, "diff tool error");
        process_result_free(&diff_result);
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }
    if (diff_result.exit_code == 1) {
        snprintf(error, error_size,
                 "dump mismatch (baseline vs transformed)");
        process_result_free(&diff_result);
        free(base_dump);
        free(xform_dump);
        free(dump_dir);
        return BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE;
    }
    process_result_free(&diff_result);
    free(base_dump);
    free(xform_dump);
    free(dump_dir);
    return BENCHMARK_ADAPTER_OK;
}

static BenchmarkAdapterOutcome generic_verify(
    void *context,
    const BenchmarkCaseContext *ctx,
    char *error,
    size_t error_size)
{
    BenchmarkGenericView view;
    BenchmarkTemplateValues values;
    char *sandbox = runner_path_join(ctx->work_dir, "sandbox");
    const BenchmarkVerify *v;
    const BenchmarkCommand *effective_run;
    BenchmarkAdapterOutcome outcome;

    (void)context;
    benchmark_generic_resolve(ctx->test_case->suite, ctx->test_case->kernel,
                              &view);
    fill_template_values(ctx, &values);
    values.sandbox = sandbox;
    v = view.verify;

    if (sandbox == NULL) {
        snprintf(error, error_size, "out of memory");
        return BENCHMARK_ADAPTER_INFRA_FAILURE;
    }

    if (v == NULL || !v->present) {
        free(sandbox);
        return BENCHMARK_ADAPTER_OK;
    }

    /* resolve effective run: verify.run → suite/kernel-level run fallback */
    effective_run = &v->run;
    if (v->run.type == BENCHMARK_COMMAND_NONE && view.run != NULL &&
        view.run->type != BENCHMARK_COMMAND_NONE)
        effective_run = view.run;

    switch (v->type) {
    case BENCHMARK_VERIFY_EXIT_CODE:
        outcome = generic_verify_exit_code(ctx, v, effective_run, &values,
                                           sandbox, error, error_size);
        break;
    case BENCHMARK_VERIFY_REGEX:
        outcome = generic_verify_regex(ctx, v, effective_run, &values,
                                       sandbox, error, error_size);
        break;
    case BENCHMARK_VERIFY_DUMP_DIFF:
        outcome = generic_verify_dump_diff(ctx, v, &values, sandbox, error,
                                           error_size);
        break;
    default:
        snprintf(error, error_size, "unknown verify type %d", v->type);
        outcome = BENCHMARK_ADAPTER_INFRA_FAILURE;
        break;
    }
    free(sandbox);
    return outcome;
}

static void generic_cleanup(void *context,
                            const BenchmarkCaseContext *ctx)
{
    (void)context;
    (void)ctx;
}

const BenchmarkAdapterVTable generic_adapter_vtable = {
    generic_expected_gt_names, generic_prepare,
    generic_cetus_transform,   generic_inject,
    generic_build,             generic_verify,
    generic_cleanup};
