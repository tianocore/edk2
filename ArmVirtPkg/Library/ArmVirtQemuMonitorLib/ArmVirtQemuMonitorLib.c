/** @file
  Arm Monitor Library that chooses the conduit based on the PSCI node in the
  device tree provided by QEMU

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmHvcLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

STATIC UINT32  mArmSmcccMethod;

/** Library constructor.

  Assign the global variable mArmSmcccMethod based on the PSCI node in the
  device tree.
**/
RETURN_STATUS
EFIAPI
ArmVirtQemuMonitorLibConstructor (
  VOID
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST VOID           *Prop;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeProperty (
                        FdtClient,
                        "arm,psci-0.2",
                        "method",
                        &Prop,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (AsciiStrnCmp (Prop, "hvc", 3) == 0) {
    mArmSmcccMethod = 1;
  } else if (AsciiStrnCmp (Prop, "smc", 3) == 0) {
    mArmSmcccMethod = 2;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Unknown SMCCC method \"%a\"\n",
      __func__,
      Prop
      ));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the default conduit.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
VOID
EFIAPI
ArmMonitorCall (
  IN OUT ARM_MONITOR_ARGS  *Args
  )
{
  if (mArmSmcccMethod == 1) {
    ArmCallHvc ((ARM_HVC_ARGS *)Args);
  } else if (mArmSmcccMethod == 2) {
    ArmCallSmc ((ARM_SMC_ARGS *)Args);
  } else {
    ASSERT ((mArmSmcccMethod == 1) || (mArmSmcccMethod == 2));
  }
}
