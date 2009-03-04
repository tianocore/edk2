/** @file

  This is a simple fault tolerant write driver.

  This boot service protocol only provides fault tolerant write capability for 
  block devices.  The protocol has internal non-volatile intermediate storage 
  of the data and private information. It should be able to recover 
  automatically from a critical fault, such as power failure. 

  The implementation uses an FTW (Fault Tolerant Write) Work Space. 
  This work space is a memory copy of the work space on the Working Block,
  the size of the work space is the FTW_WORK_SPACE_SIZE bytes.
  
  The work space stores each write record as EFI_FTW_RECORD structure.
  The spare block stores the write buffer before write to the target block.
  
  The write record has three states to specify the different phase of write operation.
  1) WRITE_ALLOCATED is that the record is allocated in write space.
     The information of write operation is stored in write record structure.
  2) SPARE_COMPLETED is that the data from write buffer is writed into the spare block as the backup.
  3) WRITE_COMPLETED is that the data is copied from the spare block to the target block.

  This driver operates the data as the whole size of spare block.
  It first read the SpareAreaLength data from the target block into the spare memory buffer.
  Then copy the write buffer data into the spare memory buffer.
  Then write the spare memory buffer into the spare block.
  Final copy the data from the spare block to the target block.

  To make this drive work well, the following conditions must be satisfied:
  1. The write NumBytes data must be fit within Spare area. 
     Offset + NumBytes <= SpareAreaLength
  2. The whole flash range has the same block size.
  3. Working block is an area which contains working space in its last block and has the same size as spare block.
  4. Working Block area must be in the single one Firmware Volume Block range which FVB protocol is produced on.  
  5. Spare area must be in the single one Firmware Volume Block range which FVB protocol is produced on.
  6. Any write data area (SpareAreaLength Area) which the data will be written into must be 
     in the single one Firmware Volume Block range which FVB protocol is produced on.
  7. If write data area (such as Variable range) is enlarged, the spare area range must be enlarged.
     The spare area must be enough large to store the write data before write them into the target range.
  If one of them is not satisfied, FtwWrite may fail.
  Usually, Spare area only takes one block. That's SpareAreaLength = BlockSize, NumberOfSpareBlock = 1.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  OUT UINTN                               *BlockSize
  )
{
  EFI_FTW_DEVICE  *FtwDevice;

  if (!FeaturePcdGet(PcdFullFtwServiceEnable)) {
    return EFI_UNSUPPORTED;
  }

  FtwDevice   = FTW_CONTEXT_FROM_THIS (This);

  *BlockSize  = FtwDevice->SpareAreaLength;

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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  IN EFI_GUID                             *CallerId,
  IN UINTN                                PrivateDataSize,
  IN UINTN                                NumberOfWrites
  )
{
  EFI_STATUS                      Status;
  UINTN                           Length;
  UINTN                           Offset;
  EFI_FTW_DEVICE                  *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER *FtwHeader;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status    = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Check if there is enough space for the coming allocation
  //
  if (WRITE_TOTAL_SIZE (NumberOfWrites, PrivateDataSize) > FtwDevice->FtwWorkSpaceHeader->WriteQueueSize) {
    DEBUG ((EFI_D_ERROR, "Ftw: Allocate() request exceed Workspace, Caller: %g\n", CallerId));
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
  Offset = (UINT8 *) FtwHeader - (UINT8 *) FtwDevice->FtwWorkSpace;
  if (Offset + WRITE_TOTAL_SIZE (NumberOfWrites, PrivateDataSize) > FtwDevice->FtwWorkSpaceSize) {
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
  FtwHeader->WritesAllocated  = FTW_INVALID_STATE;
  FtwHeader->Complete         = FTW_INVALID_STATE;
  CopyMem (&FtwHeader->CallerId, CallerId, sizeof (EFI_GUID));
  FtwHeader->NumberOfWrites   = NumberOfWrites;
  FtwHeader->PrivateDataSize  = PrivateDataSize;
  FtwHeader->HeaderAllocated  = FTW_VALID_STATE;

  Length                      = sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER);
  Status = FtwDevice->FtwFvBlock->Write (
                                    FtwDevice->FtwFvBlock,
                                    FtwDevice->FtwWorkSpaceLba,
                                    FtwDevice->FtwWorkSpaceBase + Offset,
                                    &Length,
                                    (UINT8 *) FtwHeader
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Update Header->WriteAllocated as VALID
  //
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
            FtwDevice->FtwWorkSpaceLba,
            FtwDevice->FtwWorkSpaceBase + Offset,
            WRITES_ALLOCATED
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  DEBUG (
    (EFI_D_ERROR,
    "Ftw: Allocate() success, Caller:%g, # %d\n",
    CallerId,
    NumberOfWrites)
    );

  return EFI_SUCCESS;
}


/**
  Write a record with fault tolerant mannaer.
  Since the content has already backuped in spare block, the write is
  guaranteed to be completed with fault tolerant manner.

  @param This            The pointer to this protocol instance. 
  @param Fvb             The FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwWriteRecord (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb
  )
{
  EFI_STATUS                      Status;
  EFI_FTW_DEVICE                  *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD *Record;
  UINTN                           Offset;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  //
  // Spare Complete but Destination not complete,
  // Recover the targt block with the spare block.
  //
  Header  = FtwDevice->FtwLastWriteHeader;
  Record  = FtwDevice->FtwLastWriteRecord;

  //
  // IF target block is working block, THEN Flush Spare Block To Working Block;
  // ELSE flush spare block to target block, which may be boot block after all.
  //
  if (IsWorkingBlock (FtwDevice, Fvb, Record->Lba)) {
    //
    // If target block is working block,
    // it also need to set SPARE_COMPLETED to spare block.
    //
    Offset = (UINT8 *) Record - FtwDevice->FtwWorkSpace;
    Status = FtwUpdateFvState (
              FtwDevice->FtwBackupFvb,
              FtwDevice->FtwWorkSpaceLba,
              FtwDevice->FtwWorkSpaceBase + Offset,
              SPARE_COMPLETED
              );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }

    Status = FlushSpareBlockToWorkingBlock (FtwDevice);
  } else if (IsBootBlock (FtwDevice, Fvb, Record->Lba)) {
    //
    // Update boot block
    //
    Status = FlushSpareBlockToBootBlock (FtwDevice);
  } else {
    //
    // Update blocks other than working block or boot block
    //
    Status = FlushSpareBlockToTargetBlock (FtwDevice, Fvb, Record->Lba);
  }

  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Record the DestionationComplete in record
  //
  Offset = (UINT8 *) Record - FtwDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
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
    Offset = (UINT8 *) Header - FtwDevice->FtwWorkSpace;
    Status = FtwUpdateFvState (
              FtwDevice->FtwFvBlock,
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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_LBA                               Lba,
  IN UINTN                                 Offset,
  IN UINTN                                 Length,
  IN VOID                                  *PrivateData,
  IN EFI_HANDLE                            FvBlockHandle,
  IN VOID                                  *Buffer
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

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status    = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header  = FtwDevice->FtwLastWriteHeader;
  Record  = FtwDevice->FtwLastWriteRecord;
  
  if (IsErasedFlashBuffer ((UINT8 *) Header, sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER))) {
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
      DEBUG ((EFI_D_ERROR, "Ftw: no allocates space for write record!\n"));
      DEBUG ((EFI_D_ERROR, "Ftw: Allocate service should be called before Write service!\n"));
      return EFI_NOT_READY;
    }
  }

  //
  // If Record is out of the range of Header, return access denied.
  //
  if (((UINTN)((UINT8 *) Record - (UINT8 *) Header)) > WRITE_TOTAL_SIZE (Header->NumberOfWrites - 1, Header->PrivateDataSize)) {
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
  // Check if the input data can fit within the target block
  //
  if ((Offset + Length) > FtwDevice->SpareAreaLength) {
    return EFI_BAD_BUFFER_SIZE;
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
    DEBUG ((EFI_D_ERROR, "FtwLite: Get FVB physical address - %r\n", Status));
    return EFI_ABORTED;
  }

  //
  // Set BootBlockUpdate FLAG if it's updating boot block.
  //
  if (IsBootBlock (FtwDevice, Fvb, Lba)) {
    Record->BootBlockUpdate = FTW_VALID_STATE;
  }
  //
  // Write the record to the work space.
  //
  Record->Lba     = Lba;
  Record->Offset  = Offset;
  Record->Length  = Length;
  Record->FvBaseAddress = FvbPhysicalAddress;
  CopyMem ((Record + 1), PrivateData, Header->PrivateDataSize);

  MyOffset  = (UINT8 *) Record - FtwDevice->FtwWorkSpace;
  MyLength  = RECORD_SIZE (Header->PrivateDataSize);

  Status = FtwDevice->FtwFvBlock->Write (
                                    FtwDevice->FtwFvBlock,
                                    FtwDevice->FtwWorkSpaceLba,
                                    FtwDevice->FtwWorkSpaceBase + MyOffset,
                                    &MyLength,
                                    (UINT8 *) Record
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
  MyBufferSize  = FtwDevice->SpareAreaLength;
  MyBuffer      = AllocatePool (MyBufferSize);
  if (MyBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Read all original data from target block to memory buffer
  //
  Ptr = MyBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    MyLength  = FtwDevice->BlockSize;
    Status    = Fvb->Read (Fvb, Lba + Index, 0, &MyLength, Ptr);
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
    MyLength = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Read (
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
  //
  Status  = FtwEraseSpareBlock (FtwDevice);
  Ptr     = MyBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwDevice->BlockSize;
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

    Ptr += MyLength;
  }
  //
  // Free MyBuffer
  //
  FreePool (MyBuffer);

  //
  // Set the SpareComplete in the FTW record,
  //
  MyOffset = (UINT8 *) Record - FtwDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
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
  Status = FtwWriteRecord (This, Fvb);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }
  //
  // Restore spare backup buffer into spare block , if no failure happened during FtwWrite.
  //
  Status  = FtwEraseSpareBlock (FtwDevice);
  Ptr     = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Write (
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
    (EFI_D_ERROR,
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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_HANDLE                            FvBlockHandle
  )
{
  EFI_STATUS                          Status;
  EFI_FTW_DEVICE                      *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER     *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD     *Record;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status    = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header  = FtwDevice->FtwLastWriteHeader;
  Record  = FtwDevice->FtwLastWriteRecord;

  //
  // Spare Complete but Destination not complete,
  // Recover the targt block with the spare block.
  //
  Status = FtwGetFvbByHandle (FvBlockHandle, &Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
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
  Status = FtwWriteRecord (This, Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Erase Spare block
  // This is restart, no need to keep spareblock content.
  //
  FtwEraseSpareBlock (FtwDevice);

  DEBUG ((EFI_D_ERROR, "Ftw: Restart() success \n"));
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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This
  )
{
  EFI_STATUS      Status;
  UINTN           Offset;
  EFI_FTW_DEVICE  *FtwDevice;

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status    = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  if (FtwDevice->FtwLastWriteHeader->Complete == FTW_VALID_STATE) {
    return EFI_NOT_FOUND;
  }
  //
  // Update the complete state of the header as VALID and abort.
  //
  Offset = (UINT8 *) FtwDevice->FtwLastWriteHeader - FtwDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
            FtwDevice->FtwWorkSpaceLba,
            FtwDevice->FtwWorkSpaceBase + Offset,
            WRITES_COMPLETED
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwDevice->FtwLastWriteHeader->Complete = FTW_VALID_STATE;

  DEBUG ((EFI_D_ERROR, "Ftw: Abort() success \n"));
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
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  OUT EFI_GUID                             *CallerId,
  OUT EFI_LBA                              *Lba,
  OUT UINTN                                *Offset,
  OUT UINTN                                *Length,
  IN OUT UINTN                             *PrivateDataSize,
  OUT VOID                                 *PrivateData,
  OUT BOOLEAN                              *Complete
  )
{
  EFI_STATUS                      Status;
  EFI_FTW_DEVICE                  *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER *Header;
  EFI_FAULT_TOLERANT_WRITE_RECORD *Record;

  if (!FeaturePcdGet(PcdFullFtwServiceEnable)) {
    return EFI_UNSUPPORTED;
  }

  FtwDevice = FTW_CONTEXT_FROM_THIS (This);

  Status    = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Header  = FtwDevice->FtwLastWriteHeader;
  Record  = FtwDevice->FtwLastWriteRecord;

  //
  // If Header is incompleted and the last record has completed, then
  // call Abort() to set the Header->Complete FLAG.
  //
  if ((Header->Complete != FTW_VALID_STATE) &&
      (Record->DestinationComplete == FTW_VALID_STATE) &&
      IsLastRecordOfWrites (Header, Record)
        ) {

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
    if (IsFirstRecordOfWrites (Header, Record)) {
      //
      // The First record cannot be restart and target is still healthy,
      // so abort() is a safe solution.
      //
      FtwAbort (This);

      *Complete = TRUE;
      return EFI_NOT_FOUND;
    } else {
      //
      // Step back to the previous record
      //
      GetPreviousRecordOfWrites (Header, &Record);
    }
  }
  //
  // Fill all the requested values
  //
  CopyMem (CallerId, &Header->CallerId, sizeof (EFI_GUID));
  *Lba      = Record->Lba;
  *Offset   = Record->Offset;
  *Length   = Record->Length;
  *Complete = (BOOLEAN) (Record->DestinationComplete == FTW_VALID_STATE);

  if (*PrivateDataSize < Header->PrivateDataSize) {
    *PrivateDataSize  = Header->PrivateDataSize;
    PrivateData       = NULL;
    Status            = EFI_BUFFER_TOO_SMALL;
  } else {
    *PrivateDataSize = Header->PrivateDataSize;
    CopyMem (PrivateData, Record + 1, *PrivateDataSize);
    Status = EFI_SUCCESS;
  }

  DEBUG ((EFI_D_ERROR, "Ftw: GetLasetWrite() success\n"));

  return Status;
}

/**
  This function is the entry point of the Fault Tolerant Write driver.

  @param ImageHandle     A handle for the image that is initializing this driver
  @param SystemTable     A pointer to the EFI system table

  @return EFI_SUCCESS           FTW has finished the initialization
  @retval EFI_NOT_FOUND         Locate FVB protocol error
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_VOLUME_CORRUPTED  Firmware volume is error
  @retval EFI_ABORTED           FTW initialization error

**/
EFI_STATUS
EFIAPI
InitializeFaultTolerantWrite (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINTN                               Index;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  EFI_FTW_DEVICE                      *FtwDevice;
  EFI_FAULT_TOLERANT_WRITE_HEADER     *FtwHeader;
  UINTN                               Length;
  EFI_STATUS                          Status;
  UINTN                               Offset;
  EFI_FV_BLOCK_MAP_ENTRY              *FvbMapEntry;
  UINT32                              LbaIndex;
  EFI_HANDLE                          FvbHandle;

  //
  // Allocate Private data of this driver,
  // INCLUDING THE FtwWorkSpace[FTW_WORK_SPACE_SIZE].
  //
  FvbHandle = NULL;
  FtwDevice = NULL;
  FtwDevice = AllocatePool (sizeof (EFI_FTW_DEVICE) + PcdGet32 (PcdFlashNvStorageFtwWorkingSize));
  if (FtwDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (FtwDevice, sizeof (EFI_FTW_DEVICE));
  FtwDevice->Signature = FTW_DEVICE_SIGNATURE;

  //
  // Initialize other parameters, and set WorkSpace as FTW_ERASED_BYTE.
  //

  FtwDevice->WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  FtwDevice->WorkSpaceLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwWorkingSize);

  FtwDevice->SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  FtwDevice->SpareAreaLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  if ((FtwDevice->WorkSpaceLength == 0) || (FtwDevice->SpareAreaLength == 0)) {
    DEBUG ((EFI_D_ERROR, "Ftw: Workspace or Spare block does not exist!\n"));
    FreePool (FtwDevice);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Locate FVB protocol by handle
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    FreePool (FtwDevice);
    return EFI_NOT_FOUND;
  }

  if (HandleCount <= 0) {
    FreePool (FtwDevice);
    return EFI_NOT_FOUND;
  }

  Fvb                         = NULL;
  FtwDevice->FtwFvBlock       = NULL;
  FtwDevice->FtwBackupFvb     = NULL;
  FtwDevice->FtwWorkSpaceLba  = (EFI_LBA) (-1);
  FtwDevice->FtwSpareLba      = (EFI_LBA) (-1);
  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &Fvb
                    );
    if (EFI_ERROR (Status)) {
      FreePool (FtwDevice);
      return Status;
    }

    Status = Fvb->GetPhysicalAddress (Fvb, &BaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) BaseAddress);

    if ((FtwDevice->WorkSpaceAddress >= BaseAddress) &&
        ((FtwDevice->WorkSpaceAddress + FtwDevice->WorkSpaceLength) <= (BaseAddress + FwVolHeader->FvLength))
        ) {
      FtwDevice->FtwFvBlock = Fvb;
      //
      // To get the LBA of work space
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // Now, one FV has one type of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
            if ((FtwDevice->WorkSpaceAddress >= (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwDevice->WorkSpaceAddress < (BaseAddress + FvbMapEntry->Length * LbaIndex))) {
            FtwDevice->FtwWorkSpaceLba = LbaIndex - 1;
            //
            // Get the Work space size and Base(Offset)
            //
            FtwDevice->FtwWorkSpaceSize = FtwDevice->WorkSpaceLength;
            FtwDevice->FtwWorkSpaceBase = (UINTN) (FtwDevice->WorkSpaceAddress - (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)));
            break;
          }
        }
      }
    }

    if ((FtwDevice->SpareAreaAddress >= BaseAddress) &&
        ((FtwDevice->SpareAreaAddress + FtwDevice->SpareAreaLength) <= (BaseAddress + FwVolHeader->FvLength))
        ) {
      FtwDevice->FtwBackupFvb = Fvb;
      //
      // To get the LBA of spare
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // Now, one FV has one type of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
            if ((FtwDevice->SpareAreaAddress >= (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwDevice->SpareAreaAddress < (BaseAddress + FvbMapEntry->Length * LbaIndex))) {
            //
            // Get the NumberOfSpareBlock and BlockSize
            //
            FtwDevice->FtwSpareLba        = LbaIndex - 1;
            FtwDevice->BlockSize          = FvbMapEntry->Length;
            FtwDevice->NumberOfSpareBlock = FtwDevice->SpareAreaLength / FtwDevice->BlockSize;
            //
            // Check the range of spare area to make sure that it's in FV range
            //
            if ((FtwDevice->FtwSpareLba + FtwDevice->NumberOfSpareBlock) > FvbMapEntry->NumBlocks) {
              DEBUG ((EFI_D_ERROR, "Ftw: Spare area is out of FV range\n"));
              FreePool (FtwDevice);
              return EFI_ABORTED;
            }
            break;
          }
        }
      }
    }
  }

  //
  // Calculate the start LBA of working block. Working block is an area which
  // contains working space in its last block and has the same size as spare
  // block, unless there are not enough blocks before the block that contains
  // working space.
  //
  FtwDevice->FtwWorkBlockLba = FtwDevice->FtwWorkSpaceLba - FtwDevice->NumberOfSpareBlock + 1;
  if ((INT64) (FtwDevice->FtwWorkBlockLba) < 0) {
    DEBUG ((EFI_D_ERROR, "Ftw: The spare block range is too large than the working block range!\n"));
    FreePool (FtwDevice);
    return EFI_ABORTED;
  }

  if ((FtwDevice->FtwFvBlock == NULL) ||
      (FtwDevice->FtwBackupFvb == NULL) ||
      (FtwDevice->FtwWorkSpaceLba == (EFI_LBA) (-1)) ||
      (FtwDevice->FtwSpareLba == (EFI_LBA) (-1))
      ) {
    DEBUG ((EFI_D_ERROR, "Ftw: Working or spare FVB not ready\n"));
    FreePool (FtwDevice);
    return EFI_ABORTED;
  }
  //
  // Initialize other parameters, and set WorkSpace as FTW_ERASED_BYTE.
  //
  FtwDevice->FtwWorkSpace = (UINT8 *) (FtwDevice + 1);
  FtwDevice->FtwWorkSpaceHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) FtwDevice->FtwWorkSpace;

  FtwDevice->FtwLastWriteHeader = NULL;
  FtwDevice->FtwLastWriteRecord = NULL;

  //
  // Refresh the working space data from working block
  //
  Status = WorkSpaceRefresh (FtwDevice);
  if (EFI_ERROR (Status)) {
    goto Recovery;
  }
  //
  // If the working block workspace is not valid, try the spare block
  //
  if (!IsValidWorkSpace (FtwDevice->FtwWorkSpaceHeader)) {
    //
    // Read from spare block
    //
    Length = FtwDevice->FtwWorkSpaceSize;
    Status = FtwDevice->FtwBackupFvb->Read (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba,
                                        FtwDevice->FtwWorkSpaceBase,
                                        &Length,
                                        FtwDevice->FtwWorkSpace
                                        );
    if (EFI_ERROR (Status)) {
      goto Recovery;
    }
    //
    // If spare block is valid, then replace working block content.
    //
    if (IsValidWorkSpace (FtwDevice->FtwWorkSpaceHeader)) {
      Status = FlushSpareBlockToWorkingBlock (FtwDevice);
      DEBUG ((EFI_D_ERROR, "Ftw: Restart working block update in Init() - %r\n", Status));
      FtwAbort (&FtwDevice->FtwInstance);
      //
      // Refresh work space.
      //
      Status = WorkSpaceRefresh (FtwDevice);
      if (EFI_ERROR (Status)) {
        goto Recovery;
      }
    } else {
      DEBUG ((EFI_D_ERROR, "Ftw: Both are invalid, init workspace\n"));
      //
      // If both are invalid, then initialize work space.
      //
      SetMem (
        FtwDevice->FtwWorkSpace,
        FtwDevice->FtwWorkSpaceSize,
        FTW_ERASED_BYTE
        );
      InitWorkSpaceHeader (FtwDevice->FtwWorkSpaceHeader);
      //
      // Initialize the work space
      //
      Status = FtwReclaimWorkSpace (FtwDevice, FALSE);
      if (EFI_ERROR (Status)) {
        goto Recovery;
      }
    }
  }

  //
  // If the FtwDevice->FtwLastWriteRecord is 1st record of write header &&
  // (! SpareComplete)  THEN  call Abort().
  //
  if ((FtwDevice->FtwLastWriteHeader->HeaderAllocated == FTW_VALID_STATE) &&
      (FtwDevice->FtwLastWriteRecord->SpareComplete != FTW_VALID_STATE) &&
      IsFirstRecordOfWrites (FtwDevice->FtwLastWriteHeader, FtwDevice->FtwLastWriteRecord)
        ) {
    DEBUG ((EFI_D_ERROR, "Ftw: Init.. find first record not SpareCompleted, abort()\n"));
    FtwAbort (&FtwDevice->FtwInstance);
  }
  //
  // If Header is incompleted and the last record has completed, then
  // call Abort() to set the Header->Complete FLAG.
  //
  if ((FtwDevice->FtwLastWriteHeader->Complete != FTW_VALID_STATE) &&
      (FtwDevice->FtwLastWriteRecord->DestinationComplete == FTW_VALID_STATE) &&
      IsLastRecordOfWrites (FtwDevice->FtwLastWriteHeader, FtwDevice->FtwLastWriteRecord)
        ) {
    DEBUG ((EFI_D_ERROR, "Ftw: Init.. find last record completed but header not, abort()\n"));
    FtwAbort (&FtwDevice->FtwInstance);
  }
  //
  // To check the workspace buffer following last Write header/records is EMPTY or not.
  // If it's not EMPTY, FTW also need to call reclaim().
  //
  FtwHeader = FtwDevice->FtwLastWriteHeader;
  Offset    = (UINT8 *) FtwHeader - FtwDevice->FtwWorkSpace;
  if (FtwDevice->FtwWorkSpace[Offset] != FTW_ERASED_BYTE) {
    Offset += WRITE_TOTAL_SIZE (FtwHeader->NumberOfWrites, FtwHeader->PrivateDataSize);
  }

  if (!IsErasedFlashBuffer (
        FtwDevice->FtwWorkSpace + Offset,
        FtwDevice->FtwWorkSpaceSize - Offset
        )) {
    Status = FtwReclaimWorkSpace (FtwDevice, TRUE);
    if (EFI_ERROR (Status)) {
      goto Recovery;
    }
  }
  //
  // Restart if it's boot block
  //
  if ((FtwDevice->FtwLastWriteHeader->Complete != FTW_VALID_STATE) &&
      (FtwDevice->FtwLastWriteRecord->SpareComplete == FTW_VALID_STATE)
      ) {
    if (FtwDevice->FtwLastWriteRecord->BootBlockUpdate == FTW_VALID_STATE) {
      Status = FlushSpareBlockToBootBlock (FtwDevice);
      DEBUG ((EFI_D_ERROR, "Ftw: Restart boot block update - %r\n", Status));
      if (EFI_ERROR (Status)) {
        goto Recovery;
      }
  
      FtwAbort (&FtwDevice->FtwInstance);
    } else {
      //
      // if (SpareCompleted) THEN  Restart to fault tolerant write.
      //
      FvbHandle = GetFvbByAddress (FtwDevice->FtwLastWriteRecord->FvBaseAddress, &Fvb);
      if (FvbHandle != NULL) {
        Status = FtwRestart (&FtwDevice->FtwInstance, FvbHandle);
        DEBUG ((EFI_D_ERROR, "FtwLite: Restart last write - %r\n", Status));
        if (EFI_ERROR (Status)) {
          goto Recovery;
        }
      }
      FtwAbort (&FtwDevice->FtwInstance);
    }
  }

  //
  // Hook the protocol API
  //
  FtwDevice->FtwInstance.GetMaxBlockSize  = FtwGetMaxBlockSize;
  FtwDevice->FtwInstance.Allocate         = FtwAllocate;
  FtwDevice->FtwInstance.Write            = FtwWrite;
  FtwDevice->FtwInstance.Restart          = FtwRestart;
  FtwDevice->FtwInstance.Abort            = FtwAbort;
  FtwDevice->FtwInstance.GetLastWrite     = FtwGetLastWrite;

  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &FtwDevice->Handle,
                  &gEfiFaultTolerantWriteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FtwDevice->FtwInstance
                  );
  if (EFI_ERROR (Status)) {
    goto Recovery;
  }

  return EFI_SUCCESS;

Recovery:

  if (FtwDevice != NULL) {
    FreePool (FtwDevice);
  }

  DEBUG ((EFI_D_ERROR, "Ftw: Severe Error occurs, need to recovery\n"));

  return EFI_VOLUME_CORRUPTED;
}
