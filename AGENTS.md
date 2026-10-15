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

# Suites, kernels, and profiles combine. Omit an axis to run all of it.
# Repeat --benchmark-suite, --benchmark-kernel, or --profile to select several.

# All default benchmarks, every profile each suite enables
# (polybench: parallel + paw_tiling_nt/lrw/fixed32;
#  npb-ser: those three, plus parallel and reuse_analysis)
./cetus_regression_test --benchmarks

# All kernels, one profile (only suites that enable that profile)
./cetus_regression_test --benchmarks --profile paw_tiling_nt
./cetus_regression_test --benchmarks --profile parallel --profile paw_tiling_lrw

# One suite, all of its kernels and profiles
./cetus_regression_test --benchmarks --benchmark-suite polybench

# One suite, all kernels, one profile
./cetus_regression_test --benchmarks --benchmark-suite polybench --profile parallel

# One kernel, every profile that suite enables
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-kernel gemm
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --benchmark-kernel cg

# One kernel and one profile
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-kernel gemm --profile paw_tiling_fixed32

# Several kernels and one profile
./cetus_regression_test --benchmarks --benchmark-kernel gemm --benchmark-kernel atax --profile parallel

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

### Caviness

`./queue_caviness.sh` copies this project to `/work/parot/mgltorsa/cetus_regression_test_suite`, or updates that copy, and only then submits `cetus_regression.slurm`. If a `cetus-reg` job is already queued or running, it asks before submitting another. The job generates ground truth for `paw_tiling_lrw` and `paw_tiling_fixed32`, then runs `--full`. The script prints the Slurm job id and returns. It does not wait. Log: `logs/slurm-<jobid>.out` on Caviness. `run_profiles.sh` sets the cluster Cetus and vanilla benchmark paths.

The script connects with `ssh` and `rsync`. It does not name a key file. Both tools use the SSH host alias `caviness` (`CAVINESS_SSH_HOST` overrides the alias) and `BatchMode=yes`, so OpenSSH will not ask for a password. Before the script can run, `~/.ssh/config` must define that host name, and `IdentityFile` must point at a private key that Caviness accepts:

```
Host caviness
    HostName caviness.hpc.udel.edu
    User mgltorsa
    IdentityFile ~/.ssh/mgltorsa_caviness
```

`Host` is the alias the script uses. `HostName` is the cluster. `User` is the Caviness login. `IdentityFile` is the key.

### Windows / WSL

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && make && ./cetus_regression_test --all"
```

## Project layout

```
cetus_regression_test_suite/
├── cetus_regression_test.c    # Main entry + kernel runner
├── cli.c / cli.h              # Unified CLI
├── helper_tests.h             # Kernel paths and result enums
├── kernel_config.c/.h         # Kernel suite JSON loader
├── benchmark_config.c/.h      # Benchmark JSON + paths.env loader
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
│   ├── suites/                # polybench.json, npb-ser.json, …
│   └── kernels/               # tiling.json, subsub.json
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
| Kernel runner | `cetus_regression_test.c`, `kernel_config.c` | Load JSON, preprocess → Cetus → format → diff |
| Config | `benchmark_config.c`, `kernel_config.c` | Load `paths.env`, `profiles.json`, `suites/*.json`, `kernels/*.json` |
| Lifecycle | `benchmark_runner.c` | Sandbox reset, GT check, transform, diff/generate, inject, build, verify |
| Adapters | `*_adapter.c` | Suite-specific prepare/cetus/inject/build/verify vtable hooks |
| Process | `process_runner.c` | `fork`/`execvp`, capture stdout/stderr, timeout, process-group kill |

Benchmark JSON is loaded when `--benchmarks`, `--full`, or `--list-benchmark-suites` is used. Kernel JSON under `benchmarks/config/kernels/` is loaded for kernel runs and `--list-suites`. Neither requires a rebuild.

## Test suites

### PAW tiling (`TRANSFORM_TILING`)

Six kernels in `benchmarks/config/kernels/tiling.json`. Shared flags, with `-tileSizes=64` added on `tiling_gemm.c`:

```
-alias=3 -verbosity=0 -profitable-omp=0 -tile-profitability=1
-cores=4 -cacheSize=25600 -cacheLine=64 -selection=NT -tilingLevel=0
-ddt=2 -privatize=2 -reduction=2 -parallelize-loops=1 -ompGen=1 -paw_tiling=1
```

`tiling_gemm.c` also uses `-tileSizes=64`.

