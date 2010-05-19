#------------------------------------------------------------------------------ 
#
# ARM EB Entry point. Reset vector in FV header will brach to
# _ModuleEntryPoint. 
#
# We use crazy macros, like LoadConstantToReg, since Xcode assembler 
# does not support = assembly syntax for ldr.
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <ArmEb/ArmEb.h>

.text
.align 3

.globl ASM_PFX(CEntryPoint)
.globl ASM_PFX(_ModuleEntryPoint)

ASM_PFX(_ModuleEntryPoint):
 
  // Turn off remapping NOR to 0. We can now use DRAM in low memory
  MmioOr32 (EB_SP810_CTRL_BASE ,BIT8) 

  // Enable NEON register in case folks want to use them for optimizations (CopyMem)
  mrc     p15, 0, r0, c1, c0, 2
  orr     r0, r0, #0x00f00000   // Enable VPF access (V* instructions)
  mcr     p15, 0, r0, c1, c0, 2
  mov     r0, #0x40000000       // Set EN bit in FPEXC
  msr     FPEXC,r0
  
   
  // Set CPU vectors to start of DRAM
  LoadConstantToReg (FixedPcdGet32(PcdCpuVectorBaseAddress) ,r0) // Get vector base
  mcr     p15, 0, r0, c12, c0, 0
  isb                               // Sync changes to control registers

  // Fill vector table with branchs to current pc (jmp $)
  // CPU DXE driver likes known values so it can let GDB stub hook vectors
  ldr     r1, ShouldNeverGetHere
  movs    r2, #0
FillVectors:
  str     r1, [r0, r2]
  adds    r2, r2, #4
  cmp     r2, #32
  bne     FillVectors

  //
  // Set stack based on PCD values. Need to do it this way to make C code work 
  // when it runs from FLASH. 
  //  
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackBase) ,r2)    // stack base arg2  
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackSize) ,r3)    // stack size arg3  
  add     r4, r2, r3
  mov     r13, r4

  // Call C entry point
  LoadConstantToReg (FixedPcdGet32(PcdMemorySize) ,r1)    // memory size arg1      
  LoadConstantToReg (FixedPcdGet32(PcdMemoryBase) ,r0)    // memory size arg0       
  blx      ASM_PFX(CEntryPoint) 

ShouldNeverGetHere:
  // _CEntryPoint should never return 
  b       ShouldNeverGetHere

 
