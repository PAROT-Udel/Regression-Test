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
/*  This benchmark is a serial C version of the NPB CG code. This C         */
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
/* NPB CG serial version       */
/* --------------------------------------------------------------------- */
#include "../common/print_results.h"
#include "../common/randdp.h"
#include "../common/timers.h"
#include "globals.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/* --------------------------------------------------------------------- */
/* common main_int_mem */
static int colidx[((1400 * (7 + 1)) * (7 + 1))];
static int rowstr[(1400 + 1)];
static int iv[1400];
static int arow[1400];
static int acol[(1400 * (7 + 1))];
/* common main_flt_mem */
static double aelt[(1400 * (7 + 1))];
static double a[((1400 * (7 + 1)) * (7 + 1))];
static double x[(1400 + 2)];
static double z[(1400 + 2)];
static double p[(1400 + 2)];
static double q[(1400 + 2)];
static double r[(1400 + 2)];
/* common partit_size */
static int naa;
static int nzz;
static int firstrow;
static int lastrow;
static int firstcol;
static int lastcol;
/* commonurando */
static double amult;
static double tran;
/* commontimers */
static logical timeron;
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
static void conj_grad(int colidx[], int rowstr[], double x[], double z[],
                      double a[], double p[], double q[], double r[],
                      double *rnorm);
static void makea(int n, int nz, double a[], int colidx[], int rowstr[],
                  int firstrow, int lastrow, int firstcol, int lastcol,
                  int arow[], int acol[][(7 + 1)], double aelt[][(7 + 1)],
                  int iv[]);
static void sparse(double a[], int colidx[], int rowstr[], int n, int nz,
                   int nozer, int arow[], int acol[][(7 + 1)],
                   double aelt[][(7 + 1)], int firstrow, int lastrow,
                   int nzloc[], double rcond, double shift);
static void sprnvc(int n, int nz, int nn1, double v[], int iv[]);
static int icnvrt(double x, int ipwr2);
static void vecset(int n, double v[], int iv[], int *nzv, int i, double val);
/* --------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
  int i, j, k, it;
  double zeta;
  double rnorm;
  double norm_temp1, norm_temp2;
  double t, mflops, tmax;
  char Class;
  logical verified;
  double zeta_verify_value, epsilon, err;
  char *t_names[3];
  FILE *fp;
#pragma cetus private(i)
#pragma loop name main #0
  for (i = 0; i < 3; i++) {
    timer_clear(i);
  }
  if ((fp = fopen("timer.flag", "r")) != ((void *)0)) {
    timeron = true;
    t_names[0] = "init";
    t_names[1] = "benchmk";
    t_names[2] = "conjgd";
    fclose(fp);
  } else {
    timeron = false;
  }
  timer_start(0);
  firstrow = 0;
  lastrow = (1400 - 1);
  firstcol = 0;
  lastcol = (1400 - 1);
  Class = 'S';
  zeta_verify_value = 8.5971775078648;
  printf("\n\n NAS Parallel Benchmarks (NPB3.3-SER-C) - CG Benchmark\n\n");
  printf(" Size: %11d\n", 1400);
  printf(" Iterations: %5d\n", 15);
  printf("\n");
  naa = 1400;
  nzz = ((1400 * (7 + 1)) * (7 + 1));
  /* --------------------------------------------------------------------- */
  /* Inialize random number generator */
  /* --------------------------------------------------------------------- */
  tran = 3.14159265E8;
  amult = 1.220703125E9;
  zeta = randlc(&tran, amult);
  /* --------------------------------------------------------------------- */
  /*   */
  /* --------------------------------------------------------------------- */
  makea(naa, nzz, a, colidx, rowstr, firstrow, lastrow, firstcol, lastcol, arow,
        (int(*)[(7 + 1)])((void *)acol), (double(*)[(7 + 1)])((void *)aelt),
        iv);
