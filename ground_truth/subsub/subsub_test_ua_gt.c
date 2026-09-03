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
struct named_subsub_test_ua_i_335 {
  int __val[2];
};

typedef struct named_subsub_test_ua_i_335 __fsid_t;
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
union named_subsub_test_ua_i_614 {
  unsigned int __wch;
  char __wchb[4];
};

struct named_subsub_test_ua_i_606 {
  int __count;
  union named_subsub_test_ua_i_614 __value;
};

typedef struct named_subsub_test_ua_i_606 __mbstate_t;
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
typedef float float_t;
typedef double double_t;
extern int __fpclassify(double __value);
extern int __signbit(double __value);
extern int __isinf(double __value);
extern int __finite(double __value);
extern int __isnan(double __value);
extern int __iseqsig(double __x, double __y);
extern int __issignaling(double __value);
extern double acos(double __x);
extern double __acos(double __x);
extern double asin(double __x);
extern double __asin(double __x);
extern double atan(double __x);
extern double __atan(double __x);
extern double atan2(double __y, double __x);
extern double __atan2(double __y, double __x);
extern double cos(double __x);
extern double __cos(double __x);
extern double sin(double __x);
extern double __sin(double __x);
extern double tan(double __x);
extern double __tan(double __x);
extern double cosh(double __x);
extern double __cosh(double __x);
extern double sinh(double __x);
extern double __sinh(double __x);
extern double tanh(double __x);
extern double __tanh(double __x);
extern double acosh(double __x);
extern double __acosh(double __x);
extern double asinh(double __x);
extern double __asinh(double __x);
extern double atanh(double __x);
extern double __atanh(double __x);
extern double exp(double __x);
extern double __exp(double __x);
extern double frexp(double __x, int *__exponent);
extern double __frexp(double __x, int *__exponent);
extern double ldexp(double __x, int __exponent);
extern double __ldexp(double __x, int __exponent);
extern double log(double __x);
extern double __log(double __x);
extern double log10(double __x);
extern double __log10(double __x);
extern double modf(double __x, double *__iptr);
extern double __modf(double __x, double *__iptr);
extern double expm1(double __x);
extern double __expm1(double __x);
extern double log1p(double __x);
extern double __log1p(double __x);
extern double logb(double __x);
extern double __logb(double __x);
extern double exp2(double __x);
extern double __exp2(double __x);
extern double log2(double __x);
extern double __log2(double __x);
extern double pow(double __x, double __y);
extern double __pow(double __x, double __y);
extern double sqrt(double __x);
extern double __sqrt(double __x);
extern double hypot(double __x, double __y);
extern double __hypot(double __x, double __y);
extern double cbrt(double __x);
extern double __cbrt(double __x);
extern double ceil(double __x);
extern double __ceil(double __x);
extern double fabs(double __x);
extern double __fabs(double __x);
extern double floor(double __x);
extern double __floor(double __x);
extern double fmod(double __x, double __y);
extern double __fmod(double __x, double __y);
extern double copysign(double __x, double __y);
extern double __copysign(double __x, double __y);
extern double nan(const char *__tagb);
extern double __nan(const char *__tagb);
extern double erf(double);
extern double __erf(double);
extern double erfc(double);
extern double __erfc(double);
extern double lgamma(double);
extern double __lgamma(double);
extern double tgamma(double);
extern double __tgamma(double);
extern double rint(double __x);
extern double __rint(double __x);
extern double nextafter(double __x, double __y);
extern double __nextafter(double __x, double __y);
extern double nexttoward(double __x, long double __y);
extern double __nexttoward(double __x, long double __y);
extern double remainder(double __x, double __y);
extern double __remainder(double __x, double __y);
extern double scalbn(double __x, int __n);
extern double __scalbn(double __x, int __n);
extern int ilogb(double __x);
extern int __ilogb(double __x);
extern double scalbln(double __x, long int __n);
extern double __scalbln(double __x, long int __n);
extern double nearbyint(double __x);
extern double __nearbyint(double __x);
extern double round(double __x);
extern double __round(double __x);
extern double trunc(double __x);
extern double __trunc(double __x);
extern double remquo(double __x, double __y, int *__quo);
extern double __remquo(double __x, double __y, int *__quo);
extern long int lrint(double __x);
extern long int __lrint(double __x);
extern long long int llrint(double __x);
extern long long int __llrint(double __x);
extern long int lround(double __x);
extern long int __lround(double __x);
extern long long int llround(double __x);
extern long long int __llround(double __x);
extern double fdim(double __x, double __y);
extern double __fdim(double __x, double __y);
extern double fmax(double __x, double __y);
extern double __fmax(double __x, double __y);
extern double fmin(double __x, double __y);
extern double __fmin(double __x, double __y);
extern double fma(double __x, double __y, double __z);
extern double __fma(double __x, double __y, double __z);
extern int __fpclassifyf(float __value);
extern int __signbitf(float __value);
extern int __isinff(float __value);
extern int __finitef(float __value);
extern int __isnanf(float __value);
extern int __iseqsigf(float __x, float __y);
extern int __issignalingf(float __value);
extern float acosf(float __x);
extern float __acosf(float __x);
extern float asinf(float __x);
extern float __asinf(float __x);
extern float atanf(float __x);
extern float __atanf(float __x);
extern float atan2f(float __y, float __x);
extern float __atan2f(float __y, float __x);
extern float cosf(float __x);
extern float __cosf(float __x);
extern float sinf(float __x);
extern float __sinf(float __x);
extern float tanf(float __x);
extern float __tanf(float __x);
extern float coshf(float __x);
extern float __coshf(float __x);
extern float sinhf(float __x);
extern float __sinhf(float __x);
extern float tanhf(float __x);
extern float __tanhf(float __x);
extern float acoshf(float __x);
extern float __acoshf(float __x);
extern float asinhf(float __x);
extern float __asinhf(float __x);
extern float atanhf(float __x);
extern float __atanhf(float __x);
extern float expf(float __x);
extern float __expf(float __x);
extern float frexpf(float __x, int *__exponent);
extern float __frexpf(float __x, int *__exponent);
extern float ldexpf(float __x, int __exponent);
extern float __ldexpf(float __x, int __exponent);
extern float logf(float __x);
extern float __logf(float __x);
extern float log10f(float __x);
extern float __log10f(float __x);
extern float modff(float __x, float *__iptr);
extern float __modff(float __x, float *__iptr);
extern float expm1f(float __x);
extern float __expm1f(float __x);
extern float log1pf(float __x);
extern float __log1pf(float __x);
extern float logbf(float __x);
extern float __logbf(float __x);
extern float exp2f(float __x);
extern float __exp2f(float __x);
extern float log2f(float __x);
extern float __log2f(float __x);
extern float powf(float __x, float __y);
extern float __powf(float __x, float __y);
extern float sqrtf(float __x);
extern float __sqrtf(float __x);
extern float hypotf(float __x, float __y);
extern float __hypotf(float __x, float __y);
extern float cbrtf(float __x);
extern float __cbrtf(float __x);
extern float ceilf(float __x);
extern float __ceilf(float __x);
extern float fabsf(float __x);
extern float __fabsf(float __x);
extern float floorf(float __x);
extern float __floorf(float __x);
extern float fmodf(float __x, float __y);
extern float __fmodf(float __x, float __y);
extern float copysignf(float __x, float __y);
extern float __copysignf(float __x, float __y);
extern float nanf(const char *__tagb);
extern float __nanf(const char *__tagb);
extern float erff(float);
extern float __erff(float);
extern float erfcf(float);
extern float __erfcf(float);
extern float lgammaf(float);
extern float __lgammaf(float);
extern float tgammaf(float);
extern float __tgammaf(float);
extern float rintf(float __x);
extern float __rintf(float __x);
extern float nextafterf(float __x, float __y);
extern float __nextafterf(float __x, float __y);
extern float nexttowardf(float __x, long double __y);
extern float __nexttowardf(float __x, long double __y);
extern float remainderf(float __x, float __y);
extern float __remainderf(float __x, float __y);
extern float scalbnf(float __x, int __n);
extern float __scalbnf(float __x, int __n);
extern int ilogbf(float __x);
extern int __ilogbf(float __x);
extern float scalblnf(float __x, long int __n);
extern float __scalblnf(float __x, long int __n);
extern float nearbyintf(float __x);
extern float __nearbyintf(float __x);
extern float roundf(float __x);
extern float __roundf(float __x);
extern float truncf(float __x);
extern float __truncf(float __x);
extern float remquof(float __x, float __y, int *__quo);
extern float __remquof(float __x, float __y, int *__quo);
extern long int lrintf(float __x);
extern long int __lrintf(float __x);
extern long long int llrintf(float __x);
extern long long int __llrintf(float __x);
extern long int lroundf(float __x);
extern long int __lroundf(float __x);
extern long long int llroundf(float __x);
extern long long int __llroundf(float __x);
extern float fdimf(float __x, float __y);
extern float __fdimf(float __x, float __y);
extern float fmaxf(float __x, float __y);
extern float __fmaxf(float __x, float __y);
extern float fminf(float __x, float __y);
extern float __fminf(float __x, float __y);
extern float fmaf(float __x, float __y, float __z);
extern float __fmaf(float __x, float __y, float __z);
extern int __fpclassifyl(long double __value);
extern int __signbitl(long double __value);
extern int __isinfl(long double __value);
extern int __finitel(long double __value);
extern int __isnanl(long double __value);
extern int __iseqsigl(long double __x, long double __y);
extern int __issignalingl(long double __value);
extern long double acosl(long double __x);
extern long double __acosl(long double __x);
extern long double asinl(long double __x);
extern long double __asinl(long double __x);
extern long double atanl(long double __x);
extern long double __atanl(long double __x);
extern long double atan2l(long double __y, long double __x);
extern long double __atan2l(long double __y, long double __x);
extern long double cosl(long double __x);
extern long double __cosl(long double __x);
extern long double sinl(long double __x);
extern long double __sinl(long double __x);
extern long double tanl(long double __x);
extern long double __tanl(long double __x);
extern long double coshl(long double __x);
extern long double __coshl(long double __x);
extern long double sinhl(long double __x);
extern long double __sinhl(long double __x);
extern long double tanhl(long double __x);
extern long double __tanhl(long double __x);
extern long double acoshl(long double __x);
extern long double __acoshl(long double __x);
extern long double asinhl(long double __x);
extern long double __asinhl(long double __x);
extern long double atanhl(long double __x);
extern long double __atanhl(long double __x);
extern long double expl(long double __x);
extern long double __expl(long double __x);
extern long double frexpl(long double __x, int *__exponent);
extern long double __frexpl(long double __x, int *__exponent);
extern long double ldexpl(long double __x, int __exponent);
extern long double __ldexpl(long double __x, int __exponent);
extern long double logl(long double __x);
extern long double __logl(long double __x);
extern long double log10l(long double __x);
extern long double __log10l(long double __x);
extern long double modfl(long double __x, long double *__iptr);
extern long double __modfl(long double __x, long double *__iptr);
extern long double expm1l(long double __x);
extern long double __expm1l(long double __x);
extern long double log1pl(long double __x);
extern long double __log1pl(long double __x);
extern long double logbl(long double __x);
extern long double __logbl(long double __x);
extern long double exp2l(long double __x);
extern long double __exp2l(long double __x);
extern long double log2l(long double __x);
extern long double __log2l(long double __x);
extern long double powl(long double __x, long double __y);
extern long double __powl(long double __x, long double __y);
extern long double sqrtl(long double __x);
extern long double __sqrtl(long double __x);
extern long double hypotl(long double __x, long double __y);
extern long double __hypotl(long double __x, long double __y);
extern long double cbrtl(long double __x);
extern long double __cbrtl(long double __x);
extern long double ceill(long double __x);
extern long double __ceill(long double __x);
extern long double fabsl(long double __x);
extern long double __fabsl(long double __x);
extern long double floorl(long double __x);
extern long double __floorl(long double __x);
extern long double fmodl(long double __x, long double __y);
extern long double __fmodl(long double __x, long double __y);
extern long double copysignl(long double __x, long double __y);
extern long double __copysignl(long double __x, long double __y);
extern long double nanl(const char *__tagb);
extern long double __nanl(const char *__tagb);
extern long double erfl(long double);
extern long double __erfl(long double);
extern long double erfcl(long double);
extern long double __erfcl(long double);
extern long double lgammal(long double);
extern long double __lgammal(long double);
extern long double tgammal(long double);
extern long double __tgammal(long double);
extern long double rintl(long double __x);
extern long double __rintl(long double __x);
extern long double nextafterl(long double __x, long double __y);
extern long double __nextafterl(long double __x, long double __y);
extern long double nexttowardl(long double __x, long double __y);
extern long double __nexttowardl(long double __x, long double __y);
extern long double remainderl(long double __x, long double __y);
extern long double __remainderl(long double __x, long double __y);
extern long double scalbnl(long double __x, int __n);
extern long double __scalbnl(long double __x, int __n);
extern int ilogbl(long double __x);
extern int __ilogbl(long double __x);
extern long double scalblnl(long double __x, long int __n);
extern long double __scalblnl(long double __x, long int __n);
extern long double nearbyintl(long double __x);
extern long double __nearbyintl(long double __x);
extern long double roundl(long double __x);
extern long double __roundl(long double __x);
extern long double truncl(long double __x);
extern long double __truncl(long double __x);
extern long double remquol(long double __x, long double __y, int *__quo);
extern long double __remquol(long double __x, long double __y, int *__quo);
extern long int lrintl(long double __x);
extern long int __lrintl(long double __x);
extern long long int llrintl(long double __x);
extern long long int __llrintl(long double __x);
extern long int lroundl(long double __x);
extern long int __lroundl(long double __x);
extern long long int llroundl(long double __x);
extern long long int __llroundl(long double __x);
extern long double fdiml(long double __x, long double __y);
extern long double __fdiml(long double __x, long double __y);
extern long double fmaxl(long double __x, long double __y);
extern long double __fmaxl(long double __x, long double __y);
extern long double fminl(long double __x, long double __y);
extern long double __fminl(long double __x, long double __y);
extern long double fmal(long double __x, long double __y, long double __z);
extern long double __fmal(long double __x, long double __y, long double __z);
enum subsub_test_ua_i_12196 {
  FP_NAN = 0,
  FP_INFINITE = 1,
  FP_ZERO = 2,
  FP_SUBNORMAL = 3,
  FP_NORMAL = 4
};
typedef int wchar_t;
struct named_subsub_test_ua_i_12234 {
  int quot;
  int rem;
};

