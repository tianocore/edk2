/** @file
  This is a variation on dtoa.c that converts arbitary binary
  floating-point formats to and from decimal notation.  It uses
  double-precision arithmetic internally, so there are still
  various #ifdefs that adapt the calculations to the native
  IEEE double-precision arithmetic.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  *****************************************************************

  The author of this software is David M. Gay.

  Copyright (C) 1998-2000 by Lucent Technologies
  All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
  its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of Lucent or any of its entities
  not be used in advertising or publicity pertaining to
  distribution of the software without specific, written prior
  permission.

  LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
  IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
  IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
  THIS SOFTWARE.

  Please send bug reports to David M. Gay (dmg at acm dot org,
  with " at " changed at "@" and " dot " changed to ".").

  *****************************************************************

  NetBSD: gdtoaimp.h,v 1.5.4.1 2007/05/07 19:49:06 pavel Exp
**/

/* On a machine with IEEE extended-precision registers, it is
 * necessary to specify double-precision (53-bit) rounding precision
 * before invoking strtod or dtoa.  If the machine uses (the equivalent
 * of) Intel 80x87 arithmetic, the call
 *  _control87(PC_53, MCW_PC);
 * does this with many compilers.  Whether this or another call is
 * appropriate depends on the compiler; for this to work, it may be
 * necessary to #include "float.h" or another system-dependent header
 * file.
 */

/* strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This strtod returns a nearest machine number to the input decimal
 * string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
 * broken by the IEEE round-even rule.  Otherwise ties are broken by
 * biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 112-126].
 *
 * Modifications:
 *
 *  1. We only require IEEE, IBM, or VAX double-precision
 *    arithmetic (not IEEE double-extended).
 *  2. We get by with floating-point arithmetic in a case that
 *    Clinger missed -- when we're computing d * 10^n
 *    for a small integer d and the integer n is not too
 *    much larger than 22 (the maximum integer k for which
 *    we can represent 10^k exactly), we may be able to
 *    compute (d*10^k) * 10^(e-k) with just one roundoff.
 *  3. Rather than a bit-at-a-time adjustment of the binary
 *    result in the hard case, we use floating-point
 *    arithmetic to determine the adjustment to within
 *    one bit; only in really hard cases do we need to
 *    compute a second residual.
 *  4. Because of 3., we don't need a large table of powers of 10
 *    for ten-to-e (just some small tables, e.g. of 10^k
 *    for 0 <= k <= 22).
 */

