/** @file
  Calculate the product of a 64-bit integer and another 64-bit integer

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

UINT64
EFIAPI
InternalMathMultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  )
{
  _asm {
    mov     ebx, dword ptr [Multiplicand + 0]
    mov     edx, dword ptr [Multiplier + 0]
    mov     ecx, ebx
    mov     eax, edx
    imul    ebx, dword ptr [Multiplier + 4]
    imul    edx, dword ptr [Multiplicand + 4]
    add     ebx, edx
    mul     ecx
    add     edx, ebx
  }
}