/* --------------------------------------------------------------------- */
/* Note: as a result of the above call to makea: */
/*      values of j used in indexing rowstr go from 0 --> lastrow-firstrow */
/*      values of colidx which are col indexes go from firstcol --> lastcol */
/*      So: */
/*      Shift the col index vals from actual (firstcol --> lastcol )  */
/*      to local, i.e., (0 --> lastcol-firstcol) */
/* --------------------------------------------------------------------- */
#pragma cetus parallel
#pragma cetus private(j, k)
#pragma omp parallel private(j, k)
  {
    int *reduce0 = (int *)malloc(((1400 * (7 + 1)) * (7 + 1)) * sizeof(int));
    int reduce_span_0;
    for (reduce_span_0 = 0; reduce_span_0 < ((1400 * (7 + 1)) * (7 + 1));
         reduce_span_0++) {
      reduce0[reduce_span_0] = 0;
    }
#pragma loop name main #1
#pragma cetus for
#pragma omp for
    for (j = 0; j < ((lastrow - firstrow) + 1); j++) {
#pragma cetus private(k)
#pragma loop name main #1 #0
      for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
        reduce0[k] = (reduce0[k] - firstcol);
      }
    }
#pragma cetus critical
#pragma omp critical
    {
      for (reduce_span_0 = 0; reduce_span_0 < ((1400 * (7 + 1)) * (7 + 1));
           reduce_span_0++) {
        colidx[reduce_span_0] += reduce0[reduce_span_0];
      }
    }
  }
/* --------------------------------------------------------------------- */
/* set starting vector to (1, 1, .... 1) */
/* --------------------------------------------------------------------- */
#pragma cetus private(i)
#pragma loop name main #2
#pragma cetus parallel
#pragma omp parallel for private(i)
  for (i = 0; i < (1400 + 1); i++) {
    x[i] = 1.0;
  }
#pragma cetus private(j)
#pragma loop name main #3
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = 0.0;
    p[j] = 0.0;
  }
  zeta = 0.0;
/* --------------------------------------------------------------------- */
/* ----> */
/* Do one iteration untimed to init all code and data page tables */
/* ---->                    (then reinit, start timing, to niter its) */
/* --------------------------------------------------------------------- */
#pragma cetus private(it, j, norm_temp1, norm_temp2)
#pragma loop name main #4
  for (it = 1; it <= 1; it++) {
    /* --------------------------------------------------------------------- */
    /* The call to the conjugate gradient routine: */
    /* --------------------------------------------------------------------- */
    conj_grad(colidx, rowstr, x, z, a, p, q, r, &rnorm);
    /* --------------------------------------------------------------------- */
    /* zeta = shift + 1(x.z) */
    /* So, first: (x.z) */
    /* Also, find norm of z */
    /* So, first: (z.z) */
    /* --------------------------------------------------------------------- */
    norm_temp1 = 0.0;
    norm_temp2 = 0.0;
#pragma cetus private(j)
#pragma loop name main #4 #0
#pragma cetus reduction(+ : norm_temp1, norm_temp2)
#pragma cetus parallel
#pragma omp parallel for private(j) reduction(+ : norm_temp1, norm_temp2)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      norm_temp1 = (norm_temp1 + (x[j] * z[j]));
      norm_temp2 = (norm_temp2 + (z[j] * z[j]));
    }
    norm_temp2 = (1.0 / sqrt(norm_temp2));
/* --------------------------------------------------------------------- */
/* Normalize z to obtain x */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name main #4 #1
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      x[j] = (norm_temp2 * z[j]);
    }
  }
/* end of do one iteration untimed */
/* --------------------------------------------------------------------- */
/* set starting vector to (1, 1, .... 1) */
/* --------------------------------------------------------------------- */
#pragma cetus private(i)
#pragma loop name main #5
#pragma cetus parallel
#pragma omp parallel for private(i)
  for (i = 0; i < (1400 + 1); i++) {
    x[i] = 1.0;
  }
  zeta = 0.0;
  timer_stop(0);
  printf(" Initialization time = %15.3f seconds\n", timer_read(0));
  timer_start(1);
