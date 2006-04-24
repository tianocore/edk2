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
;   ReadCr2.Asm
;
; Abstract:
;
;   AsmReadCr2 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .386p
    .model  flat
    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr2 (
;   VOID
;   );
;------------------------------------------------------------------------------
_AsmReadCr2 PROC
    mov     eax, cr2
    ret
_AsmReadCr2 ENDP

    END
