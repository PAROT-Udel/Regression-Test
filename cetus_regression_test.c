// cetus_regression_test.c - Unified kernel + benchmark test runner

#include "cli.h"
#include "helper_tests.h"
#include "master_test_cases.h"
#include "benchmark_adapters.h"
#include "benchmark_config.h"
#include "benchmark_runner.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* --------------------------------------------------------------------- */
/*  Global log file pointers (definitions; extern in helper_tests.h)     */
/* --------------------------------------------------------------------- */

FILE *log_all_fp = NULL;
FILE *log_passed_fp = NULL;
FILE *log_failed_fp = NULL;
FILE *log_crashes_fp = NULL;
FILE *log_missed_opportunities_fp = NULL;
FILE *log_incorrect_transformation_fp = NULL;

static int g_verbose = 0;

static void verbose_printf(const char *fmt, ...)
{
    if (!g_verbose)
        return;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* --------------------------------------------------------------------- */
/*  Helper function implementations                                      */
/* --------------------------------------------------------------------- */

char *get_current_time(void)
{
    static char time_str[20];
    time_t timer;
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

long file_size(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

int execute_command(const char *command)
{
    verbose_printf("[%s] DEBUG: Executing command: %s\n",
                   get_current_time(), command);
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] DEBUG: Executing command: %s\n",
                get_current_time(), command);

    char quiet_command[MAX_PATH_LENGTH * 4];
    const char *cmd_to_run = command;
    if (!g_verbose) {
        if (snprintf(quiet_command, sizeof(quiet_command),
                     "(%s) >/dev/null 2>&1",
                     command) >= (int)sizeof(quiet_command)) {
            fprintf(stderr, "[%s] ERROR: Quiet command buffer overflow.\n",
                    get_current_time());
            return -1;
        }
        cmd_to_run = quiet_command;
    }

    int result = system(cmd_to_run);
    if (result == -1) {
        fprintf(stderr,
                "[%s] ERROR: Failed to execute command: %s "
                "(System call failed: %s).\n",
                get_current_time(), command, strerror(errno));
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] ERROR: Failed to execute command: %s "
                    "(System call failed: %s).\n",
                    get_current_time(), command, strerror(errno));
        return -1;
    }
    if (WIFEXITED(result)) {
        int exit_status = WEXITSTATUS(result);
        verbose_printf("[%s] Command exited with status: %d\n",
                       get_current_time(), exit_status);
        if (log_all_fp)
            fprintf(log_all_fp, "[%s] Command exited with status: %d\n",
                    get_current_time(), exit_status);
        return exit_status;
    }
    if (WIFSIGNALED(result)) {
        int signal_num = WTERMSIG(result);
        fprintf(stderr,
                "[%s] ERROR: Command terminated by signal %d (%s): %s\n",
                get_current_time(), signal_num, strsignal(signal_num),
                command);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] ERROR: Command terminated by signal %d (%s): %s\n",
                    get_current_time(), signal_num, strsignal(signal_num),
                    command);
    } else {
        fprintf(stderr,
                "[%s] ERROR: Command did not terminate normally "
                "(unknown reason): %s\n",
                get_current_time(), command);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] ERROR: Command did not terminate normally "
                    "(unknown reason): %s\n",
                    get_current_time(), command);
    }
    return -999;
}

/* --------------------------------------------------------------------- */
/*  Enum converters                                                      */
/* --------------------------------------------------------------------- */

const char *transformation_type_to_string(TransformationType type)
{
    switch (type) {
    case TRANSFORM_PARALLELIZATION:
        return "parallelization";
    case TRANSFORM_PRIVATIZATION:
        return "privatization";
    case TRANSFORM_REDUCTION:
        return "reduction";
    case TRANSFORM_TILING:
        return "tiling";
    case TRANSFORM_SUBSUB_ANALYSIS:
        return "subsub_analysis";
    case TRANSFORM_NONE:
        return "no_transformation";
    case TRANSFORM_UNKNOWN:
    default:
        return "unknown";
    }
}

TransformationType string_to_transformation_type(const char *str)
{
    if (strcmp(str, "parallelization") == 0)
        return TRANSFORM_PARALLELIZATION;
    if (strcmp(str, "privatization") == 0)
        return TRANSFORM_PRIVATIZATION;
    if (strcmp(str, "reduction") == 0)
        return TRANSFORM_REDUCTION;
    if (strcmp(str, "tiling") == 0)
        return TRANSFORM_TILING;
    if (strcmp(str, "subsub_analysis") == 0)
        return TRANSFORM_SUBSUB_ANALYSIS;
    if (strcmp(str, "no_transformation") == 0)
        return TRANSFORM_NONE;
    return TRANSFORM_UNKNOWN;
}

const char *expected_outcome_to_string(ExpectedOutcome outcome)
{
    switch (outcome) {
    case EXPECT_SUCCESS_TRANSFORMED:
        return "success_transformed";
    case EXPECT_SUCCESS_NO_CHANGE:
        return "success_no_change";
    case EXPECT_FAILURE:
        return "expect_failure";
    case EXPECT_UNKNOWN:
    default:
        return "unknown";
    }
}

ExpectedOutcome string_to_expected_outcome(const char *str)
{
    if (strcmp(str, "success_transformed") == 0)
        return EXPECT_SUCCESS_TRANSFORMED;
    if (strcmp(str, "success_no_change") == 0)
        return EXPECT_SUCCESS_NO_CHANGE;
    if (strcmp(str, "expect_failure") == 0)
        return EXPECT_FAILURE;
    return EXPECT_UNKNOWN;
}

/* --------------------------------------------------------------------- */
/*  Path and string helpers                                              */
/* --------------------------------------------------------------------- */

static const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    if (bslash && (!slash || bslash > slash))
        slash = bslash;
    return slash ? slash + 1 : path;
}

static int str_eq_ci(const char *a, const char *b)
{
    if (!a || !b)
        return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static int test_in_suite(const TestCase *t, const char *suite)
{
    if (!t || !t->suite || !suite)
        return 0;
    if (str_eq_ci(t->suite, suite))
        return 1;
    if ((str_eq_ci(suite, "paw") || str_eq_ci(suite, "paw_tiling")) &&
        str_eq_ci(t->suite, "tiling"))
        return 1;
    if ((str_eq_ci(suite, "subsub_analysis") ||
         str_eq_ci(suite, "subscripted-subscript")) &&
        str_eq_ci(t->suite, "subsub"))
        return 1;
    return 0;
}

/* --------------------------------------------------------------------- */
/*  list_suites (kernel suites from master_test_cases.h)                 */
/* --------------------------------------------------------------------- */

static void list_suites(void)
{
    printf("Defined suites:\n");
    for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
        const char *name = master_test_cases[i].suite;
        if (!name)
            continue;
        int seen = 0;
        for (int j = 0; j < i; ++j) {
            if (master_test_cases[j].suite &&
                str_eq_ci(master_test_cases[j].suite, name)) {
                seen = 1;
                break;
            }
        }
        if (seen)
            continue;
        int count = 0;
        for (int k = 0; k < NUM_MASTER_TEST_CASES; ++k) {
            if (test_in_suite(&master_test_cases[k], name))
                count++;
        }
        printf("  %-12s  %d test(s)   --run-suite %s\n", name, count, name);
    }
}

