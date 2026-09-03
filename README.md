## Cetus Regression Test Suite: Comprehensive Guide
This document provides a comprehensive guide to setting up, running, and extending the Cetus regression test suite. The suite is designed to verify the correct behavior of the Cetus source-to-source compiler by comparing its output against predefined ground truth files for various transformations.
    The core principle of this suite is centralized test definition in master_test_cases.h, minimizing the need to modify the main test runner (cetus_regression_test.c) for each new test case.
## Table of Contents
1 - System Requirements & Setup
- Required Tools
- Directory Structure
- Initial Compilation

2 - Core Files Overview
- helper_tests.h
- master_test_cases.h
- cetus_regression_test.c
- check_syntax.sh

3 - Running Regression Tests
- Running All Tests
- Running a Specific Test
- Overriding Cetus Options for a Single Run

4 - Adding New Test Cases
- Step 1: Create Your Input C Source File
- Step 2: Determine Cetus Flags for the Desired Transformation
- Step 3: Add a New Entry to master_test_cases.h
- Step 4: Compile the Test Harness
- Step 5: Run the New Test

5 - Adding a New Transformation Type (Advanced)
- Step 1: Modify helper_tests.h
- Step 2: Modify cetus_regression_test.c
- Step 3: Add Test Cases in master_test_cases.h

6 - Debugging Tests
- Log Files
- Manual Execution

## 1. System Requirements & Setup
# Required Tools
Ensure the following tools are installed and accessible in your system's PATH, or update their paths in helper_tests.h:
- **Cetus Compiler**: The cetus executable. This is the tool whose behavior you are testing.
- **Clang Compile**r: Used for preprocessing C source files.
- **Clang-Format**: Used for consistent code formatting before comparison, which is crucial for reliable diff results.
- **diff utility**: Standard command-line tool for comparing files.

# Directory Structure
Organize your project with the following directory structure:
.
├── cetus_regression_test.c   # Main test runner source
├── helper_tests.h            # Common definitions and prototypes
├── master_test_cases.h       # Centralized test case definitions
├── check_syntax.sh           # Script for semantic checking
├── input_files/              # Directory for original C source files (e.g., simple_loop.c)
├── ground_truth/             # Directory for expected output files (e.g., simple_loop_gt.c)
├── cetus_intermediate_i_files/ # Created by test runner for preprocessed .i files
├── cetus_transformed_output/   # Created by test runner for Cetus's output
└── logs/                       # Created by test runner for test logs


# Initial Compilation
Navigate to your project's root directory in the terminal and compile the test runner:
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall


- gcc: The C compiler.
- -o cetus_regression_test: Specifies the output executable name.
- cetus_regression_test.c: The main source file.
- -I.: Tells the compiler to look for header files (like helper_tests.h and master_test_cases.h) in the current directory.
- -Wall: Enables all common warning messages (recommended for development).

# Note: If you modify cetus_regression_test.c, helper_tests.h, or master_test_cases.h, you must recompile the cetus_regression_test executable.

## 2. Core Files Overview
helper_tests.h

This header file contains fundamental definitions used across the test suite:
- **Constants**: MAX_PATH_LENGTH, GROUND_TRUTH_SUFFIX.
- **Tool Paths**: CETUS_PATH, CLANG_PATH, CLANG_FORMAT_PATH. You MUST update these paths if your tools are not in the default system PATH.
- **Enums**:
    - **TestMode**: COMPARE_MODE, GENERATE_MODE.
    - **TransformationType**: TRANSFORM_NONE, TRANSFORM_PARALLELIZATION, etc., including TRANSFORM_UNKNOWN for new types not explicitly added to the enum.
    - **ExpectedOutcome**: EXPECT_SUCCESS_TRANSFORMED, EXPECT_SUCCESS_NO_CHANGE, EXPECT_FAILURE, EXPECT_UNKNOWN.
    - **TestOutcome**: Detailed outcomes for logging (e.g., TEST_PASSED, TEST_FAILED_DIFF, TEST_FAILED_CRASH).
    -**Struct TestCase**: Defines the structure for each test case, including category, input file, expected transformation, expected outcome, and custom Cetus flags.
    - **Global Log File Pointers**: extern FILE * declarations for various log files.
    - **Function Prototypes**: Declarations for helper functions implemented in cetus_regression_test.c.
- **master_test_cases.h**
This is the centralized configuration file for all test cases. It defines a static array of TestCase structs (master_test_cases[]).
-- Each entry in this array represents a single regression test.
-- It specifies the input_file_base_name, the TransformationType (which can be TRANSFORM_UNKNOWN for new types), the ExpectedOutcome, and the precise cetus_flags that Cetus should be run with for that test.
-- The NUM_MASTER_TEST_CASES constant is automatically calculated.

