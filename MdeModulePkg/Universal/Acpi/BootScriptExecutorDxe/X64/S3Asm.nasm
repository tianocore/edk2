;; @file
;   This is the assembly code for transferring to control to OS S3 waking vector
;   for X64 platform
;
; Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;;

extern ASM_PFX(mOriginalHandler)
extern ASM_PFX(PageFaultHandler)

    DEFAULT REL
    SECTION .text

global ASM_PFX(AsmFixAddress16)
global ASM_PFX(AsmJmpAddr32)

global ASM_PFX(AsmTransferControl)
ASM_PFX(AsmTransferControl):
    ; rcx S3WakingVector    :DWORD
    ; rdx AcpiLowMemoryBase :DWORD
    lea   eax, [.0]
    mov   r8, 0x2800000000
    or    rax, r8
    push  rax
    shrd  ebx, ecx, 20
    and   ecx, 0xf
    mov   bx, cx
    mov   [@jmp_addr + 1], ebx
    retf
BITS 16
.0:
    mov ax, 0x30
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    mov   eax, cr0
    mov   ebx, cr4
    and   eax, ((~ 0x80000001) & 0xffffffff)
    and   bl, ~ (1 << 5)
    mov   cr0, eax
    mov   ecx, 0xc0000080
    rdmsr
    and   ah, ~ 1
    wrmsr
    mov   cr4, ebx
@jmp_addr:
    jmp   0x0:0x0

global ASM_PFX(AsmTransferControl32)
ASM_PFX(AsmTransferControl32):
BITS 32
    ; S3WakingVector    :DWORD
    ; AcpiLowMemoryBase :DWORD
    push  ebp
    mov   ebp, esp
    DB    0x8d, 0x5          ;  lea   eax, AsmTransferControl16
ASM_PFX(AsmFixAddress16): DD 0
    push  0x28               ; CS
    push  eax
    retf

global ASM_PFX(AsmTransferControl16)
ASM_PFX(AsmTransferControl16):
BITS 16
    mov   ax, 0x30
o32 mov   ds, eax
o32 mov   es, eax
o32 mov   fs, eax
o32 mov   gs, eax
o32 mov   ss, eax
    mov   eax, cr0          ; Get control register 0
    and   eax, 0fffffffeh  ; Clear PE bit (bit #0)
    mov   cr0, eax         ; Activate real mode
    DB    0xea              ; jmp far AsmJmpAddr32
ASM_PFX(AsmJmpAddr32): DD 0

global ASM_PFX(PageFaultHandlerHook)
ASM_PFX(PageFaultHandlerHook):
BITS 64
    push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11
    ; save volatile fp registers
    add     rsp, -0x68
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
    add     rsp, 0x68

    test    al, al

    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    pop     rax                         ; restore all volatile registers
    jnz     .1
    jmp     qword [ASM_PFX(mOriginalHandler)]
.1:
    add     rsp, 0x8                    ; skip error code for PF
    iretq

