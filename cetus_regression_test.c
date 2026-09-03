// cetus_regression_test.c - The main test runner source file

// --- Header Includes ---
#include "helper_tests.h"        // Contains common definitions, enums, TestCase struct
#include "master_test_cases.h"   // Contains the array of TestCase definitions

// Standard C Library Includes (essential for the functions used below)
#include <stdio.h>               // For printf, fprintf, fopen, fclose, FILE, snprintf
#include <stdlib.h>              // For system, exit
#include <string.h>              // For strcmp, strlen, strrchr, strerror, strsignal
#include <time.h>                // For time, localtime, strftime (used by get_current_time)
#include <unistd.h>              // For access (F_OK)
#include <sys/wait.h>            // For WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG (used by execute_command)
#include <sys/stat.h>            // For stat (used by file_size)
#include <errno.h>               // For errno
#include <ctype.h>               // For tolower (suite name matching)


// --- Global Log File Pointers (DEFINITIONS) ---
// These are declared extern in helper_tests.h and defined here.
FILE *log_all_fp = NULL;
FILE *log_passed_fp = NULL;
FILE *log_failed_fp = NULL;
FILE *log_crashes_fp = NULL;
FILE *log_missed_opportunities_fp = NULL;
FILE *log_incorrect_transformation_fp = NULL;


// --- Helper Function Implementations ---
// These functions are declared in helper_tests.h and implemented here.

/**
 * @brief Returns the current timestamp as a string.
 * @return A static char array containing the formatted time string.
 */
char* get_current_time() {
    static char time_str[20]; // Static to retain value after function returns
    time_t timer;
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

/**
 * @brief Gets the size of a file.
 * @param filename The path to the file.
 * @return The size of the file in bytes, or -1 on error.
 */
long file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1; // Error getting file size
}

/**
 * @brief Executes a shell command and returns its exit status.
 * @param command The shell command string to execute.
 * @return The exit status of the command, -1 if system() fails, or -999 if terminated by signal.
 */
int execute_command(const char *command) {
    printf("[%s] DEBUG: Executing command: %s\n", get_current_time(), command);
    if (log_all_fp) fprintf(log_all_fp, "[%s] DEBUG: Executing command: %s\n", get_current_time(), command);

    // system() returns the termination status of the command.
    // We need to use WIFEXITED and WEXITSTATUS macros to get the actual exit code.
    int result = system(command);

    if (result == -1) {
        // system() itself failed to execute the command (e.g., memory allocation error)
        fprintf(stderr, "[%s] ERROR: Failed to execute command: %s (System call failed: %s).\n",
                get_current_time(), command, strerror(errno));
        if (log_all_fp) fprintf(log_all_fp, "[%s] ERROR: Failed to execute command: %s (System call failed: %s).\n",
                                get_current_time(), command, strerror(errno));
        return -1; // Indicate failure to execute
    }

    if (WIFEXITED(result)) {
        // Command terminated normally
        int exit_status = WEXITSTATUS(result);
        printf("[%s] Command exited with status: %d\n", get_current_time(), exit_status);
        if (log_all_fp) fprintf(log_all_fp, "[%s] Command exited with status: %d\n", get_current_time(), exit_status);
        return exit_status;
    } else {
        // Command terminated abnormally (e.g., by signal)
        if (WIFSIGNALED(result)) {
            int signal_num = WTERMSIG(result);
            fprintf(stderr, "[%s] ERROR: Command terminated by signal %d (%s): %s\n",
                    get_current_time(), signal_num, strsignal(signal_num), command);
            if (log_all_fp) fprintf(log_all_fp, "[%s] ERROR: Command terminated by signal %d (%s): %s\n",
                                    get_current_time(), signal_num, strsignal(signal_num), command);
        } else {
            fprintf(stderr, "[%s] ERROR: Command did not terminate normally (unknown reason): %s\n",
                    get_current_time(), command);
            if (log_all_fp) fprintf(log_all_fp, "[%s] ERROR: Command did not terminate normally (unknown reason): %s\n",
                                    get_current_time(), command);
        }
        return -999; // Indicate abnormal termination
    }
}

/**
 * @brief Converts a TransformationType enum to its string representation.
 * @param type The TransformationType enum value.
 * @return A string literal representing the transformation type.
 */
const char* transformation_type_to_string(TransformationType type) {
    switch (type) {
        case TRANSFORM_PARALLELIZATION: return "parallelization";
        case TRANSFORM_PRIVATIZATION: return "privatization";
        case TRANSFORM_REDUCTION: return "reduction";
        case TRANSFORM_TILING: return "tiling";
        case TRANSFORM_SUBSUB_ANALYSIS: return "subsub_analysis";
        case TRANSFORM_NONE: return "no_transformation";
        case TRANSFORM_UNKNOWN:
        default: return "unknown";
    }
}

/**
 * @brief Converts a string to its TransformationType enum representation.
 * @param str The string to convert.
 * @return The corresponding TransformationType enum value.
 */
TransformationType string_to_transformation_type(const char* str) {
    if (strcmp(str, "parallelization") == 0) return TRANSFORM_PARALLELIZATION;
    if (strcmp(str, "privatization") == 0) return TRANSFORM_PRIVATIZATION;
    if (strcmp(str, "reduction") == 0) return TRANSFORM_REDUCTION;
    if (strcmp(str, "tiling") == 0) return TRANSFORM_TILING;
    if (strcmp(str, "subsub_analysis") == 0) return TRANSFORM_SUBSUB_ANALYSIS;
    if (strcmp(str, "no_transformation") == 0) return TRANSFORM_NONE;
    return TRANSFORM_UNKNOWN; // Default for any unmapped string
}

