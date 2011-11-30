/** @file
  Character classification function implementations for <ctype.h>.

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

/** Internal worker function for character classification.

    Determines if a character is a member of a set of character classes.

    @param[in]    _c      The character to be tested.
    @param[in]    mask    A bitmapped specification of the character classes to
                          test the character against.  These bits are defined
                          in _ctype.h.

    @retval   0         The character, _c, is NOT a member of the character classes specified by mask.
    @retval   nonZero   The character, _c, IS a member of a specified character class.
**/
int
__isCClass(
  IN  int _c,
  unsigned int mask
  )
{
  return ((_c < 0 || _c > 127) ? 0 : (_cClass[_c] & mask));
}

/** The isalnum function tests for any character for which isalpha or isdigit
    is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isalnum(
  IN  int c
  )
{
  return (__isCClass( c, (_CD | _CU | _CL | _XA)));
}

/** The isalpha function tests for any character for which isupper or islower
    is true, or any character that is one of a locale-specific set of
    alphabetic characters for which none of iscntrl, isdigit, ispunct, or
    isspace is true. In the "C" locale, isalpha returns true only for the
    characters for which isupper or islower is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isalpha(
  IN  int c
  )
{
  return (__isCClass( c, (_CU | _CL | _XA)));
}

/** The iscntrl function tests for any control character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
iscntrl(
  IN  int c
  )
{
  return (__isCClass( c, (_CC)));
}

/** The isdigit function tests for any decimal-digit character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isdigit(
  IN  int c
  )
{
  return (__isCClass( c, (_CD)));
}

/** The isgraph function tests for any printing character except space (' ').

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isgraph(
  IN  int c
  )
{
  return (__isCClass( c, (_CG)));
}

/** The islower function tests for any character that is a lowercase letter or
    is one of a locale-specific set of characters for which none of iscntrl,
    isdigit, ispunct, or isspace is true.  In the "C" locale, islower returns
    true only for the lowercase letters.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
islower(
  IN  int c
  )
{
  return (__isCClass( c, (_CL)));
}

/** The isprint function tests for any printing character including space (' ').

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isprint(
  IN  int c
  )
{
  return (__isCClass( c, (_CS | _CG)));
}

/** The ispunct function tests for any printing character that is one of a
    locale-specific set of punctuation characters for which neither isspace nor
    isalnum is true. In the "C" locale, ispunct returns true for every printing
    character for which neither isspace nor isalnum is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
ispunct(
  IN  int c
  )
{
  return (__isCClass( c, (_CP)));
}

/** The isspace function tests for any character that is a standard white-space
    character or is one of a locale-specific set of characters for which
    isalnum is false. The standard white-space characters are the following:
    space (' '), form feed ('\f'), new-line ('\n'), carriage return ('\r'),
    horizontal tab ('\t'), and vertical tab ('\v'). In the "C" locale, isspace
    returns true only for the standard white-space characters.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isspace(
  IN  int c
  )
{
  return (__isCClass( c, (_CW)));
}

/** The isupper function tests for any character that is an uppercase letter or
    is one of a locale-specific set of characters for which none of iscntrl,
    isdigit, ispunct, or isspace is true. In the "C" locale, isupper returns
    true only for the uppercase letters.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isupper(
  IN  int c
  )
{
  return (__isCClass( c, (_CU)));
}

/** The isxdigit function tests for any hexadecimal-digit character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isxdigit(
  IN  int c
  )
{
  return (__isCClass( c, (_CD | _CX)));
}

/** The isblank function tests that a character is a white-space character that results
    in a number of space (' ') characters being sent to the output device.  In the C locale
    this is either ' ' or '\t'.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isblank(
  IN  int c
  )
{
  return (__isCClass( c, (_CB)));
}

/** The isascii function tests that a character is one of the 128 7-bit ASCII characters.

  @param[in]  c   The character to test.

  @return     Returns nonzero (true) if c is a valid ASCII character.  Otherwize,
              zero (false) is returned.
**/
int
isascii(
  IN  int c
  )
{
  return ((c >= 0) && (c < 128));
}

/** Test whether a character is one of the characters used as a separator
    between directory elements in a path.

    Characters are '/', '\\'

    This non-standard function is unique to this implementation.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int
isDirSep(int c)
{
  return (__isCClass( c, (_C0)));
}
