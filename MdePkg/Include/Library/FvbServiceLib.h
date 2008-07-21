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

#ifndef __EDK_FVB_SERVICE_LIB_H__
#define __EDK_FVB_SERVICE_LIB_H__

/**
  Reads specified number of bytes into a buffer from the specified block.

  @param[in]     Instance    The FV instance to be read from.
  @param[in]     Lba         The logical block address to be read from
  @param[in]     Offset      Offset into the block at which to begin reading
  @param[in, out] NumBytes    Pointer that on input contains the total size of
                             the buffer. On output, it contains the total number
                             of bytes read.
  @param[in]     Buffer      Pointer to a caller allocated buffer that will be
                             used to hold the data read.

  @retval   EFI_EFI_SUCCESS        Buffer contains data read from FVB
  @retval   EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
EFIAPI
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );


/**
  Writes specified number of bytes from the input buffer to the block.

  @param[in]     Instance    The FV instance to be read from.
  @param[in]     Lba         The logical block address to be write to 
  @param[in]     Offset      Offset into the block at which to begin writing
  @param[in, out] NumBytes    Pointer that on input contains the total size of
                             the buffer. On output, it contains the total number
                             of bytes actually written.
  @param[in]     Buffer      Pointer to a caller allocated buffer that contains
                             the source for the write.

  @retval   EFI_EFI_SUCCESS        Buffer written to FVB
  @retval   EFI_INVALID_PARAMETER  invalid parameter

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

  @param[in]     Instance    The FV instance to be erased.
  @param[in]     Lba         The logical block address to be erased.

  @retval   EFI_EFI_SUCCESS        Lba was erased
  @retval   EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
EFIAPI
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  );


/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter.

  @param[in]     Instance    The FV instance.
  @param[out]    Attributes  The FV instance whose attributes is going to be 
                             returned.

  @retval   EFI_EFI_SUCCESS        Valid Attributes were returned 
  @retval   EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
EFIAPI
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
  );


/**
  Modifies the current settings of the firmware volume according to the 
  input parameter, and returns the new setting of the volume.

  @param[in]     Instance    The FV instance.
  @param[in, out]Attributes  On input, it is a pointer to EFI_FVB_ATTRIBUTES 
                             containing the desired firmware volume settings.
                             On successful return, it contains the new settings
                             of the firmware volume.

  @retval   EFI_EFI_SUCCESS        Attributes were updated
  @retval   EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
EFIAPI
EfiFvbSetVolumeAttributes (
  IN     UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES                   *Attributes
  );


/**
  Retrieves the physical address of a memory mapped FV.

  @param[in]     Instance    The FV instance
  @param[out]    BaseAddress Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                             that on successful return, contains the base address
                             of the firmware volume. 

  @retval   EFI_EFI_SUCCESS        BaseAddress was returned
  @retval   EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
EFIAPI
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *BaseAddress
  );


/**
  Retrieve the size of a logical block.

  @param[in]     Instance    The FV instance
  @param[in]     Lba         Indicates which block to return the size for.
  @param[out]    BlockSize   A pointer to a caller allocated UINTN in which
                             the size of the block is returned.
  @param[out]    NumOfBlocks a pointer to a caller allocated UINTN in which the
                             number of consecutive blocks starting with Lba is
                             returned. All blocks in this range have a size of
                             BlockSize.

  @retval   EFI_EFI_SUCCESS        BlockSize  and NumOfBlocks returned
  @retval   EFI_INVALID_PARAMETER  invalid parameter

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

  @param[in]     Instance       The FV instance.
  @param[in]     StartLba       The starting logical block index to be erased.
  @param[in]     OffsetStartLba Offset into the starting block at which to 
                                begin erasing.    
  @param[in]     LastLba        The last logical block index to be erased.
  @param[in]     OffsetLastLba  Offset into the last block at which to end erasing.   

  @retval   EFI_EFI_SUCCESS        Range was erased 
  @retval   EFI_INVALID_PARAMETER  invalid parameter
  @retval   EFI_UNSUPPORTED        Range can not be erased

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
