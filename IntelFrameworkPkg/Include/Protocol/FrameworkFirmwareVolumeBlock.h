/** @file
  This file provides control over block-oriented firmware devices.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference: 
  This protocol is defined in framework spec: Firmware Volume Block Specification.

**/

#ifndef __FRAMEWORK_FIRMWARE_VOLUME_BLOCK_H__
#define __FRAMEWORK_FIRMWARE_VOLUME_BLOCK_H__

#define FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
{ 0xDE28BC59, 0x6228, 0x41BD, {0xBD, 0xF6, 0xA3, 0xB9, 0xAD,0xB5, 0x8D, 0xA1 } }

typedef struct _FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL;
///
/// The type of EFI FVB attribute per the Framework specification.
/// 
typedef UINT32  EFI_FVB_ATTRIBUTES;

/**
  The GetAttributes() function retrieves the attributes and
  current settings of the block. 

  @param This       Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.

  @param Attributes Pointer to EFI_FVB_ATTRIBUTES in which the
                    attributes and current settings are
                    returned. 

  @retval EFI_SUCCESS The firmware volume attributes were
                      returned.

**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_GET_ATTRIBUTES)(
  IN   FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT  EFI_FVB_ATTRIBUTES                *Attributes
);


/**
  The SetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume.

  @param This         Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.

  @param Attributes   On input, Attributes is a pointer to
                      EFI_FVB_ATTRIBUTES that contains the
                      desired firmware volume settings. On
                      successful return, it contains the new
                      settings of the firmware volume. 
  
  @retval EFI_SUCCESS           The firmware volume attributes were returned.

  @retval EFI_INVALID_PARAMETER The attributes requested are in
                                conflict with the capabilities
                                as declared in the firmware
                                volume header.

**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_SET_ATTRIBUTES)(
  IN      FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT  EFI_FVB_ATTRIBUTES                *Attributes
);


/**
  The GetPhysicalAddress() function retrieves the base address of
  a memory-mapped firmware volume. This function should be called
  only for memory-mapped firmware volumes.

  @param This     Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.
  
  @param Address  Pointer to a caller-allocated
                  EFI_PHYSICAL_ADDRESS that, on successful
                  return from GetPhysicalAddress(), contains the
                  base address of the firmware volume.
  
  @retval EFI_SUCCESS       The firmware volume base address is returned.
  
  @retval EFI_NOT_SUPPORTED The firmware volume is not memory mapped.

**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_GET_PHYSICAL_ADDRESS)(
  IN    FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT   EFI_PHYSICAL_ADDRESS                *Address
);

/**
  The GetBlockSize() function retrieves the size of the requested
  block. It also returns the number of additional blocks with
  the identical size. The GetBlockSize() function is used to
  retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


  @param This           Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.

  @param Lba            Indicates the block for which to return the size.

  @param BlockSize      The pointer to a caller-allocated UINTN in which
                        the size of the block is returned.

  @param NumberOfBlocks The pointer to a caller-allocated UINTN in
                        which the number of consecutive blocks,
                        starting with Lba, is returned. All
                        blocks in this range have a size of
                        BlockSize.

  
  @retval EFI_SUCCESS             The firmware volume base address was returned.
  
  @retval EFI_INVALID_PARAMETER   The requested LBA is out of range.

**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_GET_BLOCK_SIZE)(
  IN  FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *BlockSize,
  OUT UINTN                               *NumberOfBlocks
);


/**
  Reads the specified number of bytes into a buffer from the specified block.

  The Read() function reads the requested number of bytes from the
  requested block and stores them in the provided buffer.
  Implementations should be mindful that the firmware volume
  might be in the ReadDisabled state. If it is in this state,
  the Read() function must return the status code
  EFI_ACCESS_DENIED without modifying the contents of the
  buffer. The Read() function must also prevent spanning block
  boundaries. If a read is requested that would span a block
  boundary, the read must read up to the boundary but not
  beyond. The output parameter NumBytes must be set to correctly
  indicate the number of bytes actually read. The caller must be
  aware that a read may be partially completed.

  @param This     Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.
  
  @param Lba      The starting logical block index
                  from which to read.

  @param Offset   Offset into the block at which to begin reading.

  @param NumBytes The pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes read.

  @param Buffer   The pointer to a caller-allocated buffer that will
                  be used to hold the data that is read.

  @retval EFI_SUCCESS         The firmware volume was read successfully
                              and contents are in Buffer.
  
  @retval EFI_BAD_BUFFER_SIZE A read was attempted across an LBA
                              boundary. On output, NumBytes
                              contains the total number of bytes
                              returned in Buffer.
  
  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              ReadDisabled state.
  
  @retval EFI_DEVICE_ERROR    The block device is not
                              functioning correctly and could
                              not be read.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_FVB_READ)(
  IN       FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN       EFI_LBA                             Lba,
  IN       UINTN                               Offset,
  IN OUT   UINTN                               *NumBytes,
  IN OUT   UINT8                               *Buffer
);

/**
  Writes the specified number of bytes from the input buffer to the block.

  The Write() function writes the specified number of bytes from
  the provided buffer to the specified block and offset. If the
  firmware volume is sticky write, the caller must ensure that
  all the bits of the specified range to write are in the
  EFI_FVB_ERASE_POLARITY state before calling the Write()
  function, or else the result will be unpredictable. This
  unpredictability arises because, for a sticky-write firmware
  volume, a write may negate a bit in the EFI_FVB_ERASE_POLARITY
  state but cannot flip it back again. In general, before
  calling the Write() function, the caller should call the
  EraseBlocks() function first to erase the specified block to
  write. A block erase cycle will transition bits from the
  (NOT)EFI_FVB_ERASE_POLARITY state back to the
  EFI_FVB_ERASE_POLARITY state. Implementors should note 
  that the firmware volume might be in the WriteDisabled
  state. If it is in this state, the Write() function must
  return the status code EFI_ACCESS_DENIED without modifying the
  contents of the firmware volume. The Write() function must
  also prevent spanning block boundaries. If a write is
  requested that spans a block boundary, the write must store up
  to the boundary but not beyond. The output parameter NumBytes
  must be set to correctly indicate the number of bytes actually
  written. The caller must be aware that a write may be
  partially completed. All writes, partial or otherwise, must be
  fully flushed to the hardware before the Write() service
  returns.

  @param This     Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.
  
  @param Lba      The starting logical block index to write to.
  
  @param Offset   Offset into the block at which to begin writing.
  
  @param NumBytes The pointer to a UINTN. Input: the total size of the buffer.
                  Output: the total number of bytes actually written.
  
  @param Buffer   The pointer to a caller-allocated buffer that
                  contains the source for the write.
  
  @retval EFI_SUCCESS         The firmware volume was written successfully.
  
  @retval EFI_BAD_BUFFER_SIZE The write was attempted across an
                              LBA boundary. On output, NumBytes
                              contains the total number of bytes
                              actually written.
  
  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.
  
  @retval EFI_DEVICE_ERROR    The block device is malfunctioning
                              and could not be written.


**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_WRITE)(
  IN       FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN       EFI_LBA                             Lba,
  IN       UINTN                               Offset,
  IN OUT   UINTN                               *NumBytes,
  IN       UINT8                               *Buffer
);




///
/// EFI_LBA_LIST_TERMINATOR.
///
#define FRAMEWORK_EFI_LBA_LIST_TERMINATOR   0xFFFFFFFFFFFFFFFFULL


/**
  Erases and initializes a firmware volume block.

  The EraseBlocks() function erases one or more blocks as denoted
  by the variable argument list. The entire parameter list of
  blocks must be verified before erasing any blocks. If a block is
  requested that does not exist within the associated firmware
  volume (it has a larger index than the last block of the
  firmware volume), the EraseBlocks() function must return the
  status code EFI_INVALID_PARAMETER without modifying the contents
  of the firmware volume. Implementors should note that
  the firmware volume might be in the WriteDisabled state. If it
  is in this state, the EraseBlocks() function must return the
  status code EFI_ACCESS_DENIED without modifying the contents of
  the firmware volume. All calls to EraseBlocks() must be fully
  flushed to the hardware before the EraseBlocks() service
  returns.

  @param This   Indicates the FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                instance.

  @param ...    A list of tuples.
                Each tuple describes a range of LBAs to erase
                and consists of the following:
                - An EFI_LBA that indicates the starting LBA
                - A UINTN that indicates the number of blocks to
                  erase

                The list is terminated with an
                EFI_LBA_LIST_TERMINATOR. For example, the
                following indicates that two ranges of blocks
                (5-7 and 10-11) are to be erased: EraseBlocks
                (This, 5, 3, 10, 2, EFI_LBA_LIST_TERMINATOR);

  @retval EFI_SUCCESS The erase request successfully
                      completed.
  
  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.
  @retval EFI_DEVICE_ERROR  The block device is not functioning
                            correctly and could not be written.
                            The firmware device may have been
                            partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.  

**/
typedef
EFI_STATUS
(EFIAPI * FRAMEWORK_EFI_FVB_ERASE_BLOCKS)(
  IN   FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
);

