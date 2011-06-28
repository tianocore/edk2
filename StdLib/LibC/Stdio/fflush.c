/** @file
    Implementation of fflush as declared in <stdio.h>.

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

    NetBSD: fflush.c,v 1.15 2003/08/07 16:43:22 agc Exp
    fflush.c  8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "reentrant.h"
#include "local.h"

#ifdef _REENTRANT
extern rwlock_t __sfp_lock;
#endif

/* Flush a single file, or (if fp is NULL) all files.  */
int
fflush(FILE *fp)
{
  int r;

  if (fp == NULL) {
    rwlock_rdlock(&__sfp_lock);
    r = _fwalk(__sflush);
    rwlock_unlock(&__sfp_lock);
    return r;
  }

  FLOCKFILE(fp);
  if ((fp->_flags & (__SWR | __SRW)) == 0) {
    errno = EBADF;
    r = EOF;
  } else {
    r = __sflush(fp);
  }
  FUNLOCKFILE(fp);
  return r;
}

int
__sflush(FILE *fp)
{
  unsigned char *p;
  INT64 n;
  int   t;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  t = fp->_flags;
  if ((t & __SWR) == 0)
    return (0);

  if ((p = fp->_bf._base) == NULL)
    return (0);

  n = fp->_p - p;   /* write this much */

  /*
   * Set these immediately to avoid problems with longjmp and to allow
   * exchange buffering (via setvbuf) in user write function.
   */
  fp->_p = p;
  fp->_w = t & (__SLBF|__SNBF) ? 0 : fp->_bf._size;

  for (; n > 0; n -= t, p += t) {
    t = (*fp->_write)(fp->_cookie, (char *)p, (int)n);
    if (t <= 0) {
      fp->_flags |= __SERR;
      return (EOF);
    }
  }
  return (0);
}
