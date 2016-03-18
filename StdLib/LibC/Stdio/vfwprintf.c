/** @file
    Implementation of internals for printf and wprintf.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1990, 1993
    The Regents of the University of California.  All rights reserved.

    This code is derived from software contributed to Berkeley by
    Chris Torek.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      - Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      - Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      - Neither the name of the University nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    NetBSD: vfwprintf.c,v 1.9.2.1.4.1 2008/04/08 21:10:55 jdc Exp
    vfprintf.c  8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>

#include "namespace.h"
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>

#include "reentrant.h"
#include "local.h"
#include "extern.h"
#include "fvwrite.h"

#ifdef  _MSC_VER
  // Keep compiler quiet about conversions from larger to smaller types.
  #pragma warning ( disable : 4244 )
#endif

#ifndef NARROW
#define MCHAR_T   char
#define CHAR_T    wchar_t
#define STRLEN(a) wcslen(a)
#define MEMCHR(a, b, c) wmemchr(a, b, c)
#define SCONV(a, b) __mbsconv(a, b)
#define STRCONST(a) L ## a
#define WDECL(a, b) a ## w ## b
#define END_OF_FILE WEOF
#define MULTI   0
#else
#define MCHAR_T   wchar_t
#define CHAR_T    char
#define STRLEN(a) strlen(a)
#define MEMCHR(a, b, c) memchr(a, b, c)
#define SCONV(a, b) __wcsconv(a, b)
#define STRCONST(a) a
#define WDECL(a, b) a ## b
#define END_OF_FILE EOF
#define MULTI   1
#endif

union arg {
  int                 intarg;
  u_int               uintarg;
  long                longarg;
  unsigned long       ulongarg;
  long long           longlongarg;
  unsigned long long  ulonglongarg;
  ptrdiff_t           ptrdiffarg;
  size_t              sizearg;
  intmax_t            intmaxarg;
  uintmax_t           uintmaxarg;
  void               *pvoidarg;
  char               *pchararg;
  signed char        *pschararg;
  short              *pshortarg;
  int                *pintarg;
  long               *plongarg;
  long long          *plonglongarg;
  ptrdiff_t          *pptrdiffarg;
  size_t             *psizearg;
  intmax_t           *pintmaxarg;
#ifndef NO_FLOATING_POINT
  double              doublearg;
  long double         longdoublearg;
#endif
  wint_t              wintarg;
  wchar_t            *pwchararg;
};

/*
 * Type ids for argument type table.
 */
enum typeid {
  T_UNUSED,   TP_SHORT,     T_INT,          T_U_INT,  TP_INT,
  T_LONG,     T_U_LONG,     TP_LONG,        T_LLONG,  T_U_LLONG,
  TP_LLONG,   T_PTRDIFFT,   TP_PTRDIFFT,    T_SIZET,  TP_SIZET,
  T_INTMAXT,  T_UINTMAXT,   TP_INTMAXT,     TP_VOID,  TP_CHAR,
  TP_SCHAR,   T_DOUBLE,     T_LONG_DOUBLE,  T_WINT,   TP_WCHAR
};

static int      __sbprintf(FILE *, const CHAR_T *, va_list);
static CHAR_T  *__ujtoa(uintmax_t, CHAR_T *, int, int, const char *, int,
                        char, const char *);
static CHAR_T  *__ultoa(u_long, CHAR_T *, int, int, const char *, int,
                        char, const char *);