typedef struct named_subsub_test_ua_i_12234 div_t;
struct named_subsub_test_ua_i_12253 {
  long int quot;
  long int rem;
};

typedef struct named_subsub_test_ua_i_12253 ldiv_t;
struct named_subsub_test_ua_i_12277 {
  long long int quot;
  long long int rem;
};

typedef struct named_subsub_test_ua_i_12277 lldiv_t;
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
extern int fre, niter, nmxh;
extern double alpha, dlmin, dtime;
extern int nelt, ntot, nmor, nvertex;
extern double x0, _y0, z0, time;
extern double ta1[8800][5][5][5];
extern double ta2[8800][5][5][5];
extern double trhs[8800][5][5][5];
extern double t[8800][5][5][5];
extern double tmult[8800][5][5][5];
extern double dpcelm[8800][5][5][5];
extern double pdiff[8800][5][5][5];
extern double pdiffp[8800][5][5][5];
extern double umor[334600];
extern double mormult[334600];
extern double tmort[334600];
extern double tmmor[334600];
extern double rmor[334600];
extern double dpcmor[334600];
extern double pmorx[334600];
extern double ppmor[334600];
extern int idmo[8800][6][2][2][5][5];
extern int idel[8800][6][5][5];
extern int sje[8800][6][2][2];
extern int sje_new[8800][6][2][2];
extern int ijel[8800][6][2];
extern int ijel_new[8800][6][2];
extern int cbc[8800][6];
extern int cbc_new[8800][6];
extern int vassign[8800][8];
extern int emo[(8 * 8800)][8][2];
extern int nemo[(8 * 8800)];
extern int diagn[8800][12][2];
extern int tree[8800];
extern int treenew[8800];
extern int mt_to_id[8800];
extern int mt_to_id_old[8800];
extern int id_to_mt[8800];
extern int newc[8800];
extern int newi[8800];
extern int newe[8800];
extern int ref_front_id[8800];
extern int ich[8800];
extern int size_e[8800];
extern int front[8800];
extern int action[8800];
extern double qbnew[2][5][(5 - 2)];
extern double bqnew[2][(5 - 2)][(5 - 2)];
extern double zgm1[5];
extern double wxm1[5];
extern double w3m1[5][5][5];
extern double xc[8800][8];
extern double yc[8800][8];
extern double zc[8800][8];
extern double xc_new[8800][8];
extern double yc_new[8800][8];
extern double zc_new[8800][8];
extern double dxm1[5][5];
extern double dxtm1[5][5];
extern double wdtdr[5][5];
extern double ixm31[((5 * 2) - 1)][5];
extern double ixtm31[5][((5 * 2) - 1)];
extern double ixmc1[5][5];
extern double ixtmc1[5][5];
extern double ixmc2[5][5];
extern double ixtmc2[5][5];
extern double map2[5];
extern double map4[5];
extern double xfrac[5];
extern int f_e_ef[6][4];
extern int e_c[8][3];
extern int local_corner[6][8];
extern int cal_nnb[8][3];
extern int oplc[4];
extern int cal_iijj[4][2];
extern int cal_intempx[6][4];
extern int c_f[6][4];
extern int le_arr[3][2][4];
extern int jjface[6];
extern int e_face2[6][4];
extern int op[4];
extern int localedgenumber[12][6];
extern int edgenumber[6][4];
extern int f_c[8][3];
extern int e1v1[6][6];
extern int e2v1[6][6];
extern int e1v2[6][6];
extern int e2v2[6][6];
extern int children[6][4];
extern int iijj[4][2];
extern int v_end[2];
extern int face_l1[3];
extern int face_l2[3];
extern int face_ld[3];
static int r_init(double a[], int n, double _const);
int main() {
  double tmp[2][5][5];
  double tmor[8800], tx[8800];
  int ig1, ig2, ig3, ig4, ie, iface, il1, il2, il3, il4, ntemp;
  int nnje, ije1, ije2, col, i, j, ig, il;
  int k;
  int x;
  int y;
  int z;
  int _ret_val_0;
  int i_0;
  int i_1;
  int col_0;
  int i_2;
  int i_3;
  int col_1;
  int i_4;
  int i_5;
  int i_6;
  int i_7;
  int i_8;
  int i_9;
  int i_10;
  int i_11;
  int i_12;
  v_end[0] = 0;
  v_end[1] = (5 - 1);
#pragma cetus private(i, j, k, ntemp)
#pragma loop name main #0
#pragma cetus parallel
  for (k = 0; k < 8800; k++) {
    ntemp = (((k * 5) * 5) * 5);
#pragma cetus private(i, j)
#pragma loop name main #0 #0
    for (j = 0; j < 5; j++) {
#pragma cetus private(i)
#pragma loop name main #0 #0 #0
      for (i = 0; i < 5; i++) {
        idel[k][0][j][i] = ((((ntemp + (i * 5)) + ((j * 5) * 5)) + 5) - 1);
        idel[k][1][j][i] = ((ntemp + (i * 5)) + ((j * 5) * 5));
        idel[k][2][j][i] =
            (((ntemp + (i * 1)) + ((j * 5) * 5)) + (5 * (5 - 1)));
        idel[k][3][j][i] = ((ntemp + (i * 1)) + ((j * 5) * 5));
        idel[k][4][j][i] =
            (((ntemp + (i * 1)) + (j * 5)) + ((5 * 5) * (5 - 1)));
        idel[k][5][j][i] = ((ntemp + (i * 1)) + (j * 5));
      }
    }
  }
#pragma cetus private(col, col_0, col_1, i, i_0, i_1, i_10, i_11, i_12, i_2, \
                      i_3, i_4, i_5, i_6, i_7, i_8, i_9, ie, iface, ig, ig1, \
                      ig2, ig3, ig4, ije1, ije2, il, il1, il2, il3, il4, j,  \
                      nnje, tmp, x, y, z)
