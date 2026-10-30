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
#include <Library/PcdLib.h>

STATIC BOOLEAN  mInitialized;
STATIC UINT16   mAgentId;
STATIC VOID     *mBuffer;
STATIC UINTN    mBufferSize;

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
    Status = FindFwuEndpoint (&gPsaFwuUpdateAgentGuid, FFA_PART_PROP_RECV_DIRECT_REQ2, &mAgentId);
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
