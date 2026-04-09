/** @file
  Arm HyperVisor Call (HVC) Null Library.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmHvcLib.h>
#include <Library/DebugLib.h>

/**
  Trigger an HVC call

  HVC calls can take up to 8 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_HVC_ARGS structure are used
  for both input and output values.

  @param [in,out]  Args    Arguments for the HVC call.
**/
VOID
ArmCallHvc (
  IN OUT ARM_HVC_ARGS  *Args
  )
{
  ASSERT (FALSE);
  return;
}
