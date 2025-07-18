/** @file
  This library is a TPM2 DTPM instance, supporting SVSM based vTPMs and regular
  TPM2s at the same time.
  Choosing this library means platform uses and only uses DTPM device as TPM2 engine.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/Tpm2DeviceLib.h>

#include "Tpm2DeviceLibDTpm.h"
#include "Tpm2PtpSvsmShim.h"

/**
  This service enables the sending of commands to the TPM2.

  @param[in]      InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
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
  return SvsmDTpm2SubmitCommand (
           InputParameterBlockSize,
           InputParameterBlock,
           OutputParameterBlockSize,
           OutputParameterBlock
           );
}

/**
  This service requests to use TPM2.

  @retval EFI_SUCCESS      Get the control of the TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  return SvsmDTpm2RequestUseTpm ();
}

/**
  This service registers a TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          TPM2 device was registered successfully.
  @retval EFI_UNSUPPORTED      System does not support registering this TPM2 device.
  @retval EFI_ALREADY_STARTED  This TPM2 device is already registered.
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
  Initialize the library and cache SVSM vTPM presence state and TPM interface type, if applicable.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support registering a DTPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibConstructorSvsm (
  VOID
  )
{
  if (TryUseSvsmVTpm ()) {
    return EFI_SUCCESS;
  } else {
    return InternalTpm2DeviceLibDTpmCommonConstructor (NULL);
  }
}
