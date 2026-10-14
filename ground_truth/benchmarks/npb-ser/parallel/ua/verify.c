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
#include <math.h>
#include <stdio.h>
void verify(char *Class, logical *verified) {
  double norm, epsilon, norm_dif, norm_ref;
  /* tolerance level */
  epsilon = 1.0E-8;
  /* compute the temperature integral over the whole domain */
  norm = calc_norm();
  (*verified) = true;
  if ((*Class) == 'S') {
    norm_ref = 0.001890013110962;
  } else {
    if ((*Class) == 'W') {
      norm_ref = 2.569794837076E-5;
    } else {
      if ((*Class) == 'A') {
        norm_ref = 8.939996281443E-5;
      } else {
        if ((*Class) == 'B') {
          norm_ref = 4.507561922901E-5;
        } else {
          if ((*Class) == 'C') {
            norm_ref = 1.5447365871E-5;
          } else {
            if ((*Class) == 'D') {
              norm_ref = 1.577586272355E-6;
            } else {
              (*Class) = 'U';
              norm_ref = 1.0;
              (*verified) = false;
            }
          }
        }
      }
    }
  }
  norm_dif = fabs((norm - norm_ref) / norm_ref);
  /* --------------------------------------------------------------------- */
  /* Output the comparison of computed results to known cases. */
  /* --------------------------------------------------------------------- */
  printf("\n");
  if ((*Class) != 'U') {
    printf(" Verification being performed for class %c\n", *Class);
    printf(" accuracy setting for epsilon = %20.13E\n", epsilon);
  } else {
    printf(" Unknown class\n");
  }
  if ((*Class) != 'U') {
    printf(" Comparison of temperature integrals\n");
  } else {
    printf(" Temperature integral\n");
  }
  if ((*Class) == 'U') {
    printf("          %20.13E\n", norm);
  } else {
    if (norm_dif <= epsilon) {
      printf("          %20.13E%20.13E%20.13E\n", norm, norm_ref, norm_dif);
    } else {
      (*verified) = false;
      printf(" FAILURE: %20.13E%20.13E%20.13E\n", norm, norm_ref, norm_dif);
    }
  }
  if ((*Class) == 'U') {
    printf(" No reference values provided\n");
    printf(" No verification performed\n");
  } else {
    if (*verified) {
      printf(" Verification Successful\n");
    } else {
      printf(" Verification failed\n");
    }
  }
}
