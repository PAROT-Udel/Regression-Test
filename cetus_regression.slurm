#!/bin/bash
#SBATCH --job-name=cetus-reg
#SBATCH --partition=standard
#SBATCH --account=parot
#SBATCH --time=06:00:00
#SBATCH --cpus-per-task=4
#SBATCH --output=/work/parot/mgltorsa/cetus_regression_test_suite/logs/slurm-%j.out
#SBATCH --error=/work/parot/mgltorsa/cetus_regression_test_suite/logs/slurm-%j.err
# Generate the new PAW tile-size profiles, then run kernels and benchmarks.
# devel is capped at 2 hours, which is shorter than generate plus a full run.
set -uo pipefail
ROOT=/work/parot/mgltorsa/cetus_regression_test_suite
cd "$ROOT"
make
make_rc=$?
echo "MAKE_RC:${make_rc}"
if [[ "$make_rc" -ne 0 ]]; then
    exit "$make_rc"
fi
echo "===== generate paw_tiling_lrw and paw_tiling_fixed32 ====="
bash "$ROOT/run_profiles.sh" --benchmarks \
    --profile paw_tiling_lrw --profile paw_tiling_fixed32 \
    --generate --skip-run
generate_rc=$?
echo "GENERATE_RC:${generate_rc}"
echo "===== full regression ====="
bash "$ROOT/run_profiles.sh" --full
run_rc=$?
echo "RUN_RC:${run_rc}"
if [[ "$generate_rc" -ne 0 || "$run_rc" -ne 0 ]]; then
    exit 1
fi
