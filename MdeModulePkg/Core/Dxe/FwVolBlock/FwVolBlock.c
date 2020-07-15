/** @file
  Implementations for Firmware Volume Block protocol.

  It consumes FV HOBs and creates read-only Firmare Volume Block protocol
  instances for each of them.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "FwVolBlock.h"

FV_MEMMAP_DEVICE_PATH mFvMemmapDevicePathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
      }
    },
    EfiMemoryMappedIO,
    (EFI_PHYSICAL_ADDRESS) 0,
    (EFI_PHYSICAL_ADDRESS) 0,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

FV_PIWG_DEVICE_PATH mFvPIWGDevicePathTemplate = {
  {
    {
      MEDIA_DEVICE_PATH,
      MEDIA_PIWG_FW_VOL_DP,
      {
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH)),
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH) >> 8)
      }
    },
    { 0 }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

EFI_FW_VOL_BLOCK_DEVICE  mFwVolBlock = {
  FVB_DEVICE_SIGNATURE,
  NULL,
  NULL,
  {
    FwVolBlockGetAttributes,
    (EFI_FVB_SET_ATTRIBUTES)FwVolBlockSetAttributes,
    FwVolBlockGetPhysicalAddress,
    FwVolBlockGetBlockSize,
    FwVolBlockReadBlock,
    (EFI_FVB_WRITE)FwVolBlockWriteBlock,
    (EFI_FVB_ERASE_BLOCKS)FwVolBlockEraseBlock,
    NULL
  },
  0,
  NULL,
  0,
  0,
  0
};



/**
  Retrieves Volume attributes.  No polarity translations are done.

  @param  This                   Calling context
  @param  Attributes             output buffer which contains attributes

  @retval EFI_SUCCESS            The firmware volume attributes were returned.

**/
EFI_STATUS
EFIAPI
FwVolBlockGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                *Attributes
  )
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  //
  // Since we are read only, it's safe to get attributes data from our in-memory copy.
  //
  *Attributes = FvbDevice->FvbAttributes & ~EFI_FVB2_WRITE_STATUS;

  return EFI_SUCCESS;
}



/**
  Modifies the current settings of the firmware volume according to the input parameter.

  @param  This                   Calling context
  @param  Attributes             input buffer which contains attributes

  @retval EFI_SUCCESS            The firmware volume attributes were returned.
  @retval EFI_INVALID_PARAMETER  The attributes requested are in conflict with
                                 the capabilities as declared in the firmware
                                 volume header.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
FwVolBlockSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN CONST  EFI_FVB_ATTRIBUTES_2                *Attributes
  )
{
  return EFI_UNSUPPORTED;
}



/**
  The EraseBlock() function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param  This                   Calling context
  @param  ...                    Starting LBA followed by Number of Lba to erase.
                                 a -1 to terminate the list.

  @retval EFI_SUCCESS            The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled
                                 state.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly
                                 and could not be written. The firmware device
                                 may have been partially erased.
  @retval EFI_INVALID_PARAMETER  One or more of the LBAs listed in the variable
                                 argument list do
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
FwVolBlockEraseBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  )
{
  return EFI_UNSUPPORTED;
}



/**
  Read the specified number of bytes from the block to the input buffer.

  @param  This                   Indicates the calling context.
  @param  Lba                    The starting logical block index to read.
  @param  Offset                 Offset into the block at which to begin reading.
  @param  NumBytes               Pointer to a UINT32. At entry, *NumBytes
                                 contains the total size of the buffer. At exit,
                                 *NumBytes contains the total number of bytes
                                 actually read.
  @param  Buffer                 Pinter to a caller-allocated buffer that
                                 contains the destine for the read.

  @retval EFI_SUCCESS            The firmware volume was read successfully.
  @retval EFI_BAD_BUFFER_SIZE    The read was attempted across an LBA boundary.
  @retval EFI_ACCESS_DENIED      Access denied.
  @retval EFI_DEVICE_ERROR       The block device is malfunctioning and could not
                                 be read.

**/
EFI_STATUS
EFIAPI
FwVolBlockReadBlock (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN CONST  EFI_LBA                              Lba,
  IN CONST  UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  UINT8                                 *LbaOffset;
  UINTN                                 LbaStart;
  UINTN                                 NumOfBytesRead;
  UINTN                                 LbaIndex;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  //
  // Check if This FW can be read
  //
  if ((FvbDevice->FvbAttributes & EFI_FVB2_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  LbaIndex = (UINTN) Lba;
  if (LbaIndex >= FvbDevice->NumBlocks) {
    //
    // Invalid Lba, read nothing.
    //
    *NumBytes = 0;
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Offset > FvbDevice->LbaCache[LbaIndex].Length) {
    //
    // all exceed boundary, read nothing.
    //
    *NumBytes = 0;
    return EFI_BAD_BUFFER_SIZE;
  }

  NumOfBytesRead = *NumBytes;
  if (Offset + NumOfBytesRead > FvbDevice->LbaCache[LbaIndex].Length) {
    //
    // partial exceed boundary, read data from current postion to end.
    //
    NumOfBytesRead = FvbDevice->LbaCache[LbaIndex].Length - Offset;
  }

  LbaStart = FvbDevice->LbaCache[LbaIndex].Base;
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN) FvbDevice->BaseAddress);
  LbaOffset = (UINT8 *) FwVolHeader + LbaStart + Offset;

  //
  // Perform read operation
  //
  CopyMem (Buffer, LbaOffset, NumOfBytesRead);

  if (NumOfBytesRead == *NumBytes) {
    return EFI_SUCCESS;
  }

  *NumBytes = NumOfBytesRead;
  return EFI_BAD_BUFFER_SIZE;
}



