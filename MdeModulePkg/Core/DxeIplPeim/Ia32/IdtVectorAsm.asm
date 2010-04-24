;/** @file
;  
;    IDT vector entry.
;  
;  Copyright (c) 2007 - 2008, Intel Corporation. All rights reserved.<BR>
;  This program and the accompanying materials
;  are licensed and made available under the terms and conditions of the BSD License
;  which accompanies this distribution.  The full text of the license may be found at
;  http://opensource.org/licenses/bsd-license.php
;  
;  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;  
;**/

    .686p
    .model  flat,C
    .code

;
;------------------------------------------------------------------------------
;  Generic IDT Vector Handlers for the Host. 
;
;------------------------------------------------------------------------------

ALIGN   8
PUBLIC  AsmGetVectorTemplatInfo
PUBLIC  AsmVectorFixup

PUBLIC  AsmVectorFixup

@VectorTemplateBase:
        push  eax
        db    6ah       ; push #VectorNumber
@VectorNum:
        db    0
        mov   eax, CommonInterruptEntry
        jmp   eax
@VectorTemplateEnd:


AsmGetVectorTemplatInfo PROC
        mov   ecx, [esp + 4]
        mov   [ecx], @VectorTemplateBase
        mov   eax, (@VectorTemplateEnd - @VectorTemplateBase)
        ret
AsmGetVectorTemplatInfo ENDP


AsmVectorFixup PROC
        mov   eax, dword ptr [esp + 8]
        mov   ecx, [esp + 4]
        mov   [ecx + (@VectorNum - @VectorTemplateBase)], al
        ret
AsmVectorFixup ENDP


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

CommonInterruptEntry PROC 
  cli
 
  jmp $
CommonInterruptEntry ENDP

END


