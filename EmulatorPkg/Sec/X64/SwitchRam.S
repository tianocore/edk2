#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
# Portions copyright (c) 2011, Apple Inc. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------



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
  // Adjust callers %rbp to account for stack move
  subq    %rdx, %rbp     // Calc offset of %rbp in Temp Memory
  addq    %r8,  %rbp     // add in permanent base to offset

  pushq   %rbp           // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rdx          // Save TemporaryMemoryBase
  pushq   %r8           // Save PermanentMemoryBase
  pushq   %r9           // Save CopySize

  //
  // Copy all of temp RAM to permanent memory, including stack
  //
  // CopyMem (PermanentMemoryBase, TemporaryMemoryBase, CopySize);
  //          %rcx,                %rdx,                %r8
  movq    %r8,  %rcx    // Shift arguments
  movq    %r9,  %r8
  subq    $0x28, %rsp   // Allocate register spill area & 16-byte align stack
  call    ASM_PFX(CopyMem)
  // Temp mem stack now copied to permanent location. %esp still in temp memory
  addq    $0x28, %rsp

  popq    %r9           // CopySize (old stack)
  popq    %r8           // PermanentMemoryBase (old stack)
  popq    %rdx          // TemporaryMemoryBase (old stack)

  movq    %rsp, %rcx    // Move to new stack
  subq    %rdx, %rcx    // Calc offset of stack in Temp Memory
  addq    %r8,  %rcx    // Calc PermanentMemoryBase address
  movq    %rcx, %rsp    // Update stack
  // Stack now points to permanent memory

  // ZeroMem (TemporaryMemoryBase /* rcx */, CopySize /* rdx */);
  movq    %rdx, %rcx
  movq    %r9,  %rdx
  subq    $0x28, %rsp   // Allocate register spill area & 16-byte align stack
  call    ASM_PFX(ZeroMem)
  addq    $0x28, %rsp

  // This data comes off the NEW stack
  popq    %rbp
  ret


