/*  $NetBSD: ftello.c,v 1.4 2003/08/07 16:43:26 agc Exp $ */

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
__RCSID("$NetBSD: ftello.c,v 1.4 2003/08/07 16:43:26 agc Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "reentrant.h"
#include "local.h"

#ifdef __weak_alias
__weak_alias(ftello, _ftello)
#endif

/*
 * ftell: return current offset.
 */
off_t
ftello(FILE *fp)
{
  fpos_t pos;

  FLOCKFILE(fp);

  if (fp->_seek == NULL) {
    FUNLOCKFILE(fp);
    errno = ESPIPE;     /* historic practice */
    return ((off_t)-1);
  }

  /*
   * Find offset of underlying I/O object, then
   * adjust for buffered bytes.
   */
  __sflush(fp);   /* may adjust seek offset on append stream */
  if (fp->_flags & __SOFF)
    pos = fp->_offset;
  else {
    pos = (*fp->_seek)(fp->_cookie, (fpos_t)0, SEEK_CUR);
    if (pos == (fpos_t)-1) {
      FUNLOCKFILE(fp);
      return (pos);
    }
  }
  if (fp->_flags & __SRD) {
    /*
     * Reading.  Any unread characters (including
     * those from ungetc) cause the position to be
     * smaller than that in the underlying object.
     */
    pos -= fp->_r;
    if (HASUB(fp))
      pos -= fp->_ur;
  } else if (fp->_flags & __SWR && fp->_p != NULL) {
    /*
     * Writing.  Any buffered characters cause the
     * position to be greater than that in the
     * underlying object.
     */
    pos += fp->_p - fp->_bf._base;
  }
  FUNLOCKFILE(fp);
  return (pos);
}
