/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolBlock.h

Abstract:

  Firmware Volume Block protocol..  Consumes FV hobs and creates
  appropriate block protocols.

  Also consumes NT_NON_MM_FV envinronment variable and produces appropriate
  block protocols fro them also... (this is TBD)

--*/

#ifndef _FWVOL_BLOCK_H_
#define _FWVOL_BLOCK_H_


#define FVB_DEVICE_SIGNATURE       EFI_SIGNATURE_32('_','F','V','B')

typedef struct {
  UINTN                       Base;
  UINTN                       Length;
} LBA_CACHE;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_DEVICE_PATH;


typedef struct {
  UINTN                                 Signature;
  EFI_HANDLE                            Handle;
  FV_DEVICE_PATH                        DevicePath;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
  UINTN                                 NumBlocks;
  LBA_CACHE                             *LbaCache;
  UINT32                                FvbAttributes;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
} EFI_FW_VOL_BLOCK_DEVICE;

#define FVB_DEVICE_FROM_THIS(a) \
  CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)



EFI_STATUS
EFIAPI
FwVolBlockDriverInit (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
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
;


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
;


EFI_STATUS
EFIAPI
FwVolBlockSetAttributes (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  OUT EFI_FVB_ATTRIBUTES                          *Attributes
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
;


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
;


EFI_STATUS
EFIAPI
FwVolBlockReadBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
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
;

  
EFI_STATUS
EFIAPI
FwVolBlockWriteBlock (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  IN EFI_LBA                              Lba,
  IN UINTN                                Offset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer
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
;

    
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
;


EFI_STATUS
EFIAPI
FwVolBlockGetBlockSize (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_LBA                             Lba,
  OUT UINTN                              *BlockSize,
  OUT UINTN                              *NumberOfBlocks
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
;
EFI_STATUS
FwVolBlockDriverInit (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
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
    Status code

--*/
;

EFI_STATUS
ProduceFVBProtocolOnBuffer (
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN EFI_HANDLE             ParentHandle,
  OUT EFI_HANDLE            *FvProtocolHandle  OPTIONAL
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
    FvProtocolHandle  - Firmware volume block protocol produced.
    
Returns:
    EFI_VOLUME_CORRUPTED    - Volume corrupted.
    EFI_OUT_OF_RESOURCES    - No enough buffer to be allocated.
    EFI_SUCCESS             - Successfully produced a FVB protocol on given buffer.
                     
--*/
;

#endif
