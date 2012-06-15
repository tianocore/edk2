/** @file
    Implementation of internal file buffer allocation functions.

    Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
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

    NetBSD: makebuf.c,v 1.14 2003/08/07 16:43:28 agc Exp
    makebuf.c 8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>

#include  "namespace.h"

#include  <sys/types.h>
#include  <sys/stat.h>
#include  <assert.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  "reentrant.h"
#include  "local.h"
#include  <MainData.h>

/*
 * Allocate a file buffer, or switch to unbuffered I/O.
 * Per the ANSI C standard, ALL tty devices default to line buffered.
 *
 * As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
 * optimisation) right after the fstat() that finds the buffer size.
 */
void
__smakebuf(FILE *fp)
{
  void *p;
  int flags;
  size_t size;
  int couldbetty;

  _DIAGASSERT(fp != NULL);

  if (fp != NULL) {
  if (fp->_flags & __SNBF) {
    fp->_bf._base = fp->_p = fp->_nbuf;
    fp->_bf._size = MB_LEN_MAX;
    return;
  }
  flags = __swhatbuf(fp, &size, &couldbetty);
  if ((p = malloc(size)) == NULL) {
    // malloc failed, act unbuffered.
    fp->_flags |= __SNBF;
    fp->_bf._base = fp->_p = fp->_nbuf;
    fp->_bf._size = 1;
    return;
  }
  gMD->cleanup = _cleanup;
  flags |= __SMBF;
  fp->_bf._base = fp->_p = p;
  fp->_bf._size = (int)size;
  if (couldbetty || isatty(fp->_file))
    flags |= __SLBF;
  fp->_flags |= flags;
  }
}

/*
 * Internal routine to determine `proper' buffering for a file.
 */
int
__swhatbuf(FILE *fp, size_t *bufsize, int *couldbetty)
{
  struct stat st;

  _DIAGASSERT(fp != NULL);
  _DIAGASSERT(bufsize != NULL);
  _DIAGASSERT(couldbetty != NULL);
  if(fp == NULL) {
    return (__SNPT);
  }

  if (fp->_file < 0 || fstat(fp->_file, &st) < 0) {
    *couldbetty = 0;
    *bufsize = BUFSIZ;
    return (__SNPT);
  }

  /* could be a tty iff it is a character device */
  *couldbetty = S_ISCHR(st.st_mode);
  if (st.st_blksize == 0) {
    *bufsize = BUFSIZ;
    return (__SNPT);
  }

  /*
   * Optimise fseek() only if it is a regular file.  (The test for
   * __sseek is mainly paranoia.)  It is safe to set _blksize
   * unconditionally; it will only be used if __SOPT is also set.
   */
  *bufsize = st.st_blksize;
  fp->_blksize = st.st_blksize;
  return ((st.st_mode & S_IFMT) == S_IFREG && fp->_seek == __sseek ?
      __SOPT : __SNPT);
}
