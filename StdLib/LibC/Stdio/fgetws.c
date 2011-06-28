/*
    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 2002 Tim J. Robbins.
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
 * FreeBSD: src/lib/libc/stdio/fgetws.c,v 1.4 2002/09/20 13:25:40 tjr Exp

    $NetBSD: fgetws.c,v 1.2 2006/07/03 17:06:36 tnozaki Exp $
*/
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <wchar.h>
#include "reentrant.h"
#include "local.h"

wchar_t *
fgetws(
  wchar_t * __restrict ws,
  int n,
  FILE * __restrict fp
  )
{
  wchar_t *wsp;
  wint_t wc;

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(ws != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (NULL);
  }

  FLOCKFILE(fp);
  _SET_ORIENTATION(fp, 1);

  if (n <= 0) {
    errno = EINVAL;
    goto error;
  }

  wsp = ws;
  while (n-- > 1) {
    wc = __fgetwc_unlock(fp);
    if (__sferror(fp) != 0)
      goto error;
    if (__sfeof(fp) != 0) {
      if (wsp == ws) {
        /* EOF/error, no characters read yet. */
        goto error;
      }
      break;
    }
    *wsp++ = (wchar_t)wc;
    if (wc == L'\n') {
      break;
    }
  }

  *wsp++ = L'\0';
  FUNLOCKFILE(fp);

  return (ws);

error:
  FUNLOCKFILE(fp);
  return (NULL);
}
