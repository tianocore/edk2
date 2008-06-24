/** @file
  This file provides control over block-oriented firmware devices.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference: PI
  Version 1.00.

**/

#ifndef __FIRMWARE_VOLUME_BLOCK_H__
#define __FIRMWARE_VOLUME_BLOCK_H__


#define EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
  { 0xDE28BC59, 0x6228, 0x41BD, {0xBD, 0xF6, 0xA3, 0xB9, 0xAD,0xB5, 0x8D, 0xA1 } }


typedef struct _EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL;

/**
  The GetAttributes() function retrieves the attributes and
  current settings of the block. Status Codes Returned

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                instance.

  @param Attributes Pointer to EFI_FVB_ATTRIBUTES in which the
                    attributes and current settings are
                    returned. Type EFI_FVB_ATTRIBUTES is defined
                    in EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS The firmware volume attributes were
                      returned.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_FVB_GET_ATTRIBUTES)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES                  *Attributes
);


/**
  The SetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume.

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                instance.

  @param Attributes   On input, Attributes is a pointer to
                      EFI_FVB_ATTRIBUTES that contains the
                      desired firmware volume settings. On
                      successful return, it contains the new
                      settings of the firmware volume. Type
                      EFI_FVB_ATTRIBUTES is defined in
                      EFI_FIRMWARE_VOLUME_HEADER.

  
  @retval EFI_SUCCESS The firmware volume attributes were
                      returned.

  @retval EFI_INVALID_PARAMETER The attributes requested are in
                                conflict with the capabilities
                                as declared in the firmware
                                volume header.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_FVB_SET_ATTRIBUTES)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES                  *Attributes
);


/**
  The GetPhysicalAddress() function retrieves the base address of
  a memory-mapped firmware volume. This function should be called
  only for memory-mapped firmware volumes.

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                nstance.
  
  @param Address  Pointer to a caller-allocated
                  EFI_PHYSICAL_ADDRESS that, on successful
                  return from GetPhysicalAddress(), contains the
                  base address of the firmware volume. Type
                  EFI_PHYSICAL_ADDRESS is defined in
                  AllocatePages() in the UEFI 2.0 specification.
  
  @retval EFI_SUCCESS The firmware volume base address is
                      returned.
  
  @retval EFI_NOT_SUPPORTED The firmware volume is not memory
                            mapped.


**/
typedef
EFI_STATUS
(EFIAPI * EFI_FVB_GET_PHYSICAL_ADDRESS)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                *Address
);

