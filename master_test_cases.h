#ifndef MASTER_TEST_CASES_H
#define MASTER_TEST_CASES_H

#include "helper_tests.h" // Includes TestCase struct, TransformationType, ExpectedOutcome enums

// Forward declarations for string conversion functions from cetus_regression_test.c
// These are needed here because the array initialization uses the enum values.
// The string-to-enum conversion for command-line arguments or external file parsing
// happens in cetus_regression_test.c's string_to_X functions.
extern TransformationType string_to_transformation_type(const char* str);
extern ExpectedOutcome string_to_expected_outcome(const char* str);

// Define your test cases here as a static array of TestCase structs.
// For new transformation types (e.g., "loop-fusion" as a string), since they are not
// defined in helper_tests.h's enum, you MUST use TRANSFORM_UNKNOWN here.
// The actual Cetus behavior for these new types is solely defined by the
// 'custom_cetus_flags' provided in this array.
static TestCase master_test_cases[] = {
    // Category, Input File, Transformation Type (enum), Expected Outcome (enum), Custom Cetus Flags (quoted string)

    // Existing Test Cases (as examples)
    { "Privatization_Aggressive", "correctly_privatized.c", TRANSFORM_PRIVATIZATION, EXPECT_SUCCESS_TRANSFORMED, "-privatize=2 -parallelize-loops=1 -ompGen=4" },
    { "Parallelization_Default", "simple_loop.c", TRANSFORM_PARALLELIZATION, EXPECT_SUCCESS_TRANSFORMED, "-parallelize-loops=1" },
    { "NoChange_Example", "untransformable.c", TRANSFORM_NONE, EXPECT_SUCCESS_NO_CHANGE, "" },
    { "Failure_Test", "syntax_error.c", TRANSFORM_UNKNOWN, EXPECT_FAILURE, "-invalid_flag_for_failure" },
    { "Tiling_Default", "matrix_multiply.c", TRANSFORM_TILING, EXPECT_SUCCESS_TRANSFORMED, "-UNKNOWN_TILING_FLAG_FOR_CETUS" },

    // --- How to add a NEW transformation test case (e.g., Loop Fusion) ---
    // 1. Create your input C file (e.g., input_files/fusable_loops.c)
    // 2. Determine the exact Cetus flags that perform the desired transformation.
    // 3. Add a new entry here:
    { "LoopFusion_Basic", "fusable_loops.c", TRANSFORM_UNKNOWN, EXPECT_SUCCESS_TRANSFORMED, "-CETUS_FLAG_FOR_FUSION -ANOTHER_FUSION_FLAG" },
    // Explanation:
    // - "LoopFusion_Basic": Your chosen category string.
    // - "fusable_loops.c": The name of your input file.
    // - TRANSFORM_UNKNOWN: Used because "loop-fusion" (the string name) is not in helper_tests.h's enum.
    //                      The test harness will still run Cetus with your specified flags.
    // - EXPECT_SUCCESS_TRANSFORMED: What you expect from Cetus.
    // - "-CETUS_FLAG_FOR_FUSION -ANOTHER_FUSION_FLAG": REPLACE THIS with the ACTUAL Cetus flags for loop fusion.
    //                                                   (These are placeholders as Cetus's help didn't list direct fusion flags).

    // Another example for a new type: Code Motion
    { "CodeMotion_Hoist", "hoist_var.c", TRANSFORM_UNKNOWN, EXPECT_SUCCESS_TRANSFORMED, "-CETUS_FLAG_FOR_CODE_MOTION -HOIST_OPTIMIZATION" }
};

// Define the number of test cases in the array
static const int NUM_MASTER_TEST_CASES = sizeof(master_test_cases) / sizeof(master_test_cases[0]);

#endif // MASTER_TEST_CASES_H
