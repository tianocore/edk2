/** @file
  FF-A based memory unblock handler driver for ARM64.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Pi/PiMmCis.h>
#include <IndustryStandard/ArmFfaMemMgmt.h>
#include <Guid/MmUnblockRegionFfa.h>
#include <Protocol/MmEndOfDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmFfaMemMgmtLib.h>
#include <Library/StandaloneMmMemLib.h>

STATIC_ASSERT (sizeof (MM_UNBLOCK_FFA) <= (sizeof (ARM_FFA_ARGS) - OFFSET_OF (ARM_FFA_ARGS, Arg4)), "MM_UNBLOCK_FFA size is larger than direct message buffer size");

STATIC EFI_HANDLE  mHandle       = NULL;
STATIC UINT64      mTxBufferSize = 0;
STATIC VOID        *mTxBuffer    = NULL;
STATIC UINT64      mRxBufferSize = 0;
STATIC VOID        *mRxBuffer    = NULL;

/**
  Helper function to prepare the transmit payload for memory retrieval requests.

  @param[in]  SenderId          The sender partition ID.
  @param[in]  ReceiverId        The receiver partition ID.
  @param[in]  Handle            The memory handle for the transaction.
  @param[out] MemSize           The size of the memory transaction descriptor.

  @retval EFI_SUCCESS           The transmit payload was prepared successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL.
  @retval EFI_NOT_STARTED       The transmit buffer is not initialized.
  @retval Others                An error occurred while preparing the transmit payload.
**/
EFI_STATUS
EFIAPI
PrepareRetrieveTxPayload (
  IN UINT16   SenderId,
  IN UINT16   ReceiverId,
  IN UINT64   Handle,
  OUT UINT32  *MemSize
  )
{
  FFA_MEMORY_TRANSACTION_DESCRIPTOR      *TxDescriptor;
  FFA_MEMORY_ATTRIBUTES                  Attributes;
  FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR  *MemAccessDesc;

  if (MemSize == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: MemSize is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (mTxBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TX buffer is not initialized\n", __func__));
    return EFI_NOT_STARTED;
  }

  TxDescriptor = (FFA_MEMORY_TRANSACTION_DESCRIPTOR *)mTxBuffer;
  ZeroMem (TxDescriptor, sizeof (*TxDescriptor));

  TxDescriptor->SenderId   = SenderId;
  Attributes.Shareability  = FFA_MEMORY_INNER_SHAREABLE;
  Attributes.Cacheability  = FFA_MEMORY_CACHE_WRITE_BACK;
  Attributes.Type          = FFA_MEMORY_NORMAL_MEM;
  Attributes.Security      = 0;
  Attributes.Reserved      = 0;
  TxDescriptor->Attributes = Attributes;
  TxDescriptor->Flags      = FFA_MEMORY_ACCESS_PERMISSION_FLAG_RETRIEVAL;
  TxDescriptor->Handle     = Handle;
  TxDescriptor->Tag        = 0;

  TxDescriptor->MemoryAccessDescSize = sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR);
  TxDescriptor->ReceiverCount        = 1;
  TxDescriptor->ReceiversOffset      = sizeof (FFA_MEMORY_TRANSACTION_DESCRIPTOR);

  MemAccessDesc = (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR *)((UINTN)mTxBuffer + TxDescriptor->ReceiversOffset);
  ZeroMem (MemAccessDesc, sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR));
  MemAccessDesc->ReceiverPermissions.ReceiverId                    = ReceiverId;
  MemAccessDesc->ReceiverPermissions.Permissions.DataAccess        = FFA_DATA_ACCESS_RW;
  MemAccessDesc->ReceiverPermissions.Permissions.InstructionAccess = FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED;
  MemAccessDesc->ReceiverPermissions.Permissions.Reserved          = 0;
  MemAccessDesc->ReceiverPermissions.Flags                         = 0;
  MemAccessDesc->CompositeMemoryRegionOffset                       = 0;

  *MemSize = sizeof (FFA_MEMORY_TRANSACTION_DESCRIPTOR) + sizeof (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR);

  return EFI_SUCCESS;
}

