/** @file
  Firmware File System driver that produce Firmware Volume protocol.
  Layers on top of Firmware Block protocol to produce a file abstraction
  of FV based files.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "FwVolDriver.h"

//
// Protocol notify related globals
//
VOID       *gEfiFwVolBlockNotifyReg;
EFI_EVENT  gEfiFwVolBlockEvent;

FV_DEVICE  mFvDevice = {
  FV2_DEVICE_SIGNATURE,
  NULL,
  NULL,
  {
    FvGetVolumeAttributes,
    FvSetVolumeAttributes,
    FvReadFile,
    FvReadFileSection,
    FvWriteFile,
    FvGetNextFile,
    sizeof (UINTN),
    NULL,
    FvGetVolumeInfo,
    FvSetVolumeInfo
  },
  NULL,
  NULL,
  NULL,
  NULL,
  { NULL,                 NULL},
  0,
  0,
  FALSE,
  FALSE
};

//
// FFS helper functions
//

/**
  Read data from Firmware Block by FVB protocol Read.
  The data may cross the multi block ranges.

  @param  Fvb                   The FW_VOL_BLOCK_PROTOCOL instance from which to read data.
  @param  StartLba              Pointer to StartLba.
                                On input, the start logical block index from which to read.
                                On output,the end logical block index after reading.
  @param  Offset                Pointer to Offset
                                On input, offset into the block at which to begin reading.
                                On output, offset into the end block after reading.
  @param  DataSize              Size of data to be read.
  @param  Data                  Pointer to Buffer that the data will be read into.

  @retval EFI_SUCCESS           Successfully read data from firmware block.
  @retval others
**/
EFI_STATUS
ReadFvbData (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN OUT EFI_LBA                             *StartLba,
  IN OUT UINTN                               *Offset,
  IN     UINTN                               DataSize,
  OUT    UINT8                               *Data
  )
{
  UINTN       BlockSize;
  UINTN       NumberOfBlocks;
  UINTN       BlockIndex;
  UINTN       ReadDataSize;
  EFI_STATUS  Status;

  //
  // Try read data in current block
  //
  BlockIndex   = 0;
  ReadDataSize = DataSize;
  Status       = Fvb->Read (Fvb, *StartLba, *Offset, &ReadDataSize, Data);
  if (Status == EFI_SUCCESS) {
    *Offset += DataSize;
    return EFI_SUCCESS;
  } else if (Status != EFI_BAD_BUFFER_SIZE) {
    //
    // other error will direct return
    //
    return Status;
  }

  //
  // Data crosses the blocks, read data from next block
  //
  DataSize -= ReadDataSize;
  Data     += ReadDataSize;
  *StartLba = *StartLba + 1;
  while (DataSize > 0) {
    Status = Fvb->GetBlockSize (Fvb, *StartLba, &BlockSize, &NumberOfBlocks);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Read data from the crossing blocks
    //
    BlockIndex = 0;
    while (BlockIndex < NumberOfBlocks && DataSize >= BlockSize) {
      Status = Fvb->Read (Fvb, *StartLba + BlockIndex, 0, &BlockSize, Data);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Data     += BlockSize;
      DataSize -= BlockSize;
      BlockIndex++;
    }

    //
    // Data doesn't exceed the current block range.
    //
    if (DataSize < BlockSize) {
      break;
    }

    //
    // Data must be got from the next block range.
    //
    *StartLba += NumberOfBlocks;
  }

  //
  // read the remaining data
  //
  if (DataSize > 0) {
    Status = Fvb->Read (Fvb, *StartLba + BlockIndex, 0, &DataSize, Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Update Lba and Offset used by the following read.
  //
  *StartLba += BlockIndex;
  *Offset    = DataSize;

  return EFI_SUCCESS;
}

/**
  Given the supplied FW_VOL_BLOCK_PROTOCOL, allocate a buffer for output and
  copy the real length volume header into it.

  @param  Fvb                   The FW_VOL_BLOCK_PROTOCOL instance from which to
                                read the volume header
  @param  FwVolHeader           Pointer to pointer to allocated buffer in which
                                the volume header is returned.

  @retval EFI_OUT_OF_RESOURCES  No enough buffer could be allocated.
  @retval EFI_SUCCESS           Successfully read volume header to the allocated
                                buffer.
  @retval EFI_INVALID_PARAMETER The FV Header signature is not as expected or
                                the file system could not be understood.

**/
EFI_STATUS
GetFwVolHeader (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  OUT    EFI_FIRMWARE_VOLUME_HEADER          **FwVolHeader
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  TempFvh;
  UINTN                       FvhLength;
  EFI_LBA                     StartLba;
  UINTN                       Offset;
  UINT8                       *Buffer;

  //
  // Read the standard FV header
  //
  StartLba  = 0;
  Offset    = 0;
  FvhLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  Status    = ReadFvbData (Fvb, &StartLba, &Offset, FvhLength, (UINT8 *)&TempFvh);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Validate FV Header signature, if not as expected, continue.
  //
  if (TempFvh.Signature != EFI_FVH_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see that the file system is indeed formatted in a way we can
  // understand it...
  //
  if ((!CompareGuid (&TempFvh.FileSystemGuid, &gEfiFirmwareFileSystem2Guid)) &&
      (!CompareGuid (&TempFvh.FileSystemGuid, &gEfiFirmwareFileSystem3Guid)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate a buffer for the caller
  //
  *FwVolHeader = AllocatePool (TempFvh.HeaderLength);
  if (*FwVolHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the standard header into the buffer
  //
  CopyMem (*FwVolHeader, &TempFvh, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

  //
  // Read the rest of the header
  //
  FvhLength = TempFvh.HeaderLength - sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  Buffer    = (UINT8 *)*FwVolHeader + sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  Status    = ReadFvbData (Fvb, &StartLba, &Offset, FvhLength, Buffer);
  if (EFI_ERROR (Status)) {
    //
    // Read failed so free buffer
    //
    CoreFreePool (*FwVolHeader);
  }

  return Status;
}

/**
  Free FvDevice resource when error happens

  @param  FvDevice              pointer to the FvDevice to be freed.

**/
VOID
FreeFvDeviceResource (
  IN FV_DEVICE  *FvDevice
  )
{
  FFS_FILE_LIST_ENTRY  *FfsFileEntry;
  LIST_ENTRY           *NextEntry;

  //
  // Free File List Entry
  //
  FfsFileEntry = (FFS_FILE_LIST_ENTRY *)FvDevice->FfsFileListHeader.ForwardLink;
  while (&FfsFileEntry->Link != &FvDevice->FfsFileListHeader) {
    NextEntry = (&FfsFileEntry->Link)->ForwardLink;

    if (FfsFileEntry->StreamHandle != 0) {
      //
      // Close stream and free resources from SEP
      //
      CloseSectionStream (FfsFileEntry->StreamHandle, FALSE);
    }

    if (FfsFileEntry->FileCached) {
      //
      // Free the cached file buffer.
      //
      CoreFreePool (FfsFileEntry->FfsHeader);
    }

    CoreFreePool (FfsFileEntry);

    FfsFileEntry = (FFS_FILE_LIST_ENTRY *)NextEntry;
  }

  if (!FvDevice->IsMemoryMapped) {
    //
    // Free the cached FV buffer.
    //
    CoreFreePool (FvDevice->CachedFv);
  }

  //
  // Free Volume Header
  //
  CoreFreePool (FvDevice->FwVolHeader);

  return;
}

/**
  Check if an FV is consistent and allocate cache for it.

  @param  FvDevice              A pointer to the FvDevice to be checked.

  @retval EFI_OUT_OF_RESOURCES  No enough buffer could be allocated.
  @retval EFI_SUCCESS           FV is consistent and cache is allocated.
  @retval EFI_VOLUME_CORRUPTED  File system is corrupted.

**/
EFI_STATUS
FvCheck (
  IN OUT FV_DEVICE  *FvDevice
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER      *FwVolExtHeader;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;
  EFI_FV_BLOCK_MAP_ENTRY              *BlockMap;
  FFS_FILE_LIST_ENTRY                 *FfsFileEntry;
  EFI_FFS_FILE_HEADER                 *FfsHeader;
  UINT8                               *CacheLocation;
  UINTN                               Index;
  EFI_LBA                             LbaIndex;
  UINTN                               Size;
  EFI_FFS_FILE_STATE                  FileState;
  UINT8                               *TopFvAddress;
  UINTN                               TestLength;
  EFI_PHYSICAL_ADDRESS                PhysicalAddress;
  BOOLEAN                             FileCached;
  UINTN                               WholeFileSize;
  EFI_FFS_FILE_HEADER                 *CacheFfsHeader;

  FileCached     = FALSE;
  CacheFfsHeader = NULL;

  Fvb         = FvDevice->Fvb;
  FwVolHeader = FvDevice->FwVolHeader;

  Status = Fvb->GetAttributes (Fvb, &FvbAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Size = (UINTN)FwVolHeader->FvLength;
  if ((FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {
    FvDevice->IsMemoryMapped = TRUE;

    Status = Fvb->GetPhysicalAddress (Fvb, &PhysicalAddress);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Don't cache memory mapped FV really.
    //
    FvDevice->CachedFv = (UINT8 *)(UINTN)PhysicalAddress;
  } else {
    FvDevice->IsMemoryMapped = FALSE;
    FvDevice->CachedFv       = AllocatePool (Size);

    if (FvDevice->CachedFv == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Remember a pointer to the end of the CachedFv
  //
  FvDevice->EndOfCachedFv = FvDevice->CachedFv + Size;

  if (!FvDevice->IsMemoryMapped) {
    //
    // Copy FV into memory using the block map.
    //
    BlockMap      = FwVolHeader->BlockMap;
    CacheLocation = FvDevice->CachedFv;
    LbaIndex      = 0;
    while ((BlockMap->NumBlocks != 0) || (BlockMap->Length != 0)) {
      //
      // read the FV data
      //
      Size = BlockMap->Length;
      for (Index = 0; Index < BlockMap->NumBlocks; Index++) {
        Status = Fvb->Read (
                        Fvb,
                        LbaIndex,
                        0,
                        &Size,
                        CacheLocation
                        );

        //
        // Not check EFI_BAD_BUFFER_SIZE, for Size = BlockMap->Length
        //
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        LbaIndex++;
        CacheLocation += BlockMap->Length;
      }

      BlockMap++;
    }
  }

  //
  // Scan to check the free space & File list
  //
  if ((FvbAttributes & EFI_FVB2_ERASE_POLARITY) != 0) {
    FvDevice->ErasePolarity = 1;
  } else {
    FvDevice->ErasePolarity = 0;
  }

  //
  // go through the whole FV cache, check the consistence of the FV.
  // Make a linked list of all the Ffs file headers
  //
  Status = EFI_SUCCESS;
  InitializeListHead (&FvDevice->FfsFileListHeader);

  //
  // Build FFS list
  //
  if (FwVolHeader->ExtHeaderOffset != 0) {
    //
    // Searching for files starts on an 8 byte aligned boundary after the end of the Extended Header if it exists.
    //
    FwVolExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(FvDevice->CachedFv + FwVolHeader->ExtHeaderOffset);
    FfsHeader      = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolExtHeader + FwVolExtHeader->ExtHeaderSize);
  } else {
    FfsHeader = (EFI_FFS_FILE_HEADER *)(FvDevice->CachedFv + FwVolHeader->HeaderLength);
  }

  FfsHeader    = (EFI_FFS_FILE_HEADER *)ALIGN_POINTER (FfsHeader, 8);
  TopFvAddress = FvDevice->EndOfCachedFv;
  while (((UINTN)FfsHeader >= (UINTN)FvDevice->CachedFv) && ((UINTN)FfsHeader <= (UINTN)((UINTN)TopFvAddress - sizeof (EFI_FFS_FILE_HEADER)))) {
    if (FileCached) {
      CoreFreePool (CacheFfsHeader);
      FileCached = FALSE;
    }

    TestLength = TopFvAddress - ((UINT8 *)FfsHeader);
    if (TestLength > sizeof (EFI_FFS_FILE_HEADER)) {
      TestLength = sizeof (EFI_FFS_FILE_HEADER);
    }

    if (IsBufferErased (FvDevice->ErasePolarity, FfsHeader, TestLength)) {
      //
      // We have found the free space so we are done!
      //
      goto Done;
    }

    if (!IsValidFfsHeader (FvDevice->ErasePolarity, FfsHeader, &FileState)) {
      if ((FileState == EFI_FILE_HEADER_INVALID) ||
          (FileState == EFI_FILE_HEADER_CONSTRUCTION))
      {
        if (IS_FFS_FILE2 (FfsHeader)) {
          if (!FvDevice->IsFfs3Fv) {
            DEBUG ((DEBUG_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FfsHeader->Name));
          }

          FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsHeader + sizeof (EFI_FFS_FILE_HEADER2));
        } else {
          FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsHeader + sizeof (EFI_FFS_FILE_HEADER));
        }

        continue;
      } else {
        //
        // File system is corrputed
        //
        Status = EFI_VOLUME_CORRUPTED;
        goto Done;
      }
    }

    CacheFfsHeader = FfsHeader;
    if ((CacheFfsHeader->Attributes & FFS_ATTRIB_CHECKSUM) == FFS_ATTRIB_CHECKSUM) {
      if (FvDevice->IsMemoryMapped) {
        //
        // Memory mapped FV has not been cached.
        // Here is to cache FFS file to memory buffer for following checksum calculating.
        // And then, the cached file buffer can be also used for FvReadFile.
        //
        WholeFileSize  = IS_FFS_FILE2 (CacheFfsHeader) ? FFS_FILE2_SIZE (CacheFfsHeader) : FFS_FILE_SIZE (CacheFfsHeader);
        CacheFfsHeader = AllocateCopyPool (WholeFileSize, CacheFfsHeader);
        if (CacheFfsHeader == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        FileCached = TRUE;
      }
    }

    if (!IsValidFfsFile (FvDevice->ErasePolarity, CacheFfsHeader)) {
      //
      // File system is corrupted
      //
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    if (IS_FFS_FILE2 (CacheFfsHeader)) {
      ASSERT (FFS_FILE2_SIZE (CacheFfsHeader) > 0x00FFFFFF);
      if (!FvDevice->IsFfs3Fv) {
        DEBUG ((DEBUG_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &CacheFfsHeader->Name));
        FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsHeader + FFS_FILE2_SIZE (CacheFfsHeader));
        //
        // Adjust pointer to the next 8-byte aligned boundary.
        //
        FfsHeader = (EFI_FFS_FILE_HEADER *)(((UINTN)FfsHeader + 7) & ~0x07);
        continue;
      }
    }

    FileState = GetFileState (FvDevice->ErasePolarity, CacheFfsHeader);

    //
    // check for non-deleted file
    //
    if (FileState != EFI_FILE_DELETED) {
      //
      // Create a FFS list entry for each non-deleted file
      //
      FfsFileEntry = AllocateZeroPool (sizeof (FFS_FILE_LIST_ENTRY));
      if (FfsFileEntry == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      FfsFileEntry->FfsHeader  = CacheFfsHeader;
      FfsFileEntry->FileCached = FileCached;
      FileCached               = FALSE;
      InsertTailList (&FvDevice->FfsFileListHeader, &FfsFileEntry->Link);
    }

    if (IS_FFS_FILE2 (CacheFfsHeader)) {
      FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsHeader + FFS_FILE2_SIZE (CacheFfsHeader));
    } else {
      FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsHeader + FFS_FILE_SIZE (CacheFfsHeader));
    }

    //
    // Adjust pointer to the next 8-byte aligned boundary.
    //
    FfsHeader = (EFI_FFS_FILE_HEADER *)(((UINTN)FfsHeader + 7) & ~0x07);
  }

Done:
  if (EFI_ERROR (Status)) {
    if (FileCached) {
      CoreFreePool (CacheFfsHeader);
      FileCached = FALSE;
    }

    FreeFvDeviceResource (FvDevice);
  }

  return Status;
}

/**
  This notification function is invoked when an instance of the
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL is produced.  It layers an instance of the
  EFI_FIRMWARE_VOLUME2_PROTOCOL on the same handle.  This is the function where
  the actual initialization of the EFI_FIRMWARE_VOLUME2_PROTOCOL is done.

  @param  Event                 The event that occurred
  @param  Context               For EFI compatiblity.  Not used.

**/
VOID
EFIAPI
NotifyFwVolBlock (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_HANDLE                          Handle;
  EFI_STATUS                          Status;
  UINTN                               BufferSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME2_PROTOCOL       *Fv;
  FV_DEVICE                           *FvDevice;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;

  //
  // Examine all new handles
  //
  for ( ; ;) {
    //
    // Get the next handle
    //
    BufferSize = sizeof (Handle);
    Status     = CoreLocateHandle (
                   ByRegisterNotify,
                   NULL,
                   gEfiFwVolBlockNotifyReg,
                   &BufferSize,
                   &Handle
                   );

    //
    // If not found, we're done
    //
    if (EFI_NOT_FOUND == Status) {
      break;
    }

    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Get the FirmwareVolumeBlock protocol on that handle
    //
    Status = CoreHandleProtocol (Handle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **)&Fvb);
    ASSERT_EFI_ERROR (Status);
    ASSERT (Fvb != NULL);

    //
    // Make sure the Fv Header is O.K.
    //
    Status = GetFwVolHeader (Fvb, &FwVolHeader);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (FwVolHeader != NULL);

    if (!VerifyFvHeaderChecksum (FwVolHeader)) {
      CoreFreePool (FwVolHeader);
      continue;
    }

    //
    // Check if there is an FV protocol already installed in that handle
    //
    Status = CoreHandleProtocol (Handle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);
    if (!EFI_ERROR (Status)) {
      //
      // Update Fv to use a new Fvb
      //
      FvDevice = BASE_CR (Fv, FV_DEVICE, Fv);
      if (FvDevice->Signature == FV2_DEVICE_SIGNATURE) {
        //
        // Only write into our device structure if it's our device structure
        //
        FvDevice->Fvb = Fvb;
      }
    } else {
      //
      // No FwVol protocol on the handle so create a new one
      //
      FvDevice = AllocateCopyPool (sizeof (FV_DEVICE), &mFvDevice);
      if (FvDevice == NULL) {
        CoreFreePool (FwVolHeader);
        return;
      }

      FvDevice->Fvb             = Fvb;
      FvDevice->Handle          = Handle;
      FvDevice->FwVolHeader     = FwVolHeader;
      FvDevice->IsFfs3Fv        = CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid);
      FvDevice->Fv.ParentHandle = Fvb->ParentHandle;
      //
      // Inherit the authentication status from FVB.
      //
      FvDevice->AuthenticationStatus = GetFvbAuthenticationStatus (Fvb);

      if (!EFI_ERROR (FvCheck (FvDevice))) {
        //
        // Install an New FV protocol on the existing handle
        //
        Status = CoreInstallProtocolInterface (
                   &Handle,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   &FvDevice->Fv
                   );
        ASSERT_EFI_ERROR (Status);
      } else {
        //
        // Free FvDevice Buffer for the corrupt FV image.
        //
        CoreFreePool (FvDevice);
      }
    }
  }

  return;
}

/**
  This routine is the driver initialization entry point.  It registers
  a notification function.  This notification function are responsible
  for building the FV stack dynamically.

  @param  ImageHandle           The image handle.
  @param  SystemTable           The system table.

  @retval EFI_SUCCESS           Function successfully returned.

**/
EFI_STATUS
EFIAPI
FwVolDriverInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gEfiFwVolBlockEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiFirmwareVolumeBlockProtocolGuid,
                          TPL_CALLBACK,
                          NotifyFwVolBlock,
                          NULL,
                          &gEfiFwVolBlockNotifyReg
                          );
  return EFI_SUCCESS;
}
