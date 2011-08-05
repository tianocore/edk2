/** @file
  Localization functions and macros.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1991, 1993
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
 *
 *  @(#)locale.h  8.1 (Berkeley) 6/2/93
 *  $NetBSD: locale.h,v 1.14 2005/02/03 04:39:32 perry Exp
**/

#ifndef _LOCALE_H_
#define _LOCALE_H_

/** This is a structure containing members pertaining to the formatting of numeric values.
    There is no requirement for members of this structure to be in any particular order.
**/
struct lconv {
  char  *decimal_point;
  char  *thousands_sep;
  char  *grouping;
  char  *int_curr_symbol;
  char  *currency_symbol;
  char  *mon_decimal_point;
  char  *mon_thousands_sep;
  char  *mon_grouping;
  char  *positive_sign;
  char  *negative_sign;
  char  int_frac_digits;
  char  frac_digits;
  char  p_cs_precedes;
  char  p_sep_by_space;
  char  n_cs_precedes;
  char  n_sep_by_space;
  char  p_sign_posn;
  char  n_sign_posn;
  char  int_p_cs_precedes;
  char  int_n_cs_precedes;
  char  int_p_sep_by_space;
  char  int_n_sep_by_space;
  char  int_p_sign_posn;
  char  int_n_sign_posn;
};

/** These macros expand to integer expressions suitable for use as the first
    argument to the setlocale() function.

    Only the first six macros are required by the C language specification.
    Implementations are free to extend this list, as has been done with LC_MESSAGES,
    with additional macro definitions, beginning with the characters LC_ and
    an uppercase letter.
@{
**/
#define LC_ALL      0   ///< The application's entire locale.
#define LC_COLLATE  1   ///< Affects the behavior of the strcoll and strxfrm functions.
#define LC_CTYPE    2   ///< Affects the behavior of the character handling, multibyte, and wide character functions.
#define LC_MONETARY 3   ///< Affects monetary formatting information.
#define LC_NUMERIC  4   ///< Affects the decimal-point character and non-monetary formatting information.
#define LC_TIME     5   ///< Affects the behavior of the strftime and wcsftime functions.
#define LC_MESSAGES 6
#define _LC_LAST    7   ///< Number of defined macros. Marks end.
/// @}

#include  <sys/EfiCdefs.h>

/** @fn   char *setlocale(int, const char *)
**/

/** @fn   struct lconv *localeconv(void)
**/

__BEGIN_DECLS
#ifdef __SETLOCALE_SOURCE__
  char    *setlocale(int, const char *);
  char    *__setlocale(int, const char *);
#else /* !__SETLOCALE_SOURCE__ */
  char    *setlocale(int, const char *) __RENAME(__setlocale_mb_len_max_32);
#endif /* !__SETLOCALE_SOURCE__ */
struct lconv  *localeconv(void);
  char    *__setlocale_mb_len_max_32(int, const char *);
__END_DECLS

#endif /* _LOCALE_H_ */