/* --------------------------------------------------------------------- */
/* ----> */
/* Main Iteration for inverse power method */
/* ----> */
/* --------------------------------------------------------------------- */
#pragma cetus private(it, j, norm_temp1, norm_temp2)
#pragma cetus lastprivate(zeta)
#pragma loop name main #6
  for (it = 1; it <= 15; it++) {
    /* --------------------------------------------------------------------- */
    /* The call to the conjugate gradient routine: */
    /* --------------------------------------------------------------------- */
    if (timeron) {
      timer_start(2);
    }
    /* CARV-FIX  */
    conj_grad(colidx, rowstr, x, z, a, p, q, r, &rnorm);
    if (timeron) {
      timer_stop(2);
    }
    /* --------------------------------------------------------------------- */
    /* zeta = shift + 1(x.z) */
    /* So, first: (x.z) */
    /* Also, find norm of z */
    /* So, first: (z.z) */
    /* --------------------------------------------------------------------- */
    norm_temp1 = 0.0;
    norm_temp2 = 0.0;
#pragma cetus private(j)
#pragma loop name main #6 #0
#pragma cetus reduction(+ : norm_temp1, norm_temp2)
#pragma cetus parallel
#pragma omp parallel for private(j) reduction(+ : norm_temp1, norm_temp2)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      norm_temp1 = (norm_temp1 + (x[j] * z[j]));
      norm_temp2 = (norm_temp2 + (z[j] * z[j]));
    }
    norm_temp2 = (1.0 / sqrt(norm_temp2));
    zeta = (10.0 + (1.0 / norm_temp1));
    if (it == 1) {
      printf("\n   iteration           ||r||                 zeta\n");
    }
    printf("    %5d       %20.14E%20.13f\n", it, rnorm, zeta);
/* --------------------------------------------------------------------- */
/* Normalize z to obtain x */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name main #6 #1
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      x[j] = (norm_temp2 * z[j]);
    }
  }
  /* end of main iter inv pow meth */
  timer_stop(1);
  /* --------------------------------------------------------------------- */
  /* End of timed section */
  /* --------------------------------------------------------------------- */
  t = timer_read(1);
  printf(" Benchmark completed\n");
  epsilon = 1.0E-10;
  if (Class != 'U') {
    err = (fabs(zeta - zeta_verify_value) / zeta_verify_value);
    if (err <= epsilon) {
      verified = true;
      printf(" VERIFICATION SUCCESSFUL\n");
      printf(" Zeta is    %20.13E\n", zeta);
      printf(" Error is   %20.13E\n", err);
    } else {
      verified = false;
      printf(" VERIFICATION FAILED\n");
      printf(" Zeta                %20.13E\n", zeta);
      printf(" The correct zeta is %20.13E\n", zeta_verify_value);
    }
  } else {
    verified = false;
    printf(" Problem size unknown\n");
    printf(" NO VERIFICATION PERFORMED\n");
  }
  if (t != 0.0) {
    mflops = (((((double)((2 * 15) * 1400)) *
                (((3.0 + ((double)(7 * (7 + 1)))) +
                  (25.0 * (5.0 + ((double)(7 * (7 + 1)))))) +
                 3.0)) /
               t) /
              1000000.0);
  } else {
    mflops = 0.0;
  }
  print_results("CG", Class, 1400, 0, 0, 15, t, mflops,
                "          floating point", verified, "3.3.1", "09 Sep 2026",
                "gcc", "$(CC)", "-lm", "-I../common",
                "-g -Wall -O3 -fopenmp -mcmodel=large",
                "-O3 -fopenmp -mcmodel=large", "randdp");
  /* --------------------------------------------------------------------- */
  /* More timers */
  /* --------------------------------------------------------------------- */
  if (timeron) {
    tmax = timer_read(1);
    if (tmax == 0.0) {
      tmax = 1.0;
    }
    printf("  SECTION   Time (secs)\n");
#pragma cetus private(i, t)
#pragma loop name main #7
    for (i = 0; i < 3; i++) {
      t = timer_read(i);
      if (i == 0) {
        printf("  %8s:%9.3f\n", t_names[i], t);
      } else {
        printf("  %8s:%9.3f  (%6.2f%%)\n", t_names[i], t, (t * 100.0) / tmax);
        if (i == 2) {
          t = (tmax - t);
          printf("    --> %8s:%9.3f  (%6.2f%%)\n", "rest", t,
                 (t * 100.0) / tmax);
        }
      }
    }
  }
  return 0;
}

