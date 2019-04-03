;; @file
;  Provide read ESP function
;
; Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmReadEsp (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadEsp)
ASM_PFX(AsmReadEsp):
    mov     eax, esp
    ret

