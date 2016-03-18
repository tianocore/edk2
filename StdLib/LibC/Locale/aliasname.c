/* $NetBSD: aliasname.c,v 1.2 2005/02/09 21:35:46 kleink Exp $ */

/*-
 * Copyright (c)2002 YAMAMOTO Takashi,
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
__RCSID("$NetBSD: aliasname.c,v 1.2 2005/02/09 21:35:46 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "aliasname_local.h"

__inline int __is_ws(char);

__inline int __is_ws(char ch)
{

  return (ch == ' ' || ch == '\t');
}

const char *
__unaliasname(const char *dbname, const char *alias, void *buf, size_t bufsize)
{
  FILE *fp = NULL;
  const char *result = alias;
  size_t resultlen;
  size_t aliaslen;
  const char *p;
  size_t len;

  _DIAGASSERT(dbname != NULL);
  _DIAGASSERT(alias != NULL);
  _DIAGASSERT(buf != NULL);

  fp = fopen(dbname, "r");
  if (fp == NULL)
    goto quit;

  aliaslen = strlen(alias);

  while (/*CONSTCOND*/ 1) {
    p = fgetln(fp, &len);
    if (p == NULL)
      goto quit; /* eof or error */

    _DIAGASSERT(len != 0);

    /* ignore terminating NL */
    if (p[len - 1] == '\n')
      len--;

    /* ignore null line and comment */
    if (len == 0 || p[0] == '#')
      continue;

    if (aliaslen > len)
      continue;

    if (memcmp(alias, p, aliaslen))
      continue;

    p += aliaslen;
    len -= aliaslen;

    if (len == 0 || !__is_ws(*p))
      continue;

    /* entry was found here */
    break;

    /* NOTREACHED */
  }

  /* skip white spaces */
  do {
    p++;
    len--;
  } while (len != 0 && __is_ws(*p));

  if (len == 0)
    goto quit;

  /* count length of result */
  resultlen = 0;
  while (resultlen < len && !__is_ws(*p))
    resultlen++;

  /* check if space is enough */
  if (bufsize < resultlen + 1)
    goto quit;

  memcpy(buf, p, resultlen);
  ((char *)buf)[resultlen] = 0;
  result = buf;

quit:
  if (fp)
    fclose(fp);

  return result;
}
