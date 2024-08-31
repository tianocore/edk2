;------------------------------------------------------------------------------
; @file
;
; Switch from 5-level paging mode to 4-level paging mode.
;
; This assumes everything (code, stack, page tables) is in 32-bit
; address space.  Which is true for PEI phase even in X64 builds
; because low memory is used for early firmware setup.
;
; This also assumes the standard ResetVector GDT is active.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

SECTION .text
BITS    64

global ASM_PFX(Switch4Level)
ASM_PFX(Switch4Level):

    ; save regs
    push    rax
    push    rbx
    push    rcx
    push    rdx

    ; cs:ip for long mode
    lea     rax, [rel Switch4Level64]
    mov     rbx, 0x3800000000 ; LINEAR_CODE64_SEL << 32
    or      rax, rbx
    push    rax

    ; cs:ip for 32-bit mode
    lea     rax, [rel Switch4Level32]
    mov     rbx, 0x1000000000 ; LINEAR_CODE_SEL << 32
    or      rax, rbx
    push    rax

    ; enter 32-bit mode
    retf

Switch4Level64:
    ; restore regs
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    ret

BITS    32

Switch4Level32:
    ; disable paging
    mov     eax, cr0
    btc     eax, 31   ; clear PG
    mov     cr0, eax

    ; disable 5-level paging
    mov     eax, cr4
    btc     eax, 12   ; clear la57
    mov     cr4, eax

    ; fixup cr3 (dereference 5th level)
    mov     eax, cr3
    mov     eax, [ eax ]
    and     eax, 0xfffff000
    mov     cr3, eax

    ; enable paging
    mov     eax, cr0
    bts     eax, 31   ; set PG
    mov     cr0, eax

    ; back to long mode
    retf
