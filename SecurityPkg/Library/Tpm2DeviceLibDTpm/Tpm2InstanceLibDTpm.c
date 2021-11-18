/** @file
  This library is TPM2 DTPM instance.
  It can be registered to Tpm2 Device router, to be active TPM2 engine,
  based on platform setting.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>

#include <Guid/TpmInstance.h>

#include "Tpm2DeviceLibDTpm.h"

/**
  Dump PTP register information.

  @param[in] Register                Pointer to PTP register.
**/
VOID
DumpPtpInfo (
  IN VOID  *Register
  );

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
DTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  );

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
DTpm2RequestUseTpm (
  VOID
  );

TPM2_DEVICE_INTERFACE  mDTpm2InternalTpm2Device = {
  TPM_DEVICE_INTERFACE_TPM20_DTPM,
  DTpm2SubmitCommand,
  DTpm2RequestUseTpm,
};

/**
  The function register DTPM2.0 instance and caches current active TPM interface type.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support register DTPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibDTpmConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = Tpm2RegisterTpm2DeviceLib (&mDTpm2InternalTpm2Device);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    if (Status == EFI_SUCCESS) {
      Status = InternalTpm2DeviceLibDTpmCommonConstructor ();
      DumpPtpInfo ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    }

    return EFI_SUCCESS;
  }

  return Status;
}
