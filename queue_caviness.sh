#!/usr/bin/env bash
# Copy this project to Caviness, or update the copy already there, then
# submit the full-regression Slurm job. Prints the job id and returns.
# It does not wait for the job.
#
#   ./queue_caviness.sh
#   ./queue_caviness.sh --no-sync    # submit only
#
# If a cetus-reg job is already queued or running, the script asks before
# submitting another one.
#
# Connection is OpenSSH to the host alias "caviness" (override with
# CAVINESS_SSH_HOST). BatchMode is on, so a password prompt is not possible.
# ~/.ssh/config must name that host, and IdentityFile must be a key Caviness
# accepts. See AGENTS.md, section Caviness.
#
# Remote tree: /work/parot/mgltorsa/cetus_regression_test_suite
# Log:         logs/slurm-<jobid>.out on that tree
set -euo pipefail

HOST="${CAVINESS_SSH_HOST:-caviness}"
REMOTE="${CAVINESS_SUITE:-/work/parot/mgltorsa/cetus_regression_test_suite}"
ROOT="$(cd "$(dirname "$0")" && pwd)"
SYNC=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sync) SYNC=0; shift ;;
        -h|--help)
            sed -n '2,18p' "$0"
            exit 0
            ;;
        *)
            echo "unknown arg: $1" >&2
            exit 2
            ;;
    esac
done

existing="$(ssh -o BatchMode=yes -o ConnectTimeout=20 "$HOST" \
    "squeue -h -u \"\$USER\" -n cetus-reg -o '%i %T %M'")"
if [[ -n "$existing" ]]; then
    echo "A regression job is already queued or running:"
    echo "$existing"
    read -r -p "Submit another one? [y/N] " answer
    case "$answer" in
        y|Y|yes|YES) ;;
        *)
            echo "Not submitted."
            exit 0
            ;;
    esac
fi

if [[ "$SYNC" -eq 1 ]]; then
    echo "Updating ${HOST}:${REMOTE}"
    # --delete makes a later run match this tree. Excluded paths are left
    # alone on the cluster (logs, sandboxes, the binary rebuilt by the job).
    rsync -a --delete \
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

echo "Submitting full regression"
ssh -o BatchMode=yes -o ConnectTimeout=20 "$HOST" \
    "mkdir -p '${REMOTE}/logs' && sbatch --parsable '${REMOTE}/cetus_regression.slurm'"