Developers should primarily interact with this file when adding new tests.

- cetus_regression_test.c

This is the main executable that orchestrates the entire testing process.

- It implements the helper functions declared in helper_tests.h.
- Its main function parses command-line arguments to determine whether to run all tests or a specific test, and whether to compare or generate ground truth.
- It reads test definitions directly from master_test_cases.h.
- It handles preprocessing, semantic checking, Cetus execution, output formatting (via clang-format), and comparison (diff).
- It logs detailed results to various log files in the logs/ directory.

- check_syntax.sh

This is a simple shell script used to perform a basic semantic check on the preprocessed C code before passing it to Cetus. It typically uses clang to compile the .i file without linking, ensuring the C syntax is valid.
Make sure this script is executable: chmod +x check_syntax.sh

## 3. Running Regression Tests
After compiling cetus_regression_test, you can run tests from your project's root directory.
# Running All Tests
To execute all test cases defined in master_test_cases.h:

    ./cetus_regression_test

or explicitly:

    ./cetus_regression_test --all


**Note**: When running all tests, any -cetus-options or --generate flags on the command line will be ignored. Each test uses its own cetus_flags defined in master_test_cases.h.

# Running a Specific Test
To run a single test case, identify it by its category string or its input_file_base_name string as defined in master_test_cases.h:

    ./cetus_regression_test --run-test <test_identifier>

**Examples**:

    ./cetus_regression_test --run-test Privatization_Aggressive
    ./cetus_regression_test --run-test simple_loop.c
    ./cetus_regression_test --run-test LoopFusion_Basic

# Overriding Cetus Options for a Single Run
You can temporarily override the cetus_flags defined in master_test_cases.h for a specific test run using the -cetus-options flag. This is useful for quick debugging or experimenting without modifying the master_test_cases.h file. Remember to enclose the flags in double quotes.

    ./cetus_regression_test --run-test <test_identifier> -cetus-options "YOUR_CUSTOM_CETUS_FLAGS"

**Example**:

    ./cetus_regression_test --run-test Parallelization_Default -cetus-options "-parallelize-loops=3 -ompGen=2"