/**
  Writes the specified number of bytes from the input buffer to the block.

  @param  This                   Indicates the calling context.
  @param  Lba                    The starting logical block index to write to.
  @param  Offset                 Offset into the block at which to begin writing.
  @param  NumBytes               Pointer to a UINT32. At entry, *NumBytes
                                 contains the total size of the buffer. At exit,
                                 *NumBytes contains the total number of bytes
                                 actually written.
  @param  Buffer                 Pinter to a caller-allocated buffer that
                                 contains the source for the write.

  @retval EFI_SUCCESS            The firmware volume was written successfully.
  @retval EFI_BAD_BUFFER_SIZE    The write was attempted across an LBA boundary.
                                 On output, NumBytes contains the total number of
                                 bytes actually written.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled
                                 state.
  @retval EFI_DEVICE_ERROR       The block device is malfunctioning and could not
                                 be written.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
FwVolBlockWriteBlock (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN     EFI_LBA                              Lba,
  IN     UINTN                                Offset,
  IN OUT UINTN                                *NumBytes,
  IN     UINT8                                *Buffer
  )
{
  return EFI_UNSUPPORTED;
}



/**
  Get Fvb's base address.

  @param  This                   Indicates the calling context.
  @param  Address                Fvb device base address.

  @retval EFI_SUCCESS            Successfully got Fvb's base address.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
FwVolBlockGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                *Address
  )
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  if ((FvbDevice->FvbAttributes & EFI_FVB2_MEMORY_MAPPED) != 0) {
    *Address = FvbDevice->BaseAddress;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}



/**
  Retrieves the size in bytes of a specific block within a firmware volume.

  @param  This                   Indicates the calling context.
  @param  Lba                    Indicates the block for which to return the
                                 size.
  @param  BlockSize              Pointer to a caller-allocated UINTN in which the
                                 size of the block is returned.
  @param  NumberOfBlocks         Pointer to a caller-allocated UINTN in which the
                                 number of consecutive blocks starting with Lba
                                 is returned. All blocks in this range have a
                                 size of BlockSize.

  @retval EFI_SUCCESS            The firmware volume base address is returned.
  @retval EFI_INVALID_PARAMETER  The requested LBA is out of range.

**/
EFI_STATUS
EFIAPI
FwVolBlockGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN CONST  EFI_LBA                             Lba,
  IN OUT    UINTN                               *BlockSize,
  IN OUT    UINTN                               *NumberOfBlocks
  )
{
  UINTN                                 TotalBlocks;
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  EFI_FV_BLOCK_MAP_ENTRY                *PtrBlockMapEntry;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  //
  // Do parameter checking
  //
  if (Lba >= FvbDevice->NumBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)FvbDevice->BaseAddress);

  PtrBlockMapEntry = FwVolHeader->BlockMap;

  //
  // Search the block map for the given block
  //
  TotalBlocks = 0;
  while ((PtrBlockMapEntry->NumBlocks != 0) || (PtrBlockMapEntry->Length !=0 )) {
    TotalBlocks += PtrBlockMapEntry->NumBlocks;
    if (Lba < TotalBlocks) {
      //
      // We find the range
      //
      break;
    }

    PtrBlockMapEntry++;
  }

  *BlockSize = PtrBlockMapEntry->Length;
  *NumberOfBlocks = TotalBlocks - (UINTN)Lba;

  return EFI_SUCCESS;
}

