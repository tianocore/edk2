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
    mov     rax, rcx                    ; Smi handler expect Cmd in RAX
    mov     rbx, rdx                    ; Smi handler expect Argument in RBX
@Trigger:
    out     0b2h, al                    ; write to APM port to trigger SMI

; There might ba a delay between writing the Smi trigger register and
; entering SMM, in which case the Smi handler will do nothing as only
; synchronous Smis are handled. In addition when there's no Smi handler
; or the SmmStore feature isn't compiled in, no register will be modified.

; As there's no livesign from SMM, just wait a bit for the handler to fire,
; and then try again.

    cmp     rax, rcx                    ; Check if rax was modified by SMM
    jne     @Return                     ; SMM modified rax, return now
    push    rcx                         ; save rcx to stack
    mov     rcx, 10000
    rep     pause                       ; add a small delay
    pop     rcx                         ; restore rcx
    cmp     r8, 0
    je      @Return
    dec     r8
    jmp     @Trigger
@Return:
    pop     rbx
    ret
