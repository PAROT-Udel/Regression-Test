#!/bin/bash
# Run the Cetus regression harness on Caviness against release/3.0.
# Ground truth was generated with the system gcc 4.8 headers. Do not put
# llvm's newer gcc on PATH ahead of /usr/bin/gcc, or the diffs change.
set -eo pipefail
source /etc/profile.d/valet.sh
vpkg_require llvm/20 gcc/11.2
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8
export CETUS=/work/parot/mgltorsa/The-Cetus-Project/bin/cetus
export CLANG_FORMAT
CLANG_FORMAT=$(command -v clang-format)
export GCC=/usr/bin/gcc
export CPP=/usr/bin/cpp
export DIFF=diff
export POLYBENCH_ROOT=/work/parot/mgltorsa/benchmarks/polybench-c-4.2
export NPB_SER_ROOT=/work/parot/mgltorsa/benchmarks/NPB3.3-SER-C-vanilla
export NPB_OMP_ROOT=/work/parot/mgltorsa/benchmarks/NPB3.3-OMP-C
cd /work/parot/mgltorsa/cetus_regression_test_suite
mkdir -p logs
exec ./cetus_regression_test "$@"
