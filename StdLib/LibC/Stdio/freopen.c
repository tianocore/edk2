/** @file
    Implementation of freopen as declared in <stdio.h>.

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

    NetBSD: freopen.c,v 1.14 2003/08/07 16:43:25 agc Exp
    freopen.c 8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <sys/types.h>
#include  <sys/stat.h>

#include  <assert.h>
#include  <errno.h>
#include  <fcntl.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <wchar.h>
#include  <unistd.h>
#include  "reentrant.h"
#include  "local.h"

/*
 * Re-direct an existing, open (probably) file to some other file.
 * ANSI is written such that the original file gets closed if at
 * all possible, no matter what.
 */
FILE *
freopen(const char *file, const char *mode, FILE *fp)
{
  int f;
  int flags, isopen, oflags, sverrno, wantfd;

  _DIAGASSERT(file != NULL);
  _DIAGASSERT(mode != NULL);
  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (NULL);
  }

  if ((flags = __sflags(mode, &oflags)) == 0) {
    (void) fclose(fp);
    return (NULL);
  }

  if (!__sdidinit)
    __sinit();

  /*
   * There are actually programs that depend on being able to "freopen"
   * descriptors that weren't originally open.  Keep this from breaking.
   * Remember whether the stream was open to begin with, and which file
   * descriptor (if any) was associated with it.  If it was attached to
   * a descriptor, defer closing it; freopen("/dev/stdin", "r", stdin)
   * should work.  This is unnecessary if it was not a Unix file.
   */
  if (fp->_flags == 0) {
    fp->_flags = __SEOF;  /* hold on to it */
    isopen = 0;
    wantfd = -1;
  } else {
    /* flush the stream; ANSI doesn't require this. */
    if (fp->_flags & __SWR)
      (void) __sflush(fp);
    /* if close is NULL, closing is a no-op, hence pointless */
    isopen = fp->_close != NULL;
    if (((wantfd = fp->_file) >= 0) && isopen) {
      (void) (*fp->_close)(fp->_cookie);
      isopen = 0;
    }
  }

  /* Get a new descriptor to refer to the new file. */
  f = open(file, oflags, DEFFILEMODE);
  if (f < 0 && isopen) {
    /* If out of fd's close the old one and try again. */
    if (errno == ENFILE || errno == EMFILE) {
      (void) (*fp->_close)(fp->_cookie);
      isopen = 0;
      f = open(file, oflags, DEFFILEMODE);
    }
  }
  sverrno = errno;

  /*
   * Finish closing fp.  Even if the open succeeded above, we cannot
   * keep fp->_base: it may be the wrong size.  This loses the effect
   * of any setbuffer calls, but stdio has always done this before.
   */
  if (isopen && (f != wantfd))
    (void) (*fp->_close)(fp->_cookie);
  if (fp->_flags & __SMBF)
    free((char *)fp->_bf._base);
  fp->_w = 0;
  fp->_r = 0;
  fp->_p = NULL;
  fp->_bf._base = NULL;
  fp->_bf._size = 0;
  fp->_lbfsize = 0;
  if (HASUB(fp))
    FREEUB(fp);
  WCIO_FREE(fp);
  _UB(fp)._size = 0;
  if (HASLB(fp))
    FREELB(fp);
  fp->_lb._size = 0;

  if (f < 0) {      /* did not get it after all */
    fp->_flags = 0;   /* set it free */
    errno = sverrno;  /* restore in case _close clobbered */
    return (NULL);
  }

  if (oflags & O_NONBLOCK) {
    struct stat st;
    if (fstat(f, &st) == -1) {
      sverrno = errno;
      (void)close(f);
      errno = sverrno;
      return (NULL);
    }
    if (!S_ISREG(st.st_mode)) {
      (void)close(f);
      errno = EFTYPE;
      return (NULL);
    }
  }

  /*
   * If reopening something that was open before on a real file, try
   * to maintain the descriptor.  Various C library routines (perror)
   * assume stderr is always fd STDERR_FILENO, even if being freopen'd.
   */
  if (wantfd >= 0 && f != wantfd) {
    if (dup2(f, wantfd) >= 0) {
      (void) close(f);
      f = wantfd;
    }
  }

  fp->_flags = (unsigned short)flags;
  fp->_file = (short)f;
  fp->_cookie = fp;
  fp->_read = __sread;
  fp->_write = __swrite;
  fp->_seek = __sseek;
  fp->_close = __sclose;

  /*
   * When reopening in append mode, even though we use O_APPEND,
   * we need to seek to the end so that ftell() gets the right
   * answer.  If the user then alters the seek pointer, or
   * the file extends, this will fail, but there is not much
   * we can do about this.  (We could set __SAPP and check in
   * fseek and ftell.)
   */
  if (oflags & O_APPEND)
    (void) __sseek((void *)fp, (fpos_t)0, SEEK_END);
  return (fp);
}
