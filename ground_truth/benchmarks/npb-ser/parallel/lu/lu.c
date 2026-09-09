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
/*   program applu */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/*  */
/*   driver for the performance evaluation of the solver for */
/*   five coupled parabolicelliptic partial differential equations. */
/*  */
/* --------------------------------------------------------------------- */
#include "applu.incl"
#include "print_results.h"
#include "timers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/* --------------------------------------------------------------------- */
/* grid */
/* --------------------------------------------------------------------- */
/* commoncgcon */
double dxi, deta, dzeta;
double tx1, tx2, tx3;
double ty1, ty2, ty3;
double tz1, tz2, tz3;
int nx, ny, nz;
int nx0, ny0, nz0;
int ist, iend;
int jst, jend;
int ii1, ii2;
int ji1, ji2;
int ki1, ki2;
/* --------------------------------------------------------------------- */
/* dissipation */
/* --------------------------------------------------------------------- */
/* commondisp */
double dx1, dx2, dx3, dx4, dx5;
double dy1, dy2, dy3, dy4, dy5;
double dz1, dz2, dz3, dz4, dz5;
double dssp;
/* --------------------------------------------------------------------- */
/* field variables and residuals */
/* to improve cache performance, second two dimensions padded by 1  */
/* for even number sizes only. */
/* Note: corresponding array (called "v") in routines blts, buts,  */
/* and l2norm are similarly padded */
/* --------------------------------------------------------------------- */
/* commoncvar */
double u[12][(((12 / 2) * 2) + 1)][(((12 / 2) * 2) + 1)][5];
double rsd[12][(((12 / 2) * 2) + 1)][(((12 / 2) * 2) + 1)][5];
double frct[12][(((12 / 2) * 2) + 1)][(((12 / 2) * 2) + 1)][5];
double flux[12][5];
double qs[12][(((12 / 2) * 2) + 1)][(((12 / 2) * 2) + 1)];
double rho_i[12][(((12 / 2) * 2) + 1)][(((12 / 2) * 2) + 1)];
/* --------------------------------------------------------------------- */
/* output control parameters */
/* --------------------------------------------------------------------- */
/* commoncprcon */
int ipr, inorm;
/* --------------------------------------------------------------------- */
/* newton-raphson iteration control parameters */
/* --------------------------------------------------------------------- */
/* commonctscon */
double dt, omega, tolrsd[5], rsdnm[5], errnm[5], frc, ttotal;
int itmax, invert;
/* commoncjac */
double a[12][(((12 / 2) * 2) + 1)][5][5];
double b[12][(((12 / 2) * 2) + 1)][5][5];
double c[12][(((12 / 2) * 2) + 1)][5][5];
double d[12][(((12 / 2) * 2) + 1)][5][5];
/* --------------------------------------------------------------------- */
/* coefficients of the exact solution */
/* --------------------------------------------------------------------- */
/* commoncexact */
double ce[5][13];
/* --------------------------------------------------------------------- */
/* timers */
/* --------------------------------------------------------------------- */
/* commontimer */
double maxtime;
logical timeron;
int main(int argc, char *argv[]) {
  char Class;
  logical verified;
  double mflops;
  double t, tmax, trecs[(11 + 1)];
  int i;
  char *t_names[(11 + 1)];
  /* --------------------------------------------------------------------- */
  /* Setup info for timers */
  /* --------------------------------------------------------------------- */
  FILE *fp;
  if ((fp = fopen("timer.flag", "r")) != ((void *)0)) {
    timeron = true;
    t_names[1] = "total";
    t_names[2] = "rhsx";
    t_names[3] = "rhsy";
    t_names[4] = "rhsz";
    t_names[5] = "rhs";
    t_names[6] = "jacld";
    t_names[7] = "blts";
    t_names[8] = "jacu";
    t_names[9] = "buts";
    t_names[10] = "add";
    t_names[11] = "l2norm";
    fclose(fp);
  } else {
    timeron = false;
  }
  /* --------------------------------------------------------------------- */
  /* read input data */
  /* --------------------------------------------------------------------- */
  read_input();
  /* --------------------------------------------------------------------- */
  /* set up domain sizes */
  /* --------------------------------------------------------------------- */
  domain();
  /* --------------------------------------------------------------------- */
  /* set up coefficients */
  /* --------------------------------------------------------------------- */
  setcoeff();
  /* --------------------------------------------------------------------- */
  /* set the boundary values for dependent variables */
  /* --------------------------------------------------------------------- */
  setbv();
  /* --------------------------------------------------------------------- */
  /* set the initial values for dependent variables */
  /* --------------------------------------------------------------------- */
  setiv();
  /* --------------------------------------------------------------------- */
  /* compute the forcing term based on prescribed exact solution */
  /* --------------------------------------------------------------------- */
  erhs();
  /* --------------------------------------------------------------------- */
  /* perform one SSOR iteration to touch all pages */
  /* --------------------------------------------------------------------- */
  ssor(1);
  /* --------------------------------------------------------------------- */
  /* reset the boundary and initial values */
  /* --------------------------------------------------------------------- */
  setbv();
  setiv();
  /* --------------------------------------------------------------------- */
  /* perform the SSOR iterations */
  /* --------------------------------------------------------------------- */
  ssor(itmax);
  /* --------------------------------------------------------------------- */
  /* compute the solution error */
  /* --------------------------------------------------------------------- */
  error();
  /* --------------------------------------------------------------------- */
  /* compute the surface integral */
  /* --------------------------------------------------------------------- */
  pintgr();
  /* --------------------------------------------------------------------- */
  /* verification test */
  /* --------------------------------------------------------------------- */
  verify(rsdnm, errnm, frc, &Class, &verified);
  mflops = ((((double)itmax) *
             ((((((1984.77 * ((double)nx0)) * ((double)ny0)) * ((double)nz0)) -
                (10923.3 * pow(((double)((nx0 + ny0) + nz0)) / 3.0, 2.0))) +
               ((27770.9 * ((double)((nx0 + ny0) + nz0))) / 3.0)) -
              144010.0)) /
            (maxtime * 1000000.0));
  print_results("LU", Class, nx0, ny0, nz0, itmax, maxtime, mflops,
                "          floating point", verified, "3.3.1", "09 Sep 2026",
                "gcc", "$(CC)", "-lm", "-I../common",
                "-g -Wall -O3 -fopenmp -mcmodel=large",
                "-O3 -fopenmp -mcmodel=large", "(none)");
  /* --------------------------------------------------------------------- */
  /* More timers */
  /* --------------------------------------------------------------------- */
  if (timeron) {
#pragma cetus private(i)
#pragma loop name main #0
    for (i = 1; i <= 11; i++) {
      trecs[i] = timer_read(i);
    }
    tmax = maxtime;
    if (tmax == 0.0) {
      tmax = 1.0;
    }
    printf("  SECTION     Time (secs)\n");
#pragma cetus private(i, t)
#pragma loop name main #1
    for (i = 1; i <= 11; i++) {
      printf("  %-8s:%9.3f  (%6.2f%%)\n", t_names[i], trecs[i],
             (trecs[i] * 100.0) / tmax);
      if (i == 5) {
        t = ((trecs[2] + trecs[3]) + trecs[4]);
        printf("     --> %8s:%9.3f  (%6.2f%%)\n", "sub-rhs", t,
               (t * 100.0) / tmax);
        t = (trecs[i] - t);
        printf("     --> %8s:%9.3f  (%6.2f%%)\n", "rest-rhs", t,
               (t * 100.0) / tmax);
      }
    }
  }
  return 0;
}
