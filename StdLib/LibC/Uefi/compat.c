/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: compat.c,v 1.1.1.1 2008/08/24 05:33:08 gmcgarry Exp $

 * Copyright (c) 1997, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein and Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *  $NetBSD: compat.c,v 1.1.1.1 2008/08/24 05:33:08 gmcgarry Exp $

 * Copyright (c) 1987, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  $NetBSD: compat.c,v 1.1.1.1 2008/08/24 05:33:08 gmcgarry Exp $
 */
#include  <LibConfig.h>
#include  <string.h>
#include  <fcntl.h>
#include  <sys/syslimits.h>

#ifndef HAVE_GETOPT
char *optarg;
int optind = 1;
int
getopt(int argc, char **argv, char *args)
{
        size_t n;
  size_t nlen = strlen(args);
        char cmd;
        char rv;

        if (argv[optind] && *argv[optind] == '-') {
                cmd = *(argv[optind] + 1);

                for (n = 0; n < nlen; n++) {
                        if (args[n] == ':')
        continue;
                        if (args[n] == cmd) {
                                rv = *(argv[optind] + 1);
                                if (args[n+1] == ':') {
          if (*(argv[optind] + 2) != '\0') {
                                          optarg = argv[optind] + 2;
            optind += 1;
          } else {
                                          optarg = argv[optind + 1];
                                          optind += 2;
          }
                                        if (!optarg)
             optarg="";
                                        return rv;
                                } else {
                                        optarg = NULL;
                                        optind += 1;
                                        return rv;
                                }
                        }
                }
        }
        return -1;
}
#endif

#define ISPATHSEPARATOR(x) ((x == '/') || (x == '\\'))

#ifdef HAVE_BASENAME
#ifndef PATH_MAX
  #define PATH_MAX 5000
#endif

char *
basename(char *path)
{
  static char singledot[] = ".";
  static char result[PATH_MAX];
  char *p, *lastp;
  size_t len;

  /*
   * If `path' is a null pointer or points to an empty string,
   * return a pointer to the string ".".
   */
  if ((path == NULL) || (*path == '\0'))
    return (singledot);

  /* Strip trailing slashes, if any. */
  lastp = path + strlen(path) - 1;
  while (lastp != path && ISPATHSEPARATOR(*lastp))
    lastp--;

  /* Now find the beginning of this (final) component. */
  p = lastp;
  while (p != path && !ISPATHSEPARATOR(*(p - 1)))
    p--;

  /* ...and copy the result into the result buffer. */
  len = (lastp - p) + 1 /* last char */;
  if (len > (PATH_MAX - 1))
    len = PATH_MAX - 1;

  memcpy(result, p, len);
  result[len] = '\0';

  return (result);
}
#endif

#if !defined(HAVE_MKSTEMP) && !defined(WIN32)
int
mkstemp(char *path)
{
  char *start, *trv;
  unsigned int pid;

  /* To guarantee multiple calls generate unique names even if
     the file is not created. 676 different possibilities with 7
     or more X's, 26 with 6 or less. */
  static char xtra[2] = "aa";
  int xcnt = 0;

  pid = getpid();

  /* Move to end of path and count trailing X's. */
  for (trv = path; *trv; ++trv)
    if (*trv == 'X')
      xcnt++;
    else
      xcnt = 0;

  /* Use at least one from xtra.  Use 2 if more than 6 X's. */
  if (*(trv - 1) == 'X')
    *--trv = xtra[0];
  if (xcnt > 6 && *(trv - 1) == 'X')
    *--trv = xtra[1];

  /* Set remaining X's to pid digits with 0's to the left. */
  while (*--trv == 'X') {
    *trv = (pid % 10) + '0';
    pid /= 10;
  }

  /* update xtra for next call. */
  if (xtra[0] != 'z')
    xtra[0]++;
  else {
    xtra[0] = 'a';
    if (xtra[1] != 'z')
      xtra[1]++;
    else
      xtra[1] = 'a';
  }

  return open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
}
#endif

#ifdef HAVE_FFS
int
ffs(int x)
{
  int r = 1;
  if (!x) return 0;
  if (!(x & 0xffff)) { x >>= 16; r += 16; }
  if (!(x &   0xff)) { x >>= 8;  r += 8;  }
  if (!(x &    0xf)) { x >>= 4;  r += 4;  }
  if (!(x &      3)) { x >>= 2;  r += 2;  }
  if (!(x &      1)) { x >>= 1;  r += 1;  }

  return r;
}
#endif

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)

