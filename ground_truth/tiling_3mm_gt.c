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
typedef long unsigned int size_t;
typedef __builtin_va_list __gnuc_va_list;
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;
typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
struct named_tiling_3mm_i_335 {
  int __val[2];
};

typedef struct named_tiling_3mm_i_335 __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;
typedef int __daddr_t;
typedef int __key_t;
typedef int __clockid_t;
typedef void *__timer_t;
typedef long int __blksize_t;
typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;
typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;
typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;
typedef long int __fsword_t;
typedef long int __ssize_t;
typedef long int __syscall_slong_t;
typedef unsigned long int __syscall_ulong_t;
typedef __off64_t __loff_t;
typedef char *__caddr_t;
typedef long int __intptr_t;
typedef unsigned int __socklen_t;
typedef int __sig_atomic_t;
union named_tiling_3mm_i_614 {
  unsigned int __wch;
  char __wchb[4];
};

struct named_tiling_3mm_i_606 {
  int __count;
  union named_tiling_3mm_i_614 __value;
};

typedef struct named_tiling_3mm_i_606 __mbstate_t;
struct _G_fpos_t {
  __off_t __pos;
  __mbstate_t __state;
};

typedef struct _G_fpos_t __fpos_t;
struct _G_fpos64_t {
  __off64_t __pos;
  __mbstate_t __state;
};

typedef struct _G_fpos64_t __fpos64_t;
struct _IO_FILE;

typedef struct _IO_FILE __FILE;
struct _IO_FILE;

typedef struct _IO_FILE FILE;
struct _IO_FILE;

struct _IO_marker;

struct _IO_codecvt;

struct _IO_wide_data;

typedef void _IO_lock_t;
struct _IO_FILE {
  int _flags;
  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;
  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;
  struct _IO_marker *_markers;
  struct _IO_FILE *_chain;
  int _fileno;
  int _flags2;
  __off_t _old_offset;
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];
  _IO_lock_t *_lock;
  __off64_t _offset;
  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  size_t __pad5;
  int _mode;
  char _unused2[(((15 * sizeof(int)) - (4 * sizeof(void *))) - sizeof(size_t))];
};

struct _IO_FILE;

typedef __fpos_t fpos_t;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
extern int remove(const char *__filename);
extern int rename(const char *__old, const char *__new);
extern int fclose(FILE *__stream);
extern FILE *tmpfile(void);
extern char *tmpnam(char[20]);
extern int fflush(FILE *__stream);
extern FILE *fopen(const char *__filename, const char *__modes);
extern FILE *freopen(const char *__filename, const char *__modes,
                     FILE *__stream);
extern void setbuf(FILE *__stream, char *__buf);
extern int setvbuf(FILE *__stream, char *__buf, int __modes, size_t __n);
extern int fprintf(FILE *__stream, const char *__format, ...);
extern int printf(const char *__format, ...);
extern int sprintf(char *__s, const char *__format, ...);
extern int vfprintf(FILE *__s, const char *__format, __gnuc_va_list __arg);
extern int vprintf(const char *__format, __gnuc_va_list __arg);
extern int vsprintf(char *__s, const char *__format, __gnuc_va_list __arg);
extern int snprintf(char *__s, size_t __maxlen, const char *__format, ...);
extern int vsnprintf(char *__s, size_t __maxlen, const char *__format,
                     __gnuc_va_list __arg);
extern int fscanf(FILE *__stream, const char *__format, ...);
extern int scanf(const char *__format, ...);
extern int sscanf(const char *__s, const char *__format, ...);
typedef float _Float32;
typedef double _Float64;
typedef double _Float32x;
typedef long double _Float64x;
extern int fscanf(FILE *__stream, const char *__format, ...);
extern int scanf(const char *__format, ...);
extern int sscanf(const char *__s, const char *__format, ...);
extern int vfscanf(FILE *__s, const char *__format, __gnuc_va_list __arg);
extern int vscanf(const char *__format, __gnuc_va_list __arg);
extern int vsscanf(const char *__s, const char *__format, __gnuc_va_list __arg);
extern int vfscanf(FILE *__s, const char *__format, __gnuc_va_list __arg);
extern int vscanf(const char *__format, __gnuc_va_list __arg);
extern int vsscanf(const char *__s, const char *__format, __gnuc_va_list __arg);
extern int fgetc(FILE *__stream);
extern int getc(FILE *__stream);
extern int getchar(void);
extern int fputc(int __c, FILE *__stream);
extern int putc(int __c, FILE *__stream);
extern int putchar(int __c);
extern char *fgets(char *__s, int __n, FILE *__stream);
extern int fputs(const char *__s, FILE *__stream);
extern int puts(const char *__s);
extern int ungetc(int __c, FILE *__stream);
extern size_t fread(void *__ptr, size_t __size, size_t __n, FILE *__stream);
extern size_t fwrite(const void *__ptr, size_t __size, size_t __n, FILE *__s);
extern int fseek(FILE *__stream, long int __off, int __whence);
extern long int ftell(FILE *__stream);
extern void rewind(FILE *__stream);
extern int fgetpos(FILE *__stream, fpos_t *__pos);
extern int fsetpos(FILE *__stream, const fpos_t *__pos);
extern void clearerr(FILE *__stream);
extern int feof(FILE *__stream);
extern int ferror(FILE *__stream);
extern void perror(const char *__s);
extern int __uflow(FILE *);
extern int __overflow(FILE *, int);
typedef int wchar_t;
struct named_tiling_3mm_i_2798 {
  int quot;
  int rem;
};

