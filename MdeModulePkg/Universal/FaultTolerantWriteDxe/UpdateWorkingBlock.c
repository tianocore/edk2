/** @file

   Internal functions to operate Working Block Space.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "FaultTolerantWrite.h"

EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER mWorkingBlockHeader = {ZERO_GUID, 0, 0, 0, 0, {0, 0, 0}, 0};

/**
  Initialize a local work space header.

  Since Signature and WriteQueueSize have been known, Crc can be calculated out,
  then the work space header will be fixed.
**/
VOID
InitializeLocalWorkSpaceHeader (
  VOID
  )
{
  //
  // Check signature with gEdkiiWorkingBlockSignatureGuid.
  //
  if (CompareGuid (&gEdkiiWorkingBlockSignatureGuid, &mWorkingBlockHeader.Signature)) {
    //
    // The local work space header has been initialized.
    //
    return;
  }

  SetMem (
    &mWorkingBlockHeader,
    sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER),
    FTW_ERASED_BYTE
    );

  //
  // Here using gEdkiiWorkingBlockSignatureGuid as the signature.
  //
  CopyMem (
    &mWorkingBlockHeader.Signature,
    &gEdkiiWorkingBlockSignatureGuid,
    sizeof (EFI_GUID)
    );
  mWorkingBlockHeader.WriteQueueSize = PcdGet32 (PcdFlashNvStorageFtwWorkingSize) - sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER);

  //
  // Crc is calculated with all the fields except Crc and STATE, so leave them as FTW_ERASED_BYTE.
  //

  //
  // Calculate the Crc of woking block header
  //
  mWorkingBlockHeader.Crc = FtwCalculateCrc32 (&mWorkingBlockHeader,
                              sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER));

  mWorkingBlockHeader.WorkingBlockValid    = FTW_VALID_STATE;
  mWorkingBlockHeader.WorkingBlockInvalid  = FTW_INVALID_STATE;
}

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
  if (WorkingHeader == NULL) {
    return FALSE;
  }

  if (CompareMem (WorkingHeader, &mWorkingBlockHeader, sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER)) == 0) {
    return TRUE;
  }

  DEBUG ((EFI_D_INFO, "Ftw: Work block header check mismatch\n"));
  return FALSE;
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
  if (WorkingHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (WorkingHeader, &mWorkingBlockHeader, sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER));

  return EFI_SUCCESS;
}

/**
  Read work space data from work block or spare block.

  @param FvBlock        FVB Protocol interface to access the block.
  @param BlockSize      The size of the block.
  @param Lba            Lba of the block.
  @param Offset         The offset within the block.
  @param Length         The number of bytes to read from the block.
  @param Buffer         The data is read.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_ABORTED   The function could not complete successfully.

**/
EFI_STATUS
ReadWorkSpaceData (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvBlock,
  IN UINTN                              BlockSize,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  OUT UINT8                             *Buffer
  )
{
  EFI_STATUS            Status;
  UINT8                 *Ptr;
  UINTN                 MyLength;

  //
  // Calculate the real Offset and Lba to write.
  //
  while (Offset >= BlockSize) {
    Offset -= BlockSize;
    Lba++;
  }

  Ptr = Buffer;
  while (Length > 0) {
    if ((Offset + Length) > BlockSize) {
      MyLength = BlockSize - Offset;
    } else {
      MyLength = Length;
    }

    Status = FvBlock->Read (
                        FvBlock,
                        Lba,
                        Offset,
                        &MyLength,
                        Ptr
                        );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    Offset = 0;
    Length -= MyLength;
    Ptr += MyLength;
    Lba++;
  }

  return EFI_SUCCESS;
}

