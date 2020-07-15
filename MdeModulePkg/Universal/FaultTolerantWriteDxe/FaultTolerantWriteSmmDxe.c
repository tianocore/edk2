/** @file

  Implement the Fault Tolerant Write (FTW) protocol based on SMM FTW
  module.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FaultTolerantWriteSmmDxe.h"

EFI_HANDLE                         mHandle                   = NULL;
EFI_MM_COMMUNICATION2_PROTOCOL     *mMmCommunication2        = NULL;
UINTN                              mPrivateDataSize          = 0;

EFI_FAULT_TOLERANT_WRITE_PROTOCOL  mFaultTolerantWriteDriver = {
  FtwGetMaxBlockSize,
  FtwAllocate,
  FtwWrite,
  FtwRestart,
  FtwAbort,
  FtwGetLastWrite
};

/**
  Initialize the communicate buffer using DataSize and Function number.

  @param[out]      CommunicateBuffer The communicate buffer. Caller should free it after use.
  @param[out]      DataPtr           Points to the data in the communicate buffer. Caller should not free it.
  @param[in]       DataSize          The payload size.
  @param[in]       Function          The function number used to initialize the communicate header.

**/
VOID
InitCommunicateBuffer (
  OUT     VOID                              **CommunicateBuffer,
  OUT     VOID                              **DataPtr,
  IN      UINTN                             DataSize,
  IN      UINTN                             Function
  )
{
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_COMMUNICATE_FUNCTION_HEADER       *SmmFtwFunctionHeader;

  //
  // The whole buffer size: SMM_COMMUNICATE_HEADER_SIZE + SMM_FTW_COMMUNICATE_HEADER_SIZE + DataSize.
  //
  SmmCommunicateHeader = AllocateZeroPool (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_FTW_COMMUNICATE_HEADER_SIZE);
  ASSERT (SmmCommunicateHeader != NULL);

  //
  // Prepare data buffer.
  //
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmFaultTolerantWriteProtocolGuid);
  SmmCommunicateHeader->MessageLength = DataSize + SMM_FTW_COMMUNICATE_HEADER_SIZE;

  SmmFtwFunctionHeader = (SMM_FTW_COMMUNICATE_FUNCTION_HEADER *) SmmCommunicateHeader->Data;
  SmmFtwFunctionHeader->Function = Function;

  *CommunicateBuffer = SmmCommunicateHeader;
  if (DataPtr != NULL) {
    *DataPtr = SmmFtwFunctionHeader->Data;
  }
}


/**
  Send the data in communicate buffer to SMI handler and get response.

  @param[in, out]  SmmCommunicateHeader    The communicate buffer.
  @param[in]       DataSize                The payload size.

**/
EFI_STATUS
SendCommunicateBuffer (
  IN OUT  EFI_MM_COMMUNICATE_HEADER         *SmmCommunicateHeader,
  IN      UINTN                             DataSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     CommSize;
  SMM_FTW_COMMUNICATE_FUNCTION_HEADER       *SmmFtwFunctionHeader;

  CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_FTW_COMMUNICATE_HEADER_SIZE;
  Status = mMmCommunication2->Communicate (mMmCommunication2,
                                           SmmCommunicateHeader,
                                           SmmCommunicateHeader,
                                           &CommSize);
  ASSERT_EFI_ERROR (Status);

  SmmFtwFunctionHeader = (SMM_FTW_COMMUNICATE_FUNCTION_HEADER *) SmmCommunicateHeader->Data;
  return  SmmFtwFunctionHeader->ReturnStatus;
}


/**
  Get the FvbBaseAddress and FvbAttributes from the FVB handle FvbHandle.

  @param[in]   FvbHandle         The handle of FVB protocol that provides services.
  @param[out]  FvbBaseAddress    The base address of the FVB attached with FvbHandle.
  @param[out]  FvbAttributes     The attributes of the FVB attached with FvbHandle.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval Others                 The function could not complete successfully.

**/
EFI_STATUS
ConvertFvbHandle (
  IN  EFI_HANDLE                            FvbHandle,
  OUT EFI_PHYSICAL_ADDRESS                  *FvbBaseAddress,
  OUT EFI_FVB_ATTRIBUTES_2                  *FvbAttributes
  )
{
  EFI_STATUS                                Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *Fvb;

  Status = gBS->HandleProtocol (FvbHandle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **) &Fvb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fvb->GetPhysicalAddress (Fvb, FvbBaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fvb->GetAttributes (Fvb, FvbAttributes);
  return Status;
}


/**
  Get the size of the largest block that can be updated in a fault-tolerant manner.

  @param[in]  This             Indicates a pointer to the calling context.
  @param[out] BlockSize        A pointer to a caller-allocated UINTN that is
                               updated to indicate the size of the largest block
                               that can be updated.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.

**/
EFI_STATUS
EFIAPI
FtwGetMaxBlockSize (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  OUT UINTN                                 *BlockSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER         *SmmFtwBlockSizeHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER);
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmFtwBlockSizeHeader, PayloadSize, FTW_FUNCTION_GET_MAX_BLOCK_SIZE);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM
  //
  *BlockSize = SmmFtwBlockSizeHeader->BlockSize;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Allocates space for the protocol to maintain information about writes.
  Since writes must be completed in a fault-tolerant manner and multiple
  writes require more resources to be successful, this function
  enables the protocol to ensure that enough space exists to track
  information about upcoming writes.

  @param[in]  This             A pointer to the calling context.
  @param[in]  CallerId         The GUID identifying the write.
  @param[in]  PrivateDataSize  The size of the caller's private data  that must be
                               recorded for each write.
  @param[in]  NumberOfWrites   The number of fault tolerant block writes that will
                               need to occur.

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    Not all allocated writes have been completed.  All
                               writes must be completed or aborted before another
                               fault tolerant write can occur.

