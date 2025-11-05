/** @file
  This library is TPM12 TCG protocol lib.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Protocol/TcgService.h>
#include <IndustryStandard/Tpm12.h>

EFI_TCG_PROTOCOL  *mTcgProtocol = NULL;

/**
  This service enables the sending of commands to the TPM12.

  @param[in]      InputParameterBlockSize  Size of the TPM12 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM12 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM12 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM12 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tpm12SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  EFI_STATUS           Status;
  TPM_RSP_COMMAND_HDR  *Header;

  if (mTcgProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&mTcgProtocol);
    if (EFI_ERROR (Status)) {
      //
      // TCG protocol is not installed. So, TPM12 is not present.
      //
      DEBUG ((DEBUG_ERROR, "Tpm12SubmitCommand - TCG - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }

  //
  // Assume when TCG Protocol is ready, RequestUseTpm already done.
  //
  Status = mTcgProtocol->PassThroughToTpm (
                           mTcgProtocol,
                           InputParameterBlockSize,
                           InputParameterBlock,
                           *OutputParameterBlockSize,
                           OutputParameterBlock
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Header                    = (TPM_RSP_COMMAND_HDR *)OutputParameterBlock;
  *OutputParameterBlockSize = SwapBytes32 (Header->paramSize);

  return EFI_SUCCESS;
}

/**
  This service requests use TPM12.

  @retval EFI_SUCCESS      Get the control of TPM12 chip.
  @retval EFI_NOT_FOUND    TPM12 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12RequestUseTpm (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mTcgProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&mTcgProtocol);
    if (EFI_ERROR (Status)) {
      //
      // TCG protocol is not installed. So, TPM12 is not present.
      //
      DEBUG ((DEBUG_ERROR, "Tpm12RequestUseTpm - TCG - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }

  //
  // Assume when TCG Protocol is ready, RequestUseTpm already done.
  //
  return EFI_SUCCESS;
}
