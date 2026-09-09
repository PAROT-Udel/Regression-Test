/*
Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it andor
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https:www.gnu.org/licenses/>.
*/
/*
This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.
*/
/*
glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.
*/
/*
wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISOIEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters
*/
/*

 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http:polybench.sourceforge.net

*/
/* doitgen.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "doitgen.h"
/* Array initialization. */
static void init_array(int nr, int nq, int np,
                       double A[(10 + 0)][(8 + 0)][(12 + 0)],
                       double C4[(12 + 0)][(12 + 0)]) {
  int i, j, k;
#pragma cetus private(i, j, k)
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for private(i, j, k)
  for (i = 0; i < nr; i++) {
#pragma cetus private(j, k)
#pragma loop name init_array #0 #0
    for (j = 0; j < nq; j++) {
#pragma cetus private(k)
#pragma loop name init_array #0 #0 #0
      for (k = 0; k < np; k++) {
        A[i][j][k] = (((double)(((i * j) + k) % np)) / np);
      }
    }
  }
#pragma cetus private(i, j)
#pragma loop name init_array #1
#pragma cetus parallel
#pragma omp parallel for private(i, j)
  for (i = 0; i < np; i++) {
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < np; j++) {
      C4[i][j] = (((double)((i * j) % np)) / np);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int nr, int nq, int np,
                        double A[(10 + 0)][(8 + 0)][(12 + 0)]) {
  int i, j, k;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "A");
#pragma cetus private(i, j, k)
#pragma loop name print_array #0
  for (i = 0; i < nr; i++) {
#pragma cetus private(j, k)
#pragma loop name print_array #0 #0
    for (j = 0; j < nq; j++) {
#pragma cetus private(k)
#pragma loop name print_array #0 #0 #0
      for (k = 0; k < np; k++) {
        if ((((((i * nq) * np) + (j * np)) + k) % 20) == 0) {
          fprintf(stderr, "\n");
        }
        fprintf(stderr, "%0.2lf ", A[i][j][k]);
      }
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "A");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
void kernel_doitgen(int nr, int nq, int np,
                    double A[(10 + 0)][(8 + 0)][(12 + 0)],
                    double C4[(12 + 0)][(12 + 0)], double sum[(12 + 0)]) {
  int r, q, p, s;
#pragma scop
#pragma cetus firstprivate(sum)
#pragma cetus private(p, q, r, s)
#pragma cetus lastprivate(sum)
#pragma loop name kernel_doitgen #0
#pragma cetus parallel
#pragma omp parallel for private(p, q, r, s) firstprivate(sum) lastprivate(sum)
  for (r = 0; r < nr; r++) {
#pragma cetus firstprivate(sum)
#pragma cetus private(p, q, s)
#pragma cetus lastprivate(sum)
#pragma loop name kernel_doitgen #0 #0
    for (q = 0; q < nq; q++) {
#pragma cetus private(p, s)
#pragma loop name kernel_doitgen #0 #0 #0
      for (p = 0; p < np; p++) {
        sum[p] = 0.0;
#pragma cetus private(s)
#pragma loop name kernel_doitgen #0 #0 #0 #0
        /* #pragma cetus reduction(+: sum[p])  */
        for (s = 0; s < np; s++) {
          sum[p] += (A[r][q][s] * C4[s][p]);
        }
      }
#pragma cetus private(p)
#pragma loop name kernel_doitgen #0 #0 #1
      for (p = 0; p < np; p++) {
        A[r][q][p] = sum[p];
      }
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int nr = 10;
  int nq = 8;
  int np = 12;
  /* Variable declarationallocation. */
  double(*A)[(10 + 0)][(8 + 0)][(12 + 0)];
  double(*sum)[(12 + 0)];
  double(*C4)[(12 + 0)][(12 + 0)];
  A = ((double(*)[(10 + 0)][(8 + 0)][(12 + 0)]) polybench_alloc_data(
      ((10 + 0) * (8 + 0)) * (12 + 0), sizeof(double)));
  ;
  sum = ((double(*)[(12 + 0)]) polybench_alloc_data(12 + 0, sizeof(double)));
  ;
  C4 = ((double(*)[(12 + 0)][(12 + 0)])
            polybench_alloc_data((12 + 0) * (12 + 0), sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(nr, nq, np, *A, *C4);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_doitgen(nr, nq, np, *A, *C4, *sum);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(nr, nq, np, *A);
  /* Be clean. */
  free((void*)A);
  ;
  free((void*)sum);
  ;
  free((void*)C4);
  ;
  return 0;
}
