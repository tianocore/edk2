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
;   WriteMm5.Asm
;
; Abstract:
;
;   AsmWriteMm5 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteMm5 (
;   VOID
;   );
;------------------------------------------------------------------------------
AsmWriteMm5 PROC
    DB      48h, 0fh, 6eh, 0e9h
    ret
AsmWriteMm5 ENDP

    END
