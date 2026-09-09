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
#include "applu.incl"
#include <stdio.h>
#include <stdlib.h>
void read_input() {
  FILE *fp;
  int result;
  /* --------------------------------------------------------------------- */
  /* if input file does not exist, it uses defaults */
  /*    ipr = 1 for detailed progress output */
  /*    inorm = how often the norm is printed (once every inorm iterations) */
  /*    itmax = number of pseudo time steps */
  /*    dt = time step */
  /*    omega 1 over-relaxation factor for SSOR */
  /*    tolrsd = steady state residual tolerance levels */
  /*    nx, ny, nz = number of grid points in x, y, z directions */
  /* --------------------------------------------------------------------- */
  printf("\n\n NAS Parallel Benchmarks (NPB3.3-SER-C) - LU Benchmark\n\n");
  if ((fp = fopen("inputlu.data", "r")) != ((void *)0)) {
    printf("Reading from input file inputlu.data\n");
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%d%d", &ipr, &inorm);
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%d", &itmax);
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%lf", &dt);
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%lf", &omega);
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%lf%lf%lf%lf%lf", &tolrsd[0], &tolrsd[1], &tolrsd[2],
                    &tolrsd[3], &tolrsd[4]);
    while (fgetc(fp) != '\n') {
      ;
    }
    while (fgetc(fp) != '\n') {
      ;
    }
    result = fscanf(fp, "%d%d%d", &nx0, &ny0, &nz0);
    fclose(fp);
  } else {
    ipr = 1;
    inorm = 50;
    itmax = 50;
    dt = 0.5;
    omega = 1.2;
    tolrsd[0] = 1.0E-8;
    tolrsd[1] = 1.0E-8;
    tolrsd[2] = 1.0E-8;
    tolrsd[3] = 1.0E-8;
    tolrsd[4] = 1.0E-8;
    nx0 = 12;
    ny0 = 12;
    nz0 = 12;
  }
  /* --------------------------------------------------------------------- */
  /* check problem size */
  /* --------------------------------------------------------------------- */
  if (((nx0 < 4) || (ny0 < 4)) || (nz0 < 4)) {
    printf("     PROBLEM SIZE IS TOO SMALL - \n"
           "     SET EACH OF NX, NY AND NZ AT LEAST EQUAL TO 5\n");
    exit(1);
  }
  if (((nx0 > 12) || (ny0 > 12)) || (nz0 > 12)) {
    printf("     PROBLEM SIZE IS TOO LARGE - \n"
           "     NX, NY AND NZ SHOULD BE EQUAL TO \n"
           "     ISIZ1, ISIZ2 AND ISIZ3 RESPECTIVELY\n");
    exit(1);
  }
  printf(" Size: %4dx%4dx%4d\n", nx0, ny0, nz0);
  printf(" Iterations: %4d\n", itmax);
  printf("\n");
}
