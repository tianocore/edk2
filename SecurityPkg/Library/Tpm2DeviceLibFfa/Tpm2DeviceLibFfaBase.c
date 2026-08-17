/** @file
  This library provides an implementation of Tpm2DeviceLib
  using ARM64 SMC calls to request TPM service.

  The implementation is only supporting the Command Response Buffer (CRB)
  for sharing data with the TPM.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/Tpm2DeviceLib.h>
#include "Tpm2DeviceLibFfa.h"

UINT8  mCRBIdleByPass;

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

  mCRBIdleByPass = 0xFF;

  if (PcdGet64 (PcdTpmBaseAddress) == 0) {
    return EFI_NO_MAPPING;
  }

  Status = ValidateTpmInterfaceType ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mCRBIdleByPass = Tpm2GetIdleByPass ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));

  return EFI_SUCCESS;
}