#ifndef NARROW
static CHAR_T  *__mbsconv(char *, int);
static wint_t   __xfputwc(CHAR_T, FILE *);
#else
static char    *__wcsconv(wchar_t *, int);
static int      __sprint(FILE *, struct __suio *);
#endif
static int      __find_arguments(const CHAR_T *, va_list, union arg **);
static int      __grow_type_table(int, enum typeid **, int *);

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(FILE *fp, const CHAR_T *fmt, va_list ap)
{
  int ret;
  FILE fake;
  struct __sfileext fakeext;
  unsigned char buf[BUFSIZ];

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(fmt != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  _FILEEXT_SETUP(&fake, &fakeext);

  /* copy the important variables */
  fake._flags   = fp->_flags & ~__SNBF;
  fake._file    = fp->_file;
  fake._cookie  = fp->_cookie;
  fake._write   = fp->_write;

  /* set up the buffer */
  fake._bf._base  = fake._p = buf;
  fake._bf._size  = fake._w = sizeof(buf);
  fake._lbfsize   = 0;  /* not actually used, but Just In Case */

  /* do the work, then copy any error status */
  ret = WDECL(__vf,printf_unlocked)(&fake, fmt, ap);
  if (ret >= 0 && fflush(&fake))
    ret = END_OF_FILE;
  if (fake._flags & __SERR)
    fp->_flags |= __SERR;
  return (ret);
}

#ifndef NARROW
/*
 * Like __fputwc, but handles fake string (__SSTR) files properly.
 * File must already be locked.
 */
static wint_t
__xfputwc(wchar_t wc, FILE *fp)
{
  static const mbstate_t initial = { 0 };
  mbstate_t mbs;
  char buf[MB_LEN_MAX];
  struct __suio uio;
  struct __siov iov;
  size_t len;

  if ((fp->_flags & __SSTR) == 0)
    return (__fputwc_unlock(wc, fp));

  mbs = initial;
  if ((len = wcrtomb(buf, wc, &mbs)) == (size_t)-1) {
    fp->_flags |= __SERR;
    return (END_OF_FILE);
  }
  uio.uio_iov     = &iov;
  uio.uio_resid   = (int)len;
  uio.uio_iovcnt  = 1;
  iov.iov_base    = buf;
  iov.iov_len     = len;
  return (__sfvwrite(fp, &uio) != EOF ? (wint_t)wc : END_OF_FILE);
}
#else
/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
static int
__sprint(FILE *fp, struct __suio *uio)
{
  int err;

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(uio != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if (uio->uio_resid == 0) {
    uio->uio_iovcnt = 0;
    return (0);
  }
  err = __sfvwrite(fp, uio);
  uio->uio_resid = 0;
  uio->uio_iovcnt = 0;
  return (err);
}
#endif

/*
 * Macros for converting digits to letters and vice versa
 */
#define to_digit(c) ((c) - '0')
#define is_digit(c) ((unsigned)to_digit(c) <= 9)
#define to_char(n)  (CHAR_T)((n) + '0')

/*
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static CHAR_T *
__ultoa(u_long val, CHAR_T *endp, int base, int octzero, const char *xdigs,
        int needgrp, char thousep, const char *grp)
{
  CHAR_T *cp = endp;
  LONGN sval;
  int ndig;

  /*
   * Handle the three cases separately, in the hope of getting
   * better/faster code.
   */
  switch (base) {
  case 10:
    if (val < 10) { /* many numbers are 1 digit */
      *--cp = to_char(val);
      return (cp);
    }
    ndig = 0;
    /*
     * On many machines, unsigned arithmetic is harder than
     * signed arithmetic, so we do at most one unsigned mod and
     * divide; this is sufficient to reduce the range of
     * the incoming value to where signed arithmetic works.
     */
    if (val > LONG_MAX) {
      *--cp = to_char(val % 10);
      ndig++;
      sval = (LONGN)(val / 10);
    } else
      sval = (LONGN)val;
    do {
      *--cp = to_char(sval % 10);
      ndig++;
      /*
       * If (*grp == CHAR_MAX) then no more grouping
       * should be performed.
       */
      if (needgrp && ndig == *grp && *grp != CHAR_MAX
          && sval > 9) {
        *--cp = thousep;
        ndig = 0;
        /*
         * If (*(grp+1) == '\0') then we have to
         * use *grp character (last grouping rule)
         * for all next cases
         */
        if (*(grp+1) != '\0')
          grp++;
      }
      sval /= 10;
    } while (sval != 0);
    break;

  case 8:
    do {
      *--cp = to_char(val & 7);
      val >>= 3;
    } while (val);
    if (octzero && *cp != '0')
      *--cp = '0';
    break;

  case 16:
    do {
      *--cp = xdigs[(size_t)val & 15];
      val >>= 4;
    } while (val);
    break;

  default:      /* oops */
    abort();
  }
  return (cp);
}

/* Identical to __ultoa, but for intmax_t. */
static CHAR_T *
__ujtoa(uintmax_t val, CHAR_T *endp, int base, int octzero,
        const char *xdigs, int needgrp, char thousep, const char *grp)
{
  CHAR_T *cp = endp;
  intmax_t sval;
  int ndig;

  /* quick test for small values; __ultoa is typically much faster */
  /* (perhaps instead we should run until small, then call __ultoa?) */
  if (val <= ULONG_MAX)
    return (__ultoa((u_long)val, endp, base, octzero, xdigs,
        needgrp, thousep, grp));
  switch (base) {
  case 10:
    if (val < 10) {
      *--cp = to_char(val % 10);
      return (cp);
    }
    ndig = 0;
    if (val > INTMAX_MAX) {
      *--cp = to_char(val % 10);
      ndig++;
      sval = val / 10;
    } else
      sval = val;
    do {
      *--cp = to_char(sval % 10);
      ndig++;
      /*
       * If (*grp == CHAR_MAX) then no more grouping
       * should be performed.
       */
      if (needgrp && *grp != CHAR_MAX && ndig == *grp
          && sval > 9) {
        *--cp = thousep;
        ndig = 0;
        /*
         * If (*(grp+1) == '\0') then we have to
         * use *grp character (last grouping rule)
         * for all next cases
         */
        if (*(grp+1) != '\0')
          grp++;
      }
      sval /= 10;
    } while (sval != 0);
    break;

  case 8:
    do {
      *--cp = to_char(val & 7);
      val >>= 3;
    } while (val);
    if (octzero && *cp != '0')
      *--cp = '0';
    break;

  case 16:
    do {
      *--cp = xdigs[(size_t)val & 15];
      val >>= 4;
    } while (val);
    break;

  default:
    abort();
  }
  return (cp);
}

#ifndef NARROW
/*
 * Convert a multibyte character string argument for the %s format to a wide
 * string representation. ``prec'' specifies the maximum number of bytes
 * to output. If ``prec'' is greater than or equal to zero, we can't assume
 * that the multibyte char. string ends in a null character.
 */
static wchar_t *
__mbsconv(char *mbsarg, int prec)
{
  static const mbstate_t initial = { 0 };
  mbstate_t mbs;
  wchar_t *convbuf, *wcp;
  const char *p;
  size_t insize, nchars, nconv;

  if (mbsarg == NULL)
    return (NULL);

  /*
   * Supplied argument is a multibyte string; convert it to wide
   * characters first.
   */
  if (prec >= 0) {
    /*
     * String is not guaranteed to be NUL-terminated. Find the
     * number of characters to print.
     */
    p = mbsarg;
    insize = nchars = nconv = 0;
    mbs = initial;
    while (nchars != (size_t)prec) {
      nconv = mbrlen(p, MB_CUR_MAX, &mbs);
      if (nconv == 0 || nconv == (size_t)-1 ||
          nconv == (size_t)-2)
        break;
      p += nconv;
      nchars++;
      insize += nconv;
    }
    if (nconv == (size_t)-1 || nconv == (size_t)-2)
      return (NULL);
  } else
    insize = strlen(mbsarg);

  /*
   * Allocate buffer for the result and perform the conversion,
   * converting at most `size' bytes of the input multibyte string to
   * wide characters for printing.
   */
  convbuf = malloc((insize + 1) * sizeof(*convbuf));
  if (convbuf == NULL)
    return (NULL);
  wcp = convbuf;
  p = mbsarg;
  mbs = initial;
  nconv = 0;
  while (insize != 0) {
    nconv = mbrtowc(wcp, p, insize, &mbs);
    if (nconv == 0 || nconv == (size_t)-1 || nconv == (size_t)-2)
      break;
    wcp++;
    p += nconv;
    insize -= nconv;
  }
  if (nconv == (size_t)-1 || nconv == (size_t)-2) {
    free(convbuf);
    return (NULL);
  }
  *wcp = L'\0';

  return (convbuf);
}
#else
/*
 * Convert a wide character string argument for the %ls format to a multibyte
 * string representation. If not -1, prec specifies the maximum number of
 * bytes to output, and also means that we can't assume that the wide char.
 * string ends is null-terminated.
 */
static char *
__wcsconv(wchar_t *wcsarg, int prec)
{
  static const mbstate_t initial = { 0 };
  mbstate_t mbs;
  char buf[MB_LEN_MAX];
  wchar_t *p;
  char *convbuf;
  size_t clen, nbytes;

  /* Allocate space for the maximum number of bytes we could output. */
  if (prec < 0) {
    p = wcsarg;
    mbs = initial;
    nbytes = wcsrtombs(NULL, (const wchar_t **)&p, 0, &mbs);
    if (nbytes == (size_t)-1)
      return (NULL);
  } else {
    /*
     * Optimisation: if the output precision is small enough,
     * just allocate enough memory for the maximum instead of
     * scanning the string.
     */
    if (prec < 128)
      nbytes = prec;
    else {
      nbytes = 0;
      p = wcsarg;
      mbs = initial;
      for (;;) {
        clen = wcrtomb(buf, *p++, &mbs);
        if (clen == 0 || clen == (size_t)-1 ||
            nbytes + clen > (size_t)prec)
          break;
        nbytes += clen;
      }
    }
  }
  if ((convbuf = malloc(nbytes + 1)) == NULL)
    return (NULL);

  /* Fill the output buffer. */
  p = wcsarg;
  mbs = initial;
  if ((nbytes = wcsrtombs(convbuf, (const wchar_t **)&p,
      nbytes, &mbs)) == (size_t)-1) {
    free(convbuf);
    return (NULL);
  }
  convbuf[nbytes] = '\0';
  return (convbuf);
}
#endif

/*
 * MT-safe version
 */
int
WDECL(vf,printf)(FILE * __restrict fp, const CHAR_T * __restrict fmt0, va_list ap)
{
  int ret;

  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }
  FLOCKFILE(fp);
  ret = WDECL(__vf,printf_unlocked)(fp, fmt0, ap);
  FUNLOCKFILE(fp);
  return (ret);
}

#ifndef NO_FLOATING_POINT

#include <float.h>
#include <math.h>
#include "floatio.h"

#define DEFPREC   6

static int exponent(CHAR_T *, int, int);
#ifndef WIDE_DOUBLE
static char *cvt(double, int, int, char *, int *, int, int *);
#endif

#endif /* !NO_FLOATING_POINT */