/**
 * @brief Converts an ExpectedOutcome enum to its string representation.
 * @param outcome The ExpectedOutcome enum value.
 * @return A string literal representing the expected outcome.
 */
const char* expected_outcome_to_string(ExpectedOutcome outcome) {
    switch (outcome) {
        case EXPECT_SUCCESS_TRANSFORMED: return "success_transformed";
        case EXPECT_SUCCESS_NO_CHANGE: return "success_no_change";
        case EXPECT_FAILURE: return "expect_failure";
        case EXPECT_UNKNOWN:
        default: return "unknown";
    }
}

/**
 * @brief Converts a string to its ExpectedOutcome enum representation.
 * @param str The string to convert.
 * @return The corresponding ExpectedOutcome enum value.
 */
ExpectedOutcome string_to_expected_outcome(const char* str) {
    if (strcmp(str, "success_transformed") == 0) return EXPECT_SUCCESS_TRANSFORMED;
    if (strcmp(str, "success_no_change") == 0) return EXPECT_SUCCESS_NO_CHANGE;
    if (strcmp(str, "expect_failure") == 0) return EXPECT_FAILURE;
    return EXPECT_UNKNOWN;
}

/** Last path component of a relative input path (works with / and \\). */
static const char* path_basename(const char* path) {
    const char* slash = strrchr(path, '/');
    const char* bslash = strrchr(path, '\\');
    if (bslash && (!slash || bslash > slash)) {
        slash = bslash;
    }
    return slash ? slash + 1 : path;
}

static int str_eq_ci(const char* a, const char* b) {
    if (!a || !b) {
        return 0;
    }
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/** True if the test belongs to the named suite (aliases: paw, paw_tiling, subsub_analysis). */
static int test_in_suite(const TestCase* t, const char* suite) {
    if (!t || !t->suite || !suite) {
        return 0;
    }
    if (str_eq_ci(t->suite, suite)) {
        return 1;
    }
    if ((str_eq_ci(suite, "paw") || str_eq_ci(suite, "paw_tiling")) &&
        str_eq_ci(t->suite, "tiling")) {
        return 1;
    }
    if ((str_eq_ci(suite, "subsub_analysis") || str_eq_ci(suite, "subscripted-subscript")) &&
        str_eq_ci(t->suite, "subsub")) {
        return 1;
    }
    return 0;
}

static void print_usage(const char* argv0) {
    fprintf(stderr, "\nUsage: %s [--generate] [-cetus-options \"<flags>\"] [--run-test <id>] [--run-suite <name>] [--list-suites]\n", argv0);
    fprintf(stderr, "  --generate: Generate ground truth files instead of comparing.\n");
    fprintf(stderr, "  -cetus-options \"<flags>\": Pass custom Cetus options for the current run.\n");
    fprintf(stderr, "  --run-test <id>: Run one test (category, filename, or input path).\n");
    fprintf(stderr, "  --run-suite <name>: Run all tests in a suite (tiling, subsub).\n");
    fprintf(stderr, "  --list-suites: Print defined suites and their test counts.\n");
    fprintf(stderr, "  --all (or no args): Run all tests defined in master_test_cases.h.\n");
}

static void list_suites(void) {
    printf("Defined suites:\n");
    for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
        const char* name = master_test_cases[i].suite;
        if (!name) {
            continue;
        }
        int seen = 0;
        for (int j = 0; j < i; ++j) {
            if (master_test_cases[j].suite && str_eq_ci(master_test_cases[j].suite, name)) {
                seen = 1;
                break;
            }
        }
        if (seen) {
            continue;
        }
        int count = 0;
        for (int k = 0; k < NUM_MASTER_TEST_CASES; ++k) {
            if (test_in_suite(&master_test_cases[k], name)) {
                count++;
            }
        }
        printf("  %-12s  %d test(s)   --run-suite %s\n", name, count, name);
    }
}

/**
 * @brief Logs the outcome of a test to appropriate log files.
 * @param outcome The actual outcome of the test.
 * @param category The test category.
 * @param input_file_base_name The base name of the input file.
 * @param reason_message An optional message explaining the outcome.
 * @param command_executed The command that was executed, if applicable.
 */
