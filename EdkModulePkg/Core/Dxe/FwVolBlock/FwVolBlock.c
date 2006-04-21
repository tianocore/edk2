/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolBlock.c

Abstract:

  Firmware Volume Block protocol..  Consumes FV hobs and creates
  appropriate block protocols.

  Also consumes NT_NON_MM_FV envinronment variable and produces appropriate
  block protocols fro them also... (this is TBD)

--*/

#include <DxeMain.h>


EFI_FW_VOL_BLOCK_DEVICE  mFwVolBlock = {
  FVB_DEVICE_SIGNATURE,
  NULL,
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_MEMMAP_DP,
        { (UINT8)(sizeof (MEMMAP_DEVICE_PATH)), (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8) }
      },
      EfiMemoryMappedIO,
      (EFI_PHYSICAL_ADDRESS)0,
      (EFI_PHYSICAL_ADDRESS)0,
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { END_DEVICE_PATH_LENGTH, 0 }      
    },
  },
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
  0
};




EFI_STATUS
EFIAPI
FwVolBlockGetAttributes (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  OUT EFI_FVB_ATTRIBUTES                          *Attributes
  )
/*++

Routine Description:
    Retrieves Volume attributes.  No polarity translations are done.

Arguments:
    This - Calling context
    Attributes - output buffer which contains attributes

Returns:
    EFI_SUCCESS - The firmware volume attributes were returned.

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  
  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  //
  // Since we are read only, it's safe to get attributes data from our in-memory copy.
  //
  *Attributes = FvbDevice->FvbAttributes;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
FwVolBlockSetAttributes (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_FVB_ATTRIBUTES                   *Attributes
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the input parameter.

Arguments:
  This - Calling context
  Attributes - input buffer which contains attributes

Returns:
  EFI_SUCCESS -  The firmware volume attributes were returned.
  EFI_INVALID_PARAMETER  -  The attributes requested are in conflict with the capabilities as
                             declared in the firmware volume header.
  EFI_UNSUPPORTED        -  Not supported.
--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
FwVolBlockEraseBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  )
/*++

Routine Description:
  The EraseBlock() function erases one or more blocks as denoted by the 
variable argument list. The entire parameter list of blocks must be verified
prior to erasing any blocks.  If a block is requested that does not exist 
within the associated firmware volume (it has a larger index than the last 
block of the firmware volume), the EraseBlock() function must return
EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

Arguments:
  This - Calling context
  ...  - Starting LBA followed by Number of Lba to erase. a -1 to terminate
           the list.
    
Returns:
  EFI_SUCCESS   -  The erase request was successfully completed.
  EFI_ACCESS_DENIED   -  The firmware volume is in the WriteDisabled state.
  EFI_DEVICE_ERROR    -  The block device is not functioning correctly and could not be
                         written. The firmware device may have been partially erased.
  EFI_INVALID_PARAMETER  -  One or more of the LBAs listed in the variable argument list do
  EFI_UNSUPPORTED        -  Not supported.
    
--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
FwVolBlockReadBlock (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN     EFI_LBA                                Lba,
  IN     UINTN                                  Offset,
  IN OUT UINTN                                  *NumBytes,
  IN     UINT8                                  *Buffer
  )
/*++

Routine Description:
  Read the specified number of bytes from the block to the input buffer.

Arguments:
  This          -  Indicates the calling context.
  Lba           -  The starting logical block index to read.
  Offset        -  Offset into the block at which to begin reading.
  NumBytes      -  Pointer to a UINT32. At entry, *NumBytes contains the
                   total size of the buffer. At exit, *NumBytes contains the
                   total number of bytes actually read.
  Buffer        -  Pinter to a caller-allocated buffer that contains the destine
                   for the read.    

Returns:      
  EFI_SUCCESS  -  The firmware volume was read successfully.
  EFI_BAD_BUFFER_SIZE -  The read was attempted across an LBA boundary.
  EFI_ACCESS_DENIED  -  Access denied.
  EFI_DEVICE_ERROR   -  The block device is malfunctioning and could not be read.
--*/
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
  if ((FvbDevice->FvbAttributes & EFI_FVB_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  
  LbaIndex = (UINTN)Lba;
  if (LbaIndex >= FvbDevice->NumBlocks) {
    //
    // Invalid Lba, read nothing.
    //
    *NumBytes = 0;
    return EFI_BAD_BUFFER_SIZE;
  }
  
  if (Offset > FvbDevice->LbaCache[LbaIndex].Length) {
    //
    // all exceed boundry, read nothing.
    //
    *NumBytes = 0;
    return EFI_BAD_BUFFER_SIZE;
  }
  
  NumOfBytesRead = *NumBytes;
  if (Offset + NumOfBytesRead > FvbDevice->LbaCache[LbaIndex].Length) {
    //
    // partial exceed boundry, read data from current postion to end.
    //
    NumOfBytesRead = FvbDevice->LbaCache[LbaIndex].Length - Offset;
  }
  
  LbaStart = FvbDevice->LbaCache[LbaIndex].Base;
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)FvbDevice->BaseAddress);
  LbaOffset = (UINT8 *)FwVolHeader + LbaStart + Offset;

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
  

