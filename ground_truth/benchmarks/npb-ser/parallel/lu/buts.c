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
/*  This benchmark is a serial C version of the NPB LU code. This C         */
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
/* --------------------------------------------------------------------- */
/*  */
/* compute the regular-sparse, block upper triangular solution: */
/*  */
/* v <-- ( U-inv ) v */
/*  */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* To improve cache performance, second two dimensions padded by 1  */
/* for even number sizes only.  Only needed in v. */
/* --------------------------------------------------------------------- */
void buts(int ldmx, int ldmy, int ldmz, int nx, int ny, int nz, int k,
          double omega,
          double v[][(((ldmy / 2) * 2) + 1)][(((ldmx / 2) * 2) + 1)][5],
          double tv[ldmy][(((ldmx / 2) * 2) + 1)][5],
          double d[ldmy][(((ldmx / 2) * 2) + 1)][5][5],
          double udx[ldmy][(((ldmx / 2) * 2) + 1)][5][5],
          double udy[ldmy][(((ldmx / 2) * 2) + 1)][5][5],
          double udz[ldmy][(((ldmx / 2) * 2) + 1)][5][5], int ist, int iend,
          int jst, int jend, int nx0, int ny0) {
  /* --------------------------------------------------------------------- */
  /* local variables */
  /* --------------------------------------------------------------------- */
  int i, j, m;
  double tmp, tmp1;
  double tmat[5][5];
#pragma cetus private(i, j, m)
#pragma loop name buts #0
#pragma cetus parallel
#pragma omp parallel for private(i, j, m)
  for (j = (jend - 1); j >= jst; j--) {
#pragma cetus private(i, m)
#pragma loop name buts #0 #0
    for (i = (iend - 1); i >= ist; i--) {
#pragma cetus private(m)
#pragma loop name buts #0 #0 #0
      for (m = 0; m < 5; m++) {
        tv[j][i][m] = (omega * (((((udz[j][i][0][m] * v[k + 1][j][i][0]) +
                                   (udz[j][i][1][m] * v[k + 1][j][i][1])) +
                                  (udz[j][i][2][m] * v[k + 1][j][i][2])) +
                                 (udz[j][i][3][m] * v[k + 1][j][i][3])) +
                                (udz[j][i][4][m] * v[k + 1][j][i][4])));
      }
    }
  }
#pragma cetus private(i, j, m, tmat, tmp, tmp1)
#pragma loop name buts #1
  for (j = (jend - 1); j >= jst; j--) {
#pragma cetus firstprivate(tmat)
#pragma cetus private(i, m, tmp, tmp1)
#pragma cetus lastprivate(tmat)
#pragma loop name buts #1 #0
    for (i = (iend - 1); i >= ist; i--) {
#pragma cetus private(m)
#pragma loop name buts #1 #0 #0
#pragma cetus parallel
#pragma omp parallel for private(m)
      for (m = 0; m < 5; m++) {
        tv[j][i][m] =
            (tv[j][i][m] +
             (omega * ((((((((((udy[j][i][0][m] * v[k][j + 1][i][0]) +
                               (udx[j][i][0][m] * v[k][j][i + 1][0])) +
                              (udy[j][i][1][m] * v[k][j + 1][i][1])) +
                             (udx[j][i][1][m] * v[k][j][i + 1][1])) +
                            (udy[j][i][2][m] * v[k][j + 1][i][2])) +
                           (udx[j][i][2][m] * v[k][j][i + 1][2])) +
                          (udy[j][i][3][m] * v[k][j + 1][i][3])) +
                         (udx[j][i][3][m] * v[k][j][i + 1][3])) +
                        (udy[j][i][4][m] * v[k][j + 1][i][4])) +
                       (udx[j][i][4][m] * v[k][j][i + 1][4]))));
      }
/* --------------------------------------------------------------------- */
/* diagonal block inversion */
/* --------------------------------------------------------------------- */
#pragma cetus private(m)
#pragma loop name buts #1 #0 #1
#pragma cetus parallel
#pragma omp parallel for private(m)
      for (m = 0; m < 5; m++) {
        tmat[m][0] = d[j][i][0][m];
        tmat[m][1] = d[j][i][1][m];
        tmat[m][2] = d[j][i][2][m];
        tmat[m][3] = d[j][i][3][m];
        tmat[m][4] = d[j][i][4][m];
      }
      tmp1 = (1.0 / tmat[0][0]);
      tmp = (tmp1 * tmat[1][0]);
      tmat[1][1] = (tmat[1][1] - (tmp * tmat[0][1]));
      tmat[1][2] = (tmat[1][2] - (tmp * tmat[0][2]));
      tmat[1][3] = (tmat[1][3] - (tmp * tmat[0][3]));
      tmat[1][4] = (tmat[1][4] - (tmp * tmat[0][4]));
      tv[j][i][1] = (tv[j][i][1] - (tv[j][i][0] * tmp));
      tmp = (tmp1 * tmat[2][0]);
      tmat[2][1] = (tmat[2][1] - (tmp * tmat[0][1]));
      tmat[2][2] = (tmat[2][2] - (tmp * tmat[0][2]));
      tmat[2][3] = (tmat[2][3] - (tmp * tmat[0][3]));
      tmat[2][4] = (tmat[2][4] - (tmp * tmat[0][4]));
      tv[j][i][2] = (tv[j][i][2] - (tv[j][i][0] * tmp));
      tmp = (tmp1 * tmat[3][0]);
      tmat[3][1] = (tmat[3][1] - (tmp * tmat[0][1]));
      tmat[3][2] = (tmat[3][2] - (tmp * tmat[0][2]));
      tmat[3][3] = (tmat[3][3] - (tmp * tmat[0][3]));
      tmat[3][4] = (tmat[3][4] - (tmp * tmat[0][4]));
      tv[j][i][3] = (tv[j][i][3] - (tv[j][i][0] * tmp));
      tmp = (tmp1 * tmat[4][0]);
      tmat[4][1] = (tmat[4][1] - (tmp * tmat[0][1]));
      tmat[4][2] = (tmat[4][2] - (tmp * tmat[0][2]));
      tmat[4][3] = (tmat[4][3] - (tmp * tmat[0][3]));
      tmat[4][4] = (tmat[4][4] - (tmp * tmat[0][4]));
      tv[j][i][4] = (tv[j][i][4] - (tv[j][i][0] * tmp));
      tmp1 = (1.0 / tmat[1][1]);
      tmp = (tmp1 * tmat[2][1]);
      tmat[2][2] = (tmat[2][2] - (tmp * tmat[1][2]));
      tmat[2][3] = (tmat[2][3] - (tmp * tmat[1][3]));
      tmat[2][4] = (tmat[2][4] - (tmp * tmat[1][4]));
      tv[j][i][2] = (tv[j][i][2] - (tv[j][i][1] * tmp));
      tmp = (tmp1 * tmat[3][1]);
      tmat[3][2] = (tmat[3][2] - (tmp * tmat[1][2]));
      tmat[3][3] = (tmat[3][3] - (tmp * tmat[1][3]));
      tmat[3][4] = (tmat[3][4] - (tmp * tmat[1][4]));
      tv[j][i][3] = (tv[j][i][3] - (tv[j][i][1] * tmp));
      tmp = (tmp1 * tmat[4][1]);
      tmat[4][2] = (tmat[4][2] - (tmp * tmat[1][2]));
      tmat[4][3] = (tmat[4][3] - (tmp * tmat[1][3]));
      tmat[4][4] = (tmat[4][4] - (tmp * tmat[1][4]));
      tv[j][i][4] = (tv[j][i][4] - (tv[j][i][1] * tmp));
      tmp1 = (1.0 / tmat[2][2]);
      tmp = (tmp1 * tmat[3][2]);
      tmat[3][3] = (tmat[3][3] - (tmp * tmat[2][3]));
      tmat[3][4] = (tmat[3][4] - (tmp * tmat[2][4]));
      tv[j][i][3] = (tv[j][i][3] - (tv[j][i][2] * tmp));
      tmp = (tmp1 * tmat[4][2]);
      tmat[4][3] = (tmat[4][3] - (tmp * tmat[2][3]));
      tmat[4][4] = (tmat[4][4] - (tmp * tmat[2][4]));
      tv[j][i][4] = (tv[j][i][4] - (tv[j][i][2] * tmp));
      tmp1 = (1.0 / tmat[3][3]);
      tmp = (tmp1 * tmat[4][3]);
      tmat[4][4] = (tmat[4][4] - (tmp * tmat[3][4]));
      tv[j][i][4] = (tv[j][i][4] - (tv[j][i][3] * tmp));
      /* ---------------------------------------------------------------------
       */
      /* back substitution */
      /* ---------------------------------------------------------------------
       */
      tv[j][i][4] = (tv[j][i][4] / tmat[4][4]);
      tv[j][i][3] = (tv[j][i][3] - (tmat[3][4] * tv[j][i][4]));
      tv[j][i][3] = (tv[j][i][3] / tmat[3][3]);
      tv[j][i][2] = ((tv[j][i][2] - (tmat[2][3] * tv[j][i][3])) -
                     (tmat[2][4] * tv[j][i][4]));
      tv[j][i][2] = (tv[j][i][2] / tmat[2][2]);
      tv[j][i][1] = (((tv[j][i][1] - (tmat[1][2] * tv[j][i][2])) -
                      (tmat[1][3] * tv[j][i][3])) -
                     (tmat[1][4] * tv[j][i][4]));
      tv[j][i][1] = (tv[j][i][1] / tmat[1][1]);
      tv[j][i][0] = ((((tv[j][i][0] - (tmat[0][1] * tv[j][i][1])) -
                       (tmat[0][2] * tv[j][i][2])) -
                      (tmat[0][3] * tv[j][i][3])) -
                     (tmat[0][4] * tv[j][i][4]));
      tv[j][i][0] = (tv[j][i][0] / tmat[0][0]);
      v[k][j][i][0] = (v[k][j][i][0] - tv[j][i][0]);
      v[k][j][i][1] = (v[k][j][i][1] - tv[j][i][1]);
      v[k][j][i][2] = (v[k][j][i][2] - tv[j][i][2]);
      v[k][j][i][3] = (v[k][j][i][3] - tv[j][i][3]);
      v[k][j][i][4] = (v[k][j][i][4] - tv[j][i][4]);
    }
  }
}
