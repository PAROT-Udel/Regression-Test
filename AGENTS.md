# Cetus Regression Test Suite — Agent Guide

This project is **independent of the Cetus source tree**. It validates Cetus compiler output by diffing transformed code against checked-in ground truth files, then (for benchmarks) rebuilding and running transformed programs.

**Runtime is entirely native C** — no Python. Benchmark configuration is JSON under `benchmarks/config/` (loaded at runtime, no recompile needed to change suites).

Run from **Linux or WSL**, not native Windows cmd.

## Prerequisites (WSL/Linux)

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk make diffutils
```

- `gcc`, `clang`, `clang-format`, `diff`, `make`
- Java 8+ **and** `javac` (to build Cetus)
- Cetus built from **`parot/release/3.0`** (see below)
- PolyBench/C 4.2 and NPB3.3-SER-C trees (for benchmarks)

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

## Configure paths

**Kernel tests** — [`helper_tests.h`](helper_tests.h):

```c
#define CETUS_PATH "/path/to/The-Cetus-Project/bin/cetus"
#define CLANG_PATH "clang"
#define CLANG_FORMAT_PATH "clang-format"
```

**Benchmark tests** — copy and edit env file:

```bash
cp benchmarks/config/paths.example.env benchmarks/config/paths.env
```

Set `CETUS`, `POLYBENCH_ROOT`, `NPB_SER_ROOT`, etc. Env vars override `paths.env`.

## Build the harness

```bash
cd /path/to/cetus_regression_test_suite
chmod +x check_syntax.sh run_all.sh
make
make test    # optional: unit tests
```

Do **not** use the old single-file `gcc -o cetus_regression_test cetus_regression_test.c …` command — the project is multi-file and requires `make`.

## Running tests

```bash
cd /path/to/cetus_regression_test_suite

# Kernel tests (default / --all)
./cetus_regression_test --all
./cetus_regression_test

# List / run kernel suites
./cetus_regression_test --list-suites
./cetus_regression_test --run-suite tiling
./cetus_regression_test --run-suite subsub

# Single kernel test (category, filename, or path)
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64
./cetus_regression_test --run-test SubSub_Amgmk
./cetus_regression_test --run-test tiling_gemm.c

# Override Cetus flags for one kernel run
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 -cetus-options "-paw_tiling=1 -tileSizes=32"

# Verbose (show Cetus/tool output)
./cetus_regression_test --run-suite tiling --verbose

# Regenerate kernel ground truth
./cetus_regression_test --generate
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 --generate
```

Expected for `--all`: `Test Summary: 15/15 tests passed.`

### Benchmarks

```bash
# List suites
./cetus_regression_test --list-benchmark-suites

# Default: polybench (parallel + paw_tiling) + npb-ser (parallel, CLASS=S)
./cetus_regression_test --benchmarks

# One suite / kernel / profile
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-kernel gemm
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --benchmark-kernel cg
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-kernel gemm --profile paw_tiling
./cetus_regression_test --benchmarks --benchmark-suite npb-omp   # opt-in

# GT only / verify only / regenerate GT
./cetus_regression_test --benchmarks --skip-run
./cetus_regression_test --benchmarks --skip-gt
./cetus_regression_test --benchmarks --generate --benchmark-suite polybench --benchmark-kernel gemm

# Dataset / class overrides
./cetus_regression_test --benchmarks --benchmark-suite polybench --dataset SMALL_DATASET
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --class W

# Kernels + benchmarks
./run_all.sh    # make && --full
./cetus_regression_test --full
```

### Windows / WSL

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && make && ./cetus_regression_test --all"
```

## Project layout

```
cetus_regression_test_suite/
├── cetus_regression_test.c    # Main entry + kernel runner
├── cli.c / cli.h              # Unified CLI
├── helper_tests.h             # Kernel paths, enums, TestCase struct
├── master_test_cases.h        # 15 kernel test definitions
├── benchmark_config.c/.h      # JSON + paths.env loader
├── benchmark_runner.c/.h      # Benchmark lifecycle
├── benchmark_adapters.c/.h    # Adapter factory
├── polybench_adapter.c
├── npb_adapter.c
├── generic_adapter.c
├── runner_common.c/.h
├── process_runner.c/.h
├── Makefile
├── check_syntax.sh
├── run_all.sh
├── tests/                     # Native unit tests
├── third_party/cjson/         # Vendored JSON parser
├── benchmarks/config/
│   ├── paths.example.env
│   ├── profiles.json
│   └── suites/                # polybench.json, npb-ser.json, …
├── input_files/
│   ├── tiling/                # PAW tiling (6 tests)
│   └── subsub/                # Subscripted-subscript (9 tests)
│       └── include/header.h
├── ground_truth/
│   ├── tiling/
│   ├── subsub/
│   └── benchmarks/
├── work/benchmarks/           # Sandboxes (gitignored)
├── cetus_intermediate_i_files/
├── cetus_transformed_output/
└── logs/
```

