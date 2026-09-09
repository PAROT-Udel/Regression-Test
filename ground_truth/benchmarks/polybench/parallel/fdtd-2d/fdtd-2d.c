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
/* fdtd-2d.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "fdtd-2d.h"
/* Array initialization. */
static void init_array(int tmax, int nx, int ny, double ex[(20 + 0)][(30 + 0)],
                       double ey[(20 + 0)][(30 + 0)],
                       double hz[(20 + 0)][(30 + 0)], double _fict_[(20 + 0)]) {
  int i, j;
#pragma cetus private(i)
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for private(i)
  for (i = 0; i < tmax; i++) {
    _fict_[i] = ((double)i);
  }
#pragma cetus private(i, j)
#pragma loop name init_array #1
#pragma cetus parallel
#pragma omp parallel for private(i, j)
  for (i = 0; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < ny; j++) {
      ex[i][j] = ((((double)i) * (j + 1)) / nx);
      ey[i][j] = ((((double)i) * (j + 2)) / ny);
      hz[i][j] = ((((double)i) * (j + 3)) / nx);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int nx, int ny, double ex[(20 + 0)][(30 + 0)],
                        double ey[(20 + 0)][(30 + 0)],
                        double hz[(20 + 0)][(30 + 0)]) {
  int i, j;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "ex");
#pragma cetus private(i, j)
#pragma loop name print_array #0
  for (i = 0; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name print_array #0 #0
    for (j = 0; j < ny; j++) {
      if ((((i * nx) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", ex[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "ex");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "ey");
#pragma cetus private(i, j)
#pragma loop name print_array #1
  for (i = 0; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name print_array #1 #0
    for (j = 0; j < ny; j++) {
      if ((((i * nx) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", ey[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "ey");
  fprintf(stderr, "begin dump: %s", "hz");
#pragma cetus private(i, j)
#pragma loop name print_array #2
  for (i = 0; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name print_array #2 #0
    for (j = 0; j < ny; j++) {
      if ((((i * nx) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2lf ", hz[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "hz");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
static void kernel_fdtd_2d(int tmax, int nx, int ny,
                           double ex[(20 + 0)][(30 + 0)],
                           double ey[(20 + 0)][(30 + 0)],
                           double hz[(20 + 0)][(30 + 0)],
                           double _fict_[(20 + 0)]) {
  int t, i, j;
#pragma scop
#pragma cetus private(i, j, t)
#pragma loop name kernel_fdtd_2d #0
  for (t = 0; t < tmax; t++) {
#pragma cetus private(j)
#pragma loop name kernel_fdtd_2d #0 #0
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (j = 0; j < ny; j++) {
      ey[0][j] = _fict_[t];
    }
#pragma cetus private(i, j)
#pragma loop name kernel_fdtd_2d #0 #1
#pragma cetus parallel
#pragma omp parallel for private(i, j)
    for (i = 1; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name kernel_fdtd_2d #0 #1 #0
      for (j = 0; j < ny; j++) {
        ey[i][j] = (ey[i][j] - (0.5 * (hz[i][j] - hz[i - 1][j])));
      }
    }
#pragma cetus private(i, j)
#pragma loop name kernel_fdtd_2d #0 #2
#pragma cetus parallel
#pragma omp parallel for private(i, j)
    for (i = 0; i < nx; i++) {
#pragma cetus private(j)
#pragma loop name kernel_fdtd_2d #0 #2 #0
      for (j = 1; j < ny; j++) {
        ex[i][j] = (ex[i][j] - (0.5 * (hz[i][j] - hz[i][j - 1])));
      }
    }
#pragma cetus private(i, j)
#pragma loop name kernel_fdtd_2d #0 #3
#pragma cetus parallel
#pragma omp parallel for private(i, j)
    for (i = 0; i < (nx - 1); i++) {
#pragma cetus private(j)
#pragma loop name kernel_fdtd_2d #0 #3 #0
      for (j = 0; j < (ny - 1); j++) {
        hz[i][j] =
            (hz[i][j] -
             (0.7 * (((ex[i][j + 1] - ex[i][j]) + ey[i + 1][j]) - ey[i][j])));
      }
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int tmax = 20;
  int nx = 20;
  int ny = 30;
  /* Variable declarationallocation. */
  double(*ex)[(20 + 0)][(30 + 0)];
  double(*ey)[(20 + 0)][(30 + 0)];
  double(*hz)[(20 + 0)][(30 + 0)];
  double(*_fict_)[(20 + 0)];
  ex = ((double(*)[(20 + 0)][(30 + 0)])
            polybench_alloc_data((20 + 0) * (30 + 0), sizeof(double)));
  ;
  ey = ((double(*)[(20 + 0)][(30 + 0)])
            polybench_alloc_data((20 + 0) * (30 + 0), sizeof(double)));
  ;
  hz = ((double(*)[(20 + 0)][(30 + 0)])
            polybench_alloc_data((20 + 0) * (30 + 0), sizeof(double)));
  ;
  _fict_ = ((double(*)[(20 + 0)]) polybench_alloc_data(20 + 0, sizeof(double)));
  ;
  /* Initialize array(s). */
  init_array(tmax, nx, ny, *ex, *ey, *hz, *_fict_);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_fdtd_2d(tmax, nx, ny, *ex, *ey, *hz, *_fict_);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(nx, ny, *ex, *ey, *hz);
  /* Be clean. */
  free((void*)ex);
  ;
  free((void*)ey);
  ;
  free((void*)hz);
  ;
  free((void*)_fict_);
  ;
  return 0;
}
