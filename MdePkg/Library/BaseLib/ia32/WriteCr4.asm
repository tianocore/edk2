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
;   WriteCr4.Asm
;
; Abstract:
;
;   AsmWriteCr4 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586p
    .model  flat
    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr4 (
;   VOID
;   );
;------------------------------------------------------------------------------
_AsmWriteCr4    PROC
    mov     eax, [esp + 4]
    mov     cr4, eax
    ret
_AsmWriteCr4    ENDP

    END