/* --------------------------------------------------------------------- */
/* Floaging point arrays here are named as in NPB1 spec discussion of  */
/* CG algorithm */
/* --------------------------------------------------------------------- */
static void conj_grad(int colidx[], int rowstr[], double x[], double z[],
                      double a[], double p[], double q[], double r[],
                      double *rnorm) {
#pragma scop
  int j, k;
  int cgit, cgitmax = 25;
  double d, sum, rho, rho0, alpha, beta;
  rho = 0.0;
#pragma experimental section start
/* --------------------------------------------------------------------- */
/* Initialize the CG algorithm: */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name conj_grad #0
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (j = 0; j < (naa + 1); j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
/* --------------------------------------------------------------------- */
/* rho = r.r */
/* Now, obtain the norm of r: First, sum squares of r elements locally... */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name conj_grad #1
#pragma cetus reduction(+ : rho)
#pragma cetus parallel
#pragma omp parallel for private(j) reduction(+ : rho)
  for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
    rho = (rho + (r[j] * r[j]));
  }
/* --------------------------------------------------------------------- */
/* ----> */
/* The conj grad iteration loop */
/* ----> */
/* --------------------------------------------------------------------- */
#pragma cetus private(alpha, beta, cgit, d, j, k, rho0, sum)
#pragma loop name conj_grad #2
  /* #pragma cetus reduction(+: z[j])  */
  for (cgit = 1; cgit <= cgitmax; cgit++) {
/* --------------------------------------------------------------------- */
/* q = A.p */
/* The partition submatrix-vector multiply: use workspace w */
/* --------------------------------------------------------------------- */
/*  */
/* NOTE: this version of the multiply is actually (slightly: maybe %5)  */
/*       faster on the sp2 on 16 nodes than is the unrolled-by-2 version  */
/*       below.   On the Cray t3d, the reverse is true, i.e., the  */
/*       unrolled-by-two version is some 10% faster.   */
/*       The unrolled-by-8 version below is significantly faster */
/*       on the Cray t3d - overall speed of code is 1.5 times faster. */
#pragma cetus private(j, k, sum)
#pragma loop name conj_grad #2 #0
#pragma cetus parallel
#pragma omp parallel for private(j, k, sum)
    for (j = 0; j < ((lastrow - firstrow) + 1); j++) {
      sum = 0.0;
#pragma cetus private(k)
#pragma loop name conj_grad #2 #0 #0
      /* #pragma cetus reduction(+: sum)  */
      for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
        sum = (sum + (a[k] * p[colidx[k]]));
      }
      q[j] = sum;
    }
    /* --------------------------------------------------------------------- */
    /* Obtain p.q */
    /* --------------------------------------------------------------------- */
    d = 0.0;
#pragma cetus private(j)
#pragma loop name conj_grad #2 #1
#pragma cetus reduction(+ : d)
#pragma cetus parallel
#pragma omp parallel for private(j) reduction(+ : d)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      d = (d + (p[j] * q[j]));
    }
    /* --------------------------------------------------------------------- */
    /* Obtain alpha = rho (p.q) */
    /* --------------------------------------------------------------------- */
    alpha = (rho / d);
    /* --------------------------------------------------------------------- */
    /* Save a temporary of rho */
    /* --------------------------------------------------------------------- */
    rho0 = rho;
    /* --------------------------------------------------------------------- */
    /* Obtain z = z + alphap */
    /* and    r = r - alphaq */
    /* --------------------------------------------------------------------- */
    rho = 0.0;
#pragma cetus private(j)
#pragma loop name conj_grad #2 #2
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      z[j] = (z[j] + (alpha * p[j]));
      r[j] = (r[j] - (alpha * q[j]));
    }
/* --------------------------------------------------------------------- */
/* rho = r.r */
/* Now, obtain the norm of r: First, sum squares of r elements locally... */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name conj_grad #2 #3
#pragma cetus reduction(+ : rho)
#pragma cetus parallel
#pragma omp parallel for private(j) reduction(+ : rho)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      rho = (rho + (r[j] * r[j]));
    }
    /* --------------------------------------------------------------------- */
    /* Obtain beta: */
    /* --------------------------------------------------------------------- */
    beta = (rho / rho0);
/* --------------------------------------------------------------------- */
/* p = r + betap */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name conj_grad #2 #4
#pragma cetus parallel
#pragma omp parallel for private(j)
    for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
      p[j] = (r[j] + (beta * p[j]));
    }
  }
  /* end of do cgit=1,cgitmax */
  /* --------------------------------------------------------------------- */
  /* Compute residual norm explicitly:  ||r|| = ||x - A.z|| */
  /* First, form A.z */
  /* The partition submatrix-vector multiply */
  /* --------------------------------------------------------------------- */
  sum = 0.0;
