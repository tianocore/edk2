;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   DivU64x64Remainder.asm
;
; Abstract:
;
;   Calculate the quotient of a 64-bit integer by a 64-bit integer and returns
;   both the quotient and the remainder
;
;------------------------------------------------------------------------------

    .386
    .model  flat,C
    .code

EXTERN  InternalMathDivRemU64x32:PROC

InternalMathDivRemU64x64    PROC
    mov     ecx, [esp + 16]
    test    ecx, ecx
    jnz     _@DivRemU64x64
    mov     ecx, [esp + 20]
    jecxz   @F
    and     dword ptr [ecx + 4], 0
    mov     [esp + 16], ecx
@@:
    jmp     InternalMathDivRemU64x32
InternalMathDivRemU64x64    ENDP

_@DivRemU64x64  PROC    USES    ebx esi edi
    mov     edx, dword ptr [esp + 20]
    mov     eax, dword ptr [esp + 16]
    mov     edi, edx
    mov     esi, eax
    mov     ebx, dword ptr [esp + 24]
@@:
    shr     edx, 1
    rcr     eax, 1
    shrd    ebx, ecx, 1
    shr     ecx, 1
    jnz     @B
    div     ebx
    mov     ebx, eax
    mov     ecx, [esp + 28]
    mul     dword ptr [esp + 24]
    imul    ecx, ebx
    add     edx, ecx
    mov     ecx, dword ptr [esp + 32]
    jc      @TooLarge
    cmp     edi, edx
    ja      @Correct
    jb      @TooLarge
    cmp     esi, eax
    jae     @Correct
@TooLarge:
    dec     ebx
    jecxz   @Return
    sub     eax, dword ptr [esp + 24]
    sbb     edx, dword ptr [esp + 28]
@Correct:
    jecxz   @Return
    sub     esi, eax
    sbb     edi, edx
    mov     [ecx], esi
    mov     [ecx + 4], edi
@Return:
    mov     eax, ebx
    xor     edx, edx
    ret
_@DivRemU64x64  ENDP

    END
