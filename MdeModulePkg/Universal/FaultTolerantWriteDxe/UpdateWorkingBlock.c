/** @file

   Internal functions to operate Working Block Space.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/


#include "FaultTolerantWrite.h"

/**
  Check to see if it is a valid work space.


  @param WorkingHeader   Pointer of working block header

  @retval TRUE          The work space is valid.
  @retval FALSE         The work space is invalid.

**/
BOOLEAN
IsValidWorkSpace (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  )
{
  EFI_STATUS                              Status;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER WorkingBlockHeader;

  if (WorkingHeader == NULL) {
    return FALSE;
  }

  if (WorkingHeader->WorkingBlockValid != FTW_VALID_STATE) {
    DEBUG ((EFI_D_ERROR, "Ftw: Work block header valid bit check error\n"));
    return FALSE;
  }
  //
  // Check signature with gEfiSystemNvDataFvGuid
  //
  if (!CompareGuid (&gEfiSystemNvDataFvGuid, &WorkingHeader->Signature)) {
    DEBUG ((EFI_D_ERROR, "Ftw: Work block header signature check error\n"));
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
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (WorkingBlockHeader.Crc != WorkingHeader->Crc) {
    DEBUG ((EFI_D_ERROR, "Ftw: Work block header CRC check error\n"));
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

  if (WorkingHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Here using gEfiSystemNvDataFvGuid as the signature.
  //
  CopyMem (
    &WorkingHeader->Signature,
    &gEfiSystemNvDataFvGuid,
    sizeof (EFI_GUID)
    );
  WorkingHeader->WriteQueueSize = (UINT64) (PcdGet32 (PcdFlashNvStorageFtwWorkingSize) - sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER));

  //
  // Crc is calculated with all the fields except Crc and STATE
  //
  WorkingHeader->WorkingBlockValid    = FTW_ERASE_POLARITY;
  WorkingHeader->WorkingBlockInvalid  = FTW_ERASE_POLARITY;

  SetMem (
    &WorkingHeader->Crc,
    sizeof (UINT32),
    FTW_ERASED_BYTE
    );

  //
  // Calculate the CRC value
  //
  Status = gBS->CalculateCrc32 (
                  (UINT8 *) WorkingHeader,
                  sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
                  &WorkingHeader->Crc
                  );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Restore the WorkingBlockValid flag to VALID state
  //
  WorkingHeader->WorkingBlockValid    = FTW_VALID_STATE;
  WorkingHeader->WorkingBlockInvalid  = FTW_INVALID_STATE;

  return EFI_SUCCESS;
}

/**
  Read from working block to refresh the work space in memory.

  @param FtwDevice   Point to private data of FTW driver

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
WorkSpaceRefresh (
  IN EFI_FTW_DEVICE  *FtwDevice
  )
{
  EFI_STATUS                      Status;
  UINTN                           Length;

  //
  // Initialize WorkSpace as FTW_ERASED_BYTE
  //
  SetMem (
    FtwDevice->FtwWorkSpace,
    FtwDevice->FtwWorkSpaceSize,
    FTW_ERASED_BYTE
    );

  //
  // Read from working block
  //
  Length = FtwDevice->FtwWorkSpaceSize;
  Status = FtwDevice->FtwFvBlock->Read (
                                    FtwDevice->FtwFvBlock,
                                    FtwDevice->FtwWorkSpaceLba,
                                    FtwDevice->FtwWorkSpaceBase,
                                    &Length,
                                    FtwDevice->FtwWorkSpace
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Refresh the FtwLastWriteHeader
  //
  Status = FtwGetLastWriteHeader (
            FtwDevice->FtwWorkSpaceHeader,
            FtwDevice->FtwWorkSpaceSize,
            &FtwDevice->FtwLastWriteHeader
            );
  if (EFI_ERROR (Status)) {
    //
    // reclaim work space in working block.
    //
    Status = FtwReclaimWorkSpace (FtwDevice, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Ftw: Reclaim workspace - %r\n", Status));
      return EFI_ABORTED;
    }
    //
    // Read from working block again
    //
    Length = FtwDevice->FtwWorkSpaceSize;
    Status = FtwDevice->FtwFvBlock->Read (
                                      FtwDevice->FtwFvBlock,
                                      FtwDevice->FtwWorkSpaceLba,
                                      FtwDevice->FtwWorkSpaceBase,
                                      &Length,
                                      FtwDevice->FtwWorkSpace
                                      );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }

    Status = FtwGetLastWriteHeader (
              FtwDevice->FtwWorkSpaceHeader,
              FtwDevice->FtwWorkSpaceSize,
              &FtwDevice->FtwLastWriteHeader
              );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // Refresh the FtwLastWriteRecord
  //
  Status = FtwGetLastWriteRecord (
            FtwDevice->FtwLastWriteHeader,
            &FtwDevice->FtwLastWriteRecord
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Reclaim the work space on the working block.

  @param FtwDevice       Point to private data of FTW driver
  @param PreserveRecord  Whether to preserve the working record is needed

  @retval EFI_SUCCESS            The function completed successfully
  @retval EFI_OUT_OF_RESOURCES   Allocate memory error
  @retval EFI_ABORTED            The function could not complete successfully

**/
EFI_STATUS
FtwReclaimWorkSpace (
  IN EFI_FTW_DEVICE  *FtwDevice,
  IN BOOLEAN         PreserveRecord
  )
{
  EFI_STATUS                              Status;
  UINTN                                   Length;
  EFI_FAULT_TOLERANT_WRITE_HEADER         *Header;
  UINT8                                   *TempBuffer;
  UINTN                                   TempBufferSize;
  UINTN                                   SpareBufferSize;
  UINT8                                   *SpareBuffer;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingBlockHeader;
  UINTN                                   Index;
  UINT8                                   *Ptr;
  EFI_LBA                                 WorkSpaceLbaOffset;

  DEBUG ((EFI_D_ERROR, "Ftw: start to reclaim work space\n"));

  WorkSpaceLbaOffset = FtwDevice->FtwWorkSpaceLba - FtwDevice->FtwWorkBlockLba;

  //
  // Read all original data from working block to a memory buffer
  //
  TempBufferSize = FtwDevice->SpareAreaLength;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
  if (TempBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = TempBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwDevice->BlockSize;
    Status = FtwDevice->FtwFvBlock->Read (
                                          FtwDevice->FtwFvBlock,
                                          FtwDevice->FtwWorkBlockLba + Index,
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
        (UINTN) WorkSpaceLbaOffset * FtwDevice->BlockSize +
        FtwDevice->FtwWorkSpaceBase;

  //
  // Clear the content of buffer that will save the new work space data
  //
  SetMem (Ptr, FtwDevice->FtwWorkSpaceSize, FTW_ERASED_BYTE);

  //
  // Copy EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER to buffer
  //
  CopyMem (
    Ptr,
    FtwDevice->FtwWorkSpaceHeader,
    sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER)
    );
  if (PreserveRecord) {
    //
    // Get the last record following the header,
    //
    Status = FtwGetLastWriteHeader (
               FtwDevice->FtwWorkSpaceHeader,
               FtwDevice->FtwWorkSpaceSize,
               &FtwDevice->FtwLastWriteHeader
               );
    Header = FtwDevice->FtwLastWriteHeader;
    if (!EFI_ERROR (Status) && (Header != NULL) && (Header->Complete != FTW_VALID_STATE) && (Header->HeaderAllocated == FTW_VALID_STATE)) {
      CopyMem (
        Ptr + sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
        FtwDevice->FtwLastWriteHeader,
        WRITE_TOTAL_SIZE (Header->NumberOfWrites, Header->PrivateDataSize)
        );
    }
  }

  CopyMem (
    FtwDevice->FtwWorkSpace,
    Ptr,
    FtwDevice->FtwWorkSpaceSize
    );

  FtwGetLastWriteHeader (
    FtwDevice->FtwWorkSpaceHeader,
    FtwDevice->FtwWorkSpaceSize,
    &FtwDevice->FtwLastWriteHeader
    );

  FtwGetLastWriteRecord (
    FtwDevice->FtwLastWriteHeader,
    &FtwDevice->FtwLastWriteRecord
    );

  //
  // Set the WorkingBlockValid and WorkingBlockInvalid as INVALID
  //
  WorkingBlockHeader                      = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) (TempBuffer +
                                            (UINTN) WorkSpaceLbaOffset * FtwDevice->BlockSize +
                                            FtwDevice->FtwWorkSpaceBase);
  WorkingBlockHeader->WorkingBlockValid   = FTW_INVALID_STATE;
  WorkingBlockHeader->WorkingBlockInvalid = FTW_INVALID_STATE;

  //
  // Try to keep the content of spare block
  // Save spare block into a spare backup memory buffer (Sparebuffer)
  //
  SpareBufferSize = FtwDevice->SpareAreaLength;
  SpareBuffer     = AllocatePool (SpareBufferSize);
  if (SpareBuffer == NULL) {
    FreePool (TempBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Read (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
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
  Status  = FtwEraseSpareBlock (FtwDevice);
  Ptr     = TempBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Write (
                                            FtwDevice->FtwBackupFvb,
                                            FtwDevice->FtwSpareLba + Index,
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
  // Set the WorkingBlockValid in spare block
  //
  Status = FtwUpdateFvState (
            FtwDevice->FtwBackupFvb,
            FtwDevice->FtwSpareLba + WorkSpaceLbaOffset,
            FtwDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_VALID
            );
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }
  //
  // Before erase the working block, set WorkingBlockInvalid in working block.
  //
  // Offset = OFFSET_OF(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER,
  //                          WorkingBlockInvalid);
  //
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
            FtwDevice->FtwWorkSpaceLba,
            FtwDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_INVALID
            );
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }

  FtwDevice->FtwWorkSpaceHeader->WorkingBlockInvalid = FTW_VALID_STATE;

  //
  // Write the spare block to working block
  //
  Status = FlushSpareBlockToWorkingBlock (FtwDevice);
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return Status;
  }
  //
  // Restore spare backup buffer into spare block , if no failure happened during FtwWrite.
  //
  Status  = FtwEraseSpareBlock (FtwDevice);
  Ptr     = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Write (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
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

  DEBUG ((EFI_D_ERROR, "Ftw: reclaim work space successfully\n"));

  return EFI_SUCCESS;
}
