#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
# Module Name:
#   ProcessorAsms.S
#
# Abstract:
#   This is separated from processor.c to allow this functions to be built with /O1
#
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

    .text

.globl ASM_PFX(SwitchStacks)
.globl ASM_PFX(TransferControlSetJump)
.globl ASM_PFX(TransferControlLongJump)

#
# Routine Description:
#   This allows the caller to switch the stack and goes to the new entry point
#
# Arguments:
#   EntryPoint      - Pointer to the location to enter // rcx
#   Parameter       - Parameter to pass in             // rdx
#   NewStack        - New Location of the stack        // r8
#   NewBsp          - New BSP                          // r9 - not used
#
# Returns:
#   Nothing. Goes to the Entry Point passing in the new parameters
#
ASM_PFX(SwitchStacks):

    # Adjust stack for
    #   1) leave 4 registers space
    #   2) let it 16 bytes aligned after call
    sub    $0x20,%r8
    and    -0x10,%r8w   # do not assume 16 bytes aligned

  	mov    %r8,%rsp
  	mov    %rcx,%r10
  	mov    %rdx,%rcx
  	callq  *%r10

 #
 # no ret as we have a new stack and we jumped to the new location
 #     
    ret

#SwitchStacks ENDP


.set                            EFI_SUCCESS, 0
.set                            EFI_WARN_RETURN_FROM_LONG_JUMP, 5

#
#Routine Description:
#
#  This routine implements the x64 variant of the SetJump call.  Its
#  responsibility is to store system state information for a possible
#  subsequent LongJump.
#
#Arguments:
#
#  Pointer to CPU context save buffer.
#
#Returns:
#
#  EFI_SUCCESS
#
# EFI_STATUS
# EFIAPI
# TransferControlLongJump (
#   IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
#   IN EFI_JUMP_BUFFER                    *Jump
#   );
#
# rcx - *This
# rdx - JumpBuffer
#
ASM_PFX(TransferControlSetJump):
      mov    %rbx,(%rdx)
      mov    %rsp,0x8(%rdx)
      mov    %rbp,0x10(%rdx)
      mov    %rdi,0x18(%rdx)
      mov    %rsi,0x20(%rdx)
      mov    %r10,0x28(%rdx)
      mov    %r11,0x30(%rdx)
      mov    %r12,0x38(%rdx)
      mov    %r13,0x40(%rdx)
      mov    %r14,0x48(%rdx)
      mov    %r15,0x50(%rdx)
      #; save non-volatile fp registers
      stmxcsr 0x60(%rdx)
      lea     0x68(%rdx), %rax
      movdqu  %xmm6, (%rax) 
      movdqu  %xmm7, 0x10(%rax)
      movdqu  %xmm8, 0x20(%rax)
      movdqu  %xmm9, 0x30(%rax)
      movdqu  %xmm10, 0x40(%rax)
      movdqu  %xmm11, 0x50(%rax)
      movdqu  %xmm12, 0x60(%rax)
      movdqu  %xmm13, 0x70(%rax)
      movdqu  %xmm14, 0x80(%rax)
      movdqu  %xmm15, 0x90(%rax)
      mov    (%rsp),%rax
      mov    %rax,0x58(%rdx)
      mov    $0x0,%rax
      retq   


#
# EFI_STATUS
# EFIAPI
# TransferControlLongJump (
#   IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,  // rcx
#   IN EFI_JUMP_BUFFER                    *Jump   // rdx
#   );
#
#
ASM_PFX(TransferControlLongJump):
      # set return from SetJump to EFI_WARN_RETURN_FROM_LONG_JUMP
      #; load non-volatile fp registers
      ldmxcsr 0x60(%rdx)
      lea     0x68(%rdx), %rax
      movdqu  (%rax), %xmm6
      movdqu  0x10(%rax), %xmm7
      movdqu  0x20(%rax), %xmm8
      movdqu  0x30(%rax), %xmm9
      movdqu  0x40(%rax), %xmm10
      movdqu  0x50(%rax), %xmm11
      movdqu  0x60(%rax), %xmm12
      movdqu  0x70(%rax), %xmm13
      movdqu  0x80(%rax), %xmm14
      movdqu  0x90(%rax), %xmm15
      mov    $0x5,%rax
      mov    (%rdx),%rbx
      mov    0x8(%rdx),%rsp
      mov    0x10(%rdx),%rbp
      mov    0x18(%rdx),%rdi
      mov    0x20(%rdx),%rsi
      mov    0x28(%rdx),%r10
      mov    0x30(%rdx),%r11
      mov    0x38(%rdx),%r12
      mov    0x40(%rdx),%r13
      mov    0x48(%rdx),%r14
      mov    0x50(%rdx),%r15
      add    $0x8,%rsp
      jmpq   *0x58(%rdx)
      mov    $0x5,%rax
      retq   

