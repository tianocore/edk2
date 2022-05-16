;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------

    SECTION .text

%include    "PushPopRegsNasm.inc"

extern ASM_PFX(SwapStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Pei2LoaderSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Pei2LoaderSwitchStack)
ASM_PFX(Pei2LoaderSwitchStack):
    xor     rax, rax
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Loader2PeiSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Loader2PeiSwitchStack)
ASM_PFX(Loader2PeiSwitchStack):
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; FspSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(FspSwitchStack)
ASM_PFX(FspSwitchStack):
    ; Save current contexts. The format must align with CONTEXT_STACK_64.
    push    rdx     ; Reserved QWORD for stack alignment
    push    rdx     ; ApiParam2
    push    rcx     ; ApiParam1
    push    rax     ; FspInfoHeader
    pushfq
    cli
    PUSHA_64
    sub     rsp, 16
    sidt    [rsp]

    ; Load new stack
    mov     rcx, rsp
    call    ASM_PFX(SwapStack)
    mov     rsp, rax

    ; Restore previous contexts
    lidt    [rsp]
    add     rsp, 16
    POPA_64
    popfq
    add     rsp, 32 ; FspInfoHeader + ApiParam[2] + Reserved QWORD
    ret

