/** @file
  This library provides an interfaces to access DynamicPcds used
  in Tpm2DeviceLibFfa.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Uefi/UefiBaseType.h>

#include "Tpm2DeviceLibFfa.h"

/**
 This function validate TPM interface type for TPM service over FF-A.

 @retval EFI_SUCCESS           TPM interface type is valid.

 @retval EFI_UNSUPPORTED       TPM interface type is invalid.

**/
EFI_STATUS
EFIAPI
ValidateTpmInterfaceType (
  VOID
  )
{
  TPM2_PTP_INTERFACE_TYPE  TpmInterfaceType;

  TpmInterfaceType = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
  if (TpmInterfaceType != Tpm2PtpInterfaceCrb) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Setting Tpm Active Interface Type %d\n", TpmInterfaceType));

  return EFI_SUCCESS;
}

/**
  This function is used to get the TPM service partition id.

  @param[out] PartitionId - Supplies the pointer to the TPM service partition id.

  @retval EFI_SUCCESS           The TPM command was successfully sent to the TPM
                                and the response was copied to the Output buffer.
  @retval EFI_INVALID_PARAMETER The TPM command buffer is NULL or the TPM command
                                buffer size is 0.
  @retval EFI_DEVICE_ERROR      An error occurred in communication with the TPM.
**/
EFI_STATUS
EFIAPI
GetTpmServicePartitionId (
  OUT UINT16  *PartitionId
  )
{
  return FfaTpm2GetServicePartitionId (PartitionId);
}
