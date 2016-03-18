/** @file
    Implementation of internal function to return the (stdio) flags for a given mode.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
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

    NetBSD: flags.c,v 1.14 2003/08/07 16:43:23 agc Exp
    flags.c 8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "reentrant.h"
#include "local.h"

/*
 * Return the (stdio) flags for a given mode.  Store the flags
 * to be passed to an open() syscall through *optr.
 * Return 0 on error.
 */
int
__sflags(const char *mode, int *optr)
{
  int ret, m, o;

  _DIAGASSERT(mode != NULL);

  switch (*mode++) {

    case 'r': /* open for reading */
      ret = __SRD;
      m = O_RDONLY;
      o = 0;
      break;

    case 'w': /* open for writing */
      ret = __SWR;
      m = O_WRONLY;
      o = O_CREAT | O_TRUNC;
      break;

    case 'a': /* open for appending */
      ret = __SWR;
      m = O_WRONLY;
      o = O_CREAT | O_APPEND;
      break;

    default:  /* illegal mode */
      errno = EINVAL;
      return (0);
  }

  /*
   * [rwa]\+ or [rwa]b\+ means read and write
   * f means open only plain files.
   */
  for (; *mode; mode++)
    switch (*mode) {
    case '+':
      ret = __SRW;
      m = O_RDWR;
      break;
    case 'f':
      o |= O_NONBLOCK;
      break;
    case 'b':
      break;
    default:  /* We could produce a warning here */
      break;
    }

  *optr = m | o;
  return (ret);
}
