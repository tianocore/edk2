/*  $NetBSD: wctype.h,v 1.6 2005/02/03 04:39:32 perry Exp $ */

/*-
 * Copyright (c)1999 Citrus Project,
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
 *
 *  citrus Id: wctype.h,v 1.4 2000/12/21 01:50:21 itojun Exp
 */

#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#include  <sys/EfiCdefs.h>
#include  <machine/ansi.h>

#ifdef  _BSD_WINT_T_
typedef _BSD_WINT_T_    wint_t;
#undef  _BSD_WINT_T_
#endif

#ifdef  _BSD_WCTRANS_T_
typedef wint_t (*wctrans_t)(wint_t);
#undef  _BSD_WCTRANS_T_
#endif

#ifdef  _BSD_WCTYPE_T_
typedef _BSD_WCTYPE_T_  wctype_t;
#undef  _BSD_WCTYPE_T_
#endif

#ifndef WEOF
#define WEOF  ((wint_t)-1)
#endif

__BEGIN_DECLS
int       /*EFIAPI*/ iswalnum(wint_t);
int       /*EFIAPI*/ iswalpha(wint_t);
int       /*EFIAPI*/ iswcntrl(wint_t);
int       /*EFIAPI*/ iswctype(wint_t, wctype_t);
int       /*EFIAPI*/ iswdigit(wint_t);
int       /*EFIAPI*/ iswgraph(wint_t);
int       /*EFIAPI*/ iswlower(wint_t);
int       /*EFIAPI*/ iswprint(wint_t);
int       /*EFIAPI*/ iswpunct(wint_t);
int       /*EFIAPI*/ iswblank(wint_t);
int       /*EFIAPI*/ iswspace(wint_t);
int       /*EFIAPI*/ iswupper(wint_t);
int       /*EFIAPI*/ iswxdigit(wint_t);
wint_t    /*EFIAPI*/ towctrans(wint_t, wctrans_t);
wint_t    /*EFIAPI*/ towlower(wint_t);
wint_t    /*EFIAPI*/ towupper(wint_t);
wctrans_t /*EFIAPI*/ wctrans(const char *);
wctype_t  /*EFIAPI*/ wctype(const char *);
__END_DECLS

#endif    /* _WCTYPE_H_ */
