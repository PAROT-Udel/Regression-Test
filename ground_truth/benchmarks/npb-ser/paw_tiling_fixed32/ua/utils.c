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
/* ------------------------------------------------------------------ */
/* initialize double precision array a with length of n */
/* ------------------------------------------------------------------ */
void reciprocal(double a[], int n) {
  int i;
#pragma loop name reciprocal #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = (1.0 / a[i]);
  }
}

/* ------------------------------------------------------------------ */
/* initialize double precision array a with length of n */
/* ------------------------------------------------------------------ */
void r_init(double a[], int n, double _const) {
  int i;
#pragma loop name r_init #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = _const;
  }
}

/* ------------------------------------------------------------------ */
/* initialize integer array a with length of n */
/* ------------------------------------------------------------------ */
void nr_init(int a[], int n, int _const) {
  int i;
#pragma loop name nr_init #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = _const;
  }
}

/* ------------------------------------------------------------------ */
/* initialize logical array a with length of n */
/* ------------------------------------------------------------------ */
void l_init(logical a[], int n, logical _const) {
  int i;
#pragma loop name l_init #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = _const;
  }
}

/* ------------------------------------------------------------------ */
/* copy array of integers b to a, the length of array is n */
/* ------------------------------------------------------------------ */
void ncopy(int a[], int b[], int n) {
  int i;
#pragma loop name ncopy #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = b[i];
  }
}

/* ------------------------------------------------------------------ */
/* copy double precision array b to a, the length of array is n */
/* ------------------------------------------------------------------ */
void copy(double a[], double b[], int n) {
  int i;
#pragma loop name copy #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = b[i];
  }
}

/* ----------------------------------------------------------------- */
/* a=bc1 */
/* ----------------------------------------------------------------- */
void adds2m1(double a[], double b[], double c1, int n) {
  int i;
#pragma loop name adds2m1 #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = (a[i] + (c1 * b[i]));
  }
}

/* ----------------------------------------------------------------- */
/* a=c1a+b */
/* ----------------------------------------------------------------- */
void adds1m1(double a[], double b[], double c1, int n) {
  int i;
#pragma loop name adds1m1 #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = ((c1 * a[i]) + b[i]);
  }
}

/* ------------------------------------------------------------------ */
/* a=ab */
/* ------------------------------------------------------------------ */
void col2(double a[], double b[], int n) {
  int i;
#pragma loop name col2 #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = (a[i] * b[i]);
  }
}

/* ------------------------------------------------------------------ */
/* zero out array of integers  */
/* ------------------------------------------------------------------ */
void nrzero(int na[], int n) {
  int i;
#pragma loop name nrzero #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    na[i] = 0;
  }
}

/* ------------------------------------------------------------------ */
/* a=a+b */
/* ------------------------------------------------------------------ */
void add2(double a[], double b[], int n) {
  int i;
#pragma loop name add2 #0
#pragma cetus parallel
#pragma cetus private(i)
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    a[i] = (a[i] + b[i]);
  }
}

/* ------------------------------------------------------------------ */
/* calculate the integral of ta1 over the whole domain */
/* ------------------------------------------------------------------ */
double calc_norm() {
  double total, ieltotal;
  int iel, k, j, i, isize;
  total = 0.0;
#pragma loop name calc_norm #0
#pragma cetus reduction(+ : total)
#pragma cetus parallel
#pragma cetus private(i, ieltotal, isize, j, k)
#pragma cetus private(i, iel, ieltotal, isize, j, k)
#pragma cetus reduction(+ : total)
#pragma omp parallel for private(i, iel, ieltotal, isize, j, k)                \
    reduction(+ : total)
  for (iel = 0; iel < nelt; iel++) {
    ieltotal = 0.0;
    isize = size_e[iel];
#pragma loop name calc_norm #0 #0
/* #pragma cetus reduction(+: ieltotal)  */
#pragma cetus private(i, j, k)
#pragma cetus private(i, j, k)
#pragma cetus reduction(+ : ieltotal)
    for (k = 0; k < 5; k++) {
#pragma loop name calc_norm #0 #0 #0
/* #pragma cetus reduction(+: ieltotal)  */
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma cetus reduction(+ : ieltotal)
      for (j = 0; j < 5; j++) {
#pragma loop name calc_norm #0 #0 #0 #0
/* #pragma cetus reduction(+: ieltotal)  */
#pragma cetus private(i)
#pragma cetus private(i)
#pragma cetus reduction(+ : ieltotal)
        for (i = 0; i < 5; i++) {
          ieltotal = (ieltotal + ((ta1[iel][k][j][i] * w3m1[k][j][i]) *
                                  jacm1_s[isize][k][j][i]));
        }
      }
    }
    total = (total + ieltotal);
  }
  return total;
}

