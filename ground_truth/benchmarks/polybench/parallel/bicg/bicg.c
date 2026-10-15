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
/* bicg.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "bicg.h"
/* Array initialization. */
static void init_array(int m, int n, double A[(42 + 0)][(38 + 0)],
                       double r[(42 + 0)], double p[(38 + 0)]) {
  int i, j;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for
  for (i = 0; i < m; i++) {
    p[i] = (((double)(i % m)) / m);
  }
#pragma cetus private(j)
#pragma loop name init_array #1
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (i = 0; i < n; i++) {
    r[i] = (((double)(i % n)) / n);
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < m; j++) {
      A[i][j] = (((double)((i * (j + 1)) % n)) / n);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int m, int n, double s[(38 + 0)], double q[(42 + 0)]) {
  int i;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "s");
#pragma cetus private(i)
#pragma loop name print_array #0
  for (i = 0; i < m; i++) {
    if ((i % 20) == 0) {
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%0.2lf ", s[i]);
  }
  fprintf(stderr, "\nend   dump: %s\n", "s");
  fprintf(stderr, "begin dump: %s", "q");
#pragma cetus private(i)
#pragma loop name print_array #1
  for (i = 0; i < n; i++) {
    if ((i % 20) == 0) {
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%0.2lf ", q[i]);
  }
  fprintf(stderr, "\nend   dump: %s\n", "q");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_bicg(int m, int n, double A[(42 + 0)][(38 + 0)],
                        double s[(38 + 0)], double q[(42 + 0)],
                        double p[(38 + 0)], double r[(42 + 0)]) {
  int i, j;
#pragma scop
#pragma loop name kernel_bicg #0
#pragma cetus parallel
#pragma omp parallel for
  for (i = 0; i < m; i++) {
    s[i] = 0;
  }
#pragma cetus parallel
#pragma cetus private(j)
#pragma omp parallel private(j)
  {
    double *reduce = (double *)malloc(m * sizeof(double));
    int reduce_span_0;
    for (reduce_span_0 = 0; reduce_span_0 < m; reduce_span_0++) {
      reduce[reduce_span_0] = 0;
    }
#pragma loop name kernel_bicg #1
#pragma cetus for
#pragma omp for
    for (i = 0; i < n; i++) {
      q[i] = 0.0;
#pragma cetus private(j)
#pragma loop name kernel_bicg #1 #0
      /* #pragma cetus reduction(+: q[i])  */
      for (j = 0; j < m; j++) {
        reduce[j] = (reduce[j] + (r[i] * A[i][j]));
        q[i] = (q[i] + (A[i][j] * p[j]));
      }
    }
#pragma cetus critical
#pragma omp critical
    {
      for (reduce_span_0 = 0; reduce_span_0 < m; reduce_span_0++) {
        s[reduce_span_0] += reduce[reduce_span_0];
      }
    }
  }
#pragma endscop
}

int main(int argc, char **argv) {
  /* Retrieve problem size. */
  int n = 42;
  int m = 38;
  /* Variable declarationallocation. */
  double(*A)[(42 + 0)][(38 + 0)];
  double(*s)[(38 + 0)];
  double(*q)[(42 + 0)];
  double(*p)[(38 + 0)];
  double(*r)[(42 + 0)];
  A = ((double(*)[(42 + 0)][(38 + 0)])
           polybench_alloc_data((42 + 0) * (38 + 0), sizeof(double)));
  ;
  s = ((double(*)[(38 + 0)]) polybench_alloc_data(38 + 0, sizeof(double)));
  ;
  q = ((double(*)[(42 + 0)]) polybench_alloc_data(42 + 0, sizeof(double)));
  ;
  p = ((double(*)[(38 + 0)]) polybench_alloc_data(38 + 0, sizeof(double)));
  ;
  r = ((double(*)[(42 + 0)]) polybench_alloc_data(42 + 0, sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(m, n, *A, *r, *p);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_bicg(m, n, *A, *s, *q, *p, *r);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(m, n, *s, *q);
  /* Be clean. */
  free((void *)A);
  ;
  free((void *)s);
  ;
  free((void *)q);
  ;
  free((void *)p);
  ;
  free((void *)r);
  ;
  return 0;
}
