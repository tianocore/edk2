/** @file
  Implement the warning and error output messages.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1994 Michael L. Hitch
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael L. Hitch.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/
#include  <LibConfig.h>

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static void
_Vdomessage(int doerrno, const char *fmt, va_list args)
{
  fprintf(stderr, "%s: ", getprogname());
  if (fmt) {
    vfprintf(stderr, fmt, args);
    fprintf(stderr, ": ");
  }
  if (doerrno && errno < EMAXERRORVAL) {
    fprintf(stderr, "%s", strerror(errno));
  }
  fprintf(stderr, "\n");
}

void
err(int eval, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _Vdomessage(1, fmt, ap);
  va_end(ap);
  exit(eval);
}

void
errx(int eval, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _Vdomessage(0, fmt, ap);
  va_end(ap);
  exit(eval);
}

void
warn(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _Vdomessage(1, fmt, ap);
  va_end(ap);
}

void
warnx(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _Vdomessage(0, fmt, ap);
  va_end(ap);
}
