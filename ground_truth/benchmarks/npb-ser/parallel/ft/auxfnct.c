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
#include "randdp.h"
#include <math.h>
#include <stdio.h>
/* --------------------------------------------------------------------- */
/* compute the roots-of-unity array that will be used for subsequent FFTs.  */
/* --------------------------------------------------------------------- */
void CompExp(int n, dcomplex exponent[n]) {
  int m, nu, ku, i, j, ln;
  double t, ti;
  const double pi = 3.141592653589793;
  nu = n;
  m = ilog2(n);
  exponent[0] = (dcomplex){m, 0.0};
  ku = 2;
  ln = 1;
#pragma cetus private(i, j, t, ti)
#pragma loop name CompExp #0
  for (j = 1; j <= m; j++) {
    t = (pi / ln);
#pragma cetus private(i, ti)
#pragma loop name CompExp #0 #0
    for (i = 0; i <= (ln - 1); i++) {
      ti = (i * t);
      exponent[(i + ku) - 1] = (dcomplex){cos(ti), sin(ti)};
    }
    ku = (ku + ln);
    ln = (2 * ln);
  }
}

int ilog2(int n) {
  int nn, lg;
  if (n == 1) {
    return 0;
  }
  lg = 1;
  nn = 2;
  while (nn < n) {
    nn = (nn * 2);
    lg = (lg + 1);
  }
  return lg;
}

/* --------------------------------------------------------------------- */
/* compute a^exponent mod 2^46 */
/* --------------------------------------------------------------------- */
static double ipow46(double a, int exponent) {
  double result, dummy, q, r;
  int n, n2;
  /* --------------------------------------------------------------------- */
  /* Use */
  /*   a^n = a^(n2)*a^(n/2) if n even else */
  /*   a^n = aa^(n-1)       if n odd */
  /* --------------------------------------------------------------------- */
  result = 1;
  if (exponent == 0) {
    return result;
  }
  q = a;
  r = 1;
  n = exponent;
  while (n > 1) {
    n2 = (n / 2);
    if ((n2 * 2) == n) {
      dummy = randlc(&q, q);
      n = n2;
    } else {
      dummy = randlc(&r, q);
      n = (n - 1);
    }
  }
  dummy = randlc(&r, q);
  result = r;
  return result;
}

void CalculateChecksum(dcomplex *csum, int iterN, int d1, int d2, int d3,
                       dcomplex u[d3][d2][(d1 + 1)]) {
  int i, i1, ii, ji, ki;
  dcomplex csum_temp = (dcomplex){0.0, 0.0};
#pragma cetus private(i, i1, ii, ji, ki)
#pragma loop name CalculateChecksum #0
  for (i = 1; i <= 1024; i++) {
    i1 = i;
    ii = (i1 % d1);
    ji = ((3 * i1) % d2);
    ki = ((5 * i1) % d3);
    csum_temp = (dcomplex){csum_temp.real + u[ki][ji][ii].real,
                           csum_temp.imag + u[ki][ji][ii].imag};
  }
  csum_temp = (dcomplex){csum_temp.real / ((double)((d1 * d2) * d3)),
                         csum_temp.imag / ((double)((d1 * d2) * d3))};
  printf(" T =%5d     Checksum =%22.12E%22.12E\n", iterN, csum_temp.real,
         csum_temp.imag);
  (*csum) = csum_temp;
}

void compute_initial_conditions(int d1, int d2, int d3,
                                dcomplex u0[d3][d2][(d1 + 1)]) {
  dcomplex tmp[64];
  double x0, start, an, dummy;
  double RanStarts[64];
  int i, j, k;
  const double seed = 3.14159265E8;
  const double a = 1.220703125E9;
  start = seed;
  /* --------------------------------------------------------------------- */
  /* Jump to the starting element for our first plane. */
  /* --------------------------------------------------------------------- */
  an = ipow46(a, 0);
  dummy = randlc(&start, an);
  an = ipow46(a, (2 * d1) * d2);
  /* --------------------------------------------------------------------- */
  /* Go through by z planes filling in one square at a time. */
  /* --------------------------------------------------------------------- */
  RanStarts[0] = start;
#pragma cetus private(dummy, k)
#pragma loop name compute_initial_conditions #0
  for (k = 1; k < d3; k++) {
    dummy = randlc(&start, an);
    RanStarts[k] = start;
  }
#pragma cetus private(i, j, k)
#pragma loop name compute_initial_conditions #1
  for (k = 0; k < d3; k++) {
    x0 = RanStarts[k];
#pragma cetus private(i, j)
#pragma loop name compute_initial_conditions #1 #0
    for (j = 0; j < d2; j++) {
      vranlc(2 * d1, &x0, a, (double *)tmp);
#pragma loop name compute_initial_conditions #1 #0 #0
#pragma cetus parallel
#pragma omp parallel for
      for (i = 0; i < d1; i++) {
        u0[k][j][i] = tmp[i];
      }
    }
  }
}

void evolve(int nx, int ny, int nz, dcomplex x[nz][ny][(nx + 1)],
            dcomplex y[nz][ny][(nx + 1)], double twiddle[nz][ny][(nx + 1)]) {
  int i, j, k;
#pragma cetus private(j, k)
#pragma loop name evolve #0
#pragma cetus parallel
#pragma omp parallel for private(j, k)
  for (i = 0; i < nz; i++) {
#pragma cetus private(j, k)
#pragma loop name evolve #0 #0
    for (k = 0; k < ny; k++) {
#pragma cetus private(j)
#pragma loop name evolve #0 #0 #0
      for (j = 0; j < nx; j++) {
        y[i][k][j] = (dcomplex){y[i][k][j].real * twiddle[i][k][j],
                                y[i][k][j].imag * twiddle[i][k][j]};
        x[i][k][j] = y[i][k][j];
      }
    }
  }
}
