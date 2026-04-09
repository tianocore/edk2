/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)

  @par Glossary:
    - FW    - Firmware
    - FWU   - Firmware Update
    - PSA   - Platform Security update for the A-profile specification
    - FMP   - Firmware Management Protocol Device.

**/
#include <Uefi.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <IndustryStandard/PsaMmFwUpdate.h>

#include <Library/ArmFfaLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FmpDeviceLib.h>

#include <Protocol/MmCommunication2.h>

#include "PsaFwuLib.h"

STATIC UINT16  mUpdateAgentId;

STATIC VOID   *mPsaFwuBuffer = NULL;
STATIC UINTN  mPsaFwuMaxBufferSize;

/**
 * Convert MmCommunication related addresses on VirtualAddress Change Event.
 *
 * @param [in]   Event      Registered VirtualAddress Change Event.
 * @param [in]   Context    Additional Data.
 *
 */
VOID
EFIAPI
PsaFwuVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (0x00, (VOID **)&mPsaFwuBuffer);
}

/**
 * Get firmware update function request fixed data size.
 *
 * @param [in]  Command         firmware update function id.
 *
 * @retval Size of request fixed data.
 */
STATIC
UINTN
EFIAPI
GetMmFwuReqDataSize (
  IN UINT32  Command
  )
{
  switch (Command) {
    case PSA_MM_FWU_COMMAND_BEGIN_STAGING:
      return sizeof (PSA_MM_FWU_BEGIN_STAGING_REQ);
    case PSA_MM_FWU_COMMAND_OPEN:
      return sizeof (PSA_MM_FWU_OPEN_REQ);
    case PSA_MM_FWU_COMMAND_WRITE_STREAM:
      return sizeof (PSA_MM_FWU_WRITE_STREAM_REQ);
    case PSA_MM_FWU_COMMAND_READ_STREAM:
      return sizeof (PSA_MM_FWU_READ_STREAM_REQ);
    case PSA_MM_FWU_COMMAND_COMMIT:
      return sizeof (PSA_MM_FWU_COMMIT_REQ);
    case PSA_MM_FWU_COMMAND_ACCEPT_IMAGE:
      return sizeof (PSA_MM_FWU_ACCEPT_IMAGE_REQ);
    default:
      return 0;
  }
}

/**
 * Get firmware update function response fixed data size.
 *
 * @param [in] Command      firmware update function id.
 *
 * @retval Size of response fixed data.
 */
STATIC
UINTN
EFIAPI
GetMmFwuRespDataSize (
  IN UINT32  Command
  )
{
  switch (Command) {
    case PSA_MM_FWU_COMMAND_DISCOVER:
      return sizeof (PSA_MM_FWU_DISCOVER_RESP);
    case PSA_MM_FWU_COMMAND_OPEN:
      return sizeof (PSA_MM_FWU_OPEN_RESP);
    case PSA_MM_FWU_COMMAND_READ_STREAM:
      return sizeof (PSA_MM_FWU_READ_STREAM_RESP);
    case PSA_MM_FWU_COMMAND_COMMIT:
      return sizeof (PSA_MM_FWU_COMMIT_RESP);
    default:
      return 0;
  }
}

/**
 * Calculate maximum available data size that can be sent in one MmCommunication.
 *
 * @param [in]  Command     Firmware update function id.
 *
 * @retval Maximum available data size could be sent on @Command.
 */
STATIC
UINTN
EFIAPI
FwuGetAvailableReqDataSize (
  IN UINT32  Command
  )
{
  return mPsaFwuMaxBufferSize -
         sizeof (PSA_MM_FWU_PARAMETER_HEADER) - GetMmFwuReqDataSize (Command);
}

/**
 * Calculate maximum available data size that can be received in one MmCommunication.
 *
 * @param [in]  Command     Firmware update function id.
 *
 * @retval Maximum available data size could be received on @Command.
 */
STATIC
UINTN
EFIAPI
FwuGetAvailableRespDataSize (
  IN UINT32  Command
  )
{
  return mPsaFwuMaxBufferSize -
         sizeof (PSA_MM_FWU_PARAMETER_HEADER) - GetMmFwuRespDataSize (Command);
}