void log_test_outcome(TestOutcome outcome, const char* category, const char* input_file_base_name,
                      const char* reason_message, const char* command_executed) {
    FILE* target_fp = NULL;
    const char* outcome_str = "UNKNOWN";

    switch (outcome) {
        case TEST_PASSED: target_fp = log_passed_fp; outcome_str = "PASSED"; break;
        case TEST_FAILED_CRASH: target_fp = log_crashes_fp; outcome_str = "CRASHED"; break;
        case TEST_FAILED_DIFF: target_fp = log_failed_fp; outcome_str = "DIFF_MISMATCH"; break;
        case TEST_FAILED_PREPROCESS: target_fp = log_failed_fp; outcome_str = "PREPROCESS_FAILED"; break;
        case TEST_FAILED_SEMANTIC_CHECK: target_fp = log_failed_fp; outcome_str = "SEMANTIC_CHECK_FAILED"; break;
        case TEST_FAILED_MKDIR: target_fp = log_failed_fp; outcome_str = "MKDIR_FAILED"; break;
        case TEST_FAILED_COPY_GROUND_TRUTH: target_fp = log_failed_fp; outcome_str = "COPY_GROUND_TRUTH_FAILED"; break;
        case TEST_FAILED_MISSING_OUTPUT: target_fp = log_failed_fp; outcome_str = "MISSING_OUTPUT"; break;
        case TEST_FAILED_MISSING_GROUND_TRUTH: target_fp = log_failed_fp; outcome_str = "MISSING_GROUND_TRUTH"; break;
        case TEST_FAILED_NO_EXTENSION: target_fp = log_failed_fp; outcome_str = "NO_EXTENSION"; break;
        case TEST_FAILED_MISSED_OPPORTUNITY: target_fp = log_missed_opportunities_fp; outcome_str = "MISSED_OPPORTUNITY"; break;
        case TEST_FAILED_INCORRECT_TRANSFORMATION: target_fp = log_incorrect_transformation_fp; outcome_str = "INCORRECT_TRANSFORMATION"; break;
        case TEST_FAILED_UNKNOWN: default: target_fp = log_failed_fp; outcome_str = "UNKNOWN_FAILURE"; break;
    }

    // Log to all_tests.log always
    if (log_all_fp) {
        fprintf(log_all_fp, "[%s] TEST: %s/%s - OUTCOME: %s\n", get_current_time(), category, input_file_base_name, outcome_str);
        if (reason_message && strlen(reason_message) > 0) { fprintf(log_all_fp, "[%s] REASON: %s\n", get_current_time(), reason_message); }
        if (command_executed && strlen(command_executed) > 0) { fprintf(log_all_fp, "[%s] COMMAND: %s\n", get_current_time(), command_executed); }
        fprintf(log_all_fp, "---------------------------------\n\n");
    }

    // Log to specific log file if it's not the 'all' log
    if (target_fp && target_fp != log_all_fp) {
        fprintf(target_fp, "[%s] TEST: %s/%s - OUTCOME: %s\n", get_current_time(), category, input_file_base_name, outcome_str);
        if (reason_message && strlen(reason_message) > 0) { fprintf(target_fp, "[%s] REASON: %s\n", get_current_time(), reason_message); }
        if (command_executed && strlen(command_executed) > 0) { fprintf(target_fp, "[%s] COMMAND: %s\n", get_current_time(), command_executed); }
        fprintf(target_fp, "---------------------------------\n\n");
    }

    // Print to stderr for immediate visibility of failures
    if (outcome != TEST_PASSED) {
        fprintf(stderr, "\n--- TEST FAILED ---\n");
        fprintf(stderr, "TEST: %s/%s\n", category, input_file_base_name);
        fprintf(stderr, "[%s] OUTCOME: %s\n", get_current_time(), outcome_str);
        if (reason_message && strlen(reason_message) > 0) { fprintf(stderr, "[%s] REASON: %s\n", get_current_time(), reason_message); }
        if (command_executed && strlen(command_executed) > 0) { fprintf(stderr, "[%s] COMMAND: %s\n", get_current_time(), command_executed); }
        fprintf(stderr, "---------------------------------\n\n");
    }
}


/**
 * @brief Runs a single test case.
 *
 * This function handles the full workflow for a single test:
 * 1. Directory setup.
 * 2. Preprocessing input file using Clang.
 * 3. Running semantic check script.
 * 4. Executing Cetus with specified flags.
 * 5. Comparing output with ground truth or generating ground truth.
 *
 * @param category The category of the test (e.g., "parallelization").
 * @param input_file_base_name The base name of the input source file (e.g., "my_test.c").
 * @param mode The test mode (COMPARE_MODE or GENERATE_MODE).
 * @param transform_type The expected transformation type for logging/heuristics.
 * @param expected_outcome The expected outcome from Cetus for this test case.
 * @param custom_cetus_flags The Cetus flags to use for this test.
 * @return 1 if the test passed or ground truth was generated successfully, 0 otherwise.
 */
