/** @file
  Functions in this file will program the image into flash area.

  Copyright (c) 2002 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UpdateDriver.h"

/**
  Write a block size data into flash.

  @param FvbProtocol     Pointer to FVB protocol.
  @param Lba             Logic block index to be updated.
  @param BlockSize       Block size
  @param Buffer          Buffer data to be written.

  @retval EFI_SUCCESS   Write data successfully.
  @retval other errors  Write data failed.

**/
EFI_STATUS
UpdateOneBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN EFI_LBA                            Lba,
  IN UINTN                              BlockSize,
  IN UINT8                              *Buffer
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Size;

  //
  // First erase the block
  //
  Status                = FvbProtocol->EraseBlocks (
                                         FvbProtocol,
                                         Lba,                        // Lba
                                         1,                          // NumOfBlocks
                                         EFI_LBA_LIST_TERMINATOR
                                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Write the block
  //
  Size                  = BlockSize;
  Status                = FvbProtocol->Write (
                                         FvbProtocol,
                                         Lba,                        // Lba
                                         0,                          // Offset
                                         &Size,                      // Size
                                         Buffer                      // Buffer
                                         );
  if ((EFI_ERROR (Status)) || (Size != BlockSize)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Write buffer data in a flash block.

  @param FvbProtocol     Pointer to FVB protocol.
  @param Lba             Logic block index to be updated.
  @param Offset          The offset within the block.
  @param Length          Size of buffer to be updated.
  @param BlockSize       Block size.
  @param Buffer          Buffer data to be updated.

  @retval EFI_SUCCESS   Write data successfully.
  @retval other errors  Write data failed.

**/
EFI_STATUS
UpdateBufferInOneBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  IN UINTN                              BlockSize,
  IN UINT8                              *Buffer
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Size;
  UINT8                                 *ReservedBuffer;

  //
  // If we are going to update a whole block
  //
  if ((Offset == 0) && (Length == BlockSize)) {
    Status              = UpdateOneBlock (
                            FvbProtocol,
                            Lba,
                            BlockSize,
                            Buffer
                            );
    return Status;
  }

  //
  // If it is not a full block update, we need to coalesce data in
  // the block that is not going to be updated and new data together.
  //

  //
  // Allocate a reserved buffer to make up the final buffer for update
  //
  ReservedBuffer        = NULL;
  ReservedBuffer = AllocatePool (BlockSize);
  if (ReservedBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // First get the original content of the block
  //
  Size                  = BlockSize;
  Status                = FvbProtocol->Read (
                                         FvbProtocol,
                                         Lba,
                                         0,
                                         &Size,
                                         ReservedBuffer
                                         );
  if ((EFI_ERROR (Status)) || (Size != BlockSize)) {
    FreePool (ReservedBuffer);
    return Status;
  }

  //
  // Overwrite the reserved buffer with new content
  //
  CopyMem (ReservedBuffer + Offset, Buffer, Length);

  Status                = UpdateOneBlock (
                            FvbProtocol,
                            Lba,
                            BlockSize,
                            ReservedBuffer
                            );

  FreePool (ReservedBuffer);

  return Status;
}

/**
  Get the last write log, and check the status of last write.
  If not complete, restart will be taken.

  @param FvbHandle       Handle of FVB protocol.
  @param FtwProtocol     FTW protocol instance.
  @param ConfigData      Config data on updating driver.
  @param PrivateDataSize bytes from the private data
                         stored for this write.
  @param PrivateData     A pointer to a buffer. The function will copy.
  @param Lba             The logical block address of the last write.
  @param Offset          The offset within the block of the last write.
  @param Length          The length of the last write.
  @param Pending         A Boolean value with TRUE indicating
                         that the write was completed.

  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_ABORTED           The FTW work space is damaged.
  @retval EFI_NOT_FOUND         The last write is not done by this driver.
  @retval EFI_SUCCESS           Last write log is got.

**/
EFI_STATUS
RetrieveLastWrite (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *FtwProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINTN                              PrivateDataSize,
  IN OUT UPDATE_PRIVATE_DATA            *PrivateData,
  IN OUT EFI_LBA                        *Lba,
  IN OUT UINTN                          *Offset,
  IN OUT UINTN                          *Length,
  IN OUT BOOLEAN                        *Pending
  )
{
  EFI_STATUS                            Status;
  EFI_GUID                              CallerId;
  UINTN                                 PrivateBufferSize;
  BOOLEAN                               Complete;
  VOID                                  *PrivateDataBuffer;

  //
  // Get the last write
  //
  *Pending              = FALSE;
  PrivateBufferSize     = PrivateDataSize;
  PrivateDataBuffer     = NULL;
  Status                = FtwProtocol->GetLastWrite (
                                         FtwProtocol,
                                         &CallerId,
                                         Lba,
                                         Offset,
                                         Length,
                                         &PrivateBufferSize,
                                         PrivateData,
                                         &Complete
                                         );
  if (EFI_ERROR (Status)) {
    //
    // If there is no incompleted record, return success.
    //
    if ((Status == EFI_NOT_FOUND) && Complete) {
      return EFI_SUCCESS;
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // If buffer too small, reallocate buffer and call getlastwrite again
      //
      PrivateDataBuffer = AllocatePool (PrivateBufferSize);

      if (PrivateDataBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status            = FtwProtocol->GetLastWrite (
                                         FtwProtocol,
                                         &CallerId,
                                         Lba,
                                         Offset,
                                         Length,
                                         &PrivateBufferSize,
                                         PrivateDataBuffer,
                                         &Complete
                                         );
      if (EFI_ERROR (Status)) {
        FreePool ( PrivateDataBuffer);
        return EFI_ABORTED;
      } else {
        CopyMem (PrivateData, PrivateDataBuffer, PrivateDataSize);
        FreePool (PrivateDataBuffer);
        PrivateDataBuffer = NULL;
      }
    } else {
      return EFI_ABORTED;
    }
  }

  *Pending              = TRUE;

  //
  // If the caller is not the update driver, then return.
  // The update driver cannot continue to perform the update
  //
  if (CompareMem (&CallerId, &gEfiCallerIdGuid, sizeof (EFI_GUID)) != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Check the private data and see if it is the one I need.
  //
  if (CompareMem (&(PrivateData->FileGuid), &(ConfigData->FileGuid), sizeof(EFI_GUID)) != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // If the caller is the update driver and complete is not true, then restart().
  //
  if (!Complete) {
    //
    //  Re-start the update
    //
    Status              = FtwProtocol->Restart (
                                         FtwProtocol,
                                         FvbHandle
                                         );
    //
    // If restart() error, then abort().
    //
    if (EFI_ERROR (Status)) {
      FtwProtocol->Abort (FtwProtocol);
      //
      // Now set Pending as FALSE as this record has been cleared
      //
      *Pending          = FALSE;
      return EFI_SUCCESS;
    }

  }

  return Status;
}

/**
  Update the whole FV image in fault tolerant write method.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param BlockMap        Block array to specify flash area.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.

  @retval EFI_SUCCESS            FV image is writed into flash.
  @retval EFI_INVALID_PARAMETER  Config data is not valid.
  @retval EFI_NOT_FOUND          FTW protocol doesn't exist.
  @retval EFI_OUT_OF_RESOURCES   No enough backup space.
  @retval EFI_ABORTED            Error happen when update FV.

**/
EFI_STATUS
FaultTolerantUpdateOnWholeFv (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN EFI_FV_BLOCK_MAP_ENTRY             *BlockMap,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize
  )
{
  EFI_STATUS                            Status;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *FtwProtocol;
  UINTN                                 MaxBlockSize;
  UINTN                                 FtwMaxBlockSize;
  BOOLEAN                               Pending;
  UPDATE_PRIVATE_DATA                   PrivateData;
  EFI_LBA                               PendingLba;
  EFI_LBA                               Lba;
  UINTN                                 PendingOffset;
  UINTN                                 Offset;
  UINTN                                 PendingLength;
  UINTN                                 Length;
  EFI_FV_BLOCK_MAP_ENTRY                *PtrMap;
  UINTN                                 NumOfBlocks;
  UINTN                                 Index;
  UINT8                                 *UpdateBuffer;

  if ((ConfigData->UpdateType != UpdateWholeFV)
    || (!ConfigData->FaultTolerant)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the FTW protocol
  //
  Status                = gBS->LocateProtocol (
                                 &gEfiFaultTolerantWriteProtocolGuid,
                                 NULL,
                                 (VOID **) &FtwProtocol
                                 );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the maximum block size of the FV, and number of blocks
  // NumOfBlocks will be the NumOfUdpates.
  //
  MaxBlockSize          = 0;
  NumOfBlocks           = 0;
  PtrMap                = BlockMap;
  while (TRUE) {
    if ((PtrMap->NumBlocks == 0) || (PtrMap->Length == 0)) {
      break;
    }
    if (MaxBlockSize < PtrMap->Length) {
      MaxBlockSize      = PtrMap->Length;
    }
    NumOfBlocks         = NumOfBlocks + PtrMap->NumBlocks;
    PtrMap++;
  }

  FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);
  //
  // Not enough backup space. return directly
  //
  if (FtwMaxBlockSize < MaxBlockSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  PendingLba            = 0;
  PendingOffset         = 0;
  PendingLength         = 0;
  Pending               = FALSE;

  //
  // Fault Tolerant Write can only support actual fault tolerance if the write
  // is a reclaim operation, which means the data buffer (new and old) are
  // acutally both stored in flash. But for component update write, the data
  // are now in memory. So we cannot actually recover the data after power
  // failure.
  //
  Status                = RetrieveLastWrite (
                            FvbHandle,
                            FtwProtocol,
                            ConfigData,
                            sizeof (UPDATE_PRIVATE_DATA),
                            &PrivateData,
                            &PendingLba,
                            &PendingOffset,
                            &PendingLength,
                            &Pending
                            );

  if (Pending && (Status == EFI_NOT_FOUND)) {
    //
    // Cannot continue with the write operation
    //
    return EFI_ABORTED;
  }

  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  //
  // Currently we start from the pending write if there is any. But as we
  // are going to update a whole FV, we can just abort last write and start
  // from the very begining.
  //
  if (!Pending) {
    //
    // Now allocte the update private data in FTW. If there is pending
    // write, it has already been allocated and no need to allocate here.
    //
    Status              = FtwProtocol->Allocate (
                                         FtwProtocol,
                                         &gEfiCallerIdGuid,
                                         sizeof (UPDATE_PRIVATE_DATA),
                                         NumOfBlocks
                                         );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Perform the update now. If there are pending writes, we need to
  // start from the pending write instead of the very beginning.
  //
  PtrMap                = BlockMap;
  Lba                   = 0;
  Offset                = 0;
  UpdateBuffer          = ImageBuffer;
  CopyMem (
    (VOID *) &PrivateData.FileGuid,
    (VOID *) &ConfigData->FileGuid,
     sizeof (EFI_GUID)
  );

  while (TRUE) {
    if ((PtrMap->NumBlocks == 0) || (PtrMap->Length == 0)) {
      break;
    }
    Length              = (UINTN)PtrMap->Length;
    for (Index = 0; Index < PtrMap->NumBlocks; Index++) {

      //
      // Add an extra check here to see if the pending record is correct
      //
      if (Pending && (Lba == PendingLba)) {
        if ((PendingOffset != Offset) || (PendingLength != Length)) {
          //
          // Error.
          //
          Status          = EFI_ABORTED;
          break;
        }
      }

      if ((!Pending) || (Lba >= PendingLba)) {
        Status            = FtwProtocol->Write (
                                           FtwProtocol,
                                           Lba,                  // Lba
                                           Offset,               // Offset
                                           Length,               // Size
                                           &PrivateData,         // Private Data
                                           FvbHandle,            // FVB handle
                                           UpdateBuffer          // Buffer
                                           );
      }

      if (EFI_ERROR (Status)) {
        break;
      }
      Lba++;
      UpdateBuffer      = (UINT8 *) ((UINTN)UpdateBuffer + Length);
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    PtrMap++;
  }

  return Status;

}

/**
  Directly update the whole FV image without fault tolerant write method.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param BlockMap        Block array to specify flash area.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.

  @retval EFI_SUCCESS            FV image is writed into flash.
  @retval EFI_INVALID_PARAMETER  Config data is not valid.
  @retval EFI_ABORTED            Error happen when update FV.

**/
EFI_STATUS
NonFaultTolerantUpdateOnWholeFv (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN EFI_FV_BLOCK_MAP_ENTRY             *BlockMap,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize
  )
{
  EFI_STATUS                            Status;
  EFI_FV_BLOCK_MAP_ENTRY                *PtrMap;
  UINTN                                 Index;
  EFI_LBA                               UpdateLba;
  UINT8                                 *UpdateBuffer;
  UINTN                                 UpdateSize;

  if ((ConfigData->UpdateType != UpdateWholeFV )
    || (ConfigData->FaultTolerant)) {
    return EFI_INVALID_PARAMETER;
  }

  Status                = EFI_SUCCESS;
  PtrMap                = BlockMap;
  UpdateLba             = 0;
  UpdateBuffer          = ImageBuffer;

  //
  // Perform the update now
  //
  while (TRUE) {
    if ((PtrMap->NumBlocks == 0) || (PtrMap->Length == 0)) {
      break;
    }
    UpdateSize          = (UINTN)PtrMap->Length;
    for (Index = 0; Index < PtrMap->NumBlocks; Index++) {
      Status            = UpdateOneBlock (
                            FvbProtocol,
                            UpdateLba,
                            UpdateSize,
                            UpdateBuffer
                            );
      if (EFI_ERROR (Status)) {
        break;
      }

      UpdateLba++;
      UpdateBuffer      = (UINT8 *) ((UINTN)UpdateBuffer + UpdateSize);
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    PtrMap++;
  }

  return Status;
}

/**
  Update the whole FV image, and reinsall FVB protocol for the updated FV image.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.

  @retval EFI_INVALID_PARAMETER  Update type is not UpdateWholeFV.
                                 Or Image size is not same to the size of whole FV.
  @retval EFI_OUT_OF_RESOURCES   No enoug memory is allocated.
  @retval EFI_SUCCESS            FV image is updated, and its FVB protocol is reinstalled.

**/
EFI_STATUS
PerformUpdateOnWholeFv (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize
)
{
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FV_BLOCK_MAP_ENTRY                *BlockMap;
  CHAR16                                *TmpStr;

  if (ConfigData->UpdateType != UpdateWholeFV) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the header of the firmware volume
  //
  FwVolHeader           = NULL;
  FwVolHeader = AllocatePool (((EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) (ConfigData->BaseAddress)))->HeaderLength);
  if (FwVolHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (
    FwVolHeader,
    (VOID *) ((UINTN) (ConfigData->BaseAddress)),
    ((EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) (ConfigData->BaseAddress)))->HeaderLength
    );

  //
  // Check if ImageSize is the same as the size of the whole FV
  //
  if ((UINT64)ImageSize != FwVolHeader->FvLength) {
    FreePool (FwVolHeader);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Print on screen
  //
  TmpStr  = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_FIRMWARE_VOLUME), NULL);
  if (TmpStr != NULL) {
    Print (TmpStr, ConfigData->BaseAddress, (FwVolHeader->FvLength + ConfigData->BaseAddress));
    FreePool (TmpStr);
  }

  DEBUG ((EFI_D_UPDATE, "UpdateDriver: updating whole FV from %08LX to %08LX\n",
    ConfigData->BaseAddress, (FwVolHeader->FvLength + ConfigData->BaseAddress)));

  //
  // Get the block map of the firmware volume
  //
  BlockMap              = &(FwVolHeader->BlockMap[0]);

  //
  // It is about the same if we are going to fault tolerantly update
  // a certain FV in our current design. But we divide non-fault tolerant
  // and fault tolerant udpate here for better maintenance as fault
  // tolerance may change and may be done more wisely if we have space.
  //
  if (ConfigData->FaultTolerant) {
    Status              = FaultTolerantUpdateOnWholeFv (
                            FvbHandle,
                            FvbProtocol,
                            BlockMap,
                            ConfigData,
                            ImageBuffer,
                            ImageSize
                            );
  } else {
    Status              = NonFaultTolerantUpdateOnWholeFv (
                            FvbHandle,
                            FvbProtocol,
                            BlockMap,
                            ConfigData,
                            ImageBuffer,
                            ImageSize
                            );
  }

  FreePool (FwVolHeader);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // As the whole FV has been replaced, the FV driver shall re-parse the
  // firmware volume. So re-install FVB protocol here
  //
  Status                =  gBS->ReinstallProtocolInterface (
                                   FvbHandle,
                                   &gEfiFirmwareVolumeBlockProtocolGuid,
                                   FvbProtocol,
                                   FvbProtocol
                                   );

  return Status;
}

/**
  Update certain file in the FV.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.
  @param FileType        FFS file type.
  @param FileAttributes  FFS file attribute

  @retval EFI_INVALID_PARAMETER  Update type is not UpdateFvFile.
                                 Or Image size is not same to the size of whole FV.
  @retval EFI_UNSUPPORTED        PEIM FFS is unsupported to be updated.
  @retval EFI_SUCCESS            The FFS file is added into FV.

**/
EFI_STATUS
PerformUpdateOnFvFile (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize,
  IN EFI_FV_FILETYPE                    FileType,
  IN EFI_FV_FILE_ATTRIBUTES             FileAttributes
  )
{
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL          *FwVolProtocol;
  EFI_FV_WRITE_FILE_DATA                FileData;
  CHAR16                                *TmpStr;

  if (ConfigData->UpdateType != UpdateFvFile) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Print on screen
  //
  TmpStr  = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_FIRMWARE_VOLUME_FILE), NULL);
  if (TmpStr != NULL) {
    Print (TmpStr, &(ConfigData->FileGuid));
    FreePool (TmpStr);
  }

  DEBUG ((EFI_D_UPDATE, "UpdateDriver: updating file: %g\n",
    &(ConfigData->FileGuid)));

  //
  // Get Firmware volume protocol on this FVB protocol
  //
  Status                = gBS->HandleProtocol (
                                  FvbHandle,
                                  &gEfiFirmwareVolume2ProtocolGuid,
                                  (VOID **) &FwVolProtocol
                                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If it is a PEIM, we need first to rebase it before committing
  // the write to target
  //
  if ((FileType == EFI_FV_FILETYPE_PEI_CORE) || (FileType == EFI_FV_FILETYPE_PEIM )
    || (FileType == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER)) {
    return EFI_UNSUPPORTED;
  }

  FileData.NameGuid         = &(ConfigData->FileGuid);
  FileData.Type             = FileType;
  FileData.FileAttributes   = FileAttributes;
  FileData.Buffer           = ImageBuffer;
  FileData.BufferSize       = (UINT32) ImageSize;

  Status                    = FwVolProtocol->WriteFile (
                                                FwVolProtocol,
                                                1,                        // NumberOfFiles
                                                (EFI_FV_WRITE_POLICY)ConfigData->FaultTolerant,
                                                &FileData
                                                );
  return Status;
}

/**
  Update the buffer into flash area in fault tolerant write method.

  @param ImageBuffer     Image buffer to be updated.
  @param SizeLeft        Size of the image buffer.
  @param UpdatedSize     Size of the updated buffer.
  @param ConfigData      Config data on updating driver.
  @param FlashAddress    Flash address to be updated as start address.
  @param FvbProtocol     FVB protocol.
  @param FvbHandle       Handle of FVB protocol for the updated flash range.

  @retval EFI_SUCCESS            Buffer data is updated into flash.
  @retval EFI_INVALID_PARAMETER  Base flash address is not in FVB flash area.
  @retval EFI_NOT_FOUND          FTW protocol doesn't exist.
  @retval EFI_OUT_OF_RESOURCES   No enough backup space.
  @retval EFI_ABORTED            Error happen when update flash area.

**/
EFI_STATUS
FaultTolerantUpdateOnPartFv (
  IN       UINT8                         *ImageBuffer,
  IN       UINTN                         SizeLeft,
  IN OUT   UINTN                         *UpdatedSize,
  IN       UPDATE_CONFIG_DATA            *ConfigData,
  IN       EFI_PHYSICAL_ADDRESS          FlashAddress,
  IN       EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN       EFI_HANDLE                    FvbHandle
  )
{
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeaderTmp;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_PHYSICAL_ADDRESS                  FvBase;
  EFI_PHYSICAL_ADDRESS                  NextBlock;
  EFI_FV_BLOCK_MAP_ENTRY                *BlockMap;
  EFI_FV_BLOCK_MAP_ENTRY                *PtrMap;
  UINTN                                 NumOfUpdates;
  UINTN                                 TotalSize;
  EFI_PHYSICAL_ADDRESS                  StartAddress;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *FtwProtocol;
  UINTN                                 MaxBlockSize;
  UINTN                                 FtwMaxBlockSize;
  BOOLEAN                               Pending;
  UPDATE_PRIVATE_DATA                   PrivateData;
  EFI_LBA                               PendingLba;
  EFI_LBA                               Lba;
  UINTN                                 BlockSize;
  UINTN                                 PendingOffset;
  UINTN                                 Offset;
  UINTN                                 PendingLength;
  UINTN                                 Length;
  UINTN                                 Index;
  UINT8                                 *Image;

  //
  // Get the block map to update the block one by one
  //
  Status = FvbProtocol->GetPhysicalAddress (
                          FvbProtocol,
                          &FvBase
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwVolHeaderTmp = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if ((FlashAddress < FvBase) || (FlashAddress > (FvBase + FwVolHeaderTmp->FvLength))) {
    return EFI_INVALID_PARAMETER;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)AllocateCopyPool (
                                                FwVolHeaderTmp->HeaderLength,
                                                FwVolHeaderTmp
                                                );
  if (FwVolHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // For fault tolerant write, we have to know how many blocks we need to
  // update. So we will calculate number of updates and max block size first
  //
  NumOfUpdates          = 0;
  MaxBlockSize          = 0;
  TotalSize             = SizeLeft;
  StartAddress          = FlashAddress;
  BaseAddress           = FvBase;
  BlockMap              = &(FwVolHeader->BlockMap[0]);
  PtrMap                = BlockMap;

  while (TotalSize > 0) {
    if ((PtrMap->NumBlocks == 0) || (PtrMap->Length == 0)) {
      break;
    }

    BlockSize           = PtrMap->Length;
    for (Index = 0; Index < PtrMap->NumBlocks; Index++) {
      NextBlock         = BaseAddress + BlockSize;
      //
      // Check if this block need to be updated
      //
      if ((StartAddress >= BaseAddress) && (StartAddress < NextBlock)) {
        //
        // Get the maximum block size
        //
        if (MaxBlockSize < BlockSize) {
          MaxBlockSize  = BlockSize;
        }

        //
        // This block shall be udpated. So increment number of updates
        //
        NumOfUpdates++;
        Offset          = (UINTN) (StartAddress - BaseAddress);
        Length          = TotalSize;
        if ((Length + Offset ) > BlockSize) {
          Length        = BlockSize - Offset;
        }

        StartAddress    = StartAddress + Length;
        TotalSize       = TotalSize - Length;
        if (TotalSize <= 0) {
          break;
        }
      }
      BaseAddress       = NextBlock;
    }
    PtrMap++;
  }

  //
  // Get the FTW protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteProtocolGuid,
                  NULL,
                  (VOID **) &FtwProtocol
                  );
  if (EFI_ERROR (Status)) {
    FreePool (FwVolHeader);
    return EFI_NOT_FOUND;
  }

  FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);

  //
  // Not enough backup space. return directly
  //
  if (FtwMaxBlockSize < MaxBlockSize) {
    FreePool (FwVolHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  PendingLba            = 0;
  PendingOffset         = 0;
  PendingLength         = 0;
  Pending               = FALSE;

  //
  // Fault Tolerant Write can only support actual fault tolerance if the write
  // is a reclaim operation, which means the data buffer (new and old) are
  // acutally both stored in flash. But for component update write, the data
  // are now in memory. So we cannot actually recover the data after power
  // failure.
  //
  Status = RetrieveLastWrite (
             FvbHandle,
             FtwProtocol,
             ConfigData,
             sizeof (UPDATE_PRIVATE_DATA),
             &PrivateData,
             &PendingLba,
             &PendingOffset,
             &PendingLength,
             &Pending
             );
  if (Pending && (Status == EFI_NOT_FOUND)) {
    //
    // I'm not the owner of the pending fault tolerant write record
    // Cannot continue with the write operation
    //
    FreePool (FwVolHeader);
    return EFI_ABORTED;
  }

  if (EFI_ERROR(Status)) {
    FreePool (FwVolHeader);
    return EFI_ABORTED;
  }

  //
  // Currently we start from the pending write if there is any. But if the
  // caller is exactly the same, and the new data is already a in memory, (it
  // cannot be stored in flash in last write,) we can just abort last write
  // and start from the very begining.
  //
  if (!Pending) {
    //
    // Now allocte the update private data in FTW. If there is pending
    // write, it has already been allocated and no need to allocate here.
    //
    Status = FtwProtocol->Allocate (
                            FtwProtocol,
                            &gEfiCallerIdGuid,
                            sizeof (UPDATE_PRIVATE_DATA),
                            NumOfUpdates
                            );
    if (EFI_ERROR (Status)) {
      FreePool (FwVolHeader);
      return Status;
    }
  }

  //
  // Perform the update now. If there are pending writes, we need to
  // start from the pending write instead of the very beginning.
  //
  TotalSize             = SizeLeft;
  Lba                   = 0;
  StartAddress          = FlashAddress;
  BaseAddress           = FvBase;
  PtrMap                = BlockMap;
  Image                 = ImageBuffer;
  CopyMem (
    (VOID *) &PrivateData.FileGuid,
    (VOID *) &ConfigData->FileGuid,
     sizeof (EFI_GUID)
  );

  while (TotalSize > 0) {
    if ((PtrMap->NumBlocks == 0) || (PtrMap->Length == 0)) {
      break;
    }

    BlockSize           = (UINTN)PtrMap->Length;
    for (Index = 0;  Index < PtrMap->NumBlocks; Index++) {
      NextBlock         = BaseAddress + BlockSize;
      if ((StartAddress >= BaseAddress) && (StartAddress < NextBlock)) {
        //
        // So we need to update this block
        //
        Offset          = (UINTN) (StartAddress - BaseAddress);
        Length          = TotalSize;
        if ((Length + Offset ) > BlockSize) {
          Length        = BlockSize - Offset;
        }

        //
        // Add an extra check here to see if the pending record is correct
        //
        if (Pending && (Lba == PendingLba)) {
          if ((PendingOffset != Offset) || (PendingLength != Length)) {
            //
            // Error.
            //
            Status          = EFI_ABORTED;
            break;
          }
        }

        if ((!Pending) || (Lba >= PendingLba)) {
          DEBUG ((EFI_D_UPDATE, "Update Flash area from %08LX to %08LX\n", StartAddress, (UINT64)StartAddress + Length));
          Status            = FtwProtocol->Write (
                                             FtwProtocol,
                                             Lba,                  // Lba
                                             Offset,               // Offset
                                             Length,               // Size
                                             &PrivateData,         // Private Data
                                             FvbHandle,            // FVB handle
                                             Image                 // Buffer
                                             );
          if (EFI_ERROR (Status)) {
            break;
          }
        }

        //
        // Now increment StartAddress, ImageBuffer and decrease the
        // left size to prepare for the next block update.
        //
        StartAddress    = StartAddress + Length;
        Image           = Image + Length;
        TotalSize       = TotalSize - Length;
        if (TotalSize <= 0) {
          break;
        }
      }
      BaseAddress       = NextBlock;
      Lba++;
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    PtrMap++;
  }

  FreePool (FwVolHeader);

  *UpdatedSize = SizeLeft - TotalSize;

  return EFI_SUCCESS;
}

/**
  Directly update the buffer into flash area without fault tolerant write method.

  @param ImageBuffer     Image buffer to be updated.
  @param SizeLeft        Size of the image buffer.
  @param UpdatedSize     Size of the updated buffer.
  @param FlashAddress    Flash address to be updated as start address.
  @param FvbProtocol     FVB protocol.
  @param FvbHandle       Handle of FVB protocol for the updated flash range.

  @retval EFI_SUCCESS            Buffer data is updated into flash.
  @retval EFI_INVALID_PARAMETER  Base flash address is not in FVB flash area.
  @retval EFI_OUT_OF_RESOURCES   No enough backup space.

**/
EFI_STATUS
NonFaultTolerantUpdateOnPartFv (
  IN      UINT8                         *ImageBuffer,
  IN      UINTN                         SizeLeft,
  IN OUT  UINTN                         *UpdatedSize,
  IN      EFI_PHYSICAL_ADDRESS          FlashAddress,
  IN      EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN      EFI_HANDLE                    FvbHandle
  )
{
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeaderTmp;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_PHYSICAL_ADDRESS                  NextBlock;
  EFI_FV_BLOCK_MAP_ENTRY                *BlockMap;
  UINTN                                 Index;
  UINTN                                 TotalSize;
  UINTN                                 BlockSize;
  EFI_LBA                               Lba;
  UINTN                                 Offset;
  UINTN                                 Length;
  UINT8                                 *Image;

  //
  // Get the block map to update the block one by one
  //
  Status                = FvbProtocol->GetPhysicalAddress (
                                         FvbProtocol,
                                         &BaseAddress
                                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwVolHeaderTmp = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BaseAddress;
  if ((FlashAddress < BaseAddress) || (FlashAddress > ( BaseAddress + FwVolHeaderTmp->FvLength ))) {
    return EFI_INVALID_PARAMETER;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)AllocateCopyPool (
                                                FwVolHeaderTmp->HeaderLength,
                                                FwVolHeaderTmp
                                                );
  if (FwVolHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Image                 = ImageBuffer;
  TotalSize             = SizeLeft;
  BlockMap              = &(FwVolHeader->BlockMap[0]);
  Lba                   = 0;

  while (TotalSize > 0) {
    if ((BlockMap->NumBlocks == 0) || (BlockMap->Length == 0)) {
      break;
    }

    BlockSize           = BlockMap->Length;
    for (Index = 0 ; Index < BlockMap->NumBlocks ; Index++) {
      NextBlock         = BaseAddress + BlockSize;
      if ((FlashAddress >= BaseAddress) && (FlashAddress < NextBlock)) {
        //
        // So we need to update this block
        //
        Offset          = (UINTN) FlashAddress - (UINTN) BaseAddress;
        Length          = TotalSize;
        if ((Length + Offset ) > BlockSize) {
          Length        = BlockSize - Offset;
        }

        DEBUG ((EFI_D_UPDATE, "Update Flash area from %08LX to %08LX\n", FlashAddress, (UINT64)FlashAddress + Length));
        //
        // Update the block
        //
        Status          = UpdateBufferInOneBlock (
                            FvbProtocol,
                            Lba,
                            Offset,
                            Length,
                            BlockSize,
                            Image
                            );
        if (EFI_ERROR (Status)) {
          FreePool (FwVolHeader);
          return Status;
        }

        //
        // Now increment FlashAddress, ImageBuffer and decrease the
        // left size to prepare for the next block update.
        //
        FlashAddress    = FlashAddress + Length;
        Image           = Image + Length;
        TotalSize       = TotalSize - Length;
        if (TotalSize <= 0) {
          break;
        }
      }
      BaseAddress       = NextBlock;
      Lba++;
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    BlockMap++;
  }

  FreePool (FwVolHeader);

  *UpdatedSize          = SizeLeft - TotalSize;

  return EFI_SUCCESS;
}
