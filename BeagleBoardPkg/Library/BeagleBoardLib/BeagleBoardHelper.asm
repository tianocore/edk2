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
  
  EXPORT  ArmPlatformIsBootMemoryInitialized
  EXPORT  ArmPlatformInitializeBootMemory
  
  PRESERVE8
  AREA    BeagleBoardHelper, CODE, READONLY

/**
  Called at the early stage of the Boot phase to know if the memory has already been initialized

  Running the code from the reset vector does not mean we start from cold boot. In some case, we
  can go through this code with the memory already initialized.
  Because this function is called at the early stage, the implementation must not use the stack.
  Its implementation must probably done in assembly to ensure this requirement.

  @return   Return a non zero value if initialized

**/
ArmPlatformIsBootMemoryInitialized
  // The system memory is initialized by the BeagleBoard firmware
  mov   r0, #1
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
  // We must need to go into this function
  bx    lr
  
  END
