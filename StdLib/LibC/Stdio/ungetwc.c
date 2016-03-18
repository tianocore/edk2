/*-
 * Copyright (c)2001 Citrus Project,
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
 * $Citrus$

  NetBSD: ungetwc.c,v 1.3 2005/06/12 05:21:27 lukem Exp
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <assert.h>
#include  <errno.h>
#include  <stdio.h>
#include  <wchar.h>
#include  "reentrant.h"
#include  "local.h"

wint_t
ungetwc(wint_t wc, FILE *fp)
{
  struct wchar_io_data *wcio;

  _DIAGASSERT(fp);

  if (wc == WEOF)
    return WEOF;

  FLOCKFILE(fp);
  _SET_ORIENTATION(fp, 1);
  /*
   * XXX since we have no way to transform a wchar string to
   * a char string in reverse order, we can't use ungetc.
   */
  /* XXX should we flush ungetc buffer? */

  wcio = WCIO_GET(fp);
  if (wcio == 0) {
    FUNLOCKFILE(fp);
    errno = ENOMEM; /* XXX */
    return WEOF;
  }

  if (wcio->wcio_ungetwc_inbuf >= WCIO_UNGETWC_BUFSIZE) {
    FUNLOCKFILE(fp);
    return WEOF;
  }

  wcio->wcio_ungetwc_buf[wcio->wcio_ungetwc_inbuf++] = (wchar_t)wc;
  __sclearerr(fp);
  FUNLOCKFILE(fp);

  return wc;
}
