;------------------------------------------------------------------------------ ;
; Copyright (c) 2022, 9elements GmbH. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;-------------------------------------------------------------------------------

%include "Nasm.inc"

DEFAULT REL
SECTION .text

;UINTN
;EFIAPI
;TriggerSmi (
;  UINTN   Cmd,
;  UINTN   Arg,
;  UINTN   Retry
;  )

global ASM_PFX(TriggerSmi)
ASM_PFX(TriggerSmi):
    push    rbx
    mov     rax, rcx
    mov     rbx, rdx
@Trigger:
    out     0b2h, al

    cmp     rax, rcx
    jne     @Return
    push    rcx
    mov     rcx, 10000
    rep     pause
    pop     rcx
    cmp     r8, 0
    je      @Return
    dec     r8
    jmp     @Trigger
@Return:
    pop     rbx
    ret
