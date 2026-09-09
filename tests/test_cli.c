#define _POSIX_C_SOURCE 200809L

#include "cli.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,            \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/* --------------------------------------------------------------------- */
/*  mode detection                                                       */
/* --------------------------------------------------------------------- */

static int test_no_args(void)
{
    char *argv[] = {"prog", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(1, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_KERNELS);
    CHECK(opts.generate == 0);
    CHECK(opts.verbose == 0);
    CHECK(opts.test_identifier == NULL);
    CHECK(opts.suite_identifier == NULL);
    CHECK(opts.timeout_seconds == 600);
    CHECK(strcmp(opts.config_dir, "benchmarks/config") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_all_flag(void)
{
    char *argv[] = {"prog", "--all", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_KERNELS);
    cli_options_free(&opts);
    return 0;
}

static int test_help_flag(void)
{
    char *argv[] = {"prog", "--help", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_HELP);
    cli_options_free(&opts);
    return 0;
}

static int test_help_short_flag(void)
{
    char *argv[] = {"prog", "-h", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_HELP);
    cli_options_free(&opts);
    return 0;
}

static int test_list_suites(void)
{
    char *argv[] = {"prog", "--list-suites", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_LIST_SUITES);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_LIST_BENCHMARK_SUITES);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmarks_mode(void)
{
    char *argv[] = {"prog", "--benchmarks", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_BENCHMARKS);
    cli_options_free(&opts);
    return 0;
}

static int test_full_mode(void)
{
    char *argv[] = {"prog", "--full", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_FULL);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  kernel options                                                       */
/* --------------------------------------------------------------------- */

static int test_run_test(void)
{
    char *argv[] = {"prog", "--run-test", "Tiling_PAW_GEMM", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_KERNELS);
    CHECK(opts.test_identifier != NULL);
    CHECK(strcmp(opts.test_identifier, "Tiling_PAW_GEMM") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_run_suite(void)
{
    char *argv[] = {"prog", "--run-suite", "tiling", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_KERNELS);
    CHECK(opts.suite_identifier != NULL);
    CHECK(strcmp(opts.suite_identifier, "tiling") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_cetus_options(void)
{
    char *argv[] = {"prog", "-cetus-options", "-paw_tiling=1 -tileSizes=32",
                    NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.custom_cetus_options != NULL);
    CHECK(strcmp(opts.custom_cetus_options, "-paw_tiling=1 -tileSizes=32") == 0);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  shared options                                                       */
/* --------------------------------------------------------------------- */

static int test_generate(void)
{
    char *argv[] = {"prog", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.generate == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_bare(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.verbose == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_true(void)
{
    char *argv[] = {"prog", "--verbose", "true", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.verbose == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_false(void)
{
    char *argv[] = {"prog", "--verbose", "false", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.verbose == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_zero(void)
{
    char *argv[] = {"prog", "--verbose", "0", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.verbose == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_followed_by_flag(void)
{
    char *argv[] = {"prog", "--verbose", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.verbose == 1);
    CHECK(opts.generate == 1);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  benchmark options                                                    */
/* --------------------------------------------------------------------- */

static int test_benchmark_suite_single(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-suite", "polybench",
                    NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_BENCHMARKS);
    CHECK(opts.benchmark_suite_ids.length == 1);
    CHECK(strcmp(opts.benchmark_suite_ids.items[0], "polybench") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_suite_repeatable(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-suite", "polybench",
                    "--benchmark-suite", "npb-ser", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(6, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.benchmark_suite_ids.length == 2);
    CHECK(strcmp(opts.benchmark_suite_ids.items[0], "polybench") == 0);
    CHECK(strcmp(opts.benchmark_suite_ids.items[1], "npb-ser") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_kernel(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-kernel", "gemm", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.benchmark_kernel != NULL);
    CHECK(strcmp(opts.benchmark_kernel, "gemm") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_profile(void)
{
    char *argv[] = {"prog", "--benchmarks", "--profile", "parallel", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.profile != NULL);
    CHECK(strcmp(opts.profile, "parallel") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_dataset(void)
{
    char *argv[] = {"prog", "--benchmarks", "--dataset", "SMALL_DATASET", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.dataset != NULL);
    CHECK(strcmp(opts.dataset, "SMALL_DATASET") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_class(void)
{
    char *argv[] = {"prog", "--benchmarks", "--class", "W", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.npb_class != NULL);
    CHECK(strcmp(opts.npb_class, "W") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_skip_gt(void)
{
    char *argv[] = {"prog", "--benchmarks", "--skip-gt", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.skip_gt == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_skip_run(void)
{
    char *argv[] = {"prog", "--benchmarks", "--skip-run", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.skip_run == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "300", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.timeout_seconds == 300);
    cli_options_free(&opts);
    return 0;
}

static int test_config_dir(void)
{
    char *argv[] = {"prog", "--benchmarks", "--config-dir", "/tmp/config", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(strcmp(opts.config_dir, "/tmp/config") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_generate_applies_to_benchmarks(void)
{
    char *argv[] = {"prog", "--benchmarks", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_BENCHMARKS);
    CHECK(opts.generate == 1);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  full mode combines kernel and benchmark options                      */
/* --------------------------------------------------------------------- */

static int test_full_with_kernel_and_benchmark_filters(void)
{
    char *argv[] = {"prog", "--full", "--run-suite", "tiling",
                    "--benchmark-suite", "polybench", "--benchmark-kernel",
                    "gemm", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(8, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_FULL);
    CHECK(strcmp(opts.suite_identifier, "tiling") == 0);
    CHECK(opts.benchmark_suite_ids.length == 1);
    CHECK(strcmp(opts.benchmark_kernel, "gemm") == 0);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  validation errors                                                    */
/* --------------------------------------------------------------------- */

static int test_run_test_and_run_suite_conflict(void)
{
    char *argv[] = {"prog", "--run-test", "X", "--run-suite", "Y", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(5, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-test") != NULL || strstr(error, "--run-suite") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_run_test_conflicts_with_benchmarks(void)
{
    char *argv[] = {"prog", "--benchmarks", "--run-test", "X", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-test") != NULL);
    CHECK(strstr(error, "conflict") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_run_suite_conflicts_with_benchmarks(void)
{
    char *argv[] = {"prog", "--benchmarks", "--run-suite", "tiling", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-suite") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_filter_without_benchmark_mode(void)
{
    char *argv[] = {"prog", "--benchmark-suite", "polybench", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "require") != NULL);
    CHECK(strstr(error, "--benchmarks") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_kernel_without_benchmark_mode(void)
{
    char *argv[] = {"prog", "--benchmark-kernel", "gemm", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "require") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_skip_gt_without_benchmark_mode(void)
{
    char *argv[] = {"prog", "--skip-gt", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "require") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_conflicting_modes(void)
{
    char *argv[] = {"prog", "--benchmarks", "--full", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "conflicting") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_unknown_option(void)
{
    char *argv[] = {"prog", "--bogus", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "unrecognized") != NULL);
    CHECK(strstr(error, "--bogus") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout_invalid_value(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "abc", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "positive integer") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout_zero(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "0", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "positive integer") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout_negative(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "-1", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "positive integer") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout_negative_large(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "-999", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "positive integer") != NULL);
    CHECK(strstr(error, "-999") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_run_test_missing_arg(void)
{
    char *argv[] = {"prog", "--run-test", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(2, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-test") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_verbose_bad_value(void)
{
    char *argv[] = {"prog", "--verbose", "maybe", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--verbose") != NULL);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  value-taking options reject recognized option tokens as value         */
/* --------------------------------------------------------------------- */

static int test_run_test_eats_option_token(void)
{
    char *argv[] = {"prog", "--run-test", "--verbose", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-test") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_run_suite_eats_option_token(void)
{
    char *argv[] = {"prog", "--run-suite", "--benchmarks", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--run-suite") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_suite_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-suite", "--profile",
                    NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--benchmark-suite") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmark_kernel_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-kernel", "--generate",
                    NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--benchmark-kernel") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_profile_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--profile", "--skip-gt", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--profile") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_dataset_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--dataset", "--class", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--dataset") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_class_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--class", "--help", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--class") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_timeout_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--timeout", "--all", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--timeout") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_config_dir_eats_option_token(void)
{
    char *argv[] = {"prog", "--benchmarks", "--config-dir", "--full", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--config-dir") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_cetus_options_allows_dash_value(void)
{
    char *argv[] = {"prog", "-cetus-options", "-paw_tiling=1", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.custom_cetus_options != NULL);
    CHECK(strcmp(opts.custom_cetus_options, "-paw_tiling=1") == 0);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  list modes reject ignored execution/filter options                   */
/* --------------------------------------------------------------------- */

static int test_list_suites_rejects_generate(void)
{
    char *argv[] = {"prog", "--list-suites", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_suites_rejects_run_test(void)
{
    char *argv[] = {"prog", "--list-suites", "--run-test", "X", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_suites_rejects_run_suite(void)
{
    char *argv[] = {"prog", "--list-suites", "--run-suite", "tiling", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_suites_rejects_cetus_options(void)
{
    char *argv[] = {"prog", "--list-suites", "-cetus-options", "-v=0", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_rejects_generate(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-benchmark-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_rejects_run_test(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--run-test", "X", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    CHECK(strstr(error, "--list-benchmark-suites") != NULL);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_rejects_benchmark_filter(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--benchmark-kernel",
                    "gemm", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) != 0);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_rejects_skip_gt(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--skip-gt", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) != 0);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_allows_config_dir(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--config-dir",
                    "/tmp/cfg", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_LIST_BENCHMARK_SUITES);
    CHECK(strcmp(opts.config_dir, "/tmp/cfg") == 0);
    cli_options_free(&opts);
    return 0;
}

static int test_list_benchmark_suites_allows_verbose(void)
{
    char *argv[] = {"prog", "--list-benchmark-suites", "--verbose", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(3, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_LIST_BENCHMARK_SUITES);
    CHECK(opts.verbose == 1);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  help supersedes other options                                        */
/* --------------------------------------------------------------------- */

static int test_help_with_other_flags(void)
{
    char *argv[] = {"prog", "--benchmarks", "--help", "--run-test", "X", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(5, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_HELP);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  complex valid combinations                                           */
/* --------------------------------------------------------------------- */

static int test_full_verbose_generate(void)
{
    char *argv[] = {"prog", "--full", "--verbose", "--generate", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(4, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_FULL);
    CHECK(opts.verbose == 1);
    CHECK(opts.generate == 1);
    cli_options_free(&opts);
    return 0;
}

static int test_benchmarks_all_options(void)
{
    char *argv[] = {"prog", "--benchmarks", "--benchmark-suite", "polybench",
                    "--benchmark-kernel", "gemm", "--profile", "paw_tiling",
                    "--dataset", "SMALL_DATASET", "--class", "W",
                    "--skip-gt", "--skip-run", "--timeout", "120",
                    "--config-dir", "/opt/config", "--generate",
                    "--verbose", "true", NULL};
    CliOptions opts;
    char error[256];
    CHECK(cli_parse(21, argv, &opts, error, sizeof(error)) == 0);
    CHECK(opts.mode == CLI_MODE_BENCHMARKS);
    CHECK(opts.benchmark_suite_ids.length == 1);
    CHECK(strcmp(opts.benchmark_kernel, "gemm") == 0);
    CHECK(strcmp(opts.profile, "paw_tiling") == 0);
    CHECK(strcmp(opts.dataset, "SMALL_DATASET") == 0);
    CHECK(strcmp(opts.npb_class, "W") == 0);
    CHECK(opts.skip_gt == 1);
    CHECK(opts.skip_run == 1);
    CHECK(opts.timeout_seconds == 120);
    CHECK(strcmp(opts.config_dir, "/opt/config") == 0);
    CHECK(opts.generate == 1);
    CHECK(opts.verbose == 1);
    cli_options_free(&opts);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  main                                                                 */
/* --------------------------------------------------------------------- */

int main(void)
{
    CHECK(test_no_args() == 0);
    CHECK(test_all_flag() == 0);
    CHECK(test_help_flag() == 0);
    CHECK(test_help_short_flag() == 0);
    CHECK(test_list_suites() == 0);
    CHECK(test_list_benchmark_suites() == 0);
    CHECK(test_benchmarks_mode() == 0);
    CHECK(test_full_mode() == 0);
    CHECK(test_run_test() == 0);
    CHECK(test_run_suite() == 0);
    CHECK(test_cetus_options() == 0);
    CHECK(test_generate() == 0);
    CHECK(test_verbose_bare() == 0);
    CHECK(test_verbose_true() == 0);
    CHECK(test_verbose_false() == 0);
    CHECK(test_verbose_zero() == 0);
    CHECK(test_verbose_followed_by_flag() == 0);
    CHECK(test_benchmark_suite_single() == 0);
    CHECK(test_benchmark_suite_repeatable() == 0);
    CHECK(test_benchmark_kernel() == 0);
    CHECK(test_profile() == 0);
    CHECK(test_dataset() == 0);
    CHECK(test_class() == 0);
    CHECK(test_skip_gt() == 0);
    CHECK(test_skip_run() == 0);
    CHECK(test_timeout() == 0);
    CHECK(test_config_dir() == 0);
    CHECK(test_generate_applies_to_benchmarks() == 0);
    CHECK(test_full_with_kernel_and_benchmark_filters() == 0);
    CHECK(test_run_test_and_run_suite_conflict() == 0);
    CHECK(test_run_test_conflicts_with_benchmarks() == 0);
    CHECK(test_run_suite_conflicts_with_benchmarks() == 0);
    CHECK(test_benchmark_filter_without_benchmark_mode() == 0);
    CHECK(test_benchmark_kernel_without_benchmark_mode() == 0);
    CHECK(test_skip_gt_without_benchmark_mode() == 0);
    CHECK(test_conflicting_modes() == 0);
    CHECK(test_unknown_option() == 0);
    CHECK(test_timeout_invalid_value() == 0);
    CHECK(test_timeout_zero() == 0);
    CHECK(test_timeout_negative() == 0);
    CHECK(test_timeout_negative_large() == 0);
    CHECK(test_run_test_missing_arg() == 0);
    CHECK(test_verbose_bad_value() == 0);
    CHECK(test_run_test_eats_option_token() == 0);
    CHECK(test_run_suite_eats_option_token() == 0);
    CHECK(test_benchmark_suite_eats_option_token() == 0);
    CHECK(test_benchmark_kernel_eats_option_token() == 0);
    CHECK(test_profile_eats_option_token() == 0);
    CHECK(test_dataset_eats_option_token() == 0);
    CHECK(test_class_eats_option_token() == 0);
    CHECK(test_timeout_eats_option_token() == 0);
    CHECK(test_config_dir_eats_option_token() == 0);
    CHECK(test_cetus_options_allows_dash_value() == 0);
    CHECK(test_list_suites_rejects_generate() == 0);
    CHECK(test_list_suites_rejects_run_test() == 0);
    CHECK(test_list_suites_rejects_run_suite() == 0);
    CHECK(test_list_suites_rejects_cetus_options() == 0);
    CHECK(test_list_benchmark_suites_rejects_generate() == 0);
    CHECK(test_list_benchmark_suites_rejects_run_test() == 0);
    CHECK(test_list_benchmark_suites_rejects_benchmark_filter() == 0);
    CHECK(test_list_benchmark_suites_rejects_skip_gt() == 0);
    CHECK(test_list_benchmark_suites_allows_config_dir() == 0);
    CHECK(test_list_benchmark_suites_allows_verbose() == 0);
    CHECK(test_help_with_other_flags() == 0);
    CHECK(test_full_verbose_generate() == 0);
    CHECK(test_benchmarks_all_options() == 0);
    puts("all CLI tests passed");
    return 0;
}
