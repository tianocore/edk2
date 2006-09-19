/** @file
  Calculate the quotient of a 64-bit integer by a 32-bit integer

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if _MSC_EXTENSIONS

UINT64
EFIAPI
InternalMathDivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  _asm {
    mov     eax, dword ptr [Dividend + 4]
    mov     ecx, Divisor
    xor     edx, edx
    div     ecx
    push    eax                     ; save quotient on stack
    mov     eax, dword ptr [Dividend]
    div     ecx
    pop     edx                     ; restore high-order dword of the quotient
  }
}

#endif
