//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <ArmEb/ArmEb.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc
  
  IMPORT  CEntryPoint
  EXPORT  _ModuleEntryPoint
        
  PRESERVE8
  AREA    ModuleEntryPoint, CODE, READONLY
  

StartupAddr        DCD      CEntryPoint

_ModuleEntryPoint
 
  // Turn off remapping NOR to 0. We can now use DRAM in low memory
  // CAN'T DO THIS HERE -- BRANCH FROM RESET VECTOR IS RELATIVE AND REMAINS IN REMAPPED NOR
  //MmioOr32 (0x10001000 ,BIT8) //EB_SP810_CTRL_BASE

  // Enable NEON register in case folks want to use them for optimizations (CopyMem)
  mrc     p15, 0, r0, c1, c0, 2
  orr     r0, r0, #0x00f00000   // Enable VFP access (V* instructions)
  mcr     p15, 0, r0, c1, c0, 2
  mov     r0, #0x40000000       // Set EN bit in FPEXC
  mcr     p10,#0x7,r0,c8,c0,#0  // msr     FPEXC,r0 in ARM assembly
   
  // Set CPU vectors to 0 (which is currently flash)
  LoadConstantToReg (FixedPcdGet32(PcdCpuVectorBaseAddress) ,r0) // Get vector base
  mcr     p15, 0, r0, c12, c0, 0
  isb                               // Sync changes to control registers

  //
  // Set stack based on PCD values. Need to do it this way to make C code work 
  // when it runs from FLASH. 
  //  
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackBase) ,r0)    // temp ram base  arg 0 TODO: change "stackbase" to "temprambase"
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackSize) ,r1)    // temp ram size  arg 1
  lsr     r3, r1, #1   // r4 = size of temp mem / 2
  add     r3, r3, r0  // r2 = temp ram base + r4
  mov     r13, r3     // result: stack pointer = temp ram base + (size of temp mem / 2)
    
  // lr points to area in reset vector block containing PEI core address
  ldr     r2, [lr]  // pei core   arg 3
 
  // move sec startup address into a data register
  // ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r4, StartupAddr

  // jump to SEC C code
  blx   r4
  
  // Call C entry point
  // THIS DOESN'T WORK, WE NEED A LONG JUMP
  // blx     CEntryPoint 

ShouldNeverGetHere
  // _CEntryPoint should never return 
  b       ShouldNeverGetHere
  
  END


