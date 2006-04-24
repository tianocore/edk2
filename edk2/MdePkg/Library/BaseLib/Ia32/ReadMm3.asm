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
;   ReadMm3.Asm
;
; Abstract:
;
;   AsmReadMm3 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586P
    .model  flat
    .xmm
    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadMm3 (
;   VOID
;   );
;------------------------------------------------------------------------------
_AsmReadMm3 PROC
    push    eax
    push    eax
    movq    [esp], mm3
    pop     eax
    pop     edx
    ret
_AsmReadMm3 ENDP

    END
