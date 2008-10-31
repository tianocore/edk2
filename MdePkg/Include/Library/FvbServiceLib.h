/** @file
  Firmeware Volume BLock Service Library

  Copyright (c) 2006 - 2007, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __FVB_SERVICE_LIB_H__
#define __FVB_SERVICE_LIB_H__

/**
  Reads specified number of bytes into a buffer from the specified block.

  The EfiFvbReadBlock() function reads the requested number of bytes from
  the requested block in the specified firmware volume and stores them in
  the provided buffer. Implementations should be mindful that the firmware
  volume might be in the ReadDisabled state.  If it is in this state, the 
  EfiFvbReadBlock() function must return the status code EFI_ACCESS_DENIED
  without modifying the contents of the buffer.
  
  The EfiFvbReadBlock() function must also prevent spanning block boundaries.
  If a read is requested that would span a block boundary, the read must read
  up to the boundary but not beyond.  The output parameter NumBytes must be
  set to correctly indicate the number of bytes actually read.  
  The caller must be aware that a read may be partially completed.

  If NumBytes is NULL, then ASSERT().

  If Buffer is NULL, then ASSERT().

  @param[in]      Instance         The FV instance to be read from.
  @param[in]      Lba              The logical block address to be read from
  @param[in]      Offset           The offset relative to the block, at which to begin reading.
  @param[in, out] NumBytes         Pointer to a UINTN. On input, *NumBytes contains the total
                                   size of the buffer. On output, it contains the actual number
                                   of bytes read.
  @param[out]     Buffer           Pointer to a caller allocated buffer that will be
                                   used to hold the data read.

  @retval   EFI_SUCCESS	           The firmware volume was read successfully and contents are in Buffer.
  @retval   EFI_BAD_BUFFER_SIZE    Read attempted across an LBA boundary.  On output, NumBytes contains the total number of bytes returned in Buffer.
  @retval   EFI_ACCESS_DENIED      The firmware volume is in the ReadDisabled state.
  @retval   EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be read.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter, Instance is larger than the max FVB number. Lba index is larger than the last block of the firmware volume. Offset is larger than the block size.

**/
EFI_STATUS
EFIAPI
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  OUT UINT8                                       *Buffer
  );


/**
  Writes specified number of bytes from the input buffer to the block

  The EfiFvbWriteBlock() function writes the specified number of bytes
  from the provided buffer to the specified block and offset in the 
  requested firmware volume. 

  If the firmware volume is sticky write, the caller must ensure that
  all the bits of the specified range to write are in the EFI_FVB_ERASE_POLARITY
  state before calling the EfiFvbWriteBlock() function, or else the 
  result will be unpredictable.  This unpredictability arises because,
  for a sticky-write firmware volume, a write may negate a bit in the 
  EFI_FVB_ERASE_POLARITY state but it cannot flip it back again. In 
  general, before calling the EfiFvbWriteBlock() function, the caller
  should call the EfiFvbEraseBlock() function first to erase the specified
  block to write. A block erase cycle will transition bits from the
  (NOT)EFI_FVB_ERASE_POLARITY state back to the EFI_FVB_ERASE_POLARITY state.
  Implementations should be mindful that the firmware volume might be 
  in the WriteDisabled state.  If it is in this state, the EfiFvbWriteBlock()
  function must return the status code EFI_ACCESS_DENIED without modifying
  the contents of the firmware volume.
  
  The EfiFvbWriteBlock() function must also prevent spanning block boundaries.
  If a write is requested that spans a block boundary, the write must store
  up to the boundary but not beyond. The output parameter NumBytes must be 
  set to correctly indicate the number of bytes actually written. The caller
  must be aware that a write may be partially completed.
  All writes, partial or otherwise, must be fully flushed to the hardware 
  before the EfiFvbWriteBlock() function returns. 
  
  If NumBytes is NULL, then ASSERT().

  @param Instance               The FV instance to be written to
  @param Lba                    The starting logical block index to write to
  @param Offset                 The offset relative to the block, at which to begin writting.
  @param NumBytes               Pointer to a UINTN. On input, *NumBytes contains
                                the total size of the buffer. On output, it contains
                                the actual number of bytes written.
  @param Buffer                 Pointer to a caller allocated buffer that contains
                                the source for the write

  @retval EFI_SUCCESS           The firmware volume was written successfully.
  @retval EFI_BAD_BUFFER_SIZE   The write was attempted across an LBA boundary. 
                                On output, NumBytes contains the total number of bytes actually written.
  @retval EFI_ACCESS_DENIED	    The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR      The block device is malfunctioning and could not be written.
  @retval EFI_INVALID_PARAMETER Invalid parameter, Instance is larger than the max FVB number. 
                                Lba index is larger than the last block of the firmware volume.
                                Offset is larger than the block size.
**/
EFI_STATUS
EFIAPI
EfiFvbWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );


