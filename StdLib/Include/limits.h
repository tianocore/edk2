/** @file
  The header <limits.h> defines several macros that expand to various limits and
  parameters of the standard integer types.

  The values given below are constant expressions suitable for
  use in #if preprocessing directives. Except for CHAR_BIT and
  MB_LEN_MAX, they have the same
  type as would an expression that is an object of the corresponding type
  converted according to the integer promotions.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _LIMITS_H
#define _LIMITS_H
#include  <sys/EfiCdefs.h>
#include  <Library/PcdLib.h>

/* Get the architecturally defined limits for this compilation unit. */
#include  <machine/limits.h>

/* Define the values required by the ISO/IEC 9899 Specification. */

/** Maximum number of bytes in a multibyte character, for any supported locale. **/
#define MB_LEN_MAX  4               /* UTF-8 can require up to 4 bytes */

/** Number of bits comprising the smallest object that is not a bit-field (byte). **/
#define CHAR_BIT    __CHAR_BIT

/** Minimum value for an object of type signed char. **/
#define SCHAR_MIN   __SCHAR_MIN

/** Maximum value for an object of type signed char. **/
#define SCHAR_MAX   __SCHAR_MAX

/** Maximum value for an object of type unsigned char. **/
#define UCHAR_MAX   __UCHAR_MAX

#ifdef __CHAR_UNSIGNED__
  /** Maximum value for an object of type char. **/
  #define CHAR_MAX  UCHAR_MAX
  /** Minimum value for an object of type char. **/
  #define CHAR_MIN  0
#else
  /** Maximum value for an object of type char. **/
  #define CHAR_MAX  SCHAR_MAX
  /** Minimum value for an object of type char. **/
  #define CHAR_MIN  SCHAR_MIN
#endif

/** Minimum value for an object of type short int. **/
#define SHRT_MIN    __SHRT_MIN

/** Maximum value for an object of type short int. **/
#define SHRT_MAX    __SHRT_MAX

/** Maximum value for an object of type unsigned short int. **/
#define USHRT_MAX   __USHRT_MAX

/** Minimum value for an object of type int. **/
#define INT_MIN     __INT_MIN

/** Maximum value for an object of type int. **/
#define INT_MAX     __INT_MAX

/** Maximum value for an object of type unsigned int. **/
#define UINT_MAX    __UINT_MAX

/** Minimum value for an object of type long int. **/
#define LONG_MIN    __LONG_MIN

/** Maximum value for an object of type long int. **/
#define LONG_MAX    __LONG_MAX

/** Maximum value for an object of type unsigned long int. **/
#define ULONG_MAX   __ULONG_MAX

/** Minimum value for an object of type long long int. **/
#define LLONG_MIN   __LLONG_MIN

/** Maximum value for an object of type long long int. **/
#define LLONG_MAX   __LLONG_MAX

/** Maximum value for an object of type unsigned long long int. **/
#define ULLONG_MAX  __ULLONG_MAX

/* Object limits used in the Standard Libraries */
#if (PcdGet32(PcdMaximumAsciiStringLength) > 0)
  /** Maximum length of an arbitrary "narrow-character" string. **/
  #define ASCII_STRING_MAX    PcdGet32(PcdMaximumAsciiStringLength)
#else
  /** Maximum length of an arbitrary "narrow-character" string. **/
  #define ASCII_STRING_MAX    256
#endif

#if (PcdGet32(PcdMaximumUnicodeStringLength) > 0)
  /** Maximum length of an arbitrary "wide-character" string. **/
  #define UNICODE_STRING_MAX    PcdGet32(PcdMaximumUnicodeStringLength)
#else
  /** Maximum length of an arbitrary "wide-character" string. **/
  #define UNICODE_STRING_MAX    512
#endif

/* Limits for BSD Compatibility */
#define NL_TEXTMAX    2048

#include  <sys/syslimits.h>

#endif  /* _LIMITS_H */
