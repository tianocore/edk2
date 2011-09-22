#------------------------------------------------------------------------------ 
#
# ARM VE Entry point. Reset vector in FV header will brach to
# _ModuleEntryPoint. 
#
#  Copyright (c) 2011, ARM Limited. All rights reserved.
#  
#  This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <Library/ArmPlatformLib.h>
#include <AutoGen.h>

#Start of Code section
.text
.align 3

#make _ModuleEntryPoint as global
GCC_ASM_EXPORT(_ModuleEntryPoint)

#global functions referenced by this module
GCC_ASM_IMPORT(CEntryPoint)
GCC_ASM_IMPORT(ArmPlatformSecBootAction)
GCC_ASM_IMPORT(ArmPlatformIsMemoryInitialized)
GCC_ASM_IMPORT(ArmPlatformInitializeBootMemory)
GCC_ASM_IMPORT(ArmDisableInterrupts)
GCC_ASM_IMPORT(ArmDisableCachesAndMmu)
GCC_ASM_IMPORT(ArmWriteVBar)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(SecVectorTable)

#if (FixedPcdGet32(PcdMPCoreSupport))
GCC_ASM_IMPORT(ArmIsScuEnable)
#endif

StartupAddr:        .word   ASM_PFX(CEntryPoint)
SecVectorTableAddr: .word		ASM_PFX(SecVectorTable)

ASM_PFX(_ModuleEntryPoint):
  # First ensure all interrupts are disabled
  bl   ASM_PFX(ArmDisableInterrupts)

  # Ensure that the MMU and caches are off
  bl  ASM_PFX(ArmDisableCachesAndMmu)

  # Jump to Platform Specific Boot Action function
  blx   ASM_PFX(ArmPlatformSecBootAction)

  # Set VBAR to the start of the exception vectors in Secure Mode
  ldr   r0, =SecVectorTable
  bl    ASM_PFX(ArmWriteVBar)

_IdentifyCpu: 
  # Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Get ID of this CPU in Multicore system
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCoreMask), r1)
  and   r5, r0, r1
  
  #get ID of this CPU in Multicore system
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCore), r1)
  cmp   r5, r1
  # Only the primary core initialize the memory (SMC)
  beq   _InitMem
  
#if (FixedPcdGet32(PcdMPCoreSupport))
  # ... The secondary cores wait for SCU to be enabled
_WaitForEnabledScu:
  bl    ASM_PFX(ArmIsScuEnable)
  tst   r1, #1
  beq   _WaitForEnabledScu
  b     _SetupStack
#endif
  
_InitMem:
  bl    ASM_PFX(ArmPlatformIsMemoryInitialized)
  bne   _SetupStack
  
  # Initialize Init Memory
  bl    ASM_PFX(ArmPlatformInitializeBootMemory)

  # Only Primary CPU could run this line (the secondary cores have jumped from _IdentifyCpu to _SetupStack)
  mov   r5, #0
 
_SetupStack:
  # Setup Stack for the 4 CPU cores
  #Read Stack Base address from PCD
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r1)

  #read Stack size from PCD
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecStackSize), r2)

  #calcuate Stack Pointer reg value using Stack size and CPU ID.
  mov     r3,r5         @ r3 = core_id
  mul     r3,r3,r2      @ r3 = core_id * stack_size = offset from the stack base
  add     r3,r3,r1      @ r3 ldr= stack_base + offset
  mov     sp, r3
  
  # move sec startup address into a data register
  # ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr
  
  # Move the CoreId in r0 to be the first argument of the SEC Entry Point
  mov   r0, r5

  # jump to SEC C code
  #    r0 = core_id
  blx  r3


