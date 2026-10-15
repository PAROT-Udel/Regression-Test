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
/* --------------------------------------------------------------------- */
/* FT benchmark */
/* --------------------------------------------------------------------- */
#include "global.h"
#include "print_results.h"
#include <math.h>
#include <stdio.h>
static char getclass();
logical timers_enabled;
int main(int argc, char *argv[]) {
  int niter;
  char Class;
  double total_time, mflops;
  logical verified;
  FILE *fp;
  if ((fp = fopen("timer.flag", "r")) != ((void *)0)) {
    timers_enabled = true;
    fclose(fp);
  } else {
    timers_enabled = false;
  }
  niter = 6;
  printf("\n\n NAS Parallel Benchmarks (NPB3.3-SER-C) - FT Benchmark\n\n");
  printf(" Size                : %4dx%4dx%4d\n", 64, 64, 64);
  printf(" Iterations          :     %10d\n", niter);
  printf("\n");
  Class = getclass();
  appft(niter, &total_time, &verified);
  if (total_time != 0.0) {
    mflops = (((1.0E-6 * ((double)262144)) *
               ((14.8157 + (7.19641 * log((double)262144))) +
                ((5.23518 + (7.21113 * log((double)262144))) * niter))) /
              total_time);
  } else {
    mflops = 0.0;
  }
  print_results("FT", Class, 64, 64, 64, niter, total_time, mflops,
                "          floating point", verified, "3.3.1", "09 Sep 2026",
                "gcc", "$(CC)", "-lm", "-I../common",
                "-g -Wall -O3 -fopenmp -mcmodel=large",
                "-O3 -fopenmp -mcmodel=large", "randdp");
  return 0;
}

static char getclass() { return 'S'; }