///
/// The Firmware Volume Block Protocol is the low-level interface
/// to a firmware volume. File-level access to a firmware volume
/// should not be done using the Firmware Volume Block Protocol.
/// Normal access to a firmware volume must use the Firmware
/// Volume Protocol. Typically, only the file system driver that
/// produces the Firmware Volume Protocol will bind to the
/// Firmware Volume Block Protocol.
///
struct _FRAMEWORK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL {
  FRAMEWORK_EFI_FVB_GET_ATTRIBUTES        GetAttributes;
  FRAMEWORK_EFI_FVB_SET_ATTRIBUTES        SetAttributes;
  FRAMEWORK_EFI_FVB_GET_PHYSICAL_ADDRESS  GetPhysicalAddress;
  FRAMEWORK_EFI_FVB_GET_BLOCK_SIZE        GetBlockSize;
  FRAMEWORK_EFI_FVB_READ                  Read;
  FRAMEWORK_EFI_FVB_WRITE                 Write;
  FRAMEWORK_EFI_FVB_ERASE_BLOCKS          EraseBlocks;
  ///
  /// The handle of the parent firmware volume.
  ///  
  EFI_HANDLE                    ParentHandle;
};

extern EFI_GUID gFramerworkEfiFirmwareVolumeBlockProtocolGuid;

#endif
