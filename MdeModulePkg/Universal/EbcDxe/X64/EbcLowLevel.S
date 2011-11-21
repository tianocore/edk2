#/** @file
#  
#    This code provides low level routines that support the Virtual Machine
#   for option ROMs.
#  
#  Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#  
#**/

#---------------------------------------------------------------------------
# Equate files needed.
#---------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(CopyMem);

#****************************************************************************
# EbcLLCALLEX
#
# This function is called to execute an EBC CALLEX instruction.
# This instruction requires that we thunk out to external native
# code. For x64, we switch stacks, copy the arguments to the stack
# and jump to the specified function.
# On return, we restore the stack pointer to its original location.
#
# Destroys no working registers.
#****************************************************************************
# VOID EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
ASM_GLOBAL ASM_PFX(EbcLLCALLEXNative);
ASM_PFX(EbcLLCALLEXNative):
      push   %rbp
      push   %rbx
      mov    %rsp, %rbp
      # Function prolog

      # Copy FuncAddr to a preserved register.
      mov    %rcx, %rbx

      # Set stack pointer to new value
      sub    %rdx, %r8  
      sub    %r8,  %rsp 
      mov    %rsp, %rcx
      sub    $0x20, %rsp 
      call   ASM_PFX(CopyMem)
      add    $0x20, %rsp

      # Considering the worst case, load 4 potiential arguments
      # into registers.
      mov    (%rsp), %rcx
      mov    0x8(%rsp), %rdx
      mov    0x10(%rsp), %r8
      mov    0x18(%rsp), %r9

      # Now call the external routine
      call  *%rbx

      # Function epilog
      mov      %rbp, %rsp
      pop      %rbx
      pop      %rbp
      ret


# UINTN EbcLLGetEbcEntryPoint(VOID);
# Routine Description:
#   The VM thunk code stuffs an EBC entry point into a processor
#   register. Since we can't use inline assembly to get it from
#   the interpreter C code, stuff it into the return value
#   register and return.
#
# Arguments:
#     None.
#
# Returns:
#     The contents of the register in which the entry point is passed.
#
ASM_GLOBAL ASM_PFX(EbcLLGetEbcEntryPoint);
ASM_PFX(EbcLLGetEbcEntryPoint):
    mov    %r10, %rax
    ret
