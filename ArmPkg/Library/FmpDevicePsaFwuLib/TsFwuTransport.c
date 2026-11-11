/** @file
  Trusted Services RPC transport for a firmware update agent.

  Buffer setup uses Trusted Services RPC management commands over FF-A
  direct requests. The selected endpoint must implement that protocol.

  Copyright (C) 2025 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PsaFwuTransport.h"
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <IndustryStandard/PsaMmFwUpdate.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmFfaMemMgmtLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define TS_RPC_MEM_RETRIEVE      0x00FF0001U
#define TS_RPC_MEM_RELINQUISH    0x00FF0002U
#define TS_RPC_SERVICE_INFO_GET  0x00FF0003U

STATIC BOOLEAN               mInitialized;
STATIC BOOLEAN               mCleanupFailed;
STATIC UINT16                mAgentId;
STATIC UINTN                 mBufferSize;
STATIC UINT64                mMemoryHandle;
STATIC UINT8                 mInterfaceId;
STATIC EFI_PHYSICAL_ADDRESS  mAddress;
STATIC UINTN                 mPages;

/** Request receiver retrieval or relinquishment of the shared handle. **/
STATIC
EFI_STATUS
MemoryRequest (
  IN UINT32  Command
  )
{
  DIRECT_MSG_ARGS  Args;
  EFI_STATUS       Status;

  ZeroMem (&Args, sizeof (Args));
  Args.Arg0 = Command;
  Args.Arg1 = (UINT32)mMemoryHandle;
  Args.Arg2 = (UINT32)(mMemoryHandle >> 32);
  Status    = ArmFfaLibMsgSendDirectReq32 (mAgentId, 0, &Args);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((INT32)Args.Arg1 != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/** Initialize the shared-buffer transport before ExitBootServices(). **/
EFI_STATUS
EFIAPI
GetPsaFwuTransportBuffer (
  OUT VOID   **Buffer,
  OUT UINTN  *BufferSize
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     CleanupStatus;
  UINT64                         Size;
  EFI_GUID                       ServiceUuid;
  UINT32                         UuidWords[4];
  DIRECT_MSG_ARGS                Args;
  FFA_MEMORY_ATTRIBUTES          Attributes;
  FFA_MEMORY_ACCESS_PERMISSIONS  Permissions;
  BOOLEAN                        Shared;

  if (Buffer != NULL) {
    *Buffer = NULL;
  }

  if (BufferSize != NULL) {
    *BufferSize = 0;
  }

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mCleanupFailed) {
    return EFI_DEVICE_ERROR;
  }

  if (mInitialized) {
    *Buffer     = (VOID *)(UINTN)mAddress;
    *BufferSize = mBufferSize;
    return EFI_SUCCESS;
  }

  Status = FindFwuEndpoint (&gPsaFwuUpdateAgentGuid, FFA_PART_PROP_RECV_DIRECT_REQ, &mAgentId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // The receiver uses the full shared region as its response capacity.
  Size = PcdGet64 (PcdMmBufferSize);
  if ((Size == 0) || (Size > MAX_UINT32) ||
      !IS_ALIGNED (Size, EFI_PAGE_SIZE))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Args, sizeof (Args));
  ConvertGuidToUuid (&gPsaFwuUpdateAgentGuid, &ServiceUuid);
  CopyMem (UuidWords, &ServiceUuid, sizeof (UuidWords));
  Args.Arg0 = TS_RPC_SERVICE_INFO_GET;
  Args.Arg1 = UuidWords[0];
  Args.Arg2 = UuidWords[1];
  Args.Arg3 = UuidWords[2];
  Args.Arg4 = UuidWords[3];
  Status    = ArmFfaLibMsgSendDirectReq32 (mAgentId, 0, &Args);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((INT32)Args.Arg1 != 0) || (Args.Arg2 > MAX_UINT8)) {
    return EFI_DEVICE_ERROR;
  }

  mInterfaceId = (UINT8)Args.Arg2;
  mPages       = EFI_SIZE_TO_PAGES ((UINTN)Size);
  Status       = gBS->AllocatePages (AllocateAnyPages, EfiRuntimeServicesData, mPages, &mAddress);
  if (EFI_ERROR (Status)) {
    mPages = 0;
    return Status;
  }

  mBufferSize = (UINTN)Size;
  ZeroMem ((VOID *)(UINTN)mAddress, EFI_PAGES_TO_SIZE (mPages));
  ZeroMem (&Attributes, sizeof (Attributes));
  Attributes.Shareability = FFA_MEMORY_INNER_SHAREABLE;
  Attributes.Cacheability = FFA_MEMORY_CACHE_WRITE_BACK;
  Attributes.Type         = FFA_MEMORY_NORMAL_MEM;
  Attributes.Security     = FFA_MEMORY_SECURITY_SECURE;
  ZeroMem (&Permissions, sizeof (Permissions));
  Permissions.DataAccess        = FFA_DATA_ACCESS_RW;
  Permissions.InstructionAccess = FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED;
  Shared                        = FALSE;
  Status                        = ArmFfaMemLibShareRxTxSingleRegion (
                                    mAgentId,
                                    mAddress,
                                    EFI_PAGES_TO_SIZE (mPages),
                                    &Attributes,
                                    &Permissions,
                                    &mMemoryHandle
                                    );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Shared = TRUE;
  Status = MemoryRequest (TS_RPC_MEM_RETRIEVE);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  mInitialized = TRUE;
  *Buffer      = (VOID *)(UINTN)mAddress;
  *BufferSize  = mBufferSize;
  return EFI_SUCCESS;

Error:
  if (Shared) {
    // A failed transport call does not prove the receiver did not retrieve.
    CleanupStatus = MemoryRequest (TS_RPC_MEM_RELINQUISH);
    if (EFI_ERROR (CleanupStatus)) {
      DEBUG ((DEBUG_ERROR, "%a: Receiver relinquishment failed: %r\n", __func__, CleanupStatus));
    }

    CleanupStatus = ArmFfaMemLibReclaim (mMemoryHandle, 0);
    if (EFI_ERROR (CleanupStatus)) {
      // Keep the allocation while the receiver might still have access.
      mCleanupFailed = TRUE;
      return Status;
    }
  }

  CleanupStatus = gBS->FreePages (mAddress, mPages);
  if (EFI_ERROR (CleanupStatus)) {
    mCleanupFailed = TRUE;
    return Status;
  }

  mAddress      = 0;
  mPages        = 0;
  mBufferSize   = 0;
  mMemoryHandle = 0;
  mInterfaceId  = 0;
  return Status;
}

