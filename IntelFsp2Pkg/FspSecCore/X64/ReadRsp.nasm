;; @file
;  Provide read RSP function
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadStackPointer (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadStackPointer)
ASM_PFX(AsmReadStackPointer):
    mov     rax, rsp
    ret

