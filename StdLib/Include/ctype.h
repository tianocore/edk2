/** @file
  Single-byte character classification, case conversion macros, and
  function declarations.

  The header <ctype.h> declares several functions useful for testing and mapping
  characters.  In all cases, the argument is an int, the value of which shall be
  representable as an unsigned char or shall equal the value of the macro EOF.
  If the argument has any other value, the behavior is undefined.

  The behavior of these functions is affected by the current locale.  The
  default is the "C" locale.

  The term "printing character" refers to a member of a locale-specific
  set of characters, each of which occupies at least one printing position on an output
  device; the term control character refers to a member of a locale-specific
  set of characters that are not printing characters.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _CTYPE_H
#define _CTYPE_H
#include  <sys/EfiCdefs.h>
#include  <sys/_ctype.h>

__BEGIN_DECLS
// Declarations for the classification Functions

/** The isalnum function tests for any character for which isalpha or isdigit
    is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isalnum(int c);

/** The isalpha function tests for any character for which isupper or islower
    is true, or any character that is one of a locale-specific set of
    alphabetic characters for which none of iscntrl, isdigit, ispunct, or
    isspace is true. In the "C" locale, isalpha returns true only for the
    characters for which isupper or islower is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isalpha(int c);

/** The isblank function tests that a character is a white-space character that results
    in a number of space (' ') characters being sent to the output device.  In the C locale
    this is either ' ' or '\t'.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isblank(int);

/** The iscntrl function tests for any control character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int iscntrl(int c);

/** The isdigit function tests for any decimal-digit character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isdigit(int c);

/** The isgraph function tests for any printing character except space (' ').

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isgraph(int c);

/** The islower function tests for any character that is a lowercase letter or
    is one of a locale-specific set of characters for which none of iscntrl,
    isdigit, ispunct, or isspace is true.  In the "C" locale, islower returns
    true only for the lowercase letters.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int islower(int c);

/** The isprint function tests for any printing character including space (' ').

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isprint(int c);

/** The ispunct function tests for any printing character that is one of a
    locale-specific set of punctuation characters for which neither isspace nor
    isalnum is true. In the "C" locale, ispunct returns true for every printing
    character for which neither isspace nor isalnum is true.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int ispunct(int c);

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
int isspace(int c);

/** The isupper function tests for any character that is an uppercase letter or
    is one of a locale-specific set of characters for which none of iscntrl,
    isdigit, ispunct, or isspace is true. In the "C" locale, isupper returns
    true only for the uppercase letters.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isupper(int c);

/** The isxdigit function tests for any hexadecimal-digit character.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isxdigit(int c);

/** The isascii function tests that a character is one of the 128 7-bit ASCII characters.
    This function is not part of the C standard, but is commonly used.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isascii(int c);

/** Test whether a character is one of the characters used as a separator
    between directory elements in a path.

    Characters are '/', '\\'

    This non-standard function is unique to this implementation.

    @param[in]    c   The character to be tested.

    @return   Returns nonzero (true) if and only if the value of the parameter c
              can be classified as specified in the description of the function.
**/
int isDirSep(int c);

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
int tolower(int c);

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
int toupper(int c);

__END_DECLS

/** Character Classification Macros.
    Undefine individually or define NO_CTYPE_MACROS, before including <ctype.h>,
    in order to use the Function version of the character classification macros.
@{
**/
#ifndef NO_CTYPE_MACROS
  #define isalnum(c)    (__isCClass( (int)c, (_CD | _CU | _CL | _XA)))
  #define isalpha(c)    (__isCClass( (int)c, (_CU | _CL | _XA)))
  #define iscntrl(c)    (__isCClass( (int)c, (_CC)))
  #define isdigit(c)    (__isCClass( (int)c, (_CD)))
  #define isgraph(c)    (__isCClass( (int)c, (_CG)))
  #define islower(c)    (__isCClass( (int)c, (_CL)))
  #define isprint(c)    (__isCClass( (int)c, (_CS | _CG)))
  #define ispunct(c)    (__isCClass( (int)c, (_CP)))
  #define isspace(c)    (__isCClass( (int)c, (_CW)))
  #define isupper(c)    (__isCClass( (int)c, (_CU)))
  #define isxdigit(c)   (__isCClass( (int)c, (_CD | _CX)))
  #define isDirSep(c)   (__isCClass( (int)c, (_C0)))
  #define tolower(c)    (__toLower((int)c))
  #define toupper(c)    (__toUpper((int)c))
#endif  /* NO_CTYPE_MACROS */
///@}

#endif  /* _CTYPE_H */
