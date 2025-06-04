/** @file
  This library provides an interfaces to access DynamicPcds used
  in Tpm2DeviceLibFfa.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Uefi/UefiBaseType.h>

#include "Tpm2DeviceLibFfa.h"

TPM2_PTP_INTERFACE_TYPE  mActiveTpmInterfaceType;

/**
 This function validate TPM interface type for TPM service over FF-A.

 @retval EFI_SUCCESS           TPM interface type is valid.

 @retval EFI_UNSUPPORTED       TPM interface type is invalid.

**/
EFI_STATUS
ValidateTpmInterfaceType (
  VOID
  )
{
  mActiveTpmInterfaceType = PcdGet8 (PcdActiveTpmInterfaceType);

  //
  // Start by checking the PCD out of the gate and read from the CRB if it is invalid
  //
  if (mActiveTpmInterfaceType == TPM2_FFA_INTERFACE_TYPE_UNKNOWN) {
    mActiveTpmInterfaceType = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    PcdSet8S (PcdActiveTpmInterfaceType, mActiveTpmInterfaceType);
  }

  if (mActiveTpmInterfaceType != Tpm2PtpInterfaceCrb) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Setting Tpm Active Interface Type %d\n", mActiveTpmInterfaceType));

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
  EFI_STATUS  Status;

  if (PartitionId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PcdGet16 (PcdTpmServiceFfaPartitionId) != TPM2_FFA_PARTITION_ID_INVALID) {
    *PartitionId = PcdGet16 (PcdTpmServiceFfaPartitionId);
    return EFI_SUCCESS;
  }

  Status = FfaTpm2GetServicePartitionId (PartitionId);
  if (!EFI_ERROR (Status)) {
    PcdSet16S (PcdTpmServiceFfaPartitionId, *PartitionId);
  }

  return Status;
}
