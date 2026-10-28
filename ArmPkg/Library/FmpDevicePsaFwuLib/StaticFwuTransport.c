/** @file
  PSA firmware update transport using a static communication buffer.

  Copyright (C) 2024 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PsaFwuTransport.h"
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <IndustryStandard/PsaMmFwUpdate.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

STATIC BOOLEAN  mInitialized;
STATIC UINT16   mAgentId;
STATIC VOID     *mBuffer;
STATIC UINTN    mBufferSize;

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

/** Initialize the static-buffer transport before ExitBootServices(). **/
EFI_STATUS
EFIAPI
GetPsaFwuTransportBuffer (
  OUT VOID   **Buffer,
  OUT UINTN  *BufferSize
  )
{
  EFI_STATUS  Status;
  UINT64      Base;
  UINT64      Size;

  if (Buffer != NULL) {
    *Buffer = NULL;
  }

  if (BufferSize != NULL) {
    *BufferSize = 0;
  }

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    Status = FwuGetUpdateAgentId (&mAgentId);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Base = PcdGet64 (PcdMmBufferBase);
    Size = PcdGet64 (PcdMmBufferSize);
    if ((Base == 0) || (Base > MAX_UINTN) || (Size == 0) ||
        (Size - 1 > MAX_UINTN - Base))
    {
      return EFI_INVALID_PARAMETER;
    }

    // The platform must map this reserved buffer for runtime access.
    mBuffer      = (VOID *)(UINTN)Base;
    mBufferSize  = (UINTN)MIN (Size, MAX_UINT32);
    mInitialized = TRUE;
  }

  *Buffer     = mBuffer;
  *BufferSize = mBufferSize;
  return EFI_SUCCESS;
}

/** Submit a direct request v2 using the current CPU buffer address. **/
EFI_STATUS
EFIAPI
PsaFwuTransportSendRequest (
  IN OUT VOID  *Buffer,
  IN UINTN     RequestSize,
  OUT UINTN    *ResponseSize
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  Args;

  if (ResponseSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ResponseSize = 0;
  if (!mInitialized) {
    return EFI_NOT_READY;
  }

  if ((Buffer == NULL) || (RequestSize < sizeof (UINT32)) ||
      (RequestSize > mBufferSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Args, sizeof (Args));
  Status = ArmFfaLibMsgSendDirectReq2 (mAgentId, &gPsaFwuUpdateAgentGuid, &Args);
  if (!EFI_ERROR (Status)) {
    *ResponseSize = mBufferSize;
  }

  return Status;
}
