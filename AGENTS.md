# Cetus Regression Test Suite — Agent Guide

This project is **independent of the Cetus source tree**. It validates Cetus compiler output by diffing transformed code against checked-in ground truth files.

## Cetus version requirement

Build and test against **`parot/release/3.0`**:

```bash
cd /path/to/The-Cetus-Project
git fetch parot
git checkout release/3.0
./build.sh bin
```

Point the runner at that build in [`helper_tests.h`](helper_tests.h):

```c
#define CETUS_PATH "/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus"
```

(Adjust the path for your machine; WSL paths use `/mnt/d/...`.)

## Project layout

```
cetus_regression_test_suite/
├── cetus_regression_test.c    # Main runner
├── helper_tests.h             # Tool paths, enums, TestCase struct
├── master_test_cases.h        # All test definitions (edit this to add tests)
├── check_syntax.sh            # clang -fsyntax-only on preprocessed .i files
├── input_files/               # Source C inputs, one folder per suite
│   ├── tiling/                # PAW tiling suite (6 tests)
│   └── subsub/                # Subscripted-subscript analysis (9 tests)
│       └── include/header.h   # Required by subsub_test_ua.c
├── ground_truth/              # Expected outputs (*_gt.c)
├── cetus_intermediate_i_files/  # Generated preprocessed .i (gitignored)
├── cetus_transformed_output/    # Generated Cetus output (gitignored)
└── logs/                        # Test logs (gitignored)
```

## Test suites

### PAW tiling (`TRANSFORM_TILING`)

Six kernels copied from `The-Cetus-Project/resources/paw_tests/` and `resources/paw_tiling/3mm.c`. Categories are prefixed with `Tiling_PAW_*`.

Flags (base, all tiling tests):

```
-alias=3 -verbosity=0 -profitable-omp=0 -tile-profitability=1
-cores=4 -cacheSize=25600 -cacheLine=64 -selection=NT -tilingLevel=0
-ddt=2 -privatize=2 -reduction=2 -parallelize-loops=1 -ompGen=1 -paw_tiling=1
```

`tiling_gemm.c` also uses `-tileSizes=64` for deterministic tile sizes.

### Subscripted-subscript analysis (`TRANSFORM_SUBSUB_ANALYSIS`)

Nine cases migrated from `The-Cetus-Project/integration_test/subsub_egs/`. Categories are prefixed with `SubSub_*`.

Flags:

```
-subsub_analysis -normalize-loops -ompGen=0 -profitable-omp=0
```

(`-ompGen=0 -profitable-omp=0` avoids a `ProfitableOMP` crash on some AMG-style kernels when testing via preprocessed `.i` files.)

## Adding a new test

1. Place the input C file in the suite folder (`input_files/tiling/` or `input_files/subsub/`).
2. Add a `TestCase` entry to `master_test_cases.h`:
   - **suite**: `"tiling"` or `"subsub"` (used with `--run-suite`)
   - **category**: unique string (used with `--run-test`)
   - **input file**: path under `input_files/` (e.g. `tiling/tiling_gemm.c`)
   - **transform type**: `TRANSFORM_TILING` or `TRANSFORM_SUBSUB_ANALYSIS`
   - **expected outcome**: usually `EXPECT_SUCCESS_TRANSFORMED`
   - **cetus_flags**: exact flags for Cetus
3. Recompile the runner: `gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall`
4. Generate ground truth: `./cetus_regression_test --run-test <category> --generate`
5. Commit the new `input_files/*.c` and `ground_truth/*_gt.c`.
6. Verify: `./cetus_regression_test --run-test <category>`

## Running tests

```bash
# Compile runner (Linux/WSL)
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall

# All tests
./cetus_regression_test --all

# One suite (folder)
./cetus_regression_test --list-suites
./cetus_regression_test --run-suite tiling
./cetus_regression_test --run-suite subsub

# Single test by category
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64
./cetus_regression_test --run-test SubSub_Amgmk

# Regenerate ground truth after intentional Cetus output change
./cetus_regression_test --generate
./cetus_regression_test --run-suite tiling --generate
```

## Debugging failures

Check `logs/failed_tests.log`, `logs/all_tests.log`, or run the pipeline manually:

```bash
clang -E -P -x c -std=c11 input_files/tiling/tiling_gemm.c -o /tmp/gemm.i
./check_syntax.sh /tmp/gemm.i
/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus -outdir=cetus_transformed_output <flags> /tmp/gemm.i
diff -wB cetus_transformed_output/gemm.i ground_truth/tiling_gemm_gt.c
```

## Prerequisites (WSL/Linux)

- `gcc`, `clang`, `clang-format`
- Java 8+ (for Cetus)
- Cetus built from `parot/release/3.0`