**/
EFI_STATUS
EFIAPI
FtwAllocate (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_GUID                               *CallerId,
  IN UINTN                                  PrivateDataSize,
  IN UINTN                                  NumberOfWrites
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_ALLOCATE_HEADER                   *SmmFtwAllocateHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FTW_ALLOCATE_HEADER);
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmFtwAllocateHeader, PayloadSize, FTW_FUNCTION_ALLOCATE);
  CopyGuid (&SmmFtwAllocateHeader->CallerId, CallerId);
  SmmFtwAllocateHeader->PrivateDataSize = PrivateDataSize;
  SmmFtwAllocateHeader->NumberOfWrites  = NumberOfWrites;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);
  if (!EFI_ERROR( Status)) {
    mPrivateDataSize = PrivateDataSize;
  }

  FreePool (SmmCommunicateHeader);
  return Status;
}


/**
  Starts a target block update. This records information about the write
  in fault tolerant storage, and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param[in]  This             The calling context.
  @param[in]  Lba              The logical block address of the target block.
  @param[in]  Offset           The offset within the target block to place the
                               data.
  @param[in]  Length           The number of bytes to write to the target block.
  @param[in]  PrivateData      A pointer to private data that the caller requires
                               to complete any pending writes in the event of a
                               fault.
  @param[in]  FvBlockHandle    The handle of FVB protocol that provides services
                               for reading, writing, and erasing the target block.
  @param[in]  Buffer           The data to write.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_BAD_BUFFER_SIZE  The write would span a block boundary, which is not
                               a valid action.
  @retval EFI_ACCESS_DENIED    No writes have been allocated.
  @retval EFI_NOT_READY        The last write has not been completed. Restart()
                               must be called to complete it.

**/
EFI_STATUS
EFIAPI
FtwWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_LBA                                Lba,
  IN UINTN                                  Offset,
  IN UINTN                                  Length,
  IN VOID                                   *PrivateData,
  IN EFI_HANDLE                             FvBlockHandle,
  IN VOID                                   *Buffer
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_WRITE_HEADER                      *SmmFtwWriteHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = OFFSET_OF (SMM_FTW_WRITE_HEADER, Data) + Length;
  if (PrivateData != NULL) {
    //
    // The private data buffer size should be the same one in FtwAllocate API.
    //
    PayloadSize += mPrivateDataSize;
  }
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmFtwWriteHeader, PayloadSize, FTW_FUNCTION_WRITE);

  //
  // FvBlockHandle can not be used in SMM environment. Here we get the FVB protocol first, then get FVB base address
  // and its attribute. Send these information to SMM handler, the SMM handler will find the proper FVB to write data.
  //
  Status = ConvertFvbHandle (FvBlockHandle, &SmmFtwWriteHeader->FvbBaseAddress, &SmmFtwWriteHeader->FvbAttributes);
  if (EFI_ERROR (Status)) {
    FreePool (SmmCommunicateHeader);
    return EFI_ABORTED;
  }

  SmmFtwWriteHeader->Lba    = Lba;
  SmmFtwWriteHeader->Offset = Offset;
  SmmFtwWriteHeader->Length = Length;
  CopyMem (SmmFtwWriteHeader->Data, Buffer, Length);
  if (PrivateData == NULL) {
    SmmFtwWriteHeader->PrivateDataSize = 0;
  } else {
    SmmFtwWriteHeader->PrivateDataSize = mPrivateDataSize;
    CopyMem (&SmmFtwWriteHeader->Data[Length], PrivateData, mPrivateDataSize);
  }

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);
  FreePool (SmmCommunicateHeader);
  return Status;
}


/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.

  @param[in]  This             The calling context.
  @param[in]  FvBlockHandle    The handle of FVB protocol that provides services.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    No pending writes exist.

