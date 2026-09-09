# Cetus Regression Test Suite

This project validates the [Cetus](https://github.com/PAROT-Udel/The-Cetus-Project) source-to-source compiler by comparing its output against checked-in **ground truth** files, then (for benchmarks) rebuilding and running the transformed code to confirm it still behaves correctly.

The suite is **independent of the Cetus source tree**: you build Cetus separately, point the runner at it, and run everything from **Linux or WSL**.

## What is being tested

### 1. Kernel tests (15 cases)

Small, focused C kernels grouped into two suites:

| Suite | Folder | Count | What it exercises |
|-------|--------|-------|-------------------|
| **tiling** | `input_files/tiling/` | 6 | PAW parallel-aware tiling (`-paw_tiling=1`) on GEMM, 2MM, 3MM, and related kernels |
| **subsub** | `input_files/subsub/` | 9 | Subscripted-subscript analysis (`-subsub_analysis`) on AMG-style and related patterns |

Each kernel test:

1. Preprocesses the input `.c` with `clang -E` to a `.i` file
2. Syntax-checks the preprocessed file (`check_syntax.sh`)
3. Runs Cetus with suite-specific flags from `master_test_cases.h`
4. Formats Cetus output and ground truth with `clang-format`
5. Diffs formatted output against `ground_truth/<suite>/*_gt.c`

Expected result for a clean run: **`Test Summary: 15/15 tests passed.`**

### 2. Benchmark tests (PolyBench / NPB)

Full application kernels from external benchmark trees, driven by JSON configuration under `benchmarks/config/`. Default runs include:

| Suite | Source tree | Profiles | Kernels |
|-------|-------------|----------|---------|
| **polybench** | PolyBench/C 4.2 (`POLYBENCH_ROOT`) | `parallel`, `paw_tiling` | 30 kernels (GEMM, 2MM, heat-3d, …) |
| **npb-ser** | NPB 3.3 serial C (`NPB_SER_ROOT`) | `parallel` | 9 kernels (CG, EP, MG, BT, …) at CLASS=S |

Each benchmark case runs a multi-stage pipeline:

1. **Sandbox** — copy only the files needed into `work/benchmarks/` (never modify your benchmark trees)
2. **Transform** — run Cetus with a suite-specific preprocessor (`cpp -C -I…`) and profile flags
3. **Ground truth** — format transformed `.c` output and diff against `ground_truth/benchmarks/<suite>/<profile>/<kernel>/`
4. **Inject** — copy transformed sources back into the sandbox build tree
5. **Build & verify**
   - **PolyBench**: compile baseline and transformed binaries, run both with `OMP_NUM_THREADS=1`, compare `POLYBENCH_DUMP_ARRAYS` stderr output
   - **NPB**: `make` the benchmark, run the binary, grep for `Verification Successful`

Some kernels are marked **xfail** in JSON when Cetus 3.0 is known to fail or produce incorrect results (see [Expected failures](#expected-failures-xfail)).

### 3. Unit tests (developer)

Native C tests under `tests/` validate the runner infrastructure (process execution, JSON config parsing, benchmark lifecycle, adapters, CLI). Run with `make test`.

---

## How it works

```
┌─────────────────────────────────────────────────────────────────┐
│                    cetus_regression_test                        │
│  (single native C executable — no Python at runtime)            │
├────────────────────────────┬────────────────────────────────────┤
│  Kernel mode               │  Benchmark mode                    │
│  master_test_cases.h       │  benchmarks/config/*.json          │
│  helper_tests.h (paths)    │  paths.env (tool + root paths)     │
├────────────────────────────┼────────────────────────────────────┤
│  clang → .i → Cetus →     │  adapter (polybench/npb/generic)   │
│  clang-format → diff        │  → Cetus → GT diff → build → verify│
└────────────────────────────┴────────────────────────────────────┘
```

**Key design choices:**

- **Ground truth in git** — intentional Cetus output changes require `--generate` and a committed update to `ground_truth/`.
- **Private sandboxes** — benchmark adapters copy sources into `work/benchmarks/`; your PolyBench/NPB trees are read-only inputs.
- **JSON-driven benchmarks** — add suites, kernels, profiles, and xfails without recompiling (only editing `benchmarks/config/`).
- **XFAIL / XPASS** — expected Cetus failures count as `XFAIL` (nonzero exit only for unexpected `XPASS`). When no xfail is configured, missing ground truth is a hard `FAIL`. When an xfail is set, the runner skips the GT pre-check and lets Cetus run so its failure can trigger the xfail.

---

## Prerequisites

On Ubuntu / WSL:

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk diffutils make
```

You also need:

- **Cetus** built from `parot/release/3.0` (Java 8+ and `javac`)
- **PolyBench/C 4.2** and **NPB3.3-SER-C** trees (paths configured below)
- Optional: **NPB3.3-OMP-C** for the opt-in `npb-omp` suite

Run the harness from **Linux or WSL**, not native Windows cmd.

---

## Setup

### 1. Build Cetus

Ground truth in this repo targets Cetus from the **`parot` remote, branch `release/3.0`**.

```bash
git clone https://github.com/PAROT-Udel/The-Cetus-Project.git
cd The-Cetus-Project
./build.sh bin
./bin/cetus -version
```

If you already have the clone:

```bash
cd /path/to/The-Cetus-Project
git fetch parot
git checkout release/3.0   # or: git checkout -B release/3.0 parot/release/3.0
./build.sh bin
```

**Important:** `bin/cetus` is a wrapper script with hard-coded paths to `lib/cetus.jar`. Do not copy `bin/cetus` alone to another directory.

### 2. Configure paths

**Kernel tests** — edit `CETUS_PATH` in [`helper_tests.h`](helper_tests.h):

```c
#define CETUS_PATH "/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus"
```

Recompile after changing this (`make`).

**Benchmark tests** — copy and edit the env file:

```bash
cp benchmarks/config/paths.example.env benchmarks/config/paths.env
```

Example `paths.env`:

```bash
CETUS=/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus
CLANG_FORMAT=clang-format
GCC=gcc
DIFF=diff
CPP=cpp

POLYBENCH_ROOT=/mnt/d/workspace/ud-masters/benchmarks/polybench-c-4.2
NPB_SER_ROOT=/mnt/d/workspace/ud-masters/benchmarks/NPB3.3-SER-C
NPB_OMP_ROOT=/mnt/d/workspace/ud-masters/benchmarks/NPB3.3-OMP-C
```

Environment variables override `paths.env`. You can point at tiling forks; adapters still use canonical kernel sources from the tree root.

### 3. Build the harness

```bash
cd /path/to/cetus_regression_test_suite
chmod +x check_syntax.sh run_all.sh
make
```

This builds `cetus_regression_test` and links all native modules (CLI, JSON config, benchmark adapters, process runner).

Run unit tests:

```bash
make test
```

---

## Running tests

All commands assume you are in the suite root and have run `make`.

### Quick reference

| Goal | Command |
|------|---------|
| All kernel tests | `./cetus_regression_test --all` |
| All default benchmarks | `./cetus_regression_test --benchmarks` |
| Kernels + benchmarks | `./cetus_regression_test --full` or `./run_all.sh` |
| List kernel suites | `./cetus_regression_test --list-suites` |
| List benchmark suites | `./cetus_regression_test --list-benchmark-suites` |
| Help | `./cetus_regression_test --help` |

**Note:** `./cetus_regression_test` with no arguments runs **kernel tests only** (same as `--all`). Benchmarks require `--benchmarks` or `--full`.

### Kernel suites and individual tests

```bash
# Suites
./cetus_regression_test --run-suite tiling      # 6 PAW tiling tests
./cetus_regression_test --run-suite subsub      # 9 subsub tests

# One test (by category, filename, or path)
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64
./cetus_regression_test --run-test SubSub_Amgmk
./cetus_regression_test --run-test tiling/tiling_gemm.c

# Override Cetus flags for one kernel run
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64 \
  -cetus-options "-paw_tiling=1 -tileSizes=32"
```

### Benchmark suites and individual tests

```bash
# Default: polybench (parallel + paw_tiling) + npb-ser (parallel)
./cetus_regression_test --benchmarks

# One suite
./cetus_regression_test --benchmarks --benchmark-suite polybench
./cetus_regression_test --benchmarks --benchmark-suite npb-ser

# One kernel (all profiles for that kernel)
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-kernel gemm
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --benchmark-kernel cg

# One kernel + one profile
./cetus_regression_test --benchmarks --benchmark-suite polybench \
  --benchmark-kernel gemm --profile parallel
./cetus_regression_test --benchmarks --benchmark-suite polybench \
  --benchmark-kernel gemm --profile paw_tiling

# Repeatable suite filter
./cetus_regression_test --benchmarks --benchmark-suite polybench --benchmark-suite npb-ser

# Opt-in OMP NPB suite
./cetus_regression_test --benchmarks --benchmark-suite npb-omp
```

### Benchmark options

```bash
# Dataset / class overrides (PolyBench / NPB)
./cetus_regression_test --benchmarks --benchmark-suite polybench --dataset SMALL_DATASET
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --class W

# Skip stages
./cetus_regression_test --benchmarks --skip-gt      # transform + build/verify only
./cetus_regression_test --benchmarks --skip-run     # transform + GT compare only

# Timeout (seconds, default 600)
./cetus_regression_test --benchmarks --timeout 1200

# Alternate config directory
./cetus_regression_test --benchmarks --config-dir /path/to/config
```

### Verbose output and ground truth generation

```bash
# Show Cetus/tool stdout/stderr (default is quiet)
./cetus_regression_test --run-suite tiling --verbose
./cetus_regression_test --benchmarks --benchmark-kernel gemm --verbose true

# Regenerate ground truth (only after intentional Cetus output change)
./cetus_regression_test --generate
./cetus_regression_test --run-suite tiling --generate
./cetus_regression_test --benchmarks --generate --benchmark-suite polybench --benchmark-kernel gemm
```

Then re-run without `--generate` and commit updated files under `ground_truth/`.

### Capture logs for debugging

```bash
mkdir -p logs
./cetus_regression_test --benchmarks 2>&1 | tee logs/benchmark-run-console.log
```

Per-case benchmark logs are written under `logs/benchmarks/`. Kernel logs are under `logs/`.

---

## Exit codes and result statuses

| Status | Meaning | Counts toward failure exit code? |
|--------|---------|----------------------------------|
| **PASS** | Transform matched GT and verify succeeded | No |
| **FAIL** | Unexpected failure (diff, build, verify, missing GT, infra) | Yes |
| **XFAIL** | Failed as documented in JSON `xfail` | No |
| **XPASS** | Documented xfail case passed unexpectedly | Yes |
| **SKIP** | Adapter declined the case | No |

The process exits **nonzero** if any case is **FAIL** or **XPASS**.

---

## Expected failures (xfail)

These are documented in `benchmarks/config/suites/*.json` for Cetus **release/3.0**:

| Suite | Kernel | Reason |
|-------|--------|--------|
| polybench | `atax`, `bicg` | Transformed binary SIGSEGV at MINI dataset |
| npb-ser | `bt`, `sp`, `lu` | Incorrect numerical results after transform |
| npb-ser | `ft` | Cetus InternalError in ArrayPrivatization |
| npb-ser | `ua` | Cetus parse failure on `ua.c` |

A full `--benchmarks` run should show these as **XFAIL**, not **FAIL**. If one **XPASS**es, that indicates a fix worth investigating.

---

## Configuration reference

### Directory layout

```
cetus_regression_test_suite/
├── cetus_regression_test.c   # Main entry + kernel runner
├── cli.c / cli.h               # Unified CLI parsing
├── helper_tests.h              # Kernel tool paths, TestCase struct
├── master_test_cases.h         # 15 kernel test definitions
├── benchmark_config.c/.h       # JSON + paths.env loader
├── benchmark_runner.c/.h       # Benchmark lifecycle orchestration
├── benchmark_adapters.c/.h     # Adapter factory
├── polybench_adapter.c         # PolyBench sandbox + verify
├── npb_adapter.c               # NPB sandbox + verify
├── generic_adapter.c           # Bring-your-own JSON adapter
├── runner_common.c/.h          # Paths, copy, env parsing
├── process_runner.c/.h         # Subprocess exec with timeout
├── Makefile
├── check_syntax.sh
├── run_all.sh                  # make && --full
├── benchmarks/config/
│   ├── paths.example.env
│   ├── profiles.json           # Cetus flag profiles
│   └── suites/                 # polybench.json, npb-ser.json, …
├── input_files/                # Kernel inputs
├── ground_truth/               # Expected outputs (kernels + benchmarks)
├── work/benchmarks/            # Sandboxes (gitignored)
├── cetus_intermediate_i_files/ # Preprocessed .i (gitignored)
├── cetus_transformed_output/   # Kernel Cetus output (gitignored)
└── logs/                       # Run logs (gitignored)
```

### Profiles (`benchmarks/config/profiles.json`)

| Profile | Used by | Purpose |
|---------|---------|---------|
| `parallel` | PolyBench + NPB | Standard OpenMP parallelization |
| `paw_tiling` | PolyBench | Parallel-aware tiling (`-paw_tiling=1`) |

### Bring-your-own benchmarks (generic adapter)

Copy `benchmarks/config/suites/generic-example.json` and set `"adapter": "generic"`. The generic adapter requires:

- **`copy`** — files/dirs to copy into the sandbox (safe relative paths only)
- **`cetus_inputs`** — sources to transform
- **`inject_map`** — map from Cetus output basename to sandbox destination
- **`compile`** — argv array (not shell strings)
- **`verify`** — one of:
  - `exit_code` — run command, check exit status
  - `regex` — POSIX extended regex on combined stdout/stderr
  - `dump_diff` — run baseline and transformed commands, diff captured output streams

Template placeholders: `{sandbox}`, `{dataset}`, `{class}`, `{cpp}`, `{gcc}`, etc.

Shell command strings in JSON are **rejected** at load time; use argv arrays only.

Mark known Cetus bugs with `"xfail": "reason"` or per-profile `"xfail_profiles": {"paw_tiling": "reason"}`.

---

## Adding a new kernel test

1. Add the input `.c` under `input_files/tiling/` or `input_files/subsub/`.
2. Add a `TestCase` entry to [`master_test_cases.h`](master_test_cases.h) with suite, category, flags, and expected outcome.
3. Rebuild: `make`
4. Generate ground truth: `./cetus_regression_test --run-test <category> --generate`
5. Verify: `./cetus_regression_test --run-test <category>`
6. Commit the input file and `ground_truth/<suite>/*_gt.c`.

## Adding a new benchmark kernel

1. Edit the appropriate file under `benchmarks/config/suites/`.
2. No recompile needed — JSON is loaded at runtime.
3. Generate GT: `./cetus_regression_test --benchmarks --benchmark-suite <id> --benchmark-kernel <name> --generate --skip-run`
4. Run full pipeline: `./cetus_regression_test --benchmarks --benchmark-suite <id> --benchmark-kernel <name>`
5. Commit JSON changes and new files under `ground_truth/benchmarks/`.

---

## Debugging

### Log files

| File | Contents |
|------|----------|
| `logs/all_tests.log` | Full kernel run log |
| `logs/failed_tests.log` | Kernel failures |
| `logs/benchmarks/run_*.log` | Per benchmark case |
| `logs/benchmark-run-console.log` | If you used `tee` (see above) |

### Manual kernel pipeline

```bash
clang -E -P -x c -std=c11 input_files/tiling/tiling_gemm.c -o /tmp/gemm.i
./check_syntax.sh /tmp/gemm.i
/path/to/The-Cetus-Project/bin/cetus -outdir=cetus_transformed_output/tiling <flags> /tmp/gemm.i
clang-format -i -style=Google cetus_transformed_output/tiling/tiling_gemm.i
clang-format -i -style=Google ground_truth/tiling/tiling_gemm_gt.c
diff -wB cetus_transformed_output/tiling/tiling_gemm.i ground_truth/tiling/tiling_gemm_gt.c
```

### Windows / WSL

From PowerShell:

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && make && ./cetus_regression_test --all"
```

---

## Development

```bash
make clean && make          # rebuild harness
make test                   # run all unit test binaries
./cetus_regression_test --help
```

Unit test binaries (not installed system-wide):

- `tests/test_runner` — filesystem + process runner
- `tests/test_benchmark_config` — JSON/env loading
- `tests/test_benchmark_runner` — lifecycle + xfail semantics
- `tests/test_benchmark_adapters` — PolyBench/NPB/generic adapters
- `tests/test_cli` — CLI parsing and validation

For agent-oriented notes (adding tests, xfails, layout), see [`AGENTS.md`](AGENTS.md).
