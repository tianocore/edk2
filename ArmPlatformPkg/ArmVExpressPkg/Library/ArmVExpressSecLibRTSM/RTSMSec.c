/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Drivers/PL310L2Cache.h>
#include <Drivers/SP804Timer.h>

#include <ArmPlatform.h>

// Initialize GICv3 to expose it as a GICv2 as UEFI does not support GICv3 yet
VOID
InitializeGicV3 (
  VOID
  );

/**
  Initialize the Secure peripherals and memory regions

  If Trustzone is supported by your platform then this function makes the required initialization
  of the secure peripherals and memory regions.

**/
VOID
ArmPlatformSecTrustzoneInit (
  IN  UINTN                     MpId
  )
{
  // Initialize TSC-400 to open all DRAM below 4G to nonsecure world
  
  // configure security errors to be bus errors (data/prefetch aborts);
  MmioWrite32(ARM_VE_TZC400_BASE + 0x004, 0x01);

  // enable gate keepers for all four filter enables
  MmioWrite32(ARM_VE_TZC400_BASE + 0x008, BIT3 | BIT2 | BIT1 | BIT0);

  // enable secure reads and writes to region 0 - s_wr_en, s_rd_en
  MmioOr32(ARM_VE_TZC400_BASE + 0x110, BIT31 | BIT30);
  
  // enable all IDs to do non-secure read and writes
  MmioWrite32(ARM_VE_TZC400_BASE + 0x114, 0xFFFFFFFF);
}

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example, some L2x0 requires to be initialized in Secure World

**/
RETURN_STATUS
ArmPlatformSecInitialize (
  IN  UINTN                     MpId
  )
{
  UINT32  Identification;

  // If it is not the primary core then there is nothing to do
  if (!ArmPlatformIsPrimaryCore (MpId)) {
    return RETURN_SUCCESS;
  }

  // Configure periodic timer (TIMER0) for 1MHz operation
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER0_TIMCLK);
  // Configure 1MHz clock
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER1_TIMCLK);
  // Configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER2_EN, SP810_SYS_CTRL_TIMER2_TIMCLK);
  // Configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER3_EN, SP810_SYS_CTRL_TIMER3_TIMCLK);

  // Read the GIC Identification Register
  Identification = ArmGicGetInterfaceIdentification (PcdGet32 (PcdGicInterruptInterfaceBase));

  // Check if we are GICv3
  if (ARM_GIC_ICCIIDR_GET_ARCH_VERSION(Identification) >= 0x3) {
    InitializeGicV3 ();
  }

  return RETURN_SUCCESS;
}

/**
  Call before jumping to Normal World

  This function allows the firmware platform to do extra actions before
  jumping to the Normal World

**/
VOID
ArmPlatformSecExtraAction (
  IN  UINTN         MpId,
  OUT UINTN*        JumpAddress
  )
{
  *JumpAddress = PcdGet64 (PcdFvBaseAddress);
}
