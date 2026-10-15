# Benchmarks

These are the C benchmark trees the regression suite compiles and checks. The harness does not modify them. Each run copies a kernel into `work/`.

## PolyBench/C 4.2.1-beta

Directory: `polybench-c-4.2.1-beta`

- Archive: <https://downloads.sourceforge.net/project/polybench/polybench-c-4.2.1-beta.tar.gz>
- Project page: <http://web.cse.ohio-state.edu/~pouchet.2/software/polybench/>
- Author: Louis-Noël Pouchet, Ohio State University (2016-05-20)

Correctness in this suite is a dump compare. Both the original kernel and the Cetus output are compiled with `-DPOLYBENCH_DUMP_ARRAYS`. Each binary runs with one OpenMP thread, and the text between `==BEGIN DUMP_ARRAYS==` and `==END DUMP_ARRAYS==` must match.

## NPB3.3-SER-C

Directory: `NPB3.3-SER-C`

This is the serial C port from SNU NPB 1.0.3 (Center for Manycore Programming, Seoul National University: Sangmin Seo, Jungwon Kim, Jun Lee, Jeongho Nah, Gangwon Jo, and Jaejin Lee). It is C, not the OpenCL trees in that package and not a C++ port.

- Package: <https://github.com/akshay9594/SNU_NPB-1.0.3> (`NPB3.3-SER-C` only)
- SNU project page: <http://aces.snu.ac.kr/software/snu-npb/>
- The Fortran NAS NPB 3.3.1 archive (<https://www.nas.nasa.gov/software/npb.html>) is a different distribution and is not used here.

Kernels: BT, SP, LU, MG, CG, FT, EP, IS, and UA. Class S.

Correctness is the benchmark's own check. The serial binary must exit 0 and print a line matching `Verification = SUCCESSFUL`. The suite runs that binary with 4 OpenMP threads.

`npb-omp` (the OpenMP C tree) is not included. Cetus still fails on `ua/adapt.c` in that tree, and that kernel has no ground truth.
