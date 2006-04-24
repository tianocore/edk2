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
;   WriteMm0.Asm
;
; Abstract:
;
;   AsmWriteMm0 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteMm0 (
;   VOID
;   );
;------------------------------------------------------------------------------
AsmWriteMm0 PROC
    DB      48h, 0fh, 6eh, 0c1h
    ret
AsmWriteMm0 ENDP

    END