static void
dopr(char *buffer, size_t maxlen, const char *format, va_list args);

static void
fmtstr(char *buffer, size_t *currlen, size_t maxlen, char *value, int flags,
    int min, int max);

static void
fmtint(char *buffer, size_t *currlen, size_t maxlen, long value, int base,
    int min, int max, int flags);

static void
fmtfp(char *buffer, size_t *currlen, size_t maxlen, long double fvalue,
    int min, int max, int flags);

static void
dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c);

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS  (1 << 0)
#define DP_F_PLUS   (1 << 1)
#define DP_F_SPACE  (1 << 2)
#define DP_F_NUM    (1 << 3)
#define DP_F_ZERO   (1 << 4)
#define DP_F_UP     (1 << 5)
#define DP_F_UNSIGNED   (1 << 6)

/* Conversion Flags */
#define DP_C_SHORT     1
#define DP_C_LONG      2
#define DP_C_LDOUBLE   3
#define DP_C_LONG_LONG 4

#define char_to_int(p) (p - '0')
#define abs_val(p) (p < 0 ? -p : p)


static void
dopr(char *buffer, size_t maxlen, const char *format, va_list args)
{
  char *strvalue, ch;
  long value;
  long double fvalue;
  int min = 0, max = -1, state = DP_S_DEFAULT, flags = 0, cflags = 0;
  size_t currlen = 0;

  ch = *format++;

  while (state != DP_S_DONE) {
    if ((ch == '\0') || (currlen >= maxlen))
      state = DP_S_DONE;

    switch(state) {
    case DP_S_DEFAULT:
      if (ch == '%')
        state = DP_S_FLAGS;
      else
        dopr_outch(buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) {
      case '-':
        flags |= DP_F_MINUS;
        ch = *format++;
        break;
      case '+':
        flags |= DP_F_PLUS;
        ch = *format++;
        break;
      case ' ':
        flags |= DP_F_SPACE;
        ch = *format++;
        break;
      case '#':
        flags |= DP_F_NUM;
        ch = *format++;
        break;
      case '0':
        flags |= DP_F_ZERO;
        ch = *format++;
        break;
      default:
        state = DP_S_MIN;
        break;
      }
      break;
    case DP_S_MIN:
      if (isdigit((unsigned char)ch)) {
        min = 10 * min + char_to_int (ch);
        ch = *format++;
      } else if (ch == '*') {
        min = va_arg (args, int);
        ch = *format++;
        state = DP_S_DOT;
      } else
        state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') {
        state = DP_S_MAX;
        ch = *format++;
      } else
        state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if (isdigit((unsigned char)ch)) {
        if (max < 0)
          max = 0;
        max = 10 * max + char_to_int(ch);
        ch = *format++;
      } else if (ch == '*') {
        max = va_arg (args, int);
        ch = *format++;
        state = DP_S_MOD;
      } else
        state = DP_S_MOD;
      break;
    case DP_S_MOD:
      switch (ch) {
      case 'h':
        cflags = DP_C_SHORT;
        ch = *format++;
        break;
      case 'l':
        cflags = DP_C_LONG;
        ch = *format++;
        if (ch == 'l') {
          cflags = DP_C_LONG_LONG;
          ch = *format++;
        }
        break;
      case 'q':
        cflags = DP_C_LONG_LONG;
        ch = *format++;
        break;
      case 'L':
        cflags = DP_C_LDOUBLE;
        ch = *format++;
        break;
      default:
        break;
      }
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) {
      case 'd':
      case 'i':
        if (cflags == DP_C_SHORT)
          value = va_arg(args, int);
        else if (cflags == DP_C_LONG)
          value = va_arg(args, long int);
        else if (cflags == DP_C_LONG_LONG)
          value = va_arg (args, long long);
        else
          value = va_arg (args, int);
        fmtint(buffer, &currlen, maxlen, value, 10, min, max, flags);
        break;
      case 'o':
        flags |= DP_F_UNSIGNED;
        if (cflags == DP_C_SHORT)
          value = va_arg(args, unsigned int);
        else if (cflags == DP_C_LONG)
          value = va_arg(args, unsigned long int);
        else if (cflags == DP_C_LONG_LONG)
          value = va_arg(args, unsigned long long);
        else
          value = va_arg(args, unsigned int);
        fmtint(buffer, &currlen, maxlen, value, 8, min, max, flags);
        break;
      case 'u':
        flags |= DP_F_UNSIGNED;
        if (cflags == DP_C_SHORT)
          value = va_arg(args, unsigned int);
        else if (cflags == DP_C_LONG)
          value = va_arg(args, unsigned long int);
        else if (cflags == DP_C_LONG_LONG)
          value = va_arg(args, unsigned long long);
        else
          value = va_arg(args, unsigned int);
        fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
        break;
      case 'X':
        flags |= DP_F_UP;
      case 'x':
        flags |= DP_F_UNSIGNED;
        if (cflags == DP_C_SHORT)
          value = va_arg(args, unsigned int);
        else if (cflags == DP_C_LONG)
          value = va_arg(args, unsigned long int);
        else if (cflags == DP_C_LONG_LONG)
          value = va_arg(args, unsigned long long);
        else
          value = va_arg(args, unsigned int);
        fmtint(buffer, &currlen, maxlen, value, 16, min, max, flags);
        break;
      case 'f':
        if (cflags == DP_C_LDOUBLE)
          fvalue = va_arg(args, long double);
        else
          fvalue = va_arg(args, double);
        /* um, floating point? */
        fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
        break;
      case 'E':
        flags |= DP_F_UP;
      case 'e':
        if (cflags == DP_C_LDOUBLE)
          fvalue = va_arg(args, long double);
        else
          fvalue = va_arg(args, double);
        break;
      case 'G':
        flags |= DP_F_UP;
      case 'g':
        if (cflags == DP_C_LDOUBLE)
          fvalue = va_arg(args, long double);
        else
          fvalue = va_arg(args, double);
        break;
      case 'c':
        dopr_outch(buffer, &currlen, maxlen, va_arg(args, int));
        break;
      case 's':
        strvalue = va_arg(args, char *);
        if (max < 0)
          max = maxlen; /* ie, no max */
        fmtstr(buffer, &currlen, maxlen, strvalue, flags, min, max);
        break;
      case 'p':
        strvalue = va_arg(args, void *);
        fmtint(buffer, &currlen, maxlen, (long) strvalue, 16, min, max, flags);
        break;
      case 'n':
        if (cflags == DP_C_SHORT) {
          short int *num;
          num = va_arg(args, short int *);
          *num = currlen;
        } else if (cflags == DP_C_LONG) {
          long int *num;
          num = va_arg(args, long int *);
          *num = currlen;
        } else if (cflags == DP_C_LONG_LONG) {
          long long *num;
          num = va_arg(args, long long *);
          *num = currlen;
        } else {
          int *num;
          num = va_arg(args, int *);
          *num = currlen;
        }
        break;
      case '%':
        dopr_outch(buffer, &currlen, maxlen, ch);
        break;
      case 'w': /* not supported yet, treat as next char */
        ch = *format++;
        break;
      default: /* Unknown, skip */
      break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default: /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (currlen < maxlen - 1)
    buffer[currlen] = '\0';
  else
    buffer[maxlen - 1] = '\0';
}

static void
fmtstr(char *buffer, size_t *currlen, size_t maxlen,
    char *value, int flags, int min, int max)
{
  int cnt = 0, padlen, strln;     /* amount to pad */

  if (value == 0)
    value = "<NULL>";

  for (strln = 0; value[strln]; ++strln); /* strlen */
  padlen = min - strln;
  if (padlen < 0)
    padlen = 0;
  if (flags & DP_F_MINUS)
    padlen = -padlen; /* Left Justify */

  while ((padlen > 0) && (cnt < max)) {
    dopr_outch(buffer, currlen, maxlen, ' ');
    --padlen;
    ++cnt;
  }
  while (*value && (cnt < max)) {
    dopr_outch(buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while ((padlen < 0) && (cnt < max)) {
    dopr_outch(buffer, currlen, maxlen, ' ');
    ++padlen;
    ++cnt;
  }
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void
fmtint(char *buffer, size_t *currlen, size_t maxlen,
    long value, int base, int min, int max, int flags)
{
  unsigned long uvalue;
  char convert[20];
  int signvalue = 0, place = 0, caps = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */

#define PADMAX(x,y) ((x) > (y) ? (x) : (y))

  if (max < 0)
    max = 0;

  uvalue = value;

  if (!(flags & DP_F_UNSIGNED)) {
    if (value < 0) {
      signvalue = '-';
      uvalue = -value;
    } else if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else if (flags & DP_F_SPACE)
      signvalue = ' ';
  }

  if (flags & DP_F_UP)
    caps = 1; /* Should characters be upper case? */
  do {
    convert[place++] =
        (caps ? "0123456789ABCDEF" : "0123456789abcdef")
        [uvalue % (unsigned)base];
    uvalue = (uvalue / (unsigned)base );
  } while (uvalue && (place < 20));
  if (place == 20)
    place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - PADMAX(max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0)
    zpadlen = 0;
  if (spadlen < 0)
    spadlen = 0;
  if (flags & DP_F_ZERO) {
    zpadlen = PADMAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS)
    spadlen = -spadlen; /* Left Justifty */

  /* Spaces */
  while (spadlen > 0) {
    dopr_outch(buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue)
    dopr_outch(buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if (zpadlen > 0) {
    while (zpadlen > 0) {
      dopr_outch(buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0)
    dopr_outch(buffer, currlen, maxlen, convert[--place]);

  /* Left Justified spaces */
  while (spadlen < 0) {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }
}

static long double
pow10(int exp)
{
  long double result = 1;

  while (exp) {
    result *= 10;
    exp--;
  }

  return result;
}

static long
round(long double value)
{
  long intpart = value;

  value -= intpart;
  if (value >= 0.5)
    intpart++;

  return intpart;
}

static void
fmtfp(char *buffer, size_t *currlen, size_t maxlen, long double fvalue,
      int min, int max, int flags)
{
  char iconvert[20], fconvert[20];
  int signvalue = 0, iplace = 0, fplace = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0, caps = 0;
  long intpart, fracpart;
  long double ufvalue;

  /*
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val(fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
    signvalue = '+';
  else if (flags & DP_F_SPACE)
    signvalue = ' ';

  intpart = ufvalue;

  /*
   * Sorry, we only support 9 digits past the decimal because of our
   * conversion method
   */
  if (max > 9)
    max = 9;

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  fracpart = round((pow10 (max)) * (ufvalue - intpart));

  if (fracpart >= pow10 (max)) {
    intpart++;
    fracpart -= pow10 (max);
  }

  /* Convert integer part */
  do {
    iconvert[iplace++] =
        (caps ? "0123456789ABCDEF" : "0123456789abcdef")
        [intpart % 10];
    intpart = (intpart / 10);
  } while(intpart && (iplace < 20));
  if (iplace == 20)
    iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  do {
    fconvert[fplace++] =
        (caps ? "0123456789ABCDEF" : "0123456789abcdef")
        [fracpart % 10];
    fracpart = (fracpart / 10);
  } while(fracpart && (fplace < 20));
  if (fplace == 20)
    fplace--;
  fconvert[fplace] = 0;

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0);
  zpadlen = max - fplace;
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0)
    padlen = 0;
  if (flags & DP_F_MINUS)
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) {
    if (signvalue) {
      dopr_outch(buffer, currlen, maxlen, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0) {
      dopr_outch(buffer, currlen, maxlen, '0');
      --padlen;
    }
  }
  while (padlen > 0) {
    dopr_outch(buffer, currlen, maxlen, ' ');
    --padlen;
  }
  if (signvalue)
    dopr_outch(buffer, currlen, maxlen, signvalue);

  while (iplace > 0)
    dopr_outch(buffer, currlen, maxlen, iconvert[--iplace]);

  /*
   * Decimal point.  This should probably use locale to find the
   * correct char to print out.
   */
  dopr_outch(buffer, currlen, maxlen, '.');

  while (fplace > 0)
    dopr_outch(buffer, currlen, maxlen, fconvert[--fplace]);

  while (zpadlen > 0) {
    dopr_outch(buffer, currlen, maxlen, '0');
    --zpadlen;
  }

  while (padlen < 0) {
    dopr_outch(buffer, currlen, maxlen, ' ');
    ++padlen;
  }
}

static void
dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c)
{
  if (*currlen < maxlen)
    buffer[(*currlen)++] = c;
}
#endif /* !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) */

#ifndef HAVE_VSNPRINTF
int
vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
  str[0] = 0;
  dopr(str, count, fmt, args);

  return(strlen(str));
}
#endif /* !HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
int
snprintf(char *str,size_t count,const char *fmt,...)
{
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(str, count, fmt, ap);
  va_end(ap);

  return(strlen(str));
}

#endif /* !HAVE_SNPRINTF */
