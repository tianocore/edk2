/** @file

  Copyright (c) 2011-2013, ARM Limited. All rights reserved.

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
#include <Library/ArmArchTimerLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmCortexA5x.h>

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Check if Architectural Timer frequency is valid number (should not be 0)
  ASSERT (PcdGet32 (PcdArmArchTimerFreqInHz));
  ASSERT (ArmIsArchTimerImplemented () != 0);

  // Note: System Counter frequency can only be set in Secure privileged mode,
  // if security extensions are implemented.
  ArmArchTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));

  if (ArmIsMpCore ()) {
    // Turn on SMP coherency
    ArmSetAuxCrBit (A5X_FEATURE_SMP);
  }

}

VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
}