#pragma loop name main #1
#pragma cetus parallel
  for (ie = 0; ie < nelt; ie++) {
#pragma cetus private(col, col_0, col_1, i, i_0, i_1, i_10, i_11, i_12, i_2,  \
                      i_3, i_4, i_5, i_6, i_7, i_8, i_9, iface, ig, ig1, ig2, \
                      ig3, ig4, ije1, ije2, il, il1, il2, il3, il4, j, nnje,  \
                      x, y, z)
#pragma cetus lastprivate(tmp)
#pragma loop name main #1 #0
    for (iface = 0; iface < 6; iface++) {
      il1 = idel[ie][iface][0][0];
      il2 = idel[ie][iface][0][5 - 1];
      il3 = idel[ie][iface][5 - 1][0];
      il4 = idel[ie][iface][5 - 1][5 - 1];
      ig1 = idmo[ie][iface][0][0][0][0];
      ig2 = idmo[ie][iface][1][0][0][5 - 1];
      ig3 = idmo[ie][iface][0][1][5 - 1][0];
      ig4 = idmo[ie][iface][1][1][5 - 1][5 - 1];
      tx[il1] = tmor[ig1];
      tx[il2] = tmor[ig2];
      tx[il3] = tmor[ig3];
      tx[il4] = tmor[ig4];
      if (cbc[ie][iface] == 3) {
        nnje = 2;
      } else {
        nnje = 1;
      }
      if (nnje == 2) {
#pragma cetus private(x, y, z)
#pragma loop name main #1 #0 #0
        for (x = 0; x < nnje; x++) {
#pragma cetus private(y, z)
#pragma loop name main #1 #0 #0 #0
          for (y = 0; y < 5; y++) {
#pragma cetus private(z)
#pragma loop name main #1 #0 #0 #0 #0
            for (z = 0; z < 5; z++) {
              tmp[x][y][z] = 0.0;
            }
          }
        }
#pragma cetus firstprivate(tmp)
#pragma cetus private(col, i, i_0, ig, ije1, ije2, il, j)
#pragma cetus lastprivate(tmp)
#pragma loop name main #1 #0 #1
        for (ije1 = 0; ije1 < nnje; ije1++) {
#pragma cetus firstprivate(tmp)
#pragma cetus private(col, i, i_0, ig, ije2, il, j)
#pragma cetus lastprivate(tmp)
#pragma loop name main #1 #0 #1 #0
          for (ije2 = 0; ije2 < nnje; ije2++) {
#pragma cetus private(col, i, i_0, ig, il, j)
#pragma loop name main #1 #0 #1 #0 #0
            for (col = 0; col < 5; col++) {
              i = v_end[ije2];
              ig = idmo[ie][iface][ije2][ije1][col][i];
              tmp[ije1][col][i] = tmor[ig];
/* Normalized Loop */
#pragma cetus private(ig, il, j)
#pragma cetus lastprivate(i_0)
#pragma loop name main #1 #0 #1 #0 #0 #0
              /* #pragma cetus reduction(+: tmp[ije1][col][(1+i_0)])  */
              for (i_0 = 0; i_0 <= 2; i_0++) {
                il = idel[ie][iface][col][1 + i_0];
#pragma cetus private(ig, j)
#pragma loop name main #1 #0 #1 #0 #0 #0 #0
                /* #pragma cetus reduction(+: tmp[ije1][col][(1+i_0)])  */
                for (j = 0; j < 5; j++) {
                  ig = idmo[ie][iface][ije2][ije1][col][j];
                  tmp[ije1][col][1 + i_0] =
                      (tmp[ije1][col][1 + i_0] +
                       (qbnew[ije2][j][(1 + i_0) - 1] * tmor[ig]));
                }
              }
              i = (1 + i_0);
            }
          }
        }
#pragma cetus private(col, col_0, i, i_1, i_2, i_3, ije1, il, j)
#pragma loop name main #1 #0 #2
        /* #pragma cetus reduction(+: tx[il])  */
        for (ije1 = 0; ije1 < nnje; ije1++) {
          col = 0;
/* Normalized Loop */
#pragma cetus private(il, j)
#pragma cetus lastprivate(i_1)
#pragma loop name main #1 #0 #2 #0
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_1 = 0; i_1 <= 2; i_1++) {
            il = idel[ie][iface][1 + i_1][col];
#pragma cetus private(j)
#pragma loop name main #1 #0 #2 #0 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (j = 0; j < 5; j++) {
              tx[il] =
                  (tx[il] +
                   ((qbnew[ije1][j][(1 + i_1) - 1] * tmp[ije1][j][col]) * 0.5));
            }
          }
          i = (1 + i_1);
/* Normalized Loop */
#pragma cetus private(i, i_2, il, j)
#pragma cetus lastprivate(col_0)
#pragma loop name main #1 #0 #2 #1
          /* #pragma cetus reduction(+: tx[il])  */
          for (col_0 = 0; col_0 <= 2; col_0++) {
            i = v_end[ije1];
            il = idel[ie][iface][i][1 + col_0];
            tx[il] = (tx[il] + (tmp[ije1][i][1 + col_0] * 0.5));
/* Normalized Loop */
#pragma cetus private(il, j)
#pragma cetus lastprivate(i_2)
#pragma loop name main #1 #0 #2 #1 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (i_2 = 0; i_2 <= 2; i_2++) {
              il = idel[ie][iface][1 + i_2][1 + col_0];
#pragma cetus private(j)
#pragma loop name main #1 #0 #2 #1 #0 #0
              /* #pragma cetus reduction(+: tx[il])  */
              for (j = 0; j < 5; j++) {
                tx[il] = (tx[il] + (qbnew[ije1][j][(1 + i_2) - 1] *
                                    tmp[ije1][j][1 + col_0]));
              }
            }
            i = (1 + i_2);
          }
          col = (1 + col_0);
          col = (5 - 1);
