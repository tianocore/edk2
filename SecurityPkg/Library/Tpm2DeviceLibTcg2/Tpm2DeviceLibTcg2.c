/** @file
  This library is TPM2 TCG2 protocol lib.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/Tpm20.h>

EFI_TCG2_PROTOCOL  *mTcg2Protocol = NULL;

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
  IN UINT32            InputParameterBlockSize,
  IN UINT8             *InputParameterBlock,
  IN OUT UINT32        *OutputParameterBlockSize,
  IN UINT8             *OutputParameterBlock
  )
{
  EFI_STATUS                Status;
  TPM2_RESPONSE_HEADER      *Header;

  if (mTcg2Protocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &mTcg2Protocol);
    if (EFI_ERROR (Status)) {
      //
      // Tcg2 protocol is not installed. So, TPM2 is not present.
      //
      DEBUG ((EFI_D_ERROR, "Tpm2SubmitCommand - Tcg2 - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }
  //
  // Assume when Tcg2 Protocol is ready, RequestUseTpm already done.
  //
  Status = mTcg2Protocol->SubmitCommand (
                            mTcg2Protocol,
                            InputParameterBlockSize,
                            InputParameterBlock,
                            *OutputParameterBlockSize,
                            OutputParameterBlock
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Header = (TPM2_RESPONSE_HEADER *)OutputParameterBlock;
  *OutputParameterBlockSize = SwapBytes32 (Header->paramSize);

  return EFI_SUCCESS;
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  EFI_STATUS   Status;

  if (mTcg2Protocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &mTcg2Protocol);
    if (EFI_ERROR (Status)) {
      //
      // Tcg2 protocol is not installed. So, TPM2 is not present.
      //
      DEBUG ((EFI_D_ERROR, "Tpm2RequestUseTpm - Tcg2 - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }
  //
  // Assume when Tcg2 Protocol is ready, RequestUseTpm already done.
  //
  return EFI_SUCCESS;
}

/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE   *Tpm2Device
  )
{
  return EFI_UNSUPPORTED;
}