int run_test_case(const char* category, const char* input_file_base_name, TestMode mode, TransformationType transform_type, ExpectedOutcome expected_outcome, const char* custom_cetus_flags) {
    (void)transform_type; // Suppress unused parameter warning if transform_type isn't fully used inside

    char input_file_full_path[MAX_PATH_LENGTH];
    char input_file_base_name_i[MAX_PATH_LENGTH];       // e.g., "my_test.i"
    char input_file_base_name_no_ext[MAX_PATH_LENGTH]; // e.g., "my_test" (without .c)

    const char* input_files_root_dir = "input_files";
    const char* intermediate_i_root_dir = "cetus_intermediate_i_files";
    const char* transformed_output_root_dir = "cetus_transformed_output";
    const char* ground_truth_root_dir = "ground_truth";

    char mkdir_cmd[MAX_PATH_LENGTH * 2]; // Buffer for mkdir commands
    int cmd_result; // Stores result of execute_command

    // --- Directory Creation ---
    // Ensure intermediate and output directories exist
    if (snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", intermediate_i_root_dir) >= (int)sizeof(mkdir_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "mkdir command buffer overflow for intermediate directory.", NULL); return 0;
    }
    cmd_result = execute_command(mkdir_cmd);
    if (cmd_result != 0) {
        log_test_outcome(TEST_FAILED_MKDIR, category, input_file_base_name,
                         "Failed to create intermediate .i files directory.", mkdir_cmd);
        return 0;
    }

    if (snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", transformed_output_root_dir) >= (int)sizeof(mkdir_cmd)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "mkdir command buffer overflow for transformed output directory.", NULL); return 0;
    }
    cmd_result = execute_command(mkdir_cmd);
    if (cmd_result != 0) {
        log_test_outcome(TEST_FAILED_MKDIR, category, input_file_base_name,
                         "Failed to create transformed output directory.", mkdir_cmd);
        return 0;
    }

    // --- File Path Derivations ---
    if (snprintf(input_file_full_path, sizeof(input_file_full_path), "%s/%s", input_files_root_dir, input_file_base_name) >= (int)sizeof(input_file_full_path)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Path buffer overflow for input file.", NULL); return 0;
    }

    const char* input_basename = path_basename(input_file_base_name);
    char *dot = strrchr(input_basename, '.');
    if (dot != NULL) {
        size_t base_len = (size_t)(dot - input_basename);
        // Ensure derived names fit within MAX_PATH_LENGTH
        if (base_len + 2 >= MAX_PATH_LENGTH || base_len >= MAX_PATH_LENGTH) { // +2 for ".i" or to ensure base_len fits
            log_test_outcome(TEST_FAILED_NO_EXTENSION, category, input_file_base_name, "Input file base name too long after removing extension.", NULL); return 0;
        }
        snprintf(input_file_base_name_i, sizeof(input_file_base_name_i), "%.*s.i", (int)base_len, input_basename);
        snprintf(input_file_base_name_no_ext, sizeof(input_file_base_name_no_ext), "%.*s", (int)base_len, input_basename);
    } else {
        log_test_outcome(TEST_FAILED_NO_EXTENSION, category, input_file_base_name,
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
    char format_cmd[MAX_PATH_LENGTH * 2]; // For clang-format commands


    // --- 1. Preprocessing Step (using Clang) ---
    if (snprintf(preprocessed_input_file, sizeof(preprocessed_input_file), "%s/%s", intermediate_i_root_dir, input_file_base_name_i) >= (int)sizeof(preprocessed_input_file)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Path buffer overflow for preprocessed input file.", NULL); return 0;
    }
    printf("[%s] DEBUG: Preprocessing output will be in: %s\n", get_current_time(), preprocessed_input_file);
    if (log_all_fp) fprintf(log_all_fp, "[%s] DEBUG: Preprocessing output will be in: %s\n", get_current_time(), preprocessed_input_file);

    // Command: clang -E -P -x c -std=c11 "input_files/my_test.c" -o "cetus_intermediate_i_files/my_test.i"
    if (snprintf(preprocessor_command, sizeof(preprocessor_command), "%s -E -P -x c -std=c11 \"%s\" -o \"%s\"", CLANG_PATH, input_file_full_path, preprocessed_input_file) >= (int)sizeof(preprocessor_command)) {
         log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Preprocessor command buffer overflow.", NULL); return 0;
    }

    printf("[%s] Manually preprocessing %s to %s...\n", get_current_time(), input_file_full_path, preprocessed_input_file);
    if (log_all_fp) fprintf(log_all_fp, "[%s] Manually preprocessing %s to %s...\n", get_current_time(), input_file_full_path, preprocessed_input_file);
    cmd_result = execute_command(preprocessor_command);
    if (cmd_result != 0) {
        TestOutcome outcome_type = TEST_FAILED_PREPROCESS; char reason_buf[MAX_PATH_LENGTH];
        if (cmd_result == -999) { snprintf(reason_buf, sizeof(reason_buf), "Preprocessor CRASHED."); outcome_type = TEST_FAILED_CRASH; }
        else { snprintf(reason_buf, sizeof(reason_buf), "Preprocessing failed (exit code %d).", cmd_result); }
        log_test_outcome(outcome_type, category, input_file_base_name, reason_buf, preprocessor_command); return 0;
    }
    printf("[%s] Preprocessing successful. Output file: %s (size %ld bytes).\n", get_current_time(), preprocessed_input_file, file_size(preprocessed_input_file));
    if (log_all_fp) fprintf(log_all_fp, "[%s] Preprocessing successful. Output file: %s (size %ld bytes).\n", get_current_time(), preprocessed_input_file, file_size(preprocessed_input_file));


    // --- 2. Semantic Check (using check_syntax.sh) ---
    // Command: ./check_syntax.sh "cetus_intermediate_i_files/my_test.i"
    if (snprintf(semantic_check_command, sizeof(semantic_check_command), "./check_syntax.sh \"%s\"", preprocessed_input_file) >= (int)sizeof(semantic_check_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Semantic check command buffer overflow.", NULL); return 0;
    }
    printf("[%s] Running semantic check command: %s\n", get_current_time(), semantic_check_command);
    if (log_all_fp) fprintf(log_all_fp, "[%s] Running semantic check command: %s\n", get_current_time(), semantic_check_command);
    cmd_result = execute_command(semantic_check_command);
    if (cmd_result != 0) {
        TestOutcome outcome_type = TEST_FAILED_SEMANTIC_CHECK; char reason_buf[MAX_PATH_LENGTH];
        if (cmd_result == -999) { snprintf(reason_buf, sizeof(reason_buf), "Semantic check script CRASHED."); outcome_type = TEST_FAILED_CRASH; }
        else { snprintf(reason_buf, sizeof(reason_buf), "Semantic check failed (exit code %d).", cmd_result); }
        log_test_outcome(outcome_type, category, input_file_base_name, reason_buf, semantic_check_command); return 0;
    }
    printf("[%s] Semantic check successful.\n", get_current_time());
    if (log_all_fp) fprintf(log_all_fp, "[%s] Semantic check successful.\n", get_current_time());


    printf("[%s] Applying Cetus transformation...\n", get_current_time());
    if (log_all_fp) fprintf(log_all_fp, "[%s] Applying Cetus transformation...\n", get_current_time());

    // --- 3. Cetus Execution Step ---
    // Command: cetus -outdir="cetus_transformed_output" <cetus_flags> "cetus_intermediate_i_files/my_test.i"
    if (snprintf(cetus_command, sizeof(cetus_command), "%s -outdir=\"%s\" %s \"%s\"",
             CETUS_PATH, transformed_output_root_dir, custom_cetus_flags, preprocessed_input_file) >= (int)sizeof(cetus_command)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Cetus command buffer overflow.", NULL); return 0;
    }

    printf("[%s] DEBUG: Cetus command: %s\n", get_current_time(), cetus_command);
    if (log_all_fp) fprintf(log_all_fp, "[%s] DEBUG: Cetus command: %s\n", get_current_time(), cetus_command);

    cmd_result = execute_command(cetus_command);

    // Handle Cetus execution outcomes based on expected_outcome
    if (expected_outcome == EXPECT_FAILURE) {
        if (cmd_result == 0) {
            // Cetus succeeded, but we expected it to fail
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name,
                             "Cetus executed successfully (exit code 0), but a failure was expected.", cetus_command);
            return 0;
        } else {
            // Cetus failed as expected
            printf("[%s] Cetus failed as expected. Exit code: %d.\n", get_current_time(), cmd_result);
            if (log_all_fp) fprintf(log_all_fp, "[%s] Cetus failed as expected. Exit code: %d.\n", get_current_time(), cmd_result);
            log_test_outcome(TEST_PASSED, category, input_file_base_name, "Cetus failed as expected.", cetus_command);
            return 1; // Test considered passed because expected failure occurred
        }
    } else { // Expected outcome is SUCCESS_TRANSFORMED or SUCCESS_NO_CHANGE
        if (cmd_result != 0) {
            // Cetus failed when it was expected to succeed
            TestOutcome outcome_type = TEST_FAILED_UNKNOWN; char reason_buf[MAX_PATH_LENGTH];
            if (cmd_result == -999) { snprintf(reason_buf, sizeof(reason_buf), "Cetus CRASHED (terminated abnormally)."); outcome_type = TEST_FAILED_CRASH; }
            else { snprintf(reason_buf, sizeof(reason_buf), "Cetus execution failed (returned non-zero exit code %d).", cmd_result); }
            log_test_outcome(outcome_type, category, input_file_base_name, reason_buf, cetus_command); return 0;
        }
        printf("[%s] Cetus execution successful.\n", get_current_time());
        if (log_all_fp) fprintf(log_all_fp, "[%s] Cetus execution successful.\n", get_current_time());
    }

    // --- DETERMINE ACTUAL CETUS OUTPUT FILE PATH ---
    // Cetus typically outputs the transformed .i file into the specified -outdir
    if (snprintf(cetus_actual_output_file, sizeof(cetus_actual_output_file), "%s/%s", transformed_output_root_dir, input_file_base_name_i) >= (int)sizeof(cetus_actual_output_file)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Path buffer overflow for Cetus actual output file.", NULL); return 0;
    }
    printf("[%s] DEBUG: Cetus output is expected at: %s\n", get_current_time(), cetus_actual_output_file);
    if (log_all_fp) fprintf(log_all_fp, "[%s] DEBUG: Cetus output is expected at: %s\n", get_current_time(), cetus_actual_output_file);


    // --- Determine Ground Truth File Path (flat, using GROUND_TRUTH_SUFFIX) ---
    // Example: input_files/my_test.c -> ground_truth/my_test_gt.c
    if (snprintf(ground_truth_file_path, sizeof(ground_truth_file_path), "%s/%s%s", ground_truth_root_dir, input_file_base_name_no_ext, GROUND_TRUTH_SUFFIX) >= (int)sizeof(ground_truth_file_path)) {
        log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Path buffer overflow for ground truth file.", NULL); return 0;
    }


    // --- 4. Ground Truth Generation or Comparison ---
    if (mode == GENERATE_MODE) {
        printf("[%s] Generating ground truth for %s...\n", get_current_time(), input_file_base_name);
        if (log_all_fp) fprintf(log_all_fp, "[%s] Generating ground truth for %s...\n", get_current_time(), input_file_base_name);

        // Ensure ground_truth directory exists
        char ground_truth_dir_create_cmd[MAX_PATH_LENGTH * 2];
        if (snprintf(ground_truth_dir_create_cmd, sizeof(ground_truth_dir_create_cmd), "mkdir -p \"%s\"", ground_truth_root_dir) >= (int)sizeof(ground_truth_dir_create_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "mkdir command buffer overflow for ground truth directory.", NULL); return 0;
        }
        cmd_result = execute_command(ground_truth_dir_create_cmd);
        if (cmd_result != 0) {
            log_test_outcome(TEST_FAILED_MKDIR, category, input_file_base_name,
                             "Failed to create ground truth directory.", ground_truth_dir_create_cmd);
            return 0;
        }

        // Check if Cetus output exists before copying
        if (access(cetus_actual_output_file, F_OK) != 0) {
            char reason_buf[MAX_PATH_LENGTH * 2];
            snprintf(reason_buf, sizeof(reason_buf), "Cetus output file '%s' not found for ground truth generation. (Error: %s)", cetus_actual_output_file, strerror(errno));
            log_test_outcome(TEST_FAILED_MISSING_OUTPUT, category, input_file_base_name, reason_buf, NULL);
            return 0;
        }

        // Copy Cetus output to ground_truth_file_path
        char cp_cmd[MAX_PATH_LENGTH * 2];
        if (snprintf(cp_cmd, sizeof(cp_cmd), "cp \"%s\" \"%s\"", cetus_actual_output_file, ground_truth_file_path) >= (int)sizeof(cp_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Copy command buffer overflow.", NULL); return 0;
        }
        cmd_result = execute_command(cp_cmd);
        if (cmd_result != 0) {
            char reason_buf[MAX_PATH_LENGTH * 2];
            snprintf(reason_buf, sizeof(reason_buf), "Failed to generate ground truth (copy command failed: %s).", strerror(errno));
            log_test_outcome(TEST_FAILED_COPY_GROUND_TRUTH, category, input_file_base_name, reason_buf, cp_cmd);
            return 0;
        }
        printf("[%s] Ground truth generated successfully for %s at %s.\n", get_current_time(), input_file_base_name, ground_truth_file_path);
        if (log_all_fp) fprintf(log_all_fp, "[%s] Ground truth generated successfully for %s at %s.\n", get_current_time(), input_file_base_name, ground_truth_file_path);
        log_test_outcome(TEST_PASSED, category, input_file_base_name, "Ground truth generated successfully.", NULL);
        return 1; // Test considered passed as ground truth was generated
    } else { // COMPARE_MODE
        printf("[%s] Comparing Cetus output with ground truth for %s...\n", get_current_time(), input_file_base_name);
        if (log_all_fp) fprintf(log_all_fp, "[%s] Comparing Cetus output with ground truth for %s...\n", get_current_time(), input_file_base_name);

        char reason_buf[MAX_PATH_LENGTH * 2];

        // Check if Cetus output and ground truth files exist
        if (access(cetus_actual_output_file, F_OK) != 0) {
            snprintf(reason_buf, sizeof(reason_buf), "Cetus did not produce its expected output file: '%s'. (Error: %s)", cetus_actual_output_file, strerror(errno));
            log_test_outcome(TEST_FAILED_MISSING_OUTPUT, category, input_file_base_name, reason_buf, NULL);
            return 0;
        }

        if (access(ground_truth_file_path, F_OK) != 0) {
            snprintf(reason_buf, sizeof(reason_buf), "Ground truth file not found: '%s'. Please ensure it exists with the correct naming convention ('_gt.c' suffix expected). (Error: %s)", ground_truth_file_path, strerror(errno));
            log_test_outcome(TEST_FAILED_MISSING_GROUND_TRUTH, category, input_file_base_name, reason_buf, NULL);
            return 0;
        }

        // --- NEW: Format Cetus output and Ground Truth before comparison ---
        // This is crucial for reliable diffs, as Cetus and Clang may format differently.
        printf("[%s] DEBUG: Formatting Cetus output file: %s with clang-format...\n", get_current_time(), cetus_actual_output_file);
        if (snprintf(format_cmd, sizeof(format_cmd), "%s -i -style=Google \"%s\"", CLANG_FORMAT_PATH, cetus_actual_output_file) >= (int)sizeof(format_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "clang-format command buffer overflow for Cetus output.", NULL); return 0;
        }
        cmd_result = execute_command(format_cmd);
        if (cmd_result != 0) {
             snprintf(reason_buf, sizeof(reason_buf), "Failed to format Cetus output with clang-format (exit code %d).", cmd_result);
             log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, reason_buf, format_cmd);
             return 0;
        }

        printf("[%s] DEBUG: Formatting Ground Truth file: %s with clang-format...\n", get_current_time(), ground_truth_file_path);
        if (snprintf(format_cmd, sizeof(format_cmd), "%s -i -style=Google \"%s\"", CLANG_FORMAT_PATH, ground_truth_file_path) >= (int)sizeof(format_cmd)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "clang-format command buffer overflow for ground truth.", NULL); return 0;
        }
        cmd_result = execute_command(format_cmd);
        if (cmd_result != 0) {
             snprintf(reason_buf, sizeof(reason_buf), "Failed to format Ground Truth with clang-format (exit code %d).", cmd_result);
             log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, reason_buf, format_cmd);
             return 0;
        }
        // --- END NEW FORMATTING ---

        long input_file_initial_size = file_size(input_file_full_path);
        long cetus_output_file_size = file_size(cetus_actual_output_file);
        long ground_truth_file_size = file_size(ground_truth_file_path);

        if (input_file_initial_size == -1 || cetus_output_file_size == -1 || ground_truth_file_size == -1) {
            snprintf(reason_buf, sizeof(reason_buf), "Failed to get file sizes for comparison. Check file existence or permissions. (Error: %s)", strerror(errno));
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, reason_buf, NULL);
            return 0;
        }

        // --- Perform Diff Comparison ---
        // `diff -wB`: -w (ignore all whitespace), -B (ignore blank lines) for robust comparison
        if (snprintf(diff_command, sizeof(diff_command), "diff -wB \"%s\" \"%s\"", cetus_actual_output_file, ground_truth_file_path) >= (int)sizeof(diff_command)) {
            log_test_outcome(TEST_FAILED_UNKNOWN, category, input_file_base_name, "Diff command buffer overflow.", NULL); return 0;
        }
        printf("[%s] DEBUG: Diff command: %s\n", get_current_time(), diff_command);
        if (log_all_fp) fprintf(log_all_fp, "[%s] DEBUG: Diff command: %s\n", get_current_time(), diff_command);

        int diff_result = execute_command(diff_command);

        if (diff_result != 0) { // Files are different
            TestOutcome outcome_to_log = TEST_FAILED_DIFF;
            // Heuristic for missed opportunities vs. incorrect transformations
            // These heuristics might need tuning based on specific Cetus behavior and test cases.
            if (expected_outcome == EXPECT_SUCCESS_TRANSFORMED) {
                // If diff fails and original size is similar to output, might be missed opportunity.
                // This heuristic is less reliable after clang-format, but included for logic.
                if (cetus_output_file_size <= (input_file_initial_size * 1.1)) { // Small change relative to original
                     outcome_to_log = TEST_FAILED_MISSED_OPPORTUNITY;
                     snprintf(reason_buf, sizeof(reason_buf), "Cetus output is different from ground truth after formatting, and possibly untransformed. Expected transformation.");
                } else {
                    outcome_to_log = TEST_FAILED_INCORRECT_TRANSFORMATION;
                    snprintf(reason_buf, sizeof(reason_buf), "Cetus output is different from ground truth after formatting. Appears to be an incorrect transformation.");
                }
            } else if (expected_outcome == EXPECT_SUCCESS_NO_CHANGE) {
                outcome_to_log = TEST_FAILED_INCORRECT_TRANSFORMATION;
                snprintf(reason_buf, sizeof(reason_buf), "Cetus output is different from ground truth after formatting. No transformation was expected, but output was changed.");
            } else { // Generic diff mismatch
                 snprintf(reason_buf, sizeof(reason_buf), "Output does NOT match ground truth (Diff found after formatting).");
            }

            log_test_outcome(outcome_to_log, category, input_file_base_name, reason_buf, diff_command);
            return 0; // Test failed
        } else { // Files are identical
            // Additional check for missed opportunities even if diff passes
            if (expected_outcome == EXPECT_SUCCESS_TRANSFORMED) {
                // If files are identical, but transformation was expected and original size is small,
                // it might mean no transformation happened. This heuristic is tricky with formatting.
                // A better check would be semantic analysis of the transformed code if possible.
                // For this example, if diff is 0 and transform was expected, we assume it passed.
            }

            printf("[%s] ✅ SUCCESS: %s passed. Output matches ground truth after formatting.\n", get_current_time(), input_file_base_name);
            if (log_all_fp) fprintf(log_all_fp, "[%s] SUCCESS: %s passed. Output matches ground truth after formatting.\n", get_current_time(), input_file_base_name);
            log_test_outcome(TEST_PASSED, category, input_file_base_name, "Output matches ground truth after formatting.", NULL);
            return 1; // Test passed
        }
    }
}


