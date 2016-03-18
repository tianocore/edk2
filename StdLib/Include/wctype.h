/** @file
    Wide character classification and mapping utilities.

    The following macros are defined in this file:<BR>
@verbatim
      WEOF        Wide char version of end-of-file.
@endverbatim

    The following types are defined in this file:<BR>
@verbatim
      wint_t      Type capable of holding all wchar_t values and WEOF.
      wctrans_t   A type for holding locale-specific character mappings.
      wctype_t    Type for holding locale-specific character classifications.
@endverbatim

    The following functions are declared in this file:<BR>
@verbatim
      ###############  Wide Character Classification Functions
      int           iswalnum  (wint_t);
      int           iswalpha  (wint_t);
      int           iswcntrl  (wint_t);
      int           iswdigit  (wint_t);
      int           iswgraph  (wint_t);
      int           iswlower  (wint_t);
      int           iswprint  (wint_t);
      int           iswpunct  (wint_t);
      int           iswblank  (wint_t);
      int           iswspace  (wint_t);
      int           iswupper  (wint_t);
      int           iswxdigit (wint_t);

      ###############  Extensible Wide Character Classification Functions
      wctype_t      wctype    (const char *);
      int           iswctype  (wint_t, wctype_t);

      ###############  Wide Character Case Mapping Utilities
      wint_t        towlower  (wint_t);
      wint_t        towupper  (wint_t);

      ###############  Extensible Wide Character Case Mapping Utilities
      wctrans_t     wctrans   (const char *);
      wint_t        towctrans (wint_t, wctrans_t);
@endverbatim

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

    NetBSD: wctype.h,v 1.6 2005/02/03 04:39:32 perry Exp
**/
#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#include  <sys/EfiCdefs.h>
#include  <machine/ansi.h>

#ifdef _EFI_WINT_T
  /** wint_t is an integer type unchanged by default argument promotions that can
      hold any value corresponding to members of the extended character set, as
      well as at least one value that does not correspond to any member of the
      extended character set: WEOF.
  */
  typedef _EFI_WINT_T  wint_t;
  #undef _BSD_WINT_T_
  #undef _EFI_WINT_T
#endif

#ifdef  _BSD_WCTRANS_T_
  /** A scalar type for holding locale-specific character mappings. */
  typedef wint_t (*wctrans_t)(wint_t);
  #undef  _BSD_WCTRANS_T_
#endif

#ifdef  _BSD_WCTYPE_T_
  /** A scalar type capable of holding values representing locale-specific
      character classifications. */
  typedef _BSD_WCTYPE_T_  wctype_t;
  #undef  _BSD_WCTYPE_T_
#endif

#ifndef WEOF
  /** WEOF expands to a constant expression of type wint_t whose value does not
      correspond to any member of the extended character set. It is accepted
      (and returned) by several functions, declared in this file, to indicate
      end-of-file, that is, no more input from a stream. It is also used as a
      wide character value that does not correspond to any member of the
      extended character set.
  */
  #define WEOF  ((wint_t)-1)
#endif

