/** @file

  This is a simple fault tolerant write driver.

  This boot service protocol only provides fault tolerant write capability for 
  block devices.  The protocol has internal non-volatile intermediate storage 
  of the data and private information. It should be able to recover 
  automatically from a critical fault, such as power failure. 

  The implementation uses an FTW Lite (Fault Tolerant Write) Work Space. 
  This work space is a memory copy of the work space on the Working Block,
  the size of the work space is the FTW_WORK_SPACE_SIZE bytes.
  
  The work space stores each write record as EFI_FTW_LITE_RECORD structure.
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
  If one of them is not satisfied, FtwLiteWrite may fail.
  Usually, Spare area only takes one block. That's SpareAreaLength = BlockSize, NumberOfSpareBlock = 1.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#include "FtwLite.h"

/**
  Starts a target block update. This function will record data about write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param This            The pointer to this protocol instance. 
  @param FvbHandle       The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param Lba             The logical block address of the target block.
  @param Offset          The offset within the target block to place the data.
  @param NumBytes        The number of bytes to write to the target block.
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
FtwLiteWrite (
  IN EFI_FTW_LITE_PROTOCOL                 *This,
  IN EFI_HANDLE                            FvbHandle,
  IN EFI_LBA                               Lba,
  IN UINTN                                 Offset,
  IN OUT UINTN                             *NumBytes,
  IN VOID                                  *Buffer
  )
{
  EFI_STATUS                          Status;
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice;
  EFI_FTW_LITE_RECORD                 *Record;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_PHYSICAL_ADDRESS                FvbPhysicalAddress;
  UINTN                               MyLength;
  UINTN                               MyOffset;
  UINTN                               MyBufferSize;
  UINT8                               *MyBuffer;
  UINTN                               SpareBufferSize;
  UINT8                               *SpareBuffer;
  UINTN                               Index;
  UINT8                               *Ptr;
  EFI_DEV_PATH_PTR                    DevPtr;
  //
  // Refresh work space and get last record
  //
  FtwLiteDevice = FTW_LITE_CONTEXT_FROM_THIS (This);
  Status        = WorkSpaceRefresh (FtwLiteDevice);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Record = FtwLiteDevice->FtwLastRecord;

  //
  // Check the flags of last write record
  //
  if ((Record->WriteAllocated == FTW_VALID_STATE) || (Record->SpareCompleted == FTW_VALID_STATE)) {
    return EFI_ACCESS_DENIED;
  }
  //
  // IF former record has completed, THEN use next record
  //
  if (Record->WriteCompleted == FTW_VALID_STATE) {
    Record++;
    FtwLiteDevice->FtwLastRecord = Record;
  }

  MyOffset = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;

  //
  // Check if the input data can fit within the target block
  //
  if ((Offset +*NumBytes) > FtwLiteDevice->SpareAreaLength) {
    *NumBytes = FtwLiteDevice->SpareAreaLength - Offset;
    return EFI_BAD_BUFFER_SIZE;
  }
  //
  // Check if there is enough free space for allocate a record
  //
  if ((MyOffset + FTW_LITE_RECORD_SIZE) > FtwLiteDevice->FtwWorkSpaceSize) {
    Status = FtwReclaimWorkSpace (FtwLiteDevice, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "FtwLite: Reclaim work space - %r", Status));
      return EFI_ABORTED;
    }
  }
  //
  // Get the FVB protocol by handle
  //
  Status = FtwGetFvbByHandle (FvbHandle, &Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Allocate a write record in workspace.
  // Update Header->WriteAllocated as VALID
  //
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + MyOffset,
            WRITE_ALLOCATED
            );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Allocate record - %r\n", Status));
    return EFI_ABORTED;
  }

  Record->WriteAllocated = FTW_VALID_STATE;

  //
  // Prepare data of write record, filling DevPath with memory mapped address.
  //
  DevPtr.MemMap                 = (MEMMAP_DEVICE_PATH *) &Record->DevPath;
  DevPtr.MemMap->Header.Type    = HARDWARE_DEVICE_PATH;
  DevPtr.MemMap->Header.SubType = HW_MEMMAP_DP;
  SetDevicePathNodeLength (&DevPtr.MemMap->Header, sizeof (MEMMAP_DEVICE_PATH));

  Status = Fvb->GetPhysicalAddress (Fvb, &FvbPhysicalAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Get FVB physical address - %r\n", Status));
    return EFI_ABORTED;
  }

  DevPtr.MemMap->MemoryType       = EfiMemoryMappedIO;
  DevPtr.MemMap->StartingAddress  = FvbPhysicalAddress;
  DevPtr.MemMap->EndingAddress    = FvbPhysicalAddress +*NumBytes;
  //
  // ignored!
  //
  Record->Lba       = Lba;
  Record->Offset    = Offset;
  Record->NumBytes  = *NumBytes;

  //
  // Write the record to the work space.
  //
  MyOffset  = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;
  MyLength  = FTW_LITE_RECORD_SIZE;

  Status = FtwLiteDevice->FtwFvBlock->Write (
                                        FtwLiteDevice->FtwFvBlock,
                                        FtwLiteDevice->FtwWorkSpaceLba,
                                        FtwLiteDevice->FtwWorkSpaceBase + MyOffset,
                                        &MyLength,
                                        (UINT8 *) Record
                                        );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Record has been written to working block, then write data.
  //
  //
  // Allocate a memory buffer
  //
  MyBufferSize  = FtwLiteDevice->SpareAreaLength;
  MyBuffer      = AllocatePool (MyBufferSize);
  if (MyBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Starting at Lba, if the number of the rest blocks on Fvb is less
  // than NumberOfSpareBlock.
  //
  //
  // Read all original data from target block to memory buffer
  //
  if (IsInWorkingBlock (FtwLiteDevice, Fvb, Lba)) {
    //
    // If target block falls into working block, we must follow the process of
    // updating working block.
    //
    Ptr = MyBuffer;
    for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
      MyLength = FtwLiteDevice->BlockSize;
      Status = FtwLiteDevice->FtwFvBlock->Read (
                                            FtwLiteDevice->FtwFvBlock,
                                            FtwLiteDevice->FtwWorkBlockLba + Index,
                                            0,
                                            &MyLength,
                                            Ptr
                                            );
      if (EFI_ERROR (Status)) {
        FreePool (MyBuffer);
        return EFI_ABORTED;
      }

      Ptr += MyLength;
    }
    //
    // Update Offset by adding the offset from the start LBA of working block to
    // the target LBA. The target block can not span working block!
    //
    Offset = (((UINTN) (Lba - FtwLiteDevice->FtwWorkBlockLba)) * FtwLiteDevice->BlockSize + Offset);
    ASSERT ((Offset +*NumBytes) <= FtwLiteDevice->SpareAreaLength);

  } else {

    Ptr = MyBuffer;
    for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
      MyLength  = FtwLiteDevice->BlockSize;
      Status    = Fvb->Read (Fvb, Lba + Index, 0, &MyLength, Ptr);
      if (EFI_ERROR (Status)) {
        FreePool (MyBuffer);
        return EFI_ABORTED;
      }

      Ptr += MyLength;
    }
  }
  //
  // Overwrite the updating range data with
  // the input buffer content
  //
  CopyMem (MyBuffer + Offset, Buffer, *NumBytes);

  //
  // Try to keep the content of spare block
  // Save spare block into a spare backup memory buffer (Sparebuffer)
  //
  SpareBufferSize = FtwLiteDevice->SpareAreaLength;
  SpareBuffer     = AllocatePool (SpareBufferSize);
  if (SpareBuffer == NULL) {
    FreePool (MyBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = SpareBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwLiteDevice->BlockSize;
    Status = FtwLiteDevice->FtwBackupFvb->Read (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
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
  // Don't forget to erase Flash first.
  //
  Status  = FtwEraseSpareBlock (FtwLiteDevice);
  Ptr     = MyBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwLiteDevice->BlockSize;
    Status = FtwLiteDevice->FtwBackupFvb->Write (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
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
  MyOffset = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + MyOffset,
            SPARE_COMPLETED
            );
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  Record->SpareCompleted = FTW_VALID_STATE;

  //
  //  Since the content has already backuped in spare block, the write is
  //  guaranteed to be completed with fault tolerant manner.
  //
  Status = FtwWriteRecord (FtwLiteDevice, Fvb);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  Record++;
  FtwLiteDevice->FtwLastRecord = Record;

  //
  // Restore spare backup buffer into spare block , if no failure happened during FtwWrite.
  //
  Status  = FtwEraseSpareBlock (FtwLiteDevice);
  Ptr     = SpareBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    MyLength = FtwLiteDevice->BlockSize;
    Status = FtwLiteDevice->FtwBackupFvb->Write (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
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
    "FtwLite: Write() success, (Lba:Offset)=(%lx:0x%x), NumBytes: 0x%x\n",
    Lba,
    Offset,
    *NumBytes)
    );

  return EFI_SUCCESS;
}


/**
  Write a record with fault tolerant manner.
  Since the content has already backuped in spare block, the write is
  guaranteed to be completed with fault tolerant manner.


  @param FtwLiteDevice   The private data of FTW_LITE driver
  @param Fvb             The FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwWriteRecord (
  IN EFI_FTW_LITE_DEVICE                   *FtwLiteDevice,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb
  )
{
  EFI_STATUS          Status;
  EFI_FTW_LITE_RECORD *Record;
  EFI_LBA             WorkSpaceLbaOffset;  
  UINTN               Offset;

  //
  // Spare Complete but Destination not complete,
  // Recover the targt block with the spare block.
  //
  Record = FtwLiteDevice->FtwLastRecord;

  //
  // IF target block is working block, THEN Flush Spare Block To Working Block;
  // ELSE flush spare block to normal target block.ENDIF
  //
  if (IsInWorkingBlock (FtwLiteDevice, Fvb, Record->Lba)) {
    //
    // If target block is working block, Attention:
    // it's required to set SPARE_COMPLETED to spare block.
    //
    WorkSpaceLbaOffset = FtwLiteDevice->FtwWorkSpaceLba - FtwLiteDevice->FtwWorkBlockLba;
    Offset = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;
    Status = FtwUpdateFvState (
              FtwLiteDevice->FtwBackupFvb,
              FtwLiteDevice->FtwSpareLba + WorkSpaceLbaOffset,
              FtwLiteDevice->FtwWorkSpaceBase + Offset,
              SPARE_COMPLETED
              );
    ASSERT_EFI_ERROR (Status);

    Status = FlushSpareBlockToWorkingBlock (FtwLiteDevice);
  } else {
    //
    // Update blocks other than working block
    //
    Status = FlushSpareBlockToTargetBlock (FtwLiteDevice, Fvb, Record->Lba);
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Set WriteCompleted flag in record
  //
  Offset = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + Offset,
            WRITE_COMPLETED
            );
  ASSERT_EFI_ERROR (Status);

  Record->WriteCompleted = FTW_VALID_STATE;
  return EFI_SUCCESS;
}


/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.


  @param FtwLiteDevice   The private data of FTW_LITE driver
                         FvbHandle           - The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ACCESS_DENIED    No pending writes exist
  @retval  EFI_NOT_FOUND        FVB protocol not found by the handle
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwRestart (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  )
{
  EFI_STATUS                          Status;
  EFI_FTW_LITE_RECORD                 *Record;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_DEV_PATH_PTR                    DevPathPtr;

  //
  // Spare Completed but Destination not complete,
  // Recover the targt block with the spare block.
  //
  Record = FtwLiteDevice->FtwLastRecord;

  //
  // Only support memory mapped FVB device path by now.
  //
  DevPathPtr.MemMap = (MEMMAP_DEVICE_PATH *) &Record->DevPath;
  if (!((DevPathPtr.MemMap->Header.Type == HARDWARE_DEVICE_PATH) && (DevPathPtr.MemMap->Header.SubType == HW_MEMMAP_DP))
      ) {
    DEBUG ((EFI_D_ERROR, "FtwLite: FVB Device Path is not memory mapped\n"));
    return EFI_ABORTED;
  }

  Status = GetFvbByAddress (DevPathPtr.MemMap->StartingAddress, &Fvb);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  //  Since the content has already backuped in spare block, the write is
  //  guaranteed to be completed with fault tolerant manner.
  //
  Status = FtwWriteRecord (FtwLiteDevice, Fvb);
  DEBUG ((EFI_D_INFO, "FtwLite: Restart() - %r\n", Status));

  Record++;
  FtwLiteDevice->FtwLastRecord = Record;

  //
  // Erase Spare block
  // This is restart, no need to keep spareblock content.
  //
  FtwEraseSpareBlock (FtwLiteDevice);

  return Status;
}


/**
  Aborts all previous allocated writes.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS       The function completed successfully
  @retval  EFI_ABORTED       The function could not complete successfully.
  @retval  EFI_NOT_FOUND     No allocated writes exist.

**/
EFI_STATUS
FtwAbort (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;

  if (FtwLiteDevice->FtwLastRecord->WriteCompleted == FTW_VALID_STATE) {
    return EFI_NOT_FOUND;
  }
  //
  // Update the complete state of the header as VALID and abort.
  //
  Offset = (UINT8 *) FtwLiteDevice->FtwLastRecord - FtwLiteDevice->FtwWorkSpace;
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + Offset,
            WRITE_COMPLETED
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwLiteDevice->FtwLastRecord->WriteCompleted  = FTW_VALID_STATE;

  Status = FtwGetLastRecord (FtwLiteDevice, &FtwLiteDevice->FtwLastRecord);

  //
  // Erase the spare block
  //
  Status = FtwEraseSpareBlock (FtwLiteDevice);

  DEBUG ((EFI_D_INFO, "FtwLite: Abort() success \n"));
  return EFI_SUCCESS;
}

