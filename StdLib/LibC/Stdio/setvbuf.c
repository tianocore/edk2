/** @file
    Implementation of setvbuf as declared in <stdio.h>.

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

    NetBSD: setvbuf.c,v 1.17 2003/08/07 16:43:31 agc Exp
    setvbuf.c 8.2 (Berkeley) 11/16/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "reentrant.h"
#include "local.h"
#include  <MainData.h>

/*
 * Set one of the three kinds of buffering, optionally including
 * a buffer.
 */
int
setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
  int ret, flags;
  size_t iosize;
  int ttyflag;

  _DIAGASSERT(fp != NULL);
  /* buf may be NULL */
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  /*
   * Verify arguments.  The `int' limit on `size' is due to this
   * particular implementation.  Note, buf and size are ignored
   * when setting _IONBF.
   */
  if (mode != _IONBF)
    if ((mode != _IOFBF && mode != _IOLBF) || (int)size < 0)
      return (-1);

  FLOCKFILE(fp);
  /*
   * Write current buffer, if any.  Discard unread input (including
   * ungetc data), cancel line buffering, and free old buffer if
   * malloc()ed.  We also clear any eof condition, as if this were
   * a seek.
   */
  ret = 0;
  (void)__sflush(fp);
  if (HASUB(fp))
    FREEUB(fp);
  WCIO_FREE(fp);
  fp->_r = fp->_lbfsize = 0;
  flags = fp->_flags;
  if (flags & __SMBF)
    free((void *)fp->_bf._base);
  flags &= ~(__SLBF | __SNBF | __SMBF | __SOPT | __SNPT | __SEOF);

  /* If setting unbuffered mode, skip all the hard work. */
  if (mode == _IONBF)
    goto nbf;

  /*
   * Find optimal I/O size for seek optimization.  This also returns
   * a `tty flag' to suggest that we check isatty(fd), but we do not
   * care since our caller told us how to buffer.
   */
  flags |= __swhatbuf(fp, &iosize, &ttyflag);
  if (size == 0) {
    buf = NULL; /* force local allocation */
    size = iosize;
  }

  /* Allocate buffer if needed. */
  if (buf == NULL) {
    if ((buf = malloc(size)) == NULL) {
      /*
       * Unable to honor user's request.  We will return
       * failure, but try again with file system size.
       */
      ret = -1;
      if (size != iosize) {
        size = iosize;
        buf = malloc(size);
      }
    }
    if (buf == NULL) {
      /* No luck; switch to unbuffered I/O. */
nbf:
      fp->_flags = (unsigned short)(flags | __SNBF);
      fp->_w = 0;
      fp->_bf._base = fp->_p = fp->_nbuf;
      fp->_bf._size = 1;
      FUNLOCKFILE(fp);
      return (ret);
    }
    flags |= __SMBF;
  }

  /*
   * Kill any seek optimization if the buffer is not the
   * right size.
   *
   * SHOULD WE ALLOW MULTIPLES HERE (i.e., ok iff (size % iosize) == 0)?
   */
  if (size != iosize)
    flags |= __SNPT;

  /*
   * Fix up the FILE fields, and set gMD->cleanup for output flush on
   * exit (since we are buffered in some way).
   */
  if (mode == _IOLBF)
    flags |= __SLBF;
  fp->_flags = (unsigned short)flags;
  fp->_bf._base = fp->_p = (unsigned char *)buf;
  fp->_bf._size = (int)size;
  /* fp->_lbfsize is still 0 */
  if (flags & __SWR) {
    /*
     * Begin or continue writing: see __swsetup().  Note
     * that __SNBF is impossible (it was handled earlier).
     */
    if (flags & __SLBF) {
      fp->_w = 0;
      fp->_lbfsize = -fp->_bf._size;
    } else
      fp->_w = (int)size;
  } else {
    /* begin/continue reading, or stay in intermediate state */
    fp->_w = 0;
  }
  gMD->cleanup = _cleanup;

  FUNLOCKFILE(fp);
  return (ret);
}
