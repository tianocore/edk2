/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Llvm_int_lib.h"
#include <Library/BaseLib.h>


UINT32 __udivsi3(UINT32 n, UINT32 d);
UINT32 __umodsi3(UINT32 a, UINT32 b);


UINT64
__aeabi_uidivmod(unsigned numerator, unsigned denominator)
{
  UINT64  Return;

  Return = __udivsi3 (numerator, denominator);
  Return |= LShiftU64 (__umodsi3 (numerator, denominator), 32);

  return Return;
}

unsigned
__aeabi_uidiv (unsigned n, unsigned d)
{
  return __udivsi3 (n, d);
}





