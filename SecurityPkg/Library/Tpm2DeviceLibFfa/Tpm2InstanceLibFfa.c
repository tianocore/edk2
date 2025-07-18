/** @file
  This library provides an implementation of Tpm2DeviceLib
  using ARM64 SMC calls to request TPM service.

  The implementation is only supporting the Command Response Buffer (CRB)
  for sharing data with the TPM.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2DumpLib.h>
#include <IndustryStandard/Tpm20.h>
#include <Guid/Tpm2ServiceFfa.h>

#include "Tpm2DeviceLibFfa.h"

TPM2_DEVICE_INTERFACE  mFfaTpm2InternalTpm2Device = {
  TPM2_SERVICE_FFA_GUID,
  FfaTpm2SubmitCommand,
  FfaTpm2RequestUseTpm,
};

/**
  Check that we have an address for the CRB

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval EFI_NOT_STARTED  The TPM base address is not set up.
  @retval EFI_UNSUPPORTED  The TPM interface type is not supported.
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibFfaConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = Tpm2RegisterTpm2DeviceLib (&mFfaTpm2InternalTpm2Device);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    if (Status == EFI_SUCCESS) {
      Status = InternalTpm2DeviceLibFfaConstructor ();
      DumpPtpInfo ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress), Tpm2PtpInterfaceCrb);
    }

    return Status;
  }

  return Status;
}
