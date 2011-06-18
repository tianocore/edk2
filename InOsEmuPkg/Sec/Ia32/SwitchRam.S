#------------------------------------------------------------------------------
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http:#opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
 

//  EFI_STATUS
//  EFIAPI
//  SecTemporaryRamSupport (
//    IN CONST EFI_PEI_SERVICES   **PeiServices,         // %rcx
//    IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,   // %rdx
//    IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,   // %r8 
//    IN UINTN                    CopySize               // %r9
//    )
//
ASM_GLOBAL ASM_PFX(SecTemporaryRamSupport)
ASM_PFX(SecTemporaryRamSupport):
//------------------------------------------------------------------------------
// VOID
// EFIAPI
// SecSwitchStack (
//   UINT32   TemporaryMemoryBase,
//   UINT32   PermenentMemoryBase
//   )//
//------------------------------------------------------------------------------    

    //
    // Save three register: eax, ebx, ecx
    //
    push  %eax
    push  %ebx
    push  %ecx
    push  %edx

#if 0    
// Port me to GAS syntax
    //
    // !!CAUTION!! this function addresss is pushed into stack after
    // migration of whole temporary memory, so need save it to permenent
    // memory at first!
    //
    
    mov   ebx, [esp + 20]          // Save the first parameter
    mov   ecx, [esp + 24]          // Save the second parameter
    
    //
    // Save this functions return address into permenent memory at first.
    // Then, Fixup the esp point to permenent memory
    //
    mov   eax, esp
    sub   eax, ebx
    add   eax, ecx
    mov   edx, dword ptr [esp]         // copy pushed registers value to permenent memory
    mov   dword ptr [eax], edx    
    mov   edx, dword ptr [esp + 4]
    mov   dword ptr [eax + 4], edx    
    mov   edx, dword ptr [esp + 8]
    mov   dword ptr [eax + 8], edx    
    mov   edx, dword ptr [esp + 12]
    mov   dword ptr [eax + 12], edx    
    mov   edx, dword ptr [esp + 16]    // Update this functions return address into permenent memory
    mov   dword ptr [eax + 16], edx    
    mov   esp, eax                     // From now, esp is pointed to permenent memory
        
    //
    // Fixup the ebp point to permenent memory
    //
    mov   eax, ebp
    sub   eax, ebx
    add   eax, ecx
    mov   ebp, eax                // From now, ebp is pointed to permenent memory
    
    //
    // Fixup callees ebp point for PeiDispatch
    //
    mov   eax, dword ptr [ebp]
    sub   eax, ebx
    add   eax, ecx
    mov   dword ptr [ebp], eax    // From now, Temporarys PPI callers stack is in permenent memory
#endif
    pop   %edx
    pop   %ecx
    pop   %ebx
    pop   %eax
    ret
