/** @file
    Implementation of fopen as declared in <stdio.h>.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

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

    NetBSD: fopen.c,v 1.12 2003/08/07 16:43:24 agc Exp
    fopen.c 8.1 (Berkeley) 6/4/93"
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <sys/types.h>
#include  <sys/stat.h>
#include  <assert.h>
#include  <fcntl.h>
#include  <stdio.h>
#include  <errno.h>
#include  <unistd.h>
#include  "reentrant.h"
#include  "local.h"

FILE *
fopen(const char *file, const char *mode)
{
  FILE *fp;
  int f;
  int flags, oflags;

  _DIAGASSERT(file != NULL);
  if ((flags = __sflags(mode, &oflags)) == 0)
    return (NULL);
  if ((fp = __sfp()) == NULL)
    return (NULL);
  if ((f = open(file, oflags, DEFFILEMODE)) < 0)
    goto release;
  if (oflags & O_NONBLOCK) {
    struct stat st;
    if (fstat(f, &st) == -1) {
      int sverrno = errno;
      (void)close(f);
      errno = sverrno;
      goto release;
    }
    if (!S_ISREG(st.st_mode)) {
      (void)close(f);
      errno = EFTYPE;
      goto release;
    }
  }
  fp->_file = (short)f;
  fp->_flags = (unsigned short)flags;
  fp->_cookie = fp;
  fp->_read = __sread;
  fp->_write = __swrite;
  fp->_seek = __sseek;
  fp->_close = __sclose;

  /*
   * When opening in append mode, even though we use O_APPEND,
   * we need to seek to the end so that ftell() gets the right
   * answer.  If the user then alters the seek pointer, or
   * the file extends, this will fail, but there is not much
   * we can do about this.  (We could set __SAPP and check in
   * fseek and ftell.)
   */
  if (oflags & O_APPEND)
    (void) __sseek((void *)fp, (fpos_t)0, SEEK_END);
  return (fp);
release:
  fp->_flags = 0;     /* release */
  return (NULL);
}