/**

  Get FVB authentication status

  @param FvbProtocol    FVB protocol.

  @return Authentication status.

**/
UINT32
GetFvbAuthenticationStatus (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *FvbProtocol
  )
{
  EFI_FW_VOL_BLOCK_DEVICE   *FvbDevice;
  UINT32                    AuthenticationStatus;

  AuthenticationStatus = 0;
  FvbDevice = BASE_CR (FvbProtocol, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance);
  if (FvbDevice->Signature == FVB_DEVICE_SIGNATURE) {
    AuthenticationStatus = FvbDevice->AuthenticationStatus;
  }

  return AuthenticationStatus;
}

/**
  This routine produces a firmware volume block protocol on a given
  buffer.

  @param  BaseAddress            base address of the firmware volume image
  @param  Length                 length of the firmware volume image
  @param  ParentHandle           handle of parent firmware volume, if this image
                                 came from an FV image file and section in another firmware
                                 volume (ala capsules)
  @param  AuthenticationStatus   Authentication status inherited, if this image
                                 came from an FV image file and section in another firmware volume.
  @param  FvProtocol             Firmware volume block protocol produced.

  @retval EFI_VOLUME_CORRUPTED   Volume corrupted.
  @retval EFI_OUT_OF_RESOURCES   No enough buffer to be allocated.
  @retval EFI_SUCCESS            Successfully produced a FVB protocol on given
                                 buffer.

**/
EFI_STATUS
ProduceFVBProtocolOnBuffer (
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN EFI_HANDLE             ParentHandle,
  IN UINT32                 AuthenticationStatus,
  OUT EFI_HANDLE            *FvProtocol  OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_FW_VOL_BLOCK_DEVICE       *FvbDev;
  EFI_FIRMWARE_VOLUME_HEADER    *FwVolHeader;
  UINTN                         BlockIndex;
  UINTN                         BlockIndex2;
  UINTN                         LinearOffset;
  UINT32                        FvAlignment;
  EFI_FV_BLOCK_MAP_ENTRY        *PtrBlockMapEntry;

  FvAlignment = 0;
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN) BaseAddress;
  //
  // Validate FV Header, if not as expected, return
  //
  if (FwVolHeader->Signature != EFI_FVH_SIGNATURE) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // If EFI_FVB2_WEAK_ALIGNMENT is set in the volume header then the first byte of the volume
  // can be aligned on any power-of-two boundary. A weakly aligned volume can not be moved from
  // its initial linked location and maintain its alignment.
  //
  if ((FwVolHeader->Attributes & EFI_FVB2_WEAK_ALIGNMENT) != EFI_FVB2_WEAK_ALIGNMENT) {
    //
    // Get FvHeader alignment
    //
    FvAlignment = 1 << ((FwVolHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
    //
    // FvAlignment must be greater than or equal to 8 bytes of the minimum FFS alignment value.
    //
    if (FvAlignment < 8) {
      FvAlignment = 8;
    }
    if ((UINTN)BaseAddress % FvAlignment != 0) {
      //
      // FvImage buffer is not at its required alignment.
      //
      DEBUG ((
        DEBUG_ERROR,
        "Unaligned FvImage found at 0x%lx:0x%lx, the required alignment is 0x%x\n",
        BaseAddress,
        Length,
        FvAlignment
        ));
      return EFI_VOLUME_CORRUPTED;
    }
  }

  //
  // Allocate EFI_FW_VOL_BLOCK_DEVICE
  //
  FvbDev = AllocateCopyPool (sizeof (EFI_FW_VOL_BLOCK_DEVICE), &mFwVolBlock);
  if (FvbDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FvbDev->BaseAddress   = BaseAddress;
  FvbDev->FvbAttributes = FwVolHeader->Attributes;
  FvbDev->FwVolBlockInstance.ParentHandle = ParentHandle;
  FvbDev->AuthenticationStatus = AuthenticationStatus;

  //
  // Init the block caching fields of the device
  // First, count the number of blocks
  //
  FvbDev->NumBlocks = 0;
  for (PtrBlockMapEntry = FwVolHeader->BlockMap;
       PtrBlockMapEntry->NumBlocks != 0;
       PtrBlockMapEntry++) {
    FvbDev->NumBlocks += PtrBlockMapEntry->NumBlocks;
  }

  //
  // Second, allocate the cache
  //
  if (FvbDev->NumBlocks >= (MAX_ADDRESS / sizeof (LBA_CACHE))) {
    CoreFreePool (FvbDev);
    return EFI_OUT_OF_RESOURCES;
  }
  FvbDev->LbaCache = AllocatePool (FvbDev->NumBlocks * sizeof (LBA_CACHE));
  if (FvbDev->LbaCache == NULL) {
    CoreFreePool (FvbDev);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Last, fill in the cache with the linear address of the blocks
  //
  BlockIndex = 0;
  LinearOffset = 0;
  for (PtrBlockMapEntry = FwVolHeader->BlockMap;
       PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      FvbDev->LbaCache[BlockIndex].Base = LinearOffset;
      FvbDev->LbaCache[BlockIndex].Length = PtrBlockMapEntry->Length;
      LinearOffset += PtrBlockMapEntry->Length;
      BlockIndex++;
    }
  }

  //
  // Judget whether FV name guid is produced in Fv extension header
  //
  if (FwVolHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
    //
    FvbDev->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    if (FvbDev->DevicePath == NULL) {
      FreePool (FvbDev);
      return EFI_OUT_OF_RESOURCES;
    }
    ((FV_MEMMAP_DEVICE_PATH *) FvbDev->DevicePath)->MemMapDevPath.StartingAddress = BaseAddress;
    ((FV_MEMMAP_DEVICE_PATH *) FvbDev->DevicePath)->MemMapDevPath.EndingAddress   = BaseAddress + FwVolHeader->FvLength - 1;
  } else {
    //
    // FV contains extension header, then produce MEDIA_FW_VOL_DEVICE_PATH
    //
    FvbDev->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    if (FvbDev->DevicePath == NULL) {
      FreePool (FvbDev);
      return EFI_OUT_OF_RESOURCES;
    }
    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbDev->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)(BaseAddress + FwVolHeader->ExtHeaderOffset)
      );
  }

  //
  //
  // Attach FvVolBlock Protocol to new handle
  //
  Status = CoreInstallMultipleProtocolInterfaces (
             &FvbDev->Handle,
             &gEfiFirmwareVolumeBlockProtocolGuid,     &FvbDev->FwVolBlockInstance,
             &gEfiDevicePathProtocolGuid,              FvbDev->DevicePath,
             NULL
             );

  //
  // If they want the handle back, set it.
  //
  if (FvProtocol != NULL) {
    *FvProtocol = FvbDev->Handle;
  }

  return Status;
}