/**
  Erases and initializes a firmware volume block.

  The EfiFvbEraseBlock() function erases one block specified by Lba.
  Implementations should be mindful that the firmware volume might 
  be in the WriteDisabled state. If it is in this state, the EfiFvbEraseBlock()
  function must return the status code EFI_ACCESS_DENIED without 
  modifying the contents of the firmware volume. If Instance is 
  larger than the max FVB number, or Lba index is larger than the
  last block of the firmware volume, this function return the status
  code EFI_INVALID_PARAMETER.
  
  All calls to EfiFvbEraseBlock() must be fully flushed to the 
  hardware before this function returns. 

  @param[in]     Instance    The FV instance to be erased.
  @param[in]     Lba         The logical block index to be erased from.
  
  @retval EFI_SUCCESS            The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be written.  The firmware device may 
                                 have been partially erased.
  @retval EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max
                                 FVB number. Lba index is larger than the last block
                                 of the firmware volume. 

**/
EFI_STATUS
EFIAPI
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  );


/**
  Retrieves the attributes and current settings of the specified block, 
  returns resulting attributes in output parameter.

  The EfiFvbGetAttributes() function retrieves the attributes and current
  settings of the block specified by Instance. If Instance is larger than
  the max FVB number, this function returns the status code EFI_INVALID_PARAMETER.

  If Attributes is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[out]    Attributes        Pointer to EFI_FVB_ATTRIBUTES_2 in which the
                                   attributes and current settings are returned.

  @retval   EFI_EFI_SUCCESS        The firmware volume attributes were returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 
**/
EFI_STATUS
EFIAPI
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                *Attributes
  );


/**
  Modify the attributes and current settings of the specified block
  according to the input parameter.

  The EfiFvbSetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume specified
  by Instance. If Instance is larger than the max FVB number, this function
  returns the status code EFI_INVALID_PARAMETER.

  If Attributes is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[in, out]Attributes        On input, Attributes is a pointer to EFI_FVB_ATTRIBUTES_2
                                   that contains the desired firmware volume settings.  
                                   On successful return, it contains the new settings of the firmware volume.

  @retval   EFI_EFI_SUCCESS        The firmware volume attributes were modified successfully.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number.

**/
EFI_STATUS
EFIAPI
EfiFvbSetVolumeAttributes (
  IN     UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2                   *Attributes
  );


/**
  Retrieves the physical address of the specified memory mapped FV.

  Retrieve the base address of a memory-mapped firmware volume specified by Instance.
  If Instance is larger than the max FVB number, this function returns the status 
  code EFI_INVALID_PARAMETER.
  
  If BaseAddress is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[out]    BaseAddress       Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                                   that on successful return, contains the base address
                                   of the firmware volume. 

  @retval   EFI_EFI_SUCCESS        The firmware volume base address is returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 

**/
EFI_STATUS
EFIAPI
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *BaseAddress
  );


/**
  Retrieve the block size of the specified fv.
  
  The EfiFvbGetBlockSize() function retrieves the size of the requested block. 
  It also returns the number of additional blocks with the identical size. 
  If Instance is larger than the max FVB number, or Lba index is larger than
  the last block of the firmware volume, this function return the status code
  EFI_INVALID_PARAMETER.

  If BlockSize	is NULL, then ASSERT().
  
  If NumOfBlocks  is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[in]     Lba               Indicates which block to return the size for.
  @param[out]    BlockSize         Pointer to a caller-allocated UINTN in which the
                                   size of the block is returned.
  @param[out]    NumOfBlocks       Pointer to a caller-allocated UINTN in which the 
                                   number of consecutive blocks, starting with Lba, 
                                   is returned. All blocks in this range have a size of BlockSize.

  @retval   EFI_EFI_SUCCESS        The firmware volume base address is returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number.
                                   Lba index is larger than the last block of the firmware volume.

**/
EFI_STATUS
EFIAPI
EfiFvbGetBlockSize (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  );


/**
  Erases and initializes a specified range of a firmware volume.

  The EfiFvbEraseCustomBlockRange() function erases the specified range in the firmware
  volume index by Instance. If Instance is larger than the max FVB number, StartLba or 
  LastLba  index is larger than the last block of the firmware volume, StartLba > LastLba
  or StartLba equal to LastLba but OffsetStartLba > OffsetLastLba, this function return 
  the status code EFI_INVALID_PARAMETER.

  @param[in]     Instance          The FV instance to be operated.
  @param[in]     StartLba          The starting logical block index to be erased.
  @param[in]     OffsetStartLba    Offset into the starting block at which to 
                                   begin erasing.    
  @param[in]     LastLba           The last logical block index to be erased.
  @param[in]     OffsetLastLba     Offset into the last block at which to end erasing.   

  @retval   EFI_EFI_SUCCESS        Successfully erase custom block range
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 
  @retval   EFI_UNSUPPORTED        Firmware volume block device has no this capability.

**/
EFI_STATUS
EFIAPI
EfiFvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  );

#endif
