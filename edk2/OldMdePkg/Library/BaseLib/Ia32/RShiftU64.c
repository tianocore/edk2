/** @file
  64-bit logical right shift function for IA-32

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
InternalMathRShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  _asm {
    mov     cl, byte ptr [Count]
    xor     edx, edx
    mov     eax, dword ptr [Operand + 4]
    test    cl, 32
    cmovz   edx, eax
    cmovz   eax, dword ptr [Operand + 0]
    shrd    eax, edx, cl
    shr     edx, cl
  }
}

