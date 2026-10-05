/** @file
  The instance of MM Unblock Page Library.
  This library provides an interface to request non-MMRAM pages to be mapped/unblocked
  from inside MM environment.
  For MM modules that need to access regions outside of MMRAMs, the agents that set up
  these regions are responsible for invoking this API in order for these memory areas
  to be accessed from inside MM.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/ArmFfaMemMgmt.h>
#include <Guid/MmUnblockRegionFfa.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmFfaMemMgmtLib.h>
#include <Library/ArmFfaLib.h>
#include <Base.h>

/**
  Helper function to prepare the transmit payload for memory unblock requests.

  @param[in]  BaseAddress       The base address of the memory region to be unblocked.
  @param[in]  NumberOfPages     The number of pages to be unblocked.
  @param[out] MemTransDescSize  The size of the memory transaction descriptor.
  @param[out] DestPartId        The destination partition ID.

  @retval EFI_SUCCESS           The transmit payload was prepared successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL.
  @retval EFI_NOT_FOUND         No partition descriptors were found.
  @retval EFI_UNSUPPORTED       The update agent doesn't support direct message v2.
  @retval Others                An error occurred while preparing the transmit payload.
**/
EFI_STATUS
EFIAPI
PrepareTxPayload (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 NumberOfPages,
  OUT UINT32               *MemTransDescSize,
  OUT UINT16               *DestPartId
  )
{
  FFA_MEMORY_TRANSACTION_DESCRIPTOR      *TxDescriptor;
  FFA_MEMORY_ATTRIBUTES                  Attributes;
  FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR  *MemAccessDesc;
  FFA_COMPOSITE_MEMORY_REGION            *CompositeMemoryRegion;
  UINT16                                 PartId;
  EFI_STATUS                             Status;
  UINT64                                 TxBufferSize;
  VOID                                   *TxBuffer;
  UINT64                                 RxBufferSize;
  VOID                                   *RxBuffer;
  UINT32                                 PartDescCount;
  UINT32                                 PartDescSize;
  EFI_FFA_PART_INFO_DESC                 *PartInfo;

  if ((MemTransDescSize == NULL) || (DestPartId == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: MemTransDescSize (%p) or DestPartId (%p) is NULL.\n", __func__, MemTransDescSize, DestPartId));
    return EFI_INVALID_PARAMETER;
  }

  // Discover the partition ID for myself.
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
             &TxBuffer,
             &TxBufferSize,
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

  Status = ArmFfaLibPartitionInfoGet (
             &gMmUnblockRegionFfaGuid,
             FFA_PART_INFO_FLAG_TYPE_DESC,
             &PartDescCount,
             &PartDescSize
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

  if (PartDescCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No partition descriptors found.\n",
      __func__
      ));
    Status = EFI_NOT_FOUND;
    goto ErrorHandler;
  }

  TxDescriptor = (FFA_MEMORY_TRANSACTION_DESCRIPTOR *)TxBuffer;
  ZeroMem (TxDescriptor, sizeof (*TxDescriptor));

  TxDescriptor->SenderId   = PartId;
  Attributes.Shareability  = FFA_MEMORY_INNER_SHAREABLE;
  Attributes.Cacheability  = FFA_MEMORY_CACHE_WRITE_BACK;
  Attributes.Type          = FFA_MEMORY_NORMAL_MEM;
  Attributes.Security      = 0;
  Attributes.Reserved      = 0;
  TxDescriptor->Attributes = Attributes;
  TxDescriptor->Flags      = FFA_MEMORY_ACCESS_PERMISSION_FLAG_UNSPECIFIED;
  TxDescriptor->Handle     = 0;
  TxDescriptor->Tag        = 0;

  TxDescriptor->MemoryAccessDescSize = sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR);
  TxDescriptor->ReceiverCount        = 1;
  TxDescriptor->ReceiversOffset      = sizeof (FFA_MEMORY_TRANSACTION_DESCRIPTOR);

  MemAccessDesc = (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR *)((UINTN)TxBuffer + TxDescriptor->ReceiversOffset);
  ZeroMem (MemAccessDesc, sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR));
  MemAccessDesc->ReceiverPermissions.ReceiverId                    = PartInfo->PartitionId;
  MemAccessDesc->ReceiverPermissions.Permissions.DataAccess        = FFA_DATA_ACCESS_RW;
  MemAccessDesc->ReceiverPermissions.Permissions.InstructionAccess = FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED;
  MemAccessDesc->ReceiverPermissions.Permissions.Reserved          = 0;
  MemAccessDesc->ReceiverPermissions.Flags                         = 0;

  MemAccessDesc->CompositeMemoryRegionOffset = sizeof (FFA_MEMORY_TRANSACTION_DESCRIPTOR) + sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR);

  CompositeMemoryRegion = (FFA_COMPOSITE_MEMORY_REGION *)((UINTN)TxBuffer + MemAccessDesc->CompositeMemoryRegionOffset);
  ZeroMem (CompositeMemoryRegion, sizeof (FFA_COMPOSITE_MEMORY_REGION));
  CompositeMemoryRegion->TotalPageCount   = NumberOfPages;
  CompositeMemoryRegion->ConstituentCount = 1;
  CompositeMemoryRegion->Reserved         = 0;

  CompositeMemoryRegion->Constituents[0].Address   = BaseAddress;
  CompositeMemoryRegion->Constituents[0].PageCount = NumberOfPages;
  CompositeMemoryRegion->Constituents[0].Reserved  = 0;

  *DestPartId       = PartInfo->PartitionId;
  *MemTransDescSize = sizeof (FFA_MEMORY_TRANSACTION_DESCRIPTOR) + sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR) +
                      sizeof (FFA_COMPOSITE_MEMORY_REGION) + sizeof (FFA_MEMORY_REGION_CONSTITUENT);

