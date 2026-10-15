# Cetus Regression Test Suite

This project checks that the [Cetus](https://github.com/PAROT-Udel/The-Cetus-Project) source-to-source compiler still produces the expected C and that the transformed benchmarks still run correctly.

It is separate from the Cetus source tree. Build Cetus yourself, point this suite at it, and run the harness from Linux or WSL. The harness is native C. Benchmark settings live in JSON and are read at runtime, so changing a suite does not require a rebuild.

A short picture of the same flow is in [`docs/cetus_regression_test_slides.pdf`](docs/cetus_regression_test_slides.pdf).

## Terms

| Word | Meaning |
|------|---------|
| **Kernel snippet** | One small C file in this repo (`input_files/tiling/` or `input_files/subsub/`). There are 15, listed in `benchmarks/config/kernels/*.json`. |
| **Benchmark kernel** | One program taken from PolyBench or NPB, such as `gemm` or `bt`. |
| **Profile** | One named list of Cetus flags, such as `parallel` or `paw_tiling_nt`. |
| **Benchmark case** | One combination of suite + kernel + profile. |

A default benchmark run is 165 cases: PolyBench has 30 kernels and 4 profiles (120), and NPB serial has 9 kernels and 5 profiles (45). Kernel mode is 15 source diffs. With no arguments, the harness runs the 15 kernel snippets only.

## What is checked

Kernel snippets stop after a source diff. They are not compiled or run.

Each benchmark case:

1. Copies the kernel into a sandbox under `work/benchmarks/`. The PolyBench and NPB trees are not modified.
2. Runs Cetus with that profile's flags.
3. Formats the output and diffs it against checked-in ground truth.
4. Compiles the transformed program and runs it.

Correctness has two parts:

- **Source.** The diff against `ground_truth/` is empty. Benchmark ground truth is `ground_truth/benchmarks/<suite>/<profile>/<kernel>/`.
- **Run.** NPB must print `Verification Successful` (4 threads, passive wait). PolyBench runs the original and the transformed binary with 1 thread and diffs their dumped arrays.

`parallel` enables OpenMP. It uses `-alias=3` and `-profitable-omp=0`. Stock Cetus uses alias 1 and leaves profitable-omp on.

The three PAW profiles turn tiling on for 4 cores, a 25,600 KiB shared cache, and 64-byte lines. They differ only in how the tile size is chosen:

| Profile | Where | Tile size |
|---------|--------|-----------|
| `parallel` | PolyBench and NPB | none |
| `paw_tiling_nt` | PolyBench and NPB serial | NT (shared-cache model) |
| `paw_tiling_lrw` | PolyBench and NPB serial | LRW (FindB) |
| `paw_tiling_fixed32` | PolyBench and NPB serial | 32 on every tiled loop |
| `reuse_analysis` | NPB serial | none; adds reuse-vector analysis |

## Prerequisites

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk diffutils make
```

You also need Cetus from `parot/release/3.0`, PolyBench/C 4.2, and NPB 3.3 serial C. NPB OpenMP is optional (`npb-omp` is opt-in).

## Setup

Build Cetus. `bin/cetus` is a wrapper; do not copy that script without `lib/cetus.jar`.

```bash
git clone https://github.com/PAROT-Udel/The-Cetus-Project.git
cd The-Cetus-Project
git checkout release/3.0
./build.sh bin
```

Kernel snippets and benchmark cases both read `benchmarks/config/paths.env`. Copy the template once and edit the copy. The harness does not load `paths.example.env`. An environment variable with the same name overrides the file. Changing `CETUS` does not require a rebuild.

```bash
cp benchmarks/config/paths.example.env benchmarks/config/paths.env
```

```bash
CETUS=/path/to/The-Cetus-Project/bin/cetus
CLANG_FORMAT=clang-format
GCC=gcc
DIFF=diff
CPP=cpp
POLYBENCH_ROOT=/path/to/polybench-c-4.2
NPB_SER_ROOT=/path/to/NPB3.3-SER-C
NPB_OMP_ROOT=/path/to/NPB3.3-OMP-C
```

`CETUS` and `CLANG_FORMAT` are used for kernel snippets and for benchmarks. Benchmark flags come from `profiles.json`. Kernel-snippet flags stay in `benchmarks/config/kernels/*.json`. The kernel preprocessor is `clang` (`CLANG_PATH` in `helper_tests.h`).

`paths.env` is gitignored.

Build and, if you are changing the harness, run the unit tests:

```bash
make
make test
```

Use `make`. The project is several C files; a single `gcc` of `cetus_regression_test.c` will not build it.

## Run

From the suite root:

| Goal | Command |
|------|---------|
| 15 kernel snippets | `./cetus_regression_test` |
| All default benchmarks | `./cetus_regression_test --benchmarks` |
| Kernels and benchmarks | `./cetus_regression_test --full` |
| One kernel suite | `./cetus_regression_test --run-suite tiling` |
| One kernel snippet | `./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64` |
| One benchmark suite | `./cetus_regression_test --benchmarks --benchmark-suite polybench` |
| One profile, every kernel | `./cetus_regression_test --benchmarks --profile paw_tiling_nt` |
| One kernel, one profile | `./cetus_regression_test --benchmarks --benchmark-kernel gemm --profile paw_tiling_fixed32` |
| List names | `./cetus_regression_test --list-suites` or `--list-benchmark-suites` |

`--benchmark-suite`, `--benchmark-kernel`, and `--profile` each filter one axis. Repeat a flag to include several values. Leave a flag off to keep every value on that axis. Kernel names match as substrings. Profile names must match `profiles.json` exactly.

`npb-omp` runs only when you pass `--benchmark-suite npb-omp`. It uses the `parallel` profile. Cetus fails on `ua/adapt.c` (`IntegerLiteral` cannot be cast to `RangeExpression`), so that kernel has no ground truth.

Other useful flags:

```bash
./cetus_regression_test --benchmarks --skip-run          # diff only
./cetus_regression_test --benchmarks --skip-gt           # build and run only
./cetus_regression_test --benchmarks --profile paw_tiling_lrw --generate --skip-run
./cetus_regression_test --benchmarks --benchmark-suite npb-ser --class W
./cetus_regression_test --benchmarks --timeout 1200
./cetus_regression_test --run-suite tiling --verbose
```

`--generate` rewrites ground truth. Use it after an intentional Cetus change, then run again without it and commit `ground_truth/`.

A clean kernel run prints `Test Summary: 15/15 tests passed.` The process exits nonzero when any case is `FAIL` or `XPASS`. `XFAIL` is a failure that the JSON already documents; the checked-in PolyBench and NPB suites do not use it.

Logs: `logs/failed_tests.log` for kernel failures, and `logs/benchmarks/run_*.log` for each benchmark case. A non-passing benchmark prints its log path.

From PowerShell:

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && make && ./cetus_regression_test --all"
```

## Caviness

`./queue_caviness.sh` copies this tree to `/work/parot/mgltorsa/cetus_regression_test_suite` (or updates the copy), then submits `cetus_regression.slurm`. The script prints the job id and returns. It does not wait. If a `cetus-reg` job is already queued or running, it asks before submitting another. `--no-sync` submits without copying.

The job generates ground truth for `paw_tiling_lrw` and `paw_tiling_fixed32`, then runs kernels and benchmarks (`--full`). The log on the cluster is `logs/slurm-<jobid>.out`. `run_profiles.sh` sets the cluster Cetus binary and the vanilla benchmark paths.

The script uses `ssh` and `rsync` with the host alias `caviness` and `BatchMode=yes`, so it will not prompt for a password. `~/.ssh/config` must define that host and a key Caviness accepts:

```
Host caviness
    HostName caviness.hpc.udel.edu
    User mgltorsa
    IdentityFile ~/.ssh/mgltorsa_caviness
```

`CAVINESS_SSH_HOST` overrides the alias. `Host` is the name the script uses. `HostName` is the cluster. `IdentityFile` is the private key.

## Adding a test

**Kernel snippet.** Put the `.c` file in `input_files/tiling/` or `input_files/subsub/`, and add it to `benchmarks/config/kernels/<suite>.json`. No rebuild. Then `./cetus_regression_test --run-test <name> --generate`. Commit the input, the JSON, and `ground_truth/<suite>/*_gt.c`.

**Benchmark.** Edit `benchmarks/config/suites/<suite>.json`. No rebuild. Generate with `--generate --skip-run`, then run the case without `--generate`. Commit the JSON and `ground_truth/benchmarks/`.

A custom suite can copy `benchmarks/config/suites/generic-example.json` (`"adapter": "generic"`). Commands must be argv arrays. Verify types are `exit_code`, `regex`, and `dump_diff`.

## Layout

```
cetus_regression_test.c      harness entry and kernel runner
cli.c                        arguments
kernel_config.c              loads benchmarks/config/kernels/*.json
benchmark_runner.c           sandbox, diff, build, verify
polybench_adapter.c          PolyBench copy and dump compare
npb_adapter.c                NPB build and verification string
benchmarks/config/           paths.example.env (template), profiles.json, suites/*.json, kernels/*.json
input_files/                 kernel snippet sources
ground_truth/                expected Cetus output
queue_caviness.sh            copy to Caviness and submit the Slurm job
run_profiles.sh              cluster paths, then the harness
cetus_regression.slurm       generate LRW and fixed-32, then --full
work/                        sandboxes (gitignored)
logs/                        run logs (gitignored)
```

Developer checks of the harness itself are `make test` (`tests/test_runner`, `test_benchmark_config`, `test_benchmark_runner`, `test_benchmark_adapters`, `test_cli`, `test_kernel_config`). Notes for changing the harness are in [`AGENTS.md`](AGENTS.md).