/*
 * #define IEEE_LITTLE_ENDIAN for IEEE-arithmetic machines where the least
 *  significant byte has the lowest address.
 * #define IEEE_BIG_ENDIAN for IEEE-arithmetic machines where the most
 *  significant byte has the lowest address.
 * #define Long int on machines with 32-bit ints and 64-bit longs.
 * #define Sudden_Underflow for IEEE-format machines without gradual
 *  underflow (i.e., that flush to zero on underflow).
 * #define No_leftright to omit left-right logic in fast floating-point
 *  computation of dtoa.
 * #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3.
 * #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines
 *  that use extended-precision instructions to compute rounded
 *  products and quotients) with IBM.
 * #define ROUND_BIASED for IEEE-format with biased rounding.
 * #define Inaccurate_Divide for IEEE-format with correctly rounded
 *  products but inaccurate quotients, e.g., for Intel i860.
 * #define NO_LONG_LONG on machines that do not have a "long long"
 *  integer type (of >= 64 bits).  On such machines, you can
 *  #define Just_16 to store 16 bits per 32-bit Long when doing
 *  high-precision integer arithmetic.  Whether this speeds things
 *  up or slows things down depends on the machine and the number
 *  being converted.  If long long is available and the name is
 *  something other than "long long", #define Llong to be the name,
 *  and if "unsigned Llong" does not work as an unsigned version of
 *  Llong, #define #ULLong to be the corresponding unsigned type.
 * #define Bad_float_h if your system lacks a float.h or if it does not
 *  define some or all of DBL_DIG, DBL_MAX_10_EXP, DBL_MAX_EXP,
 *  FLT_RADIX, FLT_ROUNDS, and DBL_MAX.
 * #define MALLOC your_malloc, where your_malloc(n) acts like malloc(n)
 *  if memory is available and otherwise does something you deem
 *  appropriate.  If MALLOC is undefined, malloc will be invoked
 *  directly -- and assumed always to succeed.
 * #define Omit_Private_Memory to omit logic (added Jan. 1998) for making
 *  memory allocations from a private pool of memory when possible.
 *  When used, the private pool is PRIVATE_MEM bytes long:  2304 bytes,
 *  unless #defined to be a different length.  This default length
 *  suffices to get rid of MALLOC calls except for unusual cases,
 *  such as decimal-to-binary conversion of a very long string of
 *  digits.  When converting IEEE double precision values, the
 *  longest string gdtoa can return is about 751 bytes long.  For
 *  conversions by strtod of strings of 800 digits and all gdtoa
 *  conversions of IEEE doubles in single-threaded executions with
 *  8-byte pointers, PRIVATE_MEM >= 7400 appears to suffice; with
 *  4-byte pointers, PRIVATE_MEM >= 7112 appears adequate.
 * #define INFNAN_CHECK on IEEE systems to cause strtod to check for
 *  Infinity and NaN (case insensitively).
 *  When INFNAN_CHECK is #defined and No_Hex_NaN is not #defined,
 *  strtodg also accepts (case insensitively) strings of the form
 *  NaN(x), where x is a string of hexadecimal digits and spaces;
 *  if there is only one string of hexadecimal digits, it is taken
 *  for the fraction bits of the resulting NaN; if there are two or
 *  more strings of hexadecimal digits, each string is assigned
 *  to the next available sequence of 32-bit words of fractions
 *  bits (starting with the most significant), right-aligned in
 *  each sequence.
 * #define MULTIPLE_THREADS if the system offers preemptively scheduled
 *  multiple threads.  In this case, you must provide (or suitably
 *  #define) two locks, acquired by ACQUIRE_DTOA_LOCK(n) and freed
 *  by FREE_DTOA_LOCK(n) for n = 0 or 1.  (The second lock, accessed
 *  in pow5mult, ensures lazy evaluation of only one copy of high
 *  powers of 5; omitting this lock would introduce a small
 *  probability of wasting memory, but would otherwise be harmless.)
 *  You must also invoke freedtoa(s) to free the value s returned by
 *  dtoa.  You may do so whether or not MULTIPLE_THREADS is #defined.
 * #define IMPRECISE_INEXACT if you do not care about the setting of
 *  the STRTOG_Inexact bits in the special case of doing IEEE double
 *  precision conversions (which could also be done by the strtog in
 *  dtoa.c).
 * #define NO_HEX_FP to disable recognition of C9x's hexadecimal
 *  floating-point constants.
 * #define -DNO_ERRNO to suppress setting errno (in strtod.c and
 *  strtodg.c).
 * #define NO_STRING_H to use private versions of memcpy.
 *  On some K&R systems, it may also be necessary to
 *  #define DECLARE_SIZE_T in this case.
 * #define YES_ALIAS to permit aliasing certain double values with
 *  arrays of ULongs.  This leads to slightly better code with
 *  some compilers and was always used prior to 19990916, but it
 *  is not strictly legal and can cause trouble with aggressively
 *  optimizing compilers (e.g., gcc 2.95.1 under -O2).
 * #define USE_LOCALE to use the current locale's decimal_point value.
 */

/* #define IEEE_{BIG,LITTLE}_ENDIAN in ${ARCHDIR}/gdtoa/arith.h */
#include  <LibConfig.h>

#include <stdint.h>
#define Short   int16_t
#define UShort uint16_t
#define Long    int32_t
#define ULong  uint32_t
#define LLong   int64_t
#define ULLong uint64_t

#define INFNAN_CHECK
#ifdef _REENTRANT
#define MULTIPLE_THREADS
#endif
#define USE_LOCALE

#ifndef GDTOAIMP_H_INCLUDED
#define GDTOAIMP_H_INCLUDED
#include "gdtoa.h"
#include "gd_qnan.h"

#ifdef DEBUG
#include "stdio.h"
#define Bug(x) {fprintf(stderr, "%s\n", x); exit(1);}
#endif

#include "stdlib.h"
#include "string.h"

#define Char void

#ifdef MALLOC
extern Char *MALLOC ANSI((size_t));
#else
#define MALLOC malloc
#endif

#undef IEEE_Arith
#undef Avoid_Underflow
#ifdef IEEE_BIG_ENDIAN
#define IEEE_Arith
#endif
#ifdef IEEE_LITTLE_ENDIAN
#define IEEE_Arith
#endif

#include "errno.h"
#ifdef Bad_float_h

#ifdef IEEE_Arith
#define DBL_DIG 15
#define DBL_MAX_10_EXP 308
#define DBL_MAX_EXP 1024
#define FLT_RADIX 2
#define DBL_MAX 1.7976931348623157e+308
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647
#endif

