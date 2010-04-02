#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2010, Intel Corporation
# All rights reserved. This program and the accompanying materials
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
    addq     $-0x20, %rsp
    call     ASM_PFX(PageFaultHandler)
    addq     $0x20, %rsp
    test     %rax, %rax
    popq     %r11
    popq     %r10
    popq     %r9
    popq     %r8
    popq     %rdx
    popq     %rcx
    popq     %rax                         # restore all volatile registers
    jnz      L1
    jmp      ASM_PFX(mOriginalHandler)
L1:
    addq     $0x08, %rsp                  # skip error code for PF
    iretq
