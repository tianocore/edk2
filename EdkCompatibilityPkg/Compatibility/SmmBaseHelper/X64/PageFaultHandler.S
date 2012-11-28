#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   PageFaultHandler.S
#
# Abstract:
#
#   Defines page fault handler used to hook SMM IDT
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(PageFaultHandlerHook)
ASM_PFX(PageFaultHandlerHook):
    pushq    %rax                         # save all volatile registers
    pushq    %rcx
    pushq    %rdx
    pushq    %r8
    pushq    %r9
    pushq    %r10
    pushq    %r11

    addq     $-0x68, %rsp                 # reserve memory to store XMM registers and make address 16-byte alignment
    movdqa   %xmm0, 0(%rsp) 
    movdqa   %xmm1, 0x10(%rsp)
    movdqa   %xmm2, 0x20(%rsp)
    movdqa   %xmm3, 0x30(%rsp)
    movdqa   %xmm4, 0x40(%rsp)
    movdqa   %xmm5, 0x50(%rsp)

    addq     $-0x20, %rsp
    call     ASM_PFX(PageFaultHandler)
    addq     $0x20, %rsp

    movdqa   0(%rsp), %xmm0
    movdqa   0x10(%rsp), %xmm1
    movdqa   0x20(%rsp), %xmm2
    movdqa   0x30(%rsp), %xmm3
    movdqa   0x40(%rsp), %xmm4
    movdqa   0x50(%rsp), %xmm5
    addq     $0x68, %rsp

    testb    %al, %al                     # set ZF flag
    popq     %r11
    popq     %r10
    popq     %r9
    popq     %r8
    popq     %rdx
    popq     %rcx
    popq     %rax                         # restore all volatile registers
    jnz      L1                           # check ZF flag
#ifdef __APPLE__
    int      $3
#else
    jmpq     *ASM_PFX(mOriginalHandler)
#endif
L1:
    addq     $0x08, %rsp                  # skip error code for PF
    iretq
