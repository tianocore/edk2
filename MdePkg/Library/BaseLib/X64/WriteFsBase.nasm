;------------------------------------------------------------------------------
;
; Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   WriteFsBase.nasm
;
; Abstract:
;
;   AsmWriteFsBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteFsBase (
;   UINT64  FsBase
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteFsBase)
ASM_PFX(AsmWriteFsBase):
    wrfsbase rcx
    ret

