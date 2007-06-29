/** @file
  Calculate the quotient of a 64-bit integer by a 64-bit integer and returns
  both the quotient and the remainderSet error flag for all division functions

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


UINT64
EFIAPI
InternalMathDivRemU64x64 (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder    OPTIONAL
  )
{
  _asm {
    mov     edx, dword ptr [Dividend + 4]
    mov     eax, dword ptr [Dividend + 0]   // edx:eax <- dividend
    mov     edi, edx
    mov     esi, eax                    // edi:esi <- dividend
    mov     ecx, dword ptr [Divisor + 4]
    mov     ebx, dword ptr [Divisor + 0]   // ecx:ebx <- divisor
BitLoop:
    shr     edx, 1
    rcr     eax, 1
    shrd    ebx, ecx, 1
    shr     ecx, 1
    jnz     BitLoop
    div     ebx
    mov     ebx, eax                    // ebx <- quotient
    mov     ecx, dword ptr [Divisor + 4]
    mul     dword ptr [Divisor]
    imul    ecx, ebx
    add     edx, ecx
    mov     ecx, Remainder
    jc      TooLarge                   // product > 2^64
    cmp     edi, edx                    // compare high 32 bits
    ja      Correct
    jb      TooLarge                   // product > dividend
    cmp     esi, eax
    jae     Correct                    // product <= dividend
TooLarge:
    dec     ebx                         // adjust quotient by -1
    jecxz   Return                     // return if Remainder == NULL
    sub     eax, dword ptr [Divisor + 0]
    sbb     edx, dword ptr [Divisor + 4]
Correct:
    jecxz   Return
    sub     esi, eax
    sbb     edi, edx                    // edi:esi <- remainder
    mov     [ecx], esi
    mov     [ecx + 4], edi
Return:
    mov     eax, ebx                    // eax <- quotient
    xor     edx, edx
  }
}