**/
EFI_STATUS
EFIAPI
FtwRestart (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_HANDLE                             FvBlockHandle
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_RESTART_HEADER                    *SmmFtwRestartHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FTW_RESTART_HEADER);
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmFtwRestartHeader, PayloadSize, FTW_FUNCTION_RESTART);

  //
  // FvBlockHandle can not be used in SMM environment. Here we get the FVB protocol first, then get FVB base address
  // and its attribute. Send these information to SMM handler, the SMM handler will find the proper FVB to write data.
  //
  Status = ConvertFvbHandle (FvBlockHandle, &SmmFtwRestartHeader->FvbBaseAddress, &SmmFtwRestartHeader->FvbAttributes);
  if (EFI_ERROR (Status)) {
    FreePool (SmmCommunicateHeader);
    return EFI_ABORTED;
  }

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);
  FreePool (SmmCommunicateHeader);
  return Status;
}


/**
  Aborts all previously allocated writes.

  @param[in]  This             The calling context.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwAbort (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This
  )
{
  EFI_STATUS                                Status;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;

  //
  // Initialize the communicate buffer.
  //
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, NULL, 0, FTW_FUNCTION_ABORT);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, 0);

  FreePool (SmmCommunicateHeader);
  return Status;
}


/**
  Starts a target block update. This function records information about the write
  in fault-tolerant storage and completes the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[out]     CallerId        The GUID identifying the last write.
  @param[out]     Lba             The logical block address of the last write.
  @param[out]     Offset          The offset within the block of the last write.
  @param[out]     Length          The length of the last write.
  @param[in, out] PrivateDataSize On input, the size of the PrivateData buffer. On
                                  output, the size of the private data stored for
                                  this write.
  @param[out]     PrivateData     A pointer to a buffer. The function will copy
                                  PrivateDataSize bytes from the private data stored
                                  for this write.
  @param[out]     Complete        A Boolean value with TRUE indicating that the write
                                  was completed.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             The function could not complete successfully.
  @retval EFI_NOT_FOUND           No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwGetLastWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  OUT EFI_GUID                              *CallerId,
  OUT EFI_LBA                               *Lba,
  OUT UINTN                                 *Offset,
  OUT UINTN                                 *Length,
  IN OUT UINTN                              *PrivateDataSize,
  OUT VOID                                  *PrivateData,
  OUT BOOLEAN                               *Complete
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  EFI_MM_COMMUNICATE_HEADER                 *SmmCommunicateHeader;
  SMM_FTW_GET_LAST_WRITE_HEADER             *SmmFtwGetLastWriteHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = OFFSET_OF (SMM_FTW_GET_LAST_WRITE_HEADER, Data) + *PrivateDataSize;
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmFtwGetLastWriteHeader, PayloadSize, FTW_FUNCTION_GET_LAST_WRITE);
  SmmFtwGetLastWriteHeader->PrivateDataSize = *PrivateDataSize;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM
  //
  *PrivateDataSize = SmmFtwGetLastWriteHeader->PrivateDataSize;
  if (Status == EFI_SUCCESS || Status == EFI_BUFFER_TOO_SMALL) {
    *Lba      = SmmFtwGetLastWriteHeader->Lba;
    *Offset   = SmmFtwGetLastWriteHeader->Offset;
    *Length   = SmmFtwGetLastWriteHeader->Length;
    *Complete = SmmFtwGetLastWriteHeader->Complete;
    CopyGuid (CallerId, &SmmFtwGetLastWriteHeader->CallerId);
    if (Status == EFI_SUCCESS) {
      CopyMem (PrivateData, SmmFtwGetLastWriteHeader->Data, *PrivateDataSize);
    }
  } else if (Status == EFI_NOT_FOUND) {
    *Complete = SmmFtwGetLastWriteHeader->Complete;
  }

  FreePool (SmmCommunicateHeader);
  return Status;
}

/**
  SMM Fault Tolerant Write Protocol notification event handler.

  Install Fault Tolerant Write Protocol.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
SmmFtwReady (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *Context
  )
{
  EFI_STATUS                                Status;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL         *FtwProtocol;

  //
  // Just return to avoid install SMM FaultTolerantWriteProtocol again
  // if Fault Tolerant Write protocol had been installed.
  //
  Status = gBS->LocateProtocol (&gEfiFaultTolerantWriteProtocolGuid, NULL, (VOID **)&FtwProtocol);
  if (!EFI_ERROR (Status)) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **) &mMmCommunication2);
  ASSERT_EFI_ERROR (Status);

  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiFaultTolerantWriteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mFaultTolerantWriteDriver
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
}


/**
  The driver entry point for Fault Tolerant Write driver.

  The function does the necessary initialization work.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
FaultTolerantWriteSmmInitialize (
  IN EFI_HANDLE                             ImageHandle,
  IN EFI_SYSTEM_TABLE                       *SystemTable
  )
{
  VOID                                      *SmmFtwRegistration;

  //
  // Smm FTW driver is ready
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiSmmFaultTolerantWriteProtocolGuid,
    TPL_CALLBACK,
    SmmFtwReady,
    NULL,
    &SmmFtwRegistration
    );

  return EFI_SUCCESS;
}

