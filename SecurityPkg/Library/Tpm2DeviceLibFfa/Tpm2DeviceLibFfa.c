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
#include <IndustryStandard/Tpm20.h>

#include "Tpm2DeviceLibFfa.h"

/**
  Send a command to TPM for execution and return response data.
  Used during boot only.

  @retval EFI_SUCCESS     Command was successfully sent to the TPM
                          and the response was copied to the Output buffer.
  @retval Other           Some error occurred in communication with the TPM.
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  return FfaTpm2SubmitCommand (
           InputParameterBlockSize,
           InputParameterBlock,
           OutputParameterBlockSize,
           OutputParameterBlock
           );
}

/**
  This service requests use TPM2.
  Since every communication with the TPM is blocking
  you are always good to start communicating with the TPM.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  return FfaTpm2RequestUseTpm ();
}

/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE  *Tpm2Device
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check that we have an address for the CRB

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval EFI_NO_MAPPING   The TPM base address is not set up.
  @retval EFI_UNSUPPORTED  The TPM interface type is not supported.
**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibFfaConstructor (
  VOID
  )
{
  return InternalTpm2DeviceLibFfaConstructor ();
}
