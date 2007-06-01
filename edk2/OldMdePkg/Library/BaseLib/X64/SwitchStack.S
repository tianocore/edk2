#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
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
#   SwitchStack.S
#
# Abstract:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Routine Description:
#
#   Routine for switching stacks with 1 parameter
#
# Arguments:
#
#   (rdi) EntryPoint    - Entry point with new stack.
#   (rsi) Context1      - Parameter1 for entry point.
#   (rdx)  Context2     - Parameter2 for entry point.
#   (rcx)  NewStack     - Pointer to new stack.
#
# Returns:
#
#   None
#
#------------------------------------------------------------------------------
.global _InternalSwitchStack;
_InternalSwitchStack:
    mov     %rcx, %rax
    mov     %rdx, %rcx
    mov     %r8, %rdx
    lea     -0x20(%r9), %rsp
    call    *%rax
