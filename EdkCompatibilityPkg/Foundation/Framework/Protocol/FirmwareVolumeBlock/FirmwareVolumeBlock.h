/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolumeBlock.h

Abstract:

  Firmware Volume Block protocol as defined in the Tiano Firmware Volume
  specification.

  Low level firmware device access routines to abstract firmware device
  hardware.

--*/

#ifndef _FW_VOL_BLOCK_H_
#define _FW_VOL_BLOCK_H_

#include "EfiFirmwareVolumeHeader.h"

//
// The following GUID value has been changed to EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL_GUID in
// PI 1.2 spec on purpose. This will force all platforms built with EdkCompatibilityPkg
// produce FVB 2 protocol.
//
#define EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
  { \
    0x8f644fa9, 0xe850, 0x4db1, {0x9c, 0xe2, 0xb, 0x44, 0x69, 0x8e, 0x8d, 0xa4 } \
  }

EFI_FORWARD_DECLARATION (EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL);

typedef EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  OUT EFI_FVB_ATTRIBUTES                          * Attributes
  )
/*++

Routine Description:
  Retrieves Volume attributes.  No polarity translations are done.

Arguments:
  This       - Calling context
  Attributes - output buffer which contains attributes

Returns:
  EFI_INVALID_PARAMETER
  EFI_SUCCESS

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_SET_ATTRIBUTES) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  IN OUT EFI_FVB_ATTRIBUTES                       * Attributes
  )
/*++

Routine Description:
  Sets Volume attributes.  No polarity translations are done.

Arguments:
  This       - Calling context
  Attributes - On input: contains new attributes
               On output: contains current attributes of FV

Returns:
    EFI_INVALID_PARAMETER
    EFI_SUCCESS

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_PHYSICAL_ADDRESS) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  OUT EFI_PHYSICAL_ADDRESS                        * Address
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV.

Arguments:
  This       - Calling context
  Attributes - Address is a pointer to a caller allocated EFI_PHYSICAL_ADDRESS
                 that on successful return from GetPhysicalAddress() contains the
                 base address of the firmware volume. 

Returns:
  EFI_UNSUPPORTED
  EFI_SUCCESS

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_GET_BLOCK_SIZE) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumberOfBlocks
  )
/*++

Routine Description:
  Retrieves the size in bytes of a specific block within an FV.
    
Arguments:
  This            - Calling context.
  Lba             - Indicates which block to return the size for.
  BlockSize       - BlockSize is a pointer to a caller allocated 
                    UINTN in which the size of the block is returned.
  NumberOfBlocks  - NumberOfBlocks is a pointer to a caller allocated
                    UINTN in which the number of consecutive blocks
                    starting with Lba is returned. All blocks in this
                    range have a size of BlockSize.

Returns:
  EFI_INVALID_PARAMETER
  EFI_SUCCESS

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_READ) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  OUT UINT8                                       *Buffer
  )
/*++

Routine Description:
  Reads data beginning at Lba:Offset from FV and places the data in Buffer.
  The read terminates either when *NumBytes of data have been read, or when
  a block boundary is reached.  *NumBytes is updated to reflect the actual
  number of bytes read.

Arguments:
  This - Calling context
  Lba - Block in which to begin read
  Offset - Offset in the block at which to begin read
  NumBytes - At input, indicates the requested read size.  At output, indicates
    the actual number of bytes read.
  Buffer - Data buffer in which to place data read.

Returns:
  EFI_INVALID_PARAMETER
  EFI_NOT_FOUND
  EFI_DEVICE_ERROR
  EFI_SUCCESS

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_WRITE) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:

  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written.

Arguments:
  This - Calling context
  Lba - Block in which to begin write
  Offset - Offset in the block at which to begin write
  NumBytes - At input, indicates the requested write size.  At output, indicates
    the actual number of bytes written.
  Buffer - Buffer containing source data for the write.

Returns:
  EFI_INVALID_PARAMETER
  EFI_NOT_FOUND
  EFI_DEVICE_ERROR
  EFI_SUCCESS

--*/
;

#define EFI_LBA_LIST_TERMINATOR 0xFFFFFFFFFFFFFFFF

typedef
EFI_STATUS
(EFIAPI *EFI_FVB_ERASE_BLOCKS) (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           * This,
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
    EFI_INVALID_PARAMETER
    EFI_DEVICE_ERROR
    EFI_SUCCESS
    EFI_ACCESS_DENIED
    
--*/
;

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