/*
 * The size of the buffer we use as scratch space for integer
 * conversions, among other things.  Technically, we would need the
 * most space for base 10 conversions with thousands' grouping
 * characters between each pair of digits.  100 bytes is a
 * conservative overestimate even for a 128-bit uintmax_t.
 */
#define BUF 100

#define STATIC_ARG_TBL_SIZE 8           /* Size of static argument table. */

/*
 * Flags used during conversion.
 */
#define ALT       0x001   /* alternate form */
#define LADJUST   0x004   /* left adjustment */
#define LONGDBL   0x008   /* long double */
#define LONGINT   0x010   /* long integer */
#define LLONGINT  0x020   /* long long integer */
#define SHORTINT  0x040   /* short integer */
#define ZEROPAD   0x080   /* zero (as opposed to blank) pad */
#define FPT       0x100   /* Floating point number */
#define GROUPING  0x200   /* use grouping ("'" flag) */
          /* C99 additional size modifiers: */
#define SIZET     0x400   /* size_t */
#define PTRDIFFT  0x800   /* ptrdiff_t */
#define INTMAXT   0x1000    /* intmax_t */
#define CHARINT   0x2000    /* print char using int format */

/*
 * Non-MT-safe version
 */
int
WDECL(__vf,printf_unlocked)(FILE *fp, const CHAR_T *fmt0, va_list ap)
{
  CHAR_T *fmt;    /* format string */
  int ch;     /* character from fmt */
  int n, n2;    /* handy integer (short term usage) */
  CHAR_T *cp;   /* handy char pointer (short term usage) */
  int flags;    /* flags as above */
  int ret;    /* return value accumulator (number of items converted)*/
  int width;    /* width from format (%8d), or 0 */
  int prec;   /* precision from format; <0 for N/A */
  CHAR_T sign;    /* sign prefix (' ', '+', '-', or \0) */
  char thousands_sep; /* locale specific thousands separator */
  const char *grouping; /* locale specific numeric grouping rules */
#ifndef NO_FLOATING_POINT
  /*
   * We can decompose the printed representation of floating
   * point numbers into several parts, some of which may be empty:
   *
   * [+|-| ] [0x|0X] MMM . NNN [e|E|p|P] [+|-] ZZ
   *    A       B     ---C---      D       E   F
   *
   * A: 'sign' holds this value if present; '\0' otherwise
   * B: ox[1] holds the 'x' or 'X'; '\0' if not hexadecimal
   * C: cp points to the string MMMNNN.  Leading and trailing
   *  zeros are not in the string and must be added.
   * D: expchar holds this character; '\0' if no exponent, e.g. %f
   * F: at least two digits for decimal, at least one digit for hex
   */
  char *decimal_point;  /* locale specific decimal point */
#ifdef WIDE_DOUBLE
  int signflag;   /* true if float is negative */
  union {     /* floating point arguments %[aAeEfFgG] */
    double dbl;
    long double ldbl;
  } fparg;
  char *dtoaend;    /* pointer to end of converted digits */
#else
  double _double;   /* double precision arguments %[eEfgG] */
  char softsign;    /* temporary negative sign for floats */
#endif
  char *dtoaresult; /* buffer allocated by dtoa */
  int expt = 0;   /* integer value of exponent */
  char expchar;   /* exponent character: [eEpP\0] */
  int expsize;    /* character count for expstr */
  int lead;   /* sig figs before decimal or group sep */
  int ndig;   /* actual number of digits returned by dtoa */
  CHAR_T expstr[MAXEXPDIG+2]; /* buffer for exponent string: e+ZZZ */
  int nseps;    /* number of group separators with ' */
  int nrepeats;   /* number of repeats of the last group */
#endif
  u_long  ulval;    /* integer arguments %[diouxX] */
  uintmax_t ujval;  /* %j, %ll, %q, %t, %z integers */
  int base;   /* base for [diouxX] conversion */
  int dprec;    /* a copy of prec if [diouxX], 0 otherwise */
  int realsz;   /* field size expanded by dprec, sign, etc */
  int size;   /* size of converted field or string */
  int prsize;             /* max size of printed field */
  const char *xdigs;  /* digits for %[xX] conversion */
#ifdef NARROW
#define NIOV 8
  struct __siov *iovp;  /* for PRINT macro */
  struct __suio uio;  /* output information: summary */
  struct __siov iov[NIOV];/* ... and individual io vectors */
#else
  int n3;
#endif
  CHAR_T buf[BUF];  /* buffer with space for digits of uintmax_t */
  CHAR_T ox[2];   /* space for 0x hex-prefix */
  union arg *argtable;  /* args, built due to positional arg */
  union arg statargtable [STATIC_ARG_TBL_SIZE];
  int nextarg;    /* 1-based argument index */
  va_list orgap;    /* original argument pointer */
  CHAR_T *convbuf;  /* multibyte to wide conversion result */

  /*
   * Choose PADSIZE to trade efficiency vs. size.  If larger printf
   * fields occur frequently, increase PADSIZE and make the initialisers
   * below longer.
   */
#define PADSIZE 16    /* pad chunk size */
  static CHAR_T blanks[PADSIZE] =
   {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
  static CHAR_T zeroes[PADSIZE] =
   {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

  static const char xdigs_lower[17] = "0123456789abcdef";
  static const char xdigs_upper[17] = "0123456789ABCDEF";

  /*
   * BEWARE, these `goto error' on error, PRINT uses `n2' and
   * PAD uses `n'.
   */
#ifndef NARROW
#define PRINT(ptr, len) do {      \
  for (n3 = 0; n3 < (len); n3++)    \
    __xfputwc((ptr)[n3], fp); \
} while (/*CONSTCOND*/0)
#define FLUSH()
#else
#define PRINT(ptr, len) do { \
  iovp->iov_base = __UNCONST(ptr); \
  iovp->iov_len = (len); \
  uio.uio_resid += (len); \
  iovp++; \
  if (++uio.uio_iovcnt >= NIOV) { \
    if (__sprint(fp, &uio)) \
      goto error; \
    iovp = iov; \
  } \
} while (/*CONSTCOND*/0)
#define FLUSH() do { \
  if (uio.uio_resid && __sprint(fp, &uio)) \
    goto error; \
  uio.uio_iovcnt = 0; \
  iovp = iov; \
} while (/*CONSTCOND*/0)
#endif /* NARROW */

#define PAD(howmany, with)  do {    \
  if ((n = (howmany)) > 0) {    \
    while (n > PADSIZE) {   \
      PRINT(with, PADSIZE); \
      n -= PADSIZE;   \
    }       \
    PRINT(with, n);     \
  }         \
} while (/*CONSTCOND*/0)
#define PRINTANDPAD(p, ep, len, with) do {  \
  n2 = (ep) - (p);            \
  if (n2 > (len))       \
    n2 = (len);     \
  if (n2 > 0)       \
    PRINT((p), n2);     \
  PAD((len) - (n2 > 0 ? n2 : 0), (with)); \
} while(/*CONSTCOND*/0)

  /*
   * Get the argument indexed by nextarg.   If the argument table is
   * built, use it to get the argument.  If its not, get the next
   * argument (and arguments must be gotten sequentially).
   */
#define GETARG(type) \
  ((/*CONSTCOND*/argtable != NULL) ? *((type*)(void*)(&argtable[nextarg++])) : \
      (nextarg++, va_arg(ap, type)))

  /*
   * To extend shorts properly, we need both signed and unsigned
   * argument extraction methods.
   */
