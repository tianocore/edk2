/** @file
  This header <machine/limits.h> defines several macros that expand to various
  CPU-architecture-specific limits and parameters of the standard integer types.

  The values given below are constant expressions suitable for use
  in #if preprocessing directives.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <x86/limits.h>

/** Number of bits making up a pointer. **/
#define __POINTER_BIT   32

/** Number of bits comprising an object of type long int. **/
#define __LONG_BIT      32

/** Minimum value for an object of type long int. **/
#define __LONG_MIN    (-2147483647L - 1L)   // -(2^31 - 1)

/** Maximum value for an object of type long int. **/
#define __LONG_MAX     2147483647L          // 2^31 - 1

/** Maximum value for an object of type unsigned long int. **/
#define __ULONG_MAX   0xffffffffUL          // 2^32 - 1

