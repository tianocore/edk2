/*  $NetBSD: strtoumax.c,v 1.1 2006/04/22 15:33:33 thorpej Exp $  */

/*
 * Copyright (c) 1990, 1993
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
 */
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "from: @(#)strtoul.c 8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: strtoumax.c,v 1.1 2006/04/22 15:33:33 thorpej Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>

#include <Library/BaseLib.h>

#ifdef __weak_alias
__weak_alias(strtoumax, _strtoumax)
#endif

#else /* !_KERNEL && !_STANDALONE */
#include <sys/param.h>
#include <lib/libkern/libkern.h>
#endif /* !_KERNEL && !_STANDALONE */

/*
 * Convert a string to an uintmax_t.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
uintmax_t
strtoumax(const char *nptr, char **endptr, int base)
{
  const char *s;
  uintmax_t acc, cutoff;
  int c;
  int neg, any, cutlim;

  _DIAGASSERT(nptr != NULL);
  /* endptr may be NULL */

  /*
   * See strtol for comments as to the logic used.
   */
  s = nptr;
  do {
    c = (unsigned char) *s++;
  } while (isspace(c));
  if (c == '-') {
    neg = 1;
    c = *s++;
  } else {
    neg = 0;
    if (c == '+')
      c = *s++;
  }
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X')) {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;

  cutoff = DivU64x32 ((UINT64) UINTMAX_MAX, (UINT32) base);
  cutlim = (int) ModU64x32 ((UINT64) UINTMAX_MAX, (UINT32) base);
  for (acc = 0, any = 0;; c = (unsigned char) *s++) {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c)) {
#if defined(_KERNEL) || defined(_STANDALONE)
      c = toupper(c) - 'A' + 10;
#else
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
#endif
    } else
      break;
    if (c >= base)
      break;
    if (any < 0)
      continue;
    if (acc > cutoff || (acc == cutoff && c > cutlim)) {
#if defined(_KERNEL) || defined(_STANDALONE)
      if (endptr)
        *endptr = __UNCONST(nptr);
      return UINTMAX_MAX;
#else
      any = -1;
      acc = UINTMAX_MAX;
      errno = ERANGE;
#endif
    } else {
      any = 1;
      acc *= (uintmax_t)base;
      acc += c;
    }
  }
  if (neg && any > 0)
    acc = (uintmax_t)(-((intmax_t)acc));
  if (endptr != 0)
    *endptr = __UNCONST(any ? s - 1 : nptr);
  return (acc);
}