#else /* ifndef Bad_float_h */
#include "float.h"
#endif /* Bad_float_h */

#ifdef IEEE_Arith
#define Scale_Bit 0x10
#define n_bigtens 5
#endif

#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(IEEE_LITTLE_ENDIAN) + defined(IEEE_BIG_ENDIAN) != 1
Exactly one of IEEE_LITTLE_ENDIAN or IEEE_BIG_ENDIAN should be defined.
#endif

/*  This union assumes that:
      sizeof(double) == 8
      sizeof(UINT32) == 4

    If this is not the case, the type and dimension of the L member will
    have to be modified.
*/
typedef union { double d; UINT32 L[2]; } U;

#ifdef YES_ALIAS
#define dval(x) x
#ifdef IEEE_LITTLE_ENDIAN
#define word0(x) ((ULong *)&x)[1]
#define word1(x) ((ULong *)&x)[0]
#else
#define word0(x) ((ULong *)&x)[0]
#define word1(x) ((ULong *)&x)[1]
#endif
#else /* !YES_ALIAS */
#ifdef IEEE_LITTLE_ENDIAN
#define word0(x)  ( /* LINTED */ (U*)&x)->L[1]
#define word1(x)  ( /* LINTED */ (U*)&x)->L[0]
#else
#define word0(x)  ( /* LINTED */ (U*)&x)->L[0]
#define word1(x)  ( /* LINTED */ (U*)&x)->L[1]
#endif
#define dval(x)   ( /* LINTED */ (U*)&x)->d
#endif /* YES_ALIAS */

/* The following definition of Storeinc is appropriate for MIPS processors.
 * An alternative that might be better on some machines is
 * #define Storeinc(a,b,c) (*a++ = b << 16 | c & 0xffff)
 */
#if defined(IEEE_LITTLE_ENDIAN)
#define Storeinc(a,b,c) \
 (((unsigned short *)(void *)a)[1] = (unsigned short)b, \
  ((unsigned short *)(void *)a)[0] = (unsigned short)c, \
  a++)
#else
#define Storeinc(a,b,c) \
 (((unsigned short *)(void *)a)[0] = (unsigned short)b, \
  ((unsigned short *)(void *)a)[1] = (unsigned short)c, \
  a++)
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

#ifdef IEEE_Arith
#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    0x100000
#define Exp_msk11   0x100000
#define Exp_mask  0x7ff00000
#define P 53
#define Bias 1023
#define Emin (-1022)
#define Exp_1  0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask  0xfffffU
#define Frac_mask1 0xfffffU
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask  0xfffffU
#define Bndry_mask1 0xfffffU
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14

#ifndef Flt_Rounds
#ifdef FLT_ROUNDS
#define Flt_Rounds FLT_ROUNDS
#else
#define Flt_Rounds 1
#endif
#endif /*Flt_Rounds*/

#else /* ifndef IEEE_Arith */
#undef  Sudden_Underflow
#define Sudden_Underflow
#ifdef IBM
#undef Flt_Rounds
#define Flt_Rounds 0
#define Exp_shift  24
#define Exp_shift1 24
#define Exp_msk1   0x1000000
#define Exp_msk11  0x1000000
#define Exp_mask  0x7f000000
#define P 14
#define Bias 65
#define Exp_1  0x41000000
#define Exp_11 0x41000000
#define Ebits 8 /* exponent has 7 bits, but 8 is the right value in b2d */
#define Frac_mask  0xffffff
#define Frac_mask1 0xffffff
#define Bletch 4
#define Ten_pmax 22
#define Bndry_mask  0xefffff
#define Bndry_mask1 0xffffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 4
#define Tiny0 0x100000
#define Tiny1 0
#define Quick_max 14
#define Int_max 15
#else /* VAX */
#undef Flt_Rounds
#define Flt_Rounds 1
#define Exp_shift  23
#define Exp_shift1 7
#define Exp_msk1    0x80
#define Exp_msk11   0x800000
#define Exp_mask  0x7f80
#define P 56
#define Bias 129
#define Exp_1  0x40800000
#define Exp_11 0x4080
#define Ebits 8
#define Frac_mask  0x7fffff
#define Frac_mask1 0xffff007f
#define Ten_pmax 24
#define Bletch 2
#define Bndry_mask  0xffff007f
#define Bndry_mask1 0xffff007f
#define LSB 0x10000
#define Sign_bit 0x8000
#define Log2P 1
#define Tiny0 0x80
#define Tiny1 0
#define Quick_max 15
#define Int_max 15
#endif /* IBM, VAX */
#endif /* IEEE_Arith */

