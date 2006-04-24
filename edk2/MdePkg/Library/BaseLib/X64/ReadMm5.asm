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
;   ReadMm5.Asm
;
; Abstract:
;
;   AsmReadMm5 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadMm5 (
;   VOID
;   );
;------------------------------------------------------------------------------
AsmReadMm5  PROC
    DB      48h, 0fh, 7eh, 0e8h
    ret
AsmReadMm5  ENDP

    END
