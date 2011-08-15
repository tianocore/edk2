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

/** This is a structure containing members pertaining to the formatting and display of numeric values.
    Each member of this structure is commented with its value in the "C" locale.

    The decimal_point member must point to a string with a length greater than zero.
    All other pointer members may point to "" in order to indicate that the value is not available
    in the current locale, or that it is of zero length.  Except for grouping and mon_grouping, the
    strings start and end in the initial shift state.

    The remaining members, of type char, are non-negative numbers or CHAR_MAX, which indicates that
    the value is not available in the current locale.

    Members grouping and mon_grouping point to strings where each element (character) of the string
    indicates the size of the corresponding group of digits and is interpreted as follows:
      - CHAR_MAX  No further grouping is to be performed.
      - 0         The previous element is to be repeatedly used for the remainder of the digits.
      - other     The ISO specification states: "The integer value is the number of digits that
                  compose the current group.  The next element is examined to determine the size
                  of the next group of digits before the current group."  The EDK II implementation
                  interprets this to mean that the groups are specified left-to-right.

    The *_sep_by_space members are interpreted as follows:
      - 0   No space separates the currency symbol and value.
      - 1   If the currency symbol and sign string are adjacent, a space separates them from the
            value; otherwise, a space separates the currency symbol from the value.
      - 2   If the currency symbol and sign string are adjacent, a space separates them;
            otherwise, a space separates the sign string from the value.
    For int_p_sep_by_space and int_n_sep_by_space, the fourth character of int_curr_symbol is
    used instead of a space.

    The values of the *_sign_posn members are interpreted as follows:
      - 0   Parentheses surround the quantity and currency symbol.
      - 1   The sign string precedes the quantity and currency symbol.
      - 2   The sign string succeeds the quantity and currency symbol.
      - 3   The sign string immediately precedes the currency symbol.
      - 4   The sign string immediately succeeds the currency symbol.
**/
struct lconv {
  char  *decimal_point;           /**< "."        Non-monetary decimal-point. */
  char  *thousands_sep;           /**< ""         Separates groups of digits before the decimal-point */
  char  *grouping;                /**< ""         A string whose elements (characters) indicate the size
                                                  of each group of digits in formatted nonmonetary quantities. */
  char  *int_curr_symbol;         /**< ""         A 4-character string providing the international currency
                                                  symbol.  The first three characters contain the alphabetic
                                                  international currency symbol in accordance with those
                                                  specified in ISO 4217.  The fourth character, immediately
                                                  preceding the null character, is the character used to separate
                                                  the international currency symbol from the monetary quantity. */
  char  *currency_symbol;         /**< ""         The local currency symbol for the current locale. */
  char  *mon_decimal_point;       /**< ""         The decimal point used for monetary values. */
  char  *mon_thousands_sep;       /**< ""         The separator for digit groups preceeding the decimal-point. */
  char  *mon_grouping;            /**< ""         A string, like grouping, for monetary values. */
  char  *positive_sign;           /**< ""         A string to indicate a non-negative monetary value. */
  char  *negative_sign;           /**< ""         A string to indicate a negative monetary value. */
  char  int_frac_digits;          /**< CHAR_MAX   The number of digits after the decimal-point for international
                                                  monetary values. */
  char  frac_digits;              /**< CHAR_MAX   The number of digits after the decimal-point for local
                                                  monetary values. */
  char  p_cs_precedes;            /**< CHAR_MAX   Set to 1 or 0 if the currency_symbol respectively precedes or
                                                  succeeds the value for non-negative local monetary values. */
  char  p_sep_by_space;           /**< CHAR_MAX   Value specifying the separation between the currency_symbol,
                                                  the sign string, and the value for non-negative local values. */
  char  n_cs_precedes;            /**< CHAR_MAX   Set to 1 or 0 if the currency_symbol respectively precedes or
                                                  succeeds the value for negative local monetary values. */
  char  n_sep_by_space;           /**< CHAR_MAX   Value specifying the separation between the currency_symbol,
                                                  the sign string, and the value for negative local values. */
  char  p_sign_posn;              /**< CHAR_MAX   Value specifying the positioning of the positive_sign for a
                                                  non-negative local monetary quantity. */
  char  n_sign_posn;              /**< CHAR_MAX   Value specifying the positioning of the negative_sign for a
                                                  negative local monetary quantity. */
  char  int_p_cs_precedes;        /**< CHAR_MAX   Set to 1 or 0 if the currency_symbol respectively precedes or
                                                  succeeds the value for non-negative international monetary values. */
  char  int_n_cs_precedes;        /**< CHAR_MAX   Set to 1 or 0 if the currency_symbol respectively precedes or
                                                  succeeds the value for negative international monetary values. */
  char  int_p_sep_by_space;       /**< CHAR_MAX   Value specifying the separation between the currency_symbol,
                                                  the sign string, and the value for non-negative international values. */
  char  int_n_sep_by_space;       /**< CHAR_MAX   Value specifying the separation between the currency_symbol,
                                                  the sign string, and the value for negative international values. */
  char  int_p_sign_posn;          /**< CHAR_MAX   Value specifying the positioning of the positive_sign for a
                                                  non-negative international monetary quantity. */
  char  int_n_sign_posn;          /**< CHAR_MAX   Value specifying the positioning of the negative_sign for a
                                                  negative international monetary quantity. */
};

