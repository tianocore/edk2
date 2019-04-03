;/** @file
;
;    IDT vector entry.
;
;  Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

    SECTION .text

;
;------------------------------------------------------------------------------
;  Generic IDT Vector Handlers for the Host.
;
;------------------------------------------------------------------------------

ALIGN   8
global ASM_PFX(AsmGetVectorTemplatInfo)
global ASM_PFX(AsmVectorFixup)

@VectorTemplateBase:
        push  eax
        db    0x6a       ; push #VectorNumber
@VectorNum:
        db    0
        mov   eax, CommonInterruptEntry
        jmp   eax
@VectorTemplateEnd:

global ASM_PFX(AsmGetVectorTemplatInfo)
ASM_PFX(AsmGetVectorTemplatInfo):
        mov   ecx, [esp + 4]
        mov   dword [ecx], @VectorTemplateBase
        mov   eax, (@VectorTemplateEnd - @VectorTemplateBase)
        ret

global ASM_PFX(AsmVectorFixup)
ASM_PFX(AsmVectorFixup):
        mov   eax, dword [esp + 8]
        mov   ecx, [esp + 4]
        mov   [ecx + (@VectorNum - @VectorTemplateBase)], al
        ret

;---------------------------------------;
; CommonInterruptEntry                  ;
;---------------------------------------;
; The follow algorithm is used for the common interrupt routine.

;
; +---------------------+ <-- 16-byte aligned ensured by processor
; +    Old SS           +
; +---------------------+
; +    Old RSP          +
; +---------------------+
; +    RFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    RIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +    Vector Number    +
; +---------------------+

CommonInterruptEntry:
  cli

  jmp $

