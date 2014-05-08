//
//  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <AsmMacroIoLibV8.h>
#include <Base.h>
#include <Library/ArmPlatformLib.h>
#include <AutoGen.h>
#include <ArmPlatform.h>

.text
.align 3

GCC_ASM_EXPORT(ArmPlatformSecBootAction)
GCC_ASM_EXPORT(ArmPlatformSecBootMemoryInit)
GCC_ASM_EXPORT(ArmSecMpCoreSecondariesWrite)
GCC_ASM_EXPORT(ArmSecMpCoreSecondariesRead)

/**
  Call at the beginning of the platform boot up

  This function allows the firmware platform to do extra actions at the early
  stage of the platform power up.

  Note: This function must be implemented in assembler as there is no stack set up yet

**/
ASM_PFX(ArmPlatformSecBootAction):
  ret

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the stack has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
ASM_PFX(ArmPlatformSecBootMemoryInit):
  // The SMC does not need to be initialized for RTSM
  ret


// NOTE:
// The foundation model does not have the VE_SYS_REGS like all the other VE
// platforms. We pick a spot in RAM that *should* be safe in the simple case
// of no UEFI apps interfering (Only the Linux loader getting used). By the
// time we come to load Linux we should have all the cores in a safe place.
// The image expects to be loaded at 0xa0000000. We also place the mailboxes
// here as it does not matter if we corrupt the image at this time.
// NOTE also see: "ArmVExpressLibRTSM/RTSMFoundation.c"

/* Write the flag register used to start Secondary cores */
ASM_PFX(ArmSecMpCoreSecondariesWrite):
  ldr x1, =0xa0000000
  str w0, [x1]
  ret


/* Read the flag register used to start Secondary cores */
ASM_PFX(ArmSecMpCoreSecondariesRead):
  ldr x1, =0xa0000000
  ldr w0, [x1]
  ret