/* --------------------------------------------------------------------- */
/*  list_benchmark_suites (from JSON config)                             */
/* --------------------------------------------------------------------- */

static int list_benchmark_suites(const char *config_dir)
{
    BenchmarkConfig config;
    char error[2048];
    size_t s, p, k;

    benchmark_config_init(&config);
    if (benchmark_config_load(config_dir, &config, error, sizeof(error)) != 0) {
        fprintf(stderr, "ERROR: Failed to load benchmark config from '%s': %s\n",
                config_dir, error);
        benchmark_config_free(&config);
        return 1;
    }

    printf("Benchmark suites (from %s):\n\n", config_dir);
    for (s = 0; s < config.suite_count; ++s) {
        const BenchmarkSuite *suite = &config.suites[s];
        printf("  %-20s  %zu kernel(s)  adapter=%-10s%s\n",
               suite->id, suite->kernel_count,
               suite->adapter_name ? suite->adapter_name : "?",
               suite->is_default ? "  [default]" : "");
        if (suite->description)
            printf("    %s\n", suite->description);
        printf("    profiles:");
        for (p = 0; p < suite->profiles.length; ++p)
            printf(" %s", suite->profiles.items[p]);
        printf("\n    kernels:");
        for (k = 0; k < suite->kernel_count; ++k) {
            printf(" %s", suite->kernels[k].name);
            if (suite->kernels[k].xfail)
                printf("(xfail)");
        }
        printf("\n\n");
    }

    printf("Profiles:\n");
    for (p = 0; p < config.profile_count; ++p) {
        printf("  %-20s", config.profiles[p].name);
        if (config.profiles[p].description)
            printf("  %s", config.profiles[p].description);
        printf("\n    flags:");
        for (size_t f = 0; f < config.profiles[p].flags.length; ++f)
            printf(" %s", config.profiles[p].flags.items[f]);
        printf("\n");
    }

    benchmark_config_free(&config);
    return 0;
}

/* --------------------------------------------------------------------- */
/*  log_test_outcome                                                     */
/* --------------------------------------------------------------------- */

void log_test_outcome(TestOutcome outcome, const char *category,
                      const char *input_file_base_name,
                      const char *reason_message,
                      const char *command_executed)
{
    FILE *target_fp = NULL;
    const char *outcome_str = "UNKNOWN";

    switch (outcome) {
    case TEST_PASSED:
        target_fp = log_passed_fp;
        outcome_str = "PASSED";
        break;
    case TEST_FAILED_CRASH:
        target_fp = log_crashes_fp;
        outcome_str = "CRASHED";
        break;
    case TEST_FAILED_DIFF:
        target_fp = log_failed_fp;
        outcome_str = "DIFF_MISMATCH";
        break;
    case TEST_FAILED_PREPROCESS:
        target_fp = log_failed_fp;
        outcome_str = "PREPROCESS_FAILED";
        break;
    case TEST_FAILED_SEMANTIC_CHECK:
        target_fp = log_failed_fp;
        outcome_str = "SEMANTIC_CHECK_FAILED";
        break;
    case TEST_FAILED_MKDIR:
        target_fp = log_failed_fp;
        outcome_str = "MKDIR_FAILED";
        break;
    case TEST_FAILED_COPY_GROUND_TRUTH:
        target_fp = log_failed_fp;
        outcome_str = "COPY_GROUND_TRUTH_FAILED";
        break;
    case TEST_FAILED_MISSING_OUTPUT:
        target_fp = log_failed_fp;
        outcome_str = "MISSING_OUTPUT";
        break;
    case TEST_FAILED_MISSING_GROUND_TRUTH:
        target_fp = log_failed_fp;
        outcome_str = "MISSING_GROUND_TRUTH";
        break;
    case TEST_FAILED_NO_EXTENSION:
        target_fp = log_failed_fp;
        outcome_str = "NO_EXTENSION";
        break;
    case TEST_FAILED_MISSED_OPPORTUNITY:
        target_fp = log_missed_opportunities_fp;
        outcome_str = "MISSED_OPPORTUNITY";
        break;
    case TEST_FAILED_INCORRECT_TRANSFORMATION:
        target_fp = log_incorrect_transformation_fp;
        outcome_str = "INCORRECT_TRANSFORMATION";
        break;
    case TEST_FAILED_UNKNOWN:
    default:
        target_fp = log_failed_fp;
        outcome_str = "UNKNOWN_FAILURE";
        break;
    }

    if (log_all_fp) {
        fprintf(log_all_fp, "[%s] TEST: %s/%s - OUTCOME: %s\n",
                get_current_time(), category, input_file_base_name,
                outcome_str);
        if (reason_message && strlen(reason_message) > 0)
            fprintf(log_all_fp, "[%s] REASON: %s\n", get_current_time(),
                    reason_message);
        if (command_executed && strlen(command_executed) > 0)
            fprintf(log_all_fp, "[%s] COMMAND: %s\n", get_current_time(),
                    command_executed);
        fprintf(log_all_fp, "---------------------------------\n\n");
    }

    if (target_fp && target_fp != log_all_fp) {
        fprintf(target_fp, "[%s] TEST: %s/%s - OUTCOME: %s\n",
                get_current_time(), category, input_file_base_name,
                outcome_str);
        if (reason_message && strlen(reason_message) > 0)
            fprintf(target_fp, "[%s] REASON: %s\n", get_current_time(),
                    reason_message);
        if (command_executed && strlen(command_executed) > 0)
            fprintf(target_fp, "[%s] COMMAND: %s\n", get_current_time(),
                    command_executed);
        fprintf(target_fp, "---------------------------------\n\n");
    }

    if (outcome != TEST_PASSED) {
        fprintf(stderr, "\n--- TEST FAILED ---\n");
        fprintf(stderr, "TEST: %s/%s\n", category, input_file_base_name);
        fprintf(stderr, "[%s] OUTCOME: %s\n", get_current_time(),
                outcome_str);
        if (reason_message && strlen(reason_message) > 0)
            fprintf(stderr, "[%s] REASON: %s\n", get_current_time(),
                    reason_message);
        if (command_executed && strlen(command_executed) > 0)
            fprintf(stderr, "[%s] COMMAND: %s\n", get_current_time(),
                    command_executed);
        fprintf(stderr, "---------------------------------\n\n");
    }
}

