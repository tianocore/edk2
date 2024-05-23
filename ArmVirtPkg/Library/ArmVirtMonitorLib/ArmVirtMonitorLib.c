/** @file
  Arm Monitor Library.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/ArmHvcLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

/**
  An enum representing the PSCI conduits for issuing monitor calls.
*/
typedef enum PsciConduit {
  PsciConduitHvc,   // < HVC conduit
  PsciConduitSmc,   // < SMC conduit
  PsciConduitMax
} PSCI_CONDUIT;

/**
  A variable that stores the PSCI conduit to be used.
*/
STATIC PSCI_CONDUIT  mArmPsciConduit = PsciConduitMax;

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the conduit. The library constructor for ArmVirtMonitorLib
  determines the conduit by parsing the Device Tree handed off by the VMM
  and initialising mArmPsciConduit.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
VOID
EFIAPI
ArmMonitorCall (
  IN OUT ARM_MONITOR_ARGS  *Args
  )
{
  switch (mArmPsciConduit) {
    case PsciConduitHvc:
      ArmCallHvc ((ARM_HVC_ARGS *)Args);
      break;
    case PsciConduitSmc:
      ArmCallSmc ((ARM_SMC_ARGS *)Args);
      break;
    default:
      ASSERT (0);
      CpuDeadLoop ();
  }
}

/** Constructor for ArmVirtMonitorLib.

  The library constructor for ArmVirtMonitorLib determines the conduit
  by parsing the Device Tree handed off by the VMM and initialising
  mArmPsciConduit, which can then be used to select the appropriate
  conduit for invoking the monitor call.

  @retval RETURN_SUCCESS    The constructor always returns RETURN_SUCCESS.
  @retval RETURN_NOT_FOUND  An entry for the PSCI conduit was not found in
                            the platform device tree.
**/
RETURN_STATUS
EFIAPI
ArmVirtMonitorLibConstructor (
  VOID
  )
{
  RETURN_STATUS        Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST VOID           *Prop;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = FdtClient->FindCompatibleNodeProperty (
                        FdtClient,
                        "arm,psci-0.2",
                        "method",
                        &Prop,
                        NULL
                        );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (AsciiStrnCmp (Prop, "hvc", 3) == 0) {
    mArmPsciConduit = PsciConduitHvc;
  } else if (AsciiStrnCmp (Prop, "smc", 3) == 0) {
    mArmPsciConduit = PsciConduitSmc;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Unknown PSCI method \"%a\"\n",
      __func__,
      Prop
      ));
    return RETURN_NOT_FOUND;
  }

  return RETURN_SUCCESS;
}