EFI_STATUS
EFIAPI
FwVolBlockWriteBlock (
  IN     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN     EFI_LBA                              Lba,
  IN     UINTN                                Offset,
  IN OUT UINTN                                *NumBytes,
  IN     UINT8                                *Buffer
  )
/*++

Routine Description:
  Writes the specified number of bytes from the input buffer to the block.

Arguments:
  This          -  Indicates the calling context.
  Lba           -  The starting logical block index to write to.
  Offset        -  Offset into the block at which to begin writing.
  NumBytes      -  Pointer to a UINT32. At entry, *NumBytes contains the
                   total size of the buffer. At exit, *NumBytes contains the
                   total number of bytes actually written.
  Buffer        -  Pinter to a caller-allocated buffer that contains the source
                   for the write.    

Returns:     
  EFI_SUCCESS  -  The firmware volume was written successfully.
  EFI_BAD_BUFFER_SIZE -  The write was attempted across an LBA boundary. On output,
                         NumBytes contains the total number of bytes actually written.
  EFI_ACCESS_DENIED  -  The firmware volume is in the WriteDisabled state.
  EFI_DEVICE_ERROR   -  The block device is malfunctioning and could not be written.
  EFI_UNSUPPORTED    -  Not supported.
--*/
{
  return EFI_UNSUPPORTED;
}
 

EFI_STATUS
EFIAPI
FwVolBlockGetPhysicalAddress (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
  )
/*++

Routine Description:
  Get Fvb's base address.

Arguments:
  This          -  Indicates the calling context.
  Address       -  Fvb device base address.

Returns:     
  EFI_SUCCESS  -  Successfully got Fvb's base address.
  EFI_UNSUPPORTED -  Not supported.
--*/
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  
  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  
  if (FvbDevice->FvbAttributes & EFI_FVB_MEMORY_MAPPED) {
    *Address = FvbDevice->BaseAddress;
    return EFI_SUCCESS;
  }
  
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
FwVolBlockGetBlockSize (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_LBA                              Lba,
  OUT UINTN                               *BlockSize,
  OUT UINTN                               *NumberOfBlocks
  )
/*++

Routine Description:
  Retrieves the size in bytes of a specific block within a firmware volume.

Arguments:
  This            -  Indicates the calling context.
  Lba             -  Indicates the block for which to return the size.
  BlockSize       -  Pointer to a caller-allocated UINTN in which the size of the
                     block is returned.
  NumberOfBlocks  -  Pointer to a caller-allocated UINTN in which the number of
                     consecutive blocks starting with Lba is returned. All blocks
                     in this range have a size of BlockSize.   
Returns:
  EFI_SUCCESS  -  The firmware volume base address is returned.
  EFI_INVALID_PARAMETER  -  The requested LBA is out of range.
--*/
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
  
  PtrBlockMapEntry = FwVolHeader->FvBlockMap;
  
  //
  // Search the block map for the given block
  //
  TotalBlocks = 0;
  while ((PtrBlockMapEntry->NumBlocks != 0) || (PtrBlockMapEntry->BlockLength !=0 )) {
    TotalBlocks += PtrBlockMapEntry->NumBlocks;
    if (Lba < TotalBlocks) {
      //
      // We find the range
      //
      break;
    }
    
    PtrBlockMapEntry++;
  }
  
  *BlockSize = PtrBlockMapEntry->BlockLength;
  *NumberOfBlocks = TotalBlocks - (UINTN)Lba;
  
  return EFI_SUCCESS;
}


EFI_STATUS
ProduceFVBProtocolOnBuffer (
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN EFI_HANDLE             ParentHandle,
  OUT EFI_HANDLE            *FvProtocol  OPTIONAL
  )
