/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

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
#include <Library/DebugLib.h>

#include <Chipset/ArmV7.h>

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Enable SWP instructions
  ArmEnableSWPInstruction ();

  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();
}

VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
  // The CortexA8 is a Unicore CPU. We must not initialize SMP for Non Secure Accesses
  ASSERT(0);
}
