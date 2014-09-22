#/** @file
#  
#    This code provides low level routines that support the Virtual Machine
#   for option ROMs.
#  
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
ASM_GLOBAL ASM_PFX(EbcInterpret);
ASM_GLOBAL ASM_PFX(ExecuteEbcImageEntryPoint);

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
      
      #
      # Fix X64 native function call prolog. Prepare space for at least 4 arguments,
      # even if the native function's arguments are less than 4.
      #
      # From MSDN x64 Software Conventions, Overview of x64 Calling Conventions:
      #   "The caller is responsible for allocating space for parameters to the
      #   callee, and must always allocate sufficient space for the 4 register
      #   parameters, even if the callee doesn't have that many parameters.
      #   This aids in the simplicity of supporting C unprototyped functions,
      #   and vararg C/C++ functions."
      #
      cmp    $0x20, %r8
      jae    skip_expansion
      mov    $0x20, %r8
skip_expansion:
      
      sub    %r8,  %rsp 
      
      #
      # Fix X64 native function call 16-byte alignment.
      #
      # From MSDN x64 Software Conventions, Stack Usage:
      #   "The stack will always be maintained 16-byte aligned, except within 
      #   the prolog (for example, after the return address is pushed)."
      #
      and    $0xFFFFFFFFFFFFFFF0, %rsp
      
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

ASM_GLOBAL ASM_PFX(EbcLLEbcInterpret);
ASM_PFX(EbcLLEbcInterpret):
    # save old parameter to stack
    mov  %rcx, 0x8(%rsp)
    mov  %rdx, 0x10(%rsp)
    mov  %r8, 0x18(%rsp)
    mov  %r9, 0x20(%rsp)

    # Construct new stack
    push %rbp
    mov  %rsp, %rbp
    push %rsi
    push %rdi
    push %rbx
    sub  $0x80, %rsp
    push %r10
    mov  %rbp, %rsi
    add  $0x10, %rsi
    mov  %rsp, %rdi
    add  $0x8, %rdi
    mov  $0x10, %rcx
    rep  movsq
    
    # build new paramater calling convention
    mov  0x18(%rsp), %r9
    mov  0x10(%rsp), %r8
    mov  0x8(%rsp), %rdx
    mov  %r10, %rcx

    # call C-code
    call ASM_PFX(EbcInterpret)
    add  $0x88, %esp
    pop  %rbx
    pop  %rdi
    pop  %rsi
    pop  %rbp
    ret

ASM_GLOBAL ASM_PFX(EbcLLExecuteEbcImageEntryPoint);
ASM_PFX(EbcLLExecuteEbcImageEntryPoint):
    # build new paramater calling convention
    mov  %rdx, %r8
    mov  %rcx, %rdx
    mov  %r10, %rcx

    # call C-code
    sub  $0x28, %rsp
    call ASM_PFX(ExecuteEbcImageEntryPoint)
    add  $0x28, %rsp
    ret
