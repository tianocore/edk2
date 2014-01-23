#
# Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
# Module Name:
# 
#    ProcessorAsms.Asm
#
#
#
#include "EfiBind.h"
#------------------------------------------------------------------------------

  .686P: 
  .MMX: 
  #.MODEL SMALL
  .CODE: 
#------------------------------------------------------------------------------
  
.globl ASM_PFX(TransferControlSetJump)  
.globl ASM_PFX(TransferControlLongJump)
.globl ASM_PFX(SwitchStacks)
.globl ASM_PFX(SwitchIplStacks)

#define EFI_SUCCESS                     0
#define EFI_WARN_RETURN_FROM_LONG_JUMP  5

#
# typedef struct {
#  UINT32  ebx;
#  UINT32  esi;
#  UINT32  edi;
#  UINT32  ebp;
#  UINT32  esp;
#  UINT32  eip;
#} EFI_JUMP_BUFFER;
#

#typedef 
#EFI_STATUS
#(EFIAPI *EFI_PEI_TRANSFER_CONTROL_SET_JUMP) (
#  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
#  OUT EFI_JUMP_BUFFER                   *Context
#  );
#
#Routine Description:
#
#  This routine implements the IA32 variant of the SetJump call.  Its
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
ASM_PFX(TransferControlSetJump):
  push   %ebp
  mov    %esp,%ebp
  mov    0x8(%ebp),%eax
  mov    0xc(%ebp),%ecx
  mov    %ebx,(%ecx)
  mov    %esi,0x4(%ecx)
  mov    %edi,0x8(%ecx)
  mov    0x0(%ebp),%eax
  mov    %eax,0xc(%ecx)
  lea    0x4(%ebp),%eax
  mov    %eax,0x10(%ecx)
  mov    0x4(%ebp),%eax
  mov    %eax,0x14(%ecx)
  mov    $0x0,%eax
  leave  
  ret    

#typedef 
#EFI_STATUS
#(EFIAPI *EFI_PEI_TRANSFER_CONTROL_LONG_JUMP) (
#  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
#  IN EFI_JUMP_BUFFER                    *Context
#  );
#
# Routine Description:
# 
#  This routine implements the IA32 variant of the LongJump call.  Its
#  responsibility is restore the system state to the Context Buffer and
#  pass control back.
#
# Arguments:
# 
#  Pointer to CPU context save buffer.
#
# Returns:
#
#  EFI_WARN_RETURN_FROM_LONG_JUMP
#

ASM_PFX(TransferControlLongJump):
  push   %ebp
  mov    %esp,%ebp
  push   %ebx
  push   %esi
  push   %edi
  mov    0x8(%ebp),%eax
  mov    $0x5,%eax
  mov    0xc(%ebp),%ecx
  mov    (%ecx),%ebx
  mov    0x4(%ecx),%esi
  mov    0x8(%ecx),%edi
  mov    0xc(%ecx),%ebp
  mov    0x10(%ecx),%esp
  add    $0x4,%esp
  jmp    *0x14(%ecx)
  mov    $0x5,%eax
  pop    %edi
  pop    %esi
  pop    %ebx
  leave  
  ret    

#
# Routine Description:
#       This allows the caller to switch the stack and goes to the new entry point
#
# Arguments:
#       EntryPoint      - Pointer to the location to enter
#       Parameter       - Parameter to pass in
#       NewStack        - New Location of the stack
#       NewBsp          - New BSP
#
# Returns:
#
#       Nothing. Goes to the Entry Point passing in the new parameters
#
#SwitchStacks    PROC  C \
#  EntryPoint:PTR DWORD, \
#  Parameter:DWORD, \
#  NewStack:PTR DWORD, \
#  NewBsp:PTR DWORD
ASM_PFX(SwitchStacks):
  push   %ebp
  mov    %esp,%ebp
  push   %ebx
  mov    0x14(%ebp),%eax
  mov    0xc(%ebp),%ebx
  mov    0x8(%ebp),%ecx
  mov    0x10(%ebp),%eax
  mov    %eax,%esp
  push   %ebx
  push   $0x0
  jmp    *%ecx
  pop    %ebx
  leave  
  ret    




#
# Routine Description:
#       This allows the caller to switch the stack and goes to the new entry point
#
# Arguments:
#       EntryPoint              - Pointer to the location to enter
#       Parameter1/Parameter2   - Parameter to pass in
#       NewStack                - New Location of the stack
#       NewBsp                  - New BSP
#
# Returns:
#
#       Nothing. Goes to the Entry Point passing in the new parameters
#
#SwitchIplStacks PROC  C \
#  EntryPoint:PTR DWORD, \
#  Parameter1:DWORD, \
#  Parameter2:DWORD, \
#  NewStack:PTR DWORD, \
#  NewBsp:PTR DWORD
ASM_PFX(SwitchIplStacks):
  push   %ebp
  mov    %esp,%ebp
  push   %ebx
  mov    0x18(%ebp),%eax
  mov    0xc(%ebp),%ebx
  mov    0x10(%ebp),%edx
  mov    0x8(%ebp),%ecx
  mov    0x14(%ebp),%eax
  mov    %eax,%esp
  push   %edx
  push   %ebx
  call   *%ecx
  pop    %ebx
  leave  
  ret    

#SwitchIplStacks ENDP



