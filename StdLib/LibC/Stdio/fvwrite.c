/*
    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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

    NetBSD: fvwrite.c,v 1.16.2.1 2007/05/07 19:49:09 pavel Exp
    fvwrite.c 8.1 (Berkeley) 6/4/93
*/
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"
#include "fvwrite.h"

/*
 * Write some memory regions.  Return zero on success, EOF on error.
 *
 * This routine is large and unsightly, but most of the ugliness due
 * to the three different kinds of output buffering is handled here.
 */
int
__sfvwrite(FILE *fp, struct __suio *uio)
{
  size_t len;
  char *p;
  struct __siov *iov;
  int w, s;
  char *nl;
  int nlknown, nldist;

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(uio != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if ((len = uio->uio_resid) == 0)
    return (0);
  /* make sure we can write */
  if (cantwrite(fp)) {
    errno = EBADF;
    return (EOF);
  }

//#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define COPY(n)   (void)memcpy((void *)fp->_p, (void *)p, (size_t)(n))

  iov = uio->uio_iov;
  p = iov->iov_base;
  len = iov->iov_len;
  iov++;
#define GETIOV(extra_work) \
  while (len == 0) { \
    extra_work; \
    p = iov->iov_base; \
    len = iov->iov_len; \
    iov++; \
  }
  if (fp->_flags & __SNBF) {
    /*
     * Unbuffered: write up to BUFSIZ bytes at a time.
     */
    do {
      GETIOV(;);
      w = (*fp->_write)(fp->_cookie, p,
          (int)MIN(len, BUFSIZ));
      if (w < 0)
        goto err;
      p += w;
      len -= w;
    } while ((uio->uio_resid -= w) > 0);
    uio->uio_resid = 0;   // Just in case it went negative such as when NL is expanded to CR NL
  } else if ((fp->_flags & __SLBF) == 0) {
    /*
     * Fully buffered: fill partially full buffer, if any,
     * and then flush.  If there is no partial buffer, write
     * one _bf._size byte chunk directly (without copying).
     *
     * String output is a special case: write as many bytes
     * as fit, but pretend we wrote everything.  This makes
     * snprintf() return the number of bytes needed, rather
     * than the number used, and avoids its write function
     * (so that the write function can be invalid).
     */
    do {
      GETIOV(;);
      if ((fp->_flags & (__SALC | __SSTR)) ==
          (__SALC | __SSTR) && fp->_w < (int)len) {
        size_t blen = fp->_p - fp->_bf._base;
        unsigned char *_base;
        int _size;

        /* Allocate space exponentially. */
        _size = fp->_bf._size;
        do {
          _size = (_size << 1) + 1;
        } while (_size < (int)(blen + len));
        _base = realloc(fp->_bf._base,
            (size_t)(_size + 1));
        if (_base == NULL)
          goto err;
        fp->_w += _size - fp->_bf._size;
        fp->_bf._base = _base;
        fp->_bf._size = _size;
        fp->_p = _base + blen;
      }
      w = fp->_w;
      if (fp->_flags & __SSTR) {
        if (len < (size_t)w)
          w = (int)len;
        COPY(w);  /* copy MIN(fp->_w,len), */
        fp->_w -= w;
        fp->_p += w;
        w = (int)len;  /* but pretend copied all */
      } else if (fp->_p > fp->_bf._base && len > (size_t)w) {
        /* fill and flush */
        COPY(w);
        /* fp->_w -= w; */ /* unneeded */
        fp->_p += w;
        if (fflush(fp))
          goto err;
      } else if (len >= (size_t)(w = fp->_bf._size)) {
        /* write directly */
        w = (*fp->_write)(fp->_cookie, p, w);
        if (w <= 0)
          goto err;
      } else {
        /* fill and done */
        w = (int)len;
        COPY(w);
        fp->_w -= w;
        fp->_p += w;
      }
      p += w;
      len -= w;
    } while ((uio->uio_resid -= w) != 0);
  } else {
    /*
     * Line buffered: like fully buffered, but we
     * must check for newlines.  Compute the distance
     * to the first newline (including the newline),
     * or `infinity' if there is none, then pretend
     * that the amount to write is MIN(len,nldist).
     */
    nlknown = 0;
    nldist = 0; /* XXX just to keep gcc happy */
    do {
      GETIOV(nlknown = 0);
      if (!nlknown) {
        nl = memchr((void *)p, '\n', len);          // Divide the string at the first '\n'
        nldist = (int)(nl ? nl + 1 - p : len + 1);
        nlknown = 1;
      }
      s = (int)(MIN((int)len, nldist));
      w = fp->_w + fp->_bf._size;
      if (fp->_p > fp->_bf._base && s > w) {
        COPY(w);
        /* fp->_w -= w; */
        fp->_p += w;
        if (fflush(fp))
          goto err;
      } else if (s >= (w = fp->_bf._size)) {
        w = (*fp->_write)(fp->_cookie, p, w);
        if (w <= 0)
          goto err;
      } else {
        w = s;
        COPY(w);
        fp->_w -= w;
        fp->_p += w;
      }
      if ((nldist -= w) == 0) {
        /* copied the newline: flush and forget */
        if (fflush(fp))
          goto err;
        nlknown = 0;
      }
      p += w;
      len -= w;
    } while ((uio->uio_resid -= w) != 0);
  }
  return (0);

err:
  fp->_flags |= __SERR;
  return (EOF);
}
