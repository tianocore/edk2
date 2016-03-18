/*  $NetBSD: wcsxfrm.c,v 1.2 2006/10/15 16:14:08 christos Exp $ */

/*-
 * Copyright (c)2003 Citrus Project,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: wcsxfrm.c,v 1.2 2006/10/15 16:14:08 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <wchar.h>

/*
 * Compare strings with using collating information.
 */
size_t
wcsxfrm(
  wchar_t *s1,
  const wchar_t *s2,
  size_t n
  )
{
  size_t len;

  /* XXX: LC_COLLATE should be implemented. */

  len = wcslen(s2);
  if (len<n)
    wcscpy(s1, s2);
  else {
    /*
     * SUSv3 says:
     *   If the value returned is n or more, the contents
     *   of the array pointed to by ws1 are unspecified.
     */
    /* thus, do nothing */
  }

  return (len);
}
