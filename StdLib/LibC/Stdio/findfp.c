/** @file

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

    NetBSD: findfp.c,v 1.23 2006/10/07 21:40:46 thorpej Exp
    findfp.c  8.2 (Berkeley) 1/4/94
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  "namespace.h"
#include  <sys/param.h>
#include  <stdio.h>
#include  <errno.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>
#include  "reentrant.h"
#include  "local.h"
#include  "glue.h"
#include  <MainData.h>

int __sdidinit;

#define NDYNAMIC 10   /* add ten more whenever necessary */

#define std(flags, file) \
/*    p     r  w  flags  file  bf     lfbsize  cookie       close */ \
  { NULL, 0, 0, flags, file, { NULL, 0 }, 0, __sF + file, __sclose, \
/*    read      seek     write     ext                              up */ \
    __sread,  __sseek, __swrite, { (void *)(__sFext + file), 0 }, NULL, \
/*    ur ubuf,                 nbuf      lb     blksize  offset */ \
    0, { '\0', '\0', '\0' }, { '\0' }, { NULL, 0 }, 0, (fpos_t)0 }

        /* the usual - (stdin + stdout + stderr) */
static FILE usual[FOPEN_MAX - 3];
static struct __sfileext usualext[FOPEN_MAX - 3];
static struct glue uglue = { 0, FOPEN_MAX - 3, usual };

#if defined(_REENTRANT) && !defined(__lint__) /* XXX lint is busted */
#define STDEXT { ._lock = MUTEX_INITIALIZER, ._lockcond = COND_INITIALIZER }
struct __sfileext __sFext[3] = { STDEXT,
         STDEXT,
         STDEXT};
#else
struct __sfileext __sFext[3];
#endif

FILE __sF[3] = {
  std(__SRD, STDIN_FILENO),   /* stdin */
  std(__SWR, STDOUT_FILENO),    /* stdout */
  std(__SWR|__SNBF, STDERR_FILENO)  /* stderr */
};
struct glue __sglue = { &uglue, 3, __sF };

static struct glue *moreglue(int);
void f_prealloc(void);

#ifdef _REENTRANT
rwlock_t __sfp_lock = RWLOCK_INITIALIZER;
#endif

static struct glue *
moreglue(int n)
{
  struct glue *g;
  FILE *p;
  struct __sfileext *pext;
  static FILE empty;

  g = (struct glue *)malloc(sizeof(*g) + ALIGNBYTES + n * sizeof(FILE)
    + n * sizeof(struct __sfileext));
  if (g == NULL)
    return (NULL);
  p = (FILE *)ALIGN((g + 1));
  g->next = NULL;
  g->niobs = n;
  g->iobs = p;
  pext = (void *)(p + n);
  while (--n >= 0) {
    *p = empty;
    _FILEEXT_SETUP(p, pext);
    p++;
    pext++;
  }
  return (g);
}

/*
 * Find a free FILE for fopen et al.
 */
FILE *
__sfp()
{
  FILE *fp;
  int n;
  struct glue *g;

  if (!__sdidinit)
    __sinit();

  rwlock_wrlock(&__sfp_lock);
  for (g = &__sglue;; g = g->next) {
    for (fp = g->iobs, n = g->niobs; --n >= 0; fp++)
      if (fp->_flags == 0)
        goto found;
    if (g->next == NULL && (g->next = moreglue(NDYNAMIC)) == NULL)
      break;
  }
  rwlock_unlock(&__sfp_lock);
  return (NULL);
found:
  fp->_flags = 1;   /* reserve this slot; caller sets real flags */
  fp->_p = NULL;    /* no current pointer */
  fp->_w = 0;   /* nothing to read or write */
  fp->_r = 0;
  fp->_bf._base = NULL; /* no buffer */
  fp->_bf._size = 0;
  fp->_lbfsize = 0; /* not line buffered */
  fp->_file = -1;   /* no file */
/*  fp->_cookie = <any>; */ /* caller sets cookie, _read/_write etc */
  _UB(fp)._base = NULL; /* no ungetc buffer */
  _UB(fp)._size = 0;
  fp->_lb._base = NULL; /* no line buffer */
  fp->_lb._size = 0;
  memset(WCIO_GET(fp), 0, sizeof(struct wchar_io_data));
  rwlock_unlock(&__sfp_lock);
  return (fp);
}

#if 0
/*
 * XXX.  Force immediate allocation of internal memory.  Not used by stdio,
 * but documented historically for certain applications.  Bad applications.
 */
void
f_prealloc()
{
  struct glue *g;
  int n;

  n = (int)sysconf(_SC_OPEN_MAX) - FOPEN_MAX + 20; /* 20 for slop. */
  for (g = &__sglue; (n -= g->niobs) > 0 && g->next; g = g->next)
    /* void */;
  if (n > 0)
    g->next = moreglue(n);
}
#endif

/*
 * exit() calls _cleanup() through *gMD->cleanup, set whenever we
 * open or buffer a file.  This chicanery is done so that programs
 * that do not use stdio need not link it all in.
 *
 * The name `_cleanup' is, alas, fairly well known outside stdio.
 */
void
_cleanup( void )
{
  /* (void) _fwalk(fclose); */
  (void) fflush(NULL);      /* `cheating' */
}

/*
 * __sinit() is called whenever stdio's internal variables must be set up.
 */
void
__sinit( void )
{
  int i;

  for (i = 0; i < FOPEN_MAX - 3; i++)
    _FILEEXT_SETUP(&usual[i], &usualext[i]);

  /* make sure we clean up on exit */
  gMD->cleanup = _cleanup;   /* conservative */
  __sdidinit = 1;
}
