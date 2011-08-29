## @file
#   This is the assembly code for transferring to control to OS S3 waking vector
#   for X64 platform
#
# Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are
# licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

ASM_GLOBAL ASM_PFX(AsmTransferControl)
ASM_PFX(AsmTransferControl):
    # rcx S3WakingVector    :DWORD
    # rdx AcpiLowMemoryBase :DWORD
    lea   _AsmTransferControl_al_0000, %eax 
    movq  $0x2800000000, %r8 
    orq   %r8, %rax
    pushq %rax
    shrd  $20, %ecx, %ebx
    andl  $0x0f, %ecx 
    movw  %cx, %bx
    movl  %ebx, jmp_addr 
    lret
_AsmTransferControl_al_0000:
    .byte    0x0b8, 0x30, 0      # mov ax, 30h as selector
    movl  %eax, %ds
    movl  %eax, %es
    movl  %eax, %fs
    movl  %eax, %gs
    movl  %eax, %ss
    movq  %cr0, %rax
    movq  %cr4, %rbx
    .byte    0x66
    andl  $0x7ffffffe, %eax 
    andb  $0xdf, %bl 
    movq  %rax, %cr0
    .byte    0x66
    movl  $0x0c0000080, %ecx 
    rdmsr
    andb  $0xfe, %ah 
    wrmsr
    movq  %rbx, %cr4
    .byte    0x0ea              # jmp far jmp_addr
jmp_addr:
    .long    0

ASM_GLOBAL ASM_PFX(AsmTransferControl32)
ASM_PFX(AsmTransferControl32):
    # S3WakingVector    :DWORD
    # AcpiLowMemoryBase :DWORD
    pushq %rbp
    movl  %esp,%ebp
    .byte 0x8d, 0x05        #  lea   eax, AsmTransferControl16
ASM_GLOBAL ASM_PFX(AsmFixAddress16)
ASM_PFX(AsmFixAddress16):
    .long    0
    pushq $0x28             # CS
    pushq %rax
    lret

ASM_GLOBAL ASM_PFX(AsmTransferControl16)
ASM_PFX(AsmTransferControl16):
    .byte 0xb8,0x30,0       # mov ax, 30h as selector
    movw  %ax,%ds
    movw  %ax,%es
    movw  %ax,%fs
    movw  %ax,%gs
    movw  %ax,%ss
    movq  %cr0, %rax        # Get control register 0  
    .byte 0x66
    .byte 0x83,0xe0,0xfe    # and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    .byte 0xf,0x22,0xc0     # mov    cr0, eax         ; Activate real mode
    .byte 0xea              # jmp far AsmJmpAddr32
ASM_GLOBAL ASM_PFX(AsmJmpAddr32)
ASM_PFX(AsmJmpAddr32):
    .long    0
