/** @file
  Firmware File System driver that produce Firmware Volume protocol.
  Layers on top of Firmware Block protocol to produce a file abstraction
  of FV based files.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"

#define KEYSIZE       sizeof (UINTN)

//
// Protocol notify related globals
//
VOID          *gEfiFwVolBlockNotifyReg;
EFI_EVENT     gEfiFwVolBlockEvent;

FV_DEVICE mFvDevice = {
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
    KEYSIZE,
    NULL,
    FvGetVolumeInfo,
    FvSetVolumeInfo
  },
  NULL,
  NULL,
  NULL,
  NULL,
  { NULL, NULL },
  0
};


//
// FFS helper functions
//
/**
  given the supplied FW_VOL_BLOCK_PROTOCOL, allocate a buffer for output and
  copy the volume header into it.

  @param  Fvb                   The FW_VOL_BLOCK_PROTOCOL instance from which to
                                read the volume header
  @param  FwVolHeader           Pointer to pointer to allocated buffer in which
                                the volume header is returned.

  @retval EFI_OUT_OF_RESOURCES  No enough buffer could be allocated.
  @retval EFI_SUCCESS           Successfully read volume header to the allocated
                                buffer.

**/
EFI_STATUS
GetFwVolHeader (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *Fvb,
  OUT    EFI_FIRMWARE_VOLUME_HEADER             **FwVolHeader
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  TempFvh;
  UINTN                       FvhLength;
  UINT8                       *Buffer;


  //
  //Determine the real length of FV header
  //
  FvhLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  Status = Fvb->Read (Fvb, 0, 0, &FvhLength, (UINT8 *)&TempFvh);

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
  Buffer = (UINT8 *)*FwVolHeader + sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  Status = Fvb->Read (Fvb, 0, sizeof (EFI_FIRMWARE_VOLUME_HEADER), &FvhLength, Buffer);
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
  FFS_FILE_LIST_ENTRY         *FfsFileEntry;
  LIST_ENTRY                  *NextEntry;

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
      CloseSectionStream (FfsFileEntry->StreamHandle);
    }

    CoreFreePool (FfsFileEntry);

    FfsFileEntry = (FFS_FILE_LIST_ENTRY *) NextEntry;
  }


  //
  // Free the cache
  //
  CoreFreePool (FvDevice->CachedFv);

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
  EFI_STATUS                            Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FVB_ATTRIBUTES                    FvbAttributes;
  EFI_FV_BLOCK_MAP_ENTRY                *BlockMap;
  FFS_FILE_LIST_ENTRY                   *FfsFileEntry;
  EFI_FFS_FILE_HEADER                   *FfsHeader;
  UINT8                                 *CacheLocation;
  UINTN                                 LbaOffset;
  UINTN                                 Index;
  EFI_LBA                               LbaIndex;
  UINTN                                 Size;
  UINTN                                 FileLength;
  EFI_FFS_FILE_STATE                    FileState;
  UINT8                                 *TopFvAddress;
  UINTN                                 TestLength;


  Fvb = FvDevice->Fvb;
  FwVolHeader = FvDevice->FwVolHeader;

  Status = Fvb->GetAttributes (Fvb, &FvbAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Size is the size of the FV minus the head. We have already allocated
  // the header to check to make sure the volume is valid
  //
  Size = (UINTN)(FwVolHeader->FvLength - FwVolHeader->HeaderLength);
  FvDevice->CachedFv = AllocatePool (Size);

  if (FvDevice->CachedFv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Remember a pointer to the end fo the CachedFv
  //
  FvDevice->EndOfCachedFv = FvDevice->CachedFv + Size;

  //
  // Copy FV minus header into memory using the block map we have all ready
  // read into memory.
  //
  BlockMap = FwVolHeader->BlockMap;
  CacheLocation = FvDevice->CachedFv;
  LbaIndex = 0;
  LbaOffset = FwVolHeader->HeaderLength;
  while ((BlockMap->NumBlocks != 0) || (BlockMap->Length != 0)) {

    for (Index = 0; Index < BlockMap->NumBlocks; Index ++) {

      Size = BlockMap->Length;
      if (Index == 0) {
        //
        // Cache does not include FV Header
        //
        Size -= LbaOffset;
      }
      Status = Fvb->Read (Fvb,
                      LbaIndex,
                      LbaOffset,
                      &Size,
                      CacheLocation
                      );
      //
      // Not check EFI_BAD_BUFFER_SIZE, for Size = BlockMap->Length
      //
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // After we skip Fv Header always read from start of block
      //
      LbaOffset = 0;

      LbaIndex++;
      CacheLocation += Size;
    }
    BlockMap++;
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
  // Make a linked list off all the Ffs file headers
  //
  Status = EFI_SUCCESS;
  InitializeListHead (&FvDevice->FfsFileListHeader);

  //
  // Build FFS list
  //
  FfsHeader = (EFI_FFS_FILE_HEADER *) FvDevice->CachedFv;
  TopFvAddress = FvDevice->EndOfCachedFv;
  while ((UINT8 *) FfsHeader < TopFvAddress) {

    TestLength = TopFvAddress - ((UINT8 *) FfsHeader);
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
          (FileState == EFI_FILE_HEADER_CONSTRUCTION)) {
        FfsHeader++;
        continue;
      } else {
        //
        // File system is corrputed
        //
        Status = EFI_VOLUME_CORRUPTED;
        goto Done;
      }
    }

    if (!IsValidFfsFile (FvDevice->ErasePolarity, FfsHeader)) {
      //
      // File system is corrupted
      //
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    //
    // Size[3] is a three byte array, read 4 bytes and throw one away
    //
    FileLength = *(UINT32 *)&FfsHeader->Size[0] & 0x00FFFFFF;

    FileState = GetFileState (FvDevice->ErasePolarity, FfsHeader);

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

      FfsFileEntry->FfsHeader = FfsHeader;
      InsertTailList (&FvDevice->FfsFileListHeader, &FfsFileEntry->Link);
    }

    FfsHeader =  (EFI_FFS_FILE_HEADER *)(((UINT8 *)FfsHeader) + FileLength);

    //
    // Adjust pointer to the next 8-byte aligned boundry.
    //
    FfsHeader = (EFI_FFS_FILE_HEADER *)(((UINTN)FfsHeader + 7) & ~0x07);

  }

Done:
  if (EFI_ERROR (Status)) {
    FreeFvDeviceResource (FvDevice);
  }

  return Status;
}



