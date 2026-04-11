#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# Portions copyright (c) 2011, Apple Inc. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Routine Description:
#
#   Routine for switching stacks with 2 parameters EFI ABI
#   Convert UNIX to EFI ABI
#
# Arguments:
#
#   (rdi) EntryPoint    - Entry point with new stack.
#   (rsi) Context1      - Parameter1 for entry point. (rcx)
#   (rdx) Context2      - Parameter2 for entry point. (rdx)
#   (rcx) NewStack      - The pointer to new stack.
#
# Returns:
#
#   None
#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(PeiSwitchStacks)
ASM_PFX(PeiSwitchStacks):
    pushq   $0            // tells gdb to stop unwinding frame
    movq    %rsp, %rbp

    movq    %rcx, %rsp    // update stack pointer

    movq    %rdi, %rax    // entry point to %rax
    movq    %rsi, %rcx    // Adjust Context1
                          // Context2 already in the rigth spot

    #
    # Reserve space for register parameters (rcx, rdx, r8 & r9) on the stack,
    # in case the callee wishes to spill them.
    #
    subq    $32, %rsp  // 32-byte shadow space plus alignment pad
    call    *%rax



