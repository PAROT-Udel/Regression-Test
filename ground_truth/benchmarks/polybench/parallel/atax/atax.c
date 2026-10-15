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
/* atax.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "atax.h"
/* Array initialization. */
static void init_array(int m, int n, double A[(38 + 0)][(42 + 0)],
                       double x[(42 + 0)]) {
  int i, j;
  double fn;
  fn = ((double)n);
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for
  for (i = 0; i < n; i++) {
    x[i] = (1 + (i / fn));
  }
#pragma cetus private(j)
#pragma loop name init_array #1
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (i = 0; i < m; i++) {
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < n; j++) {
      A[i][j] = (((double)((i + j) % n)) / (5 * m));
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double y[(42 + 0)]) {
  int i;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "y");
#pragma cetus private(i)
#pragma loop name print_array #0
  for (i = 0; i < n; i++) {
    if ((i % 20) == 0) {
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%0.2lf ", y[i]);
  }
  fprintf(stderr, "\nend   dump: %s\n", "y");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_atax(int m, int n, double A[(38 + 0)][(42 + 0)],
                        double x[(42 + 0)], double y[(42 + 0)],
                        double tmp[(38 + 0)]) {
  int i, j;
#pragma scop
#pragma loop name kernel_atax #0
#pragma cetus parallel
#pragma omp parallel for
  for (i = 0; i < n; i++) {
    y[i] = 0;
  }
#pragma cetus parallel
#pragma cetus private(j)
#pragma omp parallel private(j)
  {
    double *reduce = (double *)malloc(n * sizeof(double));
    int reduce_span_0;
    for (reduce_span_0 = 0; reduce_span_0 < n; reduce_span_0++) {
      reduce[reduce_span_0] = 0;
    }
#pragma loop name kernel_atax #1
#pragma cetus for
#pragma omp for
    for (i = 0; i < m; i++) {
      tmp[i] = 0.0;
#pragma cetus private(j)
#pragma loop name kernel_atax #1 #0
      /* #pragma cetus reduction(+: tmp[i])  */
      for (j = 0; j < n; j++) {
        tmp[i] = (tmp[i] + (A[i][j] * x[j]));
      }
#pragma cetus private(j)
#pragma loop name kernel_atax #1 #1
      for (j = 0; j < n; j++) {
        reduce[j] = (reduce[j] + (A[i][j] * tmp[i]));
      }
    }
#pragma cetus critical
#pragma omp critical
    {
      for (reduce_span_0 = 0; reduce_span_0 < n; reduce_span_0++) {
        y[reduce_span_0] += reduce[reduce_span_0];
      }
    }
  }
#pragma endscop
}

int main(int argc, char **argv) {
  /* Retrieve problem size. */
  int m = 38;
  int n = 42;
  /* Variable declarationallocation. */
  double(*A)[(38 + 0)][(42 + 0)];
  double(*x)[(42 + 0)];
  double(*y)[(42 + 0)];
  double(*tmp)[(38 + 0)];
  A = ((double(*)[(38 + 0)][(42 + 0)])
           polybench_alloc_data((38 + 0) * (42 + 0), sizeof(double)));
  ;
  x = ((double(*)[(42 + 0)]) polybench_alloc_data(42 + 0, sizeof(double)));
  ;
  y = ((double(*)[(42 + 0)]) polybench_alloc_data(42 + 0, sizeof(double)));
  ;
  tmp = ((double(*)[(38 + 0)]) polybench_alloc_data(38 + 0, sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(m, n, *A, *x);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_atax(m, n, *A, *x, *y, *tmp);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *y);
  /* Be clean. */
  free((void *)A);
  ;
  free((void *)x);
  ;
  free((void *)y);
  ;
  free((void *)tmp);
  ;
  return 0;
}
