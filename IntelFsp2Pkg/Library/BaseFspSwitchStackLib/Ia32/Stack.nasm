;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permenent memory.
;
;------------------------------------------------------------------------------

    SECTION .text

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
    xor     eax, eax
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
    ; Save current contexts
    push    eax
    pushfd
    cli
    pushad
    sub     esp, 8
    sidt    [esp]

    ; Load new stack
    push    esp
    call    ASM_PFX(SwapStack)
    mov     esp, eax

    ; Restore previous contexts
    lidt    [esp]
    add     esp, 8
    popad
    popfd
    add     esp, 4
    ret

