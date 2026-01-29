;; @file
;   This is the assembly code for page fault handler hook.
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;;

extern ASM_PFX(PageFaultHandler)

    DEFAULT REL
    SECTION .text

global ASM_PFX(PageFaultHandlerHook)
ASM_PFX(PageFaultHandlerHook):
    add     rsp, -0x10
    ; save rax
    mov     [rsp + 0x8], rax

    ;push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11
    ; save volatile fp registers
    ; 68h + 08h(for alignment)
    add     rsp, -0x70
    stmxcsr [rsp + 0x60]
    movdqa  [rsp + 0x0], xmm0
    movdqa  [rsp + 0x10], xmm1
    movdqa  [rsp + 0x20], xmm2
    movdqa  [rsp + 0x30], xmm3
    movdqa  [rsp + 0x40], xmm4
    movdqa  [rsp + 0x50], xmm5

    add     rsp, -0x20
    call    ASM_PFX(PageFaultHandler)
    add     rsp, 0x20

    ; load volatile fp registers
    ldmxcsr [rsp + 0x60]
    movdqa  xmm0,  [rsp + 0x0]
    movdqa  xmm1,  [rsp + 0x10]
    movdqa  xmm2,  [rsp + 0x20]
    movdqa  xmm3,  [rsp + 0x30]
    movdqa  xmm4,  [rsp + 0x40]
    movdqa  xmm5,  [rsp + 0x50]
    add     rsp, 0x70

    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    ;pop     rax                         ; restore all volatile registers

    add     rsp, 0x10

    ; rax returned from PageFaultHandler is NULL or OriginalHandler address
    ; NULL if the page fault is handled by PageFaultHandler
    ; OriginalHandler address if the page fault is not handled by PageFaultHandler
    test    rax, rax

    ; save OriginalHandler address
    mov     [rsp - 0x10], rax
    ; restore rax
    mov     rax, [rsp - 0x8]

    jz      .0

    ; jump to OriginalHandler
    jmp     qword [rsp - 0x10]

.0:
    add     rsp, 0x8                    ; skip error code for PF
    iretq

