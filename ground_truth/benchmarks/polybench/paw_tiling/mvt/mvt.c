#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CEIL(x) ((int)((x) + 0.5))
#define FLOOR(x) ((int)((x) - 0.5))
#define ROUND(x) ((int)((x) + 0.5))

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
/* mvt.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "mvt.h"
/* Array initialization. */
static void init_array(int n, double x1[(40 + 0)], double x2[(40 + 0)],
                       double y_1[(40 + 0)], double y_2[(40 + 0)],
                       double A[(40 + 0)][(40 + 0)]) {
  int i, j;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < n; i++) {
    x1[i] = (((double)(i % n)) / n);
    x2[i] = (((double)((i + 1) % n)) / n);
    y_1[i] = (((double)((i + 3) % n)) / n);
    y_2[i] = (((double)((i + 4) % n)) / n);
#pragma loop name init_array #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < n; j++) {
      A[i][j] = (((double)((i * j) % n)) / n);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double x1[(40 + 0)], double x2[(40 + 0)]) {
  int i;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "x1");
#pragma loop name print_array #0
#pragma cetus private(i)
#pragma cetus private(i)
  for (i = 0; i < n; i++) {
    if ((i % 20) == 0) {
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%0.2lf ", x1[i]);
  }
  fprintf(stderr, "\nend   dump: %s\n", "x1");
  fprintf(stderr, "begin dump: %s", "x2");
#pragma loop name print_array #1
#pragma cetus private(i)
#pragma cetus private(i)
  for (i = 0; i < n; i++) {
    if ((i % 20) == 0) {
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%0.2lf ", x2[i]);
  }
  fprintf(stderr, "\nend   dump: %s\n", "x2");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_mvt(int n, double x1[(40 + 0)], double x2[(40 + 0)],
                       double y_1[(40 + 0)], double y_2[(40 + 0)],
                       double A[(40 + 0)][(40 + 0)]) {
  int i, j;
#pragma scop
#pragma loop name kernel_mvt #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < n; i++) {
#pragma loop name kernel_mvt #0 #0
/* #pragma cetus reduction(+: x1[i])  */
#pragma cetus private(j)
#pragma cetus private(j)
#pragma cetus reduction(+ : x1[i])
    for (j = 0; j < n; j++) {
      x1[i] = (x1[i] + (A[i][j] * y_1[j]));
    }
  }
#pragma loop name kernel_mvt #1
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < n; i++) {
#pragma loop name kernel_mvt #1 #0
/* #pragma cetus reduction(+: x2[i])  */
#pragma cetus private(j)
#pragma cetus private(j)
#pragma cetus reduction(+ : x2[i])
    for (j = 0; j < n; j++) {
      x2[i] = (x2[i] + (A[j][i] * y_2[j]));
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int n = 40;
  /* Variable declarationallocation. */
  double(*A)[(40 + 0)][(40 + 0)];
  double(*x1)[(40 + 0)];
  double(*x2)[(40 + 0)];
  double(*y_1)[(40 + 0)];
  double(*y_2)[(40 + 0)];
  A = ((double(*)[(40 + 0)][(40 + 0)])
           polybench_alloc_data((40 + 0) * (40 + 0), sizeof(double)));
  ;
  x1 = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  x2 = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  y_1 = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  y_2 = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(n, *x1, *x2, *y_1, *y_2, *A);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_mvt(n, *x1, *x2, *y_1, *y_2, *A);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *x1, *x2);
  /* Be clean. */
  free((void*)A);
  ;
  free((void*)x1);
  ;
  free((void*)x2);
  ;
  free((void*)y_1);
  ;
  free((void*)y_2);
  ;
  return 0;
}
