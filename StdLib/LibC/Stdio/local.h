/** @file
    Information local to this implementation of stdio,
    in particular, function declarations and macros.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

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

    NetBSD: local.h,v 1.20 2005/05/14 23:51:02 christos Exp
    local.h 8.3 (Berkeley) 7/3/94
**/

#include "wcio.h"
#include "fileext.h"

extern int      __sflush(FILE *);
extern FILE    *__sfp(void);
extern int      __srefill(FILE *);
extern int      __sread(void *, char *, int);
extern int      __swrite(void *, char const *, int);
extern fpos_t   __sseek(void *, fpos_t, int);
extern int      __sclose(void *);
extern void     __sinit(void);
extern void     _cleanup(void);
//extern void   (*__cleanup)(void);   // Now in MainData.h.  Ref. as gMD->cleanup
extern void     __smakebuf(FILE *);
extern int      __swhatbuf(FILE *, size_t *, int *);
extern int      _fwalk(int (*)(FILE *));
extern char    *_mktemp(char *);
extern int      __swsetup(FILE *);
extern int      __sflags(const char *, int *);
extern int      __svfscanf(FILE * __restrict, const char * __restrict, va_list)
                            __attribute__((__format__(__scanf__, 2, 0)));
extern int      __svfscanf_unlocked(FILE * __restrict, const char * __restrict, va_list)
                            __attribute__((__format__(__scanf__, 2, 0)));
extern int      __vfprintf_unlocked(FILE * __restrict, const char * __restrict, va_list);


extern int      __sdidinit;

extern int      __gettemp(char *, int *, int);

extern wint_t   __fgetwc_unlock(FILE *);
extern wint_t   __fputwc_unlock(wchar_t, FILE *);

extern char    *__fgetstr(FILE * __restrict, size_t * __restrict, int);
extern int      __slbexpand(FILE *, size_t);
extern int      __vfwprintf_unlocked(FILE *, const wchar_t *, va_list);
extern int      __vfwscanf_unlocked(FILE * __restrict, const wchar_t * __restrict, va_list);

/*
 * Return true iff the given FILE cannot be written now.
 */
#define cantwrite(fp) \
  ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && __swsetup(fp))

/*
 * Test whether the given stdio file has an active ungetc buffer;
 * release such a buffer, without restoring ordinary unread data.
 */
#define HASUB(fp) (_UB(fp)._base != NULL)
#define FREEUB(fp) { \
    if (_UB(fp)._base != (fp)->_ubuf) \
      free((char *)_UB(fp)._base); \
    _UB(fp)._base = NULL; \
  }

/*
 * test for an fgetln() buffer.
 */
#define HASLB(fp) ((fp)->_lb._base != NULL)
#define FREELB(fp) { \
    free((char *)(fp)->_lb._base); \
    (fp)->_lb._base = NULL; \
  }

extern void __flockfile_internal  (FILE *, int);
extern void __funlockfile_internal(FILE *, int);