/**
  Write work space data to work block.

  @param FvBlock        FVB Protocol interface to access the block.
  @param BlockSize      The size of the block.
  @param Lba            Lba of the block.
  @param Offset         The offset within the block to place the data.
  @param Length         The number of bytes to write to the block.
  @param Buffer         The data to write.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_ABORTED   The function could not complete successfully.

**/
EFI_STATUS
WriteWorkSpaceData (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvBlock,
  IN UINTN                              BlockSize,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  IN UINT8                              *Buffer
  )
{
  EFI_STATUS            Status;
  UINT8                 *Ptr;
  UINTN                 MyLength;

  //
  // Calculate the real Offset and Lba to write.
  //
  while (Offset >= BlockSize) {
    Offset -= BlockSize;
    Lba++;
  }

  Ptr = Buffer;
  while (Length > 0) {
    if ((Offset + Length) > BlockSize) {
      MyLength = BlockSize - Offset;
    } else {
      MyLength = Length;
    }

    Status = FvBlock->Write (
                        FvBlock,
                        Lba,
                        Offset,
                        &MyLength,
                        Ptr
                        );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    Offset = 0;
    Length -= MyLength;
    Ptr += MyLength;
    Lba++;
  }
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
  UINTN                           RemainingSpaceSize;

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
  Status = ReadWorkSpaceData (
             FtwDevice->FtwFvBlock,
             FtwDevice->WorkBlockSize,
             FtwDevice->FtwWorkSpaceLba,
             FtwDevice->FtwWorkSpaceBase,
             FtwDevice->FtwWorkSpaceSize,
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
  RemainingSpaceSize = FtwDevice->FtwWorkSpaceSize - ((UINTN) FtwDevice->FtwLastWriteHeader - (UINTN) FtwDevice->FtwWorkSpace);
  DEBUG ((EFI_D_INFO, "Ftw: Remaining work space size - %x\n", RemainingSpaceSize));
  //
  // If FtwGetLastWriteHeader() returns error, or the remaining space size is even not enough to contain
  // one EFI_FAULT_TOLERANT_WRITE_HEADER + one EFI_FAULT_TOLERANT_WRITE_RECORD(It will cause that the header
  // pointed by FtwDevice->FtwLastWriteHeader or record pointed by FtwDevice->FtwLastWriteRecord may contain invalid data),
  // it needs to reclaim work space.
  //
  if (EFI_ERROR (Status) || RemainingSpaceSize < sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER) + sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD)) {
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
    Status = ReadWorkSpaceData (
               FtwDevice->FtwFvBlock,
               FtwDevice->WorkBlockSize,
               FtwDevice->FtwWorkSpaceLba,
               FtwDevice->FtwWorkSpaceBase,
               FtwDevice->FtwWorkSpaceSize,
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

  DEBUG ((EFI_D_INFO, "Ftw: start to reclaim work space\n"));

  WorkSpaceLbaOffset = FtwDevice->FtwWorkSpaceLba - FtwDevice->FtwWorkBlockLba;

  //
  // Read all original data from working block to a memory buffer
  //
  TempBufferSize = FtwDevice->NumberOfWorkBlock * FtwDevice->WorkBlockSize;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
  if (TempBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = TempBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfWorkBlock; Index += 1) {
    Length = FtwDevice->WorkBlockSize;
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
        (UINTN) WorkSpaceLbaOffset * FtwDevice->WorkBlockSize +
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
        FTW_WRITE_TOTAL_SIZE (Header->NumberOfWrites, Header->PrivateDataSize)
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
                                            (UINTN) WorkSpaceLbaOffset * FtwDevice->WorkBlockSize +
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
    Length = FtwDevice->SpareBlockSize;
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
  if (EFI_ERROR (Status)) {
    FreePool (TempBuffer);
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }
  Ptr     = TempBuffer;
  for (Index = 0; TempBufferSize > 0; Index += 1) {
    if (TempBufferSize > FtwDevice->SpareBlockSize) {
      Length = FtwDevice->SpareBlockSize;
    } else {
      Length = TempBufferSize;
    }
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
    TempBufferSize -= Length;
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
            FtwDevice->SpareBlockSize,
            FtwDevice->FtwSpareLba + FtwDevice->FtwWorkSpaceLbaInSpare,
            FtwDevice->FtwWorkSpaceBaseInSpare + sizeof (EFI_GUID) + sizeof (UINT32),
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
            FtwDevice->WorkBlockSize,
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
  if (EFI_ERROR (Status)) {
    FreePool (SpareBuffer);
    return EFI_ABORTED;
  }
  Ptr     = SpareBuffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Length = FtwDevice->SpareBlockSize;
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

  DEBUG ((EFI_D_INFO, "Ftw: reclaim work space successfully\n"));

  return EFI_SUCCESS;
}