ErrorHandler:
  ArmFfaLibRxRelease (PartId);

  return Status;
}

/**
  This API provides a way to unblock certain data pages to be accessible inside MM environment.

  @param  UnblockAddress              The address of buffer caller requests to unblock, the address
                                      has to be page aligned.
  @param  NumberOfPages               The number of pages requested to be unblocked from MM
                                      environment.
  @retval RETURN_SUCCESS              The request goes through successfully.
  @retval RETURN_NOT_AVAILABLE_YET    The requested functionality is not produced yet.
  @retval RETURN_UNSUPPORTED          The requested functionality is not supported on current platform.
  @retval RETURN_SECURITY_VIOLATION   The requested address failed to pass security check for
                                      unblocking.
  @retval RETURN_INVALID_PARAMETER    Input address either NULL pointer or not page aligned.
  @retval RETURN_INVALID_PARAMETER    Input range to unblock contains invalid types memory other than
                                      EfiRuntimeServicesData, EfiACPIMemoryNVS, and EfiReservedMemory.
  @retval RETURN_INVALID_PARAMETER    Input range to unblock contains memory that doesn't belong to
                                      any memory allocation HOB.
  @retval RETURN_OUT_OF_RESOURCES     No enough resource to handle the unblock request.
  @retval RETURN_ACCESS_DENIED        The request is rejected due to system has passed certain boot
                                      phase.
**/
EFI_STATUS
EFIAPI
MmUnblockMemoryRequest (
  IN EFI_PHYSICAL_ADDRESS  UnblockAddress,
  IN UINT64                NumberOfPages
  )
{
  EFI_STATUS       Status;
  EFI_STATUS       ReclaimStatus;
  UINT64           Handle;
  MM_UNBLOCK_FFA   *MmUnblockFfa;
  DIRECT_MSG_ARGS  Args;
  UINT32           MemTransDescSize;
  UINT16           DestPartId;

  if (!IS_ALIGNED (UnblockAddress, SIZE_4KB)) {
    DEBUG ((DEBUG_ERROR, "%a: UnblockAddress is not 4KB aligned: %p\n", __func__, UnblockAddress));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "%a unblock memory Address: %p, Number of pages: %lu\n", __func__, UnblockAddress, NumberOfPages));

  // Prepare the FFA memory sharing payload for the unblock request.
  Status = PrepareTxPayload (UnblockAddress, NumberOfPages, &MemTransDescSize, &DestPartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to prepare TX payload: %r\n", __func__, Status));
    return Status;
  }

  // Prepare the payload for the unblock request using FF-A memory management protocol.
  Status = ArmFfaMemLibShareRxTx (
             MemTransDescSize,
             MemTransDescSize,
             &Handle
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to share memory with FF-A: %r\n", __func__, Status));
    return Status;
  }

  ZeroMem (&Args, sizeof (Args));

  MmUnblockFfa                                        = (MM_UNBLOCK_FFA *)&(Args.Arg0);
  MmUnblockFfa->OpCode                                = MmUnblockFfaOpcodeRetrieve;
  MmUnblockFfa->MmUnblockFfaData.Retrieve.Handle      = Handle;
  MmUnblockFfa->MmUnblockFfaData.Retrieve.BaseAddress = UnblockAddress;
  MmUnblockFfa->MmUnblockFfaData.Retrieve.NumOfPages  = NumberOfPages;

  // Now we have the handle, need to tell the target to claim this shared memory region.
  Status = ArmFfaLibMsgSendDirectReq2 (
             DestPartId,
             &gMmUnblockRegionFfaGuid,
             &Args
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to send direct request: %r\n", __func__, Status));
    ReclaimStatus = ArmFfaMemLibReclaim (Handle, 0);
    if (EFI_ERROR (ReclaimStatus)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to reclaim memory: %r\n", __func__, ReclaimStatus));
    }

    return Status;
  }

  MmUnblockFfa = (MM_UNBLOCK_FFA *)&(Args.Arg0);
  if (EFI_ERROR (MmUnblockFfa->Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to unblock memory: %r\n", __func__, MmUnblockFfa->Status));
    Status = ArmFfaMemLibReclaim (Handle, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to reclaim memory: %r\n", __func__, Status));
    }

    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}
