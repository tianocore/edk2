/** @file
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Copyright (c) 1990, 1993
  The Regents of the University of California.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    3. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

  Original version ID:
  @(#)strtoul.c  8.1 (Berkeley) 6/4/93
  Citrus: xpg4dl/FreeBSD/lib/libc/locale/wcstoul.c,v 1.2 2001/09/21 16:11:41 yamt Exp
  NetBSD: wcstoul.c,v 1.1 2001/09/27 16:30:37 yamt Exp
  NetBSD: _wcstoul.h,v 1.3 2005/11/29 03:11:59 christos Exp
 */

#include <Library/BaseLib.h>

/*
 * function template for wcstoul, wcstoull and wcstoumax.
 *
 * parameters:
 *  _FUNCNAME  : function name
 *      __wUINT     : return type
 *      __wINT      : signed version of __wUINT
 *      __wUINT_MAX : upper limit of the return type
 */

__wUINT
_FUNCNAME(
  const wchar_t *nptr,
  wchar_t **endptr,
  int base
  )
{
  const wchar_t *s;
  __wUINT acc, cutoff;
  wint_t wc;
  int i;
  int neg, any, cutlim;

  _DIAGASSERT(nptr != NULL);
  /* endptr may be NULL */

  if (base && (base < 2 || base > 36)) {
    errno = EINVAL;
    return 0;
  }

  /*
   * Skip white space and pick up leading +/- sign if any.
   * If base is 0, allow 0x for hex and 0 for octal, else
   * assume decimal; if base is already 16, allow 0x.
   */
  s = nptr;
  do {
    wc = (wchar_t) *s++;
  } while (iswspace(wc));
  if (wc == L'-') {
    neg = 1;
    wc = *s++;
  } else {
    neg = 0;
    if (wc == L'+')
      wc = *s++;
  }
  if ((base == 0 || base == 16) &&
      wc == L'0' && (*s == L'x' || *s == L'X')) {
    wc = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = wc == L'0' ? 8 : 10;

  /*
   * See strtoul for comments as to the logic used.
   */
  cutoff = (__wUINT)DivU64x32 ((UINT64) __wUINT_MAX, (UINT32) base);
  cutlim = (int) ModU64x32 ((UINT64) __wUINT_MAX, (UINT32) base);
  for (acc = 0, any = 0;; wc = (wint_t) *s++) {
    i = __wctoint((wchar_t)wc);
    if (i == -1) {
      break;
    }
    if (i >= base)
      break;
    if (any < 0)
      continue;
    if (acc > cutoff || (acc == cutoff && i > cutlim)) {
      any = -1;
      acc = __wUINT_MAX;
      errno = ERANGE;
    } else {
      any = 1;
      acc *= (__wUINT)base;
      acc += i;
    }
  }
  if (neg && any > 0)
    acc = (__wUINT)(-((__wINT)acc));
  if (endptr != 0)
    *endptr = __UNCONST(any ? s - 1 : nptr);
  return (acc);
}
