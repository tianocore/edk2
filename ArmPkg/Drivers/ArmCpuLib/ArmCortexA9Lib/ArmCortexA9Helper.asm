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
#include <Chipset/ArmCortexA9.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmCpuSynchronizeWait
  EXPORT  ArmGetScuBaseAddress
  IMPORT  CArmCpuSynchronizeWait

  PRESERVE8
  AREA    ArmCortexA9Helper, CODE, READONLY

// VOID
// ArmCpuSynchronizeWait (
//   IN ARM_CPU_SYNCHRONIZE_EVENT   Event
//   );
ArmCpuSynchronizeWait
  cmp   r0, #ARM_CPU_EVENT_BOOT_MEM_INIT
  // The SCU enabled is the event to tell us the Init Boot Memory is initialized
  beq   ArmWaitScuEnabled
  // Case when the stack has been set up
  push	{r1,lr}
  LoadConstantToReg (CArmCpuSynchronizeWait, r1)
  blx   r1
  pop	{r1,lr}
  bx	lr

// IN None
// OUT r0 = SCU Base Address
ArmGetScuBaseAddress
  // Read Configuration Base Address Register. ArmCBar cannot be called to get
  // the Configuration BAR as a stack is not necessary setup. The SCU is at the
  // offset 0x0000 from the Private Memory Region.
  mrc   p15, 4, r0, c15, c0, 0
  bx  lr

ArmWaitScuEnabled
  // Read Configuration Base Address Register. ArmCBar cannot be called to get
  // the Configuration BAR as a stack is not necessary setup. The SCU is at the
  // offset 0x0000 from the Private Memory Region.
  mrc   p15, 4, r0, c15, c0, 0
  add   r0, r0, #A9_SCU_CONTROL_OFFSET
  ldr   r0, [r0]
  cmp   r0, #1
  bne   ArmWaitScuEnabled
  bx    lr

  END