### Subscripted-subscript analysis (`TRANSFORM_SUBSUB_ANALYSIS`)

Nine cases in `benchmarks/config/kernels/subsub.json`. Flags:

```
-subsub_analysis -normalize-loops -ompGen=0 -profitable-omp=0
```

(`-ompGen=0 -profitable-omp=0` avoids a `ProfitableOMP` crash on some AMG-style kernels.)

### Benchmark profiles

From `benchmarks/config/profiles.json`. A suite lists which of these it runs. `--profile` selects by exact name and is repeatable; omit it to run every profile that suite enables.

- **`parallel`** — OpenMP parallelization (PolyBench and NPB). Not stock Cetus: `-alias=3` (default 1) and `-profitable-omp=0` (default 1).
- **`paw_tiling_nt`** — parallel-aware tiling, NT tile sizes (PolyBench and NPB serial)
- **`paw_tiling_lrw`** — parallel-aware tiling, LRW tile sizes (PolyBench and NPB serial)
- **`paw_tiling_fixed32`** — parallel-aware tiling, fixed tile size 32 (PolyBench and NPB serial)
- **`reuse_analysis`** — OpenMP parallelization plus `-reuse-vector-analysis=1` (NPB serial)

### Selecting benchmarks and profiles

`--benchmark-suite`, `--benchmark-kernel`, and `--profile` are independent and repeatable. Each listed value is included. An omitted filter means every value on that axis (default suites, every kernel, every profile the suite enables). Kernel names match as substrings. Profile names must match `profiles.json` exactly; an unknown profile is an error. A known profile that the selected suites do not enable matches no cases.

| Goal | Command |
|------|---------|
| All benchmarks, all profiles | `--benchmarks` |
| All benchmarks, one profile | `--benchmarks --profile paw_tiling_nt` |
| All benchmarks, several profiles | `--benchmarks --profile parallel --profile paw_tiling_lrw` |
| One suite, all kernels and profiles | `--benchmarks --benchmark-suite polybench` |
| One suite, one profile | `--benchmarks --benchmark-suite npb-ser --profile reuse_analysis` |
| One kernel, all its profiles | `--benchmarks --benchmark-kernel gemm` |
| One kernel, one profile | `--benchmarks --benchmark-kernel gemm --profile paw_tiling_fixed32` |
| Several kernels, one profile | `--benchmarks --benchmark-kernel gemm --benchmark-kernel 2mm --profile parallel` |

## XFAIL semantics

- **`xfail` in JSON** — documents expected Cetus/transform/verify failures. Counted as **XFAIL** (does not fail the run).
- **Missing ground truth** — **FAIL** when no xfail is configured. When an xfail is set, the runner skips the GT pre-check, runs Cetus, and lets the xfail catch any Cetus-stage failure. Generate GT with `--generate`.
- **XPASS** — xfail case passed unexpectedly; fails the run (investigate fix).

The checked-in PolyBench and NPB suites do not use `xfail`. NPB verify sets `OMP_NUM_THREADS=4`, `OMP_WAIT_POLICY=PASSIVE`, and `OMP_DYNAMIC=false`. PolyBench verify uses one thread.

## Adding a new kernel test

1. Place input under `input_files/tiling/` or `input_files/subsub/`.
2. Add the kernel to `benchmarks/config/kernels/<suite>.json` (name, input path, and any extra flags). No recompile.
3. `./cetus_regression_test --run-test <name> --generate`
4. Commit input, the JSON, and `ground_truth/<suite>/*_gt.c`.
5. `./cetus_regression_test --run-test <name>`

## Adding a new benchmark case

1. Edit `benchmarks/config/suites/<suite>.json` — no recompile.
2. `./cetus_regression_test --benchmarks --run-test …` is **not** valid. Select cases with `--benchmark-suite`, `--benchmark-kernel`, and `--profile` (each repeatable; omit one to keep all of that axis).
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

Runs: `tests/test_runner`, `test_benchmark_config`, `test_benchmark_runner`, `test_benchmark_adapters`, `test_cli`, `test_kernel_config`.

Use TDD when changing `benchmark_runner.c`, adapters, or CLI validation.

## Do not

- Reintroduce Python benchmark runtime
- Use single-file `gcc cetus_regression_test.c` build
- Mask missing GT with xfail
- Modify external PolyBench/NPB trees (adapters use sandboxes under `work/`)
- Use shell strings in generic adapter JSON commands
