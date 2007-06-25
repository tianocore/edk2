/**@file
  Firmware File System driver that produce Firmware Volume protocol.
  Layers on top of Firmware Block protocol to produce a file abstraction 
  of FV based files.
  
Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <DxeMain.h>

#define KEYSIZE       sizeof (UINTN)

//
// Protocol notify related globals
//
VOID          *gEfiFwVolBlockNotifyReg;
EFI_EVENT     gEfiFwVolBlockEvent;

FV_DEVICE mFvDevice = {
  FV_DEVICE_SIGNATURE,
  NULL,
  NULL,
  {
    FvGetVolumeAttributes,
    FvSetVolumeAttributes,
    FvReadFile,
    FvReadFileSection,
    FvWriteFile,
    FvGetNextFile,
    KEYSIZE
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

EFI_STATUS
GetFwVolHeader (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *Fvb,
  OUT    EFI_FIRMWARE_VOLUME_HEADER             **FwVolHeader
  )
/*++

Routine Description:
  given the supplied FW_VOL_BLOCK_PROTOCOL, allocate a buffer for output and
  copy the volume header into it.

Arguments:
  Fvb - The FW_VOL_BLOCK_PROTOCOL instance from which to read the volume
          header
  FwVolHeader - Pointer to pointer to allocated buffer in which the volume
                  header is returned.

Returns:
  EFI_OUT_OF_RESOURCES    - No enough buffer could be allocated.
  EFI_SUCCESS             - Successfully read volume header to the allocated buffer.

--*/

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
  *FwVolHeader = CoreAllocateBootServicesPool (TempFvh.HeaderLength);
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


STATIC
VOID
FreeFvDeviceResource (
  IN FV_DEVICE  *FvDevice
  )
/*++

Routine Description:
  Free FvDevice resource when error happens

Arguments:
  FvDevice - pointer to the FvDevice to be freed.

Returns:
  None.

--*/
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
      FfsFileEntry->Sep->CloseSectionStream (FfsFileEntry->Sep, FfsFileEntry->StreamHandle);
    }

    CoreFreePool (FfsFileEntry);

    FfsFileEntry = (FFS_FILE_LIST_ENTRY *)NextEntry;
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


EFI_STATUS
FvCheck (
  IN OUT FV_DEVICE  *FvDevice
  )
/*++

Routine Description:
  Check if a FV is consistent and allocate cache

Arguments:
  FvDevice - pointer to the FvDevice to be checked.

Returns:
  EFI_OUT_OF_RESOURCES    - No enough buffer could be allocated.
  EFI_SUCCESS             - FV is consistent and cache is allocated.
  EFI_VOLUME_CORRUPTED    - File system is corrupted.

--*/
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
 
  Status = Fvb->GetVolumeAttributes (Fvb, &FvbAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Size is the size of the FV minus the head. We have already allocated
  // the header to check to make sure the volume is valid
  //
  Size = (UINTN)(FwVolHeader->FvLength - FwVolHeader->HeaderLength);
  FvDevice->CachedFv = CoreAllocateBootServicesPool (Size);

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
  BlockMap = FwVolHeader->FvBlockMap;
  CacheLocation = FvDevice->CachedFv;
  LbaIndex = 0;
  LbaOffset = FwVolHeader->HeaderLength;
  while ((BlockMap->NumBlocks != 0) || (BlockMap->BlockLength != 0)) {
    
    for (Index = 0; Index < BlockMap->NumBlocks; Index ++) {

      Size = BlockMap->BlockLength;
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
      // Not check EFI_BAD_BUFFER_SIZE, for Size = BlockMap->BlockLength
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
  if (FvbAttributes & EFI_FVB_ERASE_POLARITY) {
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
  FfsHeader = (EFI_FFS_FILE_HEADER *)FvDevice->CachedFv;
  TopFvAddress = FvDevice->EndOfCachedFv;
  while ((UINT8 *)FfsHeader < TopFvAddress) {

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
      FfsFileEntry = CoreAllocateZeroBootServicesPool (sizeof (FFS_FILE_LIST_ENTRY));
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


STATIC
VOID
EFIAPI
NotifyFwVolBlock (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
/*++

Routine Description:
    This notification function is invoked when an instance of the
    EFI_FW_VOLUME_BLOCK_PROTOCOL is produced.  It layers an instance of the
    EFI_FIRMWARE_VOLUME_PROTOCOL on the same handle.  This is the function where
    the actual initialization of the EFI_FIRMWARE_VOLUME_PROTOCOL is done.

Arguments:
    Event - The event that occured
    Context - For EFI compatiblity.  Not used.

Returns:

    None.

--*/
{
  EFI_HANDLE                            Handle;
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_FIRMWARE_VOLUME_PROTOCOL          *Fv;
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
    if (!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystemGuid)) {
      continue;
    }

    //
    // Check if there is an FV protocol already installed in that handle
    //
    Status = CoreHandleProtocol (Handle, &gEfiFirmwareVolumeProtocolGuid, (VOID **)&Fv);
    if (!EFI_ERROR (Status)) {
      //
      // Update Fv to use a new Fvb
      //
      FvDevice = _CR (Fv, FV_DEVICE, Fv);
      if (FvDevice->Signature == FV_DEVICE_SIGNATURE) {
        //
        // Only write into our device structure if it's our device structure
        //
        FvDevice->Fvb = Fvb;
      }

    } else {
      //
      // No FwVol protocol on the handle so create a new one
      //
      FvDevice = CoreAllocateCopyPool (sizeof (FV_DEVICE), &mFvDevice);
      if (FvDevice == NULL) {
        return;
      }
      
      FvDevice->Fvb         = Fvb;
      FvDevice->Handle      = Handle;
      FvDevice->FwVolHeader = FwVolHeader;
      FvDevice->Fv.ParentHandle = Fvb->ParentHandle;
      
      //
      // Install an New FV protocol on the existing handle
      //
      Status = CoreInstallProtocolInterface (
                  &Handle,
                  &gEfiFirmwareVolumeProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FvDevice->Fv
                  );
      ASSERT_EFI_ERROR (Status);
    }
  }
  
  return;
}


EFI_STATUS
EFIAPI
FwVolDriverInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description:
    This routine is the driver initialization entry point.  It initializes the
    libraries, and registers two notification functions.  These notification
    functions are responsible for building the FV stack dynamically.
    
Arguments:
    ImageHandle   - The image handle.
    SystemTable   - The system table.
    
Returns:
    EFI_SUCCESS   - Function successfully returned.

--*/
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