/* --------------------------------------------------------------------- */
/*  run_test_case (kernel)                                               */
/* --------------------------------------------------------------------- */

int run_test_case(const char *category, const char *input_file_base_name,
                  TestMode mode, TransformationType transform_type,
                  ExpectedOutcome expected_outcome,
                  const char *custom_cetus_flags)
{
    (void)transform_type;

    char input_file_full_path[MAX_PATH_LENGTH];
    char input_file_base_name_i[MAX_PATH_LENGTH];
    char input_file_base_name_no_ext[MAX_PATH_LENGTH];
    char suite_subdir[MAX_PATH_LENGTH];

    const char *input_files_root_dir = "input_files";
    const char *intermediate_i_root_dir = "cetus_intermediate_i_files";
    const char *transformed_output_root_dir = "cetus_transformed_output";
    const char *ground_truth_root_dir = "ground_truth";

    char intermediate_dir[MAX_PATH_LENGTH];
    char transformed_dir[MAX_PATH_LENGTH];
    char ground_truth_dir[MAX_PATH_LENGTH];

    char mkdir_cmd[MAX_PATH_LENGTH * 2];
    int cmd_result;

    suite_subdir[0] = '\0';
    {
        const char *slash = strrchr(input_file_base_name, '/');
        const char *bslash = strrchr(input_file_base_name, '\\');
        if (bslash && (!slash || bslash > slash))
            slash = bslash;
        if (slash && slash > input_file_base_name) {
            size_t suite_len = (size_t)(slash - input_file_base_name);
            if (suite_len >= sizeof(suite_subdir)) {
                log_test_outcome(TEST_FAILED_UNKNOWN, category,
                                 input_file_base_name,
                                 "Suite subdirectory path too long.", NULL);
                return 0;
            }
            memcpy(suite_subdir, input_file_base_name, suite_len);
            suite_subdir[suite_len] = '\0';
        }
    }

    if (suite_subdir[0] != '\0') {
        if (snprintf(intermediate_dir, sizeof(intermediate_dir), "%s/%s",
                     intermediate_i_root_dir,
                     suite_subdir) >= (int)sizeof(intermediate_dir) ||
            snprintf(transformed_dir, sizeof(transformed_dir), "%s/%s",
                     transformed_output_root_dir,
                     suite_subdir) >= (int)sizeof(transformed_dir) ||
            snprintf(ground_truth_dir, sizeof(ground_truth_dir), "%s/%s",
                     ground_truth_root_dir,
                     suite_subdir) >= (int)sizeof(ground_truth_dir)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category,
                             input_file_base_name,
                             "Path buffer overflow for suite directories.",
                             NULL);
            return 0;
        }
    } else {
        snprintf(intermediate_dir, sizeof(intermediate_dir), "%s",
                 intermediate_i_root_dir);
        snprintf(transformed_dir, sizeof(transformed_dir), "%s",
                 transformed_output_root_dir);
        snprintf(ground_truth_dir, sizeof(ground_truth_dir), "%s",
                 ground_truth_root_dir);
    }

    if (snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"",
                 intermediate_dir) >= (int)sizeof(mkdir_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "mkdir command buffer overflow for intermediate dir.",
                         NULL);
        return 0;
    }
    cmd_result = execute_command(mkdir_cmd);
    if (cmd_result != 0) {
        log_test_outcome(TEST_FAILED_MKDIR, category, input_file_base_name,
                         "Failed to create intermediate .i files directory.",
                         mkdir_cmd);
        return 0;
    }

    if (snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"",
                 transformed_dir) >= (int)sizeof(mkdir_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "mkdir command buffer overflow for output dir.",
                         NULL);
        return 0;
    }
    cmd_result = execute_command(mkdir_cmd);
    if (cmd_result != 0) {
        log_test_outcome(TEST_FAILED_MKDIR, category, input_file_base_name,
                         "Failed to create transformed output directory.",
                         mkdir_cmd);
        return 0;
    }

    if (snprintf(input_file_full_path, sizeof(input_file_full_path),
                 "%s/%s", input_files_root_dir,
                 input_file_base_name) >= (int)sizeof(input_file_full_path)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Path buffer overflow for input file.", NULL);
        return 0;
    }

    const char *input_basename = path_basename(input_file_base_name);
    char *dot = strrchr(input_basename, '.');
    if (dot != NULL) {
        size_t base_len = (size_t)(dot - input_basename);
        if (base_len + 2 >= MAX_PATH_LENGTH || base_len >= MAX_PATH_LENGTH) {
            log_test_outcome(TEST_FAILED_NO_EXTENSION, category,
                             input_file_base_name,
                             "Input file base name too long.", NULL);
            return 0;
        }
        snprintf(input_file_base_name_i, sizeof(input_file_base_name_i),
                 "%.*s.i", (int)base_len, input_basename);
        snprintf(input_file_base_name_no_ext,
                 sizeof(input_file_base_name_no_ext), "%.*s", (int)base_len,
                 input_basename);
    } else {
        log_test_outcome(
            TEST_FAILED_NO_EXTENSION, category, input_file_base_name,
            "Input file has no extension. Cannot derive .i/.c names.", NULL);
        return 0;
    }

    char preprocessed_input_file[MAX_PATH_LENGTH];
    char cetus_actual_output_file[MAX_PATH_LENGTH];
    char ground_truth_file_path[MAX_PATH_LENGTH];
    char preprocessor_command[MAX_PATH_LENGTH * 2];
    char cetus_command[MAX_PATH_LENGTH * 2];
    char semantic_check_command[MAX_PATH_LENGTH * 2];
    char diff_command[MAX_PATH_LENGTH * 2];
    char format_cmd[MAX_PATH_LENGTH * 2];

    /* --- 1. Preprocessing --- */
    if (snprintf(preprocessed_input_file, sizeof(preprocessed_input_file),
                 "%s/%s", intermediate_dir,
                 input_file_base_name_i) >=
        (int)sizeof(preprocessed_input_file)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Path buffer overflow for preprocessed file.", NULL);
        return 0;
    }
    verbose_printf("[%s] DEBUG: Preprocessing output will be in: %s\n",
                   get_current_time(), preprocessed_input_file);
    if (log_all_fp)
        fprintf(log_all_fp,
                "[%s] DEBUG: Preprocessing output will be in: %s\n",
                get_current_time(), preprocessed_input_file);

    if (snprintf(preprocessor_command, sizeof(preprocessor_command),
                 "%s -E -P -x c -std=c11 \"%s\" -o \"%s\"", CLANG_PATH,
                 input_file_full_path,
                 preprocessed_input_file) >=
        (int)sizeof(preprocessor_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Preprocessor command buffer overflow.", NULL);
        return 0;
    }

    verbose_printf("[%s] Manually preprocessing %s to %s...\n",
                   get_current_time(), input_file_full_path,
                   preprocessed_input_file);
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] Manually preprocessing %s to %s...\n",
                get_current_time(), input_file_full_path,
                preprocessed_input_file);
    cmd_result = execute_command(preprocessor_command);
    if (cmd_result != 0) {
        TestOutcome outcome_type = TEST_FAILED_PREPROCESS;
        char reason_buf[MAX_PATH_LENGTH];
        if (cmd_result == -999) {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Preprocessor CRASHED.");
            outcome_type = TEST_FAILED_CRASH;
        } else {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Preprocessing failed (exit code %d).", cmd_result);
        }
        log_test_outcome(outcome_type, category, input_file_base_name,
                         reason_buf, preprocessor_command);
        return 0;
    }
    verbose_printf(
        "[%s] Preprocessing successful. Output file: %s (size %ld bytes).\n",
        get_current_time(), preprocessed_input_file,
        file_size(preprocessed_input_file));
    if (log_all_fp)
        fprintf(log_all_fp,
                "[%s] Preprocessing successful. Output file: %s "
                "(size %ld bytes).\n",
                get_current_time(), preprocessed_input_file,
                file_size(preprocessed_input_file));

    /* --- 2. Semantic check --- */
    if (snprintf(semantic_check_command, sizeof(semantic_check_command),
                 "./check_syntax.sh \"%s\"",
                 preprocessed_input_file) >=
        (int)sizeof(semantic_check_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Semantic check command buffer overflow.", NULL);
        return 0;
    }
    verbose_printf("[%s] Running semantic check command: %s\n",
                   get_current_time(), semantic_check_command);
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] Running semantic check command: %s\n",
                get_current_time(), semantic_check_command);
    cmd_result = execute_command(semantic_check_command);
    if (cmd_result != 0) {
        TestOutcome outcome_type = TEST_FAILED_SEMANTIC_CHECK;
        char reason_buf[MAX_PATH_LENGTH];
        if (cmd_result == -999) {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Semantic check script CRASHED.");
            outcome_type = TEST_FAILED_CRASH;
        } else {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Semantic check failed (exit code %d).", cmd_result);
        }
        log_test_outcome(outcome_type, category, input_file_base_name,
                         reason_buf, semantic_check_command);
        return 0;
    }
    verbose_printf("[%s] Semantic check successful.\n", get_current_time());
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] Semantic check successful.\n",
                get_current_time());

    verbose_printf("[%s] Applying Cetus transformation...\n",
                   get_current_time());
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] Applying Cetus transformation...\n",
                get_current_time());

    /* --- 3. Cetus execution --- */
    if (snprintf(cetus_command, sizeof(cetus_command),
                 "%s -outdir=\"%s\" %s \"%s\"", CETUS_PATH, transformed_dir,
                 custom_cetus_flags,
                 preprocessed_input_file) >= (int)sizeof(cetus_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Cetus command buffer overflow.", NULL);
        return 0;
    }
    verbose_printf("[%s] DEBUG: Cetus command: %s\n", get_current_time(),
                   cetus_command);
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] DEBUG: Cetus command: %s\n",
                get_current_time(), cetus_command);

    cmd_result = execute_command(cetus_command);

    if (expected_outcome == EXPECT_FAILURE) {
        if (cmd_result == 0) {
            log_test_outcome(
                TEST_FAILED_UNKNOWN, category, input_file_base_name,
                "Cetus executed successfully (exit code 0), "
                "but a failure was expected.",
                cetus_command);
            return 0;
        }
        verbose_printf("[%s] Cetus failed as expected. Exit code: %d.\n",
                       get_current_time(), cmd_result);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] Cetus failed as expected. Exit code: %d.\n",
                    get_current_time(), cmd_result);
        log_test_outcome(TEST_PASSED, category, input_file_base_name,
                         "Cetus failed as expected.", cetus_command);
        return 1;
    }

    if (cmd_result != 0) {
        TestOutcome outcome_type = TEST_FAILED_UNKNOWN;
        char reason_buf[MAX_PATH_LENGTH];
        if (cmd_result == -999) {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Cetus CRASHED (terminated abnormally).");
            outcome_type = TEST_FAILED_CRASH;
        } else {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Cetus execution failed (returned non-zero exit "
                     "code %d).",
                     cmd_result);
        }
        log_test_outcome(outcome_type, category, input_file_base_name,
                         reason_buf, cetus_command);
        return 0;
    }
    verbose_printf("[%s] Cetus execution successful.\n", get_current_time());
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] Cetus execution successful.\n",
                get_current_time());

    if (snprintf(cetus_actual_output_file, sizeof(cetus_actual_output_file),
                 "%s/%s", transformed_dir,
                 input_file_base_name_i) >=
        (int)sizeof(cetus_actual_output_file)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Path buffer overflow for Cetus output file.", NULL);
        return 0;
    }
    verbose_printf("[%s] DEBUG: Cetus output is expected at: %s\n",
                   get_current_time(), cetus_actual_output_file);
    if (log_all_fp)
        fprintf(log_all_fp,
                "[%s] DEBUG: Cetus output is expected at: %s\n",
                get_current_time(), cetus_actual_output_file);

    if (snprintf(ground_truth_file_path, sizeof(ground_truth_file_path),
                 "%s/%s%s", ground_truth_dir, input_file_base_name_no_ext,
                 GROUND_TRUTH_SUFFIX) >=
        (int)sizeof(ground_truth_file_path)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Path buffer overflow for ground truth file.", NULL);
        return 0;
    }

    /* --- 4. Ground truth generation or comparison --- */
    if (mode == GENERATE_MODE) {
        verbose_printf("[%s] Generating ground truth for %s...\n",
                       get_current_time(), input_file_base_name);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] Generating ground truth for %s...\n",
                    get_current_time(), input_file_base_name);

        char ground_truth_dir_create_cmd[MAX_PATH_LENGTH * 2];
        if (snprintf(ground_truth_dir_create_cmd,
                     sizeof(ground_truth_dir_create_cmd), "mkdir -p \"%s\"",
                     ground_truth_dir) >=
            (int)sizeof(ground_truth_dir_create_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category,
                             input_file_base_name,
                             "mkdir command buffer overflow for GT dir.",
                             NULL);
            return 0;
        }
        cmd_result = execute_command(ground_truth_dir_create_cmd);
        if (cmd_result != 0) {
            log_test_outcome(TEST_FAILED_MKDIR, category,
                             input_file_base_name,
                             "Failed to create ground truth directory.",
                             ground_truth_dir_create_cmd);
            return 0;
        }

        if (access(cetus_actual_output_file, F_OK) != 0) {
            char reason_buf[MAX_PATH_LENGTH * 2];
            snprintf(reason_buf, sizeof(reason_buf),
                     "Cetus output file '%s' not found for GT generation. "
                     "(Error: %s)",
                     cetus_actual_output_file, strerror(errno));
            log_test_outcome(TEST_FAILED_MISSING_OUTPUT, category,
                             input_file_base_name, reason_buf, NULL);
            return 0;
        }

        char cp_cmd[MAX_PATH_LENGTH * 2];
        if (snprintf(cp_cmd, sizeof(cp_cmd), "cp \"%s\" \"%s\"",
                     cetus_actual_output_file,
                     ground_truth_file_path) >= (int)sizeof(cp_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category,
                             input_file_base_name,
                             "Copy command buffer overflow.", NULL);
            return 0;
        }
        cmd_result = execute_command(cp_cmd);
        if (cmd_result != 0) {
            char reason_buf[MAX_PATH_LENGTH * 2];
            snprintf(reason_buf, sizeof(reason_buf),
                     "Failed to generate ground truth (copy failed: %s).",
                     strerror(errno));
            log_test_outcome(TEST_FAILED_COPY_GROUND_TRUTH, category,
                             input_file_base_name, reason_buf, cp_cmd);
            return 0;
        }
        printf("[%s] PASS (generate): %s -> %s\n", get_current_time(),
               input_file_base_name, ground_truth_file_path);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] Ground truth generated successfully for %s "
                    "at %s.\n",
                    get_current_time(), input_file_base_name,
                    ground_truth_file_path);
        log_test_outcome(TEST_PASSED, category, input_file_base_name,
                         "Ground truth generated successfully.", NULL);
        return 1;
    }

    /* COMPARE_MODE */
    verbose_printf("[%s] Comparing Cetus output with ground truth for %s...\n",
                   get_current_time(), input_file_base_name);
    if (log_all_fp)
        fprintf(log_all_fp,
                "[%s] Comparing Cetus output with ground truth for %s...\n",
                get_current_time(), input_file_base_name);

    char reason_buf[MAX_PATH_LENGTH * 2];

    if (access(cetus_actual_output_file, F_OK) != 0) {
        snprintf(reason_buf, sizeof(reason_buf),
                 "Cetus did not produce its expected output file: '%s'. "
                 "(Error: %s)",
                 cetus_actual_output_file, strerror(errno));
        log_test_outcome(TEST_FAILED_MISSING_OUTPUT, category,
                         input_file_base_name, reason_buf, NULL);
        return 0;
    }

    if (access(ground_truth_file_path, F_OK) != 0) {
        snprintf(reason_buf, sizeof(reason_buf),
                 "Ground truth file not found: '%s'. (Error: %s)",
                 ground_truth_file_path, strerror(errno));
        log_test_outcome(TEST_FAILED_MISSING_GROUND_TRUTH, category,
                         input_file_base_name, reason_buf, NULL);
        return 0;
    }

    verbose_printf(
        "[%s] DEBUG: Formatting Cetus output file: %s with clang-format...\n",
        get_current_time(), cetus_actual_output_file);
    if (snprintf(format_cmd, sizeof(format_cmd),
                 "%s -i -style=Google \"%s\"", CLANG_FORMAT_PATH,
                 cetus_actual_output_file) >= (int)sizeof(format_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "clang-format command buffer overflow for output.",
                         NULL);
        return 0;
    }
    cmd_result = execute_command(format_cmd);
    if (cmd_result != 0) {
        snprintf(reason_buf, sizeof(reason_buf),
                 "Failed to format Cetus output with clang-format "
                 "(exit code %d).",
                 cmd_result);
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         reason_buf, format_cmd);
        return 0;
    }

    verbose_printf(
        "[%s] DEBUG: Formatting Ground Truth file: %s with clang-format...\n",
        get_current_time(), ground_truth_file_path);
    if (snprintf(format_cmd, sizeof(format_cmd),
                 "%s -i -style=Google \"%s\"", CLANG_FORMAT_PATH,
                 ground_truth_file_path) >= (int)sizeof(format_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "clang-format command buffer overflow for GT.", NULL);
        return 0;
    }
    cmd_result = execute_command(format_cmd);
    if (cmd_result != 0) {
        snprintf(reason_buf, sizeof(reason_buf),
                 "Failed to format Ground Truth with clang-format "
                 "(exit code %d).",
                 cmd_result);
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         reason_buf, format_cmd);
        return 0;
    }

    long input_file_initial_size = file_size(input_file_full_path);
    long cetus_output_file_size = file_size(cetus_actual_output_file);
    long ground_truth_file_size = file_size(ground_truth_file_path);

    if (input_file_initial_size == -1 || cetus_output_file_size == -1 ||
        ground_truth_file_size == -1) {
        snprintf(reason_buf, sizeof(reason_buf),
                 "Failed to get file sizes for comparison. (Error: %s)",
                 strerror(errno));
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         reason_buf, NULL);
        return 0;
    }

    if (snprintf(diff_command, sizeof(diff_command),
                 "diff -wB \"%s\" \"%s\"", cetus_actual_output_file,
                 ground_truth_file_path) >= (int)sizeof(diff_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                         "Diff command buffer overflow.", NULL);
        return 0;
    }
    verbose_printf("[%s] DEBUG: Diff command: %s\n", get_current_time(),
                   diff_command);
    if (log_all_fp)
        fprintf(log_all_fp, "[%s] DEBUG: Diff command: %s\n",
                get_current_time(), diff_command);

    int diff_result = execute_command(diff_command);

    if (diff_result != 0) {
        TestOutcome outcome_to_log = TEST_FAILED_DIFF;
        if (expected_outcome == EXPECT_SUCCESS_TRANSFORMED) {
            if (cetus_output_file_size <=
                (input_file_initial_size * 1.1)) {
                outcome_to_log = TEST_FAILED_MISSED_OPPORTUNITY;
                snprintf(reason_buf, sizeof(reason_buf),
                         "Cetus output differs from ground truth after "
                         "formatting, possibly untransformed.");
            } else {
                outcome_to_log = TEST_FAILED_INCORRECT_TRANSFORMATION;
                snprintf(reason_buf, sizeof(reason_buf),
                         "Cetus output differs from ground truth after "
                         "formatting. Incorrect transformation.");
            }
        } else if (expected_outcome == EXPECT_SUCCESS_NO_CHANGE) {
            outcome_to_log = TEST_FAILED_INCORRECT_TRANSFORMATION;
            snprintf(reason_buf, sizeof(reason_buf),
                     "Cetus output differs from ground truth after "
                     "formatting. No transformation was expected.");
        } else {
            snprintf(reason_buf, sizeof(reason_buf),
                     "Output does NOT match ground truth (diff found "
                     "after formatting).");
        }
        log_test_outcome(outcome_to_log, category, input_file_base_name,
                         reason_buf, diff_command);
        return 0;
    }

    printf("[%s] \xe2\x9c\x85 SUCCESS: %s passed. Output matches ground "
           "truth after formatting.\n",
           get_current_time(), input_file_base_name);
    if (log_all_fp)
        fprintf(log_all_fp,
                "[%s] SUCCESS: %s passed. Output matches ground truth "
                "after formatting.\n",
                get_current_time(), input_file_base_name);
    log_test_outcome(TEST_PASSED, category, input_file_base_name,
                     "Output matches ground truth after formatting.", NULL);
    return 1;
}

