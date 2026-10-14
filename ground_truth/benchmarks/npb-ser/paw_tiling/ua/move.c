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
/* ------------------------------------------------------------------------- */
/*                                                                          */
/*  This benchmark is a serial C version of the NPB UA code. This C         */
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
#include "header.h"
/* --------------------------------------------------------------- */
/* move element to proper location in morton space filling curve */
/* --------------------------------------------------------------- */
void move() {
  int i, iside, jface, iel, ntemp, ii1, ii2, n1, n2, cb;
  n2 = ((2 * 6) * nelt);
  n1 = (n2 * 2);
  nr_init((int *)sje_new, n1, -1);
  nr_init((int *)ijel_new, n2, -1);
#pragma cetus private(cb, i, iel, ii1, ii2, iside, jface, ntemp)
#pragma loop name move #0
#pragma cetus private(cb, i, iel, ii1, ii2, iside, jface, ntemp)
  for (iel = 0; iel < nelt; iel++) {
    i = mt_to_id[iel];
    treenew[iel] = tree[i];
    copy(xc_new[iel], xc[i], 8);
    copy(yc_new[iel], yc[i], 8);
    copy(zc_new[iel], zc[i], 8);
#pragma cetus private(cb, ii1, ii2, jface, ntemp)
#pragma loop name move #0 #0
#pragma cetus parallel
#pragma cetus private(cb, ii1, ii2, iside, jface, ntemp)
#pragma omp parallel for private(cb, ii1, ii2, iside, jface, ntemp)
    for (iside = 0; iside < 6; iside++) {
      jface = jjface[iside];
      cb = cbc[i][iside];
      xc_new[iel][iside] = xc[i][iside];
      yc_new[iel][iside] = yc[i][iside];
      zc_new[iel][iside] = zc[i][iside];
      cbc_new[iel][iside] = cb;
      if (cb == 2) {
        ntemp = sje[i][iside][0][0];
        ijel_new[iel][iside][0] = 0;
        ijel_new[iel][iside][1] = 0;
        sje_new[iel][iside][0][0] = id_to_mt[ntemp];
      } else {
        if (cb == 1) {
          ntemp = sje[i][iside][0][0];
          ijel_new[iel][iside][0] = ijel[i][iside][0];
          ijel_new[iel][iside][1] = ijel[i][iside][1];
          sje_new[iel][iside][0][0] = id_to_mt[ntemp];
        } else {
          if (cb == 3) {
#pragma cetus firstprivate(ijel_new)
#pragma cetus private(ii1, ii2, ntemp)
#pragma cetus lastprivate(ijel_new)
#pragma loop name move #0 #0 #0
#pragma cetus firstprivate(ijel_new)
#pragma cetus private(ii1, ii2, ntemp)
#pragma cetus lastprivate(ijel_new)
            for (ii2 = 0; ii2 < 2; ii2++) {
#pragma cetus firstprivate(ijel_new)
#pragma cetus private(ii1, ntemp)
#pragma cetus lastprivate(ijel_new)
#pragma loop name move #0 #0 #0 #0
#pragma cetus firstprivate(ijel_new)
#pragma cetus private(ii1, ntemp)
#pragma cetus lastprivate(ijel_new)
              for (ii1 = 0; ii1 < 2; ii1++) {
                ntemp = sje[i][iside][ii2][ii1];
                ijel_new[iel][iside][0] = 0;
                ijel_new[iel][iside][1] = 0;
                sje_new[iel][iside][ii2][ii1] = id_to_mt[ntemp];
              }
            }
          } else {
            if (cb == 0) {
              sje_new[iel][iside][0][0] = (-1);
              sje_new[iel][iside][1][0] = (-1);
              sje_new[iel][iside][0][1] = (-1);
              sje_new[iel][iside][1][1] = (-1);
            }
          }
        }
      }
    }
    copy(ta2[iel][0][0], ta1[i][0][0], (5 * 5) * 5);
  }
  copy((double *)xc, (double *)xc_new, 8 * nelt);
  copy((double *)yc, (double *)yc_new, 8 * nelt);
  copy((double *)zc, (double *)zc_new, 8 * nelt);
  ncopy((int *)sje, (int *)sje_new, (4 * 6) * nelt);
  ncopy((int *)ijel, (int *)ijel_new, (2 * 6) * nelt);
  ncopy((int *)cbc, (int *)cbc_new, 6 * nelt);
  ncopy((int *)tree, (int *)treenew, nelt);
  copy((double *)ta1, (double *)ta2, ((5 * 5) * 5) * nelt);
#pragma loop name move #1
#pragma cetus parallel
#pragma omp parallel for
  for (iel = 0; iel < nelt; iel++) {
    mt_to_id[iel] = iel;
    id_to_mt[iel] = iel;
  }
}
