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
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmCortexA15.h>

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Check if Architectural Timer frequency is valid number (should not be 0)
  ASSERT (PcdGet32 (PcdArmArchTimerFreqInHz));
  ASSERT(ArmIsArchTimerImplemented () != 0);

  // Enable SWP instructions
  ArmEnableSWPInstruction ();

  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  // Note: System Counter frequency can only be set in Secure privileged mode,
  // if security extensions are implemented.
  ArmGenericTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));

  if (ArmIsMpCore()) {
    // Turn on SMP coherency
    ArmSetAuxCrBit (A15_FEATURE_SMP);
  }

}


VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
  /*// Make the SCU accessible in Non Secure world
  if (ArmPlatformIsPrimaryCore (MpId)) {
    ScuBase = ArmGetScuBaseAddress();

    // Allow NS access to SCU register
    MmioOr32 (ScuBase + A9_SCU_SACR_OFFSET, 0xf);
    // Allow NS access to Private Peripherals
    MmioOr32 (ScuBase + A9_SCU_SSACR_OFFSET, 0xfff);
  }*/
}