/* --------------------------------------------------------------------- */
/*  run_benchmarks - execute selected benchmark cases                    */
/* --------------------------------------------------------------------- */

static int run_benchmarks(const CliOptions *opts,
                          BenchmarkSummary *summary)
{
    BenchmarkConfig config;
    BenchmarkSelection selection;
    BenchmarkRunnerOptions bench_opts;
    char error[2048];
    char **saved_datasets = NULL;
    char **saved_classes = NULL;
    size_t override_count = 0;
    size_t i;
    int result = 0;
    char *abs_project_root = NULL;

    memset(summary, 0, sizeof(*summary));
    benchmark_config_init(&config);
    if (benchmark_config_load(opts->config_dir, &config, error,
                              sizeof(error)) != 0) {
        fprintf(stderr,
                "ERROR: Failed to load benchmark config from '%s': %s\n",
                opts->config_dir, error);
        benchmark_config_free(&config);
        return 1;
    }

    /* Apply dataset / class overrides (swap-and-restore). */
    if (opts->dataset != NULL || opts->npb_class != NULL) {
        saved_datasets = calloc(config.suite_count, sizeof(char *));
        saved_classes = calloc(config.suite_count, sizeof(char *));
        if (saved_datasets == NULL || saved_classes == NULL) {
            fprintf(stderr, "ERROR: Out of memory for overrides.\n");
            free(saved_datasets);
            free(saved_classes);
            benchmark_config_free(&config);
            return 1;
        }
        for (i = 0; i < config.suite_count; ++i) {
            int ds_swapped = 0;
            if (opts->dataset != NULL) {
                saved_datasets[i] = config.suites[i].dataset;
                config.suites[i].dataset = strdup(opts->dataset);
                if (config.suites[i].dataset == NULL) {
                    fprintf(stderr,
                            "ERROR: strdup failed for dataset override.\n");
                    config.suites[i].dataset = saved_datasets[i];
                    result = 1;
                    goto cleanup;
                }
                ds_swapped = 1;
            }
            if (opts->npb_class != NULL) {
                saved_classes[i] = config.suites[i].npb_class;
                config.suites[i].npb_class = strdup(opts->npb_class);
                if (config.suites[i].npb_class == NULL) {
                    fprintf(stderr,
                            "ERROR: strdup failed for class override.\n");
                    config.suites[i].npb_class = saved_classes[i];
                    if (ds_swapped) {
                        free(config.suites[i].dataset);
                        config.suites[i].dataset = saved_datasets[i];
                    }
                    result = 1;
                    goto cleanup;
                }
            }
            override_count = i + 1;
        }
    }

    benchmark_selection_init(&selection);
    if (benchmark_runner_select(
            &config,
            opts->benchmark_suite_ids.length > 0
                ? &opts->benchmark_suite_ids
                : NULL,
            opts->benchmark_kernel, opts->profile, &selection, error,
            sizeof(error)) != 0) {
        fprintf(stderr, "ERROR: Benchmark selection failed: %s\n", error);
        result = 1;
        goto cleanup;
    }

    if (selection.length == 0) {
        fprintf(stderr, "ERROR: No benchmark cases matched the given "
                        "filters.\n");
        result = 1;
        goto cleanup;
    }

    printf("\n--- Running %zu benchmark case(s) ---\n", selection.length);
    printf("--------------------------------------------------\n");

    benchmark_runner_options_init(&bench_opts);
    if (abs_project_root == NULL) {
        char resolved[PATH_MAX];
        if (realpath(".", resolved) != NULL) {
            abs_project_root = strdup(resolved);
        } else {
            abs_project_root = getcwd(resolved, sizeof(resolved)) != NULL
                                   ? strdup(resolved)
                                   : NULL;
        }
        if (abs_project_root == NULL) {
            fprintf(stderr,
                    "ERROR: cannot resolve absolute project root: %s\n",
                    strerror(errno));
            result = 1;
            goto cleanup;
        }
    }
    printf("Detailed logs under logs/benchmarks/ "
           "(and logs/ for kernel tests).\n");
    printf("--------------------------------------------------\n");
    bench_opts.project_root = abs_project_root;
    bench_opts.timeout_seconds = opts->timeout_seconds;
    bench_opts.generate = opts->generate;
    bench_opts.skip_ground_truth = opts->skip_gt;
    bench_opts.skip_run = opts->skip_run;

    for (i = 0; i < selection.length; ++i) {
        BenchmarkSelectionItem *item = &selection.items[i];
        BenchmarkCase test_case;
        BenchmarkAdapterInstance adapter;
        BenchmarkCaseResult case_result;
        const char *case_id;
        const char *message;

        test_case.suite = item->suite;
        test_case.kernel = item->kernel;
        test_case.profile = item->profile;
        case_id = item->kernel->name;

        printf("\n--- Running Benchmark [%zu/%zu]: %s/%s profile=%s ---\n",
               i + 1, selection.length, item->suite->id, case_id,
               item->profile->name);

        if (benchmark_adapter_create(item->suite->adapter, &adapter,
                                     error, sizeof(error)) != 0) {
            BenchmarkCaseResult fail_result;
            memset(&fail_result, 0, sizeof(fail_result));
            fail_result.status = BENCHMARK_STATUS_FAIL;
            fail_result.stage = BENCHMARK_STAGE_SETUP;
            fprintf(stderr,
                    "[%s] \xe2\x9d\x8c FAIL: %s/%s (%s) failed at setup: %s\n",
                    get_current_time(), item->suite->id, case_id,
                    item->profile->name, error);
            benchmark_summary_add(summary, &fail_result);
            continue;
        }

        if (benchmark_runner_run_case(&config, &test_case, &adapter,
                                      &bench_opts, &case_result, error,
                                      sizeof(error)) != 0) {
            BenchmarkCaseResult fail_result;
            memset(&fail_result, 0, sizeof(fail_result));
            fail_result.status = BENCHMARK_STATUS_FAIL;
            fail_result.stage = BENCHMARK_STAGE_SETUP;
            fprintf(stderr,
                    "[%s] \xe2\x9d\x8c FAIL: %s/%s (%s) failed at setup: %s\n",
                    get_current_time(), item->suite->id, case_id,
                    item->profile->name, error);
            benchmark_summary_add(summary, &fail_result);
            benchmark_adapter_destroy(&adapter);
            continue;
        }

        message = case_result.message != NULL ? case_result.message : "";
        switch (case_result.status) {
        case BENCHMARK_STATUS_PASS:
            if (case_result.stage == BENCHMARK_STAGE_DONE ||
                case_result.stage == BENCHMARK_STAGE_VERIFY) {
                if (message[0] != '\0') {
                    printf("[%s] \xe2\x9c\x85 SUCCESS: %s/%s (%s) passed. "
                           "Ground truth matched; program reported: %s\n",
                           get_current_time(), item->suite->id, case_id,
                           item->profile->name, message);
                } else {
                    printf("[%s] \xe2\x9c\x85 SUCCESS: %s/%s (%s) passed. "
                           "Ground truth matched and verification "
                           "succeeded.\n",
                           get_current_time(), item->suite->id, case_id,
                           item->profile->name);
                }
            } else if (case_result.stage == BENCHMARK_STAGE_GROUND_TRUTH) {
                printf("[%s] \xe2\x9c\x85 SUCCESS: %s/%s (%s) passed. "
                       "Output matches ground truth after formatting.\n",
                       get_current_time(), item->suite->id, case_id,
                       item->profile->name);
            } else {
                printf("[%s] \xe2\x9c\x85 SUCCESS: %s/%s (%s) passed "
                       "at stage %s.\n",
                       get_current_time(), item->suite->id, case_id,
                       item->profile->name,
                       benchmark_stage_name(case_result.stage));
            }
            break;
        case BENCHMARK_STATUS_XFAIL:
            printf("[%s] \xe2\x9a\xa0 XFAIL: %s/%s (%s) expected failure "
                   "at %s",
                   get_current_time(), item->suite->id, case_id,
                   item->profile->name,
                   benchmark_stage_name(case_result.stage));
            if (g_verbose && message[0] != '\0')
                printf(": %s", message);
            printf("\n");
            break;
        case BENCHMARK_STATUS_XPASS:
            printf("[%s] \xe2\x9d\x8c XPASS: %s/%s (%s) unexpectedly "
                   "passed at %s",
                   get_current_time(), item->suite->id, case_id,
                   item->profile->name,
                   benchmark_stage_name(case_result.stage));
            if (g_verbose && message[0] != '\0')
                printf(": %s", message);
            printf("\n");
            break;
        case BENCHMARK_STATUS_SKIP:
            printf("[%s] SKIP: %s/%s (%s) skipped at %s",
                   get_current_time(), item->suite->id, case_id,
                   item->profile->name,
                   benchmark_stage_name(case_result.stage));
            if (message[0] != '\0')
                printf(": %s", message);
            printf("\n");
            break;
        case BENCHMARK_STATUS_FAIL:
        default:
            printf("[%s] \xe2\x9d\x8c FAIL: %s/%s (%s) failed at %s",
                   get_current_time(), item->suite->id, case_id,
                   item->profile->name,
                   benchmark_stage_name(case_result.stage));
            if (message[0] != '\0') {
                /* Keep console short; full Cetus/tool output is in the log. */
                if (!g_verbose && strlen(message) > 120) {
                    printf(": %.117s...", message);
                } else {
                    printf(": %s", message);
                }
            }
            printf("\n");
            break;
        }
        if (case_result.log_path != NULL)
            printf("         log: %s\n", case_result.log_path);

        benchmark_summary_add(summary, &case_result);
        benchmark_case_result_free(&case_result);
        benchmark_adapter_destroy(&adapter);
    }

    printf("--------------------------------------------------\n");
    printf("[%s] Benchmark Summary: %zu total, %zu pass, %zu fail, "
           "%zu xfail, %zu xpass, %zu skip\n",
           get_current_time(), summary->total, summary->pass,
           summary->fail, summary->xfail, summary->xpass, summary->skip);

    if (benchmark_summary_exit_code(summary) != 0)
        result = 1;

cleanup:
    benchmark_selection_free(&selection);
    if (saved_datasets != NULL || saved_classes != NULL) {
        for (i = 0; i < override_count; ++i) {
            if (opts->dataset != NULL) {
                free(config.suites[i].dataset);
                config.suites[i].dataset = saved_datasets[i];
            }
            if (opts->npb_class != NULL) {
                free(config.suites[i].npb_class);
                config.suites[i].npb_class = saved_classes[i];
            }
        }
        free(saved_datasets);
        free(saved_classes);
    }
    free(abs_project_root);
    benchmark_config_free(&config);
    return result;
}

