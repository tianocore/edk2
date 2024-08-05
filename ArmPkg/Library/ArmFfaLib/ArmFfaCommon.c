/** @file
  Arm Ffa library common code.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/
#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

#include <Library/ArmLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaPartInfo.h>

#include "ArmFfaCommon.h"

BOOLEAN  gFfaSupported;
UINT16   gPartId;

/**
  Convert EFI_GUID to UUID format.
  for example, If there is EFI_GUID named
  "378daedc-f06b-4446-8314-40ab933c87a3",

  EFI_GUID is saved in memory like:
     dc ae 8d 37
     6b f0 46 44
     83 14 40 ab
     93 3c 87 a3

  However, UUID should be saved like:
     37 8d ae dc
     f0 6b 44 46
     83 14 40 ab
     93 3c 87 a3

  FF-A and other software components (i.e. linux-kernel)
  uses below format.

  @param [in] Guid            EFI_GUID
  @param [out] Uuid           Uuid

**/
STATIC
VOID
EFIAPI
ConvertEfiGuidToUuid (
  IN   EFI_GUID  *Guid,
  OUT  UINT64    *Uuid
  )
{
  UINT32  *Data32;
  UINT16  *Data16;

  CopyGuid ((EFI_GUID *)Uuid, Guid);
  Data32    = (UINT32 *)Uuid;
  Data32[0] = SwapBytes32 (Data32[0]);
  Data16    = (UINT16 *)&Data32[1];
  Data16[0] = SwapBytes16 (Data16[0]);
  Data16[1] = SwapBytes16 (Data16[1]);
}

/**
  Convert EFI_STATUS to FFA return code.

  @param [in] Status          edk2 status code.

  @retval ARM_FFA_RET_*       return value correspond to EFI_STATUS.

**/
UINTN
EFIAPI
EfiStatusToFfaStatus (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return ARM_FFA_RET_SUCCESS;
    case EFI_INVALID_PARAMETER:
      return ARM_FFA_RET_INVALID_PARAMETERS;
    case EFI_OUT_OF_RESOURCES:
      return ARM_FFA_RET_NO_MEMORY;
    case EFI_NO_RESPONSE:
      return ARM_FFA_RET_BUSY;
    case EFI_INTERRUPT_PENDING:
      return ARM_FFA_RET_INTERRUPTED;
    case EFI_ACCESS_DENIED:
      return ARM_FFA_RET_DENIED;
    case EFI_ABORTED:
      return ARM_FFA_RET_ABORTED;
    case EFI_NOT_FOUND:
      return ARM_FFA_RET_NODATA;
    case EFI_NOT_READY:
      return ARM_FFA_RET_NOT_READY;
    default:
      return ARM_FFA_RET_NOT_SUPPORTED;
  }
}

/**
  Convert FFA return code to EFI_STATUS.

  @param [in] FfaStatus          Ffa return Status

  @retval EFI_STATUS             return value correspond EFI_STATUS to FfaStatus

**/
EFI_STATUS
EFIAPI
FfaStatusToEfiStatus (
  IN UINTN  FfaStatus
  )
{
  switch ((UINT32)FfaStatus) {
    case ARM_FFA_RET_SUCCESS:
      return EFI_SUCCESS;
    case ARM_FFA_RET_INVALID_PARAMETERS:
      return EFI_INVALID_PARAMETER;
    case ARM_FFA_RET_NO_MEMORY:
      return EFI_OUT_OF_RESOURCES;
    case ARM_FFA_RET_BUSY:
      return EFI_NO_RESPONSE;
    case ARM_FFA_RET_INTERRUPTED:
      return EFI_INTERRUPT_PENDING;
    case ARM_FFA_RET_DENIED:
      return EFI_ACCESS_DENIED;
    case ARM_FFA_RET_ABORTED:
      return EFI_ABORTED;
    case ARM_FFA_RET_NODATA:
      return EFI_NOT_FOUND;
    case ARM_FFA_RET_NOT_READY:
      return EFI_NOT_READY;
    default:
      return EFI_UNSUPPORTED;
  }
}

