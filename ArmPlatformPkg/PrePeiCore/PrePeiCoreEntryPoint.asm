//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
//  
//  This program and the accompanying materials                          
//  are licensed and made available under the terms and conditions of the BSD License         
//  which accompanies this distribution.  The full text of the license may be found at        
//  http://opensource.org/licenses/bsd-license.php                                            
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
//
//

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc
  
  IMPORT  CEntryPoint
  EXPORT  _ModuleEntryPoint
  
  PRESERVE8
  AREA    PrePeiCoreEntryPoint, CODE, READONLY
  
StartupAddr        DCD      CEntryPoint

SCC_SYS_SW         EQU      0x0004

_ModuleEntryPoint
  // Identify CPU ID
  mrc   p15, 0, r0, c0, c0, 5
  and   r0, #0xf

_SetupStack
  // Setup Stack for the 4 CPU cores
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresNonSecStackBase) ,r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresNonSecStackSize) ,r2)
  
  mov     r3,r0         // r3 = core_id
  mul     r3,r3,r2      // r3 = core_id * stack_size = offset from the stack base
  add     r3,r3,r1      // r3 = stack_base + offset
  add     r3,r3,r2,LSR #1   // r3 = stack_offset + (stack_size/2)   <-- the top half is for the heap
  mov     sp, r3

  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  LoadConstantToReg (FixedPcdGet32(PcdEmbeddedFdBaseAddress), r2)
  add   r2, r2, #4
  ldr   r1, [r2]

  // move sec startup address into a data register
  // ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r2, StartupAddr

  // jump to SEC C code
  //    r0 = core_id
  //    r1 = pei_core_address
  blx   r2

  END