/**
  This routine consumes FV hobs and produces instances of FW_VOL_BLOCK_PROTOCOL as appropriate.

  @param  ImageHandle            The image handle.
  @param  SystemTable            The system table.

  @retval EFI_SUCCESS            Successfully initialized firmware volume block
                                 driver.

**/
EFI_STATUS
EFIAPI
FwVolBlockDriverInit (
  IN EFI_HANDLE                 ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  )
{
  EFI_PEI_HOB_POINTERS          FvHob;
  EFI_PEI_HOB_POINTERS          Fv3Hob;
  UINT32                        AuthenticationStatus;

  //
  // Core Needs Firmware Volumes to function
  //
  FvHob.Raw = GetHobList ();
  while ((FvHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, FvHob.Raw)) != NULL) {
    AuthenticationStatus = 0;
    //
    // Get the authentication status propagated from PEI-phase to DXE.
    //
    Fv3Hob.Raw = GetHobList ();
    while ((Fv3Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV3, Fv3Hob.Raw)) != NULL) {
      if ((Fv3Hob.FirmwareVolume3->BaseAddress == FvHob.FirmwareVolume->BaseAddress) &&
          (Fv3Hob.FirmwareVolume3->Length == FvHob.FirmwareVolume->Length)) {
        AuthenticationStatus = Fv3Hob.FirmwareVolume3->AuthenticationStatus;
        break;
      }
      Fv3Hob.Raw = GET_NEXT_HOB (Fv3Hob);
    }
    //
    // Produce an FVB protocol for it
    //
    ProduceFVBProtocolOnBuffer (FvHob.FirmwareVolume->BaseAddress, FvHob.FirmwareVolume->Length, NULL, AuthenticationStatus, NULL);
    FvHob.Raw = GET_NEXT_HOB (FvHob);
  }

  return EFI_SUCCESS;
}