__BEGIN_DECLS
  /** Test for any wide character for which iswalpha or iswdigit is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswalnum  (wint_t WC);

  /** Test for any wide character for which iswupper or iswlower is TRUE,
      OR, a locale-specific character where none of iswcntrl, iswdigit,
      iswpunct, or iswspace is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswalpha  (wint_t WC);

  /** Test for any wide control character.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswcntrl  (wint_t WC);

  /** Test if the value of WC is a wide character that corresponds to a decimal digit.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswdigit  (wint_t WC);

  /** Test for wide characters for which iswprint is TRUE and iswspace is FALSE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswgraph  (wint_t WC);

  /** The iswlower function tests for any wide character that corresponds to a
      lowercase letter or is one of a locale-specific set of wide characters
      for which none of iswcntrl, iswdigit, iswpunct, or iswspace is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswlower  (wint_t WC);

  /** Test for any printing wide character.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswprint  (wint_t WC);

  /** The iswpunct function tests for any printing wide character that is one
      of a locale-specific set of punctuation wide characters for which
      neither iswspace nor iswalnum is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswpunct  (wint_t WC);

  /** Test for standard blank characters or locale-specific characters
      for which iswspace is TRUE and are used to separate words within a line
      of text.  In the "C" locale, iswblank only returns TRUE for the standard
      blank characters space (L' ') and horizontal tab (L'\t').

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswblank  (wint_t WC);

  /** The iswspace function tests for any wide character that corresponds to a
      locale-specific set of white-space wide characters for which none of
      iswalnum, iswgraph, or iswpunct is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswspace  (wint_t WC);

  /** Tests for any wide character that corresponds to an uppercase letter or
      is one of a locale-specific set of wide characters for which none of
      iswcntrl, iswdigit, iswpunct, or iswspace is TRUE.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswupper  (wint_t WC);

  /** The iswxdigit function tests for any wide character that corresponds to a
      hexadecimal-digit character.

    @param[in]  WC    The wide character to be classified.

    @return   Returns non-zero (TRUE) if and only if the value of WC conforms
              to the classification described for this function.
  */
  int           iswxdigit (wint_t WC);

  /** Construct a value that describes a class of wide characters, identified
      by the string pointed to by Desc.  The constructed value is suitable for
      use as the second argument to the iswctype function.

      The following strings name classes of wide characters that the iswctype
      function is able to test against.  These strings are valid in all locales
      as Desc arguments to wctype().
        - "alnum"
        - "alpha"
        - "blank"
        - "cntrl"
        - "digit"
        - "graph"
        - "lower"
        - "print"
        - "punct"
        - "space"
        - "upper"
        - "xdigit

    @param[in]  Desc    A pointer to a multibyte character string naming a
                        class of wide characters.

    @return   If Desc identifies a valid class of wide characters in the
              current locale, the wctype function returns a nonzero value that
              is valid as the second argument to the iswctype function;
              otherwise, it returns zero.
  */
  wctype_t      wctype    (const char *Desc);

  /** Determine whether the wide character WC has the property described by Wct.

    @param[in]  WC      The wide character to be classified.
    @param[in]  Wct     A value describing a class of wide characters.

    @return   The iswctype function returns nonzero (TRUE) if and only if the
              value of the wide character WC has the property described by Wct.
  */
  int           iswctype  (wint_t WC, wctype_t Wct);

  /** Convert an uppercase letter to a corresponding lowercase letter.

    @param[in]  WC    The wide character to be converted.

    @return   If the argument is a wide character for which iswupper is TRUE
              and there are one or more corresponding wide characters, as
              specified by the current locale, for which iswlower is TRUE, the
              towlower function returns one of the corresponding wide
              characters (always the same one for any given locale); otherwise,
              the argument is returned unchanged.
  */
  wint_t        towlower  (wint_t WC);

  /** Convert a lowercase letter to a corresponding uppercase letter.

    @param[in]  WC    The wide character to be converted.

    @return   If the argument is a wide character for which iswlower is TRUE
              and there are one or more corresponding wide characters, as
              specified by the current locale, for which iswupper is TRUE, the
              towupper function returns one of the corresponding wide
              characters (always the same one for any given locale); otherwise,
              the argument is returned unchanged.
  */
  wint_t        towupper  (wint_t WC);

  /** Construct a value that describes a mapping between wide characters
      identified by the string argument, S.

      The strings listed below are valid in all locales as the S argument to
      the wctrans function.
        - "tolower"
        - "toupper"

    @param[in]  S   A pointer to a multibyte character string naming a
                    mapping between wide characters.

    @return   If S identifies a valid mapping of wide characters in the current
              locale, the wctrans function returns a nonzero value that is
              valid as the second argument to the towctrans function;
              otherwise, it returns zero.
  */
  wctrans_t     wctrans   (const char *S);

  /** Map the wide character WC using the mapping described by WTr. The current
      locale will be the same as during the call to wctrans that returned
      the value WTr.

    @param[in]  WC    The wide character to be converted.
    @param[in]  WTr   A value describing a mapping of wide characters in the
                      current locale.

    @return   Returns the mapped value of WC using the mapping selected by WTr.
  */
  wint_t        towctrans (wint_t WC, wctrans_t WTr);
__END_DECLS

#endif    /* _WCTYPE_H_ */
