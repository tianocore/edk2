/** @file
    Implementation of fgets as declared in <stdio.h>.

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

    NetBSD: fgets.c,v 1.20 2003/12/14 23:56:28 lukem Exp
    fgets.c 8.2 (Berkeley) 12/22/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include  <errno.h>
#include "reentrant.h"
#include "local.h"

/*
 * Read at most n-1 characters from the given file.
 * Stop when a newline has been read, or the count runs out.
 * Return first argument, or NULL if no characters were read.
 */
char *
fgets(char *buf, int n, FILE *fp)
{
  size_t len;
  char *s;
  unsigned char *p, *t;

  _DIAGASSERT(buf != NULL);
  _DIAGASSERT(fp != NULL);
  if ((fp == NULL) || (n <= 0)) {        /* sanity check */
    errno = EINVAL;
    return (NULL);
  }

  FLOCKFILE(fp);
  _SET_ORIENTATION(fp, -1);
  s = buf;
  n--;      /* leave space for NUL */
  while (n != 0) {
    /*
     * If the buffer is empty, refill it.
     */
    if (fp->_r <= 0) {
      if (__srefill(fp)) {
        /* EOF/error: stop with partial or no line */
        if (s == buf) {
          FUNLOCKFILE(fp);
          return (NULL);
        }
        break;
      }
    }
    len = fp->_r;
    p = fp->_p;

    /*
     * Scan through at most n bytes of the current buffer,
     * looking for '\n'.  If found, copy up to and including
     * newline, and stop.  Otherwise, copy entire chunk
     * and loop.
     */
    if (len > (size_t)n)
      len = n;
    t = memchr((void *)p, '\n', len);
    if (t != NULL) {
      len = ++t - p;
      fp->_r -= (int)len;
      fp->_p = t;
      (void)memcpy((void *)s, (void *)p, len);
      s[len] = 0;
      FUNLOCKFILE(fp);
      return (buf);
    }
    fp->_r -= (int)len;
    fp->_p += len;
    (void)memcpy((void *)s, (void *)p, len);
    s += len;
    n -= (int)len;
  }
  *s = 0;
  FUNLOCKFILE(fp);
  return (buf);
}
