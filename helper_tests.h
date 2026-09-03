#ifndef HELPER_TESTS_H
#define HELPER_TESTS_H

#include <stdio.h> // Required for FILE*

// --- Constants ---
#define MAX_PATH_LENGTH 1024 // Maximum length for file paths
#define GROUND_TRUTH_SUFFIX "_gt.c" // Suffix for ground truth files

// Paths to external tools
// IMPORTANT: Update these paths to match your system's installation
#define CETUS_PATH       "/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus"
#define CLANG_PATH       "clang"          // Or "/usr/bin/clang"
#define CLANG_FORMAT_PATH "clang-format" // Or "/usr/bin/clang-format"


// --- Enums for Test Configuration ---

// Defines the mode of operation for the test runner
typedef enum {
    COMPARE_MODE, // Default: run test and compare output to ground truth
    GENERATE_MODE // Generate ground truth from test output
} TestMode;

// Defines the type of transformation expected for a test case
typedef enum {
    TRANSFORM_NONE,            // No transformation expected
    TRANSFORM_PARALLELIZATION, // Expects parallelization
    TRANSFORM_PRIVATIZATION,   // Expects privatization
    TRANSFORM_REDUCTION,       // Expects reduction
    TRANSFORM_TILING,          // Expects loop tiling
    TRANSFORM_SUBSUB_ANALYSIS, // Expects subscripted-subscript analysis
    TRANSFORM_UNKNOWN          // For cases where a specific type isn't relevant or known
} TransformationType;

// Defines the expected outcome of running Cetus on an input file
typedef enum {
    EXPECT_SUCCESS_TRANSFORMED, // Cetus should successfully transform the code
    EXPECT_SUCCESS_NO_CHANGE,   // Cetus should run successfully but make no changes
    EXPECT_FAILURE,             // Cetus is expected to fail (e.g., due to syntax error)
    EXPECT_UNKNOWN              // Outcome not specifically defined
} ExpectedOutcome;


// --- Enums for Test Results / Outcomes ---

// Defines the actual outcome of a test run
typedef enum {
    TEST_PASSED,
    TEST_FAILED_CRASH,                 // Cetus or a tool crashed
    TEST_FAILED_DIFF,                  // Output differs from ground truth
    TEST_FAILED_PREPROCESS,            // Preprocessing step failed
    TEST_FAILED_SEMANTIC_CHECK,        // Semantic check failed
    TEST_FAILED_MKDIR,                 // Directory creation failed
    TEST_FAILED_COPY_GROUND_TRUTH,     // Copying ground truth failed during generation
    TEST_FAILED_MISSING_OUTPUT,        // Cetus output file not found
    TEST_FAILED_MISSING_GROUND_TRUTH,  // Ground truth file not found
    TEST_FAILED_NO_EXTENSION,          // Input file has no extension
    TEST_FAILED_MISSED_OPPORTUNITY,    // Code was not transformed when it should have been
    TEST_FAILED_INCORRECT_TRANSFORMATION, // Code was transformed incorrectly
    TEST_FAILED_UNKNOWN                // Any other unhandled failure
} TestOutcome;


// --- Struct for Test Case Definition ---

// Structure to define a single test case
typedef struct {
    const char* suite;              // Suite name for --run-suite (e.g., "tiling", "subsub")
    const char* category;           // Unique test id (e.g., "Tiling_PAW_GEMM_Fixed64")
    const char* input_file_base_name; // Path under input_files/ (e.g., "tiling/tiling_gemm.c")
    TransformationType transform_type; // Expected transformation type
    ExpectedOutcome expected_outcome; // What is Cetus expected to do?
    const char* cetus_flags;        // Specific Cetus flags for this test
} TestCase;


// --- Global Log File Pointers (Extern Declarations) ---
// These are defined in cetus_regression_test.c's main function
extern FILE *log_all_fp;
extern FILE *log_passed_fp;
extern FILE *log_failed_fp;
extern FILE *log_crashes_fp;
extern FILE *log_missed_opportunities_fp;
extern FILE *log_incorrect_transformation_fp;


// --- Helper Function Prototypes (Extern Declarations) ---
// These functions are implemented in cetus_regression_test.c

extern char* get_current_time();
extern long file_size(const char *filename);
extern int execute_command(const char *command);
extern const char* transformation_type_to_string(TransformationType type);
extern TransformationType string_to_transformation_type(const char* str);
extern const char* expected_outcome_to_string(ExpectedOutcome outcome);
extern ExpectedOutcome string_to_expected_outcome(const char* str);
extern void log_test_outcome(TestOutcome outcome, const char* category, const char* input_file_base_name,
                             const char* reason_message, const char* command_executed);
extern int run_test_case(const char* category, const char* input_file_base_name, TestMode mode,
                         TransformationType transform_type, ExpectedOutcome expected_outcome, const char* custom_cetus_flags);

#endif // HELPER_TESTS_H