## Architecture (for agents)

| Layer | Files | Role |
|-------|-------|------|
| CLI | `cli.c`, `cetus_regression_test.c` | Parse args, dispatch kernel vs benchmark vs `--full` |
| Kernel runner | `cetus_regression_test.c`, `master_test_cases.h` | Preprocess → Cetus → format → diff |
| Config | `benchmark_config.c` | Load `paths.env`, `profiles.json`, `suites/*.json`; validate |
| Lifecycle | `benchmark_runner.c` | Sandbox reset, GT check, transform, diff/generate, inject, build, verify |
| Adapters | `*_adapter.c` | Suite-specific prepare/cetus/inject/build/verify vtable hooks |
| Process | `process_runner.c` | `fork`/`execvp`, capture stdout/stderr, timeout, process-group kill |

Benchmark JSON is loaded **only** when `--benchmarks`, `--full`, or `--list-benchmark-suites` is used.

## Test suites

### PAW tiling (`TRANSFORM_TILING`)

Six kernels. Base flags (all tiling tests):

```
-alias=3 -verbosity=0 -profitable-omp=0 -tile-profitability=1
-cores=4 -cacheSize=25600 -cacheLine=64 -selection=NT -tilingLevel=0
-ddt=2 -privatize=2 -reduction=2 -parallelize-loops=1 -ompGen=1 -paw_tiling=1
```

`tiling_gemm.c` also uses `-tileSizes=64`.

### Subscripted-subscript analysis (`TRANSFORM_SUBSUB_ANALYSIS`)

Nine cases. Flags:

```
-subsub_analysis -normalize-loops -ompGen=0 -profitable-omp=0
```

(`-ompGen=0 -profitable-omp=0` avoids a `ProfitableOMP` crash on some AMG-style kernels.)

### Benchmark profiles

From `benchmarks/config/profiles.json`:

- **`parallel`** — standard OpenMP parallelization (NPB default)
- **`paw_tiling`** — parallel-aware tiling (PolyBench default alongside parallel)

## XFAIL semantics

- **`xfail` in JSON** — documents expected Cetus/transform/verify failures. Counted as **XFAIL** (does not fail the run).
- **Missing ground truth** — **FAIL** when no xfail is configured. When an xfail is set, the runner skips the GT pre-check, runs Cetus, and lets the xfail catch any Cetus-stage failure. Generate GT with `--generate`.
- **XPASS** — xfail case passed unexpectedly; fails the run (investigate fix).

Known xfails (release/3.0): PolyBench `atax`/`bicg` (SIGSEGV); NPB `bt`/`sp`/`lu` (bad numerics); NPB `ft` (InternalError); NPB `ua` (parse error).

## Adding a new kernel test

1. Place input under `input_files/tiling/` or `input_files/subsub/`.
2. Add entry to `master_test_cases.h` (suite, category, flags, expected outcome).
3. `make`
4. `./cetus_regression_test --run-test <category> --generate`
5. Commit input + `ground_truth/*_gt.c`.
6. `./cetus_regression_test --run-test <category>`

## Adding a new benchmark case

1. Edit `benchmarks/config/suites/<suite>.json` — no recompile.
2. `./cetus_regression_test --benchmarks --run-test …` is **not** valid; use `--benchmark-suite` + `--benchmark-kernel`.
3. Generate GT: `--generate --skip-run` then full run without `--generate`.
4. Commit JSON + `ground_truth/benchmarks/`.

Generic adapter: copy `benchmarks/config/suites/generic-example.json`. Commands must be **argv arrays** (shell strings rejected). Verify types: `exit_code`, `regex`, `dump_diff`.

## Debugging failures

Kernel logs: `logs/failed_tests.log`, `logs/all_tests.log`.

Benchmark logs: `logs/benchmarks/run_*.log` (path printed per case). Logs include per-process details via `benchmark_log_process()`: full argv, exit code, signal/timeout status, and captured stdout/stderr. Non-PASS results always show the log path, even without `--verbose`.

Manual kernel pipeline:

```bash
clang -E -P -x c -std=c11 input_files/tiling/tiling_gemm.c -o /tmp/gemm.i
./check_syntax.sh /tmp/gemm.i
/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus -outdir=cetus_transformed_output/tiling <flags> /tmp/gemm.i
diff -wB cetus_transformed_output/tiling/tiling_gemm.i ground_truth/tiling/tiling_gemm_gt.c
```

## Unit tests (agents modifying C code)

```bash
make test
```

Runs: `tests/test_runner`, `test_benchmark_config`, `test_benchmark_runner`, `test_benchmark_adapters`, `test_cli`.

Use TDD when changing `benchmark_runner.c`, adapters, or CLI validation.

## Do not

- Reintroduce Python benchmark runtime
- Use single-file `gcc cetus_regression_test.c` build
- Mask missing GT with xfail
- Modify external PolyBench/NPB trees (adapters use sandboxes under `work/`)
- Use shell strings in generic adapter JSON commands
