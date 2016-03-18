/** @file
    Small standard I/O/seek/close functions.
    These maintain the `known seek offset' for seek optimisation.

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

    NetBSD: stdio.c,v 1.13 2003/08/07 16:43:33 agc Exp
    stdio.c 8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>

#include  "namespace.h"

#include  <assert.h>
#include  <errno.h>
#include  <fcntl.h>
#include  <stdio.h>
#include  <unistd.h>

#include  "reentrant.h"
#include  "local.h"

int
__sread(void *cookie, char *buf, int n)
{
  FILE *fp = cookie;
  int ret;

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(buf != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  ret = (int)read(fp->_file, buf, (size_t)n);
  /* if the read succeeded, update the current offset */
  if (ret >= 0)
    fp->_offset += ret;
  else
    fp->_flags &= ~__SOFF;  /* paranoia */
  return (ret);
}

int
__swrite(void *cookie, char const *buf, int n)
{
  FILE *fp = cookie;

  _DIAGASSERT(cookie != NULL);
  _DIAGASSERT(buf != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if (fp->_flags & __SAPP)
    (void) lseek(fp->_file, (off_t)0, SEEK_END);
  fp->_flags &= ~__SOFF;  /* in case FAPPEND mode is set */
  return (int)(write(fp->_file, (char *)buf, (size_t)n));
}

fpos_t
__sseek(void *cookie, fpos_t offset, int whence)
{
  FILE *fp = cookie;
  off_t ret;

  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  ret = lseek(fp->_file, (off_t)offset, whence);
  if (ret == -1L)
    fp->_flags &= ~__SOFF;
  else {
    fp->_flags |= __SOFF;
    fp->_offset = ret;
  }
  return (ret);
}

int
__sclose(void *cookie)
{

  _DIAGASSERT(cookie != NULL);
  if(cookie == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  return (close(((FILE *)cookie)->_file));
}
