#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
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
#   Routine for switching stacks with 3 parameters EFI ABI
#
# Arguments:
#
#   (rcx) EntryPoint    - Entry point with new stack.
#   (rdx) Context1      - Parameter1 for entry point.
#   (r8)  Context2      - Parameter2 for entry point.
#   (r9)  Context3      - Parameter3 for entry point.
#   (rsp)0x20  NewStack - The pointer to new stack.
#
# Returns:
#
#   None
#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(MsftPeiSwitchStacks)
ASM_PFX(MsftPeiSwitchStacks):
    mov     %rcx, %rax
    mov     %rdx, %rcx
    mov     %r8, %rdx
    mov     %r9, %r8
    
    # get new stack from the stack
    mov     0x20(%rsp), %rsp  # is this off by 8?
    
    #
    # Reserve space for register parameters (rcx, rdx, r8 & r9) on the stack,
    # in case the callee wishes to spill them.
    #
    lea     -0x20(%rsp), %rsp
    call    *%rax


#------------------------------------------------------------------------------
# Routine Description:
#
#   Routine for switching stacks with 3 parameters UNIX ABI
#
# Arguments:
#
#   (rdi) EntryPoint    - Entry point with new stack.
#   (rsi) Context1      - Parameter1 for entry point.
#   (rdx) Context2      - Parameter2 for entry point.
#   (rcx) Context3      - Parameter3 for entry point.
#   (r8)  NewStack      - The pointer to new stack.
#
# Returns:
#
#   None
#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(PeiSwitchStacks)
ASM_PFX(PeiSwitchStacks):
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rdx, %rsi
    mov     %rcx, %rdx
    mov     %r8,  %rsp
    
    
    #
    # Reserve space for redzone on the stack,
    # in case the callee wishes to spill them.
    #
    lea     -0x80(%rsp), %rsp
    call    *%rax



#------------------------------------------------------------------------------
# VOID
# EFIAPI
# SecSwitchStack (
#   UINT32   TemporaryMemoryBase, // Rcx, Rdi
#   UINT32   PermenentMemoryBase  // Rdx, Rsi
#   );
#------------------------------------------------------------------------------    
ASM_GLOBAL ASM_PFX(SecSwitchStack)
ASM_PFX(SecSwitchStack):

  mov     %rsp, %rax
  sub     %rdi, %rax
  add     %rsi, %rax
  mov    (%rip), %r10
  mov    %r10, (%rax) 
  ret
  
  