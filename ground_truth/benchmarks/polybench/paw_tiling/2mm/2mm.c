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
/* 2mm.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include "polybench.h"
/* Include benchmark-specific header. */
#include "2mm.h"
/* Array initialization. */
static void init_array(int ni, int nj, int nk, int nl, double* alpha,
                       double* beta, double A[(16 + 0)][(22 + 0)],
                       double B[(22 + 0)][(18 + 0)],
                       double C[(18 + 0)][(24 + 0)],
                       double D[(16 + 0)][(24 + 0)]) {
  int i, j;
  (*alpha) = 1.5;
  (*beta) = 1.2;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < ni; i++) {
#pragma loop name init_array #0 #0
#pragma cetus private(j)
    for (j = 0; j < nk; j++) {
      A[i][j] = (((double)(((i * j) + 1) % ni)) / ni);
    }
  }
#pragma loop name init_array #1
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < nk; i++) {
#pragma loop name init_array #1 #0
#pragma cetus private(j)
    for (j = 0; j < nj; j++) {
      B[i][j] = (((double)((i * (j + 1)) % nj)) / nj);
    }
  }
#pragma loop name init_array #2
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < nj; i++) {
#pragma loop name init_array #2 #0
#pragma cetus private(j)
    for (j = 0; j < nl; j++) {
      C[i][j] = (((double)(((i * (j + 3)) + 1) % nl)) / nl);
    }
  }
#pragma loop name init_array #3
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < ni; i++) {
#pragma loop name init_array #3 #0
#pragma cetus private(j)
    for (j = 0; j < nl; j++) {
      D[i][j] = (((double)((i * (j + 2)) % nk)) / nk);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int ni, int nl, double D[(16 + 0)][(24 + 0)]) {
  int i, j;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "D");
#pragma loop name print_array #0
#pragma cetus private(i, j)
  for (i = 0; i < ni; i++) {
#pragma loop name print_array #0 #0
#pragma cetus private(j)
    for (j = 0; j < nl; j++) {
      if ((((i * ni) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", D[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "D");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_2mm(int ni, int nj, int nk, int nl, double alpha,
                       double beta, double tmp[(16 + 0)][(18 + 0)],
                       double A[(16 + 0)][(22 + 0)],
                       double B[(22 + 0)][(18 + 0)],
                       double C[(18 + 0)][(24 + 0)],
                       double D[(16 + 0)][(24 + 0)]) {
  int i, j, k;
#pragma scop
#pragma experimental section start
/* D := alphaA*B*C + beta*D */
#pragma cetus firstprivate(tmp)
#pragma cetus lastprivate(tmp)
#pragma loop name kernel_2mm #0
#pragma cetus parallel
#pragma cetus firstprivate(tmp)
#pragma cetus private(i, j, k)
#pragma cetus lastprivate(tmp)
#pragma omp parallel for private(i, j, k) firstprivate(tmp) lastprivate(tmp)
  for (i = 0; i < ni; i++) {
#pragma cetus firstprivate(tmp)
#pragma cetus lastprivate(tmp)
#pragma loop name kernel_2mm #0 #0
#pragma cetus firstprivate(tmp)
#pragma cetus private(j, k)
#pragma cetus lastprivate(tmp)
    for (j = 0; j < nj; j++) {
      tmp[i][j] = 0.0;
#pragma loop name kernel_2mm #0 #0 #0
/* #pragma cetus reduction(+: tmp[i][j])  */
#pragma cetus private(k)
      for (k = 0; k < nk; ++k) {
        tmp[i][j] += ((alpha * A[i][k]) * B[k][j]);
      }
    }
  }
#pragma loop name kernel_2mm #1
#pragma cetus parallel
#pragma cetus private(i, j, k)
#pragma omp parallel for private(i, j, k)
  for (i = 0; i < ni; i++) {
#pragma loop name kernel_2mm #1 #0
#pragma cetus private(j, k)
    for (j = 0; j < nl; j++) {
      D[i][j] *= beta;
#pragma loop name kernel_2mm #1 #0 #0
/* #pragma cetus reduction(+: D[i][j])  */
#pragma cetus private(k)
      for (k = 0; k < nj; ++k) {
        D[i][j] += (tmp[i][k] * C[k][j]);
      }
    }
  }
#pragma experimental section stop
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int ni = 16;
  int nj = 18;
  int nk = 22;
  int nl = 24;
  /* Variable declarationallocation. */
  double alpha;
  double beta;
  double(*tmp)[(16 + 0)][(18 + 0)];
  double(*A)[(16 + 0)][(22 + 0)];
  double(*B)[(22 + 0)][(18 + 0)];
  double(*C)[(18 + 0)][(24 + 0)];
  double(*D)[(16 + 0)][(24 + 0)];
  tmp = ((double(*)[(16 + 0)][(18 + 0)])
             polybench_alloc_data((16 + 0) * (18 + 0), sizeof(double)));
  ;
  A = ((double(*)[(16 + 0)][(22 + 0)])
           polybench_alloc_data((16 + 0) * (22 + 0), sizeof(double)));
  ;
  B = ((double(*)[(22 + 0)][(18 + 0)])
           polybench_alloc_data((22 + 0) * (18 + 0), sizeof(double)));
  ;
  C = ((double(*)[(18 + 0)][(24 + 0)])
           polybench_alloc_data((18 + 0) * (24 + 0), sizeof(double)));
  ;
  D = ((double(*)[(16 + 0)][(24 + 0)])
           polybench_alloc_data((16 + 0) * (24 + 0), sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(ni, nj, nk, nl, &alpha, &beta, *A, *B, *C, *D);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_2mm(ni, nj, nk, nl, alpha, beta, *tmp, *A, *B, *C, *D);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(ni, nl, *D);
  /* Be clean. */
  free((void*)tmp);
  ;
  free((void*)A);
  ;
  free((void*)B);
  ;
  free((void*)C);
  ;
  free((void*)D);
  ;
  return 0;
}
