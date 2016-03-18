/** @file
  Case conversion function implementations for <ctype.h>

  The tolower function converts an uppercase letter to a corresponding
  lowercase letter.  If the argument is a character for which isupper
  is true and there are one or more corresponding characters, as
  specified by the current locale, for which islower is true, the tolower
  function returns one of the corresponding characters (always the same one
  for any given locale); otherwise, the argument is returned unchanged.

  The toupper function converts a lowercase letter to a corresponding
  uppercase letter.  If the argument is a character for which islower is true
  and there are one or more corresponding characters, as specified by the
  current locale, for which isupper is true, the toupper function returns one
  of the corresponding characters (always the same one for any given locale);
  otherwise, the argument is returned unchanged.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>

#define NO_CTYPE_MACROS            // So that we don't define the classification macros
#include  <ctype.h>

/** The tolower function converts an uppercase letter to a corresponding
    lowercase letter.

    @param[in]    c   The character to be converted.

    @return   If the argument is a character for which isupper is true and
              there are one or more corresponding characters, as specified by
              the current locale, for which islower is true, the tolower
              function returns one of the corresponding characters (always the
              same one for any given locale); otherwise, the argument is
              returned unchanged.
**/
int
tolower(
  IN  int _c
  )
{
  return (isupper(_c) ? _lConvT[_c] : _c);
}

/** The toupper function converts a lowercase letter to a corresponding
    uppercase letter.

    @param[in]    c   The character to be converted.

    @return   If the argument is a character for which islower is true and
              there are one or more corresponding characters, as specified by
              the current locale, for which isupper is true, the toupper
              function returns one of the corresponding characters (always the
              same one for any given locale); otherwise, the argument is
              returned unchanged.
**/
int
toupper(
  IN  int _c
  )
{
  return (islower(_c) ? _uConvT[_c] : _c);
}
