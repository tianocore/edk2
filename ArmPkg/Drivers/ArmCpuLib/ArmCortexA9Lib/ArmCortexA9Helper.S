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

.text
.align 3

GCC_ASM_EXPORT(ArmCpuSynchronizeWait)
GCC_ASM_EXPORT(ArmGetScuBaseAddress)
GCC_ASM_IMPORT(CArmCpuSynchronizeWait)

// VOID
// ArmCpuSynchronizeWait (
//   IN ARM_CPU_SYNCHRONIZE_EVENT   Event
//   );
ASM_PFX(ArmCpuSynchronizeWait):
  cmp   r0, #ARM_CPU_EVENT_BOOT_MEM_INIT
  // The SCU enabled is the event to tell us the Init Boot Memory is initialized
  beq   ASM_PFX(ArmWaitScuEnabled)
  // Case when the stack has been set up
  push	{r1,lr}
  LoadConstantToReg (ASM_PFX(CArmCpuSynchronizeWait), r1)
  blx   r1
  pop	{r1,lr}
  bx	lr

// IN None
// OUT r0 = SCU Base Address
ASM_PFX(ArmGetScuBaseAddress):
  // Read Configuration Base Address Register. ArmCBar cannot be called to get
  // the Configuration BAR as a stack is not necessary setup. The SCU is at the
  // offset 0x0000 from the Private Memory Region.
  mrc   p15, 4, r0, c15, c0, 0
  bx  lr

ASM_PFX(ArmWaitScuEnabled):
  // Read Configuration Base Address Register. ArmCBar cannot be called to get
  // the Configuration BAR as a stack is not necessary setup. The SCU is at the
  // offset 0x0000 from the Private Memory Region.
  mrc   p15, 4, r0, c15, c0, 0
  add   r0, r0, #A9_SCU_CONTROL_OFFSET
  ldr   r0, [r0]
  cmp   r0, #1
  bne   ASM_PFX(ArmWaitScuEnabled)
  bx    lr