#pragma cetus private(d, j, k)
#pragma loop name conj_grad #3
#pragma cetus parallel
#pragma omp parallel for private(d, j, k)
  for (j = 0; j < ((lastrow - firstrow) + 1); j++) {
    d = 0.0;
#pragma cetus private(k)
#pragma loop name conj_grad #3 #0
    /* #pragma cetus reduction(+: d)  */
    for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
      d = (d + (a[k] * z[colidx[k]]));
    }
    r[j] = d;
  }
/* --------------------------------------------------------------------- */
/* At this point, r contains A.z */
/* --------------------------------------------------------------------- */
#pragma cetus private(d, j)
#pragma loop name conj_grad #4
#pragma cetus reduction(+ : sum)
#pragma cetus parallel
#pragma omp parallel for private(d, j) reduction(+ : sum)
  for (j = 0; j < ((lastcol - firstcol) + 1); j++) {
    d = (x[j] - r[j]);
    sum = (sum + (d * d));
  }
#pragma endscop
#pragma experimental section stop
  (*rnorm) = sqrt(sum);
}

/* --------------------------------------------------------------------- */
/* generate the test problem for benchmark 6 */
/* makea generates a sparse matrix with a */
/* prescribed sparsity distribution */
/*  */
/* parameter    type        usage */
/*  */
/* input */
/*  */
/* n            i           number of colsrows of matrix */
/* nz           i           nonzeros as declared array size */
/* rcond        r8         condition number */
/* shift        r8         main diagonal shift */
/*  */
/* output */
/*  */
/* a            r8         array for nonzeros */
/* colidx       i           col indices */
/* rowstr       i           row pointers */
/*  */
/* workspace */
/*  */
/* iv, arow, acol i */
/* aelt           r8 */
/* --------------------------------------------------------------------- */
static void makea(int n, int nz, double a[], int colidx[], int rowstr[],
                  int firstrow, int lastrow, int firstcol, int lastcol,
                  int arow[], int acol[][(7 + 1)], double aelt[][(7 + 1)],
                  int iv[]) {
  int iouter, ivelt, nzv, nn1;
  int ivc[(7 + 1)];
  double vc[(7 + 1)];
  /* --------------------------------------------------------------------- */
  /* nonzer is approximately  (int(sqrt(nnzan))); */
  /* --------------------------------------------------------------------- */
  /* --------------------------------------------------------------------- */
  /* nn1 is the smallest power of two not less than n */
  /* --------------------------------------------------------------------- */
  nn1 = 1;
  do {
    nn1 = (2 * nn1);
  } while (nn1 < n);

/* --------------------------------------------------------------------- */
/* Generate nonzero positions and save for the use in sparse. */
/* --------------------------------------------------------------------- */
#pragma cetus private(iouter, ivelt)
#pragma loop name makea #0
  for (iouter = 0; iouter < n; iouter++) {
    nzv = 7;
    sprnvc(n, nzv, nn1, vc, ivc);
    vecset(n, vc, ivc, &nzv, iouter + 1, 0.5);
    arow[iouter] = nzv;
#pragma cetus private(ivelt)
#pragma loop name makea #0 #0
#pragma cetus parallel
#pragma omp parallel for private(ivelt)
    for (ivelt = 0; ivelt < nzv; ivelt++) {
      acol[iouter][ivelt] = (ivc[ivelt] - 1);
      aelt[iouter][ivelt] = vc[ivelt];
    }
  }
  /* --------------------------------------------------------------------- */
  /* ... make the sparse matrix from list of elements with duplicates */
  /*     (iv is used as  workspace) */
  /* --------------------------------------------------------------------- */
  sparse(a, colidx, rowstr, n, nz, 7, arow, acol, aelt, firstrow, lastrow, iv,
         0.1, 10.0);
}

