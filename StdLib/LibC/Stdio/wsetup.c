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

    NetBSD: wsetup.c,v 1.11 2003/08/07 16:43:35 agc Exp
    wsetup.c  8.1 (Berkeley) 6/4/93
*/
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "reentrant.h"
#include "local.h"

/*
 * Various output routines call wsetup to be sure it is safe to write,
 * because either _flags does not include __SWR, or _buf is NULL.
 * _wsetup returns 0 if OK to write, nonzero otherwise.
 */
int
__swsetup(FILE *fp)
{

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (-1);
  }

  /* make sure stdio is set up */
  if (!__sdidinit)
    __sinit();

  /*
   * If we are not writing, we had better be reading and writing.
   */
  if ((fp->_flags & __SWR) == 0) {
    if ((fp->_flags & __SRW) == 0)
      return (EOF);
    if (fp->_flags & __SRD) {
      /* clobber any ungetc data */
      if (HASUB(fp))
        FREEUB(fp);
      fp->_flags &= ~(__SRD|__SEOF);
      fp->_r = 0;
      fp->_p = fp->_bf._base;
    }
    fp->_flags |= __SWR;
  }

  /*
   * Make a buffer if necessary, then set _w.
   */
  if (fp->_bf._base == NULL)
    __smakebuf(fp);
  if (fp->_flags & __SLBF) {
    /*
     * It is line buffered, so make _lbfsize be -_bufsize
     * for the putc() macro.  We will change _lbfsize back
     * to 0 whenever we turn off __SWR.
     */
    fp->_w = 0;
    fp->_lbfsize = -fp->_bf._size;
  } else
    fp->_w = fp->_flags & __SNBF ? 0 : fp->_bf._size;
  return (0);
}