// --- Main Function ---
int main(int argc, char *argv[]) {
    // 1. Initialize overall_exit_status FIRST to avoid uninitialized use warnings
    int overall_exit_status = 0; // Initialize here, before any potential 'goto' jumps

    // Initialize log files and create logs directory
    if (system("mkdir -p logs") != 0) { // Use system for directory creation, check return
        fprintf(stderr, "[%s] ERROR: Failed to create 'logs/' directory. Exiting.\n", get_current_time());
        overall_exit_status = 1; // Set status on error
        goto cleanup_logs; // Jump to cleanup before exiting
    }

    log_all_fp = fopen("logs/all_tests.log", "w");
    log_passed_fp = fopen("logs/passed_tests.log", "w");
    log_failed_fp = fopen("logs/failed_tests.log", "w");
    log_crashes_fp = fopen("logs/crashes_test.log", "w");
    log_missed_opportunities_fp = fopen("logs/missed_opportunities.log", "w");
    log_incorrect_transformation_fp = fopen("logs/incorrect_transformation.log", "w");

    // Check if all log files were opened successfully
    if (!log_all_fp || !log_passed_fp || !log_failed_fp || !log_crashes_fp || !log_missed_opportunities_fp || !log_incorrect_transformation_fp) {
        fprintf(stderr, "[%s] ERROR: Failed to open one or more log files. Check 'logs/' directory permissions or disk space.\n", get_current_time());
        overall_exit_status = 1; // Set status on error
        goto cleanup_logs; // Jump to cleanup before exiting
    }

    int total_tests = 0;
    int passed_tests = 0;

    TestMode current_mode = COMPARE_MODE; // Default mode for running tests
    char* custom_cetus_options = NULL;
    char* test_identifier = NULL; // To store the identifier for --run-test
    char* suite_identifier = NULL;
    int list_suites_only = 0;

    // 2. Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--generate") == 0) {
            current_mode = GENERATE_MODE;
        } else if (strcmp(argv[i], "-cetus-options") == 0) {
            if (i + 1 < argc) {
                custom_cetus_options = argv[++i];
            } else {
                fprintf(stderr, "[%s] ERROR: -cetus-options requires an argument (a quoted string of flags).\n", get_current_time());
                print_usage(argv[0]);
                overall_exit_status = 1;
                goto cleanup_logs;
            }
        } else if (strcmp(argv[i], "--run-test") == 0) {
            if (i + 1 < argc) {
                test_identifier = argv[++i];
            } else {
                fprintf(stderr, "[%s] ERROR: --run-test requires a test identifier (category or input file name).\n", get_current_time());
                print_usage(argv[0]);
                overall_exit_status = 1;
                goto cleanup_logs;
            }
        } else if (strcmp(argv[i], "--run-suite") == 0) {
            if (i + 1 < argc) {
                suite_identifier = argv[++i];
            } else {
                fprintf(stderr, "[%s] ERROR: --run-suite requires a suite name (e.g. tiling, subsub).\n", get_current_time());
                print_usage(argv[0]);
                overall_exit_status = 1;
                goto cleanup_logs;
            }
        } else if (strcmp(argv[i], "--list-suites") == 0) {
            list_suites_only = 1;
        } else if (strcmp(argv[i], "--all") == 0) {
            // This flag is handled implicitly if test_identifier remains NULL.
        } else {
            fprintf(stderr, "[%s] ERROR: Unrecognized argument: %s\n", get_current_time(), argv[i]);
            print_usage(argv[0]);
            overall_exit_status = 1;
            goto cleanup_logs;
        }
    }

    if (list_suites_only) {
        list_suites();
        goto cleanup_logs;
    }

    if (test_identifier != NULL && suite_identifier != NULL) {
        fprintf(stderr, "[%s] ERROR: Use either --run-test or --run-suite, not both.\n", get_current_time());
        print_usage(argv[0]);
        overall_exit_status = 1;
        goto cleanup_logs;
    }

    // 3. Execute Tests (Specific Test, Suite, or All Tests)
    if (test_identifier != NULL) {
        TestCase* found_test = NULL;
        for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
            const char* basename = path_basename(master_test_cases[i].input_file_base_name);
            if (strcmp(master_test_cases[i].category, test_identifier) == 0 ||
                strcmp(master_test_cases[i].input_file_base_name, test_identifier) == 0 ||
                strcmp(basename, test_identifier) == 0) {
                found_test = &master_test_cases[i];
                break;
            }
        }

        if (found_test == NULL) {
            fprintf(stderr, "[%s] ERROR: Test with identifier '%s' not found in master_test_cases.h.\n", get_current_time(), test_identifier);
            overall_exit_status = 1;
        } else {
            printf("\n--- Running Specific Test: %s/%s ---\n", found_test->category, found_test->input_file_base_name);
            if (run_test_case(found_test->category, found_test->input_file_base_name, current_mode,
                              found_test->transform_type, found_test->expected_outcome,
                              custom_cetus_options ? custom_cetus_options : found_test->cetus_flags)) {
                passed_tests++;
            } else {
                overall_exit_status = 1;
            }
            total_tests++;
        }
    } else {
        int selected[NUM_MASTER_TEST_CASES];
        int selected_count = 0;
        for (int i = 0; i < NUM_MASTER_TEST_CASES; ++i) {
            if (suite_identifier == NULL || test_in_suite(&master_test_cases[i], suite_identifier)) {
                selected[selected_count++] = i;
            }
        }

        if (suite_identifier != NULL && selected_count == 0) {
            fprintf(stderr, "[%s] ERROR: Suite '%s' not found. Use --list-suites.\n", get_current_time(), suite_identifier);
            overall_exit_status = 1;
        } else {
            if (suite_identifier != NULL) {
                printf("[%s] Running suite '%s' (%d test(s))...\n", get_current_time(), suite_identifier, selected_count);
            } else {
                printf("[%s] Running ALL master test cases...\n", get_current_time());
            }
            printf("--------------------------------------------------\n");
            for (int s = 0; s < selected_count; ++s) {
                TestCase* current_test = &master_test_cases[selected[s]];
                printf("\n--- Running Test [%d/%d]: %s/%s ---\n", s + 1, selected_count, current_test->category, current_test->input_file_base_name);
                if (run_test_case(current_test->category, current_test->input_file_base_name, current_mode,
                                  current_test->transform_type, current_test->expected_outcome,
                                  custom_cetus_options ? custom_cetus_options : current_test->cetus_flags)) {
                    passed_tests++;
                } else {
                    overall_exit_status = 1;
                }
                total_tests++;
            }
            printf("\n--------------------------------------------------\n");
        }
    }

    // 4. Final summary and cleanup
    printf("[%s] Test Summary: %d/%d tests passed.\n", get_current_time(), passed_tests, total_tests);
    if (log_all_fp) {
        fprintf(log_all_fp, "[%s] Test Summary: %d/%d tests passed.\n", get_current_time(), passed_tests, total_tests);
    }

cleanup_logs:
    if (log_all_fp) fclose(log_all_fp);
    if (log_passed_fp) fclose(log_passed_fp);
    if (log_failed_fp) fclose(log_failed_fp);
    if (log_crashes_fp) fclose(log_crashes_fp);
    if (log_missed_opportunities_fp) fclose(log_missed_opportunities_fp);
    if (log_incorrect_transformation_fp) fclose(log_incorrect_transformation_fp);
    
    return overall_exit_status; // Return 0 for overall success, 1 for failure
}