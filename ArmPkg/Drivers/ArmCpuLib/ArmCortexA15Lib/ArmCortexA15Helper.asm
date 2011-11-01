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
#include <Library/ArmCpuLib.h>
#include <Library/ArmGicLib.h>
#include <Library/PcdLib.h>
#include <Chipset/ArmV7.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmCpuSynchronizeWait
  IMPORT  CArmCpuSynchronizeWait
  // Dirty hack to get the Fixed value of GicDistributorBase
  IMPORT  _gPcd_FixedAtBuild_PcdGicDistributorBase

  PRESERVE8
  AREA    ArmCortexA15Helper, CODE, READONLY

// VOID
// ArmCpuSynchronizeWait (
//   IN ARM_CPU_SYNCHRONIZE_EVENT   Event
//   );
ArmCpuSynchronizeWait
  cmp   r0, #ARM_CPU_EVENT_BOOT_MEM_INIT
  // The SCU enabled is the event to tell us the Init Boot Memory is initialized
  beq   ArmWaitGicDistributorEnabled
  // Case when the stack has been set up
  push	{r1,lr}
  LoadConstantToReg (CArmCpuSynchronizeWait, r1)
  blx   r1
  pop	{r1,lr}
  bx	lr


// IN  None
ArmWaitGicDistributorEnabled
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdGicDistributorBase, r0)
  ldr   r0, [r0]
_WaitGicDistributor
  ldr   r1, [r0, #ARM_GIC_ICDDCR]
  cmp   r1, #1
  bne   _WaitGicDistributor
  bx    lr

  END