/* --------------------------------------------------------------------- */
/* rows range from firstrow to lastrow */
/* the rowstr pointers are defined for nrows = lastrow-firstrow+1 values */
/* --------------------------------------------------------------------- */
static void sparse(double a[], int colidx[], int rowstr[], int n, int nz,
                   int nozer, int arow[], int acol[][(7 + 1)],
                   double aelt[][(7 + 1)], int firstrow, int lastrow,
                   int nzloc[], double rcond, double shift) {
  int nrows;
  /* --------------------------------------------------- */
  /* generate a sparse matrix from a list of */
  /* [col, row, element] tri */
  /* --------------------------------------------------- */
  int i, j, j1, j2, nza, k, kk, nzrow, jcol;
  double size, scale, ratio, va;
  logical cont40;
  /* --------------------------------------------------------------------- */
  /* how many rows of result */
  /* --------------------------------------------------------------------- */
  nrows = ((lastrow - firstrow) + 1);
/* --------------------------------------------------------------------- */
/* ...count the number of triples in each row */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name sparse #0
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (j = 0; j < (nrows + 1); j++) {
    rowstr[j] = 0;
  }
#pragma cetus private(i, j, nza)
#pragma loop name sparse #1
  /* #pragma cetus reduction(+: )  */
  for (i = 0; i < n; i++) {
#pragma cetus private(j, nza)
#pragma loop name sparse #1 #0
    /* #pragma cetus reduction(+: rowstr[j])  */
    for (nza = 0; nza < arow[i]; nza++) {
      j = (acol[i][nza] + 1);
      rowstr[j] = (rowstr[j] + arow[i]);
    }
  }
  rowstr[0] = 0;
#pragma cetus private(j)
#pragma loop name sparse #2
  for (j = 1; j < (nrows + 1); j++) {
    rowstr[j] = (rowstr[j] + rowstr[j - 1]);
  }
  nza = (rowstr[nrows] - 1);
  /* --------------------------------------------------------------------- */
  /* ... rowstr(j) now is the location of the first nonzero */
  /*     of row j of a */
  /* --------------------------------------------------------------------- */
  if (nza > nz) {
    printf("Space for matrix elements exceeded in sparse\n");
    printf("nza, nzmax = %d, %d\n", nza, nz);
    exit(1);
  }
/* --------------------------------------------------------------------- */
/* ... preload data pages */
/* --------------------------------------------------------------------- */
#pragma cetus private(j, k)
#pragma loop name sparse #3
#pragma cetus parallel
#pragma omp parallel for private(j, k)
  for (j = 0; j < nrows; j++) {
#pragma cetus private(k)
#pragma loop name sparse #3 #0
    for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
      a[k] = 0.0;
      colidx[k] = (-1);
    }
    nzloc[j] = 0;
  }
  /* --------------------------------------------------------------------- */
  /* ... generate actual values by summing duplicates */
  /* --------------------------------------------------------------------- */
  size = 1.0;
  ratio = pow(rcond, 1.0 / ((double)n));
#pragma cetus private(i, j, jcol, k, kk, nza, nzrow, scale, va)
#pragma loop name sparse #4
  /* #pragma cetus reduction(+: nzloc[j])  */
  for (i = 0; i < n; i++) {
#pragma cetus private(j, jcol, k, kk, nza, nzrow, scale, va)
#pragma loop name sparse #4 #0
    /* #pragma cetus reduction(+: nzloc[j])  */
    for (nza = 0; nza < arow[i]; nza++) {
      j = acol[i][nza];
      scale = (size * aelt[i][nza]);
#pragma cetus private(jcol, k, kk, nzrow, va)
#pragma loop name sparse #4 #0 #0
      /* #pragma cetus reduction(+: nzloc[j])  */
      for (nzrow = 0; nzrow < arow[i]; nzrow++) {
        jcol = acol[i][nzrow];
        va = (aelt[i][nzrow] * scale);
        /* --------------------------------------------------------------------
         */
        /* ... add the identity rcond to the generated matrix to bound */
        /*     the smallest eigenvalue from below by rcond */
        /* --------------------------------------------------------------------
         */
        if ((jcol == j) && (j == i)) {
          va = ((va + rcond) - shift);
        }
        cont40 = false;
#pragma cetus private(kk)
#pragma loop name sparse #4 #0 #0 #0
        /* #pragma cetus reduction(+: nzloc[j])  */
        for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
          if (colidx[k] > jcol) {
/* ---------------------------------------------------------------- */
/* ... insert colidx here orderly */
/* ---------------------------------------------------------------- */
#pragma cetus private(kk)
#pragma loop name sparse #4 #0 #0 #0 #0
            for (kk = (rowstr[j + 1] - 2); kk >= k; kk--) {
              if (colidx[kk] > (-1)) {
                a[kk + 1] = a[kk];
                colidx[kk + 1] = colidx[kk];
              }
            }
            colidx[k] = jcol;
            a[k] = 0.0;
            cont40 = true;
            break;
          } else {
            if (colidx[k] == (-1)) {
              colidx[k] = jcol;
              cont40 = true;
              break;
            } else {
              if (colidx[k] == jcol) {
                /* --------------------------------------------------------------
                 */
                /* ... mark the duplicated entry */
                /* --------------------------------------------------------------
                 */
                nzloc[j] = (nzloc[j] + 1);
                cont40 = true;
                break;
              }
            }
          }
        }
        if (cont40 == false) {
          printf("internal error in sparse: i=%d\n", i);
          exit(1);
        }
        a[k] = (a[k] + va);
      }
    }
    size = (size * ratio);
  }
