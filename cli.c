#define _POSIX_C_SOURCE 200809L

#include "cli.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *error, size_t size, const char *fmt, ...)
{
    va_list args;
    if (error != NULL && size != 0) {
        va_start(args, fmt);
        vsnprintf(error, size, fmt, args);
        va_end(args);
    }
    return -1;
}

static int str_eq_ci(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
        return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static int parse_bool(const char *value, int *out)
{
    if (value == NULL || out == NULL)
        return 0;
    if (str_eq_ci(value, "true") || str_eq_ci(value, "1") ||
        str_eq_ci(value, "yes") || str_eq_ci(value, "on")) {
        *out = 1;
        return 1;
    }
    if (str_eq_ci(value, "false") || str_eq_ci(value, "0") ||
        str_eq_ci(value, "no") || str_eq_ci(value, "off")) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int parse_timeout(const char *text, int *out,
                         char *error, size_t error_size)
{
    char *end;
    long value;
    if (text == NULL || text[0] == '\0')
        return fail(error, error_size, "--timeout requires a positive integer");
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value <= 0 || value > INT_MAX)
        return fail(error, error_size,
                    "--timeout requires a positive integer (got '%s')", text);
    *out = (int)value;
    return 0;
}

static int is_option_token(const char *arg)
{
    static const char *known[] = {
        "--help", "-h", "--generate", "--verbose", "--all",
        "--list-suites", "--list-benchmark-suites", "--benchmarks",
        "--full", "--run-test", "--run-suite", "-cetus-options",
        "--benchmark-suite", "--benchmark-kernel", "--profile",
        "--dataset", "--class", "--skip-gt", "--skip-run",
        "--timeout", "--config-dir"
    };
    size_t i;
    if (arg == NULL)
        return 0;
    for (i = 0; i < sizeof(known) / sizeof(known[0]); ++i) {
        if (strcmp(arg, known[i]) == 0)
            return 1;
    }
    return 0;
}

#define NEED_VALUE(flag)                                                \
    do {                                                                \
        if (i + 1 >= argc || is_option_token(argv[i + 1]))             \
            return fail(error, error_size, "%s requires a value", flag);\
    } while (0)

void cli_options_init(CliOptions *options)
{
    memset(options, 0, sizeof(*options));
    options->mode = CLI_MODE_KERNELS;
    options->timeout_seconds = 600;
    options->config_dir = "benchmarks/config";
    runner_string_vector_init(&options->benchmark_suite_ids);
}

void cli_options_free(CliOptions *options)
{
    runner_string_vector_free(&options->benchmark_suite_ids);
}

int cli_parse(int argc, char **argv, CliOptions *options,
              char *error, size_t error_size)
{
    int has_benchmarks = 0;
    int has_full = 0;
    int has_list_suites = 0;
    int has_list_benchmark_suites = 0;
    int has_help = 0;
    int has_benchmark_filter = 0;
    int i;

    if (error != NULL && error_size != 0)
        error[0] = '\0';
    cli_options_init(options);

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            has_help = 1;
        } else if (strcmp(argv[i], "--generate") == 0) {
            options->generate = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            options->verbose = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                int parsed;
                if (!parse_bool(argv[i + 1], &parsed))
                    return fail(error, error_size,
                                "--verbose expects true/false (got '%s')",
                                argv[i + 1]);
                options->verbose = parsed;
                ++i;
            }
        } else if (strcmp(argv[i], "--all") == 0) {
            /* handled implicitly: KERNELS mode with no test/suite filter */
        } else if (strcmp(argv[i], "--list-suites") == 0) {
            has_list_suites = 1;
        } else if (strcmp(argv[i], "--list-benchmark-suites") == 0) {
            has_list_benchmark_suites = 1;
        } else if (strcmp(argv[i], "--benchmarks") == 0) {
            has_benchmarks = 1;
        } else if (strcmp(argv[i], "--full") == 0) {
            has_full = 1;
        } else if (strcmp(argv[i], "--run-test") == 0) {
            NEED_VALUE("--run-test");
            options->test_identifier = argv[++i];
        } else if (strcmp(argv[i], "--run-suite") == 0) {
            NEED_VALUE("--run-suite");
            options->suite_identifier = argv[++i];
        } else if (strcmp(argv[i], "-cetus-options") == 0) {
            if (i + 1 >= argc)
                return fail(error, error_size,
                            "-cetus-options requires an argument");
            options->custom_cetus_options = argv[++i];
        } else if (strcmp(argv[i], "--benchmark-suite") == 0) {
            NEED_VALUE("--benchmark-suite");
            if (runner_string_vector_push(&options->benchmark_suite_ids,
                                          argv[++i]) != 0)
                return fail(error, error_size, "out of memory");
        } else if (strcmp(argv[i], "--benchmark-kernel") == 0) {
            NEED_VALUE("--benchmark-kernel");
            options->benchmark_kernel = argv[++i];
        } else if (strcmp(argv[i], "--profile") == 0) {
            NEED_VALUE("--profile");
            options->profile = argv[++i];
        } else if (strcmp(argv[i], "--dataset") == 0) {
            NEED_VALUE("--dataset");
            options->dataset = argv[++i];
        } else if (strcmp(argv[i], "--class") == 0) {
            NEED_VALUE("--class");
            options->npb_class = argv[++i];
        } else if (strcmp(argv[i], "--skip-gt") == 0) {
            options->skip_gt = 1;
        } else if (strcmp(argv[i], "--skip-run") == 0) {
            options->skip_run = 1;
        } else if (strcmp(argv[i], "--timeout") == 0) {
            NEED_VALUE("--timeout");
            if (parse_timeout(argv[++i], &options->timeout_seconds,
                              error, error_size) != 0)
                return -1;
        } else if (strcmp(argv[i], "--config-dir") == 0) {
            NEED_VALUE("--config-dir");
            options->config_dir = argv[++i];
        } else {
            return fail(error, error_size,
                        "unrecognized argument: %s", argv[i]);
        }
    }

    /* --- determine mode --- */
    if (has_help) {
        options->mode = CLI_MODE_HELP;
        return 0;
    }
    {
        int mode_count = has_list_suites + has_list_benchmark_suites +
                         has_benchmarks + has_full;
        if (mode_count > 1)
            return fail(error, error_size,
                        "conflicting mode flags (use only one of --benchmarks, "
                        "--full, --list-suites, --list-benchmark-suites)");
    }
    if (has_list_suites)
        options->mode = CLI_MODE_LIST_SUITES;
    else if (has_list_benchmark_suites)
        options->mode = CLI_MODE_LIST_BENCHMARK_SUITES;
    else if (has_benchmarks)
        options->mode = CLI_MODE_BENCHMARKS;
    else if (has_full)
        options->mode = CLI_MODE_FULL;
    else
        options->mode = CLI_MODE_KERNELS;

    /* --- validate kernel options --- */
    if (options->test_identifier != NULL &&
        options->suite_identifier != NULL)
        return fail(error, error_size,
                    "use either --run-test or --run-suite, not both");

    if (options->mode == CLI_MODE_BENCHMARKS) {
        if (options->test_identifier != NULL)
            return fail(error, error_size,
                        "--run-test conflicts with --benchmarks; "
                        "use --full for both");
        if (options->suite_identifier != NULL)
            return fail(error, error_size,
                        "--run-suite conflicts with --benchmarks; "
                        "use --full for both");
    }

    /* --- list modes reject execution/filter options that would be ignored --- */
    if (options->mode == CLI_MODE_LIST_SUITES ||
        options->mode == CLI_MODE_LIST_BENCHMARK_SUITES) {
        if (options->generate ||
            options->test_identifier != NULL ||
            options->suite_identifier != NULL ||
            options->custom_cetus_options != NULL)
            return fail(error, error_size,
                        "%s does not accept --generate, --run-test, "
                        "--run-suite, or -cetus-options",
                        options->mode == CLI_MODE_LIST_SUITES
                            ? "--list-suites"
                            : "--list-benchmark-suites");
    }

    /* --- benchmark filters require benchmark mode --- */
    has_benchmark_filter = (int)(
        options->benchmark_suite_ids.length > 0 ||
        options->benchmark_kernel != NULL ||
        options->profile != NULL ||
        options->dataset != NULL ||
        options->npb_class != NULL ||
        options->skip_gt || options->skip_run);

    if (has_benchmark_filter &&
        options->mode != CLI_MODE_BENCHMARKS &&
        options->mode != CLI_MODE_FULL)
        return fail(error, error_size,
                    "benchmark filters (--benchmark-suite, --benchmark-kernel, "
                    "--profile, --dataset, --class, --skip-gt, --skip-run) "
                    "require --benchmarks or --full");

    return 0;
}