typedef struct named_tiling_3mm_i_2798 div_t;
struct named_tiling_3mm_i_2817 {
  long int quot;
  long int rem;
};

typedef struct named_tiling_3mm_i_2817 ldiv_t;
struct named_tiling_3mm_i_2841 {
  long long int quot;
  long long int rem;
};

typedef struct named_tiling_3mm_i_2841 lldiv_t;
extern size_t __ctype_get_mb_cur_max(void);
extern double atof(const char *__nptr);
extern int atoi(const char *__nptr);
extern long int atol(const char *__nptr);
extern long long int atoll(const char *__nptr);
extern double strtod(const char *__nptr, char **__endptr);
extern float strtof(const char *__nptr, char **__endptr);
extern long double strtold(const char *__nptr, char **__endptr);
extern long int strtol(const char *__nptr, char **__endptr, int __base);
extern unsigned long int strtoul(const char *__nptr, char **__endptr,
                                 int __base);
extern long long int strtoll(const char *__nptr, char **__endptr, int __base);
extern unsigned long long int strtoull(const char *__nptr, char **__endptr,
                                       int __base);
extern int rand(void);
extern void srand(unsigned int __seed);
extern void *malloc(size_t __size);
extern void *calloc(size_t __nmemb, size_t __size);
extern void *realloc(void *__ptr, size_t __size);
extern void free(void *__ptr);
extern void *aligned_alloc(size_t __alignment, size_t __size);
extern void abort(void);
extern int atexit(void (*__func)(void));
extern int at_quick_exit(void (*__func)(void));
extern void exit(int __status);
extern void quick_exit(int __status);
extern void _Exit(int __status);
extern char *getenv(const char *__name);
extern int system(const char *__command);
typedef int (*__compar_fn_t)(const void *, const void *);
extern void *bsearch(const void *__key, const void *__base, size_t __nmemb,
                     size_t __size, __compar_fn_t __compar);
extern void qsort(void *__base, size_t __nmemb, size_t __size,
                  __compar_fn_t __compar);
extern int abs(int __x);
extern long int labs(long int __x);
extern long long int llabs(long long int __x);
extern div_t div(int __numer, int __denom);
extern ldiv_t ldiv(long int __numer, long int __denom);
extern lldiv_t lldiv(long long int __numer, long long int __denom);
extern int mblen(const char *__s, size_t __n);
extern int mbtowc(wchar_t *__pwc, const char *__s, size_t __n);
extern int wctomb(char *__s, wchar_t __wchar);
extern size_t mbstowcs(wchar_t *__pwcs, const char *__s, size_t __n);
extern size_t wcstombs(char *__s, const wchar_t *__pwcs, size_t __n);
void init_array(int ni, int nj, int nk, double A[ni][nk], double B[nk][nj],
                double C[ni][nj]) {
  int i;
  int j;
#pragma loop name init_array #0
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma loop name init_array #0
#pragma omp parallel for private(i, j)
  for (i = 0; i < ni; i++) {
#pragma loop name init_array #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
#pragma loop name init_array #0 #0
    for (j = 0; j < nk; j++) {
      A[i][j] = (((double)(((i * j) + 1) % ni)) / ni);
    }
  }
#pragma loop name init_array #1
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma loop name init_array #1
#pragma omp parallel for private(i, j)
  for (i = 0; i < nk; i++) {
#pragma loop name init_array #1 #0
#pragma cetus private(j)
#pragma cetus private(j)
#pragma loop name init_array #1 #0
    for (j = 0; j < nj; j++) {
      B[i][j] = (((double)(((i * j) + 2) % nj)) / nj);
    }
  }
#pragma loop name init_array #2
#pragma cetus parallel
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma loop name init_array #2
#pragma omp parallel for private(i, j)
  for (i = 0; i < ni; i++) {
#pragma loop name init_array #2 #0
#pragma cetus private(j)
#pragma cetus private(j)
#pragma loop name init_array #2 #0
    for (j = 0; j < nj; j++) {
      C[i][j] = 0.0;
    }
  }
}