/* Normalized Loop */
#pragma cetus private(il, j)
#pragma cetus lastprivate(i_3)
#pragma loop name main #1 #0 #2 #2
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_3 = 0; i_3 <= 2; i_3++) {
            il = idel[ie][iface][1 + i_3][col];
#pragma cetus private(j)
#pragma loop name main #1 #0 #2 #2 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (j = 0; j < 5; j++) {
              tx[il] =
                  (tx[il] +
                   ((qbnew[ije1][j][(1 + i_3) - 1] * tmp[ije1][j][col]) * 0.5));
            }
          }
          i = (1 + i_3);
        }
      } else {
/* Normalized Loop */
#pragma cetus private(i, i_4, ig, il)
#pragma cetus lastprivate(col_1)
#pragma loop name main #1 #0 #3
        for (col_1 = 0; col_1 <= 2; col_1++) {
/* Normalized Loop */
#pragma cetus private(ig, il)
#pragma cetus lastprivate(i_4)
#pragma loop name main #1 #0 #3 #0
          for (i_4 = 0; i_4 <= 2; i_4++) {
            il = idel[ie][iface][1 + col_1][1 + i_4];
            ig = idmo[ie][iface][0][0][1 + col_1][1 + i_4];
            tx[il] = tmor[ig];
          }
          i = (1 + i_4);
        }
        col = (1 + col_1);
        if (idmo[ie][iface][0][0][0][5 - 1] != (-1)) {
/* Normalized Loop */
#pragma cetus private(ig, ije1, il, j)
#pragma cetus lastprivate(i_5)
#pragma loop name main #1 #0 #4
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_5 = 0; i_5 <= 2; i_5++) {
            il = idel[ie][iface][0][1 + i_5];
#pragma cetus private(ig, ije1, j)
#pragma loop name main #1 #0 #4 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (ije1 = 0; ije1 < 2; ije1++) {
#pragma cetus private(ig, j)
#pragma loop name main #1 #0 #4 #0 #0
              /* #pragma cetus reduction(+: tx[il])  */
              for (j = 0; j < 5; j++) {
                ig = idmo[ie][iface][ije1][0][0][j];
                tx[il] = (tx[il] +
                          ((qbnew[ije1][j][(1 + i_5) - 1] * tmor[ig]) * 0.5));
              }
            }
          }
          i = (1 + i_5);
        } else {
/* Normalized Loop */
#pragma cetus private(ig, il)
#pragma cetus lastprivate(i_6)
#pragma loop name main #1 #0 #5
          for (i_6 = 0; i_6 <= 2; i_6++) {
            il = idel[ie][iface][0][1 + i_6];
            ig = idmo[ie][iface][0][0][0][1 + i_6];
            tx[il] = tmor[ig];
          }
          i = (1 + i_6);
        }
        if (idmo[ie][iface][1][0][1][5 - 1] != (-1)) {
/* Normalized Loop */
#pragma cetus private(ig, ije1, il, j)
#pragma cetus lastprivate(i_7)
#pragma loop name main #1 #0 #6
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_7 = 0; i_7 <= 2; i_7++) {
            il = idel[ie][iface][1 + i_7][5 - 1];
#pragma cetus private(ig, ije1, j)
#pragma loop name main #1 #0 #6 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (ije1 = 0; ije1 < 2; ije1++) {
#pragma cetus private(ig, j)
#pragma loop name main #1 #0 #6 #0 #0
              /* #pragma cetus reduction(+: tx[il])  */
              for (j = 0; j < 5; j++) {
                ig = idmo[ie][iface][1][ije1][j][5 - 1];
                tx[il] = (tx[il] +
                          ((qbnew[ije1][j][(1 + i_7) - 1] * tmor[ig]) * 0.5));
              }
            }
          }
          i = (1 + i_7);
        } else {
/* Normalized Loop */
#pragma cetus private(ig, il)
#pragma cetus lastprivate(i_8)
#pragma loop name main #1 #0 #7
          for (i_8 = 0; i_8 <= 2; i_8++) {
            il = idel[ie][iface][1 + i_8][5 - 1];
            ig = idmo[ie][iface][0][0][1 + i_8][5 - 1];
            tx[il] = tmor[ig];
          }
          i = (1 + i_8);
        }
        if (idmo[ie][iface][0][1][5 - 1][1] != (-1)) {
/* Normalized Loop */
#pragma cetus private(ig, ije1, il, j)
#pragma cetus lastprivate(i_9)
#pragma loop name main #1 #0 #8
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_9 = 0; i_9 <= 2; i_9++) {
            il = idel[ie][iface][5 - 1][1 + i_9];
#pragma cetus private(ig, ije1, j)
#pragma loop name main #1 #0 #8 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (ije1 = 0; ije1 < 2; ije1++) {
#pragma cetus private(ig, j)
#pragma loop name main #1 #0 #8 #0 #0
              /* #pragma cetus reduction(+: tx[il])  */
              for (j = 0; j < 5; j++) {
                ig = idmo[ie][iface][ije1][1][5 - 1][j];
                tx[il] = (tx[il] +
                          ((qbnew[ije1][j][(1 + i_9) - 1] * tmor[ig]) * 0.5));
              }
            }
          }
          i = (1 + i_9);
        } else {
/* Normalized Loop */
#pragma cetus private(ig, il)
#pragma cetus lastprivate(i_10)
#pragma loop name main #1 #0 #9
          for (i_10 = 0; i_10 <= 2; i_10++) {
            il = idel[ie][iface][5 - 1][1 + i_10];
            ig = idmo[ie][iface][0][0][5 - 1][1 + i_10];
            tx[il] = tmor[ig];
          }
          i = (1 + i_10);
        }
        if (idmo[ie][iface][0][0][5 - 1][0] != (-1)) {
/* Normalized Loop */
#pragma cetus private(ig, ije1, il, j)
#pragma cetus lastprivate(i_11)
#pragma loop name main #1 #0 #10
          /* #pragma cetus reduction(+: tx[il])  */
          for (i_11 = 0; i_11 <= 2; i_11++) {
            il = idel[ie][iface][1 + i_11][0];
#pragma cetus private(ig, ije1, j)
#pragma loop name main #1 #0 #10 #0
            /* #pragma cetus reduction(+: tx[il])  */
            for (ije1 = 0; ije1 < 2; ije1++) {
#pragma cetus private(ig, j)
#pragma loop name main #1 #0 #10 #0 #0
              /* #pragma cetus reduction(+: tx[il])  */
              for (j = 0; j < 5; j++) {
                ig = idmo[ie][iface][0][ije1][j][0];
                tx[il] = (tx[il] +
                          ((qbnew[ije1][j][(1 + i_11) - 1] * tmor[ig]) * 0.5));
              }
            }
          }
          i = (1 + i_11);
        } else {
/* Normalized Loop */
#pragma cetus private(ig, il)
#pragma cetus lastprivate(i_12)
#pragma loop name main #1 #0 #11
          for (i_12 = 0; i_12 <= 2; i_12++) {
            il = idel[ie][iface][1 + i_12][0];
            ig = idmo[ie][iface][0][0][1 + i_12][0];
            tx[il] = tmor[ig];
          }
          i = (1 + i_12);
        }
      }
    }
  }
  _ret_val_0 = 0;
  return _ret_val_0;
}

