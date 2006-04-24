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
;   DivError.asm
;
; Abstract:
;
;   Set error flag for all division functions
;
;------------------------------------------------------------------------------

    .386
    .model  flat,C
    .code

InternalMathDivRemU64x32    PROC
    mov     ecx, [esp + 12]
    mov     eax, [esp + 8]
    xor     edx, edx
    div     ecx
    push    eax
    mov     eax, [esp + 8]
    div     ecx
    mov     ecx, [esp + 20]
    jecxz   @F
    mov     [ecx], edx
@@:
    pop     edx
    ret
InternalMathDivRemU64x32    ENDP

    END