#define SARG() \
  ((long)(flags&LONGINT ? GETARG(long) : \
      flags&SHORTINT ? (short)GETARG(int) : \
      flags&CHARINT ? (signed char)GETARG(int) : \
      GETARG(int)))

#define UARG() \
  ((u_long)(flags&LONGINT ? GETARG(u_long) : \
      flags&SHORTINT ? (u_long)(u_short)GETARG(int) : \
      flags&CHARINT ? (u_long)(u_char)GETARG(int) : \
      (u_long)GETARG(u_int)))

#define INTMAX_SIZE (INTMAXT|SIZET|PTRDIFFT|LLONGINT)

#define SJARG() \
  (flags&INTMAXT ? GETARG(intmax_t) : \
      flags&SIZET ? (intmax_t)GETARG(size_t) : \
      flags&PTRDIFFT ? (intmax_t)GETARG(ptrdiff_t) : \
      (intmax_t)GETARG(long long))

#define UJARG() \
  (flags&INTMAXT ? GETARG(uintmax_t) : \
      flags&SIZET ? (uintmax_t)GETARG(size_t) : \
      flags&PTRDIFFT ? (uintmax_t)GETARG(ptrdiff_t) : \
      (uintmax_t)GETARG(unsigned long long))

  /*
   * Get * arguments, including the form *nn$.  Preserve the nextarg
   * that the argument can be gotten once the type is determined.
   */
#define GETASTER(val) \
  n2 = 0; \
  cp = fmt; \
  while (is_digit(*cp)) { \
    n2 = 10 * n2 + to_digit(*cp); \
    cp++; \
  } \
  if (*cp == '$') { \
    int hold = nextarg; \
    if (argtable == NULL) { \
      argtable = statargtable; \
      if (__find_arguments(fmt0, orgap, &argtable) == -1) \
        goto oomem; \
    } \
    nextarg = n2; \
    val = GETARG (int); \
    nextarg = hold; \
    fmt = ++cp; \
  } else { \
    val = GETARG (int); \
  }

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(fmt0 != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  _SET_ORIENTATION(fp, -1);

  ndig = -1;  /* XXX gcc */

  thousands_sep = '\0';
  grouping = NULL;
#ifndef NO_FLOATING_POINT
  decimal_point = localeconv()->decimal_point;
  expsize = 0;    /* XXXGCC -Wuninitialized [sh3,m68000] */
#endif
  convbuf = NULL;
  /* sorry, f{w,}printf(read_only_file, L"") returns {W,}EOF, not 0 */
  if (cantwrite(fp)) {
    errno = EBADF;
    return (END_OF_FILE);
  }

  /* optimise fprintf(stderr) (and other unbuffered Unix files) */
  if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
      fp->_file >= 0)
    return (__sbprintf(fp, fmt0, ap));

  fmt = (CHAR_T *)__UNCONST(fmt0);
  argtable = NULL;
  nextarg = 1;
  va_copy(orgap, ap);
#ifdef NARROW
  uio.uio_iov = iovp = iov;
  uio.uio_resid = 0;
  uio.uio_iovcnt = 0;