/* --------------------------------------------------------------------- */
/* ... remove empty entries and generate final results */
/* --------------------------------------------------------------------- */
#pragma cetus private(j)
#pragma loop name sparse #5
  for (j = 1; j < nrows; j++) {
    nzloc[j] = (nzloc[j] + nzloc[j - 1]);
  }
#pragma cetus private(j, j1, j2, k, nza)
#pragma loop name sparse #6
  for (j = 0; j < nrows; j++) {
    if (j > 0) {
      j1 = (rowstr[j] - nzloc[j - 1]);
    } else {
      j1 = 0;
    }
    j2 = (rowstr[j + 1] - nzloc[j]);
    nza = rowstr[j];
#pragma cetus private(k)
#pragma loop name sparse #6 #0
    for (k = j1; k < j2; k++) {
      a[k] = a[((-1 * j1) + k) + rowstr[j]];
      colidx[k] = colidx[((-1 * j1) + k) + rowstr[j]];
    }
    if ((((-1 + (-1 * j1)) + (-1 * nzloc[j])) + rowstr[1 + j]) >= 0) {
      nza += (((-1 * j1) + (-1 * nzloc[j])) + rowstr[1 + j]);
    }
  }
#pragma cetus private(j)
#pragma loop name sparse #7
#pragma cetus parallel
#pragma omp parallel for private(j)
  for (j = 1; j < (nrows + 1); j++) {
    rowstr[j] = (rowstr[j] - nzloc[j - 1]);
  }
  nza = (rowstr[nrows] - 1);
}

/* --------------------------------------------------------------------- */
/* generate a sparse n-vector (v, iv) */
/* having nzv nonzeros */
/*  */
/* mark(i) is set to 1 if position i is nonzero. */
/* mark is all zero on entry and is reset to all zero before exit */
/* this corrects a performance bug found by John G. Lewis, caused by */
/* reinitialization of mark on every one of the n calls to sprnvc */
/* --------------------------------------------------------------------- */
static void sprnvc(int n, int nz, int nn1, double v[], int iv[]) {
  int nzv, ii, i;
  double vecelt, vecloc;
  nzv = 0;
  while (nzv < nz) {
    logical was_gen = false;
    vecelt = randlc(&tran, amult);
    /* --------------------------------------------------------------------- */
    /* generate an integer between 1 and n in a portable manner */
    /* --------------------------------------------------------------------- */
    vecloc = randlc(&tran, amult);
    i = (icnvrt(vecloc, nn1) + 1);
    if (i > n) {
      continue;
    }
/* --------------------------------------------------------------------- */
/* was this integer generated already? */
/* --------------------------------------------------------------------- */
#pragma loop name sprnvc #0
    for (ii = 0; ii < nzv; ii++) {
      if (iv[ii] == i) {
        was_gen = true;
        break;
      }
    }
    if (was_gen) {
      continue;
    }
    v[nzv] = vecelt;
    iv[nzv] = i;
    nzv = (nzv + 1);
  }
}

/* --------------------------------------------------------------------- */
/* scale a double precision number x in (0,1) by a power of 2 and chop it */
/* --------------------------------------------------------------------- */
static int icnvrt(double x, int ipwr2) { return ((int)(ipwr2 * x)); }

/* --------------------------------------------------------------------- */
/* set ith element of sparse vector (v, iv) with */
/* nzv nonzeros to val */
/* --------------------------------------------------------------------- */
static void vecset(int n, double v[], int iv[], int *nzv, int i, double val) {
  int k;
  logical set;
  set = false;
#pragma cetus private(k)
#pragma loop name vecset #0
  for (k = 0; k < (*nzv); k++) {
    if (iv[k] == i) {
      v[k] = val;
      set = true;
    }
  }
  if (set == false) {
    v[*nzv] = val;
    iv[*nzv] = i;
    (*nzv) = ((*nzv) + 1);
  }
}
