/** @file

   Internal functions to operate Working Block Space.

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/


#include "FtwLite.h"

/**
  Check to see if it is a valid work space.


  @param WorkingHeader   Pointer of working block header

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
BOOLEAN
IsValidWorkSpace (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  )
{
  EFI_STATUS                              Status;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER WorkingBlockHeader;

  ASSERT (WorkingHeader != NULL);
  if (WorkingHeader->WorkingBlockValid != FTW_VALID_STATE) {
    return FALSE;
  }
  //
  // Check signature with gEfiSystemNvDataFvGuid
  //
  if (!CompareGuid (&gEfiSystemNvDataFvGuid, &WorkingHeader->Signature)) {
    return FALSE;
  }
  //
  // Check the CRC of header
  //
  CopyMem (
    &WorkingBlockHeader,
    WorkingHeader,
    sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER)
    );

  //
  // Filter out the Crc and State fields
  //
  SetMem (
    &WorkingBlockHeader.Crc,
    sizeof (UINT32),
    FTW_ERASED_BYTE
    );
  WorkingBlockHeader.WorkingBlockValid    = FTW_ERASE_POLARITY;
  WorkingBlockHeader.WorkingBlockInvalid  = FTW_ERASE_POLARITY;

  //
  // Calculate the Crc of woking block header
  //
  Status = gBS->CalculateCrc32 (
                  (UINT8 *) &WorkingBlockHeader,
                  sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
                  &WorkingBlockHeader.Crc
                  );
  ASSERT_EFI_ERROR (Status);

  if (WorkingBlockHeader.Crc != WorkingHeader->Crc) {
    DEBUG ((EFI_D_FTW_LITE, "FtwLite: Work block header CRC check error\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Initialize a work space when there is no work space.


  @param WorkingHeader   Pointer of working block header

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
InitWorkSpaceHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  )
{
  EFI_STATUS  Status;

  ASSERT (WorkingHeader != NULL);

  //
  // Here using gEfiSystemNvDataFvGuid as the signature.
  //
  CopyMem (
    &WorkingHeader->Signature,
    &gEfiSystemNvDataFvGuid,
    sizeof (EFI_GUID)
    );
  WorkingHeader->WriteQueueSize = FTW_WORKING_QUEUE_SIZE;

  //
  // Crc is calculated with all the fields except Crc and STATE
  //
  WorkingHeader->WorkingBlockValid    = FTW_ERASE_POLARITY;
  WorkingHeader->WorkingBlockInvalid  = FTW_ERASE_POLARITY;
  SetMem (&WorkingHeader->Crc, sizeof (UINT32), FTW_ERASED_BYTE);

  //
  // Calculate the CRC value
  //
  Status = gBS->CalculateCrc32 (
                  (UINT8 *) WorkingHeader,
                  sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
                  &WorkingHeader->Crc
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Restore the WorkingBlockValid flag to VALID state
  //
  WorkingHeader->WorkingBlockValid    = FTW_VALID_STATE;
  WorkingHeader->WorkingBlockInvalid  = FTW_INVALID_STATE;

  return EFI_SUCCESS;
}

/**
  Update a bit of state on a block device. The location of the bit is
  calculated by the (Lba, Offset, bit). Here bit is determined by the
  the name of a certain bit.


  @param FvBlock         FVB Protocol interface to access SrcBlock and DestBlock
  @param Lba             Lba of a block
  @param Offset          Offset on the Lba
  @param NewBit          New value that will override the old value if it can be change

  @retval  EFI_SUCCESS    A state bit has been updated successfully
  @retval  Others         Access block device error.
                          Notes:
                          Assume all bits of State are inside the same BYTE.
  @retval  EFI_ABORTED    Read block fail

**/
EFI_STATUS
FtwUpdateFvState (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  IN EFI_LBA                             Lba,
  IN UINTN                               Offset,
  IN UINT8                               NewBit
  )
{
  EFI_STATUS  Status;
  UINT8       State;
  UINTN       Length;

  //
  // Read state from device, assume State is only one byte.
  //
  Length  = sizeof (UINT8);
  Status  = FvBlock->Read (FvBlock, Lba, Offset, &Length, &State);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  State ^= FTW_POLARITY_REVERT;
  State = (UINT8) (State | NewBit);
  State ^= FTW_POLARITY_REVERT;

  //
  // Write state back to device
  //
  Length  = sizeof (UINT8);
  Status  = FvBlock->Write (FvBlock, Lba, Offset, &Length, &State);

  return Status;
}

