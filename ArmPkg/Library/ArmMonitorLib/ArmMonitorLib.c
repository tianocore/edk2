/** @file
  Arm Monitor Library.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmHvcLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/PcdLib.h>

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the default conduit. PcdMonitorConduitHvc determines the type
  of the call: if true, do an HVC.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
VOID
EFIAPI
ArmMonitorCall (
  IN OUT ARM_MONITOR_ARGS  *Args
  )
{
  if (FeaturePcdGet (PcdMonitorConduitHvc)) {
    ArmCallHvc ((ARM_HVC_ARGS *)Args);
  } else {
    ArmCallSmc ((ARM_SMC_ARGS *)Args);
  }
}
