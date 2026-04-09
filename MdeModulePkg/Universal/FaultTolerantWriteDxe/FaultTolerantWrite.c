/** @file

  These are the common Fault Tolerant Write (FTW) functions that are shared
  by DXE FTW driver and SMM FTW driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FaultTolerantWrite.h"

//
// Fault Tolerant Write Protocol API
//

/**
  Query the largest block that may be updated in a fault tolerant manner.


  @param This            The pointer to this protocol instance.
  @param BlockSize       A pointer to a caller allocated UINTN that is updated to
                         indicate the size of the largest block that can be updated.

  @return EFI_SUCCESS   The function completed successfully

**/
EFI_STATUS
EFIAPI
FtwGetMaxBlockSize (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This,
  OUT UINTN                             *BlockSize
  )
{
  EFI_FTW_DEVICE  *FtwDevice;

  if (!FeaturePcdGet (PcdFullFtwServiceEnable)) {
    return EFI_UNSUPPORTED;
  }

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  *BlockSize = FtwDevice->SpareAreaLength;

  return EFI_SUCCESS;
}

/**
  Allocates space for the protocol to maintain information about writes.
  Since writes must be completed in a fault tolerant manner and multiple
  updates will require more resources to be successful, this function
  enables the protocol to ensure that enough space exists to track
  information about the upcoming writes.

  All writes must be completed or aborted before another fault tolerant write can occur.

  @param This            The pointer to this protocol instance.
  @param CallerId        The GUID identifying the write.
  @param PrivateDataSize The size of the caller's private data
                         that must be recorded for each write.
  @param NumberOfWrites  The number of fault tolerant block writes
                         that will need to occur.

  @return EFI_SUCCESS        The function completed successfully
  @retval EFI_ABORTED        The function could not complete successfully.
  @retval EFI_ACCESS_DENIED  All allocated writes have not been completed.

**/
EFI_STATUS
EFIAPI
FtwAllocate (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This,
  IN EFI_GUID                           *CallerId,
  IN UINTN                              PrivateDataSize,
  IN UINTN                              NumberOfWrites
  )
{
  EFI_STATUS                       Status;
  UINTN                            Offset;
  EFI_FTW_DEVICE                   *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER  *FtwHeader;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Check if there is enough space for the coming allocation
  //
  if (FTW_WRITE_TOTAL_SIZE (NumberOfWrites, PrivateDataSize) > FtwDevice->FtwWorkSpaceHeader->WriteQueueSize) {
    DEBUG ((DEBUG_ERROR, "Ftw: Allocate() request exceed Workspace, Caller: %g\n", CallerId));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Find the last write header and record.
  // If the FtwHeader is complete, skip the completed last write header/records
  //
  FtwHeader = FtwDevice->FtwLastWriteHeader;

  //
  // Previous write has not completed, access denied.
  //
  if ((FtwHeader->HeaderAllocated == FTW_VALID_STATE) || (FtwHeader->WritesAllocated == FTW_VALID_STATE)) {
    return EFI_ACCESS_DENIED;
  }

  //
  // If workspace is not enough, then reclaim workspace
  //
  Offset = (UINT8 *)FtwHeader - (UINT8 *)FtwDevice->FtwWorkSpace;
  if (Offset + FTW_WRITE_TOTAL_SIZE (NumberOfWrites, PrivateDataSize) > FtwDevice->FtwWorkSpaceSize) {
    Status = FtwReclaimWorkSpace (FtwDevice, TRUE);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }

    FtwHeader = FtwDevice->FtwLastWriteHeader;
  }

  //
  // Prepare FTW write header,
  // overwrite the buffer and write to workspace.
  //
  FtwHeader->WritesAllocated = FTW_INVALID_STATE;
  FtwHeader->Complete        = FTW_INVALID_STATE;
  CopyMem (&FtwHeader->CallerId, CallerId, sizeof (EFI_GUID));
  FtwHeader->NumberOfWrites  = NumberOfWrites;
  FtwHeader->PrivateDataSize = PrivateDataSize;
  FtwHeader->HeaderAllocated = FTW_VALID_STATE;

  Status = WriteWorkSpaceData (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase + Offset,
             sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER),
             (UINT8 *)FtwHeader
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Update Header->WriteAllocated as VALID
  //
  Status = FtwUpdateFvState (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase + Offset,
             WRITES_ALLOCATED
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  DEBUG (
    (DEBUG_INFO,
     "Ftw: Allocate() success, Caller:%g, # %d\n",
     CallerId,
     NumberOfWrites)
    );

  return EFI_SUCCESS;
}

/**
  Write a record with fault tolerant manner.
  Since the content has already backuped in spare block, the write is
  guaranteed to be completed with fault tolerant manner.

  @param This            The pointer to this protocol instance.
  @param Fvb             The FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param BlockSize       The size of the block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwWriteRecord (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL   *This,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN UINTN                               BlockSize
  )
{
  EFI_STATUS                       Status;
  EFI_FTW_DEVICE                   *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER  *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD  *Record;
  UINTN                            Offset;
  UINTN                            NumberOfWriteBlocks;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  //
  // Spare Complete but Destination not complete,
  // Recover the target block with the spare block.
  //
  Header = FtwDevice->FtwLastWriteHeader;
  Record = FtwDevice->FtwLastWriteRecord;

  //
  // IF target block is working block, THEN Flush Spare Block To Working Block;
  // ELSE flush spare block to target block, which may be boot block after all.
  //
  if (IsWorkingBlock (FtwDevice, Fvb, Record->Lba)) {
    //
    // If target block is working block,
    // it also need to set SPARE_COMPLETED to spare block.
    //
    Offset = (UINT8 *)Record - FtwDevice->FtwWorkSpace;
    Status = FtwUpdateFvState (
               FtwDevice->FtwBackupFvb,
               FtwDevice->SpareBlockSize,
               FtwDevice->FtwSpareLba + FtwDevice->FtwWorkSpaceLbaInSpare,
               FtwDevice->FtwWorkSpaceBaseInSpare + Offset,
               SPARE_COMPLETED
               );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }

    Status = FlushSpareBlockToWorkingBlock (FtwDevice);
  } else if (IsBootBlock (FtwDevice, Fvb)) {
    //
    // Update boot block
    //
    Status = FlushSpareBlockToBootBlock (FtwDevice);
  } else {
    //
    // Update blocks other than working block or boot block
    //
    NumberOfWriteBlocks = FTW_BLOCKS ((UINTN)(Record->Offset + Record->Length), BlockSize);
    Status              = FlushSpareBlockToTargetBlock (FtwDevice, Fvb, Record->Lba, BlockSize, NumberOfWriteBlocks);
  }

  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Record the DestionationComplete in record
  //
  Offset = (UINT8 *)Record - FtwDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase + Offset,
             DEST_COMPLETED
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Record->DestinationComplete = FTW_VALID_STATE;

  //
  // If this is the last Write in these write sequence,
  // set the complete flag of write header.
  //
  if (IsLastRecordOfWrites (Header, Record)) {
    Offset = (UINT8 *)Header - FtwDevice->FtwWorkSpace;
    Status = FtwUpdateFvState (
               FtwDevice->FtwFvBlock,
               FtwDevice->WorkBlockSize,
               FtwDevice->FtwWorkSpaceLba,
               FtwDevice->FtwWorkSpaceBase + Offset,
               WRITES_COMPLETED
               );
    Header->Complete = FTW_VALID_STATE;
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Starts a target block update. This function will record data about write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param This            The pointer to this protocol instance.
  @param Lba             The logical block address of the target block.
  @param Offset          The offset within the target block to place the data.
  @param Length          The number of bytes to write to the target block.
  @param PrivateData     A pointer to private data that the caller requires to
                         complete any pending writes in the event of a fault.
  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param Buffer          The data to write.

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_BAD_BUFFER_SIZE  The input data can't fit within the spare block.
                               Offset + *NumBytes > SpareAreaLength.
  @retval EFI_ACCESS_DENIED    No writes have been allocated.
  @retval EFI_OUT_OF_RESOURCES Cannot allocate enough memory resource.
  @retval EFI_NOT_FOUND        Cannot find FVB protocol by handle.

**/
EFI_STATUS
EFIAPI
FtwWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  IN VOID                               *PrivateData,
  IN EFI_HANDLE                         FvBlockHandle,
  IN VOID                               *Buffer
  )
{
  EFI_STATUS                          Status;
  EFI_FTW_DEVICE                      *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER     *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD     *Record;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINTN                               MyLength;
  UINTN                               MyOffset;
  UINTN                               MyBufferSize;
  UINT8                               *MyBuffer;
  UINTN                               SpareBufferSize;
  UINT8                               *SpareBuffer;
  UINTN                               Index;
  UINT8                               *Ptr;
  EFI_PHYSICAL_ADDRESS                FvbPhysicalAddress;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;
  UINTN                               NumberOfWriteBlocks;
  UINTN                               WriteLength;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header = FtwDevice->FtwLastWriteHeader;
  Record = FtwDevice->FtwLastWriteRecord;

  if (IsErasedFlashBuffer ((UINT8 *)Header, sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER))) {
    if (PrivateData == NULL) {
      //
      // Ftw Write Header is not allocated.
      // No additional private data, the private data size is zero. Number of record can be set to 1.
      //
      Status = FtwAllocate (This, &gEfiCallerIdGuid, 0, 1);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else {
      //
      // Ftw Write Header is not allocated
      // Additional private data is not NULL, the private data size can't be determined.
      //
      DEBUG ((DEBUG_ERROR, "Ftw: no allocates space for write record!\n"));
      DEBUG ((DEBUG_ERROR, "Ftw: Allocate service should be called before Write service!\n"));
      return EFI_NOT_READY;
    }
  }

  //
  // If Record is out of the range of Header, return access denied.
  //
  if (((UINTN)Record - (UINTN)Header) > FTW_WRITE_TOTAL_SIZE (Header->NumberOfWrites - 1, Header->PrivateDataSize)) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Check the COMPLETE flag of last write header
  //
  if (Header->Complete == FTW_VALID_STATE) {
    return EFI_ACCESS_DENIED;
  }

  if (Record->DestinationComplete == FTW_VALID_STATE) {
    return EFI_ACCESS_DENIED;
  }

  if ((Record->SpareComplete == FTW_VALID_STATE) && (Record->DestinationComplete != FTW_VALID_STATE)) {
    return EFI_NOT_READY;
  }

  //
  // Get the FVB protocol by handle
  //
  Status = FtwGetFvbByHandle (FvBlockHandle, &Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = Fvb->GetPhysicalAddress (Fvb, &FvbPhysicalAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ftw: Write(), Get FVB physical address - %r\n", Status));
    return EFI_ABORTED;
  }

  //
  // Now, one FVB has one type of BlockSize.
  //
  Status = Fvb->GetBlockSize (Fvb, 0, &BlockSize, &NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ftw: Write(), Get block size - %r\n", Status));
    return EFI_ABORTED;
  }

  NumberOfWriteBlocks = FTW_BLOCKS (Offset + Length, BlockSize);
  DEBUG ((DEBUG_INFO, "Ftw: Write(), BlockSize - 0x%x, NumberOfWriteBlock - 0x%x\n", BlockSize, NumberOfWriteBlocks));
  WriteLength = NumberOfWriteBlocks * BlockSize;

  //
  // Check if the input data can fit within the spare block.
  //
  if (WriteLength > FtwDevice->SpareAreaLength) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Set BootBlockUpdate FLAG if it's updating boot block.
  //
  if (IsBootBlock (FtwDevice, Fvb)) {
    Record->BootBlockUpdate = FTW_VALID_STATE;
    //
    // Boot Block and Spare Block should have same block size and block numbers.
    //
    ASSERT ((BlockSize == FtwDevice->SpareBlockSize) && (NumberOfWriteBlocks == FtwDevice->NumberOfSpareBlock));
  }

  //
  // Write the record to the work space.
  //
  Record->Lba            = Lba;
  Record->Offset         = Offset;
  Record->Length         = Length;
  Record->RelativeOffset = (INT64)(FvbPhysicalAddress + (UINTN)Lba * BlockSize) - (INT64)FtwDevice->SpareAreaAddress;
  if (PrivateData != NULL) {
    CopyMem ((Record + 1), PrivateData, (UINTN)Header->PrivateDataSize);
  }

  MyOffset = (UINT8 *)Record - FtwDevice->FtwWorkSpace;
  MyLength = FTW_RECORD_SIZE (Header->PrivateDataSize);

  Status = WriteWorkSpaceData (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase + MyOffset,
             MyLength,
             (UINT8 *)Record
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Record has written to working block, then do the data.
  //
  //
  // Allocate a memory buffer
  //
  MyBufferSize = WriteLength;
  MyBuffer     = AllocatePool (MyBufferSize);
  if (MyBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read all original data from target block to memory buffer
  //
  Ptr = MyBuffer;
  for (Index = 0; Index < NumberOfWriteBlocks; Index += 1) {
    MyLength = BlockSize;
    Status   = Fvb->Read (Fvb, Lba + Index, 0, &MyLength, Ptr);
    if (EFI_ERROR (Status)) {
      FreePool (MyBuffer);
      return EFI_ABORTED;
    }

    Ptr += MyLength;
  }

  //
  // Overwrite the updating range data with
  // the input buffer content
  //
  CopyMem (MyBuffer + Offset, Buffer, Length);

  //
  // Try to keep the content of spare block
  // Save spare block into a spare backup memory buffer (Sparebuffer)
  //
  SpareBufferSize = FtwDevice->SpareAreaLength;
  SpareBuffer     = AllocatePool (SpareBufferSize);
  if (SpareBuffer == NULL) {
    FreePool (MyBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwDevice->SpareBlockSize;
    Status   = FtwDevice->FtwBackupFvb->Read (
                                          FtwDevice->FtwBackupFvb,
                                          FtwDevice->FtwSpareLba + Index,
                                          0,
                                          &MyLength,
                                          Ptr
                                          );
    if (EFI_ERROR (Status)) {
      FreePool (MyBuffer);
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr += MyLength;
  }

  //
  // Write the memory buffer to spare block
  // Do not assume Spare Block and Target Block have same block size
  //
  Status = FtwEraseSpareBlock (FtwDevice);
  if (EFI_ERROR (Status)) {
    FreePool (MyBuffer);
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  Ptr = MyBuffer;
  for (Index = 0; MyBufferSize > 0; Index += 1) {
    if (MyBufferSize > FtwDevice->SpareBlockSize) {
      MyLength = FtwDevice->SpareBlockSize;
    } else {
      MyLength = MyBufferSize;
    }

    Status = FtwDevice->FtwBackupFvb->Write (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
                                        0,
                                        &MyLength,
                                        Ptr
                                        );
    if (EFI_ERROR (Status)) {
      FreePool (MyBuffer);
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr          += MyLength;
    MyBufferSize -= MyLength;
  }

  //
  // Free MyBuffer
  //
  FreePool (MyBuffer);

  //
  // Set the SpareComplete in the FTW record,
  //
  MyOffset = (UINT8 *)Record - FtwDevice->FtwWorkSpace;
  Status   = FtwUpdateFvState (
               FtwDevice->FtwFvBlock,
               FtwDevice->WorkBlockSize,
               FtwDevice->FtwWorkSpaceLba,
               FtwDevice->FtwWorkSpaceBase + MyOffset,
               SPARE_COMPLETED
               );
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  Record->SpareComplete = FTW_VALID_STATE;

  //
  //  Since the content has already backuped in spare block, the write is
  //  guaranteed to be completed with fault tolerant manner.
  //
  Status = FtwWriteRecord (This, Fvb, BlockSize);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  //
  // Restore spare backup buffer into spare block , if no failure happened during FtwWrite.
  //
  Status = FtwEraseSpareBlock (FtwDevice);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  Ptr = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwDevice->SpareBlockSize;
    Status   = FtwDevice->FtwBackupFvb->Write (
                                          FtwDevice->FtwBackupFvb,
                                          FtwDevice->FtwSpareLba + Index,
                                          0,
                                          &MyLength,
                                          Ptr
                                          );
    if (EFI_ERROR (Status)) {
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr += MyLength;
  }

  //
  // All success.
  //
  FreePool (SpareBuffer);

  DEBUG (
    (DEBUG_INFO,
     "Ftw: Write() success, (Lba:Offset)=(%lx:0x%x), Length: 0x%x\n",
     Lba,
     Offset,
     Length)
    );

  return EFI_SUCCESS;
}

/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.

  @param This            The pointer to this protocol instance.
  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ACCESS_DENIED    No pending writes exist
  @retval  EFI_NOT_FOUND        FVB protocol not found by the handle
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
EFIAPI
FtwRestart (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This,
  IN EFI_HANDLE                         FvBlockHandle
  )
{
  EFI_STATUS                          Status;
  EFI_FTW_DEVICE                      *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER     *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD     *Record;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header = FtwDevice->FtwLastWriteHeader;
  Record = FtwDevice->FtwLastWriteRecord;

  //
  // Spare Complete but Destination not complete,
  // Recover the targt block with the spare block.
  //
  Status = FtwGetFvbByHandle (FvBlockHandle, &Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Now, one FVB has one type of BlockSize
  //
  Status = Fvb->GetBlockSize (Fvb, 0, &BlockSize, &NumberOfBlocks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ftw: Restart(), Get block size - %r\n", Status));
    return EFI_ABORTED;
  }

  //
  // Check the COMPLETE flag of last write header
  //
  if (Header->Complete == FTW_VALID_STATE) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Check the flags of last write record
  //
  if (Record->DestinationComplete == FTW_VALID_STATE) {
    return EFI_ACCESS_DENIED;
  }

  if ((Record->SpareComplete != FTW_VALID_STATE)) {
    return EFI_ABORTED;
  }

  //
  //  Since the content has already backuped in spare block, the write is
  //  guaranteed to be completed with fault tolerant manner.
  //
  Status = FtwWriteRecord (This, Fvb, BlockSize);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Erase Spare block
  // This is restart, no need to keep spareblock content.
  //
  Status = FtwEraseSpareBlock (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  DEBUG ((DEBUG_INFO, "%a(): success\n", __func__));
  return EFI_SUCCESS;
}

/**
  Aborts all previous allocated writes.

  @param This                  The pointer to this protocol instance.

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwAbort (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This
  )
{
  EFI_STATUS      Status;
  UINTN           Offset;
  EFI_FTW_DEVICE  *FtwDevice;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  if (FtwDevice->FtwLastWriteHeader->HeaderAllocated != FTW_VALID_STATE) {
    return EFI_NOT_FOUND;
  }

  if (FtwDevice->FtwLastWriteHeader->Complete == FTW_VALID_STATE) {
    return EFI_NOT_FOUND;
  }

  //
  // Update the complete state of the header as VALID and abort.
  //
  Offset = (UINT8 *)FtwDevice->FtwLastWriteHeader - FtwDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase + Offset,
             WRITES_COMPLETED
             );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwDevice->FtwLastWriteHeader->Complete = FTW_VALID_STATE;

  DEBUG ((DEBUG_INFO, "%a(): success\n", __func__));
  return EFI_SUCCESS;
}

/**
  Starts a target block update. This records information about the write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param This            The pointer to this protocol instance.
  @param CallerId        The GUID identifying the last write.
  @param Lba             The logical block address of the last write.
  @param Offset          The offset within the block of the last write.
  @param Length          The length of the last write.
  @param PrivateDataSize bytes from the private data
                         stored for this write.
  @param PrivateData     A pointer to a buffer. The function will copy
  @param Complete        A Boolean value with TRUE indicating
                         that the write was completed.

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_ABORTED           The function could not complete successfully
  @retval EFI_NOT_FOUND         No allocated writes exist
  @retval EFI_BUFFER_TOO_SMALL  Input buffer is not larget enough

**/
EFI_STATUS
EFIAPI
FtwGetLastWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *This,
  OUT EFI_GUID                          *CallerId,
  OUT EFI_LBA                           *Lba,
  OUT UINTN                             *Offset,
  OUT UINTN                             *Length,
  IN OUT UINTN                          *PrivateDataSize,
  OUT VOID                              *PrivateData,
  OUT BOOLEAN                           *Complete
  )
{
  EFI_STATUS                       Status;
  EFI_FTW_DEVICE                   *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER  *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD  *Record;

  if (!FeaturePcdGet (PcdFullFtwServiceEnable)) {
    return EFI_UNSUPPORTED;
  }

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header = FtwDevice->FtwLastWriteHeader;
  Record = FtwDevice->FtwLastWriteRecord;

  //
  // If Header is incompleted and the last record has completed, then
  // call Abort() to set the Header->Complete FLAG.
  //
  if ((Header->Complete != FTW_VALID_STATE) &&
      (Record->DestinationComplete == FTW_VALID_STATE) &&
      IsLastRecordOfWrites (Header, Record)
      )
  {
    Status    = FtwAbort (This);
    *Complete = TRUE;
    return EFI_NOT_FOUND;
  }

  //
  // If there is no write header/record, return not found.
  //
  if (Header->HeaderAllocated != FTW_VALID_STATE) {
    *Complete = TRUE;
    return EFI_NOT_FOUND;
  }

  //
  // If this record SpareComplete has not set, then it can not restart.
  //
  if (Record->SpareComplete != FTW_VALID_STATE) {
    Status = GetPreviousRecordOfWrites (Header, &Record);
    if (EFI_ERROR (Status)) {
      FtwAbort (This);
      *Complete = TRUE;
      return EFI_NOT_FOUND;
    }

    ASSERT (Record != NULL);
  }

  //
  // Fill all the requested values
  //
  CopyMem (CallerId, &Header->CallerId, sizeof (EFI_GUID));
  *Lba      = Record->Lba;
  *Offset   = (UINTN)Record->Offset;
  *Length   = (UINTN)Record->Length;
  *Complete = (BOOLEAN)(Record->DestinationComplete == FTW_VALID_STATE);

  if (*PrivateDataSize < Header->PrivateDataSize) {
    *PrivateDataSize = (UINTN)Header->PrivateDataSize;
    PrivateData      = NULL;
    Status           = EFI_BUFFER_TOO_SMALL;
  } else {
    *PrivateDataSize = (UINTN)Header->PrivateDataSize;
    CopyMem (PrivateData, Record + 1, *PrivateDataSize);
    Status = EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a(): success\n", __func__));

  return Status;
}
