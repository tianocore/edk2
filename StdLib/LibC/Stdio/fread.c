/** @file
    Implementation of fread as declared in <stdio.h>.

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

    NetBSD: fread.c,v 1.16 2003/08/07 16:43:25 agc Exp
    fread.c 8.2 (Berkeley) 12/11/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

size_t
fread(void *buf, size_t size, size_t count, FILE *fp)
{
  size_t resid;
  char *p;
  int r;
  size_t total;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (0);
  }
  /*
   * The ANSI standard requires a return value of 0 for a count
   * or a size of 0.  Whilst ANSI imposes no such requirements on
   * fwrite, the SUSv2 does.
   */
  if ((resid = count * size) == 0)
    return (0);

  _DIAGASSERT(buf != NULL);

  FLOCKFILE(fp);
  if (fp->_r < 0)
    fp->_r = 0;
  total = resid;
  p = buf;
  while (resid > (size_t)(r = fp->_r)) {
    (void)memcpy((void *)p, (void *)fp->_p, (size_t)r);
    fp->_p += r;
    /* fp->_r = 0 ... done in __srefill */
    p += r;
    resid -= r;
    if (__srefill(fp)) {
      /* no more input: return partial result */
      FUNLOCKFILE(fp);
      return ((total - resid) / size);
    }
  }
  (void)memcpy((void *)p, (void *)fp->_p, resid);
  fp->_r -= (int)resid;
  fp->_p += resid;
  FUNLOCKFILE(fp);
  return (count);
}