/* ----------------------------------------------------------------- */
/* input array frontier, perform (potentially) parallel add so that */
/* the output frontier[i] has sum of frontier[1]+frontier[2]+...+frontier[i] */
/* ----------------------------------------------------------------- */
void parallel_add(int frontier[]) {
  int nellog, i, ahead, ii, ntemp, n1, ntemp1, n2, iel;
  if (nelt <= 1) {
    return;
  }
  nellog = 0;
  iel = 1;
  do {
    iel = (iel * 2);
    nellog = (nellog + 1);
  } while (iel < nelt);

  ntemp = 1;
#pragma loop name parallel_add #0
#pragma cetus private(ahead, i, iel, ii, n1, n2, ntemp1)
#pragma cetus private(ahead, i, iel, ii, n1, n2, ntemp1)
  for (i = 0; i < nellog; i++) {
    n1 = (ntemp * 2);
    n2 = n1;
#pragma cetus lastprivate(n2)
#pragma loop name parallel_add #0 #0
#pragma cetus private(ahead, iel, ii)
#pragma cetus lastprivate(n2)
#pragma cetus private(ahead, iel, ii)
#pragma cetus lastprivate(n2)
    for (iel = n1; iel <= nelt; iel += n1) {
      ahead = frontier[(iel - ntemp) - 1];
#pragma loop name parallel_add #0 #0 #0
#pragma cetus parallel
#pragma cetus private(ii)
#pragma omp parallel for private(ii)
      for (ii = (ntemp - 1); ii >= 0; ii--) {
        frontier[(iel - ii) - 1] = (frontier[(iel - ii) - 1] + ahead);
      }
      n2 = iel;
    }
    if (n2 <= nelt) {
      n2 = (n2 + n1);
    }
    ntemp1 = (n2 - nelt);
    if (ntemp1 < ntemp) {
      ahead = frontier[(n2 - ntemp) - 1];
#pragma loop name parallel_add #0 #1
#pragma cetus parallel
#pragma cetus private(ii)
#pragma omp parallel for private(ii)
      for (ii = (ntemp - 1); ii >= ntemp1; ii--) {
        frontier[(n2 - ii) - 1] = (frontier[(n2 - ii) - 1] + ahead);
      }
    }
    ntemp = n1;
  }
}

/* ------------------------------------------------------------------ */
/* Perform stiffness summation: element-mortar-element mapping */
/* ------------------------------------------------------------------ */
void dssum() {
  transfb(dpcmor, (double *)dpcelm);
  transf(dpcmor, (double *)dpcelm);
}

/* ------------------------------------------------------------------ */
/* assign the value val to face(iface,iel) of array a. */
/* ------------------------------------------------------------------ */
void facev(double a[5][5][5], int iface, double val) {
  int kx1, kx2, ky1, ky2, kz1, kz2, ix, iy, iz;
  kx1 = 1;
  ky1 = 1;
  kz1 = 1;
  kx2 = 5;
  ky2 = 5;
  kz2 = 5;
  if (iface == 0) {
    kx1 = 5;
  }
  if (iface == 1) {
    kx2 = 1;
  }
  if (iface == 2) {
    ky1 = 5;
  }
  if (iface == 3) {
    ky2 = 1;
  }
  if (iface == 4) {
    kz1 = 5;
  }
  if (iface == 5) {
    kz2 = 1;
  }
#pragma loop name facev #0
#pragma cetus parallel
#pragma cetus private(ix, iy)
#pragma cetus private(ix, iy, iz)
#pragma omp parallel for private(ix, iy, iz)
  for (iz = (kz1 - 1); iz < kz2; iz++) {
#pragma loop name facev #0 #0
#pragma cetus private(ix, iy)
#pragma cetus private(ix, iy)
    for (iy = (ky1 - 1); iy < ky2; iy++) {
#pragma loop name facev #0 #0 #0
#pragma cetus private(ix)
#pragma cetus private(ix)
      for (ix = (kx1 - 1); ix < kx2; ix++) {
        a[iz][iy][ix] = val;
      }
    }
  }
}