/*++

Routine Description:
    This routine produces a firmware volume block protocol on a given
    buffer. 

Arguments:
    BaseAddress     - base address of the firmware volume image
    Length          - length of the firmware volume image
    ParentHandle    - handle of parent firmware volume, if this
                      image came from an FV image file in another
                      firmware volume (ala capsules)
    FvProtocol      - Firmware volume block protocol produced.
    
Returns:
    EFI_VOLUME_CORRUPTED    - Volume corrupted.
    EFI_OUT_OF_RESOURCES    - No enough buffer to be allocated.
    EFI_SUCCESS             - Successfully produced a FVB protocol on given buffer.
                     
--*/
{
  EFI_STATUS                    Status;
  EFI_FW_VOL_BLOCK_DEVICE       *FvbDev;
  EFI_FIRMWARE_VOLUME_HEADER    *FwVolHeader;
  UINTN                         BlockIndex;
  UINTN                         BlockIndex2;
  UINTN                         LinearOffset;
  EFI_FV_BLOCK_MAP_ENTRY        *PtrBlockMapEntry;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BaseAddress;
  //
  // Validate FV Header, if not as expected, return
  //
  if (FwVolHeader->Signature != EFI_FVH_SIGNATURE) {
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Allocate EFI_FW_VOL_BLOCK_DEVICE 
  //
  FvbDev = CoreAllocateCopyPool (sizeof (EFI_FW_VOL_BLOCK_DEVICE), &mFwVolBlock);
  if (FvbDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FvbDev->BaseAddress   = BaseAddress;
  FvbDev->FvbAttributes = FwVolHeader->Attributes;
  FvbDev->FwVolBlockInstance.ParentHandle = ParentHandle;

  //
  // Init the block caching fields of the device
  // First, count the number of blocks
  //
  FvbDev->NumBlocks = 0;
  for (PtrBlockMapEntry = FwVolHeader->FvBlockMap;
        PtrBlockMapEntry->NumBlocks != 0;
        PtrBlockMapEntry++) {
    FvbDev->NumBlocks += PtrBlockMapEntry->NumBlocks;
  }
  //
  // Second, allocate the cache
  //
  FvbDev->LbaCache = CoreAllocateBootServicesPool (FvbDev->NumBlocks * sizeof (LBA_CACHE));
  if (FvbDev->LbaCache == NULL) {
    CoreFreePool (FvbDev);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Last, fill in the cache with the linear address of the blocks
  //
  BlockIndex = 0;
  LinearOffset = 0;
  for (PtrBlockMapEntry = FwVolHeader->FvBlockMap;
        PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      FvbDev->LbaCache[BlockIndex].Base = LinearOffset;
      FvbDev->LbaCache[BlockIndex].Length = PtrBlockMapEntry->BlockLength;
      LinearOffset += PtrBlockMapEntry->BlockLength;
      BlockIndex++;
    }
  }

  //
  // Set up the devicepath
  //
  FvbDev->DevicePath.MemMapDevPath.StartingAddress = BaseAddress;
  FvbDev->DevicePath.MemMapDevPath.EndingAddress = BaseAddress + FwVolHeader->FvLength - 1;

  //
  //
  // Attach FvVolBlock Protocol to new handle
  //
  Status = CoreInstallMultipleProtocolInterfaces (
            &FvbDev->Handle,
            &gEfiFirmwareVolumeBlockProtocolGuid,     &FvbDev->FwVolBlockInstance,
            &gEfiDevicePathProtocolGuid,              &FvbDev->DevicePath,
            &gEfiFirmwareVolumeDispatchProtocolGuid,  NULL,
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


EFI_STATUS
EFIAPI
FwVolBlockDriverInit (
  IN EFI_HANDLE                 ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  )
/*++

Routine Description:
    This routine is the driver initialization entry point.  It initializes the
    libraries, consumes FV hobs and NT_NON_MM_FV environment variable and
    produces instances of FW_VOL_BLOCK_PROTOCOL as appropriate.
Arguments:
    ImageHandle   - The image handle.
    SystemTable   - The system table.
Returns:
    EFI_SUCCESS   - Successfully initialized firmware volume block driver.
--*/
{
  EFI_PEI_HOB_POINTERS          FvHob;
  //
  // Core Needs Firmware Volumes to function
  //
  FvHob.Raw = GetHobList ();
  while ((FvHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, FvHob.Raw)) != NULL) {
    //
    // Produce an FVB protocol for it
    //
    ProduceFVBProtocolOnBuffer (FvHob.FirmwareVolume->BaseAddress, FvHob.FirmwareVolume->Length, NULL, NULL);    
    FvHob.Raw = GET_NEXT_HOB (FvHob);
  }
  return EFI_SUCCESS;
}


EFI_STATUS
CoreProcessFirmwareVolume (
  IN VOID                             *FvHeader,
  IN UINTN                            Size, 
  OUT EFI_HANDLE                      *FVProtocolHandle
  )
/*++

Routine Description:
    This DXE service routine is used to process a firmware volume. In
    particular, it can be called by BDS to process a single firmware
    volume found in a capsule. 

Arguments:
    FvHeader              - pointer to a firmware volume header
    Size                  - the size of the buffer pointed to by FvHeader
    FVProtocolHandle      - the handle on which a firmware volume protocol
                            was produced for the firmware volume passed in.

Returns:
    EFI_OUT_OF_RESOURCES  - if an FVB could not be produced due to lack of 
                            system resources
    EFI_VOLUME_CORRUPTED  - if the volume was corrupted
    EFI_SUCCESS           - a firmware volume protocol was produced for the
                            firmware volume

--*/
{
  VOID        *Ptr;
  EFI_STATUS  Status;

  *FVProtocolHandle = NULL;
  Status = ProduceFVBProtocolOnBuffer ( 
            (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader, 
            (UINT64)Size, 
            NULL, 
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
    Ptr = NULL;
    Status = CoreHandleProtocol (*FVProtocolHandle, &gEfiFirmwareVolumeProtocolGuid, (VOID **)&Ptr);
    if (EFI_ERROR(Status) || (Ptr == NULL)) {
      return EFI_VOLUME_CORRUPTED;
    }
    return EFI_SUCCESS;
  }
  return Status;
}