#endif
  ret = 0;

  /*
   * Scan the format for conversions (`%' character).
   */
  for (;;)
  {
    const CHAR_T *result;

    for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
      continue;
    if ((n = (int)(fmt - cp)) != 0) {
      if ((unsigned)ret + n > INT_MAX) {
        ret = END_OF_FILE;
        goto error;
      }
      PRINT(cp, n);
      ret += n;
    }
    if (ch == '\0')
      goto done;
    fmt++;    /* skip over '%' */

    flags = 0;
    dprec = 0;
    width = 0;
    prec = -1;
    sign = '\0';
    ox[1] = '\0';
    expchar = '\0';
    lead = 0;
    nseps = nrepeats = 0;
    ulval = 0;
    ujval = 0;
    xdigs = NULL;

rflag:    ch = *fmt++;
reswitch: switch (ch) {
    case ' ':
      /*-
       * ``If the space and + flags both appear, the space
       * flag will be ignored.''
       *  -- ANSI X3J11
       */
      if (!sign)
        sign = ' ';
      goto rflag;
    case '#':
      flags |= ALT;
      goto rflag;
    case '*':
      /*-
       * ``A negative field width argument is taken as a
       * - flag followed by a positive field width.''
       *  -- ANSI X3J11
       * They don't exclude field widths read from args.
       */
      GETASTER (width);
      if (width >= 0)
        goto rflag;
      width = -width;
      /* FALLTHROUGH */
    case '-':
      flags |= LADJUST;
      goto rflag;
    case '+':
      sign = '+';
      goto rflag;
    case '\'':
      flags |= GROUPING;
      thousands_sep = *(localeconv()->thousands_sep);
      grouping = localeconv()->grouping;
      goto rflag;
    case '.':
      if ((ch = *fmt++) == '*') {
        GETASTER (prec);
        goto rflag;
      }
      prec = 0;
      while (is_digit(ch)) {
        prec = 10 * prec + to_digit(ch);
        ch = *fmt++;
      }
      goto reswitch;
    case '0':
      /*-
       * ``Note that 0 is taken as a flag, not as the
       * beginning of a field width.''
       *  -- ANSI X3J11
       */
      flags |= ZEROPAD;
      goto rflag;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      n = 0;
      do {
        n = 10 * n + to_digit(ch);
        ch = *fmt++;
      } while (is_digit(ch));
      if (ch == '$') {
        nextarg = n;
        if (argtable == NULL) {
          argtable = statargtable;
          if (__find_arguments(fmt0, orgap,
              &argtable) == -1)
            goto oomem;
        }
        goto rflag;
      }
      width = n;
      goto reswitch;
#ifndef NO_FLOATING_POINT
    case 'L':
      flags |= LONGDBL;
      goto rflag;
#endif
    case 'h':
      if (flags & SHORTINT) {
        flags &= ~SHORTINT;
        flags |= CHARINT;
      } else
        flags |= SHORTINT;
      goto rflag;
    case 'j':
      flags |= INTMAXT;
      goto rflag;
    case 'l':
      if (flags & LONGINT) {
        flags &= ~LONGINT;
        flags |= LLONGINT;
      } else
        flags |= LONGINT;
      goto rflag;
    case 'q':
      flags |= LLONGINT;  /* not necessarily */
      goto rflag;
    case 't':
      flags |= PTRDIFFT;
      goto rflag;
    case 'z':
      flags |= SIZET;
      goto rflag;
    case 'C':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'c':
#ifdef NARROW
      if (flags & LONGINT) {
        static const mbstate_t initial = { 0 };
        mbstate_t mbs;
        size_t mbseqlen;

        mbs = initial;
        mbseqlen = wcrtomb(buf,
            /* The compiler "knows" that wint_t may be smaller than an int so
               it warns about it when used as the type argument to va_arg().
               Since any type of parameter smaller than an int is promoted to an int on a
               function call, we must call GETARG with type int instead of wint_t.
            */
            (wchar_t)GETARG(int), &mbs);
        if (mbseqlen == (size_t)-1) {
          fp->_flags |= __SERR;
          goto error;
        }
        size = (int)mbseqlen;
      } else {
        *buf = (char)(GETARG(int));
        size = 1;
      }
#else
      if (flags & LONGINT)
        *buf = (wchar_t)GETARG(int);
      else
        *buf = (wchar_t)btowc(GETARG(int));
      size = 1;
#endif
      result = buf;
      sign = '\0';
      break;
    case 'D':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'd':
    case 'i':
      if (flags & INTMAX_SIZE) {
        ujval = SJARG();
        if ((intmax_t)ujval < 0) {
          ujval = (uintmax_t)(-((intmax_t)ujval));
          sign = '-';
        }
      } else {
        ulval = SARG();
        if ((long)ulval < 0) {
          ulval = (u_long)(-((long)ulval));
          sign = '-';
        }
      }
      base = 10;
      goto number;
#ifndef NO_FLOATING_POINT
#ifdef WIDE_DOUBLE
    case 'a':
    case 'A':
      if (ch == 'a') {
        ox[1] = 'x';
        xdigs = xdigs_lower;
        expchar = 'p';
      } else {
        ox[1] = 'X';
        xdigs = xdigs_upper;
        expchar = 'P';
      }
      if (flags & LONGDBL) {
        fparg.ldbl = GETARG(long double);
        dtoaresult =
            __hldtoa(fparg.ldbl, xdigs, prec,
                &expt, &signflag, &dtoaend);
      } else {
        fparg.dbl = GETARG(double);
        dtoaresult =
            __hdtoa(fparg.dbl, xdigs, prec,
                &expt, &signflag, &dtoaend);
      }
      if (dtoaresult == NULL)
        goto oomem;

      if (prec < 0)
        prec = dtoaend - dtoaresult;
      if (expt == INT_MAX)
        ox[1] = '\0';
      ndig = dtoaend - dtoaresult;
      if (convbuf != NULL)
        free(convbuf);
#ifndef NARROW
      result = convbuf = __mbsconv(dtoaresult, -1);
#else
      /*XXX inefficient*/
      result = convbuf = strdup(dtoaresult);
#endif
      if (result == NULL)
        goto oomem;
      __freedtoa(dtoaresult);
      goto fp_common;
    case 'e':
    case 'E':
      expchar = ch;
      if (prec < 0)
        prec = DEFPREC;
      goto fp_begin;
    case 'f':
    case 'F':
      expchar = '\0';
      goto fp_begin;
    case 'g':
    case 'G':
      expchar = ch - ('g' - 'e');
      if (prec == 0)
        prec = 1;
fp_begin:
      if (prec < 0)
        prec = DEFPREC;
      if (flags & LONGDBL) {
        fparg.ldbl = GETARG(long double);
        dtoaresult =
            __ldtoa(&fparg.ldbl, expchar ? 2 : 3, prec,
            &expt, &signflag, &dtoaend);
      } else {
        fparg.dbl = GETARG(double);
        dtoaresult =
            __dtoa(fparg.dbl, expchar ? 2 : 3, prec,
            &expt, &signflag, &dtoaend);
        if (expt == 9999)
          expt = INT_MAX;
      }
      if (dtoaresult == NULL)
        goto oomem;
      ndig = dtoaend - dtoaresult;
      if (convbuf != NULL)
        free(convbuf);
#ifndef NARROW
      result = convbuf = __mbsconv(dtoaresult, -1);
#else
      /*XXX inefficient*/
      result = convbuf = strdup(dtoaresult);
#endif
      if (result == NULL)
        goto oomem;
      __freedtoa(dtoaresult);
fp_common:
      if (signflag)
        sign = '-';
      if (expt == INT_MAX) {  /* inf or nan */
        if (*result == 'N') {
          result = (ch >= 'a') ? STRCONST("nan") :
              STRCONST("NAN");
          sign = '\0';
        } else
          result = (ch >= 'a') ? STRCONST("inf") :
              STRCONST("INF");
        size = 3;
        break;
      }
#else
    //case 'e':
    //case 'E':
    //case 'f':
    //case 'F':
    //case 'g':
    //case 'G':
    //  if (prec == -1) {
    //    prec = DEFPREC;
    //  } else if ((ch == 'g' || ch == 'G') && prec == 0) {
    //    prec = 1;
    //  }
    case 'e':
    case 'E':
      expchar = ch;
      if (prec < 0)
        prec = DEFPREC;
      goto fp_begin;
    case 'f':
    case 'F':
      expchar = '\0';
      goto fp_begin;
    case 'g':
    case 'G':
      expchar = ch - ('g' - 'e');
      if (prec == 0)
        prec = 1;
fp_begin:
      if (prec < 0)
        prec = DEFPREC;

      if (flags & LONGDBL) {
        _double = (double) GETARG(long double);
      } else {
        _double = GETARG(double);
      }

      /* do this before tricky precision changes */
      if (isinf(_double)) {
        if (_double < 0)
          sign = '-';
        if (ch == 'E' || ch == 'F' || ch == 'G')
          result = STRCONST("INF");
        else
          result = STRCONST("inf");
        size = 3;
        break;
      }
      if (isnan(_double)) {
        if (ch == 'E' || ch == 'F' || ch == 'G')
          result = STRCONST("NAN");
        else
          result = STRCONST("nan");
        size = 3;
        break;
      }

      flags |= FPT;
      dtoaresult = cvt(_double, prec, flags, &softsign, &expt, ch, &ndig);
      if (dtoaresult == NULL)
        goto oomem;
      if (convbuf != NULL)
        free(convbuf);
#ifndef NARROW
      result = convbuf = __mbsconv(dtoaresult, -1);
#else
      /*XXX inefficient*/
      result = convbuf = strdup(dtoaresult);
#endif
      if (result == NULL)
        goto oomem;
      __freedtoa(dtoaresult);
      if (softsign)
        sign = '-';
#endif
      flags |= FPT;
      if (ch == 'g' || ch == 'G') {
        if (expt > -4 && expt <= prec) {
          /* Make %[gG] smell like %[fF] */
          expchar = '\0';
          if (flags & ALT)
            prec -= expt;
          else
            prec = ndig - expt;
          if (prec < 0)
            prec = 0;
        } else {
          /*
           * Make %[gG] smell like %[eE], but
           * trim trailing zeroes if no # flag.
           *
           * Note: The precision field used with [gG] is the number significant
           * digits to print.  When converting to [eE] the digit before the
           * decimal must not be included in the precision value.
           */
          if (!(flags & ALT))
            prec = ndig - 1;
        }
      }
      if (expchar) {
        dprec = prec; /* In some cases dprec will not be set.  Make sure it is set now */
        expsize = exponent(expstr, expt - 1, expchar);
        size = expsize + prec + 1; /* Leading digit + exponent string + precision */
        if (prec >= 1 || flags & ALT)
          ++size; /* Decimal point is added to character count */
      } else {
        /* space for digits before decimal point */
        if (expt > 0)
          size = expt;
        else  /* "0" */
          size = 1;
        /* space for decimal pt and following digits */
        if (prec || flags & ALT)
          size += prec + 1;
        if (grouping && expt > 0) {
          /* space for thousands' grouping */
          nseps = nrepeats = 0;
          lead = expt;
          while (*grouping != CHAR_MAX) {
            if (lead <= *grouping)
              break;
            lead -= *grouping;
            if (*(grouping+1)) {
              nseps++;
              grouping++;
            } else
              nrepeats++;
          }
          size += nseps + nrepeats;
        } else
          lead = expt;
      }
      break;
#endif /* !NO_FLOATING_POINT */
    case 'n':
      /*
       * Assignment-like behavior is specified if the
       * value overflows or is otherwise unrepresentable.
       * C99 says to use `signed char' for %hhn conversions.
       */
      if (flags & LLONGINT)
        *GETARG(long long *) = ret;
      else if (flags & SIZET)
        *GETARG(ssize_t *) = (ssize_t)ret;
      else if (flags & PTRDIFFT)
        *GETARG(ptrdiff_t *) = ret;
      else if (flags & INTMAXT)
        *GETARG(intmax_t *) = ret;
      else if (flags & LONGINT)
        *GETARG(long *) = ret;
      else if (flags & SHORTINT)
        *GETARG(short *) = ret;
      else if (flags & CHARINT)
        *GETARG(signed char *) = ret;
      else
        *GETARG(int *) = ret;
      continue; /* no output */
    case 'O':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'o':
      if (flags & INTMAX_SIZE)
        ujval = UJARG();
      else
        ulval = UARG();
      base = 8;
      goto nosign;
    case 'p':
      /*-
       * ``The argument shall be a pointer to void.  The
       * value of the pointer is converted to a sequence
       * of printable characters, in an implementation-
       * defined manner.''
       *  -- ANSI X3J11
       */
      ujval = (uintmax_t) (UINTN) GETARG(void *);
      base = 16;
      xdigs = xdigs_lower;
      flags = flags | INTMAXT;
      ox[1] = 'x';
      goto nosign;
    case 'S':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 's':
      if (((flags & LONGINT) ? 1:0) != MULTI) {
        if ((result = GETARG(CHAR_T *)) == NULL)
          result = STRCONST("(null)");
      } else {
        MCHAR_T *mc;

        if (convbuf != NULL)
          free(convbuf);
        if ((mc = GETARG(MCHAR_T *)) == NULL)
          result = STRCONST("(null)");
        else {
          convbuf = SCONV(mc, prec);
          if (convbuf == NULL) {
            fp->_flags |= __SERR;
            goto error;
          }
          result = convbuf;
        }
      }

      if (prec >= 0) {
        /*
         * can't use STRLEN; can only look for the
         * NUL in the first `prec' characters, and
         * STRLEN() will go further.
         */
        CHAR_T *p = MEMCHR(result, 0, (size_t)prec);

        if (p != NULL) {
          size = p - result;
          if (size > prec)
            size = prec;
        } else
          size = prec;
      } else
        size = (int)STRLEN(result);
      sign = '\0';
      break;
    case 'U':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'u':
      if (flags & INTMAX_SIZE)
        ujval = UJARG();
      else
        ulval = UARG();
      base = 10;
      goto nosign;
    case 'X':
      xdigs = xdigs_upper;
      goto hex;
    case 'x':
      xdigs = xdigs_lower;
hex:
      if (flags & INTMAX_SIZE)
        ujval = UJARG();
      else
        ulval = UARG();
      base = 16;
      /* leading 0x/X only if non-zero */
      if (flags & ALT &&
          (flags & INTMAX_SIZE ? ujval != 0 : ulval != 0))
        ox[1] = ch;

      flags &= ~GROUPING;
      /* unsigned conversions */
nosign:     sign = '\0';
      /*-
       * ``... diouXx conversions ... if a precision is
       * specified, the 0 flag will be ignored.''
       *  -- ANSI X3J11
       */
number:     if ((dprec = prec) >= 0)
        flags &= ~ZEROPAD;

      /*-
       * ``The result of converting a zero value with an
       * explicit precision of zero is no characters.''
       *  -- ANSI X3J11
       *
       * ``The C Standard is clear enough as is.  The call
       * printf("%#.0o", 0) should print 0.''
       *  -- Defect Report #151
       */
      result = cp = buf + BUF;
      if (flags & INTMAX_SIZE) {
        if (ujval != 0 || prec != 0 ||
            (flags & ALT && base == 8))
        {
          result = __ujtoa(ujval, cp, base,
              flags & ALT, xdigs,
              flags & GROUPING, thousands_sep,
              grouping);
        }
      } else {
        if (ulval != 0 || prec != 0 ||
            (flags & ALT && base == 8))
          result = __ultoa(ulval, cp, base,
              flags & ALT, xdigs,
              flags & GROUPING, thousands_sep,
              grouping);
      }
      size = buf + BUF - result;
      if (size > BUF) /* should never happen */
        abort();
      break;
    default:  /* "%?" prints ?, unless ? is NUL */
      if (ch == '\0')
        goto done;
      /* pretend it was %c with argument ch */
      *buf = ch;
      result = buf;
      size = 1;
      sign = '\0';
      break;
    }

    /*
     * All reasonable formats wind up here.  At this point, `result'
     * points to a string which (if not flags&LADJUST) should be
     * padded out to `width' places.  If flags&ZEROPAD, it should
     * first be prefixed by any sign or other prefix; otherwise,
     * it should be blank padded before the prefix is emitted.
     * After any left-hand padding and prefixing, emit zeroes
     * required by a decimal [diouxX] precision, then print the
     * string proper, then emit zeroes required by any leftover
     * floating precision; finally, if LADJUST, pad with blanks.
     *
     * Compute actual size, so we know how much to pad.
     * size excludes decimal prec; realsz includes it.
     */
    realsz = dprec > size ? dprec : size;
    if (sign)
      realsz++;
    if (ox[1])
      realsz += 2;

    prsize = width > realsz ? width : realsz;
    if ((unsigned)ret + prsize > INT_MAX) {
      ret = END_OF_FILE;
      goto error;
    }

    /* right-adjusting blank padding */
    if ((flags & (LADJUST|ZEROPAD)) == 0)
      PAD(width - realsz, blanks);

    /* prefix */
    if (sign)
      PRINT(&sign, 1);

    if (ox[1]) {  /* ox[1] is either x, X, or \0 */
      ox[0] = '0';
      PRINT(ox, 2);
    }

    /* right-adjusting zero padding */
    if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
      PAD(width - realsz, zeroes);

    /* leading zeroes from decimal precision */
    PAD(dprec - size, zeroes);

    /* the string or number proper */
#ifndef NO_FLOATING_POINT
    if ((flags & FPT) == 0) {
      PRINT(result, size);
    } else {  /* glue together f_p fragments */
      if (!expchar) { /* %[fF] or sufficiently short %[gG] */
        if (expt <= 0) {
          PRINT(zeroes, 1);
          if (prec || flags & ALT)
            PRINT(decimal_point, 1);
          PAD(-expt, zeroes);
          /* already handled initial 0's */
          prec += expt;
        } else {
          PRINTANDPAD(result, convbuf + ndig,
              lead, zeroes);
          result += lead;
          if (grouping) {
            while (nseps>0 || nrepeats>0) {
              if (nrepeats > 0)
                nrepeats--;
              else {
                grouping--;
                nseps--;
              }
              PRINT(&thousands_sep,
                  1);
              PRINTANDPAD(result,
                  convbuf + ndig,
                  *grouping, zeroes);
              result += *grouping;
            }
            if (result > convbuf + ndig)
              result = convbuf + ndig;
          }
          if (prec || flags & ALT) {
            buf[0] = *decimal_point;
            PRINT(buf, 1);
          }
        }
        PRINTANDPAD(result, convbuf + ndig, prec,
            zeroes);
      } else {  /* %[eE] or sufficiently long %[gG] */
        if (prec >= 1 || flags & ALT) {
          buf[0] = *result++;
          buf[1] = *decimal_point;
          PRINT(buf, 2);
          PRINT(result, ndig-1);
          PAD(prec - ndig, zeroes);
        } else  /* XeYYY */
          PRINT(result, 1);
        PRINT(expstr, expsize);
      }
    }
#else
    PRINT(result, size);
#endif
    /* left-adjusting padding (always blank) */
    if (flags & LADJUST)
      PAD(width - realsz, blanks);

    /* finally, adjust ret */
    ret += prsize;
    FLUSH();
  }
