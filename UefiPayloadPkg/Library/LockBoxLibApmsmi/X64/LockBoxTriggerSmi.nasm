;------------------------------------------------------------------------------ ;
; LockBox APMC SMI trigger helper
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; UINTN EFIAPI LockBoxTriggerSmi(UINTN Cmd, UINTN Arg, UINTN Retry)
;  - Cmd:   RAX value (AH=subcmd, AL=cmd byte)
;  - Arg:   RBX value (pointer/physical address in flat memory)
;  - Retry: number of retries if SMM does not modify RAX
;------------------------------------------------------------------------------ ;

%include "Nasm.inc"

DEFAULT REL
SECTION .text

global ASM_PFX(LockBoxTriggerSmi)
ASM_PFX(LockBoxTriggerSmi):
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