/** @{
    These macros expand to integer expressions suitable for use as the first
    argument (category) to the setlocale() function.

    Only the first six macros are required by the C language specification.
    Implementations are free to extend this list, as has been done with LC_MESSAGES,
    with additional macro definitions, beginning with the characters LC_ and
    an uppercase letter.
**/
#define LC_ALL      0   ///< The application's entire locale.
#define LC_COLLATE  1   ///< Affects the behavior of the strcoll and strxfrm functions.
#define LC_CTYPE    2   ///< Affects the behavior of the character handling, multibyte, and wide character functions.
#define LC_MONETARY 3   ///< Affects monetary formatting information.
#define LC_NUMERIC  4   ///< Affects the decimal-point character and non-monetary formatting information.
#define LC_TIME     5   ///< Affects the behavior of the strftime and wcsftime functions.
#define LC_MESSAGES 6
#define _LC_LAST    7   ///< Number of defined macros. Marks end.
/*@}*/

#include  <sys/EfiCdefs.h>

/** @fn   char *setlocale(int category, const char *locale)

    @brief    The setlocale function is used to retrieve or change parts or all of the current locale.

    @details  If locale is NULL, or the same as the current locale, this function just retrieves the
              values for the specified category in the current locale.  Otherwise, the specified category
              in the current locale is set to the corresponding values from the specified locale and a pointer
              to the new values is returned.

    @param[in]    category    The portion of the current locale to be affected by this call.
                              The LC_ macros list the supported categories and the meaning of each.
    @param[in]    locale      A value of "C" for locale specifies the minimal environment for C translation;
                              A value of "" specifies the native environment, which is "C" for this
                              implementation.  If locale is NULL, the current locale is specified.

    @return     A pointer to the string associated with the specified category for the new locale,
                a pointer to the string associated with the category for the current locale,
                or NULL if category or locale can not be honored.  The return value should not be
                modified by the program, but may be overwritten by subsequent calls to either
                setlocale or localeconv.
**/

/** @fn   struct lconv *localeconv(void)

    @brief    The localeconv function returns a pointer to a lconv structure containing the appropriate
              values for the current locale.

    @return   A pointer to a filled-in lconv structure.  The returned structure should not be
              modified by the program, but may be overwritten by subsequent calls to either
              setlocale or localeconv.
**/

__BEGIN_DECLS
#ifdef __SETLOCALE_SOURCE__
  char    *setlocale(int category, const char *locale);
  char    *__setlocale(int category, const char *locale);
#else /* !__SETLOCALE_SOURCE__ */
  char    *setlocale(int category, const char *locale) __RENAME(__setlocale_mb_len_max_32);
#endif /* !__SETLOCALE_SOURCE__ */
struct lconv  *localeconv(void);
  char    *__setlocale_mb_len_max_32(int category, const char *locale);
__END_DECLS

#endif /* _LOCALE_H_ */
