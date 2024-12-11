/** @file
  PTP (Platform TPM Profile) CRB (Command Response Buffer) interface shim that switches between
  SVSM vTPM Ptp and regular Ptp implementations.

  Use TryUseSvsmVTpm () do check for SVSM vTPM presnece and initialzie the shim.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/Tpm2DeviceLib.h>
#include "Tpm2Ptp.h"
#include "Tpm2Svsm.h"

static BOOLEAN  mUseSvsmVTpm = FALSE;

/**
 Test if SVSM vTPM is present and initialize the Ptp-Svsm-Shim.

 If an SVSM based vTPM is found, use it from now on.
 If none is found, call the regular Ptp TPM implementetion instead.

 This function is meant to be called from the DTpm library constructor.
 If it has not been called, the regular Ptp implementation is used.

 @retval TRUE   SVSM vTPM is present.
 @retval FALSE  SVSM vTPM was not discovered.
 */
BOOLEAN
EFIAPI
TryUseSvsmVTpm (
  )
{
  mUseSvsmVTpm = Tpm2SvsmQueryTpmSendCmd ();
  return mUseSvsmVTpm;
}

/**
  This service enables the sending of commands to the selected TPM2.

  Commands are send to either the SVSM vTPM or the regular Ptp based TPM, depending
  on what was discovered by TryUseSvsmVTpm ().

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
SvsmDTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  if (mUseSvsmVTpm) {
    return Tpm2SvsmTpmSendCommand (
             InputParameterBlock,
             InputParameterBlockSize,
             OutputParameterBlock,
             OutputParameterBlockSize
             );
  } else {
    return DTpm2SubmitCommand (
             InputParameterBlockSize,
             InputParameterBlock,
             OutputParameterBlockSize,
             OutputParameterBlock
             );
  }
}

/**
  This service requests use TPM2.

  Depending on what was discovered by TryUseSvsmVTpm (), this function either
  returns EFI_SUCCESS, for SVSM vTPM, or calls the regular Ptp implementation.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
SvsmDTpm2RequestUseTpm (
  VOID
  )
{
  if (mUseSvsmVTpm) {
    return EFI_SUCCESS;
  } else {
    return DTpm2RequestUseTpm ();
  }
}
