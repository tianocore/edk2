/** @file
    Implementation of ungetc as declared in <stdio.h>.

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

    NetBSD: ungetc.c,v 1.14 2003/08/07 16:43:34 agc Exp
    ungetc.c  8.2 (Berkeley) 11/3/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

static int __submore(FILE *);
/*
 * Expand the ungetc buffer `in place'.  That is, adjust fp->_p when
 * the buffer moves, so that it points the same distance from the end,
 * and move the bytes in the buffer around as necessary so that they
 * are all at the end (stack-style).
 */
static int
__submore(FILE *fp)
{
  int i;
  unsigned char *p;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if (_UB(fp)._base == fp->_ubuf) {
    /*
     * Get a new buffer (rather than expanding the old one).
     */
    if ((p = malloc((size_t)BUFSIZ)) == NULL)
      return (EOF);
    _UB(fp)._base = p;
    _UB(fp)._size = BUFSIZ;
    p += BUFSIZ - sizeof(fp->_ubuf);
    for (i = sizeof(fp->_ubuf); --i >= 0;)
      p[i] = fp->_ubuf[i];
    fp->_p = p;
    return (0);
  }
  i = _UB(fp)._size;
  p = realloc(_UB(fp)._base, (size_t)(i << 1));
  if (p == NULL)
    return (EOF);
  /* no overlap (hence can use memcpy) because we doubled the size */
  (void)memcpy((void *)(p + i), (void *)p, (size_t)i);
  fp->_p = p + i;
  _UB(fp)._base = p;
  _UB(fp)._size = i << 1;
  return (0);
}

int
ungetc(int c, FILE *fp)
{
  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if (c == EOF)
    return (EOF);
  if (!__sdidinit)
    __sinit();
  FLOCKFILE(fp);
  _SET_ORIENTATION(fp, -1);
  if ((fp->_flags & __SRD) == 0) {
    /*
     * Not already reading: no good unless reading-and-writing.
     * Otherwise, flush any current write stuff.
     */
    if ((fp->_flags & __SRW) == 0) {
      FUNLOCKFILE(fp);
      return (EOF);
    }
    if (fp->_flags & __SWR) {
      if (__sflush(fp)) {
        FUNLOCKFILE(fp);
        return (EOF);
      }
      fp->_flags &= ~__SWR;
      fp->_w = 0;
      fp->_lbfsize = 0;
    }
    fp->_flags |= __SRD;
  }
  c = (unsigned char)c;

  /*
   * If we are in the middle of ungetc'ing, just continue.
   * This may require expanding the current ungetc buffer.
   */
  if (HASUB(fp)) {
    if (fp->_r >= _UB(fp)._size && __submore(fp)) {
      FUNLOCKFILE(fp);
      return (EOF);
    }
    *--fp->_p = (unsigned char)c;
    fp->_r++;
    FUNLOCKFILE(fp);
    return (c);
  }
  fp->_flags &= ~__SEOF;

  /*
   * If we can handle this by simply backing up, do so,
   * but never replace the original character.
   * (This makes sscanf() work when scanning `const' data.)
   */
  if (fp->_bf._base != NULL && fp->_p > fp->_bf._base &&
      fp->_p[-1] == c) {
    fp->_p--;
    fp->_r++;
    FUNLOCKFILE(fp);
    return (c);
  }

  /*
   * Create an ungetc buffer.
   * Initially, we will use the `reserve' buffer.
   */
  fp->_ur = fp->_r;
  fp->_up = fp->_p;
  _UB(fp)._base = fp->_ubuf;
  _UB(fp)._size = sizeof(fp->_ubuf);
  fp->_ubuf[sizeof(fp->_ubuf) - 1] = (unsigned char)c;
  fp->_p = &fp->_ubuf[sizeof(fp->_ubuf) - 1];
  fp->_r = 1;
  FUNLOCKFILE(fp);
  return (c);
}