static void calc() {
  int i, j, k, ntemp, temp, dtemp, temp1, temp2;
  int g1m1_s[6][5][5][5];
  int g4m1_s[6][5][5][5];
  int isize;
#pragma cetus private(i)
#pragma loop name calc #0
#pragma cetus parallel
  for (i = 0; i < 5; i++) {
    xfrac[i] = ((zgm1[i] * 0.5) + 0.5);
  }
#pragma cetus private(dtemp, i, isize, j, k, temp, temp1, temp2)
#pragma loop name calc #1
#pragma cetus parallel
  for (isize = 0; isize < 6; isize++) {
    temp = pow(2.0, (-isize) - 2);
    dtemp = (1.0 / temp);
    temp1 = ((temp * temp) * temp);
    temp2 = (temp * temp);
#pragma cetus private(i, j, k)
#pragma loop name calc #1 #0
    for (k = 0; k < 5; k++) {
#pragma cetus private(i, j)
#pragma loop name calc #1 #0 #0
      for (j = 0; j < 5; j++) {
#pragma cetus private(i)
#pragma loop name calc #1 #0 #0 #0
        for (i = 0; i < 5; i++) {
          g1m1_s[isize][k][j][i] = (g1m1_s[isize][k][j][i] / wxm1[i]);
          g4m1_s[isize][k][j][i] = (g1m1_s[isize][k][j][i] / wxm1[i]);
        }
      }
    }
  }
  return;
}

static int r_init(double a[], int n, double _const) {
  int i;
  int _ret_val_0;
#pragma cetus private(i)
#pragma loop name r_init #0
#pragma cetus parallel
  for (i = 0; i < n; i++) {
    a[i] = _const;
  }
  _ret_val_0 = 0;
  return _ret_val_0;
}
