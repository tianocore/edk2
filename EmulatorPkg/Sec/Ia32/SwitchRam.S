#------------------------------------------------------------------------------
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   Stack.asm
#
# Abstract:
#
#   Switch the stack from temporary memory to permenent memory.
#
#------------------------------------------------------------------------------

 .text


//------------------------------------------------------------------------------
// VOID
// EFIAPI
// SecSwitchStack (
//   UINT32   TemporaryMemoryBase,
//   UINT32   PermenentMemoryBase
//   )//
//------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SecSwitchStack)
ASM_PFX(SecSwitchStack):
#
# Save three register: eax, ebx, ecx
#
    push  %eax
    push  %ebx
    push  %ecx
    push  %edx

#
# !!CAUTION!! this function address's is pushed into stack after
# migration of whole temporary memory, so need save it to permenent
# memory at first!
#

    movl  20(%esp), %ebx            # Save the first parameter
    movl  24(%esp), %ecx            # Save the second parameter

#
# Save this function's return address into permenent memory at first.
# Then, Fixup the esp point to permenent memory
#

    movl  %esp, %eax
    subl  %ebx, %eax
    addl  %ecx, %eax
    movl  (%esp), %edx                 # copy pushed register's value to permenent memory
    movl  %edx, (%eax)
    movl  4(%esp), %edx
    movl  %edx, 4(%eax)
    movl  8(%esp), %edx
    movl  %edx, 8(%eax)
    movl  12(%esp), %edx
    movl  %edx, 12(%eax)
    movl  16(%esp), %edx
    movl  %edx, 16(%eax)
    movl  %eax, %esp                   # From now, esp is pointed to permenent memory

#
# Fixup the ebp point to permenent memory
#
#ifndef __APPLE__
    movl   %ebp, %eax
    subl   %ebx, %eax
    addl   %ecx, %eax
    movl   %eax, %ebp                  # From now, ebp is pointed to permenent memory

#
# Fixup callee's ebp point for PeiDispatch
#
    movl   (%ebp), %eax
    subl   %ebx, %eax
    addl   %ecx, %eax
    movl   %eax, (%ebp)                # From now, Temporary's PPI caller's stack is in permenent memory
#endif

    pop   %edx
    pop   %ecx
    pop   %ebx
    pop   %eax
    ret