/** Send a firmware update function request to StMM and Receive response.

  @param[in]  Command               Firmware update function id.
  @param[in]  ReqData               Request fixed data according to @Command.
  @param[in]  ClientSendData        Request variable data according to @Command.
  @param[in]  ClientSendDataSize    Size of request variable data.
  @param[in]  ClientRecvDataSize    Size of response variable data.
  @param[out] RespMsg               Response including data from StMM.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval Others                Fail to handle firmware update function.

**/
STATIC
EFI_STATUS
EFIAPI
SendFwuRequest (
  IN UINT32                Command,
  IN VOID                  *ReqData,
  IN CONST VOID            *ClientSendData,
  IN UINTN                 ClientSendDataSize,
  IN UINTN                 ClientRecvDataSize,
  OUT PSA_MM_FWU_CMD_DATA  **RespMsg
  )
{
  EFI_STATUS           Status;
  PSA_MM_FWU_CMD_DATA  *CmdData;
  VOID                 *Buffer;
  UINTN                ReqDataSize;
  UINTN                RespDataSize;
  UINTN                TotalReqCommBufferSize;
  UINTN                TotalRespCommBufferSize;
  UINTN                TotalBufferSize;
  DIRECT_MSG_ARGS      EmptyArgs;

  if ((RespMsg == NULL) || (mPsaFwuBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *RespMsg     = NULL;
  ReqDataSize  = GetMmFwuReqDataSize (Command);
  RespDataSize = GetMmFwuRespDataSize (Command);

  if ((FwuGetAvailableReqDataSize (Command) < ClientSendDataSize) ||
      (FwuGetAvailableRespDataSize (Command) < ClientRecvDataSize))
  {
    return EFI_BUFFER_TOO_SMALL;
  }

  TotalReqCommBufferSize = sizeof (PSA_MM_FWU_PARAMETER_HEADER) +
                           ReqDataSize + ClientSendDataSize;

  TotalRespCommBufferSize = sizeof (PSA_MM_FWU_PARAMETER_HEADER) +
                            RespDataSize + ClientRecvDataSize;

  TotalBufferSize = MAX (TotalReqCommBufferSize, TotalRespCommBufferSize);

  ZeroMem (mPsaFwuBuffer, TotalBufferSize);
  ZeroMem (&EmptyArgs, sizeof (DIRECT_MSG_ARGS));
  CmdData = (PSA_MM_FWU_CMD_DATA *)mPsaFwuBuffer;

  CmdData->Header.Command = Command;
  Buffer                  = GET_FWU_DATA_BUFFER (CmdData);
  CopyMem (Buffer, ReqData, ReqDataSize);
  if ((ClientSendData != NULL) && (ClientSendDataSize > 0)) {
    CopyMem (((UINT8 *)Buffer + ReqDataSize), ClientSendData, ClientSendDataSize);
  }

  Status = ArmFfaLibMsgSendDirectReq2 (
             mUpdateAgentId,
             &gPsaFwuUpdateAgentGuid,
             &EmptyArgs
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *RespMsg = (PSA_MM_FWU_CMD_DATA *)mPsaFwuBuffer;

  return EFI_SUCCESS;
}

/**
 * Convert FwuStatus to EFI_STATUS.
 *
 * @param [in]   FwuStatus        Firmware update function return status.
 *
 * @retval Matched EFI_STATUS with FwuStatus.
 *
 */
STATIC
EFI_STATUS
EFIAPI
FwuStatusToEfiStatus (
  IN UINT32  FwuStatus
  )
{
  switch (FwuStatus) {
    case PSA_MM_FWU_SUCCESS:
      return EFI_SUCCESS;
    case PSA_MM_FWU_UNKNOWN:
      return EFI_NOT_FOUND;
    case PSA_MM_FWU_BUSY:
      return EFI_NOT_READY;
    case PSA_MM_FWU_OUT_OF_BOUNDS:
      return EFI_OUT_OF_RESOURCES;
    case PSA_MM_FWU_AUTH_FAIL:
      return EFI_SECURITY_VIOLATION;
    case PSA_MM_FWU_NO_PERMISSION:
    case PSA_MM_FWU_DENIED:
      return EFI_ACCESS_DENIED;
    case PSA_MM_FWU_RESUME:
      return EFI_TIMEOUT;
    case PSA_MM_FWU_NOT_AVAILABLE:
      return EFI_UNSUPPORTED;
  }

  return EFI_DEVICE_ERROR;
}

/** Send and receive response data for firmware update function request.

  @param[in]  Command               Firmware update function id.
  @param[in]  ReqData               Request fixed data according to @Command.
  @param[in]  ClientSendData        Request variable data according to @Command.
  @param[in]  ClientSendDataSize    Size of request variable data.
  @param[in]  ClientRecvDataSize    Size of response variable data.
  @param[out] RespData              If successes, Response data correspond to @Command.

  @retval EFI_SUCCESS
  @retval Others                    Failed to @Command request.

**/
STATIC
EFI_STATUS
EFIAPI
FwuCommunicate (
  IN UINT32      Command,
  IN VOID        *ReqData,
  IN CONST VOID  *ClientSendData,
  IN UINTN       ClientSendDataSize,
  IN UINTN       ClientRecvDataSize,
  OUT VOID       **RespData
  )
{
  EFI_STATUS           Status;
  PSA_MM_FWU_CMD_DATA  *RespMsg;
  UINT32               FwuStatus;
  UINTN                RespDataSize;

  if (RespData != NULL) {
    *RespData = NULL;
  }

  /**
   * SendFwuRequest calls Smc call.
   * So, it needs to pass RespBuffer.
   */
  Status = SendFwuRequest (
             Command,
             ReqData,
             ClientSendData,
             ClientSendDataSize,
             ClientRecvDataSize,
             &RespMsg
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwuStatus = RespMsg->Header.ResponseStatus;
  if (FwuStatus == PSA_MM_FWU_SUCCESS) {
    RespDataSize = GetMmFwuRespDataSize (Command);
    if ((RespDataSize > 0) && (RespData != NULL)) {
      *RespData = (VOID *)RespMsg + sizeof (PSA_MM_FWU_CMD_DATA);
    }
  }

  return FwuStatusToEfiStatus (FwuStatus);
}

/**
 * Get firmware update agent partition id.
 *
 * @param [out]   UpdateAgentId           Update agent partition id
 *
 * @retval EFI_SUCCESS                    Success to get Update agent id
 * @retval EFI_UNSUPPORTED                Firmware update doesn't support
 * @retval Others                         Error for FF-A request.
 *
 */
STATIC
EFI_STATUS
EFIAPI
FwuGetUpdateAgentId (
  OUT UINT16  *UpdateAgentId
  )
{
  EFI_STATUS              Status;
  EFI_FFA_PART_INFO_DESC  *PartInfo;
  EFI_GUID                *ServiceGuid;
  VOID                    *RxBuffer;
  UINT64                  RxBufferSize;
  UINT32                  Count;
  UINT32                  Size;
  UINT16                  PartId;

  if (!IsFfaSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Status = ArmFfaLibPartitionIdGet (&PartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get my partition id. Status:%r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = ArmFfaLibGetRxTxBuffers (
             NULL,
             NULL,
             &RxBuffer,
             &RxBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Rx Buffer. Status:%r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ServiceGuid = &gPsaFwuUpdateAgentGuid;
  Status      = ArmFfaLibPartitionInfoGet (
                  ServiceGuid,
                  FFA_PART_INFO_FLAG_TYPE_DESC,
                  &Count,
                  &Size
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: (Fallback) Failed to get Update agent partition info. Status:%r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  PartInfo = (EFI_FFA_PART_INFO_DESC *)RxBuffer;
  if ((PartInfo->PartitionProps & FFA_PART_PROP_RECV_DIRECT_REQ2) == 0x00) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: (Fallback) Update agent doesn't support direct msg v2...\n",
      __func__
      ));
    Status = EFI_UNSUPPORTED;
    goto ErrorHandler;
  }

  *UpdateAgentId = PartInfo->PartitionId;

ErrorHandler:
  ArmFfaLibRxRelease (PartId);
  return Status;
}

/**
 * Send a FWU discovery request to StMM and get the associated response.
 *
 * @param [out]   Discovery       Discovery data related to firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval Others                 fwu_discovery isn't implemented or
 *                                Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuDiscovery (
  OUT PSA_MM_FWU_DISCOVER_RESP  **Discovery
  )
{
  EFI_STATUS  Status;

  if (Discovery == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Discovery = NULL;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |              fwu_discovery request              |
   *        |------------------------------------------------>|
   *        |                                                 |
   *        |<------------------------------------------------|
   *                         fwu_status with
   *                       (fwu discoery info)
   *                    (See PSA-FWU spec 3.4.2.1)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_DISCOVER,
             NULL,
             NULL,
             0,
             FwuGetAvailableRespDataSize (PSA_MM_FWU_COMMAND_DISCOVER),
             (VOID **)Discovery
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send a Begin Staging request to StMM and get the associated response thereby
 * start new staging process for firmware update.
 *
 * @param [in]   UpdateGuids        Image type guids to update (partial update).
 *                                  If NULL, try to update all.
 * @param [in]   UpdateCount        Number of Guids in @UpdateGuids.
 * @param [in]   VendorFlags        Vendor Specific flags.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - Firmware Store in Trial State or
 *                                    Not boot correctly using active bank.
 *                                  - Temporarily couldn't enter staging.
 * @retval EFI_NOT_FOUND            Couldn't find one image or more in @UpdateGuid.
 * @retval Others                   fwu_begin_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuBeginStaging (
  IN CONST EFI_GUID  *UpdateGuids,
  IN UINT32          UpdateCount,
  IN UINT32          VendorFlags
  )
{
  EFI_STATUS                    Status;
  PSA_MM_FWU_BEGIN_STAGING_REQ  ReqData;

  if ((UpdateCount > 0) && (UpdateGuids == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ReqData.VendorFlags        = VendorFlags;
  ReqData.PartialUpdateCount = UpdateCount;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_begin_staging request             |
   *        | (vendor flags, patrial_update_count, updateGuids) |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.2)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_BEGIN_STAGING,
             &ReqData,
             UpdateGuids,
             sizeof (EFI_GUID) * UpdateCount,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send a FWU End Staging request to the StMM and get the associated response,
 * thereby finish staging process for firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - Firmware store is not on staging state.
 *                                  - There're still open image handles.
 * @retval EFI_SECURITY_VIOLATION   Some updated image fail to authenticate.
 * @retval EFI_UNSUPPORTED          Update Agent doesn't support partial updates
 *                                  or Client has not updated all the images.
 * @retval Others                   fwu_end_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuEndStaging (
  IN VOID
  )
{
  EFI_STATUS  Status;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_end_staging request             |
   *        |------------------------------------------------>|
   *        |                                                 |
   *        |<------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.3)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_END_STAGING,
             NULL,
             NULL,
             0,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send a FWU Cancel Staging request to StMM and get the associated response
 * thereby cancel staging process for firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            Firmware store is not on staging state.
 * @retval Others                   fwu_cancel_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuCancelStaging (
  IN VOID
  )
{
  EFI_STATUS  Status;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_cancel_staging request          |
   *        |------------------------------------------------>|
   *        |                                                 |
   *        |<------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.4)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_CANCEL_STAGING,
             NULL,
             NULL,
             0,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send a FWU Open request to StMM and get associated response
 * thereby open image file handle related to @ImageTypeGuid.
 *
 * @param [in]   ImageTypeGuid      Image type guid.
 * @param [in]   OpType             FwuOpStreamRead or FwuOpStreamWrite.
 * @param [out]  Handle             Image File Handle.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Couldn't find image with @ImageTypeGuid.
 * @retval EFI_NOT_READY            Try to open with write out of staging state.
 * @retval EFI_UNSUPPORTED          @ImageTypeGuid doesn't support open with @OpType.
 * @retval Others                   fwu_open isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuOpen (
  IN  CONST EFI_GUID  *ImageTypeGuid,
  IN  FWU_OP_TYPE     OpType,
  OUT UINT32          *Handle
  )
{
  EFI_STATUS            Status;
  PSA_MM_FWU_OPEN_REQ   ReqData;
  PSA_MM_FWU_OPEN_RESP  *RespData;

  if ((ImageTypeGuid == NULL) || (Handle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Handle = FWU_INVALID_HANDLE;

  CopyGuid (&ReqData.ImageTypeGuid, ImageTypeGuid);
  ReqData.OperationType = OpType;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |                 fwu_open request                  |
   *        |             (image_type_guid, op_type)            |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                       fwu_status && handle
   *                    (See PSA-FWU spec 3.4.2.5)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_OPEN,
             &ReqData,
             NULL,
             0,
             0,
             (VOID **)&RespData
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
    return Status;
  }

  *Handle = RespData->Handle;

  return EFI_SUCCESS;
}

/**
 * Send FWU Write Stream request to StMM and get associated response
 * thereby write new firmware image to @Handle.
 *
 * @param [in]   Handle             Image file handle.
 * @param [in]   Buffer             New firmware image data.
 * @param [in]   BufferSize         Size of @Buffer in bytes.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_OUT_OF_RESOURCE      @BufferSize is over maximum image size.
 * @retval EFI_ACCESS_DENIED        Image cannot be write to.
 * @retval EFI_NOT_READY            Firmware store isn't staging state.
 * @retval Others                   fwu_write_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuWriteStream (
  IN UINT32      Handle,
  IN CONST VOID  *Buffer,
  IN UINT32      BufferSize
  )
{
  EFI_STATUS                   Status;
  PSA_MM_FWU_WRITE_STREAM_REQ  ReqData;

  if ((Handle == FWU_INVALID_HANDLE) || (Buffer == NULL) || (BufferSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize > FwuGetAvailableReqDataSize (PSA_MM_FWU_COMMAND_WRITE_STREAM)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  ReqData.Handle  = Handle;
  ReqData.DataLen = BufferSize;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_write_stream request              |
   *        |            (handle, data_len, payload)            |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.6)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_WRITE_STREAM,
             &ReqData,
             Buffer,
             BufferSize,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send FWU Read Stream request to StMM and get associated response
 * thereby read firmware image from @Handle.
 *
 * @param [in]   Handle             Image file handle.
 * @param [out]  Buffer             Data Buffer
 * @param [in]   BufferSize         Request bytes to read.
 * @param [out]  ReadyBytes         Real read bytes.
 * @param [out]  TotalBytes         Current image size of @Handle.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_ACCESS_DENIED        Image cannot be read from.
 * @retval EFI_NOT_READY            Image cannot be read temporarily.
 * @retval Others                   fwu_read_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuReadStream (
  IN      UINT32  Handle,
  OUT     VOID    *Buffer,
  IN      UINT32  BufferSize,
  OUT     UINT32  *ReadBytes,
  OUT     UINT32  *TotalBytes
  )
{
  EFI_STATUS                   Status;
  PSA_MM_FWU_READ_STREAM_REQ   ReqData;
  PSA_MM_FWU_READ_STREAM_RESP  *RespData;
  UINTN                        MaxReadSize;
  VOID                         *Payload;

  if ((Handle == FWU_INVALID_HANDLE) || ((BufferSize != 0) && (Buffer == NULL)) ||
      (ReadBytes == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  MaxReadSize = FwuGetAvailableRespDataSize (PSA_MM_FWU_COMMAND_READ_STREAM);

  ReqData.Handle = Handle;

  *ReadBytes = MIN (BufferSize, MaxReadSize);

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_read_stream request               |
   *        |                    (handle)                       |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                         fwu_status with
   *                (read_bytes, total_bytes, payload)
   *                    (See PSA-FWU spec 3.4.2.6)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_READ_STREAM,
             &ReqData,
             NULL,
             0,
             *ReadBytes,
             (VOID **)&RespData
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
    return Status;
  }

  Payload = (VOID *)GET_FWU_DATA_BUFFER (RespData);

  if (Buffer != NULL) {
    CopyMem (Buffer, Payload, RespData->ReadBytes);
  }

  *ReadBytes = RespData->ReadBytes;

  if (TotalBytes != NULL) {
    *TotalBytes = RespData->TotalBytes;
  }

  return EFI_SUCCESS;
}

/**
 * Send FWU Commit request to StMM and get associated response
 * thereby close the Handle and commit the image on firmware storage.
 *
 * @param [in]   Handle             Image file handle.
 * @param [in]   AcceptReq          If > 0, Not accept image in commit.
 *                                  remain as unacceptable status.
 *                                  otherwise, try to accept image in commit.
 * @param [in]   MaxAtomicTimeNs    Maximum time (in ns) executing without
 *                                  yielding back to client.
 * @param [out]  Progress           Unit of work already completed.
 * @param [out]  TotalWork          Unit of work must be completed.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_SECURITY_VIOLATION   Handle is closed but fail to authenticate.
 * @retval EFI_TIMEOUT              Update procedure is yielded.
 *                                  must call again fwu_commit.
 * @retval EFI_NOT_READY            Image can only be accepted after activation.
 *                                  @AcceptReq should be > 0.
 * @retval Others                   fwu_read_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuCommit (
  IN      UINT32  Handle,
  IN      UINT32  AcceptReq,
  IN      UINT32  MaxAtomicTimeNs,
  OUT     UINT32  *Progress,
  OUT     UINT32  *TotalWork
  )
{
  EFI_STATUS              Status;
  PSA_MM_FWU_COMMIT_REQ   ReqData;
  PSA_MM_FWU_COMMIT_RESP  *RespData;

  if ((Handle == FWU_INVALID_HANDLE) || (Progress == NULL) || (TotalWork == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ReqData.Handle        = Handle;
  ReqData.AcceptanceReq = AcceptReq;
  ReqData.MaxAtomicLen  = MaxAtomicTimeNs;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |                fwu_commit request                 |
   *        |          (handle, accept_req, max_atomic_len)     |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                         fwu_status with
   *                      (progress, total_work)
   *                    (See PSA-FWU spec 3.4.2.8)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_COMMIT,
             &ReqData,
             NULL,
             0,
             0,
             (VOID **)&RespData
             );
  if (EFI_ERROR (Status) && (Status != EFI_TIMEOUT)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
    return Status;
  }

  *Progress  = RespData->Progress;
  *TotalWork = RespData->TotalWork;

  return Status;
}

/**
 * Send FWU Accept Image request to StMM and get associated response
 * thereby accept updated image.
 *
 * Note:
 *  fwu_accept_image only allow when boot correctly (boot with active_index).
 *  That means this call only can accept the image on the active indexed bank.
 *
 * @param [in]   ImageTypeGuid      Image type guid.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Couldn't find image with @ImageTypeGuid
 * @retval EFI_NOT_READY            System boot incorrectly.
 * @retval Others                   fwu_accept_image isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuAcceptImage (
  IN  CONST EFI_GUID  *ImageTypeGuid
  )
{
  EFI_STATUS                   Status;
  PSA_MM_FWU_ACCEPT_IMAGE_REQ  ReqData;

  if (ImageTypeGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ReqData.Reserved = 0x00000000;
  CopyGuid (&ReqData.ImageTypeGuid, ImageTypeGuid);

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_accpet_image request              |
   *        |                (image_type_guid)                  |
   *        |-------------------------------------------------->|
   *        |                                                   |
   *        |<--------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.9)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_ACCEPT_IMAGE,
             &ReqData,
             NULL,
             0,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
 * Send FWU Select Previous request to StMM and get associated response.
 * When Firmware store is in Trial state or System boot incorrectly,
 * Rollback active indexed bank's images using previous indexed ones.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - System isn't in Trial state or boot correctly.
 *                                  - Previous indexed bank couldn't be booted.
 * @retval Others                   fwu_accept_image isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuSelectPrevious (
  IN VOID
  )
{
  EFI_STATUS  Status;

  /**
   *  +-----------------------+            +-------------------------------+
   *  |   UEFI (FmpDevicePkg) |            | StandaloneMm (FwuStMm Driver) |
   *  +-----------------------+            +-------------------------------+
   *        |             fwu_select_previous request         |
   *        |------------------------------------------------>|
   *        |                                                 |
   *        |<------------------------------------------------|
   *                            fwu_status
   *                    (See PSA-FWU spec 3.4.2.10)
   */
  Status = FwuCommunicate (
             PSA_MM_FWU_COMMAND_SELECT_PREVIOUS,
             NULL,
             NULL,
             0,
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to %a. Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
  Clear resources when PsaFwuLibInit() is failed.

**/
VOID
EFIAPI
PsaFwuLibExit (
  IN VOID
  )
{
  mPsaFwuMaxBufferSize = 0;
  mPsaFwuBuffer        = NULL;
}

/**
  Platform Flash Access Lib Constructor.

**/
EFI_STATUS
EFIAPI
PsaFwuLibInit (
  IN VOID
  )
{
  EFI_STATUS  Status;

  if (MM_SHARED_BUFFER_SIZE == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FwuGetUpdateAgentId (&mUpdateAgentId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Buffer used to communicate with update agent.
  // currently, use Non secure shared buffer.
  mPsaFwuBuffer        = (VOID *)(UINTN)PcdGet64 (PcdMmBufferBase);
  mPsaFwuMaxBufferSize = MM_SHARED_BUFFER_SIZE;

  DEBUG ((DEBUG_INFO, "%a: Update agent partition id: 0x%x\n", __func__, mUpdateAgentId));
  DEBUG ((DEBUG_INFO, "%a: mPsaFwuMaxBufferSize: %d\n", __func__, mPsaFwuMaxBufferSize));

  return EFI_SUCCESS;
}
