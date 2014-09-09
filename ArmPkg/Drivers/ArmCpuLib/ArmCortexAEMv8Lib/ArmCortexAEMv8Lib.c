/** @file

  Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/ArmCpuLib.h>
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmAemV8.h>

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Note: System Counter frequency can only be set in Secure privileged mode,
  // if security extensions are implemented.
  ArmGenericTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));
}


VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
  // Nothing to do
}
