;------------------------------------------------------------------------------
;
; Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   VmgExitSvsm.Asm
;
; Abstract:
;
;   AsmVmgExitSvsm function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmVmgExitSvsm (
;   UINT64  Rcx,
;   UINT64  Rdx,
;   UINT64  R8,
;   UINT64  R9,
;   UINT64  Rax,
;   UINT8   *CaaCallPending,
;   UINT8   *CallPending
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmVmgExitSvsm)
ASM_PFX(AsmVmgExitSvsm):
;
; Calling convention puts Rcx/Rdx/R8/R9 arguments in the proper registers already.
; Only Rax needs to be loaded (from the stack).
;
    mov     rax, [rsp + 40]     ; Get Rax parameter from the stack
    rep     vmmcall

;
; Perform the atomic exchange and return the CAA call pending value
;
    mov     r10, [rsp + 56]     ; Get Pending address
    mov     cl, byte [r10]
    mov     r11, [rsp + 48]     ; Get CaaCallPending address
    xchg    byte [r11], cl
    mov     byte [r10], cl      ; Return the exchanged value

;
; RAX has the value to be returned from the SVSM
;
    ret

