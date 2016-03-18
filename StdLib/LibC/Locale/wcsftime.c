/*   $NetBSD: wcsftime.c,v 1.2 2006/10/04 14:19:16 tnozaki Exp $  */
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
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
__FBSDID("$FreeBSD: src/lib/libc/locale/wcsftime.c,v 1.4 2004/04/07 09:47:56 tjr Exp $");
#else
__RCSID("$NetBSD: wcsftime.c,v 1.2 2006/10/04 14:19:16 tnozaki Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include  <machine/int_limits.h>

/*
 * Convert date and time to a wide-character string.
 *
 * This is the wide-character counterpart of strftime(). So that we do not
 * have to duplicate the code of strftime(), we convert the format string to
 * multibyte, call strftime(), then convert the result back into wide
 * characters.
 *
 * This technique loses in the presence of stateful multibyte encoding if any
 * of the conversions in the format string change conversion state. When
 * stateful encoding is implemented, we will need to reset the state between
 * format specifications in the format string.
 */
size_t
wcsftime(wchar_t *wcs, size_t maxsize,
    const wchar_t *format, const struct tm *timeptr)
{
  char *dst, *dstp, *sformat;
  size_t n, sflen;
  int sverrno;

  sformat = dst = NULL;

  /*
   * Convert the supplied format string to a multibyte representation
   * for strftime(), which only handles single-byte characters.
   */
  sflen = wcstombs(NULL, format, 0);
  if (sflen == (size_t)-1)
    goto error;
  if ((sformat = malloc(sflen + 1)) == NULL)
    goto error;
  wcstombs(sformat, format, sflen + 1);

  /*
   * Allocate memory for longest multibyte sequence that will fit
   * into the caller's buffer and call strftime() to fill it.
   * Then, copy and convert the result back into wide characters in
   * the caller's buffer.
   */
  if (SIZE_T_MAX / MB_CUR_MAX <= maxsize) {
    /* maxsize is preposturously large - avoid int. overflow. */
    errno = EINVAL;
    goto error;
  }
  dst = malloc(maxsize * MB_CUR_MAX);
  if (dst == NULL)
    goto error;
  if (strftime(dst, maxsize, sformat, timeptr) == 0)
    goto error;
  dstp = dst;
  n = mbstowcs(wcs, dstp, maxsize);
  if (n == (size_t)-2 || n == (size_t)-1)
    goto error;

  free(sformat);
  free(dst);
  return n;

error:
  sverrno = errno;
  free(sformat);
  free(dst);
  errno = sverrno;
  return 0;
}
