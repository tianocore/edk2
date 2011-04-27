/*  $NetBSD: rune.h,v 1.11 2006/02/16 19:19:49 tnozaki Exp $  */

/*-
 * Copyright (c) 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *
 *  @(#)rune.h  8.1 (Berkeley) 6/27/93
 */
#ifndef _RUNE_H_
#define _RUNE_H_

#include  <LibConfig.h>

#include <stdio.h>
#include <wchar.h>
#include "runetype.h"

/*
 * map _RTYPE_x to _CTYPE_x
 *
 * XXX: these should be defined in ctype.h and used in isxxx macros.
 *      (note: current isxxx macros use "old" NetBSD masks and
 *       _CTYPE_x are not public.)
 */
#define _CTYPE_A  _RUNETYPE_A
#define _CTYPE_C  _RUNETYPE_C
#define _CTYPE_D  _RUNETYPE_D
#define _CTYPE_G  _RUNETYPE_G
#define _CTYPE_L  _RUNETYPE_L
#define _CTYPE_P  _RUNETYPE_P
#define _CTYPE_S  _RUNETYPE_S
#define _CTYPE_U  _RUNETYPE_U
#define _CTYPE_X  _RUNETYPE_X
#define _CTYPE_B  _RUNETYPE_B
#define _CTYPE_R  _RUNETYPE_R
#define _CTYPE_I  _RUNETYPE_I
#define _CTYPE_T  _RUNETYPE_T
#define _CTYPE_Q  _RUNETYPE_Q
#define _CTYPE_SWM  _RUNETYPE_SWM
#define _CTYPE_SWS  _RUNETYPE_SWS
#define _CTYPE_SW0  _RUNETYPE_SW0
#define _CTYPE_SW1  _RUNETYPE_SW1
#define _CTYPE_SW2  _RUNETYPE_SW2
#define _CTYPE_SW3  _RUNETYPE_SW3

/*
 * Other namespace conversion.
 */

#define rune_t      __nbrune_t
#define _RUNE_ISCACHED    _NB_RUNE_ISCACHED
#define _CACHED_RUNES   _NB_CACHED_RUNES
#define _DEFAULT_INVALID_RUNE _NB_DEFAULT_INVALID_RUNE
#define _RuneEntry    _NBRuneEntry
#define _RuneRange    _NBRuneRange
#define _RuneLocale   _NBRuneLocale
#define _RUNE_MAGIC_1   _NB_RUNE_MAGIC_1
#define _RUNE_MODULE_1    _NB_RUNE_MODULE_1
#define _RUNE_CODESET   _NB_RUNE_CODESET

/*
 * global variables
 */
extern size_t __mb_len_max_runtime;
#define __MB_LEN_MAX_RUNTIME  __mb_len_max_runtime

extern _RuneLocale _DefaultRuneLocale;
extern _RuneLocale *_CurrentRuneLocale;
extern const char *_PathLocale;

#define _LOCALE_ALIAS_NAME  "locale.alias"

#endif  /*! _RUNE_H_ */
