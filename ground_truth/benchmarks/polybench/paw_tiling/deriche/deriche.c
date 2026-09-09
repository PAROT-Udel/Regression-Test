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
/* deriche.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "deriche.h"
/* Array initialization. */
static void init_array(int w, int h, float* alpha,
                       float imgIn[(64 + 0)][(64 + 0)],
                       float imgOut[(64 + 0)][(64 + 0)]) {
  int i, j;
  (*alpha) = 0.25;
/* parameter of the filter */
/* input should be between 0 and 1 (grayscale image pixel) */
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < w; i++) {
#pragma loop name init_array #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < h; j++) {
      imgIn[i][j] = (((float)(((313 * i) + (991 * j)) % 65536)) / 65535.0F);
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int w, int h, float imgOut[(64 + 0)][(64 + 0)]) {
  int i, j;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "imgOut");
#pragma loop name print_array #0
#pragma cetus private(i, j)
#pragma cetus private(i, j)
  for (i = 0; i < w; i++) {
#pragma loop name print_array #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < h; j++) {
      if ((((i * h) + j) % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%0.2f ", imgOut[i][j]);
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "imgOut");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
/* Original code provided by Gael Deest */
static void kernel_deriche(int w, int h, float alpha,
                           float imgIn[(64 + 0)][(64 + 0)],
                           float imgOut[(64 + 0)][(64 + 0)],
                           float y1[(64 + 0)][(64 + 0)],
                           float y2[(64 + 0)][(64 + 0)]) {
  int i, j;
  float xm1, tm1, ym1, ym2;
  float xp1, xp2;
  float tp1, tp2;
  float yp1, yp2;
  float k;
  float a1, a2, a3, a4, a5, a6, a7, a8;
  float b1, b2, c1, c2;
#pragma scop
  k = (((1.0F - expf(-alpha)) * (1.0F - expf(-alpha))) /
       ((1.0F + ((2.0F * alpha) * expf(-alpha))) - expf(2.0F * alpha)));
  a1 = (a5 = k);
  a2 = (a6 = ((k * expf(-alpha)) * (alpha - 1.0F)));
  a3 = (a7 = ((k * expf(-alpha)) * (alpha + 1.0F)));
  a4 = (a8 = ((-k) * expf((-2.0F) * alpha)));
  b1 = powf(2.0F, -alpha);
  b2 = (-expf((-2.0F) * alpha));
  c1 = (c2 = 1);
#pragma loop name kernel_deriche #0
#pragma cetus parallel
#pragma cetus private(i, j, xm1, ym1, ym2)
#pragma cetus private(i, j, xm1, ym1, ym2)
#pragma omp parallel for private(i, j, xm1, ym1, ym2)
  for (i = 0; i < w; i++) {
    ym1 = 0.0F;
    ym2 = 0.0F;
    xm1 = 0.0F;
#pragma loop name kernel_deriche #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < h; j++) {
      y1[i][j] =
          ((((a1 * imgIn[i][j]) + (a2 * xm1)) + (b1 * ym1)) + (b2 * ym2));
      xm1 = imgIn[i][j];
      ym2 = ym1;
      ym1 = y1[i][j];
    }
  }
#pragma loop name kernel_deriche #1
#pragma cetus parallel
#pragma cetus private(i, j, xp1, xp2, yp1, yp2)
#pragma cetus private(i, j, xp1, xp2, yp1, yp2)
#pragma omp parallel for private(i, j, xp1, xp2, yp1, yp2)
  for (i = 0; i < w; i++) {
    yp1 = 0.0F;
    yp2 = 0.0F;
    xp1 = 0.0F;
    xp2 = 0.0F;
#pragma loop name kernel_deriche #1 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = (h - 1); j >= 0; j--) {
      y2[i][j] = ((((a3 * xp1) + (a4 * xp2)) + (b1 * yp1)) + (b2 * yp2));
      xp2 = xp1;
      xp1 = imgIn[i][j];
      yp2 = yp1;
      yp1 = y2[i][j];
    }
  }
#pragma cetus private(i, j)
#pragma loop name kernel_deriche #2
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < w; i++) {
#pragma loop name kernel_deriche #2 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < h; j++) {
      imgOut[i][j] = (c1 * (y1[i][j] + y2[i][j]));
    }
  }