void matrix_multiply(double A[10000][10000], double B[10000][10000],
                     double C[10000][10000]) {
#pragma experimental section start
  int i;
  int j;
  int k;
  int cetus_tile_matrix_multiply_0_j = 16;
  int j_cetus_cross;
  int cetus_tile_matrix_multiply_0_k = 16;
  int k_cetus_cross;
  int cetus_tile_matrix_multiply_0_i = 8;
  int i_cetus_cross;
#pragma cetus parallel
#pragma cetus private(i, i_cetus_cross, j, j_cetus_cross, k, k_cetus_cross)
#pragma cetus reduction(+ : C[i][j])
#pragma loop name matrix_multiply #0
#pragma c_paw_tiling matrix_multiply #0 = j #16
#pragma c_paw_tiling matrix_multiply #0 = k #16
#pragma c_paw_tiling matrix_multiply #0 = i #8
#pragma omp parallel for private(i, i_cetus_cross, j, j_cetus_cross, k, \
                                     k_cetus_cross) reduction(+ : C[i][j])
  for (i_cetus_cross = 0; i_cetus_cross < 10000;
       i_cetus_cross += cetus_tile_matrix_multiply_0_i) {
#pragma cetus private(i, j, j_cetus_cross, k, k_cetus_cross)
#pragma cetus reduction(+ : C[i][j])
#pragma loop name matrix_multiply #0 #0
    for (k_cetus_cross = 0; k_cetus_cross < 10000;
         k_cetus_cross += cetus_tile_matrix_multiply_0_k) {
#pragma cetus private(i, j, j_cetus_cross, k)
#pragma cetus reduction(+ : C[i][j])
#pragma loop name matrix_multiply #0 #0 #0
      for (j_cetus_cross = 0; j_cetus_cross < 10000;
           j_cetus_cross += cetus_tile_matrix_multiply_0_j) {
#pragma cetus private(i, j, k)
#pragma loop name matrix_multiply #0 #0 #0 #0
        for (i = i_cetus_cross;
             i < MIN(10000, (cetus_tile_matrix_multiply_0_i + i_cetus_cross));
             i++) {
#pragma loop name matrix_multiply #0 #0
#pragma cetus private(j, k)
#pragma cetus private(j, k)
#pragma cetus reduction(+ : C[i][j])
#pragma loop name matrix_multiply #0 #0 #0 #0 #0
          for (k = k_cetus_cross;
               k < MIN(10000, (cetus_tile_matrix_multiply_0_k + k_cetus_cross));
               k++) {
#pragma loop name matrix_multiply #0 #0 #0
/* #pragma cetus reduction(+: C[i][j])  */
#pragma cetus private(j)
#pragma cetus private(j)
#pragma loop name matrix_multiply #0 #0 #0 #0 #0 #0
            for (j = j_cetus_cross;
                 j <
                 MIN(10000, (cetus_tile_matrix_multiply_0_j + j_cetus_cross));
                 j++) {
              C[i][j] += (A[i][k] * B[k][j]);
            }
          }
        }
      }
    }
  }
#pragma experimental section stop
}

void print_array(int ni, int nj, double C[ni][nj]) {
  int i;
  int j;
#pragma loop name print_array #0
#pragma cetus private(i, j)
#pragma cetus private(i, j)
#pragma loop name print_array #0
  for (i = 0; i < ni; i++) {
#pragma loop name print_array #0 #0
#pragma cetus private(j)
#pragma cetus private(j)
#pragma loop name print_array #0 #0
    for (j = 0; j < nj; j++) {
      printf("%0.2lf ", C[i][j]);
    }
    printf("\n");
  }
}

int main() {
  double(*A)[10000] = malloc((10000 * 10000) * sizeof(double));
  double(*B)[10000] = malloc((10000 * 10000) * sizeof(double));
  double(*C)[10000] = malloc((10000 * 10000) * sizeof(double));
  init_array(10000, 10000, 10000, A, B, C);
  matrix_multiply(A, B, C);
  print_array(10000, 10000, C);
  free(A);
  free(B);
  free(C);
  return 0;
}
