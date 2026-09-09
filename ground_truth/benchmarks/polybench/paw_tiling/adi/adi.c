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
/* adi.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "adi.h"
/* Array initialization. */
static void init_array(int n, double u[(20 + 0)][(20 + 0)]) {
  int i, j;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < n; i++) {
#pragma loop name init_array #0 #0
#pragma cetus private(j)
    for (j = 0; j < n; j++) {
      u[i][j] = (((double)((i + n) - j)) / n);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, double u[(20 + 0)][(20 + 0)]) {
  int i, j;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "u");
#pragma loop name print_array #0
#pragma cetus private(i, j)
  for (i = 0; i < n; i++) {
#pragma loop name print_array #0 #0
#pragma cetus private(j)
    for (j = 0; j < n; j++) {
      if ((((i * n) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", u[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "u");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
/*
Based on a Fortran code fragment from Figure 5 of
 "Automatic Data and Computation Decomposition on Distributed Memory Parallel
Computers"
 * by Peizong Lee and Zvi Meir Kedem, TOPLAS, 2002

*/
static void kernel_adi(int tsteps, int n, double u[(20 + 0)][(20 + 0)],
                       double v[(20 + 0)][(20 + 0)],
                       double p[(20 + 0)][(20 + 0)],
                       double q[(20 + 0)][(20 + 0)]) {
  int t, i, j;
  double DX, DY, DT;
  double B1, B2;
  double mul1, mul2;
  double a, b, c, d, e, f;
#pragma scop
#pragma experimental section start
  DX = (1.0 / ((double)n));
  DY = (1.0 / ((double)n));
  DT = (1.0 / ((double)tsteps));
  B1 = 2.0;
  B2 = 1.0;
  mul1 = ((B1 * DT) / (DX * DX));
  mul2 = ((B2 * DT) / (DY * DY));
  a = ((-mul1) / 2.0);
  b = (1.0 + mul1);
  c = a;
  d = ((-mul2) / 2.0);
  e = (1.0 + mul2);
  f = d;
#pragma loop name kernel_adi #0
#pragma cetus private(i, j, t)
  for (t = 1; t <= tsteps; t++) {
/* Column Sweep */
#pragma loop name kernel_adi #0 #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
    for (i = 1; i < (n - 1); i++) {
      v[0][i] = 1.0;
      p[i][0] = 0.0;
      q[i][0] = v[0][i];
#pragma loop name kernel_adi #0 #0 #0
#pragma cetus private(j)
      for (j = 1; j < (n - 1); j++) {
        p[i][j] = ((-c) / ((a * p[i][j - 1]) + b));
        q[i][j] = ((((((-d) * u[j][i - 1]) + ((1.0 + (2.0 * d)) * u[j][i])) -
                     (f * u[j][i + 1])) -
                    (a * q[i][j - 1])) /
                   ((a * p[i][j - 1]) + b));
      }
      v[n - 1][i] = 1.0;
#pragma loop name kernel_adi #0 #0 #1
#pragma cetus private(j)
      for (j = (n - 2); j >= 1; j--) {
        v[j][i] = ((p[i][j] * v[j + 1][i]) + q[i][j]);
      }
    }
/* Row Sweep */
#pragma cetus private(i, j)
#pragma loop name kernel_adi #0 #1
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
    for (i = 1; i < (n - 1); i++) {
      u[i][0] = 1.0;
      p[i][0] = 0.0;
      q[i][0] = u[i][0];
#pragma loop name kernel_adi #0 #1 #0
#pragma cetus private(j)
      for (j = 1; j < (n - 1); j++) {
        p[i][j] = ((-f) / ((d * p[i][j - 1]) + e));
        q[i][j] = ((((((-a) * v[i - 1][j]) + ((1.0 + (2.0 * a)) * v[i][j])) -
                     (c * v[i + 1][j])) -
                    (d * q[i][j - 1])) /
                   ((d * p[i][j - 1]) + e));
      }
      u[i][n - 1] = 1.0;
#pragma loop name kernel_adi #0 #1 #1
#pragma cetus private(j)
      for (j = (n - 2); j >= 1; j--) {
        u[i][j] = ((p[i][j] * u[i][j + 1]) + q[i][j]);
      }
    }
  }
#pragma experimental section stop
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int n = 20;
  int tsteps = 20;
  /* Variable declarationallocation. */
  double(*u)[(20 + 0)][(20 + 0)];
  double(*v)[(20 + 0)][(20 + 0)];
  double(*p)[(20 + 0)][(20 + 0)];
  double(*q)[(20 + 0)][(20 + 0)];
  u = ((double(*)[(20 + 0)][(20 + 0)])
           polybench_alloc_data((20 + 0) * (20 + 0), sizeof(double)));
  ;
  v = ((double(*)[(20 + 0)][(20 + 0)])
           polybench_alloc_data((20 + 0) * (20 + 0), sizeof(double)));
  ;
  p = ((double(*)[(20 + 0)][(20 + 0)])
           polybench_alloc_data((20 + 0) * (20 + 0), sizeof(double)));
  ;
  q = ((double(*)[(20 + 0)][(20 + 0)])
           polybench_alloc_data((20 + 0) * (20 + 0), sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(n, *u);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_adi(tsteps, n, *u, *v, *p, *q);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *u);
  /* Be clean. */
  free((void*)u);
  ;
  free((void*)v);
  ;
  free((void*)p);
  ;
  free((void*)q);
  ;
  return 0;
}