/**
  This function is the entry point of the Fault Tolerant Write driver.

  @param ImageHandle     A handle for the image that is initializing this driver
  @param SystemTable     A pointer to the EFI system table

  @retval  EFI_SUCCESS            FTW has finished the initialization
  @retval  EFI_ABORTED            FTW initialization error

**/
EFI_STATUS
EFIAPI
InitializeFtwLite (
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
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice;
  EFI_FTW_LITE_RECORD                 *Record;
  EFI_STATUS                          Status;
  UINTN                               Offset;
  UINTN                               Length;
  EFI_FV_BLOCK_MAP_ENTRY              *FvbMapEntry;
  UINT32                              LbaIndex;
  //
  // Allocate Private data of this driver, including the FtwWorkSpace[FTW_WORK_SPACE_SIZE].
  //
  FtwLiteDevice = NULL;
  FtwLiteDevice = AllocatePool (sizeof (EFI_FTW_LITE_DEVICE) + PcdGet32 (PcdFlashNvStorageFtwWorkingSize));
  ASSERT (FtwLiteDevice != NULL);

  ZeroMem (FtwLiteDevice, sizeof (EFI_FTW_LITE_DEVICE));
  FtwLiteDevice->Signature = FTW_LITE_DEVICE_SIGNATURE;

  //
  // Initialize other parameters, and set WorkSpace as FTW_ERASED_BYTE.
  //
  FtwLiteDevice->FtwWorkSpace     = (UINT8 *) (FtwLiteDevice + 1);
  FtwLiteDevice->FtwWorkSpaceHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) FtwLiteDevice->FtwWorkSpace;
  FtwLiteDevice->FtwLastRecord      = NULL;

  FtwLiteDevice->WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  FtwLiteDevice->WorkSpaceLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwWorkingSize);

  FtwLiteDevice->SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  FtwLiteDevice->SpareAreaLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  ASSERT ((FtwLiteDevice->WorkSpaceLength != 0) && (FtwLiteDevice->SpareAreaLength != 0));

  //
  // Locate FVB protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT_EFI_ERROR (Status);

  ASSERT (HandleCount > 0);

  FtwLiteDevice->FtwFvBlock       = NULL;
  FtwLiteDevice->FtwBackupFvb     = NULL;
  FtwLiteDevice->FtwWorkSpaceLba  = (EFI_LBA) (-1);
  FtwLiteDevice->FtwSpareLba      = (EFI_LBA) (-1);
  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &Fvb
                    );
    ASSERT_EFI_ERROR (Status);

    Status = Fvb->GetPhysicalAddress (Fvb, &BaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) BaseAddress);

    if ((FtwLiteDevice->WorkSpaceAddress >= BaseAddress) &&
        ((FtwLiteDevice->WorkSpaceAddress + FtwLiteDevice->WorkSpaceLength) <= (BaseAddress + FwVolHeader->FvLength))
        ) {
      FtwLiteDevice->FtwFvBlock = Fvb;
      //
      // To get the LBA of work space
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // FV may have multiple types of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        while (!((FvbMapEntry->NumBlocks == 0) && (FvbMapEntry->Length == 0))) {
          for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
            if ((FtwLiteDevice->WorkSpaceAddress >= (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwLiteDevice->WorkSpaceAddress < (BaseAddress + FvbMapEntry->Length * LbaIndex))) {
              FtwLiteDevice->FtwWorkSpaceLba = LbaIndex - 1;
              //
              // Get the Work space size and Base(Offset)
              //
              FtwLiteDevice->FtwWorkSpaceSize = FtwLiteDevice->WorkSpaceLength;
              FtwLiteDevice->FtwWorkSpaceBase = (UINTN) (FtwLiteDevice->WorkSpaceAddress - (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)));
              break;
            }
          }
          //
          // end for
          //
          if (LbaIndex <= FvbMapEntry->NumBlocks) {
            //
            // Work space range is found.
            //
            break;
          }
          FvbMapEntry++;
        }
        //
        // end while
        //
      }
    }

    if ((FtwLiteDevice->SpareAreaAddress >= BaseAddress) &&
        ((FtwLiteDevice->SpareAreaAddress + FtwLiteDevice->SpareAreaLength) <= (BaseAddress + FwVolHeader->FvLength))
        ) {
      FtwLiteDevice->FtwBackupFvb = Fvb;
      //
      // To get the LBA of spare
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // FV may have multiple types of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        while (!((FvbMapEntry->NumBlocks == 0) && (FvbMapEntry->Length == 0))) {
          for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
            if ((FtwLiteDevice->SpareAreaAddress >= (BaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwLiteDevice->SpareAreaAddress < (BaseAddress + FvbMapEntry->Length * LbaIndex))) {
              //
              // Get the NumberOfSpareBlock and BlockSize
              //
              FtwLiteDevice->FtwSpareLba        = LbaIndex - 1;
              FtwLiteDevice->BlockSize   = FvbMapEntry->Length;
              FtwLiteDevice->NumberOfSpareBlock = FtwLiteDevice->SpareAreaLength / FtwLiteDevice->BlockSize;
              //
              // Check the range of spare area to make sure that it's in FV range
              // To do delete
              //
              ASSERT ((FtwLiteDevice->FtwSpareLba + FtwLiteDevice->NumberOfSpareBlock) <= FvbMapEntry->NumBlocks);
              break;
            }
          }
          if (LbaIndex <= FvbMapEntry->NumBlocks) {
            //
            // Spare FV range is found.
            //
            break;
          }
          FvbMapEntry++;
        }
        //
        // end while
        //
      }
    }
  }

  //
  // Calculate the start LBA of working block. Working block is an area which
  // contains working space in its last block and has the same size as spare
  // block, unless there are not enough blocks before the block that contains
  // working space.
  //
  FtwLiteDevice->FtwWorkBlockLba = FtwLiteDevice->FtwWorkSpaceLba - FtwLiteDevice->NumberOfSpareBlock + 1;
  if ((INT64) (FtwLiteDevice->FtwWorkBlockLba) < 0) {
    DEBUG ((EFI_D_ERROR, "FtwLite: The spare block range is too large than the working block range!\n"));
    FreePool (FtwLiteDevice);
    return EFI_ABORTED;
  }

  if ((FtwLiteDevice->FtwFvBlock == NULL) ||
      (FtwLiteDevice->FtwBackupFvb == NULL) ||
      (FtwLiteDevice->FtwWorkSpaceLba == (EFI_LBA) (-1)) ||
      (FtwLiteDevice->FtwSpareLba == (EFI_LBA) (-1))
      ) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Working or spare FVB not ready\n"));
    FreePool (FtwLiteDevice);
    return EFI_ABORTED;
  }

  //
  // Refresh workspace data from working block
  //
  Status = WorkSpaceRefresh (FtwLiteDevice);
  ASSERT_EFI_ERROR (Status);

  //
  // If the working block workspace is not valid, try the spare block
  //
  if (!IsValidWorkSpace (FtwLiteDevice->FtwWorkSpaceHeader)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Workspace invalid, read from backup\n"));
    //
    // Read from spare block
    //
    Length = FtwLiteDevice->FtwWorkSpaceSize;
    Status = FtwLiteDevice->FtwBackupFvb->Read (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba,
                                            FtwLiteDevice->FtwWorkSpaceBase,
                                            &Length,
                                            FtwLiteDevice->FtwWorkSpace
                                            );
    ASSERT_EFI_ERROR (Status);

    //
    // If spare block is valid, then replace working block content.
    //
    if (IsValidWorkSpace (FtwLiteDevice->FtwWorkSpaceHeader)) {
      Status = FlushSpareBlockToWorkingBlock (FtwLiteDevice);
      DEBUG ((EFI_D_ERROR, "FtwLite: Restart working block in Init() - %r\n", Status));
      ASSERT_EFI_ERROR (Status);

      FtwAbort (FtwLiteDevice);
      //
      // Refresh work space.
      //
      Status = WorkSpaceRefresh (FtwLiteDevice);
      if (EFI_ERROR (Status)) {
        FreePool (FtwLiteDevice);
        return EFI_ABORTED;
      }
    } else {
      DEBUG ((EFI_D_ERROR, "FtwLite: Both are invalid, init workspace\n"));
      //
      // If both are invalid, then initialize work space.
      //
      SetMem (
        FtwLiteDevice->FtwWorkSpace,
        FtwLiteDevice->FtwWorkSpaceSize,
        FTW_ERASED_BYTE
        );
      InitWorkSpaceHeader (FtwLiteDevice->FtwWorkSpaceHeader);
      //
      // Initialize the work space
      //
      Status = FtwReclaimWorkSpace (FtwLiteDevice, FALSE);

      if (EFI_ERROR (Status)) {
        FreePool (FtwLiteDevice);
        return EFI_ABORTED;
      }
    }
  }
  //
  // Hook the protocol API
  //
  FtwLiteDevice->FtwLiteInstance.Write = FtwLiteWrite;

  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &FtwLiteDevice->Handle,
                  &gEfiFaultTolerantWriteLiteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FtwLiteDevice->FtwLiteInstance
                  );
  if (EFI_ERROR (Status)) {
    FreePool (FtwLiteDevice);
    return EFI_ABORTED;
  }
  //
  // If (!SpareCompleted)  THEN  Abort to rollback.
  //
  if ((FtwLiteDevice->FtwLastRecord->WriteAllocated == FTW_VALID_STATE) &&
      (FtwLiteDevice->FtwLastRecord->SpareCompleted != FTW_VALID_STATE)
      ) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Init.. record not SpareCompleted, abort()\n"));
    FtwAbort (FtwLiteDevice);
  }
  //
  // if (SpareCompleted) THEN  Restart to fault tolerant write.
  //
  if ((FtwLiteDevice->FtwLastRecord->SpareCompleted == FTW_VALID_STATE) &&
      (FtwLiteDevice->FtwLastRecord->WriteCompleted != FTW_VALID_STATE)
      ) {

    Status = FtwRestart (FtwLiteDevice);
    DEBUG ((EFI_D_ERROR, "FtwLite: Restart last write - %r\n", Status));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // To check the workspace buffer behind last records is EMPTY or not.
  // If it's not EMPTY, FTW_LITE also need to call reclaim().
  //
  Record  = FtwLiteDevice->FtwLastRecord;
  Offset  = (UINT8 *) Record - FtwLiteDevice->FtwWorkSpace;
  if (FtwLiteDevice->FtwWorkSpace[Offset] != FTW_ERASED_BYTE) {
    Offset += FTW_LITE_RECORD_SIZE;
  }

  if (!IsErasedFlashBuffer (
        FTW_ERASE_POLARITY,
        FtwLiteDevice->FtwWorkSpace + Offset,
        FtwLiteDevice->FtwWorkSpaceSize - Offset
        )) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Workspace is dirty, call reclaim...\n"));
    Status = FtwReclaimWorkSpace (FtwLiteDevice, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "FtwLite: Workspace reclaim - %r\n", Status));
      FreePool (FtwLiteDevice);
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}
