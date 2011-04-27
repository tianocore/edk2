/** @file
  Case conversion functions for <ctype.h>

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

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <LibConfig.h>

#define NO_CTYPE_MACROS            // So that we don't define the classification macros
#include  <ctype.h>

int
tolower(
  int _c
  )
{
//  return ((_c < 0 || _c > 127) ? _c : _lConvT[_c]);
  return (isupper(_c) ? _lConvT[_c] : _c);
}

int toupper(
  int _c
  )
{
//  return ((_c < 0 || _c > 127) ? _c : _uConvT[_c]);
  return (islower(_c) ? _uConvT[_c] : _c);
}
