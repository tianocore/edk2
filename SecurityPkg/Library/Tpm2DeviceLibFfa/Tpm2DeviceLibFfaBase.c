/** @file
  This library provides an implementation of Tpm2DeviceLib
  using ARM64 SMC calls to request TPM service.

  The implementation is only supporting the Command Response Buffer (CRB)
  for sharing data with the TPM.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/ArmFfaSvc.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/TimerLib.h>

#include "Tpm2DeviceLibFfa.h"

TPM2_PTP_INTERFACE_TYPE  mActiveTpmInterfaceType;
UINT8                    mCRBIdleByPass;

/**
  Return cached PTP CRB interface IdleByPass state.

  @return Cached PTP CRB interface IdleByPass state.
**/
UINT8
GetCachedIdleByPass (
  VOID
  )
{
  return mCRBIdleByPass;
}

/**
  Check that we have an address for the CRB

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval EFI_NO_MAPPING   The TPM base address is not set up.
  @retval EFI_UNSUPPORTED  The TPM interface type is not supported.
**/
EFI_STATUS
EFIAPI
InternalTpm2DeviceLibFfaConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  mActiveTpmInterfaceType = PcdGet8 (PcdActiveTpmInterfaceType);
  mCRBIdleByPass          = 0xFF;

  if (PcdGet64 (PcdTpmBaseAddress) == 0) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  //
  // Start by checking the PCD out of the gate and read from the CRB if it is invalid
  //
  if (mActiveTpmInterfaceType == 0xFF) {
    mActiveTpmInterfaceType = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    PcdSet8S (PcdActiveTpmInterfaceType, mActiveTpmInterfaceType);
  }

  if (mActiveTpmInterfaceType != Tpm2PtpInterfaceCrb) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "Setting Tpm Active Interface Type %d\n", mActiveTpmInterfaceType));
  mCRBIdleByPass = Tpm2GetIdleByPass ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));

  Status = EFI_SUCCESS;

Exit:
  return Status;
}