/**
  This notification function is invoked when an instance of the
  EFI_FW_VOLUME_BLOCK_PROTOCOL is produced.  It layers an instance of the
  EFI_FIRMWARE_VOLUME2_PROTOCOL on the same handle.  This is the function where
  the actual initialization of the EFI_FIRMWARE_VOLUME2_PROTOCOL is done.

  @param  Event                 The event that occured
  @param  Context               For EFI compatiblity.  Not used.

**/
VOID
EFIAPI
NotifyFwVolBlock (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  EFI_HANDLE                            Handle;
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_FIRMWARE_VOLUME2_PROTOCOL         *Fv;
  FV_DEVICE                             *FvDevice;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  //
  // Examine all new handles
  //
  for (;;) {
    //
    // Get the next handle
    //
    BufferSize = sizeof (Handle);
    Status = CoreLocateHandle (
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


    //
    // Make sure the Fv Header is O.K.
    //
    Status = GetFwVolHeader (Fvb, &FwVolHeader);
    if (EFI_ERROR (Status)) {
      return;
    }

    if (!VerifyFvHeaderChecksum (FwVolHeader)) {
      CoreFreePool (FwVolHeader);
      continue;
    }


    //
    // Check to see that the file system is indeed formatted in a way we can
    // understand it...
    //
    if (!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid)) {
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
      FvDevice = _CR (Fv, FV_DEVICE, Fv);
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
        return;
      }

      FvDevice->Fvb             = Fvb;
      FvDevice->Handle          = Handle;
      FvDevice->FwVolHeader     = FwVolHeader;
      FvDevice->Fv.ParentHandle = Fvb->ParentHandle;

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
    }
  }

  return;
}



/**
  This routine is the driver initialization entry point.  It initializes the
  libraries, and registers two notification functions.  These notification
  functions are responsible for building the FV stack dynamically.

  @param  ImageHandle           The image handle.
  @param  SystemTable           The system table.

  @retval EFI_SUCCESS           Function successfully returned.

**/
EFI_STATUS
EFIAPI
FwVolDriverInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  gEfiFwVolBlockEvent = CoreCreateProtocolNotifyEvent (
                          &gEfiFirmwareVolumeBlockProtocolGuid,
                          TPL_CALLBACK,
                          NotifyFwVolBlock,
                          NULL,
                          &gEfiFwVolBlockNotifyReg,
                          TRUE
                          );
  return EFI_SUCCESS;
}


