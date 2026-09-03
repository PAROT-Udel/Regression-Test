# Example: Run the Cetus regression suites

End-to-end walkthrough for running the **PAW tiling** and **subscripted-subscript analysis** regression suites against Cetus `parot/release/3.0`.

## 1. Build Cetus from `parot/release/3.0`

```bash
cd /mnt/d/workspace/cetus/The-Cetus-Project   # adjust path
git fetch parot
git checkout release/3.0
./build.sh bin
```

Verify:

```bash
./bin/cetus -version
```

## 2. Configure the regression runner

Edit `helper_tests.h` so `CETUS_PATH` points at your `bin/cetus` wrapper:

```c
#define CETUS_PATH "/mnt/d/workspace/cetus/The-Cetus-Project/bin/cetus"
```

## 3. Compile the regression harness

```bash
cd /mnt/d/workspace/cetus/cetus_regression_test_suite
chmod +x check_syntax.sh
gcc -o cetus_regression_test cetus_regression_test.c -I. -Wall
```

## 4. Run all tests (compare mode)

```bash
./cetus_regression_test --all
```

Expected output:

```
Test Summary: 15/15 tests passed.
```

## 5. Run one suite or one test

```bash
# List suite names
./cetus_regression_test --list-suites

# Entire tiling suite (all files in input_files/tiling/)
./cetus_regression_test --run-suite tiling

# Entire subsub suite (all files in input_files/subsub/)
./cetus_regression_test --run-suite subsub

# Single tiling test
./cetus_regression_test --run-test Tiling_PAW_GEMM_Fixed64

# Single subsub test
./cetus_regression_test --run-test SubSub_Amgmk
```

## 6. Regenerate ground truth (after Cetus changes)

Only do this when output changes are **intentional**:

```bash
# All tests
./cetus_regression_test --generate

# One suite
./cetus_regression_test --run-suite tiling --generate

# One test
./cetus_regression_test --run-test Tiling_PAW_3mm_ExperimentalSections --generate
```

Then re-run compare mode and commit updated `ground_truth/*_gt.c` files.

## 7. What each suite covers

| Suite (`--run-suite`) | Folder | Pass | Count |
|-----------------------|--------|------|-------|
| `tiling` | `input_files/tiling/` | `-paw_tiling` | 6 |
| `subsub` | `input_files/subsub/` | `-subsub_analysis` | 9 |

## 8. Windows + WSL notes

The runner uses POSIX APIs (`unistd.h`, `bash`, `mkdir -p`). On Windows, run inside **WSL**:

```powershell
wsl bash -lc "cd /mnt/d/workspace/cetus/cetus_regression_test_suite && ./cetus_regression_test --all"
```

Install toolchain in WSL if needed:

```bash
sudo apt-get install -y gcc clang clang-format openjdk-21-jdk
```
