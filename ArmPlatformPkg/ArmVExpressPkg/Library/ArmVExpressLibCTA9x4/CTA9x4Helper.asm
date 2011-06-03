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
#include <ArmPlatform.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc
  
  EXPORT  ArmPlatformIsMemoryInitialized
  EXPORT  ArmPlatformInitializeBootMemory
  IMPORT  InitializeSMC
  
  PRESERVE8
  AREA    CTA9x4Helper, CODE, READONLY

/**
  Called at the early stage of the Boot phase to know if the memory has already been initialized

  Running the code from the reset vector does not mean we start from cold boot. In some case, we
  can go through this code with the memory already initialized.
  Because this function is called at the early stage, the implementation must not use the stack.
  Its implementation must probably done in assembly to ensure this requirement.

  @return   Return the condition value into the 'Z' flag

**/
ArmPlatformIsMemoryInitialized
  // Check if the memory has been already mapped, if so skipped the memory initialization
  LoadConstantToReg (ARM_VE_SYS_CFGRW1_REG ,r0)
  ldr   r0, [r0, #0]
  
  // 0x40000000 = Value of Physical Configuration Switch SW[0]
  and   r0, r0, #0x40000000
  tst   r0, #0x40000000
  bx	lr
    
/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
ArmPlatformInitializeBootMemory
  mov   r5, lr
  // Initialize PL354 SMC
  LoadConstantToReg (ARM_VE_SMC_CTRL_BASE, r1)
  LoadConstantToReg (VRAM_MOTHERBOARD_BASE, r2)
  blx   InitializeSMC
  bx	r5
  
  END