/**
  This DXE service routine is used to process a firmware volume. In
  particular, it can be called by BDS to process a single firmware
  volume found in a capsule.

  Caution: The caller need validate the input firmware volume to follow
  PI specification.
  DxeCore will trust the input data and process firmware volume directly.

  @param  FvHeader               pointer to a firmware volume header
  @param  Size                   the size of the buffer pointed to by FvHeader
  @param  FVProtocolHandle       the handle on which a firmware volume protocol
                                 was produced for the firmware volume passed in.

  @retval EFI_OUT_OF_RESOURCES   if an FVB could not be produced due to lack of
                                 system resources
  @retval EFI_VOLUME_CORRUPTED   if the volume was corrupted
  @retval EFI_SUCCESS            a firmware volume protocol was produced for the
                                 firmware volume

**/
EFI_STATUS
EFIAPI
CoreProcessFirmwareVolume (
  IN VOID                             *FvHeader,
  IN UINTN                            Size,
  OUT EFI_HANDLE                      *FVProtocolHandle
  )
{
  VOID        *Ptr;
  EFI_STATUS  Status;

  *FVProtocolHandle = NULL;
  Status = ProduceFVBProtocolOnBuffer (
            (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader,
            (UINT64)Size,
            NULL,
            0,
            FVProtocolHandle
            );
  //
  // Since in our implementation we use register-protocol-notify to put a
  // FV protocol on the FVB protocol handle, we can't directly verify that
  // the FV protocol was produced. Therefore here we will check the handle
  // and make sure an FV protocol is on it. This indicates that all went
  // well. Otherwise we have to assume that the volume was corrupted
  // somehow.
  //
  if (!EFI_ERROR(Status)) {
    ASSERT (*FVProtocolHandle != NULL);
    Ptr = NULL;
    Status = CoreHandleProtocol (*FVProtocolHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **) &Ptr);
    if (EFI_ERROR(Status) || (Ptr == NULL)) {
      return EFI_VOLUME_CORRUPTED;
    }
    return EFI_SUCCESS;
  }
  return Status;
}



