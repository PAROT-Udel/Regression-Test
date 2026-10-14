#!/usr/bin/env bash
# Sync this tree to Caviness and submit the generate-then-run job.
# Prints the Slurm job id and returns. It does not wait for the job.
#
#   ./queue_caviness.sh
#   ./queue_caviness.sh --no-sync
#
# Logs: /work/parot/mgltorsa/cetus_regression_test_suite/logs/slurm-<jobid>.out
set -euo pipefail

HOST="${CAVINESS_SSH_HOST:-caviness}"
REMOTE="${CAVINESS_SUITE:-/work/parot/mgltorsa/cetus_regression_test_suite}"
ROOT="$(cd "$(dirname "$0")" && pwd)"
SYNC=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sync) SYNC=0; shift ;;
        -h|--help)
            sed -n '2,8p' "$0"
            exit 0
            ;;
        *)
            echo "unknown arg: $1" >&2
            exit 2
            ;;
    esac
done

if [[ "$SYNC" -eq 1 ]]; then
    rsync -a \
        --exclude work/ \
        --exclude logs/ \
        --exclude cetus_intermediate_i_files/ \
        --exclude cetus_transformed_output/ \
        --exclude benchmarks/config/paths.env \
        --exclude '*.o' \
        --exclude cetus_regression_test \
        --exclude tests/test_runner \
        --exclude tests/test_benchmark_config \
        --exclude tests/test_benchmark_runner \
        --exclude tests/test_benchmark_adapters \
        --exclude tests/test_cli \
        --exclude docs/'*.aux' \
        --exclude docs/'*.log' \
        --exclude docs/'*.nav' \
        --exclude docs/'*.out' \
        --exclude docs/'*.snm' \
        --exclude docs/'*.toc' \
        "${ROOT}/" "${HOST}:${REMOTE}/"
fi

ssh -o BatchMode=yes -o ConnectTimeout=20 "$HOST" \
    "mkdir -p '${REMOTE}/logs' && sbatch --parsable '${REMOTE}/cetus_regression.slurm'"
