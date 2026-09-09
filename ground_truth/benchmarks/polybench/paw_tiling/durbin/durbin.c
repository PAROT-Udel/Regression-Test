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
/* durbin.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "durbin.h"
/* Array initialization. */
static void init_array(int n, double r[(40 + 0)]) {
  int i, j;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    r[i] = ((n + 1) - i);
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double y[(40 + 0)]) {
  int i;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "y");
#pragma loop name print_array #0
#pragma cetus private(i)
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
static void kernel_durbin(int n, double r[(40 + 0)], double y[(40 + 0)]) {
  double z[40];
  double alpha;
  double beta;
  double sum;
  int i, k;
#pragma scop
  y[0] = (-r[0]);
  beta = 1.0;
  alpha = (-r[0]);
#pragma loop name kernel_durbin #0
#pragma cetus private(i, k, sum)
  for (k = 1; k < n; k++) {
    beta = ((1 - (alpha * alpha)) * beta);
    sum = 0.0;
#pragma loop name kernel_durbin #0 #0
#pragma cetus reduction(+ : sum)
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i) reduction(+ : sum)
    for (i = 0; i < k; i++) {
      sum += (r[(k - i) - 1] * y[i]);
    }
    alpha = ((-(r[k] + sum)) / beta);
#pragma loop name kernel_durbin #0 #1
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
    for (i = 0; i < k; i++) {
      z[i] = (y[i] + (alpha * y[(k - i) - 1]));
    }
#pragma loop name kernel_durbin #0 #2
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
    for (i = 0; i < k; i++) {
      y[i] = z[i];
    }
    y[k] = alpha;
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int n = 40;
  /* Variable declarationallocation. */
  double(*r)[(40 + 0)];
  double(*y)[(40 + 0)];
  r = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  y = ((double(*)[(40 + 0)]) polybench_alloc_data(40 + 0, sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(n, *r);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_durbin(n, *r, *y);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *y);
  /* Be clean. */
  free((void*)r);
  ;
  free((void*)y);
  ;
  return 0;
}