#ifndef IEEE_Arith
#define ROUND_BIASED
#endif

#ifdef RND_PRODQUOT
#define rounded_product(a,b) a = rnd_prod(a, b)
#define rounded_quotient(a,b) a = rnd_quot(a, b)
extern double rnd_prod(double, double), rnd_quot(double, double);
#else
#define rounded_product(a,b) a *= b
#define rounded_quotient(a,b) a /= b
#endif

#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#define Big1 0xffffffffU

#undef  Pack_16
#ifndef Pack_32
#define Pack_32
#endif

#ifdef NO_LONG_LONG
#undef ULLong
#ifdef Just_16
#undef Pack_32
#define Pack_16
/* When Pack_32 is not defined, we store 16 bits per 32-bit Long.
 * This makes some inner loops simpler and sometimes saves work
 * during multiplications, but it often seems to make things slightly
 * slower.  Hence the default is now to store 32 bits per Long.
 */
#endif
#else /* long long available */
#ifndef Llong
#define Llong long long
#endif
#ifndef ULLong
#define ULLong unsigned Llong
#endif
#endif /* NO_LONG_LONG */

#ifdef Pack_32
#define ULbits 32
#define kshift 5
#define kmask 31
#define ALL_ON 0xffffffff
#else
#define ULbits 16
#define kshift 4
#define kmask 15
#define ALL_ON 0xffff
#endif

#ifndef MULTIPLE_THREADS
#define ACQUIRE_DTOA_LOCK(n)  /*nothing*/
#define FREE_DTOA_LOCK(n) /*nothing*/
#else
#include "reentrant.h"

extern mutex_t __gdtoa_locks[2];

#define ACQUIRE_DTOA_LOCK(n)  \
  do {              \
    if (__isthreaded)       \
      mutex_lock(&__gdtoa_locks[n]);    \
  } while (/* CONSTCOND */ 0)
#define FREE_DTOA_LOCK(n) \
  do {              \
    if (__isthreaded)       \
      mutex_unlock(&__gdtoa_locks[n]);  \
  } while (/* CONSTCOND */ 0)
#endif

#define Kmax (sizeof(size_t) << 3)

 struct
Bigint {
  struct Bigint *next;
  int k, maxwds, sign, wds;
  ULong x[1];
  };

 typedef struct Bigint Bigint;

#ifdef NO_STRING_H
#ifdef DECLARE_SIZE_T
typedef unsigned int size_t;
#endif
extern void memcpy_D2A ANSI((void*, const void*, size_t));
#define Bcopy(x,y) memcpy_D2A(&x->sign,&y->sign,y->wds*sizeof(ULong) + 2*sizeof(int))
#else /* !NO_STRING_H */
#define Bcopy(x,y) memcpy(&x->sign,&y->sign,y->wds*sizeof(ULong) + 2*sizeof(int))
#endif /* NO_STRING_H */

#define Balloc        __Balloc_D2A
#define Bfree         __Bfree_D2A
#define ULtoQ         __ULtoQ_D2A
#define ULtof         __ULtof_D2A
#define ULtod         __ULtod_D2A
#define ULtodd        __ULtodd_D2A
#define ULtox         __ULtox_D2A
#define ULtoxL        __ULtoxL_D2A
#define any_on        __any_on_D2A
#define b2d           __b2d_D2A
#define bigtens       __bigtens_D2A
#define cmp           __cmp_D2A
#define copybits      __copybits_D2A
#define d2b           __d2b_D2A
#define decrement     __decrement_D2A
#define diff          __diff_D2A
#define dtoa_result   __dtoa_result_D2A
#define g__fmt        __g__fmt_D2A
#define gethex        __gethex_D2A
#define hexdig        __hexdig_D2A
#define hexdig_init_D2A __hexdig_init_D2A
#define hexnan        __hexnan_D2A
#define hi0bits       __hi0bits_D2A
#define hi0bits_D2A   __hi0bits_D2A
#define i2b           __i2b_D2A
#define increment     __increment_D2A
#define lo0bits       __lo0bits_D2A
#define lshift        __lshift_D2A
#define match         __match_D2A
#define mult          __mult_D2A
#define multadd       __multadd_D2A
#define nrv_alloc     __nrv_alloc_D2A
#define pow5mult      __pow5mult_D2A
#define quorem        __quorem_D2A
#define ratio         __ratio_D2A
#define rshift        __rshift_D2A
#define rv_alloc      __rv_alloc_D2A
#define s2b           __s2b_D2A
#define set_ones      __set_ones_D2A
#define strcp         __strcp_D2A
#define strcp_D2A     __strcp_D2A
#define strtoIg       __strtoIg_D2A
#define sum           __sum_D2A
#define tens          __tens_D2A
#define tinytens      __tinytens_D2A
#define tinytens      __tinytens_D2A
#define trailz        __trailz_D2A
#define ulp           __ulp_D2A