/**
  Convert FfArgs to EFI_STATUS.

  @param [in] FfaArgs            Ffa arguments

  @retval EFI_STATUS             return value correspond EFI_STATUS to FfaStatus

**/
EFI_STATUS
EFIAPI
FfaArgsToEfiStatus (
  IN ARM_FFA_ARGS  *FfaArgs
  )
{
  UINT32  FfaStatus;

  if (FfaArgs == NULL) {
    FfaStatus = ARM_FFA_RET_INVALID_PARAMETERS;
  } else if (IS_FID_FFA_ERROR (FfaArgs->Arg0)) {
    /*
     * In case of error, the Arg0 will be set to the fid FFA_ERROR.
     * and Error code is set in Arg2.
     */
    FfaStatus = FfaArgs->Arg2;
  } else if (FfaArgs->Arg0 == ARM_FFA_RET_NOT_SUPPORTED) {
    /*
     * If Some FF-A ABI doesn't support, it sets ARM_FFA_RET_NOT_SUPPORTED
     * in Arg0 and other register has no meaning.
     * In this case, set Arg2 as ARM_FFA_RET_NOT_SUPPORTED so that
     * FfaStatusToEfiStatus (FfaARgs.Arg2) returns proper EFI_STATUS.
     */
    FfaStatus = ARM_FFA_RET_NOT_SUPPORTED;
  } else if (FfaArgs->Arg0 == ARM_FID_FFA_INTERRUPT) {
    FfaStatus = ARM_FFA_RET_INTERRUPTED;
  } else {
    FfaStatus = ARM_FFA_RET_SUCCESS;
  }

  return FfaStatusToEfiStatus (FfaStatus);
}

/**
  Trigger FF-A ABI call according to PcdFfaLibConduitSmc.

  @param [in, out]  FfaArgs        Ffa arguments

**/
VOID
EFIAPI
ArmCallFfa (
  IN OUT ARM_FFA_ARGS  *FfaArgs
  )
{
  if (PcdGetBool (PcdFfaLibConduitSmc)) {
    ArmCallSmc ((ARM_SMC_ARGS *)FfaArgs);
  } else {
    ArmCallSvc ((ARM_SVC_ARGS *)FfaArgs);
  }
}

/**
  Check FF-A support or not.

  @retval TRUE                   Supported
  @retval FALSE                  Not supported

**/
BOOLEAN
EFIAPI
IsFfaSupported (
  IN VOID
  )
{
  return gFfaSupported;
}

