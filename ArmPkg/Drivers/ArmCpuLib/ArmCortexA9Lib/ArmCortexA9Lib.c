/** @file

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmCpuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmCortexA9.h>

VOID
ArmEnableScu (
  VOID
  )
{
  INTN          ScuBase;

  ScuBase = ArmGetScuBaseAddress();

  // Invalidate all: write -1 to SCU Invalidate All register
  MmioWrite32(ScuBase + A9_SCU_INVALL_OFFSET, 0xffffffff);
  // Enable SCU
  MmioWrite32(ScuBase + A9_SCU_CONTROL_OFFSET, 0x1);
}

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Enable SWP instructions
  ArmEnableSWPInstruction ();

  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  // If MPCore then Enable the SCU
  if (ArmIsMpCore()) {
    // Signals the Cortex-A9 processor is taking part in coherency
    ArmSetAuxCrBit (A9_FEATURE_SMP);

    ArmEnableScu ();
  }
}


VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
  INTN          ScuBase;

  // Make the SCU accessible in Non Secure world
  if (ArmPlatformIsPrimaryCore (MpId)) {
    ScuBase = ArmGetScuBaseAddress();

    // Allow NS access to SCU register
    MmioOr32 (ScuBase + A9_SCU_SACR_OFFSET, 0xf);
    // Allow NS access to Private Peripherals
    MmioOr32 (ScuBase + A9_SCU_SSACR_OFFSET, 0xfff);
  }
}
