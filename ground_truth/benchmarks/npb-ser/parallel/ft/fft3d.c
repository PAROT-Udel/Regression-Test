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
/* commonblockinfo */
static int fftblock;
/* static int fftblockpad; */
/* commonworkarr */
static dcomplex plane[((32 + 1) * 64)];
/* static dcomplex pad[128]; */
static dcomplex scr[64][(32 + 1)];
/* --------------------------------------------------------------------- */
/* Computes NY N-point complex-to-complex FFTs of X using an algorithm due */
/* to Swarztrauber.  X is both the input and the output array, while Y is a  */
/* scratch array.  It is assumed that N = 2^M.  Before calling  */
/* Swarztrauber to  */
/* perform FFTs */
/* --------------------------------------------------------------------- */
static void Swarztrauber(int is, int m, int vlen, int n, int xd1, void *ox,
                         dcomplex exponent[n]) {
  dcomplex(*x)[xd1] = (dcomplex(*)[xd1])ox;
  int i, j, l;
  dcomplex u1, x11, x21;
  int k, n1, li, lj, lk, ku, i11, i12, i21, i22;
  if (timers_enabled) {
    timer_start(4);
  }
  /* --------------------------------------------------------------------- */
  /* Perform one variant of the Stockham FFT. */
  /* --------------------------------------------------------------------- */
  n1 = (n / 2);
  lj = 1;
  li = (1 << m);
#pragma cetus private(i, i11, i12, i21, i22, j, k, ku, l, lk)
#pragma loop name Swarztrauber #0
  for (l = 1; l <= m; l += 2) {
    lk = lj;
    lj = (2 * lk);
    li = (li / 2);
    ku = li;
#pragma cetus private(i, i11, i12, i21, i22, j, k)
#pragma loop name Swarztrauber #0 #0
    for (i = 0; i <= (li - 1); i++) {
      i11 = (i * lk);
      i12 = (i11 + n1);
      i21 = (i * lj);
      i22 = (i21 + lk);
      if (is >= 1) {
        u1 = exponent[ku + i];
      } else {
        u1 = (dcomplex){exponent[ku + i].real, (-1.0) * exponent[ku + i].imag};
      }
#pragma cetus private(j, k)
#pragma loop name Swarztrauber #0 #0 #0
      for (k = 0; k <= (lk - 1); k++) {
#pragma cetus private(j)
#pragma loop name Swarztrauber #0 #0 #0 #0
        for (j = 0; j < vlen; j++) {
          x11 = x[i11 + k][j];
          x21 = x[i12 + k][j];
          scr[i21 + k][j] =
              (dcomplex){x11.real + x21.real, x11.imag + x21.imag};
          scr[i22 + k][j] = (dcomplex){
              (u1.real *
               (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.real) -
                  (u1.imag *
                   (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.imag),
              (u1.real *
               (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.imag) +
                  (u1.imag *
                   (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.real)};
        }
      }
    }
    if (l == m) {
#pragma cetus private(j)
#pragma loop name Swarztrauber #0 #1
#pragma cetus parallel
#pragma omp parallel for private(j)
      for (k = 0; k < n; k++) {
#pragma cetus private(j)
#pragma loop name Swarztrauber #0 #1 #0
        for (j = 0; j < vlen; j++) {
          x[k][j] = scr[k][j];
        }
      }
    } else {
      lk = lj;
      lj = (2 * lk);
      li = (li / 2);
      ku = li;
#pragma cetus private(i, i11, i12, i21, i22, j, k)
#pragma loop name Swarztrauber #0 #2
      for (i = 0; i <= (li - 1); i++) {
        i11 = (i * lk);
        i12 = (i11 + n1);
        i21 = (i * lj);
        i22 = (i21 + lk);
        if (is >= 1) {
          u1 = exponent[ku + i];
        } else {
          u1 =
              (dcomplex){exponent[ku + i].real, (-1.0) * exponent[ku + i].imag};
        }
#pragma cetus private(j, k)
#pragma loop name Swarztrauber #0 #2 #0
        for (k = 0; k <= (lk - 1); k++) {
#pragma cetus private(j)
#pragma loop name Swarztrauber #0 #2 #0 #0
          for (j = 0; j < vlen; j++) {
            x11 = scr[i11 + k][j];
            x21 = scr[i12 + k][j];
            x[i21 + k][j] =
                (dcomplex){x11.real + x21.real, x11.imag + x21.imag};
            x[i22 + k][j] = (dcomplex){
                (u1.real *
                 (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.real) -
                    (u1.imag *
                     (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.imag),
                (u1.real *
                 (dcomplex){x11.real - x21.real, x11.imag - x21.imag}.imag) +
                    (u1.imag *
                     (dcomplex){x11.real - x21.real, x11.imag - x21.imag}
                         .real)};
          }
        }
      }
    }
  }
  if (timers_enabled) {
    timer_stop(4);
  }
}

void fftXYZ(int sign, int n1, int n2, int n3, dcomplex x[n3][n2][(n1 + 1)],
            dcomplex xout[(((n1 + 1) * n2) * n3)], dcomplex exp1[n1],
            dcomplex exp2[n2], dcomplex exp3[n3]) {
  int i, j, k, log;
  int bls, ble;
  int len;
  int blkp;
  if (timers_enabled) {
    timer_start(3);
  }
  fftblock = (8192 / n1);
  if (fftblock >= 32) {
    fftblock = 32;
  }
  blkp = (fftblock + 1);
  log = ilog2(n1);
  if (timers_enabled) {
    timer_start(7);
  }
#pragma cetus private(ble, bls, i, j, k, len)
#pragma loop name fftXYZ #0
  for (k = 0; k < n3; k++) {
#pragma cetus private(ble, bls, i, j, len)
#pragma loop name fftXYZ #0 #0
    for (bls = 0; bls < n2; bls += fftblock) {
      ble = ((bls + fftblock) - 1);
      if (ble > n2) {
        ble = (n2 - 1);
      }
      len = ((ble - bls) + 1);
#pragma cetus private(i, j)
#pragma loop name fftXYZ #0 #0 #0
      for (j = bls; j <= ble; j++) {
#pragma cetus private(i)
#pragma loop name fftXYZ #0 #0 #0 #0
        for (i = 0; i < n1; i++) {
          plane[(j - bls) + (blkp * i)] = x[k][j][i];
        }
      }
      Swarztrauber(sign, log, len, n1, blkp, plane, exp1);
#pragma cetus private(i)
#pragma loop name fftXYZ #0 #0 #1
#pragma cetus parallel
#pragma omp parallel for private(i)
      for (j = bls; j <= ble; j++) {
#pragma cetus private(i)
#pragma loop name fftXYZ #0 #0 #1 #0
        for (i = 0; i < n1; i++) {
          x[k][j][i] = plane[(j - bls) + (blkp * i)];
        }
      }
    }
  }
  if (timers_enabled) {
    timer_stop(7);
  }
  fftblock = (8192 / n2);
  if (fftblock >= 32) {
    fftblock = 32;
  }
  blkp = (fftblock + 1);
  log = ilog2(n2);
  if (timers_enabled) {
    timer_start(8);
  }
#pragma cetus private(ble, bls, k, len)
#pragma loop name fftXYZ #1
  for (k = 0; k < n3; k++) {
#pragma cetus private(ble, bls, len)
#pragma loop name fftXYZ #1 #0
    for (bls = 0; bls < n1; bls += fftblock) {
      ble = ((bls + fftblock) - 1);
      if (ble > n1) {
        ble = (n1 - 1);
      }
      len = ((ble - bls) + 1);
      Swarztrauber(sign, log, len, n2, n1 + 1, &x[k][0][bls], exp2);
    }
  }
  if (timers_enabled) {
    timer_stop(8);
  }
  fftblock = (8192 / n3);
  if (fftblock >= 32) {
    fftblock = 32;
  }
  blkp = (fftblock + 1);
  log = ilog2(n3);
  if (timers_enabled) {
    timer_start(9);
  }
#pragma cetus private(ble, bls, i, j, k, len)
#pragma loop name fftXYZ #2
  for (k = 0; k < n2; k++) {
#pragma cetus private(ble, bls, i, j, len)
#pragma loop name fftXYZ #2 #0
    for (bls = 0; bls < n1; bls += fftblock) {
      ble = ((bls + fftblock) - 1);
      if (ble > n1) {
        ble = (n1 - 1);
      }
      len = ((ble - bls) + 1);
#pragma cetus private(i, j)
#pragma loop name fftXYZ #2 #0 #0
      for (i = 0; i < n3; i++) {
#pragma loop name fftXYZ #2 #0 #0 #0
#pragma cetus parallel
#pragma omp parallel for
        for (j = bls; j <= ble; j++) {
          plane[(j - bls) + (blkp * i)] = x[i][k][j];
        }
      }
      Swarztrauber(sign, log, len, n3, blkp, plane, exp3);
#pragma cetus private(i, j)
#pragma loop name fftXYZ #2 #0 #1
      for (i = 0; i <= (n3 - 1); i++) {
#pragma loop name fftXYZ #2 #0 #1 #0
#pragma cetus parallel
#pragma omp parallel for
        for (j = bls; j <= ble; j++) {
          xout[j + ((n1 + 1) * (k + (n2 * i)))] = plane[(j - bls) + (blkp * i)];
        }
      }
    }
  }
  if (timers_enabled) {
    timer_stop(9);
  }
  if (timers_enabled) {
    timer_stop(3);
  }
}
