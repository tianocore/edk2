/** @file
  AsmReadCr4 function

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

UINTN
EFIAPI
AsmReadCr4 (
  VOID
  )
{
  __asm {
    _emit  0x0f  // mov  eax, cr4
    _emit  0x20
    _emit  0xE0
  }
}

