#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# Portitions copyright (c) 2011, Apple Inc. All rights reserved.
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Routine Description:
#
#   Routine for switching stacks with 3 parameters EFI ABI
#   Convert UNIX to EFI ABI
#
# Arguments:
#
#   (rdi) EntryPoint    - Entry point with new stack.
#   (rsi) Context1      - Parameter1 for entry point. (rcx)
#   (rdx) Context2      - Parameter2 for entry point. (rdx) 
#   (rcx) Context3      - Parameter3 for entry point. (r8)
#   (r8)  NewStack      - The pointer to new stack.
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

    movq    %r8,  %rsp
    
    movq    %rdi, %rax
    movq    %rsi, %rcx
    movq    %rcx, %r8    
    
    #
    # Reserve space for register parameters (rcx, rdx, r8 & r9) on the stack,
    # in case the callee wishes to spill them.
    #
    subq    $32, %rsp  // 32-byte shadow space plus alignment pad
    call    *%rax

  

//  EFI_STATUS
//  EFIAPI
//  SecTemporaryRamSupport (
//    IN CONST EFI_PEI_SERVICES   **PeiServices,         // %rcx
//    IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,   // %rdx
//    IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,   // %r8 
//    IN UINTN                    CopySize               // %r9
//    )
//
ASM_GLOBAL ASM_PFX(GasketSecTemporaryRamSupport)
ASM_PFX(GasketSecTemporaryRamSupport):
  // Adjust callers %rbp to account for stack move
  subq    %rdx, %rbp     // Calc offset of %rbp in Temp Memory
  addq    %r8,  %rbp     // add in permanent base to offset
  
  pushq   %rbp           // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  pushq   %rdx          // Save TemporaryMemoryBase
  pushq   %r8           // Save PermanentMemoryBase
  pushq   %r9           // Save CopySize
  
  //
  // Copy all of temp RAM to permanent memory, including stack
  //
  // CopyMem (PermanentMemoryBase, TemporaryMemoryBase, CopySize);
  //          %rdi,                %rsi,                %rdx
  movq    %r8,  %rdi    // Swizzle args  
  movq    %rdx, %rsi
  movq    %r9,  %rdx
  call    ASM_PFX(CopyMem)
  // Temp mem stack now copied to permanent location. %esp still in temp memory
  
  popq    %r9           // CopySize (old stack)
  popq    %r8           // PermanentMemoryBase (old stack)
  popq    %rdx          // TemporaryMemoryBase (old stack)
    
  movq    %rsp, %rcx    // Move to new stack
  subq    %rdx, %rcx    // Calc offset of stack in Temp Memory
  addq    %r8,  %rcx    // Calc PermanentMemoryBase address
  movq    %rcx, %rsp    // Update stack 
  // Stack now points to permanent memory
  
  // ZeroMem (TemporaryMemoryBase /* rdi */, CopySize /* rsi */);
  movq    %rdx, %rdi
  movq    %r9,  %rsi
  call    ASM_PFX(ZeroMem)

  // This data comes off the NEW stack
  popq    %rdi   
  popq    %rsi
  popq    %rbp
  ret


