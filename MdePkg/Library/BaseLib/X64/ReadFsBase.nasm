;------------------------------------------------------------------------------
;
; Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   ReadFsBase.nasm
;
; Abstract:
;
;   AsmReadFsBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadFsBase (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadFsBase)
ASM_PFX(AsmReadFsBase):
    rdfsbase rax
    ret

