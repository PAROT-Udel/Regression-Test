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
int initialize(int a[], int m);
int main() {
  int a[30000], n;
  int b[30000];
  int c[30000];
  int i;
  int j;
  int _ret_val_0;
  initialize(b, n);
#pragma cetus private(c, i, j)
#pragma loop name main #0
#pragma cetus parallel
  for (i = 0; i < n; i++) {
#pragma cetus firstprivate(c)
#pragma cetus private(j)
#pragma cetus lastprivate(c)
#pragma loop name main #0 #0
    for (j = 0; j < n; j++) {
      a[((2 * n) * i) + j] = b[i];
      c[i] = a[(((2 * n) * i) + j) + n];
    }
  }
  return _ret_val_0;
}

int initialize(int a[], int m) {
  int i;
  int _ret_val_0;
#pragma cetus private(i)
#pragma loop name initialize #0
#pragma cetus parallel
  for (i = 0; i < m; i++) {
    a[i] = i;
  }
  _ret_val_0 = 0;
  return _ret_val_0;
}
