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
;   InternalX86WriteFsBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86WriteFsBase (
;   UINT64  FsBase
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86WriteFsBase)
ASM_PFX(InternalX86WriteFsBase):
    wrfsbase rcx
    ret
