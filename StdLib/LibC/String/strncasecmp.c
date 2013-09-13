/** @file
  strncasecmp implementation

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1987, 1993
 *  The Regents of the University of California.  All rights reserved.
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

  $NetBSD: strncasecmp.c,v 1.2 2007/06/04 18:19:27 christos Exp $
  strcasecmp.c  8.1 (Berkeley) 6/4/93
**/
#include <LibConfig.h>
#include <sys/cdefs.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include "namespace.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef __weak_alias
__weak_alias(strcasecmp,_strcasecmp)
__weak_alias(strncasecmp,_strncasecmp)
#endif
#else
#include <lib/libkern/libkern.h>
#include <machine/limits.h>
#endif

int
strncasecmp(const char *s1, const char *s2, size_t n)
{
  int   CompareVal;

  _DIAGASSERT(s1 != NULL);
  _DIAGASSERT(s2 != NULL);

  if (n != 0) {
    do {
      CompareVal = tolower(*s1) - tolower(*s2);
      if (CompareVal != 0) {
        return (CompareVal);
      }
      ++s1;
      ++s2;
      if (*s1 == '\0') {
        break;
      }
    } while (--n != 0);
  }
  return (0);
}
