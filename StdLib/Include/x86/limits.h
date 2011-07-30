/** @file
  Machine specific values for <limits.h>.

  Within this file, the ^ character is used in comments to represent exponentiation.
  Thus, 2^7 means "2 to the 7th power", NOT "2 XOR 7".

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _MACHINE_LIMITS_H
#define _MACHINE_LIMITS_H

/** Number of bits for smallest object that is not a bit-field (byte). **/
#define __CHAR_BIT    8

/** minimum value for an object of type signed char **/
#define __SCHAR_MIN   -128 // -(2^7 - 1)

/** maximum value for an object of type signed char **/
#define __SCHAR_MAX   +127 // 2^7 - 1

/** maximum value for an object of type unsigned char **/
#define __UCHAR_MAX   255  // 2^8 - 1

/** minimum value for an object of type short int **/
#define __SHRT_MIN    -32768 // -(2^15 - 1)

/** maximum value for an object of type short int **/
#define __SHRT_MAX    +32767 // 2^15 - 1

/** maximum value for an object of type unsigned short int **/
#define __USHRT_MAX   65535 // 2^16 - 1

/** maximum value for an object of type int **/
#define __INT_MAX     +2147483647 // 2^31 - 1

/** minimum value for an object of type int **/
#define __INT_MIN     (-2147483647 - 1) // -(2^31 - 1)

/** maximum value for an object of type unsigned int **/
#define __UINT_MAX    0xffffffff // 2^32 - 1

/** minimum value for an object of type long long int **/
//#define __LLONG_MIN   -9223372036854775808LL // -(2^63 - 1)
//#define __LLONG_MIN   ((-9223372036854775807LL)-1) // -(2^63 - 1)
#define __LLONG_MIN   (-9223372036854775807LL - 1LL) // -(2^63 - 1)

/** maximum value for an object of type long long int **/
#define __LLONG_MAX   9223372036854775807LL // 2^63 - 1

/** maximum value for an object of type unsigned long long int **/
//#define __ULLONG_MAX  18446744073709551615ULL // 2^64 - 1
#define __ULLONG_MAX  0xFFFFFFFFFFFFFFFFULL // 2^64 - 1

/* Intel extensions to <limits.h> for UEFI */
#define __SHORT_BIT     16
#define __WCHAR_BIT     16
#define __INT_BIT       32
#define __LONG_LONG_BIT 64

#endif    /* _MACHINE_LIMITS_H */
