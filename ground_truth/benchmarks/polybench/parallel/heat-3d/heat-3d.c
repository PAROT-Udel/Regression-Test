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
/* heat-3d.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "heat-3d.h"
/* Array initialization. */
static void init_array(int n, double A[(10 + 0)][(10 + 0)][(10 + 0)],
                       double B[(10 + 0)][(10 + 0)][(10 + 0)]) {
  int i, j, k;
#pragma cetus private(i, j, k)
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for private(i, j, k)
  for (i = 0; i < n; i++) {
#pragma cetus private(j, k)
#pragma loop name init_array #0 #0
    for (j = 0; j < n; j++) {
#pragma cetus private(k)
#pragma loop name init_array #0 #0 #0
      for (k = 0; k < n; k++) {
        A[i][j][k] = (B[i][j][k] = ((((double)((i + j) + (n - k))) * 10) / n));
      }
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double A[(10 + 0)][(10 + 0)][(10 + 0)]) {
  int i, j, k;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "A");
#pragma cetus private(i, j, k)
#pragma loop name print_array #0
  for (i = 0; i < n; i++) {
#pragma cetus private(j, k)
#pragma loop name print_array #0 #0
    for (j = 0; j < n; j++) {
#pragma cetus private(k)
#pragma loop name print_array #0 #0 #0
      for (k = 0; k < n; k++) {
        if ((((((i * n) * n) + (j * n)) + k) % 20) == 0) {
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
static void kernel_heat_3d(int tsteps, int n,
                           double A[(10 + 0)][(10 + 0)][(10 + 0)],
                           double B[(10 + 0)][(10 + 0)][(10 + 0)]) {
  int t, i, j, k;
#pragma scop
#pragma cetus private(i, j, k, t)
#pragma loop name kernel_heat_3d #0
#pragma cetus parallel
#pragma omp parallel for private(i, j, k, t)
  for (t = 1; t <= 20; t++) {
#pragma cetus private(i, j, k)
#pragma loop name kernel_heat_3d #0 #0
    for (i = 1; i < (n - 1); i++) {
#pragma cetus private(j, k)
#pragma loop name kernel_heat_3d #0 #0 #0
      for (j = 1; j < (n - 1); j++) {
#pragma cetus private(k)
#pragma loop name kernel_heat_3d #0 #0 #0 #0
        for (k = 1; k < (n - 1); k++) {
          B[i][j][k] =
              ((((0.125 *
                  ((A[i + 1][j][k] - (2.0 * A[i][j][k])) + A[i - 1][j][k])) +
                 (0.125 *
                  ((A[i][j + 1][k] - (2.0 * A[i][j][k])) + A[i][j - 1][k]))) +
                (0.125 *
                 ((A[i][j][k + 1] - (2.0 * A[i][j][k])) + A[i][j][k - 1]))) +
               A[i][j][k]);
        }
      }
    }
#pragma cetus private(i, j, k)
#pragma loop name kernel_heat_3d #0 #1
    for (i = 1; i < (n - 1); i++) {
#pragma cetus private(j, k)
#pragma loop name kernel_heat_3d #0 #1 #0
      for (j = 1; j < (n - 1); j++) {
#pragma cetus private(k)
#pragma loop name kernel_heat_3d #0 #1 #0 #0
        for (k = 1; k < (n - 1); k++) {
          A[i][j][k] =
              ((((0.125 *
                  ((B[i + 1][j][k] - (2.0 * B[i][j][k])) + B[i - 1][j][k])) +
                 (0.125 *
                  ((B[i][j + 1][k] - (2.0 * B[i][j][k])) + B[i][j - 1][k]))) +
                (0.125 *
                 ((B[i][j][k + 1] - (2.0 * B[i][j][k])) + B[i][j][k - 1]))) +
               B[i][j][k]);
        }
      }
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int n = 10;
  int tsteps = 20;
  /* Variable declarationallocation. */
  double(*A)[(10 + 0)][(10 + 0)][(10 + 0)];
  double(*B)[(10 + 0)][(10 + 0)][(10 + 0)];
  A = ((double(*)[(10 + 0)][(10 + 0)][(10 + 0)]) polybench_alloc_data(
      ((10 + 0) * (10 + 0)) * (10 + 0), sizeof(double)));
  ;
  B = ((double(*)[(10 + 0)][(10 + 0)][(10 + 0)]) polybench_alloc_data(
      ((10 + 0) * (10 + 0)) * (10 + 0), sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(n, *A, *B);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_heat_3d(tsteps, n, *A, *B);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *A);
  /* Be clean. */
  free((void*)A);
  ;
  return 0;
}
