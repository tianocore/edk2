/*
    Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

NetBSD: fputwc.c,v 1.4 2005/06/12 05:21:27 lukem Exp
*/
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <wchar.h>
#include "reentrant.h"
#include "local.h"
#include "fvwrite.h"

wint_t
__fputwc_unlock(wchar_t wc, FILE *fp)
{
  struct wchar_io_data *wcio;
  mbstate_t *st;
  size_t size;
  char buf[MB_LEN_MAX];
  struct __suio uio;
  struct __siov iov;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (WEOF);
  }

  /* LINTED we don't play with buf */
  iov.iov_base = (void *)buf;
  uio.uio_iov = &iov;
  uio.uio_iovcnt = 1;

  _SET_ORIENTATION(fp, 1);
  wcio = WCIO_GET(fp);
  if (wcio == 0) {
    errno = ENOMEM;
    return WEOF;
  }

  wcio->wcio_ungetwc_inbuf = 0;
  st = &wcio->wcio_mbstate_out;

  size = wcrtomb(buf, wc, st);
  if (size == (size_t)-1) {
    return WEOF;
  }

  _DIAGASSERT(size != 0);

  uio.uio_resid = (int)(iov.iov_len = size);
  if (__sfvwrite(fp, &uio)) {
    return WEOF;
  }

  return (wint_t)wc;
}

wint_t
fputwc(wchar_t wc, FILE *fp)
{
  wint_t r;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (WEOF);
  }

  FLOCKFILE(fp);
  r = __fputwc_unlock(wc, fp);
  FUNLOCKFILE(fp);

  return (r);
}
