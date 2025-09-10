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
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Guid/ArmFfaRxTxBufferInfo.h>

#include "ArmFfaCommon.h"

BOOLEAN  gFfaSupported;
UINT16   gPartId;

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
    ConvertGuidToUuid (ServiceGuid, (GUID *)Uuid);
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
  @param [out]  DirectMsgArg return arguments for direct msg resp/resp2

  @retval EFI_SUCCESS
  @retval Other              Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibRun (
  IN  UINT16           PartId,
  IN  UINT16           CpuNumber,
  OUT DIRECT_MSG_ARGS  *DirectMsgArg OPTIONAL
  )
{
  EFI_STATUS    Status;
  ARM_FFA_ARGS  FfaArgs;

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0 = ARM_FID_FFA_RUN;
  FfaArgs.Arg1 = PACK_PARTITION_ID_INFO (PartId, CpuNumber);

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DirectMsgArg != NULL) {
    ZeroMem (DirectMsgArg, sizeof (DIRECT_MSG_ARGS));

    if (FfaArgs.Arg0 == ARM_FID_FFA_MSG_SEND_DIRECT_RESP) {
      DirectMsgArg->Arg0 = FfaArgs.Arg3;
      DirectMsgArg->Arg1 = FfaArgs.Arg4;
      DirectMsgArg->Arg2 = FfaArgs.Arg5;
      DirectMsgArg->Arg3 = FfaArgs.Arg6;
      DirectMsgArg->Arg4 = FfaArgs.Arg7;
    } else if (FfaArgs.Arg0 == ARM_FID_FFA_MSG_SEND_DIRECT_RESP2) {
      DirectMsgArg->Arg0  = FfaArgs.Arg4;
      DirectMsgArg->Arg1  = FfaArgs.Arg5;
      DirectMsgArg->Arg2  = FfaArgs.Arg6;
      DirectMsgArg->Arg3  = FfaArgs.Arg7;
      DirectMsgArg->Arg4  = FfaArgs.Arg8;
      DirectMsgArg->Arg5  = FfaArgs.Arg9;
      DirectMsgArg->Arg6  = FfaArgs.Arg10;
      DirectMsgArg->Arg7  = FfaArgs.Arg11;
      DirectMsgArg->Arg8  = FfaArgs.Arg12;
      DirectMsgArg->Arg9  = FfaArgs.Arg13;
      DirectMsgArg->Arg10 = FfaArgs.Arg14;
      DirectMsgArg->Arg11 = FfaArgs.Arg15;
      DirectMsgArg->Arg12 = FfaArgs.Arg16;
      DirectMsgArg->Arg13 = FfaArgs.Arg17;
    }
  }

  return Status;
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
    ConvertGuidToUuid (ServiceGuid, (GUID *)Uuid);
  } else {
    ZeroMem (Uuid, sizeof (Uuid));
  }

  ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));

  FfaArgs.Arg0  = ARM_FID_FFA_MSG_SEND_DIRECT_REQ2;
  FfaArgs.Arg1  = PACK_PARTITION_ID_INFO (gPartId, DestPartId);
  FfaArgs.Arg2  = Uuid[0];
  FfaArgs.Arg3  = Uuid[1];
  FfaArgs.Arg4  = ImpDefArgs->Arg0;
  FfaArgs.Arg5  = ImpDefArgs->Arg1;
  FfaArgs.Arg6  = ImpDefArgs->Arg2;
  FfaArgs.Arg7  = ImpDefArgs->Arg3;
  FfaArgs.Arg8  = ImpDefArgs->Arg4;
  FfaArgs.Arg9  = ImpDefArgs->Arg5;
  FfaArgs.Arg10 = ImpDefArgs->Arg6;
  FfaArgs.Arg11 = ImpDefArgs->Arg7;
  FfaArgs.Arg12 = ImpDefArgs->Arg8;
  FfaArgs.Arg13 = ImpDefArgs->Arg9;
  FfaArgs.Arg14 = ImpDefArgs->Arg10;
  FfaArgs.Arg15 = ImpDefArgs->Arg11;
  FfaArgs.Arg16 = ImpDefArgs->Arg12;
  FfaArgs.Arg17 = ImpDefArgs->Arg13;

  ArmCallFfa (&FfaArgs);

  Status = FfaArgsToEfiStatus (&FfaArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ImpDefArgs->Arg0  = FfaArgs.Arg4;
  ImpDefArgs->Arg1  = FfaArgs.Arg5;
  ImpDefArgs->Arg2  = FfaArgs.Arg6;
  ImpDefArgs->Arg3  = FfaArgs.Arg7;
  ImpDefArgs->Arg4  = FfaArgs.Arg8;
  ImpDefArgs->Arg5  = FfaArgs.Arg9;
  ImpDefArgs->Arg6  = FfaArgs.Arg10;
  ImpDefArgs->Arg7  = FfaArgs.Arg11;
  ImpDefArgs->Arg8  = FfaArgs.Arg12;
  ImpDefArgs->Arg9  = FfaArgs.Arg13;
  ImpDefArgs->Arg10 = FfaArgs.Arg14;
  ImpDefArgs->Arg11 = FfaArgs.Arg15;
  ImpDefArgs->Arg12 = FfaArgs.Arg16;
  ImpDefArgs->Arg13 = FfaArgs.Arg17;

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

  if ((ARM_FFA_MAJOR_VERSION != CurrentMajorVersion) ||
      (ARM_FFA_MINOR_VERSION > CurrentMinorVersion))
  {
    DEBUG ((
      DEBUG_INFO,
      "Incompatible FF-A Versions.\n" \
      "Request Version: Major=0x%x, Minor=0x%x.\n" \
      "Current Version: Major=0x%x, Minor>=0x%x.\n",
      ARM_FFA_MAJOR_VERSION,
      ARM_FFA_MINOR_VERSION,
      CurrentMajorVersion,
      CurrentMinorVersion
      ));
    return EFI_UNSUPPORTED;
  }

  Status = ArmFfaLibPartitionIdGet (&gPartId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gFfaSupported = TRUE;

  return EFI_SUCCESS;
}

/**
  Get first Rx/Tx Buffer allocation hob.
  If UseGuid is TRUE, BufferAddr and BufferSize parameters are ignored.

  @param[in]  BufferAddr       Buffer address
  @param[in]  BufferSize       Buffer Size
  @param[in]  UseGuid          Find MemoryAllocationHob using gArmFfaRxTxBufferInfoGuid.

  @retval     NULL             Not found
  @retval     Other            MemoryAllocationHob related to Rx/Tx buffer

**/
EFI_HOB_MEMORY_ALLOCATION *
EFIAPI
GetRxTxBufferAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BufferAddr,
  IN UINT64                BufferSize,
  IN BOOLEAN               UseGuid
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  EFI_PHYSICAL_ADDRESS       MemoryBase;
  UINT64                     MemorySize;

  if (!UseGuid && (BufferAddr == 0x00)) {
    return NULL;
  }

  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);

  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) {
      continue;
    }

    MemoryBase = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
    MemorySize = Hob.MemoryAllocation->AllocDescriptor.MemoryLength;

    if ((!UseGuid && (BufferAddr >= MemoryBase) &&
         ((BufferAddr + BufferSize) <= (MemoryBase + MemorySize))) ||
        (UseGuid && CompareGuid (
                      &gArmFfaRxTxBufferInfoGuid,
                      &Hob.MemoryAllocation->AllocDescriptor.Name
                      )))
    {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }

  return MemoryAllocationHob;
}