## 4. Adding New Test Cases
Adding a new test case for an existing or UNKNOWN transformation type is straightforward and primarily involves modifying master_test_cases.h.
# Step 1: Create Your Input C Source File
Create the C source code file that you want Cetus to transform (or not transform). Place this file in the input_files/ directory.
**Example**: input_files/my_new_optimization_test.c

    // input_files/my_new_optimization_test.c

    void compute(int *data, int n) {
    for (int i = 0; i < n; ++i) {
        data[i] = i * 2 + 1;
    }
    // Assume Cetus should optimize this loop in some way
    }

    int main() {
    int arr[10];
    compute(arr, 10);
    for (int i = 0; i < 10; ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    return 0;
    }


# Step 2: Determine Cetus Flags for the Desired Transformation
Identify the precise Cetus command-line flags that will perform the transformation you are testing. Consult Cetus's documentation or its --help output.
**Example**: If you're testing a new loop unrolling feature, the flags might be "-loop-unroll -unroll-factor=4".
# Step 3: Add a New Entry to master_test_cases.h
Open the master_test_cases.h file and add a new TestCase entry to the master_test_cases array.

    // In master_test_cases.h

    static TestCase master_test_cases[] = {
    // ... existing test cases ...

    // New Test Case for Loop Unrolling
    { "LoopUnroll_Factor4", "my_new_optimization_test.c", TRANSFORM_UNKNOWN, EXPECT_SUCCESS_TRANSFORMED, "-loop-unroll -unroll-factor=4" },
    // Explanation:
    // - "LoopUnroll_Factor4": A unique category string for this test.
    // - "my_new_optimization_test.c": The name of your input file.
    // - TRANSFORM_UNKNOWN: Used because "loop-unroll" is not in helper_tests.h's enum.
    //                      The test harness still uses the custom_cetus_flags.
    // - EXPECT_SUCCESS_TRANSFORMED: What you expect from Cetus (e.g., it should transform the code).
    // - "-loop-unroll -unroll-factor=4": The actual Cetus flags for this test.};


# Step 4: Compile the Test Harness
After modifying master_test_cases.h, you must recompile the cetus_regression_test executable for the changes to take effect:

    gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall

# Step 6: Run the New Test
Now, run your new test to ensure it passes:

    ./cetus_regression_test --run-test LoopUnroll_Factor4


You can also run all tests to include your new one:

    ./cetus_regression_test --all


## 5. Adding a New Transformation Type (Advanced)
This section describes how to add a completely new named transformation type (e.g., "Loop Fusion") to the test suite's internal understanding, beyond just using TRANSFORM_UNKNOWN. This is less common than just adding new test cases, but necessary if you want to categorize tests more granularly within the TransformationType enum.
# Step 1: Modify helper_tests.h
Add a new enumerator for your desired transformation type to the TransformationType enum in helper_tests.h.
// helper_tests.h

    // ... existing includes ...

    // Enum for transformation types
    typedef enum {
    TRANSFORM_NONE,
    TRANSFORM_PARALLELIZATION,
    TRANSFORM_PRIVATIZATION,
    TRANSFORM_REDUCTION,
    TRANSFORM_TILING,
    TRANSFORM_LOOP_FUSION, // <-- ADD THIS NEW ENUMERATOR
    TRANSFORM_UNKNOWN      // Keep UNKNOWN for flexibility
    } TransformationType;

    // ... rest of the file ...

# Step 2: Modify cetus_regression_test.c
Update the transformation_type_to_string and string_to_transformation_type functions in cetus_regression_test.c to handle the new enum value.
# a. Update transformation_type_to_string:
    // cetus_regression_test.c

    // ... other helper function implementations ...

    // String conversion for TransformationType
    const char* transformation_type_to_string(TransformationType type) {
    switch (type) {
        // ... existing cases ...
        case TRANSFORM_TILING: return "tiling";
        case TRANSFORM_LOOP_FUSION: return "loop_fusion"; // <-- ADD THIS CASE
        case TRANSFORM_UNKNOWN:
        default: return "unknown";
    }}


# b. Update string_to_transformation_type:
    // cetus_regression_test.c

    // ... other helper function implementation...

    // String to TransformationType conversion
    TransformationType string_to_transformation_type(const char* str) {
    if (strcmp(str, "parallelization") == 0) return TRANSFORM_PARALLELIZATION;
    // ... existing string comparisons ...
    if (strcmp(str, "tiling") == 0) return TRANSFORM_TILING;
    if (strcmp(str, "loop_fusion") == 0) return TRANSFORM_LOOP_FUSION; // <-- ADD THIS CASE
    return TRANSFORM_UNKNOWN; // Default for any unmapped string}


# Step 3: Add Test Cases in master_test_cases.h
Now you can add new test cases to master_test_cases.h that explicitly use your new TRANSFORM_LOOP_FUSION type:

    // In master_test_cases.h
    static TestCase master_test_cases[] = {
    // ... existing test cases ...

    // New Loop Fusion Test using the defined enum
    { "LoopFusion_Simple", "simple_fusion_test.c", TRANSFORM_LOOP_FUSION, EXPECT_SUCCESS_TRANSFORMED, "-fuse-loops" },};


Remember to recompile cetus_regression_test after these changes.
## 6. Debugging Tests
If a test fails, the logs/ directory is your first stop.
# Log Files
The test runner generates several log files in the logs/ directory:
- **all_tests.log**: Comprehensive log of all test runs, including debug information and command executions.
- **passed_tests.log**: Lists all tests that passed successfully.
- **failed_tests.log**: Details tests that failed due to diff mismatches, preprocessing errors, or other issues.
- **crashes_test.log**: Records tests where Cetus or a supporting tool crashed.
- **missed_opportunities.log**: Lists tests where Cetus was expected to transform the code but didn't (output matched input).
- **incorrect_transformation.log**: Lists tests where Cetus transformed the code, but the output did not match the ground truth.
Examine the relevant log file for the failing test to get details on the specific command executed and the reason for failure.
# Manual Execution
For deep debugging, manually execute the steps performed by the test runner:
1- **Preprocessing**:

    clang -E -P -x c -std=c11 input_files/YOUR_TEST.c -o cetus_intermediate_i_files/YOUR_TEST.i


2- **Semantic Check**:

    ./check_syntax.sh cetus_intermediate_i_files/YOUR_TEST.i


3- **Cetus Execution**:

    cetus -outdir=cetus_transformed_output YOUR_CETUS_FLAGS cetus_intermediate_i_files/YOUR_TEST.i


4 - **Formatting**:

    clang-format -i -style=Google cetus_transformed_output/YOUR_TEST.i
    clang-format -i -style=Google ground_truth/YOUR_TEST_gt.c


5. **Diff Comparison**:

    diff -wB cetus_transformed_output/YOUR_TEST.i ground_truth/YOUR_TEST_gt.c

This step-by-step process allows you to isolate where the discrepancy occurs and debug Cetus's behavior or refine your ground truth file.