done:
  FLUSH();
error:
  va_end(orgap);
  if (convbuf != NULL)
    free(convbuf);
  if (__sferror(fp))
    ret = END_OF_FILE;
  if ((argtable != NULL) && (argtable != statargtable))
    free (argtable);
  return (ret);
  /* NOTREACHED */
oomem:
  errno = ENOMEM;
  ret = END_OF_FILE;
  goto error;
}

/*
 * Find all arguments when a positional parameter is encountered.  Returns a
 * table, indexed by argument number, of pointers to each arguments.  The
 * initial argument table should be an array of STATIC_ARG_TBL_SIZE entries.
 * It will be replaces with a malloc-ed one if it overflows.
 */
static int
__find_arguments(const CHAR_T *fmt0, va_list ap, union arg **argtable)
{
  CHAR_T *fmt;    /* format string */
  int ch;     /* character from fmt */
  int n, n2;    /* handy integer (short term usage) */
  CHAR_T *cp;   /* handy char pointer (short term usage) */
  int flags;    /* flags as above */
  enum typeid *typetable; /* table of types */
  enum typeid stattypetable [STATIC_ARG_TBL_SIZE];
  int tablesize;    /* current size of type table */
  int tablemax;   /* largest used index in table */
  int nextarg;    /* 1-based argument index */

  /*
   * Add an argument type to the table, expanding if necessary.
   */
#define ADDTYPE(type) \
  do { \
    if (nextarg >= tablesize) \
      if (__grow_type_table(nextarg, &typetable, \
          &tablesize) == -1) \
        return -1; \
    if (nextarg > tablemax) \
      tablemax = nextarg; \
    typetable[nextarg++] = type; \
  } while (/*CONSTCOND*/0)

#define ADDSARG() \
  do { \
    if (flags & INTMAXT)  \
      ADDTYPE(T_INTMAXT); \
    else if (flags & SIZET)  \
      ADDTYPE(T_SIZET); \
    else if (flags & PTRDIFFT) \
      ADDTYPE(T_PTRDIFFT); \
    else if (flags & LLONGINT) \
      ADDTYPE(T_LLONG); \
    else if (flags & LONGINT) \
      ADDTYPE(T_LONG); \
    else \
      ADDTYPE(T_INT); \
  } while (/*CONSTCOND*/0)

#define ADDUARG() \
  do { \
    if (flags & INTMAXT)  \
      ADDTYPE(T_UINTMAXT); \
    else if (flags & SIZET)  \
      ADDTYPE(T_SIZET); \
    else if (flags & PTRDIFFT) \
      ADDTYPE(T_PTRDIFFT); \
    else if (flags & LLONGINT) \
      ADDTYPE(T_U_LLONG); \
    else if (flags & LONGINT) \
      ADDTYPE(T_U_LONG); \
    else \
      ADDTYPE(T_U_INT); \
  } while (/*CONSTCOND*/0)
  /*
   * Add * arguments to the type array.
   */
#define ADDASTER() \
  n2 = 0; \
  cp = fmt; \
  while (is_digit(*cp)) { \
    n2 = 10 * n2 + to_digit(*cp); \
    cp++; \
  } \
  if (*cp == '$') { \
    int hold = nextarg; \
    nextarg = n2; \
    ADDTYPE(T_INT); \
    nextarg = hold; \
    fmt = ++cp; \
  } else { \
    ADDTYPE(T_INT); \
  }
  fmt = (CHAR_T *)__UNCONST(fmt0);
  typetable = stattypetable;
  tablesize = STATIC_ARG_TBL_SIZE;
  tablemax = 0;
  nextarg = 1;
  for (n = 0; n < STATIC_ARG_TBL_SIZE; n++)
    typetable[n] = T_UNUSED;

  /*
   * Scan the format for conversions (`%' character).
   */
  for (;;) {
    for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
      /* void */;
    if (ch == '\0')
      goto done;
    fmt++;    /* skip over '%' */

    flags = 0;

rflag:    ch = *fmt++;
reswitch: switch (ch) {
    case ' ':
    case '#':
      goto rflag;
    case '*':
      ADDASTER ();
      goto rflag;
    case '-':
    case '+':
    case '\'':
      goto rflag;
    case '.':
      if ((ch = *fmt++) == '*') {
        ADDASTER ();
        goto rflag;
      }
      while (is_digit(ch)) {
        ch = *fmt++;
      }
      goto reswitch;
    case '0':
      goto rflag;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      n = 0;
      do {
        n = 10 * n + to_digit(ch);
        ch = *fmt++;
      } while (is_digit(ch));
      if (ch == '$') {
        nextarg = n;
        goto rflag;
      }
      goto reswitch;
#ifndef NO_FLOATING_POINT
    case 'L':
      flags |= LONGDBL;
      goto rflag;
#endif
    case 'h':
      if (flags & SHORTINT) {
        flags &= ~SHORTINT;
        flags |= CHARINT;
      } else
        flags |= SHORTINT;
      goto rflag;
    case 'j':
      flags |= INTMAXT;
      goto rflag;
    case 'l':
      if (flags & LONGINT) {
        flags &= ~LONGINT;
        flags |= LLONGINT;
      } else
        flags |= LONGINT;
      goto rflag;
    case 'q':
      flags |= LLONGINT;  /* not necessarily */
      goto rflag;
    case 't':
      flags |= PTRDIFFT;
      goto rflag;
    case 'z':
      flags |= SIZET;
      goto rflag;
    case 'C':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'c':
      if (flags & LONGINT)
        ADDTYPE(T_WINT);
      else
        ADDTYPE(T_INT);
      break;
    case 'D':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'd':
    case 'i':
      ADDSARG();
      break;
#ifndef NO_FLOATING_POINT
    case 'a':
    case 'A':
    case 'e':
    case 'E':
    case 'f':
    case 'g':
    case 'G':
      if (flags & LONGDBL)
        ADDTYPE(T_LONG_DOUBLE);
      else
        ADDTYPE(T_DOUBLE);
      break;
#endif /* !NO_FLOATING_POINT */
    case 'n':
      if (flags & INTMAXT)
        ADDTYPE(TP_INTMAXT);
      else if (flags & PTRDIFFT)
        ADDTYPE(TP_PTRDIFFT);
      else if (flags & SIZET)
        ADDTYPE(TP_SIZET);
      else if (flags & LLONGINT)
        ADDTYPE(TP_LLONG);
      else if (flags & LONGINT)
        ADDTYPE(TP_LONG);
      else if (flags & SHORTINT)
        ADDTYPE(TP_SHORT);
      else if (flags & CHARINT)
        ADDTYPE(TP_SCHAR);
      else
        ADDTYPE(TP_INT);
      continue; /* no output */
    case 'O':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'o':
      ADDUARG();
      break;
    case 'p':
      ADDTYPE(TP_VOID);
      break;
    case 'S':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 's':
      if (flags & LONGINT)
        ADDTYPE(TP_WCHAR);
      else
        ADDTYPE(TP_CHAR);
      break;
    case 'U':
      flags |= LONGINT;
      /*FALLTHROUGH*/
    case 'u':
    case 'X':
    case 'x':
      ADDUARG();
      break;
    default:  /* "%?" prints ?, unless ? is NUL */
      if (ch == '\0')
        goto done;
      break;
    }
  }
done:
  /*
   * Build the argument table.
   */
  if (tablemax >= STATIC_ARG_TBL_SIZE) {
    *argtable = (union arg *)
        malloc (sizeof (union arg) * (tablemax + 1));
    if (*argtable == NULL)
      return -1;
  }

  (*argtable) [0].intarg = 0;
  for (n = 1; n <= tablemax; n++) {
    switch (typetable [n]) {
        case T_UNUSED: /* whoops! */
      (*argtable) [n].intarg = va_arg (ap, int);
      break;
        case TP_SCHAR:
      (*argtable) [n].pschararg = va_arg (ap, signed char *);
      break;
        case TP_SHORT:
      (*argtable) [n].pshortarg = va_arg (ap, short *);
      break;
        case T_INT:
      (*argtable) [n].intarg = va_arg (ap, int);
      break;
        case T_U_INT:
      (*argtable) [n].uintarg = va_arg (ap, unsigned int);
      break;
        case TP_INT:
      (*argtable) [n].pintarg = va_arg (ap, int *);
      break;
        case T_LONG:
      (*argtable) [n].longarg = va_arg (ap, long);
      break;
        case T_U_LONG:
      (*argtable) [n].ulongarg = va_arg (ap, unsigned long);
      break;
        case TP_LONG:
      (*argtable) [n].plongarg = va_arg (ap, long *);
      break;
        case T_LLONG:
      (*argtable) [n].longlongarg = va_arg (ap, long long);
      break;
        case T_U_LLONG:
      (*argtable) [n].ulonglongarg = va_arg (ap, unsigned long long);
      break;
        case TP_LLONG:
      (*argtable) [n].plonglongarg = va_arg (ap, long long *);
      break;
        case T_PTRDIFFT:
      (*argtable) [n].ptrdiffarg = va_arg (ap, ptrdiff_t);
      break;
        case TP_PTRDIFFT:
      (*argtable) [n].pptrdiffarg = va_arg (ap, ptrdiff_t *);
      break;
        case T_SIZET:
      (*argtable) [n].sizearg = va_arg (ap, size_t);
      break;
        case TP_SIZET:
      (*argtable) [n].psizearg = va_arg (ap, size_t *);
      break;
        case T_INTMAXT:
      (*argtable) [n].intmaxarg = va_arg (ap, intmax_t);
      break;
        case T_UINTMAXT:
      (*argtable) [n].uintmaxarg = va_arg (ap, uintmax_t);
      break;
        case TP_INTMAXT:
      (*argtable) [n].pintmaxarg = va_arg (ap, intmax_t *);
      break;
        case T_DOUBLE:
#ifndef NO_FLOATING_POINT
      (*argtable) [n].doublearg = va_arg (ap, double);
#endif
      break;
        case T_LONG_DOUBLE:
#ifndef NO_FLOATING_POINT
      (*argtable) [n].longdoublearg = va_arg (ap, long double);
#endif
      break;
        case TP_CHAR:
      (*argtable) [n].pchararg = va_arg (ap, char *);
      break;
        case TP_VOID:
      (*argtable) [n].pvoidarg = va_arg (ap, void *);
      break;
        case T_WINT:
      (*argtable) [n].wintarg = va_arg (ap, int);
      break;
        case TP_WCHAR:
      (*argtable) [n].pwchararg = va_arg (ap, wchar_t *);
      break;
    }
  }

  if ((typetable != NULL) && (typetable != stattypetable))
    free (typetable);
  return 0;
}

