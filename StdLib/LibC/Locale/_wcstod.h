/* $NetBSD: _wcstod.h,v 1.1 2006/04/15 12:17:23 tnozaki Exp $ */

/*-
 * Copyright (c) 2002 Tim J. Robbins
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Original version ID:
 *   FreeBSD: /repoman/r/ncvs/src/lib/libc/locale/wcstod.c,v 1.4 2004/04/07 09:47:56 tjr Exp
 *   NetBSD: wcstod.c,v 1.8 2006/04/13 01:25:13 tnozaki Exp
 */

/*
 * function template for wcstof, wcstod, wcstold.
 *
 * parameters:
 *  _FUNCNAME    : function name
 *  _RETURN_TYPE : return type
 *  _STRTOD_FUNC : real conversion function
 */
#ifndef __WCSTOD_H_
#define __WCSTOD_H_

/*
 * Convert a string to a double-precision number.
 *
 * This is the wide-character counterpart of strto{f,d,ld}(). So that
 * we do not have to duplicate the code of strto{f,d,ld}() here,
 * we convert the supplied wide character string to multibyte and
 * call strto{f,d,ld}() on the result.
 * This assumes that the multibyte encoding is compatible with ASCII
 * for at least the digits, radix character and letters.
 */
_RETURN_TYPE
_FUNCNAME(const wchar_t * __restrict nptr, wchar_t ** __restrict endptr)
{
  const wchar_t *src, *start;
  _RETURN_TYPE val;
  char *buf, *end;
  size_t bufsiz, len;

  _DIAGASSERT(nptr != NULL);
  /* endptr may be null */

  src = nptr;
  while (iswspace((wint_t)*src) != 0)
    ++src;
  if (*src == L'\0')
    goto no_convert;

  /*
   * Convert the supplied numeric wide char. string to multibyte.
   *
   * We could attempt to find the end of the numeric portion of the
   * wide char. string to avoid converting unneeded characters but
   * choose not to bother; optimising the uncommon case where
   * the input string contains a lot of text after the number
   * duplicates a lot of strto{f,d,ld}()'s functionality and
   * slows down the most common cases.
   */
  start = src;
  len = wcstombs(NULL, src, 0);
  if (len == (size_t)-1)
    /* errno = EILSEQ */
    goto no_convert;

  _DIAGASSERT(len > 0);

  bufsiz = len;
  buf = (void *)malloc(bufsiz + 1);
  if (buf == NULL)
    /* errno = ENOMEM */
    goto no_convert;

  len = wcstombs(buf, src, bufsiz + 1);

  _DIAGASSERT(len == bufsiz);
  _DIAGASSERT(buf[len] == '\0');

  /* Let strto{f,d,ld}() do most of the work for us. */
  val = _STRTOD_FUNC(buf, &end);
  if (buf == end) {
    free(buf);
    goto no_convert;
  }

  /*
   * We only know where the number ended in the _multibyte_
   * representation of the string. If the caller wants to know
   * where it ended, count multibyte characters to find the
   * corresponding position in the wide char string.
   */
  if (endptr != NULL)
    /* XXX Assume each wide char is one byte. */
    *endptr = __UNCONST(start + (size_t)(end - buf));

  free(buf);

  return val;

no_convert:
  if (endptr != NULL)
    *endptr = __UNCONST(nptr);
  return 0.0;
}
#endif /*__WCSTOD_H_*/
