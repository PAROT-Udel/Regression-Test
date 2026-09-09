#!/usr/bin/env bash
# Build the native runner, then run kernels and default benchmark suites.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

make
exec ./cetus_regression_test --full "$@"
