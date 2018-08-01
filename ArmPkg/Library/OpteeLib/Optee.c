/** @file
  Api's to communicate with OP-TEE OS (Trusted OS based on ARM TrustZone) via
  secure monitor calls.

  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/OpteeLib.h>

#include <IndustryStandard/ArmStdSmc.h>

/**
  Check for OP-TEE presence.
**/
BOOLEAN
EFIAPI
IsOpteePresent (
  VOID
  )
{
  ARM_SMC_ARGS ArmSmcArgs;

  // Send a Trusted OS Calls UID command
  ArmSmcArgs.Arg0 = ARM_SMC_ID_TOS_UID;
  ArmCallSmc (&ArmSmcArgs);

  if ((ArmSmcArgs.Arg0 == OPTEE_OS_UID0) &&
      (ArmSmcArgs.Arg1 == OPTEE_OS_UID1) &&
      (ArmSmcArgs.Arg2 == OPTEE_OS_UID2) &&
      (ArmSmcArgs.Arg3 == OPTEE_OS_UID3)) {
    return TRUE;
  } else {
    return FALSE;
  }
}
