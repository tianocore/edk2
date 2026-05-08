;------------------------------------------------------------------------------ ;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   CpuBreakpoint.Asm
;
; Abstract:
;
;   CpuBreakpoint function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuBreakpoint (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(CpuBreakpoint)
ASM_PFX(CpuBreakpoint):
    int3
    ret