extern char          *dtoa_result;
extern CONST double   bigtens[], tens[], tinytens[];
extern unsigned char  hexdig[];

extern Bigint  *Balloc      (int);
extern void     Bfree       (Bigint*);
extern void     ULtof       (ULong*, ULong*, Long, int);
extern void     ULtod       (ULong*, ULong*, Long, int);
extern void     ULtodd      (ULong*, ULong*, Long, int);
extern void     ULtoQ       (ULong*, ULong*, Long, int);
extern void     ULtox       (UShort*, ULong*, Long, int);
extern void     ULtoxL      (ULong*, ULong*, Long, int);
extern ULong    any_on      (Bigint*, int);
extern double   b2d         (Bigint*, int*);
extern int      cmp         (Bigint*, Bigint*);
extern void     copybits    (ULong*, int, Bigint*);
extern Bigint  *d2b         (double, int*, int*);
extern int      decrement   (Bigint*);
extern Bigint  *diff        (Bigint*, Bigint*);
extern char    *dtoa        (double d, int mode, int ndigits,
                                  int *decpt, int *sign, char **rve);
extern char    *g__fmt      (char*, char*, char*, int, ULong);
extern int      gethex      (CONST char**, CONST FPI*, Long*, Bigint**, int);
extern void     hexdig_init_D2A(Void);
extern int      hexnan      (CONST char**, CONST FPI*, ULong*);
extern int      hi0bits_D2A (ULong);
extern Bigint  *i2b         (int);
extern Bigint  *increment   (Bigint*);
extern int      lo0bits     (ULong*);
extern Bigint  *lshift      (Bigint*, int);
extern int      match       (CONST char**, CONST char*);
extern Bigint  *mult        (Bigint*, Bigint*);
extern Bigint  *multadd     (Bigint*, int, int);
extern char    *nrv_alloc   (CONST char*, char **, size_t);
extern Bigint  *pow5mult    (Bigint*, int);
extern int      quorem      (Bigint*, Bigint*);
extern double   ratio       (Bigint*, Bigint*);
extern void     rshift      (Bigint*, int);
extern char    *rv_alloc    (size_t);
extern Bigint  *s2b         (CONST char*, int, int, ULong);
extern Bigint  *set_ones    (Bigint*, int);
extern char    *strcp       (char*, const char*);
extern int      strtoIg     (CONST char*, char**, FPI*, Long*, Bigint**, int*);
extern double   strtod      (const char *s00, char **se);
extern Bigint  *sum         (Bigint*, Bigint*);
extern int      trailz      (CONST Bigint*);
extern double   ulp         (double);

#ifdef __cplusplus
}
#endif
/*
 * NAN_WORD0 and NAN_WORD1 are only referenced in strtod.c.  Prior to
 * 20050115, they used to be hard-wired here (to 0x7ff80000 and 0,
 * respectively), but now are determined by compiling and running
 * qnan.c to generate gd_qnan.h, which specifies d_QNAN0 and d_QNAN1.
 * Formerly gdtoaimp.h recommended supplying suitable -DNAN_WORD0=...
 * and -DNAN_WORD1=...  values if necessary.  This should still work.
 * (On HP Series 700/800 machines, -DNAN_WORD0=0x7ff40000 works.)
 */
#ifdef IEEE_Arith
#ifdef IEEE_BIG_ENDIAN
#define _0 0
#define _1 1
#ifndef NAN_WORD0
#define NAN_WORD0 d_QNAN0
#endif
#ifndef NAN_WORD1
#define NAN_WORD1 d_QNAN1
#endif
#else
#define _0 1
#define _1 0
#ifndef NAN_WORD0
#define NAN_WORD0 d_QNAN1
#endif
#ifndef NAN_WORD1
#define NAN_WORD1 d_QNAN0
#endif
#endif
#else
#undef INFNAN_CHECK
#endif

#undef SI
#ifdef Sudden_Underflow
#define SI 1
#else
#define SI 0
#endif

#endif /* GDTOAIMP_H_INCLUDED */
