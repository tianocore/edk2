/** @file
  This file declares Firmware Volume Block protocol.

  Low level firmware device access routines to abstract firmware device
  hardware.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  FirmwareVolumeBlock.h

  @par Revision Reference:
  This protocol is defined in Framework of EFI Firmware Volume Block specification.
  Version 0.9

**/

#ifndef __FIRMWARE_VOLUME_BLOCK_H__
#define __FIRMWARE_VOLUME_BLOCK_H__


#define EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
  { \
    0xDE28BC59, 0x6228, 0x41BD, {0xBD, 0xF6, 0xA3, 0xB9, 0xAD, 0xB5, 0x8D, 0xA1 } \
  }

typedef struct _EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL;

/**
  Retrieves Volume attributes.  No polarity translations are done.

  @param  This Calling context
  @param  Attributes output buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_FVB_ATTRIBUTES                          *Attributes
  )
;

/**
  Sets Volume attributes.  No polarity translations are done.

  @param  This Calling context
  @param  Attributes On input: contains new attributes
  On output: contains current attributes of FV

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_SET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN OUT EFI_FVB_ATTRIBUTES                       *Attributes
  )
;

/**
  Retrieves the physical address of a memory mapped FV.

  @param  This Calling context
  @param  Attributes Address is a pointer to a caller allocated EFI_PHYSICAL_ADDRESS
  that on successful return from GetPhysicalAddress() contains the
  base address of the firmware volume.

  @retval EFI_UNSUPPORTED
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_PHYSICAL_ADDRESS) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
  )
;

/**
  Retrieves the size in bytes of a specific block within an FV.

  @param  This Calling context.
  @param  Lba Indicates which block to return the size for.
  @param  BlockSize BlockSize is a pointer to a caller allocated
  UINTN in which the size of the block is returned.
  @param  NumberOfBlocks NumberOfBlocks is a pointer to a caller allocated
  UINTN in which the number of consecutive blocks
  starting with Lba is returned. All blocks in this
  range have a size of BlockSize.

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_BLOCK_SIZE) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumberOfBlocks
  )
;

/**
  Reads data beginning at Lba:Offset from FV and places the data in Buffer.
  The read terminates either when *NumBytes of data have been read, or when
  a block boundary is reached.  *NumBytes is updated to reflect the actual
  number of bytes read.

  @param  This Calling context
  @param  Lba Block in which to begin read
  @param  Offset Offset in the block at which to begin read
  @param  NumBytes At input, indicates the requested read size.  At output, indicates
  the actual number of bytes read.
  @param  Buffer Data buffer in which to place data read.

  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_READ) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  OUT UINT8                                       *Buffer
  )
;

/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written.

  @param  This Calling context
  @param  Lba Block in which to begin write
  @param  Offset Offset in the block at which to begin write
  @param  NumBytes At input, indicates the requested write size.  At output, indicates
  the actual number of bytes written.
  @param  Buffer Buffer containing source data for the write.

  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_WRITE) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
;

#define EFI_LBA_LIST_TERMINATOR 0xFFFFFFFFFFFFFFFFULL

/**
  The EraseBlock() function erases one or more blocks as denoted by the 
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist 
  within the associated firmware volume (it has a larger index than the last 
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param  This Calling context
  @param  ... Starting LBA followed by Number of Lba to erase. a -1 to terminate
  the list.

  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS
  @retval EFI_ACCESS_DENIED

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FVB_ERASE_BLOCKS) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  ...
  )
;

/**
  @par Protocol Description:
  This protocol provides control over block-oriented firmware devices.  
  Typically, the FFS (or an alternate file system) driver consumes the 
  Firmware Volume Block Protocol and produces the Firmware Volume Protocol. 

  @param GetAttributes
  Retrieves the current volume attributes.

  @param SetAttributes
  Sets the current volume attributes. 

  @param GetPhysicalAddress
  Retrieves the memory-mapped address of the firmware volume. 

  @param GetBlockSize
  Retrieves the size for a specific block.

  @param Read
  Reads n bytes into a buffer from the firmware volume hardware.

  @param Write
  Writes n bytes from a buffer into the firmware volume hardware. 

  @param EraseBlocks
  Erases specified block(s) and sets all values as indicated by 
  the EFI_FVB_ERASE_POLARITY bit.

  @param ParentHandle
  Handle of the parent firmware volume.

**/
struct _EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL {
  EFI_FVB_GET_ATTRIBUTES        GetVolumeAttributes;
  EFI_FVB_SET_ATTRIBUTES        SetVolumeAttributes;
  EFI_FVB_GET_PHYSICAL_ADDRESS  GetPhysicalAddress;
  EFI_FVB_GET_BLOCK_SIZE        GetBlockSize;
  EFI_FVB_READ                  Read;
  EFI_FVB_WRITE                 Write;
  EFI_FVB_ERASE_BLOCKS          EraseBlocks;
  EFI_HANDLE                    ParentHandle;
};

extern EFI_GUID gEfiFirmwareVolumeBlockProtocolGuid;

#endif
