/** @file
  This library provides an implementation of Tpm2DeviceLib
  using ARM64 SMC calls to request TPM service.

  The implementation is only supporting the Command Response Buffer (CRB)
  for sharing data with the TPM.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaBootInfo.h>
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <Guid/Tpm2ServiceFfa.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/ArmFfaLib.h>

#include "Tpm2DeviceLibFfa.h"

UINT16  mFfaTpm2PartitionId = TPM2_FFA_PARTITION_ID_INVALID;

/**
  Check the return status from the FF-A call and returns EFI_STATUS

  @param EFI_LOAD_ERROR  FF-A status code returned in x0

  @retval EFI_SUCCESS    The entry point is executed successfully.
**/
EFI_STATUS
TranslateTpmReturnStatus (
  UINTN  TpmReturnStatus
  )
{
  EFI_STATUS  Status;

  switch (TpmReturnStatus) {
    case TPM2_FFA_SUCCESS_OK:
    case TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED:
      Status = EFI_SUCCESS;
      break;

    case TPM2_FFA_ERROR_NOFUNC:
      Status = EFI_NOT_FOUND;
      break;

    case TPM2_FFA_ERROR_NOTSUP:
      Status = EFI_UNSUPPORTED;
      break;

    case TPM2_FFA_ERROR_INVARG:
      Status = EFI_INVALID_PARAMETER;
      break;

    case TPM2_FFA_ERROR_INV_CRB_CTRL_DATA:
      Status = EFI_COMPROMISED_DATA;
      break;

    case TPM2_FFA_ERROR_ALREADY:
      Status = EFI_ALREADY_STARTED;
      break;

    case TPM2_FFA_ERROR_DENIED:
      Status = EFI_ACCESS_DENIED;
      break;

    case TPM2_FFA_ERROR_NOMEM:
      Status = EFI_OUT_OF_RESOURCES;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  This function is used to get the TPM service partition id via FF-A.

  @param[out] PartitionId - Supplies the pointer to the TPM service partition id.

  @retval EFI_SUCCESS           The TPM command was successfully sent to the TPM
                                and the response was copied to the Output buffer.
  @retval EFI_INVALID_PARAMETER The TPM command buffer is NULL or the TPM command
                                buffer size is 0.
  @retval EFI_DEVICE_ERROR      The TPM partition information is wrong.
  @retval EFI_DEVICE_ERROR      An error occurred in communication with the TPM.
**/
EFI_STATUS
FfaTpm2GetServicePartitionId (
  OUT UINT16  *PartitionId
  )
{
  EFI_STATUS              Status;
  EFI_FFA_PART_INFO_DESC  TpmPartInfo;

  if (PartitionId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ArmFfaLibGetPartitionInfo (&gTpm2ServiceFfaGuid, &TpmPartInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Tpm2 partition info. Status: %r\n", Status));
    return Status;
  }

  *PartitionId = TpmPartInfo.PartitionId;
  if (TpmPartInfo.PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    /*
     * Tpm partition id never be TPM2_FFA_PARTITION_ID_INVALID.
     */
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function is used to get the TPM interface version.

  @param[out] Version - Supplies the pointer to the TPM interface version.

  @retval EFI_SUCCESS           The TPM command was successfully sent to the TPM
                                and the response was copied to the Output buffer.
  @retval EFI_INVALID_PARAMETER The TPM command buffer is NULL or the TPM command
                                buffer size is 0.
  @retval EFI_DEVICE_ERROR      An error occurred in communication with the TPM.
**/
EFI_STATUS
Tpm2GetInterfaceVersion (
  OUT UINT32  *Version
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (Version == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_GET_INTERFACE_VERSION;

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

  if (!EFI_ERROR (Status)) {
    *Version = FfaDirectReq2Args.Arg1;
  }

Exit:
  return Status;
}

/**
  This function is used to get the TPM feature information.

  @param[out] FeatureInfo - Supplies the pointer to the feature information.

  @retval EFI_SUCCESS           The TPM command was successfully sent to the TPM
                                and the response was copied to the Output buffer.
  @retval EFI_INVALID_PARAMETER The TPM command buffer is NULL or the TPM command
                                buffer size is 0.
  @retval EFI_DEVICE_ERROR      An error occurred in communication with the TPM.
**/
EFI_STATUS
Tpm2GetFeatureInfo (
  OUT UINT32  *FeatureInfo
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (FeatureInfo == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_GET_FEATURE_INFO;
  FfaDirectReq2Args.Arg1 = TPM_SERVICE_FEATURE_SUPPORT_NOTIFICATION;

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

Exit:
  return Status;
}

/**
  This service enables the sending of commands to the TPM2.

  @param[in]  FuncQualifier          Function qualifier.
  @param[in]  LocalityQualifier      Locality qualifier.

  @retval EFI_SUCCESS           The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR      The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL  The output parameter block is too small.
**/
EFI_STATUS
Tpm2ServiceStart (
  IN UINT64  FuncQualifier,
  IN UINT64  LocalityQualifier
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_START;
  FfaDirectReq2Args.Arg1 = (FuncQualifier & 0xFF);
  FfaDirectReq2Args.Arg2 = (LocalityQualifier & 0xFF);

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

Exit:
  return Status;
}

/**
  Register TPM2 device notification.

  @param[in] NotificationTypeQualifier  Notification type qualifier.
  @param[in] vCpuId                     vCPU ID.
  @param[in] NotificationId             Bitmap ID for the notification.

  @retval EFI_SUCCESS  The command was successfully sent to the device and a response was successfully received.
  @retval Others       Some error occurred in communication with the device.
**/
EFI_STATUS
Tpm2RegisterNotification (
  IN BOOLEAN  NotificationTypeQualifier,
  IN UINT16   vCpuId,
  IN UINT64   NotificationId
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_REGISTER_FOR_NOTIFICATION;
  FfaDirectReq2Args.Arg1 = (NotificationTypeQualifier << 16 | vCpuId);
  FfaDirectReq2Args.Arg2 = (NotificationId & 0xFF);

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

Exit:
  return Status;
}

/**
  Unregister TPM2 device notification.

  @retval EFI_SUCCESS  The command was successfully sent to the device and a response was successfully received.
  @retval Others       Some error occurred in communication with the device.
**/
EFI_STATUS
Tpm2UnregisterNotification (
  VOID
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_UNREGISTER_FROM_NOTIFICATION;

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

Exit:
  return Status;
}

/**
  Issue a finished notification command to the TPM service over FF-A.

  @retval EFI_SUCCESS  The command was successfully sent to the device and a response was successfully received.
  @retval Others       Some error occurred in communication with the device.
**/
EFI_STATUS
Tpm2FinishNotified (
  VOID
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  FfaDirectReq2Args;

  if (mFfaTpm2PartitionId == TPM2_FFA_PARTITION_ID_INVALID) {
    GetTpmServicePartitionId (&mFfaTpm2PartitionId);
  }

  ZeroMem (&FfaDirectReq2Args, sizeof (DIRECT_MSG_ARGS));
  FfaDirectReq2Args.Arg0 = TPM2_FFA_FINISH_NOTIFIED;

  Status = ArmFfaLibMsgSendDirectReq2 (mFfaTpm2PartitionId, &gTpm2ServiceFfaGuid, &FfaDirectReq2Args);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (mFfaTpm2PartitionId, 0x00);
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = TranslateTpmReturnStatus (FfaDirectReq2Args.Arg0);

Exit:
  return Status;
}
