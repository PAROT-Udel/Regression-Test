# Cetus Regression Test Suite — Agent Guide

This project is **independent of the Cetus source tree**. It validates Cetus compiler output by diffing transformed code against checked-in ground truth files.

## Prerequisites (WSL/Linux)

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk
```

- `gcc`, `clang`, `clang-format`, `diff`
- Java 8+ **and** `javac` (to build Cetus)
- Cetus built from **`parot/release/3.0`** (see below)

Run the harness from **Linux or WSL**, not native Windows cmd.

## How to build Cetus (`parot/release/3.0`)

**Clone (if needed):**

```bash
git clone https://github.com/PAROT-Udel/The-Cetus-Project.git
cd The-Cetus-Project
```

**Existing clone with `parot` remote:**

```bash
cd /path/to/The-Cetus-Project
git fetch parot
git checkout release/3.0
```

**Existing clone missing the `parot` remote:**

```bash
git remote add parot https://github.com/PAROT-Udel/The-Cetus-Project.git
git fetch parot
git checkout -B release/3.0 parot/release/3.0
```

**Build the wrapper** (`bin/cetus` + `lib/cetus.jar`). Do not copy `bin/cetus` alone; the script hard-codes the jar path.

```bash
cd /path/to/The-Cetus-Project
./build.sh bin
/path/to/The-Cetus-Project/bin/cetus -version
```

## Where to point `CETUS_PATH`

The runner executes the path in [`helper_tests.h`](helper_tests.h). Default (WSL path for `/path/to/bin/cetus...`):

```c
#define CETUS_PATH "/path/to/bin/cetus"
#define CLANG_PATH "clang"
#define CLANG_FORMAT_PATH "clang-format"
```

Change `CETUS_PATH` to your `The-Cetus-Project/bin/cetus`. After editing, recompile the harness (the path is a C `#define`).

**Alternative** if `bin/` is on `PATH`:

```bash
export PATH="/path/to/The-Cetus-Project/bin:$PATH"
```

```c
#define CETUS_PATH "cetus"
```

## Compile the harness

```bash
cd /path/to/cetus_regression_test_suite
chmod +x check_syntax.sh
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

Recompile after any change to `cetus_regression_test.c`, `helper_tests.h`, or `master_test_cases.h`:

```bash
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

## Running tests

```bash
cd /path/to/cetus_regression_test_suite

# All tests
./cetus_regression_test --all
./cetus_regression_test

# List / run a suite (folder)
./cetus_regression_test --list-suites
./cetus_regression_test --run-suite tiling
./cetus_regression_test --run-suite subsub

# Single test (category, filename, or path)
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64
./cetus_regression_test --run-test SubSub_Amgmk
./cetus_regression_test --run-test tiling_gemm.c
./cetus_regression_test --run-test tiling/tiling_gemm.c

# Override Cetus flags for one run
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 -cetus-options "-paw_tiling=1 -tileSizes=32"

# Quiet by default (no Cetus stdout). Show full Cetus/tool output:
./cetus_regression_test --run-suite tiling --verbose
./cetus_regression_test --run-suite tiling --verbose true

# Regenerate ground truth (intentional Cetus output change only)
./cetus_regression_test --generate
./cetus_regression_test --run-suite tiling --generate
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 --generate
```

Expected for `--all`: `Test Summary: 15/15 tests passed.`

### Windows / WSL

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall && ./cetus_regression_test --all"
```

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
├── ground_truth/              # Expected outputs, mirrored suite folders
│   ├── tiling/                # *_gt.c for tiling suite
│   └── subsub/                # *_gt.c for subsub suite
├── cetus_intermediate_i_files/<suite>/  # Generated preprocessed .i (gitignored)
├── cetus_transformed_output/<suite>/    # Generated Cetus output (gitignored)
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
3. Recompile:

```bash
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

4. Generate ground truth:

```bash
./cetus_regression_test --run-test <category> --generate
```

5. Commit the new `input_files/` source and `ground_truth/*_gt.c`.
6. Verify:

```bash
./cetus_regression_test --run-test <category>
```

## Debugging failures

Check `logs/failed_tests.log`, `logs/all_tests.log`, or run the pipeline manually:

```bash
clang -E -P -x c -std=c11 input_files/tiling/tiling_gemm.c -o /tmp/gemm.i
./check_syntax.sh /tmp/gemm.i
/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus -outdir=cetus_transformed_output/tiling <flags> /tmp/gemm.i
diff -wB cetus_transformed_output/tiling/tiling_gemm.i ground_truth/tiling/tiling_gemm_gt.c
```
