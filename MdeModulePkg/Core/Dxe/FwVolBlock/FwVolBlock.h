/** @file
  Firmware Volume Block protocol functions.
  Consumes FV hobs and creates appropriate block protocols.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FWVOL_BLOCK_H_
#define _FWVOL_BLOCK_H_


#define FVB_DEVICE_SIGNATURE       SIGNATURE_32('_','F','V','B')


typedef struct {
  UINTN                       Base;
  UINTN                       Length;
} LBA_CACHE;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

//
// UEFI Specification define FV device path format if FV provide name guid in extension header
//
typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH    FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  UINTN                                 Signature;
  EFI_HANDLE                            Handle;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
  UINTN                                 NumBlocks;
  LBA_CACHE                             *LbaCache;
  UINT32                                FvbAttributes;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  UINT32                                AuthenticationStatus;
} EFI_FW_VOL_BLOCK_DEVICE;


#define FVB_DEVICE_FROM_THIS(a) \
  CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)


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
  );



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
  );



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
  );



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
  );



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
  );



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
  );



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
  );


#endif