#pragma loop name kernel_deriche #3
#pragma cetus parallel
#pragma cetus private(i, j, tm1, ym1, ym2)
#pragma cetus private(i, j, tm1, ym1, ym2)
#pragma omp parallel for private(i, j, tm1, ym1, ym2)
  for (j = 0; j < h; j++) {
    tm1 = 0.0F;
    ym1 = 0.0F;
    ym2 = 0.0F;
#pragma loop name kernel_deriche #3 #0
#pragma cetus private(i)
#pragma cetus private(i)
    for (i = 0; i < w; i++) {
      y1[i][j] =
          ((((a5 * imgOut[i][j]) + (a6 * tm1)) + (b1 * ym1)) + (b2 * ym2));
      tm1 = imgOut[i][j];
      ym2 = ym1;
      ym1 = y1[i][j];
    }
  }
#pragma loop name kernel_deriche #4
#pragma cetus parallel
#pragma cetus private(i, j, tp1, tp2, yp1, yp2)
#pragma cetus private(i, j, tp1, tp2, yp1, yp2)
#pragma omp parallel for private(i, j, tp1, tp2, yp1, yp2)
  for (j = 0; j < h; j++) {
    tp1 = 0.0F;
    tp2 = 0.0F;
    yp1 = 0.0F;
    yp2 = 0.0F;
#pragma loop name kernel_deriche #4 #0
#pragma cetus private(i)
#pragma cetus private(i)
    for (i = (w - 1); i >= 0; i--) {
      y2[i][j] = ((((a7 * tp1) + (a8 * tp2)) + (b1 * yp1)) + (b2 * yp2));
      tp2 = tp1;
      tp1 = imgOut[i][j];
      yp2 = yp1;
      yp1 = y2[i][j];
    }
  }
#pragma cetus private(i, j)
#pragma loop name kernel_deriche #5
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma omp parallel for private(i, j)
  for (i = 0; i < w; i++) {
#pragma loop name kernel_deriche #5 #0
#pragma cetus private(j)
#pragma cetus private(j)
    for (j = 0; j < h; j++) {
      imgOut[i][j] = (c2 * (y1[i][j] + y2[i][j]));
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int w = 64;
  int h = 64;
  /* Variable declarationallocation. */
  float alpha;
  float(*imgIn)[(64 + 0)][(64 + 0)];
  float(*imgOut)[(64 + 0)][(64 + 0)];
  float(*y1)[(64 + 0)][(64 + 0)];
  float(*y2)[(64 + 0)][(64 + 0)];
  imgIn = ((float(*)[(64 + 0)][(64 + 0)])
               polybench_alloc_data((64 + 0) * (64 + 0), sizeof(float)));
  ;
  imgOut = ((float(*)[(64 + 0)][(64 + 0)])
                polybench_alloc_data((64 + 0) * (64 + 0), sizeof(float)));
  ;
  y1 = ((float(*)[(64 + 0)][(64 + 0)])
            polybench_alloc_data((64 + 0) * (64 + 0), sizeof(float)));
  ;
  y2 = ((float(*)[(64 + 0)][(64 + 0)])
            polybench_alloc_data((64 + 0) * (64 + 0), sizeof(float)));
  ;
  /* Initialize array(s). */
  init_array(w, h, &alpha, *imgIn, *imgOut);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_deriche(w, h, alpha, *imgIn, *imgOut, *y1, *y2);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(w, h, *imgOut);
  /* Be clean. */
  free((void*)imgIn);
  ;
  free((void*)imgOut);
  ;
  free((void*)y1);
  ;
  free((void*)y2);
  ;
  return 0;
}
