;------------------------------------------------------------------------------
;
; Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   VmgExit.Asm
;
; Abstract:
;
;   AsmVmgExit function
;
; Notes:
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmVmgExit (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmVmgExit)
ASM_PFX(AsmVmgExit):
;
; NASM doesn't support the vmmcall instruction in 32-bit mode, so work around
; this by temporarily switching to 64-bit mode.
;
BITS    64
    rep     vmmcall
BITS    32
    ret

