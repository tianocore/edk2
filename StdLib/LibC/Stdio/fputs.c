/** @file
    Implementation of fputs as declared in <stdio.h>.

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

    NetBSD: fputs.c,v 1.14 2005/06/22 19:45:22 christos Exp
    fputs.c 8.1 (Berkeley) 6/4/93
**/
#include  <LibConfig.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"
#include "fvwrite.h"

/*
 * Write the given string to the given file.
 */
int
fputs(const char *s, FILE *fp)
{
  struct __suio uio;
  struct __siov iov;
  int r;

  _DIAGASSERT(s != NULL);
  _DIAGASSERT(fp != NULL);
  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  if (s == NULL)
    s = "(null)";

  iov.iov_base = __UNCONST(s);
  uio.uio_resid = (int)(iov.iov_len = strlen(s));
  uio.uio_iov = &iov;
  uio.uio_iovcnt = 1;
  FLOCKFILE(fp);
  _SET_ORIENTATION(fp, -1);
  r = __sfvwrite(fp, &uio);
  FUNLOCKFILE(fp);
  return r;
}
