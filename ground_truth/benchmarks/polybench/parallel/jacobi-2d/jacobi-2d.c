/*
Copyright (C) 1991-2012 Free Software Foundation, Inc.
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
   <http:www.gnu.org/licenses/>.
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
/* We do support the IEC 559 math functionality, real and complex.  */
/*
wchar_t uses ISOIEC 10646 (2nd ed., published 2011-03-15) /
   Unicode 6.0.
*/
/* We do not support C11 <threads.h>.  */
/*

 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http:polybench.sourceforge.net

*/
/* jacobi-2d.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "jacobi-2d.h"
/* Array initialization. */
static void init_array(int n, double A[(30 + 0)][(30 + 0)],
                       double B[(30 + 0)][(30 + 0)]) {
  int i, j;
#pragma cetus private(j)
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (i = 0; i < n; i++) {
#pragma cetus private(j)
#pragma loop name init_array #0 #0
    for (j = 0; j < n; j++) {
      A[i][j] = (((((double)i) * (j + 2)) + 2) / n);
      B[i][j] = (((((double)i) * (j + 3)) + 3) / n);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double A[(30 + 0)][(30 + 0)]) {
  int i, j;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "A");
#pragma cetus private(i, j)
#pragma loop name print_array #0
  for (i = 0; i < n; i++) {
#pragma cetus private(j)
#pragma loop name print_array #0 #0
    for (j = 0; j < n; j++) {
      if ((((i * n) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", A[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "A");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_jacobi_2d(int tsteps, int n, double A[(30 + 0)][(30 + 0)],
                             double B[(30 + 0)][(30 + 0)]) {
  int t, i, j;
#pragma scop
#pragma cetus private(i, j, t)
#pragma loop name kernel_jacobi_2d #0
  for (t = 0; t < tsteps; t++) {
#pragma cetus private(j)
#pragma loop name kernel_jacobi_2d #0 #0
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (i = 1; i < (n - 1); i++) {
#pragma cetus private(j)
#pragma loop name kernel_jacobi_2d #0 #0 #0
      for (j = 1; j < (n - 1); j++) {
        B[i][j] =
            (0.2 * ((((A[i][j] + A[i][j - 1]) + A[i][1 + j]) + A[1 + i][j]) +
                    A[i - 1][j]));
      }
    }
#pragma cetus private(j)
#pragma loop name kernel_jacobi_2d #0 #1
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (i = 1; i < (n - 1); i++) {
#pragma cetus private(j)
#pragma loop name kernel_jacobi_2d #0 #1 #0
      for (j = 1; j < (n - 1); j++) {
        A[i][j] =
            (0.2 * ((((B[i][j] + B[i][j - 1]) + B[i][1 + j]) + B[1 + i][j]) +
                    B[i - 1][j]));
      }
    }
  }
#pragma endscop
}

int main(int argc, char **argv) {
  /* Retrieve problem size. */
  int n = 30;
  int tsteps = 20;
  /* Variable declarationallocation. */
  double(*A)[(30 + 0)][(30 + 0)];
  double(*B)[(30 + 0)][(30 + 0)];
  A = ((double(*)[(30 + 0)][(30 + 0)])
           polybench_alloc_data((30 + 0) * (30 + 0), sizeof(double)));
  ;
  B = ((double(*)[(30 + 0)][(30 + 0)])
           polybench_alloc_data((30 + 0) * (30 + 0), sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(n, *A, *B);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_jacobi_2d(tsteps, n, *A, *B);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *A);
  /* Be clean. */
  free((void *)A);
  ;
  free((void *)B);
  ;
  return 0;
}