/**
  Get FF-A version.

  @param [in]    RequestMajorVersion          Minimal request major version
  @param [in]    RequestMinorVersion          Minimal request minor version
  @param [out]   CurrentMajorVersion          Current major version
  @param [out]   CurrentMinorVersion          Current minor version

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetVersion (
  IN  UINT16  RequestMajorVersion,
  IN  UINT16  RequestMinorVersion,
  OUT UINT16  *CurrentMajorVersion,
  OUT UINT16  *CurrentMinorVersion
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_VERSION;
  FfaArgs.Arg1 = ARM_FFA_CREATE_VERSION (
                   RequestMajorVersion,
                   RequestMinorVersion
                   );

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CurrentMajorVersion != NULL) {
    *CurrentMajorVersion = ARM_FFA_MAJOR_VERSION_GET (FfaArgs.Arg0);
  }

  if (CurrentMinorVersion != NULL) {
    *CurrentMinorVersion = ARM_FFA_MINOR_VERSION_GET (FfaArgs.Arg0);
  }

  return EFI_SUCCESS;
}

/**
  Get FF-A features.

  @param [in]   Id               Feature id or function id
  @param [in]   InputProperties  Input properties according to Id
  @param [out]  Property1        First property.
  @param [out]  Property2        Second property.

  @retval EFI_SUCCESS
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetFeatures (
  IN  UINT32  Id,
  IN  UINT32  InputProperties,
  OUT UINTN   *Property1,
  OUT UINTN   *Property2
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  if ((Property1 == NULL) || (Property2 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Property1 = 0x00;
  *Property2 = 0x00;

  switch (Id) {
    case ARM_FID_FFA_RXTX_MAP_AARCH32:
    case ARM_FID_FFA_RXTX_MAP_AARCH64:
      if ((InputProperties != FFA_RXTX_MAP_INPUT_PROPERTY_DEFAULT)) {
        DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter for FunctionId: 0x%x", __func__, Id));
        return EFI_INVALID_PARAMETER;
      }

      break;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));
  FfaArgs.Arg0 = ARM_FID_FFA_FEATURES;
  FfaArgs.Arg1 = Id;
  FfaArgs.Arg2 = InputProperties;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Id) {
    case ARM_FID_FFA_RXTX_MAP_AARCH32:
    case ARM_FID_FFA_RXTX_MAP_AARCH64:
    case ARM_FFA_FEATURE_ID_NOTIFICATION_PENDING_INTERRUPT:
    case ARM_FFA_FEATURE_ID_SCHEDULE_RECEIVER_INTERRUPT:
    case ARM_FFA_FEATURE_ID_MANAGED_EXIT_INTERRUPT:
      *Property1 = FfaArgs.Arg2;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Acquire ownership of the Rx buffer.

  @param [in]  PartId    Partition Id

  @retval EFI_SUCCESS
  @retval Others         Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxAcquire (
  IN UINT16  PartId
  )
{
  ARM_FFA_ARGS  FfaArgs;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RX_ACQUIRE;
  FfaArgs.Arg1 = PartId;

  ArmCallFfa (&FfaArgs);

  return FfaArgsToEfiStatus (&FfaArgs);
}

/**
  Release ownership of the Rx buffer.

  @param [in]  PartId    Partition Id

  @retval EFI_SUCCESS
  @retval Others         Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRxRelease (
  IN UINT16  PartId
  )
{
  ARM_FFA_ARGS  FfaArgs;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RX_RELEASE;
  FfaArgs.Arg1 = PartId;

  ArmCallFfa (&FfaArgs);

  return FfaArgsToEfiStatus (&FfaArgs);
}

/**
  Get partition or VM id.

  @param [out]    PartId      Partition id

  @retval EFI_SUCCESS
  @retval Others              Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibPartitionIdGet (
  OUT UINT16  *PartId
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  if (PartId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_ID_GET;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get partition id. Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  *PartId = (FfaArgs.Arg2 >> ARM_FFA_DEST_EP_SHIFT) & ARM_FFA_PARTITION_ID_MASK;

  return EFI_SUCCESS;
}

/**
  Get spmc or spmd partition id.

  @param [out]    SpmPartId      spmc/spmd partition id

  @retval EFI_SUCCESS
  @retval Others              Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibSpmIdGet (
  OUT UINT16  *SpmPartId
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  if (SpmPartId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_SPM_ID_GET;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get partition id. Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  *SpmPartId = (FfaArgs.Arg2 >> ARM_FFA_DEST_EP_SHIFT) & ARM_FFA_PARTITION_ID_MASK;

  return EFI_SUCCESS;
}

/**
  Get Partition info.
  If This function is called to get partition descriptors
  (Flags isn't set with FFA_PART_INFO_FLAG_TYPE_COUNT),
  It should call ArmFfaLibRxRelease() to release RX buffer.

  @param [in]   ServiceGuid    Service guid.
  @param [in]   Flags          If this function called to get partition desc
                               and get successfully,
                               Caller should release RX buffer by calling
                               ArmFfaLibRxRelease
  @param [out]  Count          Number of partition or partition descriptor
  @param [out]  Size           Size of Partition Info structure in Rx Buffer

  @retval EFI_SUCCESS
  @retval Others               Error
**/
EFI_STATUS
EFIAPI
ArmFfaLibPartitionInfoGet (
  IN  EFI_GUID  *ServiceGuid,
  IN  UINT32    Flags,
  OUT UINT32    *Count,
  OUT UINT32    *Size           OPTIONAL
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;
  UINT64        Uuid[2];
  UINT32        *SmcUuid;

  if (Count == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((((Flags >> FFA_PART_INFO_FLAG_TYPE_SHIFT) & FFA_PART_INFO_FLAG_TYPE_MASK) !=
       FFA_PART_INFO_FLAG_TYPE_COUNT) && (Size == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (ServiceGuid != NULL) {
    ConvertEfiGuidToUuid (ServiceGuid, Uuid);
  } else {
    ZeroMem (Uuid, sizeof (Uuid));
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));
  SmcUuid = (UINT32 *)Uuid;

  FfaArgs.Arg0 = ARM_FID_FFA_PARTITION_INFO_GET;
  FfaArgs.Arg1 = SmcUuid[0];
  FfaArgs.Arg2 = SmcUuid[1];
  FfaArgs.Arg3 = SmcUuid[2];
  FfaArgs.Arg4 = SmcUuid[3];
  FfaArgs.Arg5 = Flags;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get partition information of %g. Status: %r\n",
      __func__,
      (ServiceGuid != NULL) ? ServiceGuid : (EFI_GUID *)Uuid,
      Status
      ));
    goto ErrorHandler;
  }

  *Count = FfaArgs.Arg2;
  if (Size != NULL) {
    *Size = FfaArgs.Arg3;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *Count = 0;
  if (Size != NULL) {
    *Size = 0;
  }

  return Status;
}