void cli_print_usage(const char *program_name)
{
    fprintf(stderr,
        "\nUsage: %s [options]\n"
        "\nKernel test modes:\n"
        "  (no args), --all         Run all kernel tests\n"
        "  --run-test <id>          Run one test (category, filename, or path)\n"
        "  --run-suite <name>       Run all tests in a suite (tiling, subsub)\n"
        "  --list-suites            List defined kernel suites\n"
        "\nBenchmark modes:\n"
        "  --benchmarks             Run benchmark tests only\n"
        "  --full                   Run both kernels and default benchmarks\n"
        "  --list-benchmark-suites  List benchmark suites/profiles/kernels\n"
        "\nShared options:\n"
        "  --generate               Generate ground truth instead of comparing\n"
        "  --verbose [true|false]   Show verbose output (default: false)\n"
        "  -cetus-options \"<flags>\" Override Cetus options for kernel tests\n"
        "  --help, -h               Show this help\n"
        "\nBenchmark options (require --benchmarks or --full):\n"
        "  --benchmark-suite <id>   Select benchmark suite (repeatable)\n"
        "  --benchmark-kernel <name> Filter to matching kernel name\n"
        "  --profile <name>         Filter to matching profile\n"
        "  --dataset <value>        Override dataset for PolyBench suites\n"
        "  --class <value>          Override class for NPB suites\n"
        "  --skip-gt                Skip ground-truth comparison\n"
        "  --skip-run               Skip build/run/verify after transform\n"
        "  --timeout <seconds>      Benchmark tool timeout (default: 600)\n"
        "  --config-dir <path>      Benchmark config directory "
        "(default: benchmarks/config)\n",
        program_name);
}
