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
/*

 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http:polybench.sourceforge.net

*/
/* nussinov.c: this file is part of PolyBenchC */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* Include polybench common header. */
#include <polybench.h>
/* Include benchmark-specific header. */
#include "nussinov.h"
/* RNA bases represented as chars, range is [0,3] */
typedef char base;
/* Array initialization. */
static void init_array(int n, base seq[(60 + 0)],
                       int table[(60 + 0)][(60 + 0)]) {
  int i, j;
/* base is AGCT0..3 */
#pragma cetus private(i)
#pragma loop name init_array #0
#pragma cetus parallel
#pragma omp parallel for private(i)
  for (i = 0; i < n; i++) {
    seq[i] = ((base)((i + 1) % 4));
  }
#pragma cetus private(i, j)
#pragma loop name init_array #1
#pragma cetus parallel
#pragma omp parallel for private(i, j)
  for (i = 0; i < n; i++) {
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < n; j++) {
      table[i][j] = 0;
    }
  }
}

/*
DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output.
*/
static void print_array(int n, int table[(60 + 0)][(60 + 0)]) {
  int i, j;
  int t = 0;
  fprintf(stderr, "==BEGIN DUMP_ARRAYS==\n");
  fprintf(stderr, "begin dump: %s", "table");
#pragma cetus private(i, j)
#pragma loop name print_array #0
  for (i = 0; i < n; i++) {
#pragma cetus private(j)
#pragma loop name print_array #0 #0
    for (j = i; j < n; j++) {
      if ((t % 20) == 0) {
        fprintf(stderr, "\n");
      }
      fprintf(stderr, "%d ", table[i][j]);
      t++;
    }
  }
  fprintf(stderr, "\nend   dump: %s\n", "table");
  fprintf(stderr, "==END   DUMP_ARRAYS==\n");
}

/*
Main computational kernel. The whole function will be timed,
   including the call and return.
*/
/*

  Original version by Dave Wonnacott at Haverford College
  <davew@cs.haverford.edu>, with help from Allison Lake, Ting Zhou, and Tian
  Jin, based on algorithm by Nussinov, described in Allison Lake's senior
  thesis.

*/
static void kernel_nussinov(int n, base seq[(60 + 0)],
                            int table[(60 + 0)][(60 + 0)]) {
  int i, j, k;
#pragma scop
#pragma cetus private(i, j, k)
#pragma loop name kernel_nussinov #0
  for (i = (n - 1); i >= 0; i--) {
#pragma cetus private(j, k)
#pragma loop name kernel_nussinov #0 #0
#pragma cetus parallel
#pragma omp parallel for private(j, k)
    for (j = (i + 1); j < n; j++) {
      if ((j - 1) >= 0) {
        table[i][j] =
            ((table[i][j] >= table[i][j - 1]) ? table[i][j] : table[i][j - 1]);
      }
      if ((i + 1) < n) {
        table[i][j] =
            ((table[i][j] >= table[i + 1][j]) ? table[i][j] : table[i + 1][j]);
      }
      if (((j - 1) >= 0) && ((i + 1) < n)) {
        /* don't allow adjacent elements to bond */
        if (i < (j - 1)) {
          table[i][j] = ((table[i][j] >= (table[i + 1][j - 1] +
                                          (((seq[i] + seq[j]) == 3) ? 1 : 0)))
                             ? table[i][j]
                             : (table[i + 1][j - 1] +
                                (((seq[i] + seq[j]) == 3) ? 1 : 0)));
        } else {
          table[i][j] =
              ((table[i][j] >= table[i + 1][j - 1]) ? table[i][j]
                                                    : table[i + 1][j - 1]);
        }
      }
#pragma cetus private(k)
#pragma loop name kernel_nussinov #0 #0 #0
      for (k = (i + 1); k < j; k++) {
        table[i][j] = ((table[i][j] >= (table[i][k] + table[k + 1][j]))
                           ? table[i][j]
                           : (table[i][k] + table[k + 1][j]));
      }
    }
  }
#pragma endscop
}

int main(int argc, char** argv) {
  /* Retrieve problem size. */
  int n = 60;
  /* Variable declarationallocation. */
  base(*seq)[(60 + 0)];
  int(*table)[(60 + 0)][(60 + 0)];
  seq = ((base(*)[(60 + 0)]) polybench_alloc_data(60 + 0, sizeof(base)));
  ;
  table = ((int(*)[(60 + 0)][(60 + 0)])
               polybench_alloc_data((60 + 0) * (60 + 0), sizeof(int)));
  ;
  /* Initialize array(s). */
  init_array(n, *seq, *table);
  /* Start timer. */
  ;
  /* Run kernel. */
  kernel_nussinov(n, *seq, *table);
  /* Stop and print timer. */
  ;
  ;
  /*
  Prevent dead-code elimination. All live-out data must be printed
       by the function call in argument.
  */
  print_array(n, *table);
  /* Be clean. */
  free((void*)seq);
  ;
  free((void*)table);
  ;
  return 0;
}
