/** @file
    Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    NetBSD: float.h,v 1.5 2003/10/23 23:26:06 kleink Exp
**/
#ifndef _X86_FLOAT_H_
#define _X86_FLOAT_H_

/* long double and double are the same in Microsoft compilers.
    In GCC long double is fully supported.
*/
#if !defined(_MSC_VER)        /* Non-Microsoft compiler specifics. */
  #define LDBL_MANT_DIG   64
  #define LDBL_EPSILON    1.0842021724855044340E-19L
  #define LDBL_DIG        18
  #define LDBL_MIN_EXP    (-16381)
  #define LDBL_MIN        3.3621031431120935063E-4932L
  #define LDBL_MIN_10_EXP (-4931)
  #define LDBL_MAX_EXP    16384
  #define LDBL_MAX        1.1897314953572317650E+4932L
  #define LDBL_MAX_10_EXP 4932

  #define DECIMAL_DIG     21
#endif  // !defined(_MSC_VER)

#include <sys/float_ieee754.h>

#endif  /* _X86_FLOAT_H_ */
