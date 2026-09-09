#ifndef CLI_H
#define CLI_H

#include "runner_common.h"

typedef enum {
    CLI_MODE_KERNELS,
    CLI_MODE_BENCHMARKS,
    CLI_MODE_FULL,
    CLI_MODE_LIST_SUITES,
    CLI_MODE_LIST_BENCHMARK_SUITES,
    CLI_MODE_HELP
} CliMode;

typedef struct {
    CliMode mode;
    int generate;
    int verbose;

    /* Kernel selection (mode == KERNELS or FULL). */
    const char *test_identifier;
    const char *suite_identifier;
    const char *custom_cetus_options;

    /* Benchmark selection (mode == BENCHMARKS, FULL, or LIST_BENCHMARK_SUITES). */
    RunnerStringVector benchmark_suite_ids;
    const char *benchmark_kernel;
    const char *profile;
    const char *dataset;
    const char *npb_class;
    int skip_gt;
    int skip_run;
    int timeout_seconds;
    const char *config_dir;
} CliOptions;

void cli_options_init(CliOptions *options);
void cli_options_free(CliOptions *options);

int cli_parse(int argc, char **argv, CliOptions *options,
              char *error, size_t error_size);

void cli_print_usage(const char *program_name);

#endif /* CLI_H */