/**
  Restore the context which was interrupted with FFA_INTERRUPT (EFI_INTERRUPT_PENDING).

  @param [in]   PartId       Partition id
  @param [in]   CpuNumber    Cpu number in partition

  @retval EFI_SUCCESS
  @retval Other              Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRun (
  IN  UINT16  PartId,
  IN  UINT16  CpuNumber
  )
{
  ARM_FFA_ARGS  FfaArgs;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RUN;
  FfaArgs.Arg1 = PACK_PARTITION_ID_INFO (PartId, CpuNumber);

  ArmCallFfa (&FfaArgs);

  return FfaArgsToEfiStatus (&FfaArgs);
}

/**
  Send direct message request version 1.

  @param [in]      DestPartId       Dest partition id
  @param [in]      Flags            Message flags
  @param [in, out] ImpDefArgs       Implemented defined arguments and
                                    Implemented defined return values

  @retval EFI_SUCCESS               Success
  @retval Others                    Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibMsgSendDirectReq (
  IN  UINT16               DestPartId,
  IN  UINT32               Flags,
  IN  OUT DIRECT_MSG_ARGS  *ImpDefArgs
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  if ((DestPartId == gPartId) || (ImpDefArgs == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_MSG_SEND_DIRECT_REQ;
  FfaArgs.Arg1 = PACK_PARTITION_ID_INFO (gPartId, DestPartId);
  FfaArgs.Arg2 = Flags;
  FfaArgs.Arg3 = ImpDefArgs->Arg0;
  FfaArgs.Arg4 = ImpDefArgs->Arg1;
  FfaArgs.Arg5 = ImpDefArgs->Arg2;
  FfaArgs.Arg6 = ImpDefArgs->Arg3;
  FfaArgs.Arg7 = ImpDefArgs->Arg4;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ImpDefArgs->Arg0 = FfaArgs.Arg3;
  ImpDefArgs->Arg1 = FfaArgs.Arg4;
  ImpDefArgs->Arg2 = FfaArgs.Arg5;
  ImpDefArgs->Arg3 = FfaArgs.Arg6;
  ImpDefArgs->Arg4 = FfaArgs.Arg7;

  return EFI_SUCCESS;
}

/**
  Send direct message request version 2.

  @param [in]       DestPartId       Dest partition id
  @param [in]       ServiceGuid      Service guid
  @param [in, out]  ImpDefArgs       Implemented defined arguments and
                                     Implemented defined return values

  @retval EFI_SUCCESS               Success
  @retval Others                    Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibMsgSendDirectReq2 (
  IN      UINT16           DestPartId,
  IN      EFI_GUID         *ServiceGuid,
  IN OUT  DIRECT_MSG_ARGS  *ImpDefArgs
  )
{
  EFI_STATUS    Status;
  UINT64        Uuid[2];
  ARM_FFA_ARGS  FfaArgs;

  /*
   * Direct message request 2 is only supported on AArch64.
   */
  if (sizeof (UINTN) != sizeof (UINT64)) {
    return EFI_UNSUPPORTED;
  }

  if ((DestPartId == gPartId) || (ImpDefArgs == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ServiceGuid != NULL) {
    ConvertEfiGuidToUuid (ServiceGuid, Uuid);
  } else {
    ZeroMem (Uuid, sizeof (Uuid));
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_MSG_SEND_DIRECT_REQ2;
  FfaArgs.Arg1 = PACK_PARTITION_ID_INFO (gPartId, DestPartId);
  FfaArgs.Arg2 = Uuid[0];
  FfaArgs.Arg3 = Uuid[1];
  FfaArgs.Arg4 = ImpDefArgs->Arg0;
  FfaArgs.Arg5 = ImpDefArgs->Arg1;
  FfaArgs.Arg6 = ImpDefArgs->Arg2;
  FfaArgs.Arg7 = ImpDefArgs->Arg3;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ImpDefArgs->Arg0 = FfaArgs.Arg4;
  ImpDefArgs->Arg1 = FfaArgs.Arg5;
  ImpDefArgs->Arg2 = FfaArgs.Arg6;
  ImpDefArgs->Arg3 = FfaArgs.Arg7;

  return EFI_SUCCESS;
}

/**
  Common ArmFfaLib init.

  @retval EFI_SUCCESS            Success
  @retval EFI_UNSUPPORTED        FF-A isn't supported
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibCommonInit (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINT16      CurrentMajorVersion;
  UINT16      CurrentMinorVersion;

  gFfaSupported = FALSE;

  Status = ArmFfaLibGetVersion (
             ARM_FFA_MAJOR_VERSION,
             ARM_FFA_MINOR_VERSION,
             &CurrentMajorVersion,
             &CurrentMinorVersion
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = ArmFfaLibPartitionIdGet (&gPartId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gFfaSupported = TRUE;

  return EFI_SUCCESS;
}
