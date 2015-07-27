## @file
#   This is the assembly code for page fault handler hook.
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
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

ASM_GLOBAL ASM_PFX(PageFaultHandlerHook)
ASM_PFX(PageFaultHandlerHook):
    addq     $-0x10, %rsp
    # save rax
    movq     %rax, 0x08(%rsp)

    # pushq    %rax                         # save all volatile registers
    pushq    %rcx
    pushq    %rdx
    pushq    %r8
    pushq    %r9
    pushq    %r10
    pushq    %r11
    # save volatile fp registers
    # 68h + 08h(for alignment)
    addq     $-0x70, %rsp
    stmxcsr  0x60(%rsp)
    movdqa   %xmm0, 0x0(%rsp) 
    movdqa   %xmm1, 0x10(%rsp) 
    movdqa   %xmm2, 0x20(%rsp) 
    movdqa   %xmm3, 0x30(%rsp) 
    movdqa   %xmm4, 0x40(%rsp) 
    movdqa   %xmm5, 0x50(%rsp) 

    addq     $-0x20, %rsp
    call     ASM_PFX(PageFaultHandler)
    addq     $0x20, %rsp

    # load volatile fp registers
    ldmxcsr  0x60(%rsp)
    movdqa   0x0(%rsp), %xmm0
    movdqa   0x10(%rsp), %xmm1
    movdqa   0x20(%rsp), %xmm2
    movdqa   0x30(%rsp), %xmm3
    movdqa   0x40(%rsp), %xmm4
    movdqa   0x50(%rsp), %xmm5
    addq     $0x70, %rsp

    popq     %r11
    popq     %r10
    popq     %r9
    popq     %r8
    popq     %rdx
    popq     %rcx
    # popq     %rax                         # restore all volatile registers

    addq     $0x10, %rsp

    # rax returned from PageFaultHandler is NULL or OriginalHandler address
    # NULL if the page fault is handled by PageFaultHandler
    # OriginalHandler address if the page fault is not handled by PageFaultHandler
    testq    %rax, %rax

    # save OriginalHandler address
    movq     %rax, -0x10(%rsp)
    # restore rax
    movq     -0x08(%rsp), %rax

    jz       L1

    # jump to OriginalHandler
    jmpq     *-0x10(%rsp)

L1:
    addq     $0x08, %rsp                  # skip error code for PF
    iretq
