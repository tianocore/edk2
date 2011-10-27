/** @file

  Firmware File System driver that produce full Firmware Volume2 protocol.
  Layers on top of Firmware Block protocol to produce a file abstraction
  of FV based files.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FwVolDriver.h"

#define KEYSIZE sizeof (UINTN)

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
  @retval EFI_ACCESS_DENIED     Read status of FV is not enabled.
**/
EFI_STATUS
GetFwVolHeader (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *Fvb,
  OUT EFI_FIRMWARE_VOLUME_HEADER                **FwVolHeader
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  TempFvh;
  EFI_FVB_ATTRIBUTES_2        FvbAttributes;
  UINTN                       FvhLength;
  EFI_PHYSICAL_ADDRESS        BaseAddress;

  //
  // Determine the real length of FV header
  //
  Status = Fvb->GetAttributes (
                  Fvb,
                  &FvbAttributes
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((FvbAttributes & EFI_FVB2_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Just avoid compiling warning
  //
  BaseAddress = 0;
  FvhLength   = sizeof (EFI_FIRMWARE_VOLUME_HEADER);

  //
  // memory-mapped FV and non memory-mapped has different ways to read
  //
  if ((FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {
    Status = Fvb->GetPhysicalAddress (
                    Fvb,
                    &BaseAddress
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (&TempFvh, (VOID *) (UINTN) BaseAddress, FvhLength);
  } else {
    Status = Fvb->Read (
                    Fvb,
                    0,
                    0,
                    &FvhLength,
                    (UINT8 *) &TempFvh
                    );
  }

  *FwVolHeader = AllocatePool (TempFvh.HeaderLength);
  if (*FwVolHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Read the whole header
  //
  if ((FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {
    CopyMem (*FwVolHeader, (VOID *) (UINTN) BaseAddress, TempFvh.HeaderLength);
  } else {
    //
    // Assumed the first block is bigger than the length of Fv headder
    //
    FvhLength = TempFvh.HeaderLength;
    Status = Fvb->Read (
                    Fvb,
                    0,
                    0,
                    &FvhLength,
                    (UINT8 *) *FwVolHeader
                    );
    //
    // Check whether Read successes.
    //
    if (EFI_ERROR (Status)) {
      FreePool (*FwVolHeader);
      *FwVolHeader = NULL;
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Free FvDevice resource when error happens.

  @param FvDevice   Pointer to the FvDevice to be freed.
**/
VOID
FreeFvDeviceResource (
  IN FV_DEVICE  *FvDevice
  )
{
  LBA_ENTRY           *LbaEntry;
  FREE_SPACE_ENTRY    *FreeSpaceEntry;
  FFS_FILE_LIST_ENTRY *FfsFileEntry;
  LIST_ENTRY      *NextEntry;

  //
  // Free LAB Entry
  //
  LbaEntry = (LBA_ENTRY *) FvDevice->LbaHeader.ForwardLink;
  while (&LbaEntry->Link != &FvDevice->LbaHeader) {
    NextEntry = (&LbaEntry->Link)->ForwardLink;
    FreePool (LbaEntry);
    LbaEntry = (LBA_ENTRY *) NextEntry;
  }
  //
  // Free File List Entry
  //
  FfsFileEntry = (FFS_FILE_LIST_ENTRY *) FvDevice->FfsFileListHeader.ForwardLink;
  while (&FfsFileEntry->Link != &FvDevice->FfsFileListHeader) {
    NextEntry = (&FfsFileEntry->Link)->ForwardLink;
    FreePool (FfsFileEntry);
    FfsFileEntry = (FFS_FILE_LIST_ENTRY *) NextEntry;
  }
  //
  // Free Space Entry
  //
  FreeSpaceEntry = (FREE_SPACE_ENTRY *) FvDevice->FreeSpaceHeader.ForwardLink;
  while (&FreeSpaceEntry->Link != &FvDevice->FreeSpaceHeader) {
    NextEntry = (&FreeSpaceEntry->Link)->ForwardLink;
    FreePool (FreeSpaceEntry);
    FreeSpaceEntry = (FREE_SPACE_ENTRY *) NextEntry;
  }
  //
  // Free the cache
  //
  FreePool ((UINT8 *) (UINTN) FvDevice->CachedFv);

  return ;
}

/**
  Check if an FV is consistent and allocate cache for it.

  @param  FvDevice              A pointer to the FvDevice to be checked.

  @retval EFI_OUT_OF_RESOURCES  No enough buffer could be allocated.
  @retval EFI_VOLUME_CORRUPTED  File system is corrupted.
  @retval EFI_SUCCESS           FV is consistent and cache is allocated.

**/
EFI_STATUS
FvCheck (
  IN FV_DEVICE  *FvDevice
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;
  EFI_FV_BLOCK_MAP_ENTRY              *BlockMap;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  UINT8                               *FwCache;
  LBA_ENTRY                           *LbaEntry;
  FREE_SPACE_ENTRY                    *FreeSpaceEntry;
  FFS_FILE_LIST_ENTRY                 *FfsFileEntry;
  UINT8                               *LbaStart;
  UINTN                               Index;
  EFI_LBA                             LbaIndex;
  UINT8                               *Ptr;
  UINTN                               Size;
  UINT8                               *FreeStart;
  UINTN                               FreeSize;
  UINT8                               ErasePolarity;
  EFI_FFS_FILE_STATE                  FileState;
  UINT8                               *TopFvAddress;
  UINTN                               TestLength;
  EFI_PHYSICAL_ADDRESS                BaseAddress;

  Fvb     = FvDevice->Fvb;

  Status  = Fvb->GetAttributes (Fvb, &FvbAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  InitializeListHead (&FvDevice->LbaHeader);
  InitializeListHead (&FvDevice->FreeSpaceHeader);
  InitializeListHead (&FvDevice->FfsFileListHeader);

  FwVolHeader = NULL;
  Status = GetFwVolHeader (Fvb, &FwVolHeader);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (FwVolHeader != NULL);

  FvDevice->IsFfs3Fv = CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid);

  //
  // Double Check firmware volume header here
  //
  if (!VerifyFvHeaderChecksum (FwVolHeader)) {
    FreePool (FwVolHeader);
    return EFI_VOLUME_CORRUPTED;
  }

  BlockMap = FwVolHeader->BlockMap;

  //
  // FwVolHeader->FvLength is the whole FV length including FV header
  //
  FwCache = AllocateZeroPool ((UINTN) FwVolHeader->FvLength);
  if (FwCache == NULL) {
    FreePool (FwVolHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  FvDevice->CachedFv = (EFI_PHYSICAL_ADDRESS) (UINTN) FwCache;

  //
  // Copy to memory
  //
  LbaStart  = FwCache;
  LbaIndex  = 0;
  Ptr       = NULL;

  if ((FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {
    //
    // Get volume base address
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &BaseAddress);
    if (EFI_ERROR (Status)) {
      FreePool (FwVolHeader);
      return Status;
    }

    Ptr = (UINT8 *) ((UINTN) BaseAddress);

    DEBUG((EFI_D_INFO, "Fv Base Address is 0x%LX\n", BaseAddress));
  }
  //
  // Copy whole FV into the memory
  //
  while ((BlockMap->NumBlocks != 0) || (BlockMap->Length != 0)) {

    for (Index = 0; Index < BlockMap->NumBlocks; Index++) {
      LbaEntry = AllocatePool (sizeof (LBA_ENTRY));
      if (LbaEntry == NULL) {
        FreePool (FwVolHeader);
        FreeFvDeviceResource (FvDevice);
        return EFI_OUT_OF_RESOURCES;
      }

      LbaEntry->LbaIndex        = LbaIndex;
      LbaEntry->StartingAddress = LbaStart;
      LbaEntry->BlockLength     = BlockMap->Length;

      //
      // Copy each LBA into memory
      //
      if ((FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {

        CopyMem (LbaStart, Ptr, BlockMap->Length);
        Ptr += BlockMap->Length;

      } else {

        Size = BlockMap->Length;
        Status = Fvb->Read (
                        Fvb,
                        LbaIndex,
                        0,
                        &Size,
                        LbaStart
                        );
        //
        // Not check EFI_BAD_BUFFER_SIZE, for Size = BlockMap->Length
        //
        if (EFI_ERROR (Status)) {
          FreePool (FwVolHeader);
          FreeFvDeviceResource (FvDevice);
          return Status;
        }

      }

      LbaIndex++;
      LbaStart += BlockMap->Length;

      InsertTailList (&FvDevice->LbaHeader, &LbaEntry->Link);
    }

    BlockMap++;
  }

  FvDevice->FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) FwCache;

  //
  // it is not used any more, so free FwVolHeader
  //
  FreePool (FwVolHeader);

  //
  // Scan to check the free space & File list
  //
  if ((FvbAttributes & EFI_FVB2_ERASE_POLARITY) != 0) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }

  FvDevice->ErasePolarity = ErasePolarity;

  //
  // go through the whole FV cache, check the consistence of the FV
  //
  Ptr           = (UINT8 *) (UINTN) (FvDevice->CachedFv + FvDevice->FwVolHeader->HeaderLength);
  TopFvAddress  = (UINT8 *) (UINTN) (FvDevice->CachedFv + FvDevice->FwVolHeader->FvLength - 1);

  //
  // Build FFS list & Free Space List here
  //
  while (Ptr <= TopFvAddress) {
    TestLength = TopFvAddress - Ptr + 1;

    if (TestLength > sizeof (EFI_FFS_FILE_HEADER)) {
      TestLength = sizeof (EFI_FFS_FILE_HEADER);
    }

    if (IsBufferErased (ErasePolarity, Ptr, TestLength)) {
      //
      // We found free space
      //
      FreeStart = Ptr;
      FreeSize  = 0;

      do {
        TestLength = TopFvAddress - Ptr + 1;

        if (TestLength > sizeof (EFI_FFS_FILE_HEADER)) {
          TestLength = sizeof (EFI_FFS_FILE_HEADER);
        }

        if (!IsBufferErased (ErasePolarity, Ptr, TestLength)) {
          break;
        }

        FreeSize += TestLength;
        Ptr += TestLength;
      } while (Ptr <= TopFvAddress);

      FreeSpaceEntry = AllocateZeroPool (sizeof (FREE_SPACE_ENTRY));
      if (FreeSpaceEntry == NULL) {
        FreeFvDeviceResource (FvDevice);
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Create a Free space entry
      //
      FreeSpaceEntry->StartingAddress = FreeStart;
      FreeSpaceEntry->Length          = FreeSize;
      InsertTailList (&FvDevice->FreeSpaceHeader, &FreeSpaceEntry->Link);
      continue;
    }
    //
    // double check boundry
    //
    if (TestLength < sizeof (EFI_FFS_FILE_HEADER)) {
      break;
    }

    if (!IsValidFFSHeader (
          FvDevice->ErasePolarity,
          (EFI_FFS_FILE_HEADER *) Ptr
          )) {
      FileState = GetFileState (
                    FvDevice->ErasePolarity,
                    (EFI_FFS_FILE_HEADER *) Ptr
                    );
      if ((FileState == EFI_FILE_HEADER_INVALID) || (FileState == EFI_FILE_HEADER_CONSTRUCTION)) {
        if (IS_FFS_FILE2 (Ptr)) {
          if (!FvDevice->IsFfs3Fv) {
            DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &((EFI_FFS_FILE_HEADER *) Ptr)->Name));
          }
          Ptr = Ptr + sizeof (EFI_FFS_FILE_HEADER2);
        } else {
          Ptr = Ptr + sizeof (EFI_FFS_FILE_HEADER);
        }

        continue;

      } else {
        //
        // File system is corrputed, return
        //
        FreeFvDeviceResource (FvDevice);
        return EFI_VOLUME_CORRUPTED;
      }
    }

    if (IS_FFS_FILE2 (Ptr)) {
      ASSERT (FFS_FILE2_SIZE (Ptr) > 0x00FFFFFF);
      if (!FvDevice->IsFfs3Fv) {
        DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &((EFI_FFS_FILE_HEADER *) Ptr)->Name));
        Ptr = Ptr + FFS_FILE2_SIZE (Ptr);
        //
        // Adjust Ptr to the next 8-byte aligned boundry.
        //
        while (((UINTN) Ptr & 0x07) != 0) {
          Ptr++;
        }
        continue;
      }
    }

    if (IsValidFFSFile (FvDevice, (EFI_FFS_FILE_HEADER *) Ptr)) {
      FileState = GetFileState (
                    FvDevice->ErasePolarity,
                    (EFI_FFS_FILE_HEADER *) Ptr
                    );

      //
      // check for non-deleted file
      //
      if (FileState != EFI_FILE_DELETED) {
        //
        // Create a FFS list entry for each non-deleted file
        //
        FfsFileEntry = AllocateZeroPool (sizeof (FFS_FILE_LIST_ENTRY));
        if (FfsFileEntry == NULL) {
          FreeFvDeviceResource (FvDevice);
          return EFI_OUT_OF_RESOURCES;
        }

        FfsFileEntry->FfsHeader = Ptr;
        InsertTailList (&FvDevice->FfsFileListHeader, &FfsFileEntry->Link);
      }

      if (IS_FFS_FILE2 (Ptr)) {
        Ptr = Ptr + FFS_FILE2_SIZE (Ptr);
      } else {
        Ptr = Ptr + FFS_FILE_SIZE (Ptr);
      }

      //
      // Adjust Ptr to the next 8-byte aligned boundry.
      //
      while (((UINTN) Ptr & 0x07) != 0) {
        Ptr++;
      }
    } else {
      //
      // File system is corrupted, return
      //
      FreeFvDeviceResource (FvDevice);
      return EFI_VOLUME_CORRUPTED;
    }
  }

  FvDevice->CurrentFfsFile = NULL;

  return EFI_SUCCESS;
}

/**
  Entry point function does install/reinstall FV2 protocol with full functionality.

  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table

  @retval EFI_SUCCESS    At least one Fv protocol install/reinstall successfully.
  @retval EFI_NOT_FOUND  No FV protocol install/reinstall successfully.
**/
EFI_STATUS
EFIAPI
FwVolDriverInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME2_PROTOCOL       *Fv;
  FV_DEVICE                           *FvDevice;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  BOOLEAN                             Reinstall;
  BOOLEAN                             InstallFlag;

  DEBUG ((EFI_D_INFO, "=========FwVol writable driver installed\n"));
  InstallFlag   =  FALSE;
  //
  // Locate all handles of Fvb protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &Fvb
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = NULL;
    Status = GetFwVolHeader (Fvb, &FwVolHeader);
    if (EFI_ERROR (Status)) {
      continue;
    }
    ASSERT (FwVolHeader != NULL);
    //
    // Check to see that the file system is indeed formatted in a way we can
    // understand it...
    //
    if ((!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid)) &&
        (!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid))) {
      FreePool (FwVolHeader);
      continue;
    }
    FreePool (FwVolHeader);

    Reinstall = FALSE;
    //
    // Check if there is an FV protocol already installed in that handle
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      Reinstall = TRUE;
    }
    //
    // FwVol protocol on the handle so create a new one
    //
    FvDevice = AllocateZeroPool (sizeof (FV_DEVICE));
    if (FvDevice == NULL) {
      goto Done;
    }

    FvDevice->Signature = FV_DEVICE_SIGNATURE;
    FvDevice->Fvb       = Fvb;

    //
    // Firmware Volume Protocol interface
    //
    FvDevice->Fv.GetVolumeAttributes  = FvGetVolumeAttributes;
    FvDevice->Fv.SetVolumeAttributes  = FvSetVolumeAttributes;
    FvDevice->Fv.ReadFile             = FvReadFile;
    FvDevice->Fv.ReadSection          = FvReadFileSection;
    FvDevice->Fv.WriteFile            = FvWriteFile;
    FvDevice->Fv.GetNextFile          = FvGetNextFile;
    FvDevice->Fv.KeySize              = KEYSIZE;
    FvDevice->Fv.GetInfo              = FvGetVolumeInfo;
    FvDevice->Fv.SetInfo              = FvSetVolumeInfo;

    Status = FvCheck (FvDevice);
    if (EFI_ERROR (Status)) {
      //
      // The file system is not consistence
      //
      FreePool (FvDevice);
      continue;
    }

    if (Reinstall) {
      //
      // Reinstall an New FV protocol
      //
      // FvDevice = FV_DEVICE_FROM_THIS (Fv);
      // FvDevice->Fvb = Fvb;
      // FreeFvDeviceResource (FvDevice);
      //
      Status = gBS->ReinstallProtocolInterface (
                      HandleBuffer[Index],
                      &gEfiFirmwareVolume2ProtocolGuid,
                      Fv,
                      &FvDevice->Fv
                      );
      if (!EFI_ERROR (Status)) {
        InstallFlag = TRUE;
      } else {
        FreePool (FvDevice);
      }
      
      DEBUG ((EFI_D_INFO, "Reinstall FV protocol as writable - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
    } else {
      //
      // Install an New FV protocol
      //
      Status = gBS->InstallProtocolInterface (
                      &FvDevice->Handle,
                      &gEfiFirmwareVolume2ProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &FvDevice->Fv
                      );
      if (!EFI_ERROR (Status)) {
        InstallFlag = TRUE;
      } else {
        FreePool (FvDevice);
      }
      
      DEBUG ((EFI_D_INFO, "Install FV protocol as writable - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
    }
  }

Done:
  //
  // As long as one Fv protocol install/reinstall successfully,
  // success should return to ensure this image will be not unloaded.
  // Otherwise, new Fv protocols are corrupted by other loaded driver.
  //
  if (InstallFlag) {
    return EFI_SUCCESS;
  }
  
  //
  // No FV protocol install/reinstall successfully.
  // EFI_NOT_FOUND should return to ensure this image will be unloaded.
  //
  return EFI_NOT_FOUND;
}
