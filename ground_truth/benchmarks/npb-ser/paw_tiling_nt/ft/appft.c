#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CEIL(x) ((int)((x) + 0.5))
#define FLOOR(x) ((int)((x)-0.5))
#define ROUND(x) ((int)((x) + 0.5))

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
/* ------------------------------------------------------------------------- */
/*                                                                          */
/*  This benchmark is a serial C version of the NPB FT code. This C         */
/*  version is developed by the Center for Manycore Programming at Seoul    */
/*  National University and derived from the serial Fortran versions in     */
/*  "NPB3.3-SER" developed by NAS.                                          */
/*                                                                          */
/*  Permission to use, copy, distribute and modify this software for any    */
/*  purpose with or without fee is hereby granted. This software is         */
/*  provided "as is" without express or implied warranty.                   */
/*                                                                          */
/*  Information on NPB 3.3, including the technical report, the original    */
/*  specifications, source code, results and information on how to submit   */
/*  new results, is available at:                                           */
/*                                                                          */
/*           http:www.nas.nasa.govSoftware/NPB/                          */
/*                                                                          */
/*  Send comments or suggestions for this C version to cmp@aces.snu.ac.kr   */
/*                                                                          */
/*          Center for Manycore Programming                                 */
/*          School of Computer Science and Engineering                      */
/*          Seoul National University                                       */
/*          Seoul 151-744, Korea                                            */
/*                                                                          */
/*          E-mail:  cmp@aces.snu.ac.kr                                     */
/*                                                                          */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* Authors: Sangmin Seo, Jungwon Kim, Jun Lee, Jeongho Nah, Gangwon Jo,     */
/*          and Jaejin Lee                                                  */
/* ------------------------------------------------------------------------- */
#include "global.h"
#include "timers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/* for checksum data */
/* commonsumcomm */
static dcomplex sums[(6 + 1)];
/* commonmainarrays */
static double twiddle[64][64][(64 + 1)];
static dcomplex xnt[64][64][(64 + 1)];
static dcomplex y[64][64][(64 + 1)];
/* static dcomplex pad1[128], pad2[128]; */
void appft(int niter, double *total_time, logical *verified) {
  int i, j, k, kt, n12, n22, n32, ii, jj, kk, ii2, ik2;
  double ap;
  dcomplex exp1[64], exp2[64], exp3[64];
#pragma loop name appft #0
#pragma cetus private(i)
  for (i = 1; i <= 15; i++) {
    timer_clear(i);
  }
  timer_start(2);
  compute_initial_conditions(64, 64, 64, xnt);
  CompExp(64, exp1);
  CompExp(64, exp2);
  CompExp(64, exp3);
  fftXYZ(1, 64, 64, 64, xnt, (dcomplex *)y, exp1, exp2, exp3);
  timer_stop(2);
  timer_start(1);
  if (timers_enabled) {
    timer_start(13);
  }
  n12 = (64 / 2);
  n22 = (64 / 2);
  n32 = (64 / 2);
  ap = (((-4.0) * 1.0E-6) * (3.141592653589793 * 3.141592653589793));
#pragma loop name appft #1
#pragma cetus parallel
#pragma cetus private(i, ii, ii2, ik2, j, jj, k, kk)
#pragma omp parallel for private(i, ii, ii2, ik2, j, jj, k, kk)
  for (i = 0; i < 64; i++) {
    ii = (i - ((i / n32) * 64));
    ii2 = (ii * ii);
#pragma loop name appft #1 #0
#pragma cetus private(ik2, j, jj, k, kk)
    for (k = 0; k < 64; k++) {
      kk = (k - ((k / n22) * 64));
      ik2 = (ii2 + (kk * kk));
#pragma loop name appft #1 #0 #0
#pragma cetus private(j, jj)
      for (j = 0; j < 64; j++) {
        jj = (j - ((j / n12) * 64));
        twiddle[i][k][j] = exp(ap * ((double)((jj * jj) + ik2)));
      }
    }
  }
  if (timers_enabled) {
    timer_stop(13);
  }
  if (timers_enabled) {
    timer_start(12);
  }
  compute_initial_conditions(64, 64, 64, xnt);
  if (timers_enabled) {
    timer_stop(12);
  }
  if (timers_enabled) {
    timer_start(15);
  }
  fftXYZ(1, 64, 64, 64, xnt, (dcomplex *)y, exp1, exp2, exp3);
  if (timers_enabled) {
    timer_stop(15);
  }
#pragma loop name appft #2
#pragma cetus private(kt)
  for (kt = 1; kt <= niter; kt++) {
    if (timers_enabled) {
      timer_start(11);
    }
    evolve(64, 64, 64, xnt, y, twiddle);
    if (timers_enabled) {
      timer_stop(11);
    }
    if (timers_enabled) {
      timer_start(15);
    }
    fftXYZ(-1, 64, 64, 64, xnt, (dcomplex *)xnt, exp1, exp2, exp3);
    if (timers_enabled) {
      timer_stop(15);
    }
    if (timers_enabled) {
      timer_start(10);
    }
    CalculateChecksum(&sums[kt], kt, 64, 64, 64, xnt);
    if (timers_enabled) {
      timer_stop(10);
    }
  }
  /* Verification test. */
  if (timers_enabled) {
    timer_start(14);
  }
  verify(64, 64, 64, niter, sums, verified);
  if (timers_enabled) {
    timer_stop(14);
  }
  timer_stop(1);
  (*total_time) = timer_read(1);
  if (!timers_enabled) {
    return;
  }
  printf(" FT subroutine timers \n");
  printf(" %26s =%9.4f\n", "FT total                  ", timer_read(1));
  printf(" %26s =%9.4f\n", "WarmUp time               ", timer_read(2));
  printf(" %26s =%9.4f\n", "fftXYZ body               ", timer_read(3));
  printf(" %26s =%9.4f\n", "Swarztrauber              ", timer_read(4));
  printf(" %26s =%9.4f\n", "X time                    ", timer_read(7));
  printf(" %26s =%9.4f\n", "Y time                    ", timer_read(8));
  printf(" %26s =%9.4f\n", "Z time                    ", timer_read(9));
  printf(" %26s =%9.4f\n", "CalculateChecksum         ", timer_read(10));
  printf(" %26s =%9.4f\n", "evolve                    ", timer_read(11));
  printf(" %26s =%9.4f\n", "compute_initial_conditions", timer_read(12));
  printf(" %26s =%9.4f\n", "twiddle                   ", timer_read(13));
  printf(" %26s =%9.4f\n", "verify                    ", timer_read(14));
  printf(" %26s =%9.4f\n", "fftXYZ                    ", timer_read(15));
  printf(" %26s =%9.4f\n", "Benchmark time            ", *total_time);
}
