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
- How to build Cetus
- Where to place / point the Cetus executable
- Compile the harness
- Run all tests
- Run a suite (folder)
- Run a specific test
- Generate ground truth
- Windows / WSL
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
The regression runner invokes these tools by path (see `helper_tests.h`). They must be available on **Linux or WSL**:

- **Cetus** (`parot/release/3.0`): built with `./build.sh bin` (needs **Java 8+** and `javac`). See section 3.
- **gcc**: compiles this test harness.
- **clang**: preprocesses input `.c` files to `.i`.
- **clang-format**: formats Cetus output and ground truth before `diff`.
- **diff**: compares formatted files.

On Ubuntu/WSL:

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk
```


# Directory Structure
.
├── cetus_regression_test.c     # Main test runner source
├── helper_tests.h              # Tool paths, enums, TestCase struct
├── master_test_cases.h         # Centralized test case definitions
├── check_syntax.sh             # Script for semantic checking
├── input_files/
│   ├── tiling/                 # PAW tiling suite
│   └── subsub/                 # Subscripted-subscript analysis suite
├── ground_truth/
│   ├── tiling/                 # tiling *_gt.c
│   └── subsub/                 # subsub *_gt.c
├── cetus_intermediate_i_files/<suite>/  # Created by the runner (preprocessed .i)
├── cetus_transformed_output/<suite>/    # Created by the runner (Cetus output)
└── logs/                       # Test logs


# Initial Compilation

```bash
cd /path/to/cetus_regression_test_suite
chmod +x check_syntax.sh
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

- `gcc`: The C compiler.
- `-o cetus_regression_test`: Output executable name.
- `cetus_regression_test.c`: Main source file.
- `-I.`: Find `helper_tests.h` and `master_test_cases.h` in the current directory.
- `-Wall`: Enable common warnings.

If you modify `cetus_regression_test.c`, `helper_tests.h`, or `master_test_cases.h`, recompile with the same `gcc` command.

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

## 3. How to Run Regression Tests

The runner lives in this project (`cetus_regression_test_suite`). It is **independent of the Cetus source tree**: you build Cetus separately, then tell the runner where that executable is.

Run the harness from **Linux or WSL** (POSIX `mkdir -p`, `diff`, and `check_syntax.sh`).

### How to build Cetus

Ground truth in this repo was generated with Cetus from the **`parot` remote, branch `release/3.0`**.

**1. Get the source** (if you do not already have it):

```bash
git clone https://github.com/PAROT-Udel/The-Cetus-Project.git
cd The-Cetus-Project
```

If the clone already exists and has the `parot` remote:

```bash
cd /path/to/The-Cetus-Project
git fetch parot
git checkout release/3.0
```

If `parot` is missing:

```bash
git remote add parot https://github.com/PAROT-Udel/The-Cetus-Project.git
git fetch parot
git checkout -B release/3.0 parot/release/3.0
```

**2. Prerequisites inside the Cetus tree**

- `java` and `javac` (JDK 8 or newer) on `PATH`
- `lib/antlr.jar` (already in the Cetus repo)

**3. Build the compiler wrapper**

```bash
cd /path/to/The-Cetus-Project
./build.sh bin
```

This compiles Java sources, writes `lib/cetus.jar`, and generates a shell wrapper:

```
The-Cetus-Project/
├── bin/cetus          # <-- this is what the regression runner executes
└── lib/cetus.jar      # <-- required; the wrapper hard-codes this path
```

`bin/cetus` is a small `java -cp .../lib/cetus.jar ... cetus.exec.Driver` script. **Do not copy `bin/cetus` by itself** to another folder: the generated script contains **absolute** classpath paths to `lib/cetus.jar`. Leave the wrapper next to that build, or rebuild after moving the tree.

**4. Sanity check**

```bash
/path/to/The-Cetus-Project/bin/cetus -version
```

### Where to place / point the Cetus executable

The regression runner does **not** search `PATH` unless you configure it that way. It runs whatever `CETUS_PATH` is in [`helper_tests.h`](helper_tests.h).

**Recommended:** keep Cetus in `The-Cetus-Project/bin/cetus` and set the **absolute path** (WSL uses `/mnt/d/...` for `D:\`):

```c
#define CETUS_PATH "/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus"
#define CLANG_PATH "clang"
#define CLANG_FORMAT_PATH "clang-format"
```

That is the current default in this repo. Change it if your Cetus clone lives somewhere else.

**Alternative:** if `bin/cetus` is on your `PATH` after you `export PATH="/path/to/The-Cetus-Project/bin:$PATH"`, you can use:

```c
#define CETUS_PATH "cetus"
```

Still keep `lib/cetus.jar` where the wrapper expects it (the path baked into `bin/cetus` at build time).

After changing `CETUS_PATH`, **recompile** the harness (the path is a C `#define`):

```bash
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

### Compile the harness

```bash
cd /path/to/cetus_regression_test_suite
chmod +x check_syntax.sh
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

Recompile whenever you change `cetus_regression_test.c`, `helper_tests.h`, or `master_test_cases.h`.

### Run all tests

```bash
./cetus_regression_test --all
# same as:
./cetus_regression_test
```

Expected: `Test Summary: 15/15 tests passed.`

Each test uses the `cetus_flags` from `master_test_cases.h` unless you pass `-cetus-options`.

### Run a suite (folder)

Inputs are grouped by suite under `input_files/`:

| `--run-suite` | Folder | Tests |
|---------------|--------|-------|
| `tiling` | `input_files/tiling/` | PAW tiling (`-paw_tiling`) |
| `subsub` | `input_files/subsub/` | Subscripted-subscript analysis |

```bash
./cetus_regression_test --list-suites

./cetus_regression_test --run-suite tiling
./cetus_regression_test --run-suite subsub
```

Aliases: `paw` / `paw_tiling` for tiling; `subsub_analysis` for subsub.

### Run a specific test

Identify a case by **category**, **filename**, or **path** from `master_test_cases.h`:

```bash
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64
./cetus_regression_test --run-test SubSub_Amgmk
./cetus_regression_test --run-test tiling_gemm.c
./cetus_regression_test --run-test tiling/tiling_gemm.c
```

### Generate ground truth

Only after an **intentional** Cetus output change. This copies Cetus output into `ground_truth/*_gt.c`.

```bash
./cetus_regression_test --generate
./cetus_regression_test --run-suite tiling --generate
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 --generate
```

Then re-run without `--generate` and commit the updated ground-truth files.

### Windows / WSL

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && ./cetus_regression_test --all"
```

WSL packages: `gcc`, `clang`, `clang-format`, Java 8+ (for Cetus).

### Overriding Cetus options for a single run

Temporarily replace the flags in `master_test_cases.h` (quote the flag string):

```bash
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 -cetus-options "-paw_tiling=1 -tileSizes=32"
```

### Verbose output

By default the runner **suppresses Cetus and tool stdout/stderr** and only prints per-test headers, PASS/FAIL lines, and the final summary. Logs under `logs/` still capture full detail.

```bash
# Show Cetus/tool output and runner DEBUG lines
./cetus_regression_test --run-suite tiling --verbose
./cetus_regression_test --run-suite tiling --verbose true

# Explicitly quiet (default)
./cetus_regression_test --run-suite tiling --verbose false
```


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