/**
  Helper function to verify the received payload for memory retrieval requests.

  @param[in]  SenderId          The sender partition ID.
  @param[in]  ReceiverId        The receiver partition ID.
  @param[in]  BaseAddress       The base address of the memory region.
  @param[in]  NumberOfPages     The number of pages in the memory region.
  @param[in]  Handle            The memory handle for the transaction.

  @retval EFI_SUCCESS           The received payload was verified successfully.
  @retval EFI_ACCESS_DENIED     The received payload did not match the expected values.
  @retval EFI_NOT_STARTED       The receive buffer is not initialized.
  @retval Others                An error occurred while verifying the received payload.
**/
EFI_STATUS
EFIAPI
VerifyRxPayload (
  IN UINT16                SenderId,
  IN UINT16                ReceiverId,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 NumberOfPages,
  IN UINT64                Handle
  )
{
  FFA_MEMORY_TRANSACTION_DESCRIPTOR      *RxDescriptor;
  FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR  *MemAccessDesc;
  FFA_COMPOSITE_MEMORY_REGION            *CompositeMemoryRegion;

  if (mRxBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: RX buffer is not initialized\n", __func__));
    return EFI_NOT_STARTED;
  }

  RxDescriptor = (FFA_MEMORY_TRANSACTION_DESCRIPTOR *)mRxBuffer;
  if (RxDescriptor->Handle != Handle) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor handle mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (RxDescriptor->SenderId != SenderId) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor sender ID mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  MemAccessDesc = (FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR *)((UINTN)RxDescriptor + RxDescriptor->ReceiversOffset);
  if (MemAccessDesc->ReceiverPermissions.ReceiverId != ReceiverId) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor receiver ID mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (MemAccessDesc->ReceiverPermissions.Permissions.DataAccess != FFA_DATA_ACCESS_RW) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor receiver data access not RW\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (MemAccessDesc->ReceiverPermissions.Permissions.InstructionAccess != FFA_INSTRUCTION_ACCESS_NX) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor receiver instruction access not NX\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  CompositeMemoryRegion = (FFA_COMPOSITE_MEMORY_REGION *)((UINTN)RxDescriptor + MemAccessDesc->CompositeMemoryRegionOffset);
  if (CompositeMemoryRegion->TotalPageCount != NumberOfPages) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor number of pages mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (CompositeMemoryRegion->ConstituentCount != 1) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor constituent count mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (CompositeMemoryRegion->Constituents[0].PageCount != NumberOfPages) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor constituent page count mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  if (CompositeMemoryRegion->Constituents[0].Address != BaseAddress) {
    DEBUG ((DEBUG_ERROR, "%a: RX descriptor constituent address mismatch\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Helper function to prepare the transmit payload for memory relinquish requests.

  @param[in]  EndpointId        The endpoint ID for the memory relinquish request.
  @param[in]  Handle            The memory handle for the transaction.

  @retval EFI_SUCCESS           The transmit payload was prepared successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL.
  @retval EFI_NOT_STARTED       The transmit buffer is not initialized.
  @retval Others                An error occurred while preparing the transmit payload.
**/
EFI_STATUS
PrepareRelinquishTxPayload (
  IN UINT16  EndpointId,
  IN UINT64  Handle
  )
{
  FFA_MEM_RELINQUISH_DESCRIPTOR  *RelinquishDesc;

  if (mTxBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RelinquishDesc                = (FFA_MEM_RELINQUISH_DESCRIPTOR *)mTxBuffer;
  RelinquishDesc->Handle        = Handle;
  RelinquishDesc->Flags         = 0;
  RelinquishDesc->EndpointCount = 1;
  RelinquishDesc->Endpoints[0]  = EndpointId;

  return EFI_SUCCESS;
}

/**
  This function is the main entry point for an MM handler that handles memory unblock requests.

  @param  DispatchHandle  The unique handle assigned to this handler by
                          MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was
                          specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an
                          MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
UnblockMemoryMmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS       Status;
  EFI_STATUS       CleanupStatus;
  UINT64           PhysicalAddress;
  UINT64           NumOfPages;
  UINT64           Handle;
  UINT32           MemSize;
  UINT32           RespFragSize;
  MM_UNBLOCK_FFA   *LocalUnblockFfa;
  DIRECT_MSG_ARGS  *DirectMsgArgs;

  if ((CommBuffer == NULL) ||
      (CommBufferSize == NULL))
  {
    DEBUG ((DEBUG_ERROR, "Invalid parameter: CommBuffer, or CommBufferSize is NULL\n"));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (*CommBufferSize < sizeof (ARM_FFA_ARGS)) {
    DEBUG ((DEBUG_ERROR, "Invalid parameter: CommBufferSize (%lu) is too small\n", *CommBufferSize));
    ASSERT (FALSE);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer, *CommBufferSize) == FALSE) {
    DEBUG ((DEBUG_ERROR, "[%a] - MM Communication buffer is outside of MM region!\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  LocalUnblockFfa = (MM_UNBLOCK_FFA *)((UINT8 *)CommBuffer + OFFSET_OF (ARM_FFA_ARGS, Arg4));
  DirectMsgArgs   = (DIRECT_MSG_ARGS *)CommBuffer;

  switch (LocalUnblockFfa->OpCode) {
    case MmUnblockFfaOpcodeRetrieve:

      PhysicalAddress = LocalUnblockFfa->MmUnblockFfaData.Retrieve.BaseAddress;
      NumOfPages      = LocalUnblockFfa->MmUnblockFfaData.Retrieve.NumOfPages;
      Handle          = LocalUnblockFfa->MmUnblockFfaData.Retrieve.Handle;
      DEBUG ((DEBUG_INFO, "Retrieve request: PhysicalAddress=0x%lx, NumOfPages=%lu, Handle=0x%lx\n", PhysicalAddress, NumOfPages, Handle));

      // Issue FFA command to retrieve the memory unblock information
      Status = PrepareRetrieveTxPayload (GET_SOURCE_PARTITION_ID (DirectMsgArgs->Header.x1), GET_DEST_PARTITION_ID (DirectMsgArgs->Header.x1), Handle, &MemSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to prepare TX payload: %r\n", __func__, Status));
        break;
      }

      Status = ArmFfaMemLibRetrieveReqRxTx (MemSize, MemSize, &MemSize, &RespFragSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to retrieve memory unblock information: %r\n", __func__, Status));
        break;
      }

      if ((MemSize > mRxBufferSize) || (RespFragSize > mRxBufferSize)) {
        DEBUG ((DEBUG_ERROR, "%a: Retrieved memory size (%lu) or response fragment size (%lu) exceeds RX buffer size (%lu)\n", __func__, MemSize, RespFragSize, mRxBufferSize));
        Status = EFI_BUFFER_TOO_SMALL;
      }

      // Ensure the retrieval matches the direct req2 information
      if (!EFI_ERROR (Status)) {
        Status = VerifyRxPayload (GET_SOURCE_PARTITION_ID (DirectMsgArgs->Header.x1), GET_DEST_PARTITION_ID (DirectMsgArgs->Header.x1), PhysicalAddress, NumOfPages, Handle);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Failed to verify memory unblock information: %r\n", __func__, Status));
        }
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Memory unblock retrieval failed: %r\n", __func__, Status));
        CleanupStatus = PrepareRelinquishTxPayload (GET_DEST_PARTITION_ID (DirectMsgArgs->Header.x1), Handle);
        if (EFI_ERROR (CleanupStatus)) {
          DEBUG ((DEBUG_ERROR, "%a: Failed to prepare relinquish TX payload: %r\n", __func__, CleanupStatus));
        } else {
          CleanupStatus = ArmFfaMemLibRelinquish ();
          if (EFI_ERROR (CleanupStatus)) {
            DEBUG ((DEBUG_ERROR, "%a: Also failed to relinquish memory: %r\n", __func__, CleanupStatus));
          }
        }
      }

      CleanupStatus = ArmFfaLibRxRelease (0);
      if (EFI_ERROR (CleanupStatus)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to release RX buffer: %r\n", __func__, CleanupStatus));
      }

      if (!EFI_ERROR (Status)) {
        // Only update the status with the release status if the retrieval was successfully completed
        Status = CleanupStatus;
      }

      break;
    case MmUnblockFfaOpcodeRelinquish:
      Status = EFI_UNSUPPORTED;
      break;
    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  DEBUG ((DEBUG_INFO, "UnblockMemoryMmiHandler completed with status: %r\n", Status));
  LocalUnblockFfa->Status = Status;
  *CommBufferSize         = sizeof (MM_UNBLOCK_FFA);

  return EFI_SUCCESS;
}

/**
  The callback function for the end of DXE phase. It unregisters the memory unblock MMI handler.

  @param[in]  Protocol   The protocol GUID.
  @param[in]  Interface  The interface pointer.
  @param[in]  Handle     The handle on which the protocol was installed.

  @retval EFI_SUCCESS           The callback was processed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL.
  @retval Others                An error occurred while processing the callback.
**/
EFI_STATUS
EFIAPI
MmEndofDxeMmNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS  Status;

  Status = gMmst->MmiHandlerUnRegister (
                    mHandle
                    );
  DEBUG ((DEBUG_INFO, "%a Unregister unblock handler at End of Dxe - %r\n", __func__, Status));

  return Status;
}

/**
  The entry point for the MM Unblock Memory driver.

  @param[in]  ImageHandle  The image handle of the driver.
  @param[in]  SystemTable  The MM system table.

  @retval EFI_SUCCESS           The driver was initialized successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL.
  @retval Others                An error occurred while initializing the driver.
**/
EFI_STATUS
EFIAPI
MmUnblockMemArmInitialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  DEBUG ((DEBUG_INFO, "%a: Initializing UnblockMemoryHandler\n", __func__));

  Status = ArmFfaLibGetRxTxBuffers (
             &mTxBuffer,
             &mTxBufferSize,
             &mRxBuffer,
             &mRxBufferSize
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

  // register the MMI handler for memory unblock requests, through FFA_DIRECT_REQ2
  mHandle = NULL;
  Status  = gMmst->MmiHandlerRegister (
                     UnblockMemoryMmiHandler,
                     &gMmUnblockRegionFfaGuid,
                     &mHandle
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to register MMI handler: %r\n", __func__, Status));
    return Status;
  }

  // register a callback for end of dxe, so that we do not take further requests after DXE phase.
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiMmEndOfDxeProtocolGuid,
                    MmEndofDxeMmNotify,
                    &Registration
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to register end of DXE protocol notify: %r\n", __func__, Status));
    gMmst->MmiHandlerUnRegister (
             mHandle
             );
    return Status;
  }

  return EFI_SUCCESS;
}
