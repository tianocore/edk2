/** @file
  The header <limits.h> defines several macros that expand to various limits and
  parameters of the standard integer types.

  The values given below shall be replaced by constant expressions suitable for
  use in #if preprocessing directives. Moreover, except for CHAR_BIT and
  MB_LEN_MAX, the following shall be replaced by expressions that have the same
  type as would an expression that is an object of the corresponding type
  converted according to the integer promotions. Their implementation-defined
  values shall be equal or greater in magnitude (absolute value) to those
  documented, with the same sign.

  If the value of an object of type char is treated as a signed integer when
  used in an expression, the value of CHAR_MIN shall be the same as that of
  SCHAR_MIN and the value of CHAR_MAX shall be the same as that of SCHAR_MAX.
  Otherwise, the value of CHAR_MIN shall be 0 and the value of CHAR_MAX shall
  be the same as that of UCHAR_MAX.)
  The value UCHAR_MAX shall equal 2^(CHAR_BIT - 1).

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _LIMITS_H
#define _LIMITS_H
#include  <sys/EfiCdefs.h>
#include  <Library/PcdLib.h>

/* Get the architecturally defined limits for this compilation unit. */
#include  <machine/limits.h>

/* Define the values required by Specification. */

/** maximum number of bytes in a multibyte character, for any supported locale **/
#define MB_LEN_MAX  2 /* 16-bit UTC-2 */

/** Number of bits for smallest object that is not a bit-field (byte). **/
#define CHAR_BIT    __CHAR_BIT

/** minimum value for an object of type signed char **/
#define SCHAR_MIN   __SCHAR_MIN

/** maximum value for an object of type signed char **/
#define SCHAR_MAX   __SCHAR_MAX

/** maximum value for an object of type unsigned char **/
#define UCHAR_MAX   __UCHAR_MAX

#ifdef __CHAR_UNSIGNED__
  /** maximum value for an object of type char **/
  #define CHAR_MAX  UCHAR_MAX
  /** minimum value for an object of type char **/
  #define CHAR_MIN  0
#else
  /** maximum value for an object of type char **/
  #define CHAR_MAX  SCHAR_MAX
  /** minimum value for an object of type char **/
  #define CHAR_MIN  SCHAR_MIN
#endif

/** minimum value for an object of type short int **/
#define SHRT_MIN    __SHRT_MIN

/** maximum value for an object of type short int **/
#define SHRT_MAX    __SHRT_MAX

/** maximum value for an object of type unsigned short int **/
#define USHRT_MAX   __USHRT_MAX

/** minimum value for an object of type int **/
#define INT_MIN     __INT_MIN

/** maximum value for an object of type int **/
#define INT_MAX     __INT_MAX

/** maximum value for an object of type unsigned int **/
#define UINT_MAX    __UINT_MAX

/** minimum value for an object of type long int **/
#define LONG_MIN    __LONG_MIN

/** maximum value for an object of type long int **/
#define LONG_MAX    __LONG_MAX

/** maximum value for an object of type unsigned long int **/
#define ULONG_MAX   __ULONG_MAX

/** minimum value for an object of type long long int **/
#define LLONG_MIN   __LLONG_MIN

/** maximum value for an object of type long long int **/
#define LLONG_MAX   __LLONG_MAX

/** maximum value for an object of type unsigned long long int **/
#define ULLONG_MAX  __ULLONG_MAX

/* Object limits used in the Standard Libraries */
#if (PcdGet32(PcdMaximumAsciiStringLength) > 0)
  #define ASCII_STRING_MAX    PcdGet32(PcdMaximumAsciiStringLength)
#else
  #define ASCII_STRING_MAX    256
#endif

#if (PcdGet32(PcdMaximumUnicodeStringLength) > 0)
  #define UNICODE_STRING_MAX    PcdGet32(PcdMaximumUnicodeStringLength)
#else
  #define UNICODE_STRING_MAX    512
#endif

/* Limits for BSD Compatibility */
#define NL_TEXTMAX    2048
#include  <sys/syslimits.h>

#endif  /* _LIMITS_H */
