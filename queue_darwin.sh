#!/usr/bin/env bash
# Submit the regression suite on Darwin as separate Slurm jobs.
# Run this on a Darwin login node. It builds the harness once, submits
# the jobs, prints their ids, and returns. It does not wait.
#
#   ./queue_darwin.sh                 # kernels, NPB serial, and PolyBench
#   ./queue_darwin.sh kernels         # one or more of: kernels, npb, poly
#   ./queue_darwin.sh npb poly
#
# Logs: logs/slurm-<jobid>.out and logs/slurm-<jobid>.err
#
# The parot Cetus install is the default. Override any of these before
# running the script: CETUS, CLANG_FORMAT, GCC, CPP, DIFF,
# POLYBENCH_ROOT, NPB_SER_ROOT.
set -eo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
CETUS="${CETUS:-/lustre/parot/cetus/cetus_release_3_0/bin/cetus}"
GCC="${GCC:-/usr/bin/gcc}"
CPP="${CPP:-/usr/bin/cpp}"
DIFF="${DIFF:-diff}"
POLYBENCH_ROOT="${POLYBENCH_ROOT:-$ROOT/benchmarks/polybench-c-4.2.1-beta}"
NPB_SER_ROOT="${NPB_SER_ROOT:-$ROOT/benchmarks/NPB3.3-SER-C}"

usage() {
    sed -n '2,16p' "$0"
}

submit_one() {
    local name="$1"
    local walltime="$2"
    local args="$3"
    local script="$ROOT/logs/queue-darwin-${name}.slurm"
    local out job_id

    cat > "$script" <<EOF
#!/bin/bash
#SBATCH --job-name=cetus-reg-${name}
#SBATCH --partition=standard
#SBATCH --account=parot
#SBATCH --time=${walltime}
#SBATCH --cpus-per-task=4
#SBATCH --output=${ROOT}/logs/slurm-%j.out
#SBATCH --error=${ROOT}/logs/slurm-%j.err
set -o pipefail
source /etc/profile.d/valet.sh
vpkg_require llvm/17
cd "${ROOT}"
export CETUS="${CETUS}"
export GCC="${GCC}"
export CPP="${CPP}"
export DIFF="${DIFF}"
export POLYBENCH_ROOT="${POLYBENCH_ROOT}"
export NPB_SER_ROOT="${NPB_SER_ROOT}"
CLANG_FORMAT="\${CLANG_FORMAT:-\$(command -v clang-format)}"
export CLANG_FORMAT
./cetus_regression_test ${args}
echo ${name}_RC:\$?
EOF
    chmod +x "$script"
    out="$(workgroup -g parot --command "cd '${ROOT}' && sbatch --parsable '${script}'")"
    job_id="$(printf '%s\n' "$out" | grep -oE '[0-9]+' | tail -n 1)"
    if [[ -z "$job_id" ]]; then
        echo "sbatch failed for ${name}:" >&2
        printf '%s\n' "$out" >&2
        exit 1
    fi
    printf '%-8s %s  %s\n' "$name" "$job_id" "logs/slurm-${job_id}.out"
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

mkdir -p "$ROOT/logs"
echo "Building the harness in ${ROOT}"
make -C "$ROOT"

if [[ $# -eq 0 ]]; then
    set -- kernels npb poly
fi

for name in "$@"; do
    case "$name" in
        kernels) submit_one kernels 00:30:00 "--all" ;;
        npb)     submit_one npb     04:00:00 "--benchmarks --benchmark-suite npb-ser" ;;
        poly)    submit_one poly    06:00:00 "--benchmarks --benchmark-suite polybench" ;;
        *)
            echo "unknown suite: $name (use kernels, npb, or poly)" >&2
            exit 2
            ;;
    esac
done
