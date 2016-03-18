/*  $NetBSD: snprintf_ss.c,v 1.3.2.1 2007/05/07 19:49:08 pavel Exp $  */

/*-
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
 */
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)snprintf.c  8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: snprintf_ss.c,v 1.3.2.1 2007/05/07 19:49:08 pavel Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "reentrant.h"
#include "extern.h"
#include "local.h"

#ifdef __weak_alias
__weak_alias(snprintf_ss,_snprintf_ss)
#endif

int
snprintf_ss(char *str, size_t n, char const *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vsnprintf_ss(str, n, fmt, ap);
  va_end(ap);
  return ret;
}