/* --------------------------------------------------------------------- */
/*  main                                                                 */
/* --------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    int overall_exit_status = 0;
    CliOptions opts;
    char cli_error[1024];
    int total_tests = 0;
    int passed_tests = 0;
    BenchmarkSummary bench_summary;
    int do_kernels;
    int do_benchmarks;

    memset(&bench_summary, 0, sizeof(bench_summary));

    if (cli_parse(argc, argv, &opts, cli_error, sizeof(cli_error)) != 0) {
        fprintf(stderr, "ERROR: %s\n", cli_error);
        cli_print_usage(argv[0]);
        cli_options_free(&opts);
        return 1;
    }

    g_verbose = opts.verbose;

    /* --- Early-exit modes --- */
    if (opts.mode == CLI_MODE_HELP) {
        cli_print_usage(argv[0]);
        cli_options_free(&opts);
        return 0;
    }
    if (opts.mode == CLI_MODE_LIST_SUITES) {
        list_suites();
        cli_options_free(&opts);
        return 0;
    }
    if (opts.mode == CLI_MODE_LIST_BENCHMARK_SUITES) {
        int rc = list_benchmark_suites(opts.config_dir);
        cli_options_free(&opts);
        return rc;
    }

    /* --- Create log directory and open log files --- */
    if (system("mkdir -p logs") != 0) {
        fprintf(stderr,
                "[%s] ERROR: Failed to create 'logs/' directory. "
                "Exiting.\n",
                get_current_time());
        cli_options_free(&opts);
        return 1;
    }

    log_all_fp = fopen("logs/all_tests.log", "w");
    log_passed_fp = fopen("logs/passed_tests.log", "w");
    log_failed_fp = fopen("logs/failed_tests.log", "w");
    log_crashes_fp = fopen("logs/crashes_test.log", "w");
    log_missed_opportunities_fp = fopen("logs/missed_opportunities.log", "w");
    log_incorrect_transformation_fp =
        fopen("logs/incorrect_transformation.log", "w");

    if (!log_all_fp || !log_passed_fp || !log_failed_fp || !log_crashes_fp ||
        !log_missed_opportunities_fp || !log_incorrect_transformation_fp) {
        fprintf(stderr,
                "[%s] ERROR: Failed to open one or more log files.\n",
                get_current_time());
        overall_exit_status = 1;
        goto cleanup_logs;
    }

    do_kernels =
        (opts.mode == CLI_MODE_KERNELS || opts.mode == CLI_MODE_FULL);
    do_benchmarks =
        (opts.mode == CLI_MODE_BENCHMARKS || opts.mode == CLI_MODE_FULL);

    /* ================================================================= */
    /*  KERNEL TESTS                                                     */
    /* ================================================================= */
    if (do_kernels) {
        TestMode current_mode =
            opts.generate ? GENERATE_MODE : COMPARE_MODE;

        if (opts.test_identifier != NULL) {
            /* --- Run one specific kernel test --- */
            TestCase *found_test = NULL;
            for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
                const char *basename =
                    path_basename(master_test_cases[i].input_file_base_name);
                if (strcmp(master_test_cases[i].category,
                          opts.test_identifier) == 0 ||
                    strcmp(master_test_cases[i].input_file_base_name,
                          opts.test_identifier) == 0 ||
                    strcmp(basename, opts.test_identifier) == 0) {
                    found_test = &master_test_cases[i];
                    break;
                }
            }
            if (found_test == NULL) {
                fprintf(stderr,
                        "[%s] ERROR: Test with identifier '%s' not "
                        "found in master_test_cases.h.\n",
                        get_current_time(), opts.test_identifier);
                overall_exit_status = 1;
            } else {
                printf("\n--- Running Specific Test: %s/%s ---\n",
                       found_test->category,
                       found_test->input_file_base_name);
                if (run_test_case(
                        found_test->category,
                        found_test->input_file_base_name, current_mode,
                        found_test->transform_type,
                        found_test->expected_outcome,
                        opts.custom_cetus_options != NULL
                            ? opts.custom_cetus_options
                            : found_test->cetus_flags)) {
                    passed_tests++;
                } else {
                    overall_exit_status = 1;
                }
                total_tests++;
            }
        } else {
            /* --- Run all or suite-filtered kernel tests --- */
            int selected[NUM_MASTER_TEST_CASES];
            int selected_count = 0;
            for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
                if (opts.suite_identifier == NULL ||
                    test_in_suite(&master_test_cases[i],
                                  opts.suite_identifier)) {
                    selected[selected_count++] = i;
                }
            }

            if (opts.suite_identifier != NULL && selected_count == 0) {
                fprintf(stderr,
                        "[%s] ERROR: Suite '%s' not found. "
                        "Use --list-suites.\n",
                        get_current_time(), opts.suite_identifier);
                overall_exit_status = 1;
            } else {
                if (opts.suite_identifier != NULL) {
                    printf("[%s] Running suite '%s' (%d test(s))...\n",
                           get_current_time(), opts.suite_identifier,
                           selected_count);
                } else {
                    printf("[%s] Running ALL kernel tests...\n",
                           get_current_time());
                }
                printf("-----------------------------------------------"
                       "---\n");
                for (int s = 0; s < selected_count; ++s) {
                    TestCase *current_test =
                        &master_test_cases[selected[s]];
                    printf("\n--- Running Test [%d/%d]: %s/%s ---\n",
                           s + 1, selected_count,
                           current_test->category,
                           current_test->input_file_base_name);
                    if (run_test_case(
                            current_test->category,
                            current_test->input_file_base_name,
                            current_mode, current_test->transform_type,
                            current_test->expected_outcome,
                            opts.custom_cetus_options != NULL
                                ? opts.custom_cetus_options
                                : current_test->cetus_flags)) {
                        passed_tests++;
                    } else {
                        overall_exit_status = 1;
                    }
                    total_tests++;
                }
                printf("\n---------------------------------------------"
                       "-----\n");
            }
        }

        printf("[%s] Kernel Summary: %d/%d tests passed.\n",
               get_current_time(), passed_tests, total_tests);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] Kernel Summary: %d/%d tests passed.\n",
                    get_current_time(), passed_tests, total_tests);
    }

    /* ================================================================= */
    /*  BENCHMARK TESTS                                                  */
    /* ================================================================= */
    if (do_benchmarks) {
        if (run_benchmarks(&opts, &bench_summary) != 0)
            overall_exit_status = 1;
    }

    /* ================================================================= */
    /*  COMBINED SUMMARY (--full only)                                   */
    /* ================================================================= */
    if (opts.mode == CLI_MODE_FULL) {
        printf("\n=== Combined Summary ===\n");
        printf("  Kernels:    %d/%d passed\n", passed_tests, total_tests);
        printf("  Benchmarks: %zu/%zu passed (%zu fail, %zu xfail, "
               "%zu xpass, %zu skip)\n",
               bench_summary.pass, bench_summary.total, bench_summary.fail,
               bench_summary.xfail, bench_summary.xpass,
               bench_summary.skip);
    } else if (!do_benchmarks && do_kernels) {
        /* Kernel-only: print the legacy "Test Summary" line. */
        printf("[%s] Test Summary: %d/%d tests passed.\n",
               get_current_time(), passed_tests, total_tests);
        if (log_all_fp)
            fprintf(log_all_fp,
                    "[%s] Test Summary: %d/%d tests passed.\n",
                    get_current_time(), passed_tests, total_tests);
    }

cleanup_logs:
    if (log_all_fp)
        fclose(log_all_fp);
    if (log_passed_fp)
        fclose(log_passed_fp);
    if (log_failed_fp)
        fclose(log_failed_fp);
    if (log_crashes_fp)
        fclose(log_crashes_fp);
    if (log_missed_opportunities_fp)
        fclose(log_missed_opportunities_fp);
    if (log_incorrect_transformation_fp)
        fclose(log_incorrect_transformation_fp);

    cli_options_free(&opts);
    return overall_exit_status;
}