/**
  Get Rx/Tx buffer MinSizeAndAign and MaxSize

  @param[out] MinSizeAndAlign  Minimum size of Buffer.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED          Wrong min size received from SPMC
  @retval EFI_INVALID_PARAMETER    Wrong buffer size
  @retval Others                   Failure of ArmFfaLibGetFeatures()

**/
EFI_STATUS
EFIAPI
GetRxTxBufferMinSizeAndAlign (
  OUT UINTN  *MinSizeAndAlign
  )
{
  EFI_STATUS  Status;
  UINTN       MinAndAlign;
  UINTN       MaxSize;
  UINTN       Property1;
  UINTN       Property2;

  Status = ArmFfaLibGetFeatures (
             ARM_FID_FFA_RXTX_MAP,
             FFA_RXTX_MAP_INPUT_PROPERTY_DEFAULT,
             &Property1,
             &Property2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get RX/TX buffer property... Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MinAndAlign =
    ((Property1 >>
      ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_SHIFT) &
     ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_MASK);

  switch (MinAndAlign) {
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_4K:
      MinAndAlign = SIZE_4KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_16K:
      MinAndAlign = SIZE_16KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_64K:
      MinAndAlign = SIZE_64KB;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid MinSizeAndAlign: 0x%x\n", __func__, MinAndAlign));
      return EFI_UNSUPPORTED;
  }

  MaxSize =
    (((Property1 >>
       ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_SHIFT) &
      ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_MASK));

  MaxSize = ((MaxSize == 0) ? MAX_UINTN : (MaxSize * MinAndAlign));

  if ((MinAndAlign > (PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE)) ||
      (MaxSize < (PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is too small! MinSize: 0x%x, MaxSize: 0x%x, PageCount: %d\n",
      __func__,
      MinAndAlign,
      MaxSize,
      PcdGet64 (PcdFfaTxRxPageCount)
      ));
    return EFI_INVALID_PARAMETER;
  }

  *MinSizeAndAlign = MinAndAlign;

  return EFI_SUCCESS;
}