/**
  The GetBlockSize() function retrieves the size of the requested
  block. It also returns the number of additional blocks with
  the identical size. The GetBlockSize() function is used to
  retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                instance.

  @param Lba  Indicates the block for which to return the size.
              Type EFI_LBA is defined in the BLOCK_IO Protocol
              (section 11.6) in the UEFI 2.0 specification.

  @param BlockSize  Pointer to a caller-allocated UINTN in which
                    the size of the block is returned.

  @param NumberOfBlocks Pointer to a caller-allocated UINTN in
                        which the number of consecutive blocks,
                        starting with Lba, is returned. All
                        blocks in this range have a size of
                        BlockSize.

  
  @retval EFI_SUCCESS The firmware volume base address is
                      returned.
  
  @retval EFI_INVALID_PARAMETER   The requested LBA is out of
                                  range.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_FVB_GET_BLOCK_SIZE)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  OUT       UINTN                               *BlockSize,
  OUT       UINTN                               *NumberOfBlocks
);


/**
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

  @param This Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
              instance. Lba The starting logical block index
              from which to read. Type EFI_LBA is defined in the
              BLOCK_IO Protocol (section 11.6) in the UEFI 2.0
              specification.

  @param Offset Offset into the block at which to begin reading.

  @param NumBytes Pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes read.

  @param Buffer   Pointer to a caller-allocated buffer that will
                  be used to hold the data that is read.

  @retval EFI_SUCCESS The firmware volume was read successfully
                      and contents are in Buffer.
  
  @retval EFI_BAD_BUFFER_SIZE Read attempted across an LBA
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
(EFIAPI *EFI_FVB_READ)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN OUT    UINTN                               *NumBytes,
  IN OUT    UINT8                               *Buffer
);



/**
  The Write() function writes the specified number of bytes from
  the provided buffer to the specified block and offset. If the
  firmware volume is sticky write, the caller must ensure that
  all the bits of the specified range to write are in the
  EFI_FVB_ERASE_POLARITY state before calling the Write()
  function, or else the result will be unpredictable. This
  unpredictability arises because, for a sticky-write firmware
  volume, a write may negate a bit in the EFI_FVB_ERASE_POLARITY
  state but it cannot flip it back again. In general, before
  calling the Write() function, the caller should call the
  EraseBlocks() function first to erase the specified block to
  write. A block erase cycle will transition bits from the
  (NOT)EFI_FVB_ERASE_POLARITY state back to the
  EFI_FVB_ERASE_POLARITY state. Implementations should be
  mindful that the firmware volume might be in the WriteDisabled
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

  @param This Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
              instance.
  
  @param Lba  The starting logical block index to write to. Type
              EFI_LBA is defined in the BLOCK_IO Protocol
              (section 11.6) in the UEFI 2.0 specification.
              Offset Offset into the block at which to begin
              writing.
  
  @param NumBytes Pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes actually written.
  
  @param Buffer Pointer to a caller-allocated buffer that
                contains the source for the write.
  
  @retval EFI_SUCCESS The firmware volume was written
                      successfully.
  
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
(EFIAPI * EFI_FVB_WRITE)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN OUT    UINTN                               *NumBytes,
  IN        UINT8                               *Buffer
);




//
// EFI_LBA_LIST_TERMINATOR
//
#define EFI_LBA_LIST_TERMINATOR   0xFFFFFFFFFFFFFFFFULL


/**
  The EraseBlocks() function erases one or more blocks as denoted
  by the variable argument list. The entire parameter list of
  blocks must be verified before erasing any blocks. If a block is
  requested that does not exist within the associated firmware
  volume (it has a larger index than the last block of the
  firmware volume), the EraseBlocks() function must return the
  status code EFI_INVALID_PARAMETER without modifying the contents
  of the firmware volume. Implementations should be mindful that
  the firmware volume might be in the WriteDisabled state. If it
  is in this state, the EraseBlocks() function must return the
  status code EFI_ACCESS_DENIED without modifying the contents of
  the firmware volume. All calls to EraseBlocks() must be fully
  flushed to the hardware before the EraseBlocks() service
  returns.

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL
                instance.

  @param ...    The variable argument list is a list of tuples.
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

  @retval EFI_SUCCESS The erase request was successfully
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
(EFIAPI * EFI_FVB_ERASE_BLOCKS)(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
);



/**
  The Firmware Volume Block Protocol is the low-level interface
  to a firmware volume. File-level access to a firmware volume
  should not be done using the Firmware Volume Block Protocol.
  Normal access to a firmware volume must use the Firmware
  Volume Protocol. Typically, only the file system driver that
  produces the Firmware Volume Protocol will bind to the
  Firmware Volume Block Protocol. The Firmware Volume Block
  Protocol provides the following:
  - Byte-level read/write functionality.
  - Block-level erase functionality.
  - It further exposes device-hardening features, such as may be
    equired to protect the firmware from unwanted overwriting
    and/or erasure.
  - It is useful to layer a file system driver on top of the
    Firmware Volume Block Protocol.

  This file system driver produces the Firmware Volume Protocol,
  which provides file-level access to a firmware volume. The
  Firmware Volume Protocol abstracts the file system that is
  used to format the firmware volume and the hardware
  device-hardening features that may be present.


  @param GetPhysicalAddress   Retrieves the memory-mapped
                              address of the firmware volume.
                              See the GetPhysicalAddress()
                              function description. 

  @param GetBlockSize   Retrieves the size for a specific block.
                        Also returns the number of consecutive
                        similarly sized blocks. See the
                        GetBlockSize() function description.

  @param Read   Reads n bytes into a buffer from the firmware
                volume hardware. See the Read() function
                description.

  @param Write  Writes n bytes from a buffer into the firmware
                volume hardware. See the Write() function
                description.

  @param EraseBlocks  Erases specified block(s) and sets all
                      values as indicated by the
                      EFI_FVB_ERASE_POLARITY bit. See the
                      EraseBlocks() function description. Type
                      EFI_FVB_ERASE_POLARITY is defined in
                      EFI_FIRMWARE_VOLUME_HEADER. ParentHandle
                      Handle of the parent firmware volume. Type
                      EFI_HANDLE is defined in
                      InstallProtocolInterface() in the UEFI 2.0
                      specification.
  
  @param GetAttributes Retrieves the current volume attributes.
                       See the GetAttributes() function
                       description.
  
  @param SetAttributes Sets the current volume attributes. See
                       the SetAttributes() function description.
  

**/
struct _EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL {
  EFI_FVB_GET_ATTRIBUTES        GetAttributes;
  EFI_FVB_SET_ATTRIBUTES        SetAttributes;
  EFI_FVB_GET_PHYSICAL_ADDRESS  GetPhysicalAddress;
  EFI_FVB_GET_BLOCK_SIZE        GetBlockSize;
  EFI_FVB_READ                  Read;
  EFI_FVB_WRITE                 Write;
  EFI_FVB_ERASE_BLOCKS          EraseBlocks;
  EFI_HANDLE                    ParentHandle;
};


extern EFI_GUID gEfiFirmwareVolumeBlockProtocolGuid;


#endif
