#/** @file
#  
#    This code provides low level routines that support the Virtual Machine
#   for option ROMs.
#  
#  Copyright (c) 2007 - 2008, Intel Corporation. <BR>
#  All rights reserved. This program and the accompanying materials
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

#---------------------------------------------------------------------------
##GenericPostSegment      SEGMENT USE16
#---------------------------------------------------------------------------

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
.global _CopyMem;

# VOID EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
.global _EbcLLCALLEXNative;
_EbcLLCALLEXNative:
      push   %rbp
      push   %rbx
      mov    %rsp, %rbp
      # Function prolog

      # Copy FuncAddr to a preserved register.
      mov    %rcx, %rbx

      # Set stack pointer to new value
      sub    %r8,  %rdx
      sub    %rsp, %r8
      mov    %rsp, %rcx
      sub    %rsp, 0x20
      call   _CopyMem
      add    %rsp, 0x20

      # Considering the worst case, load 4 potiential arguments
      # into registers.
      mov    (%rsp), %rcx
      mov    8(%rsp), %rdx
      mov    10(%rsp), %r8
      mov    18(%rsp), %r9

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
.global _EbcLLGetEbcEntryPoint;
_EbcLLGetEbcEntryPoint:
    ret

#/*++
#
#Routine Description:
#
#  Return the caller's value of the stack pointer.
#
#Arguments:
#
#  None.
#
#Returns:
#
#  The current value of the stack pointer for the caller. We
#  adjust it by 4 here because when they called us, the return address
#  is put on the stack, thereby lowering it by 4 bytes.
#
#--*/

# UINTN EbcLLGetStackPointer()
.global _EbcLLGetStackPointer;
_EbcLLGetStackPointer:
    mov    %rsp, %rax
    # Stack adjusted by this much when we were called,
    # For this function, it's 4.
    add   $4, %rax
    ret

.global _EbcLLGetReturnValue;
_EbcLLGetReturnValue:
# UINT64 EbcLLGetReturnValue(VOID);
# Routine Description:
#   When EBC calls native, on return the VM has to stuff the return
#   value into a VM register. It's assumed here that the value is still
#    in the register, so simply return and the caller should get the
#   return result properly.
#
# Arguments:
#     None.
#
# Returns:
#     The unmodified value returned by the native code.
#
    ret
