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
;   InternalX86ReadFsBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalX86ReadFsBase (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86ReadFsBase)
ASM_PFX(InternalX86ReadFsBase):
    rdfsbase rax
    ret
