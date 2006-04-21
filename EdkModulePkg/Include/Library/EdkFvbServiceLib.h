/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdkFvbServiceLib.h

Abstract:

--*/
#ifndef __EDK_FVB_SERVICE_LIB_H__
#define __EDK_FVB_SERVICE_LIB_H__

EFI_STATUS
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Reads specified number of bytes into a buffer from the specified block

Arguments:
  Instance              - The FV instance to be read from
  Lba                   - The logical block address to be read from
  Offset                - Offset into the block at which to begin reading
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes read
  Buffer                - Pointer to a caller allocated buffer that will be
                          used to hold the data read

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the block

Arguments:
  Instance              - The FV instance to be written to
  Lba                   - The starting logical block index to write to
  Offset                - Offset into the block at which to begin writing
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  Buffer                - Pointer to a caller allocated buffer that contains
                          the source for the write

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  )
/*++

Routine Description:
  Erases and initializes a firmware volume block

Arguments:
  Instance              - The FV instance to be erased
  Lba                   - The logical block index to be erased
  
Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
  )
/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          returned
  Attributes            - Output buffer which contains attributes

Returns: 
  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN EFI_FVB_ATTRIBUTES                   Attributes
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the 
  input parameter, and returns the new setting of the volume

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          modified
  Attributes            - On input, it is a pointer to EFI_FVB_ATTRIBUTES 
                          containing the desired firmware volume settings.
                          On successful return, it contains the new settings
                          of the firmware volume

Returns: 
  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *BaseAddress
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  BaseAddress           - Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                          that on successful return, contains the base address
                          of the firmware volume. 

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbGetBlockSize (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  )
/*++

Routine Description:
  Retrieve the size of a logical block

Arguments:
  Instance              - The FV instance whose block size is going to be
                          returned
  Lba                   - Indicates which block to return the size for.
  BlockSize             - A pointer to a caller allocated UINTN in which
                          the size of the block is returned
  NumOfBlocks           - a pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize

Returns: 
  EFI_SUCCESS           - The firmware volume was read successfully and 
                          contents are in Buffer
                          
  EFI_INVALID_PARAMETER - invalid parameter

--*/
;

EFI_STATUS
EfiFvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  )
/*++

Routine Description:
  Erases and initializes a specified range of a firmware volume

Arguments:
  Instance              - The FV instance to be erased
  StartLba              - The starting logical block index to be erased
  OffsetStartLba        - Offset into the starting block at which to 
                          begin erasing
  LastLba               - The last logical block index to be erased
  OffsetLastLba         - Offset into the last block at which to end erasing

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter
  
  EFI_UNSUPPORTED       - not support
  
--*/
;

#endif