/*
 * Increase the size of the type table.
 */
static int
__grow_type_table (int nextarg, enum typeid **typetable, int *tablesize)
{
  enum typeid *const oldtable = *typetable;
  const int oldsize = *tablesize;
  enum typeid *newtable;
  int n, newsize = oldsize * 2;

  if (newsize < nextarg + 1)
    newsize = nextarg + 1;
  if (oldsize == STATIC_ARG_TBL_SIZE) {
    if ((newtable = malloc(newsize * sizeof(enum typeid))) == NULL)
      return -1;
    memcpy(newtable, oldtable, oldsize * sizeof(enum typeid));
  } else {
    newtable = realloc(oldtable, newsize * sizeof(enum typeid));
    if (newtable == NULL) {
      free(oldtable);
      return -1;
    }
  }
  for (n = oldsize; n < newsize; n++)
    newtable[n] = T_UNUSED;

  *typetable = newtable;
  *tablesize = newsize;
  return 0;
}


#ifndef NO_FLOATING_POINT
#ifndef WIDE_DOUBLE
static char *
cvt(double value, int ndigits, int flags, char *sign, int *decpt, int ch,
    int *length)
{
  int mode, dsgn;
  char *digits, *bp, *rve;

  _DIAGASSERT(decpt != NULL);
  _DIAGASSERT(length != NULL);
  _DIAGASSERT(sign != NULL);

  if (ch == 'f') {
    mode = 3;   /* ndigits after the decimal point */
  } else {
    /* To obtain ndigits after the decimal point for the 'e'
     * and 'E' formats, round to ndigits + 1 significant
     * figures.
     */
    if (ch == 'e' || ch == 'E') {
      ndigits++;
    }
    mode = 2;   /* ndigits significant digits */
  }

  digits = __dtoa(value, mode, ndigits, decpt, &dsgn, &rve);
  if (digits == NULL)
    return NULL;
  if (dsgn) {
    value = -value;
    *sign = '-';
  } else
    *sign = '\000';
  if ((ch != 'g' && ch != 'G') || flags & ALT) {  /* Print trailing zeros */
    bp = digits + ndigits;
    if (ch == 'f') {
      if (*digits == '0' && value)
        *decpt = -ndigits + 1;
      bp += *decpt;
    }
    while (rve < bp)
      *rve++ = '0';
  }
  *length = rve - digits;
  return digits;
}
#endif

static int
exponent(CHAR_T *p0, int expo, int fmtch)
{
  CHAR_T *p, *t;
  CHAR_T expbuf[MAXEXPDIG];

  p = p0;
  *p++ = fmtch;
  if (expo < 0) {
    expo = -expo;
    *p++ = '-';
  }
  else
    *p++ = '+';
  t = expbuf + MAXEXPDIG;
  if (expo > 9) {
    do {
      *--t = to_char(expo % 10);
    } while ((expo /= 10) > 9);
    *--t = to_char(expo);
    for (; t < expbuf + MAXEXPDIG; *p++ = *t++);
  }
  else {
    /*
     * Exponents for decimal floating point conversions
     * (%[eEgG]) must be at least two characters long,
     * whereas exponents for hexadecimal conversions can
     * be only one character long.
     */
    if (fmtch == 'e' || fmtch == 'E')
      *p++ = '0';
    *p++ = to_char(expo);
  }
  return (p - p0);
}
#endif /* !NO_FLOATING_POINT */
