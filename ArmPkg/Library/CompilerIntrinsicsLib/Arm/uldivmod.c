/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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