/**
  Get the last Write record pointer.
  The last record is the record whose 'complete' state hasn't been set.
  After all, this header may be a EMPTY header entry for next Allocate.


  @param FtwLiteDevice   Private data of this driver
  @param FtwLastRecord   Pointer to retrieve the last write record

  @retval  EFI_SUCCESS      Get the last write record successfully
  @retval  EFI_ABORTED      The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastRecord (
  IN  EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  OUT EFI_FTW_LITE_RECORD  **FtwLastRecord
  )
{
  EFI_FTW_LITE_RECORD *Record;

  *FtwLastRecord = NULL;
  Record = (EFI_FTW_LITE_RECORD *) (FtwLiteDevice->FtwWorkSpaceHeader + 1);
  while (Record->WriteCompleted == FTW_VALID_STATE) {
    //
    // If Offset exceed the FTW work space boudary, return error.
    //
    if ((UINTN) ((UINT8 *) Record - FtwLiteDevice->FtwWorkSpace) > FtwLiteDevice->FtwWorkSpaceSize) {
      return EFI_ABORTED;
    }

    Record++;
  }
  //
  // Last write record is found
  //
  *FtwLastRecord = Record;
  return EFI_SUCCESS;
}

/**
  Read from working block to refresh the work space in memory.


  @param FtwLiteDevice   Point to private data of FTW driver

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
WorkSpaceRefresh (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice
  )
{
  EFI_STATUS          Status;
  UINTN               Length;
  UINTN               Offset;
  EFI_FTW_LITE_RECORD *Record;

  //
  // Initialize WorkSpace as FTW_ERASED_BYTE
  //
  SetMem (
    FtwLiteDevice->FtwWorkSpace,
    FtwLiteDevice->FtwWorkSpaceSize,
    FTW_ERASED_BYTE
    );

  //
  // Read from working block
  //
  Length = FtwLiteDevice->FtwWorkSpaceSize;
  Status = FtwLiteDevice->FtwFvBlock->Read (
                                        FtwLiteDevice->FtwFvBlock,
                                        FtwLiteDevice->FtwWorkSpaceLba,
                                        FtwLiteDevice->FtwWorkSpaceBase,
                                        &Length,
                                        FtwLiteDevice->FtwWorkSpace
                                        );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Refresh the FtwLastRecord
  //
  Status  = FtwGetLastRecord (FtwLiteDevice, &FtwLiteDevice->FtwLastRecord);

  Record  = FtwLiteDevice->FtwLastRecord;
  Offset  = (UINTN) (UINT8 *) Record - (UINTN) FtwLiteDevice->FtwWorkSpace;

  //
  // If work space has error or Record is out of the workspace limit, THEN
  //   call reclaim.
  //
  if (EFI_ERROR (Status) || (Offset + WRITE_TOTAL_SIZE >= FtwLiteDevice->FtwWorkSpaceSize)) {
    //
    // reclaim work space in working block.
    //
    Status = FtwReclaimWorkSpace (FtwLiteDevice, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_FTW_LITE, "FtwLite: Reclaim workspace - %r\n", Status));
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Reclaim the work space on the working block.


  @param  FtwLiteDevice          Point to private data of FTW driver
  @param  PreserveRecord         Whether get the last record or not

  @retval EFI_SUCCESS            The function completed successfully
  @retval EFI_OUT_OF_RESOURCES   Allocate memory error
  @retval EFI_ABORTED            The function could not complete successfully

**/
EFI_STATUS
FtwReclaimWorkSpace (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  IN BOOLEAN              PreserveRecord
  )
{
  EFI_STATUS                              Status;
  UINT8                                   *TempBuffer;
  UINTN                                   TempBufferSize;
  UINT8                                   *Ptr;
  UINTN                                   Length;
  UINTN                                   Index;
  UINTN                                   SpareBufferSize;
  UINT8                                   *SpareBuffer;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingBlockHeader;
  EFI_FTW_LITE_RECORD                     *Record;

  DEBUG ((EFI_D_FTW_LITE, "FtwLite: start to reclaim work space\n"));

  //
  // Read all original data from working block to a memory buffer
  //
  TempBufferSize = FtwLiteDevice->SpareAreaLength;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
  if (TempBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = TempBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwFvBlock->Read (
                                          FtwLiteDevice->FtwFvBlock,
                                          FtwLiteDevice->FtwWorkBlockLba + Index,
                                          0,
                                          &Length,
                                          Ptr
                                          );
    if (EFI_ERROR (Status)) {
      FreePool (TempBuffer);
      return EFI_ABORTED;
    }

    Ptr += Length;
  }
  //
  // Clean up the workspace, remove all the completed records.
  //
  Ptr = TempBuffer +
    ((UINTN) (FtwLiteDevice->FtwWorkSpaceLba - FtwLiteDevice->FtwWorkBlockLba)) *
    FtwLiteDevice->SizeOfSpareBlock + FtwLiteDevice->FtwWorkSpaceBase;

  //
  // Clear the content of buffer that will save the new work space data
  //
  SetMem (Ptr, FtwLiteDevice->FtwWorkSpaceSize, FTW_ERASED_BYTE);

  //
  // Copy EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER to buffer
  //
  CopyMem (
    Ptr,
    FtwLiteDevice->FtwWorkSpaceHeader,
    sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER)
    );
  if (PreserveRecord) {
    //
    // Get the last record
    //
    Status = FtwGetLastRecord (FtwLiteDevice, &FtwLiteDevice->FtwLastRecord);
    Record = FtwLiteDevice->FtwLastRecord;
    if (!EFI_ERROR (Status)                       &&
        Record                 != NULL            &&
        Record->WriteAllocated == FTW_VALID_STATE &&
        Record->WriteCompleted != FTW_VALID_STATE) {
      CopyMem (
        (UINT8 *) Ptr + sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
        Record,
        WRITE_TOTAL_SIZE
        );
    }
  }

  CopyMem (
    FtwLiteDevice->FtwWorkSpace,
    Ptr,
    FtwLiteDevice->FtwWorkSpaceSize
    );

  Status = FtwGetLastRecord (FtwLiteDevice, &FtwLiteDevice->FtwLastRecord);

  //
  // Set the WorkingBlockValid and WorkingBlockInvalid as INVALID
  //
  WorkingBlockHeader                      = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) Ptr;
  WorkingBlockHeader->WorkingBlockValid   = FTW_INVALID_STATE;
  WorkingBlockHeader->WorkingBlockInvalid = FTW_INVALID_STATE;

  //
  // Try to keep the content of spare block
  // Save spare block into a spare backup memory buffer (Sparebuffer)
  //
  SpareBufferSize = FtwLiteDevice->SpareAreaLength;
  SpareBuffer     = AllocatePool (SpareBufferSize);
  if (SpareBuffer == NULL) {
    FreePool (TempBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = SpareBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Read (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
                                            0,
                                            &Length,
                                            Ptr
                                            );
    if (EFI_ERROR (Status)) {
      FreePool (TempBuffer);
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr += Length;
  }
  //
  // Write the memory buffer to spare block
  //
  Status  = FtwEraseSpareBlock (FtwLiteDevice);
  Ptr     = TempBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Write (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
                                            0,
                                            &Length,
                                            Ptr
                                            );
    if (EFI_ERROR (Status)) {
      FreePool (TempBuffer);
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr += Length;
  }
  //
  // Free TempBuffer
  //
  FreePool (TempBuffer);

  //
  // Write the spare block to working block
  //
  Status = FlushSpareBlockToWorkingBlock (FtwLiteDevice);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return Status;
  }
  //
  // Restore spare backup buffer into spare block , if no failure happened during FtwWrite.
  //
  Status  = FtwEraseSpareBlock (FtwLiteDevice);
  Ptr     = SpareBuffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Write (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
                                            0,
                                            &Length,
                                            Ptr
                                            );
    if (EFI_ERROR (Status)) {
      FreePool (SpareBuffer);
      return EFI_ABORTED;
    }

    Ptr += Length;
  }

  FreePool (SpareBuffer);

  DEBUG ((EFI_D_FTW_LITE, "FtwLite: reclaim work space success\n"));

  return EFI_SUCCESS;
}
