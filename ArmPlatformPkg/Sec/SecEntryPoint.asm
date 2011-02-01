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

#include <AutoGen.h>
#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <Library/ArmPlatformLib.h>

  INCLUDE AsmMacroIoLib.inc
  
  IMPORT  CEntryPoint
  IMPORT  ArmPlatformIsMemoryInitialized
  IMPORT  ArmPlatformInitializeBootMemory
  IMPORT  ArmDisableInterrupts
  IMPORT  ArmDisableCachesAndMmu
  IMPORT  ArmWriteVBar
  IMPORT  ArmReadMpidr
  IMPORT  SecVectorTable
  EXPORT  _ModuleEntryPoint

#if (FixedPcdGet32(PcdMPCoreSupport))
  IMPORT  ArmIsScuEnable
#endif
  
  PRESERVE8
  AREA    SecEntryPoint, CODE, READONLY
  
StartupAddr        DCD      CEntryPoint

_ModuleEntryPoint
  //Set VBAR to the start of the exception vectors in Secure Mode
  ldr   r0, =SecVectorTable
  blx   ArmWriteVBar

  // First ensure all interrupts are disabled
  blx   ArmDisableInterrupts

  // Ensure that the MMU and caches are off
  blx   ArmDisableCachesAndMmu

_IdentifyCpu 
  // Identify CPU ID
  bl    ArmReadMpidr
  and   r5, r0, #0xf
  
  //get ID of this CPU in Multicore system
  cmp   r5, #0
  // Only the primary core initialize the memory (SMC)
  beq   _InitMem
  
#if (FixedPcdGet32(PcdMPCoreSupport))
  // ... The secondary cores wait for SCU to be enabled
_WaitForEnabledScu
  bl    ArmIsScuEnable
  tst   r1, #1
  beq   _WaitForEnabledScu
  b     _SetupStack
#endif
  
_InitMem
  bl    ArmPlatformIsMemoryInitialized
  bne   _SetupStack
  
  // Initialize Init Memory
  bl    ArmPlatformInitializeBootMemory

  // Only Primary CPU could run this line (the secondary cores have jumped from _IdentifyCpu to _SetupStack)
  mov   r5, #0

_SetupStack
  // Setup Stack for the 4 CPU cores
  //Read Stack Base address from PCD
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r1)

  // Read Stack size from PCD
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecStackSize), r2)

  // Calcuate Stack Pointer reg value using Stack size and CPU ID.
  mov     r3,r5         // r3 = core_id
  mul     r3,r3,r2      // r3 = core_id * stack_size = offset from the stack base
  add     r3,r3,r1      // r3 = stack_base + offset
  mov     sp, r3
  
  // Move sec startup address into a data register
  // ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr
  
  // Jump to SEC C code
  //    r0 = core_id
  mov   r0, r5
  blx   r3
  
  END
