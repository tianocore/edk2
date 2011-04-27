/*  $NetBSD: flockfile.c,v 1.8 2003/07/22 00:56:25 nathanw Exp $  */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flockfile.c,v 1.8 2003/07/22 00:56:25 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

#ifdef __weak_alias
__weak_alias(flockfile,_flockfile)
__weak_alias(ftrylockfile,_ftrylockfile)
__weak_alias(funlockfile,_funlockfile)
#endif

#ifdef _REENTRANT
/*
 * XXX This code makes the assumption that a thr_t (pthread_t) is a
 * XXX pointer.
 */

extern int __isthreaded;

void
flockfile(FILE *fp)
{

  __flockfile_internal(fp, 0);
}

int
ftrylockfile(FILE *fp)
{
  int retval;

  if (__isthreaded == 0)
    return 0;

  retval = 0;
  mutex_lock(&_LOCK(fp));

  if (_LOCKOWNER(fp) == thr_self()) {
    _LOCKCOUNT(fp)++;
  } else if (_LOCKOWNER(fp) == NULL) {
    _LOCKOWNER(fp) = thr_self();
    _LOCKCOUNT(fp) = 1;
  } else
    retval = -1;

  mutex_unlock(&_LOCK(fp));

  return retval;
}

void
funlockfile(FILE *fp)
{

  __funlockfile_internal(fp, 0);
}

void
__flockfile_internal(FILE *fp, int internal)
{

  if (__isthreaded == 0)
    return;

  mutex_lock(&_LOCK(fp));

  if (_LOCKOWNER(fp) == thr_self()) {
    _LOCKCOUNT(fp)++;
    if (internal)
      _LOCKINTERNAL(fp)++;
  } else {
    /* danger! cond_wait() is a cancellation point. */
    int oldstate;
    thr_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    while (_LOCKOWNER(fp) != NULL)
      cond_wait(&_LOCKCOND(fp), &_LOCK(fp));
    thr_setcancelstate(oldstate, NULL);
    _LOCKOWNER(fp) = thr_self();
    _LOCKCOUNT(fp) = 1;
    if (internal)
      _LOCKINTERNAL(fp) = 1;
  }

  if (_LOCKINTERNAL(fp) == 1)
    /* stash cancellation state and disable */
    thr_setcancelstate(PTHREAD_CANCEL_DISABLE,
        &_LOCKCANCELSTATE(fp));

  mutex_unlock(&_LOCK(fp));
}

void
__funlockfile_internal(FILE *fp, int internal)
{

  if (__isthreaded == 0)
    return;

  mutex_lock(&_LOCK(fp));

  if (internal) {
    _LOCKINTERNAL(fp)--;
    if (_LOCKINTERNAL(fp) == 0)
      thr_setcancelstate(_LOCKCANCELSTATE(fp), NULL);
  }

  _LOCKCOUNT(fp)--;
  if (_LOCKCOUNT(fp) == 0) {
    _LOCKOWNER(fp) = NULL;
    cond_signal(&_LOCKCOND(fp));
  }

  mutex_unlock(&_LOCK(fp));
}

#else /* _REENTRANT */

void
flockfile(FILE *fp)
{
  /* LINTED deliberate lack of effect */
  (void)fp;

  return;
}

int
ftrylockfile(FILE *fp)
{
  /* LINTED deliberate lack of effect */
  (void)fp;

  return (0);
}

void
funlockfile(FILE *fp)
{
  /* LINTED deliberate lack of effect */
  (void)fp;

  return;
}

#endif /* _REENTRANT */