/** Dispatch a request without retaining a runtime CPU buffer pointer. **/
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
  UINT32           FunctionId;

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

  CopyMem (&FunctionId, Buffer, sizeof (FunctionId));
  ZeroMem (&Args, sizeof (Args));
  if (FunctionId > MAX_UINT16) {
    return EFI_INVALID_PARAMETER;
  }

  if (FunctionId == PSA_MM_FWU_COMMAND_OPEN) {
    PSA_MM_FWU_OPEN_REQ  *Open;
    EFI_GUID             Guid;
    EFI_GUID             WireGuid;

    if (RequestSize < sizeof (PSA_MM_FWU_PARAMETER_HEADER) + sizeof (*Open)) {
      return EFI_INVALID_PARAMETER;
    }

    Open = (PSA_MM_FWU_OPEN_REQ *)((UINT8 *)Buffer + sizeof (PSA_MM_FWU_PARAMETER_HEADER));
    if (Open->OperationType == FwuOpStreamRead) {
      CopyMem (&Guid, &Open->ImageTypeGuid, sizeof (Guid));
      ConvertGuidToUuid (&Guid, &WireGuid);
      CopyMem (&Open->ImageTypeGuid, &WireGuid, sizeof (WireGuid));
    }
  }

  // Trusted Services RPC framing; the shared buffer holds the raw FWU message.
  Args.Arg0 = ((UINT32)mInterfaceId << 16) | FunctionId;
  Args.Arg1 = (UINT32)mMemoryHandle;
  Args.Arg2 = (UINT32)(mMemoryHandle >> 32);
  Args.Arg3 = (UINT32)RequestSize;
  Status    = ArmFfaLibMsgSendDirectReq32 (mAgentId, 0, &Args);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((UINT32)Args.Arg1 != 0) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32)Args.Arg3 > mBufferSize) {
    return EFI_PROTOCOL_ERROR;
  }

  *ResponseSize = (UINT32)Args.Arg3;

  return EFI_SUCCESS;
}
