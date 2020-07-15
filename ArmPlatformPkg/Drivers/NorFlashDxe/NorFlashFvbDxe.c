/*++ @file  NorFlashFvbDxe.c

 Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 --*/

#include <PiDxe.h>

#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/NvVarStoreFormatted.h>

#include "NorFlashDxe.h"

STATIC EFI_EVENT mFvbVirtualAddrChangeEvent;
STATIC UINTN     mFlashNvStorageVariableBase;

///
/// The Firmware Volume Block Protocol is the low-level interface
/// to a firmware volume. File-level access to a firmware volume
/// should not be done using the Firmware Volume Block Protocol.
/// Normal access to a firmware volume must use the Firmware
/// Volume Protocol. Typically, only the file system driver that
/// produces the Firmware Volume Protocol will bind to the
/// Firmware Volume Block Protocol.
///

/**
  Initialises the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Ptr - Location to initialise the headers

**/
EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN NOR_FLASH_INSTANCE *Instance
  )
{
  EFI_STATUS                          Status;
  VOID*                               Headers;
  UINTN                               HeadersLength;
  EFI_FIRMWARE_VOLUME_HEADER          *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;

  HeadersLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY) + sizeof(VARIABLE_STORE_HEADER);
  Headers = AllocateZeroPool(HeadersLength);

  // FirmwareVolumeHeader->FvLength is declared to have the Variable area AND the FTW working area AND the FTW Spare contiguous.
  ASSERT(PcdGet32(PcdFlashNvStorageVariableBase) + PcdGet32(PcdFlashNvStorageVariableSize) == PcdGet32(PcdFlashNvStorageFtwWorkingBase));
  ASSERT(PcdGet32(PcdFlashNvStorageFtwWorkingBase) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) == PcdGet32(PcdFlashNvStorageFtwSpareBase));

  // Check if the size of the area is at least one block size
  ASSERT((PcdGet32(PcdFlashNvStorageVariableSize) > 0) && (PcdGet32(PcdFlashNvStorageVariableSize) / Instance->Media.BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwWorkingSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwWorkingSize) / Instance->Media.BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwSpareSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwSpareSize) / Instance->Media.BlockSize > 0));

  // Ensure the Variable area Base Addresses are aligned on a block size boundaries
  ASSERT(PcdGet32(PcdFlashNvStorageVariableBase) % Instance->Media.BlockSize == 0);
  ASSERT(PcdGet32(PcdFlashNvStorageFtwWorkingBase) % Instance->Media.BlockSize == 0);
  ASSERT(PcdGet32(PcdFlashNvStorageFtwSpareBase) % Instance->Media.BlockSize == 0);

  //
  // EFI_FIRMWARE_VOLUME_HEADER
  //
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength =
      PcdGet32(PcdFlashNvStorageVariableSize) +
      PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize);
  FirmwareVolumeHeader->Signature = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = (EFI_FVB_ATTRIBUTES_2) (
                                          EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                                          EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                                          EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                                          EFI_FVB2_MEMORY_MAPPED      | // It is memory mapped
                                          EFI_FVB2_ERASE_POLARITY     | // After erasure all bits take this value (i.e. '1')
                                          EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                                          EFI_FVB2_WRITE_ENABLED_CAP    // Writes may be enabled
                                      );
  FirmwareVolumeHeader->HeaderLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = Instance->Media.LastBlock + 1;
  FirmwareVolumeHeader->BlockMap[0].Length      = Instance->Media.BlockSize;
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length      = 0;
  FirmwareVolumeHeader->Checksum = CalculateCheckSum16 ((UINT16*)FirmwareVolumeHeader,FirmwareVolumeHeader->HeaderLength);

  //
  // VARIABLE_STORE_HEADER
  //
  VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)Headers + FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  VariableStoreHeader->Size = PcdGet32(PcdFlashNvStorageVariableSize) - FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format            = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State             = VARIABLE_STORE_HEALTHY;

  // Install the combined super-header in the NorFlash
  Status = FvbWrite (&Instance->FvbProtocol, 0, 0, &HeadersLength, Headers);

  FreePool (Headers);
  return Status;
}

/**
  Check the integrity of firmware volume header.

  @param[in] FwVolHeader - A pointer to a firmware volume header

  @retval  EFI_SUCCESS   - The firmware volume is consistent
  @retval  EFI_NOT_FOUND - The firmware volume has been corrupted.

**/
EFI_STATUS
ValidateFvHeader (
  IN  NOR_FLASH_INSTANCE *Instance
  )
{
  UINT16                      Checksum;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;
  UINTN                       FvLength;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Instance->RegionBaseAddress;

  FvLength = PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize);

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if (   (FwVolHeader->Revision  != EFI_FVH_REVISION)
      || (FwVolHeader->Signature != EFI_FVH_SIGNATURE)
      || (FwVolHeader->FvLength  != FvLength)
      )
  {
    DEBUG ((EFI_D_INFO, "%a: No Firmware Volume header present\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // Check the Firmware Volume Guid
  if( CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid) == FALSE ) {
    DEBUG ((EFI_D_INFO, "%a: Firmware Volume Guid non-compatible\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // Verify the header checksum
  Checksum = CalculateSum16((UINT16*)FwVolHeader, FwVolHeader->HeaderLength);
  if (Checksum != 0) {
    DEBUG ((EFI_D_INFO, "%a: FV checksum is invalid (Checksum:0x%X)\n",
      __FUNCTION__, Checksum));
    return EFI_NOT_FOUND;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)FwVolHeader + FwVolHeader->HeaderLength);

  // Check the Variable Store Guid
  if (!CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
      !CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid)) {
    DEBUG ((EFI_D_INFO, "%a: Variable Store Guid non-compatible\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) - FwVolHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG ((EFI_D_INFO, "%a: Variable Store Length does not match\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
 The GetAttributes() function retrieves the attributes and
 current settings of the block.

 @param This         Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Attributes   Pointer to EFI_FVB_ATTRIBUTES_2 in which the attributes and
                     current settings are returned.
                     Type EFI_FVB_ATTRIBUTES_2 is defined in EFI_FIRMWARE_VOLUME_HEADER.

 @retval EFI_SUCCESS The firmware volume attributes were returned.

 **/
EFI_STATUS
EFIAPI
FvbGetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL    *This,
  OUT       EFI_FVB_ATTRIBUTES_2                   *Attributes
  )
{
  EFI_FVB_ATTRIBUTES_2  FlashFvbAttributes;
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS(This);

  FlashFvbAttributes = (EFI_FVB_ATTRIBUTES_2) (

      EFI_FVB2_READ_ENABLED_CAP | // Reads may be enabled
      EFI_FVB2_READ_STATUS      | // Reads are currently enabled
      EFI_FVB2_STICKY_WRITE     | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
      EFI_FVB2_MEMORY_MAPPED    | // It is memory mapped
      EFI_FVB2_ERASE_POLARITY     // After erasure all bits take this value (i.e. '1')

      );

  // Check if it is write protected
  if (Instance->Media.ReadOnly != TRUE) {

    FlashFvbAttributes = FlashFvbAttributes         |
                         EFI_FVB2_WRITE_STATUS      | // Writes are currently enabled
                         EFI_FVB2_WRITE_ENABLED_CAP;  // Writes may be enabled
  }

  *Attributes = FlashFvbAttributes;

  DEBUG ((DEBUG_BLKIO, "FvbGetAttributes(0x%X)\n", *Attributes));

  return EFI_SUCCESS;
}

/**
 The SetAttributes() function sets configurable firmware volume attributes
 and returns the new settings of the firmware volume.


 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Attributes               On input, Attributes is a pointer to EFI_FVB_ATTRIBUTES_2
                                 that contains the desired firmware volume settings.
                                 On successful return, it contains the new settings of
                                 the firmware volume.
                                 Type EFI_FVB_ATTRIBUTES_2 is defined in EFI_FIRMWARE_VOLUME_HEADER.

 @retval EFI_SUCCESS             The firmware volume attributes were returned.

 @retval EFI_INVALID_PARAMETER   The attributes requested are in conflict with the capabilities
                                 as declared in the firmware volume header.

 **/
EFI_STATUS
EFIAPI
FvbSetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  DEBUG ((DEBUG_BLKIO, "FvbSetAttributes(0x%X) is not supported\n",*Attributes));
  return EFI_UNSUPPORTED;
}

/**
 The GetPhysicalAddress() function retrieves the base address of
 a memory-mapped firmware volume. This function should be called
 only for memory-mapped firmware volumes.

 @param This               Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Address            Pointer to a caller-allocated
                           EFI_PHYSICAL_ADDRESS that, on successful
                           return from GetPhysicalAddress(), contains the
                           base address of the firmware volume.

 @retval EFI_SUCCESS       The firmware volume base address was returned.

 @retval EFI_NOT_SUPPORTED The firmware volume is not memory mapped.

 **/
EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS(This);

  DEBUG ((DEBUG_BLKIO, "FvbGetPhysicalAddress(BaseAddress=0x%08x)\n", Instance->RegionBaseAddress));

  ASSERT(Address != NULL);

  *Address = mFlashNvStorageVariableBase;
  return EFI_SUCCESS;
}

/**
 The GetBlockSize() function retrieves the size of the requested
 block. It also returns the number of additional blocks with
 the identical size. The GetBlockSize() function is used to
 retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                      Indicates the block for which to return the size.

 @param BlockSize                Pointer to a caller-allocated UINTN in which
                                 the size of the block is returned.

 @param NumberOfBlocks           Pointer to a caller-allocated UINTN in
                                 which the number of consecutive blocks,
                                 starting with Lba, is returned. All
                                 blocks in this range have a size of
                                 BlockSize.


 @retval EFI_SUCCESS             The firmware volume base address was returned.

 @retval EFI_INVALID_PARAMETER   The requested LBA is out of range.

 **/
EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  EFI_STATUS Status;
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS(This);

  DEBUG ((DEBUG_BLKIO, "FvbGetBlockSize(Lba=%ld, BlockSize=0x%x, LastBlock=%ld)\n", Lba, Instance->Media.BlockSize, Instance->Media.LastBlock));

  if (Lba > Instance->Media.LastBlock) {
    DEBUG ((EFI_D_ERROR, "FvbGetBlockSize: ERROR - Parameter LBA %ld is beyond the last Lba (%ld).\n", Lba, Instance->Media.LastBlock));
    Status = EFI_INVALID_PARAMETER;
  } else {
    // This is easy because in this platform each NorFlash device has equal sized blocks.
    *BlockSize = (UINTN) Instance->Media.BlockSize;
    *NumberOfBlocks = (UINTN) (Instance->Media.LastBlock - Lba + 1);

    DEBUG ((DEBUG_BLKIO, "FvbGetBlockSize: *BlockSize=0x%x, *NumberOfBlocks=0x%x.\n", *BlockSize, *NumberOfBlocks));

    Status = EFI_SUCCESS;
  }

  return Status;
}

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

 @param This                 Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                  The starting logical block index from which to read.

 @param Offset               Offset into the block at which to begin reading.

 @param NumBytes             Pointer to a UINTN.
                             At entry, *NumBytes contains the total size of the buffer.
                             At exit, *NumBytes contains the total number of bytes read.

 @param Buffer               Pointer to a caller-allocated buffer that will be used
                             to hold the data that is read.

 @retval EFI_SUCCESS         The firmware volume was read successfully,  and contents are
                             in Buffer.

 @retval EFI_BAD_BUFFER_SIZE Read attempted across an LBA boundary.
                             On output, NumBytes contains the total number of bytes
                             returned in Buffer.

 @retval EFI_ACCESS_DENIED   The firmware volume is in the ReadDisabled state.

 @retval EFI_DEVICE_ERROR    The block device is not functioning correctly and could not be read.

 **/
EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL   *This,
  IN        EFI_LBA                               Lba,
  IN        UINTN                                 Offset,
  IN OUT    UINTN                                 *NumBytes,
  IN OUT    UINT8                                 *Buffer
  )
{
  EFI_STATUS    TempStatus;
  UINTN         BlockSize;
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS(This);

  DEBUG ((DEBUG_BLKIO, "FvbRead(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n", Instance->StartLba + Lba, Offset, *NumBytes, Buffer));

  TempStatus = EFI_SUCCESS;

  // Cache the block size to avoid de-referencing pointers all the time
  BlockSize = Instance->Media.BlockSize;

  DEBUG ((DEBUG_BLKIO, "FvbRead: Check if (Offset=0x%x + NumBytes=0x%x) <= BlockSize=0x%x\n", Offset, *NumBytes, BlockSize ));

  // The read must not span block boundaries.
  // We need to check each variable individually because adding two large values together overflows.
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize)) {
    DEBUG ((EFI_D_ERROR, "FvbRead: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n", Offset, *NumBytes, BlockSize ));
    return EFI_BAD_BUFFER_SIZE;
  }

  // We must have some bytes to read
  if (*NumBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Decide if we are doing full block reads or not.
  if (*NumBytes % BlockSize != 0) {
    TempStatus = NorFlashRead (Instance, Instance->StartLba + Lba, Offset, *NumBytes, Buffer);
    if (EFI_ERROR (TempStatus)) {
      return EFI_DEVICE_ERROR;
    }
  } else {
    // Read NOR Flash data into shadow buffer
    TempStatus = NorFlashReadBlocks (Instance, Instance->StartLba + Lba, BlockSize, Buffer);
    if (EFI_ERROR (TempStatus)) {
      // Return one of the pre-approved error statuses
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

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
 state but cannot flip it back again.  Before calling the
 Write() function,  it is recommended for the caller to first call
 the EraseBlocks() function to erase the specified block to
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

 @param This                 Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                  The starting logical block index to write to.

 @param Offset               Offset into the block at which to begin writing.

 @param NumBytes             The pointer to a UINTN.
                             At entry, *NumBytes contains the total size of the buffer.
                             At exit, *NumBytes contains the total number of bytes actually written.

 @param Buffer               The pointer to a caller-allocated buffer that contains the source for the write.

 @retval EFI_SUCCESS         The firmware volume was written successfully.

 @retval EFI_BAD_BUFFER_SIZE The write was attempted across an LBA boundary.
                             On output, NumBytes contains the total number of bytes
                             actually written.

 @retval EFI_ACCESS_DENIED   The firmware volume is in the WriteDisabled state.

 @retval EFI_DEVICE_ERROR    The block device is malfunctioning and could not be written.


 **/
EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL   *This,
  IN        EFI_LBA                               Lba,
  IN        UINTN                                 Offset,
  IN OUT    UINTN                                 *NumBytes,
  IN        UINT8                                 *Buffer
  )
{
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  return NorFlashWriteSingleBlock (Instance, Instance->StartLba + Lba, Offset, NumBytes, Buffer);
}

/**
 Erases and initialises a firmware volume block.

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

 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL
 instance.

 @param ...                      The variable argument list is a list of tuples.
                                 Each tuple describes a range of LBAs to erase
                                 and consists of the following:
                                 - An EFI_LBA that indicates the starting LBA
                                 - A UINTN that indicates the number of blocks to erase.

                                 The list is terminated with an EFI_LBA_LIST_TERMINATOR.
                                 For example, the following indicates that two ranges of blocks
                                 (5-7 and 10-11) are to be erased:
                                 EraseBlocks (This, 5, 3, 10, 2, EFI_LBA_LIST_TERMINATOR);

 @retval EFI_SUCCESS             The erase request successfully completed.

 @retval EFI_ACCESS_DENIED       The firmware volume is in the WriteDisabled state.

 @retval EFI_DEVICE_ERROR        The block device is not functioning correctly and could not be written.
                                 The firmware device may have been partially erased.

 @retval EFI_INVALID_PARAMETER   One or more of the LBAs listed in the variable argument list do
                                 not exist in the firmware volume.

 **/
EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Args;
  UINTN       BlockAddress; // Physical address of Lba to erase
  EFI_LBA     StartingLba; // Lba from which we start erasing
  UINTN       NumOfLba; // Number of Lba blocks to erase
  NOR_FLASH_INSTANCE *Instance;

  Instance = INSTANCE_FROM_FVB_THIS(This);

  DEBUG ((DEBUG_BLKIO, "FvbEraseBlocks()\n"));

  Status = EFI_SUCCESS;

  // Detect WriteDisabled state
  if (Instance->Media.ReadOnly == TRUE) {
    // Firmware volume is in WriteDisabled state
    DEBUG ((EFI_D_ERROR, "FvbEraseBlocks: ERROR - Device is in WriteDisabled state.\n"));
    return EFI_ACCESS_DENIED;
  }

  // Before erasing, check the entire list of parameters to ensure all specified blocks are valid

  VA_START (Args, This);
  do {
    // Get the Lba from which we start erasing
    StartingLba = VA_ARG (Args, EFI_LBA);

    // Have we reached the end of the list?
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      //Exit the while loop
      break;
    }

    // How many Lba blocks are we requested to erase?
    NumOfLba = VA_ARG (Args, UINTN);

    // All blocks must be within range
    DEBUG ((
      DEBUG_BLKIO,
      "FvbEraseBlocks: Check if: ( StartingLba=%ld + NumOfLba=%Lu - 1 ) > LastBlock=%ld.\n",
      Instance->StartLba + StartingLba,
      (UINT64)NumOfLba,
      Instance->Media.LastBlock
      ));
    if ((NumOfLba == 0) || ((Instance->StartLba + StartingLba + NumOfLba - 1) > Instance->Media.LastBlock)) {
      VA_END (Args);
      DEBUG ((EFI_D_ERROR, "FvbEraseBlocks: ERROR - Lba range goes past the last Lba.\n"));
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }
  } while (TRUE);
  VA_END (Args);

  //
  // To get here, all must be ok, so start erasing
  //
  VA_START (Args, This);
  do {
    // Get the Lba from which we start erasing
    StartingLba = VA_ARG (Args, EFI_LBA);

    // Have we reached the end of the list?
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      // Exit the while loop
      break;
    }

    // How many Lba blocks are we requested to erase?
    NumOfLba = VA_ARG (Args, UINTN);

    // Go through each one and erase it
    while (NumOfLba > 0) {

      // Get the physical address of Lba to erase
      BlockAddress = GET_NOR_BLOCK_ADDRESS (
          Instance->RegionBaseAddress,
          Instance->StartLba + StartingLba,
          Instance->Media.BlockSize
      );

      // Erase it
      DEBUG ((DEBUG_BLKIO, "FvbEraseBlocks: Erasing Lba=%ld @ 0x%08x.\n", Instance->StartLba + StartingLba, BlockAddress));
      Status = NorFlashUnlockAndEraseSingleBlock (Instance, BlockAddress);
      if (EFI_ERROR(Status)) {
        VA_END (Args);
        Status = EFI_DEVICE_ERROR;
        goto EXIT;
      }

      // Move to the next Lba
      StartingLba++;
      NumOfLba--;
    }
  } while (TRUE);
  VA_END (Args);

EXIT:
  return Status;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
FvbVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EfiConvertPointer (0x0, (VOID**)&mFlashNvStorageVariableBase);
  return;
}

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE* Instance
  )
{
  EFI_STATUS  Status;
  UINT32      FvbNumLba;
  EFI_BOOT_MODE BootMode;
  UINTN       RuntimeMmioRegionSize;

  DEBUG((DEBUG_BLKIO,"NorFlashFvbInitialize\n"));
  ASSERT((Instance != NULL));

  //
  // Declare the Non-Volatile storage as EFI_MEMORY_RUNTIME
  //

  // Note: all the NOR Flash region needs to be reserved into the UEFI Runtime memory;
  //       even if we only use the small block region at the top of the NOR Flash.
  //       The reason is when the NOR Flash memory is set into program mode, the command
  //       is written as the base of the flash region (ie: Instance->DeviceBaseAddress)
  RuntimeMmioRegionSize = (Instance->RegionBaseAddress - Instance->DeviceBaseAddress) + Instance->Size;

  Status = gDS->AddMemorySpace (
      EfiGcdMemoryTypeMemoryMappedIo,
      Instance->DeviceBaseAddress, RuntimeMmioRegionSize,
      EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
      );
  ASSERT_EFI_ERROR (Status);

  Status = gDS->SetMemorySpaceAttributes (
      Instance->DeviceBaseAddress, RuntimeMmioRegionSize,
      EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  ASSERT_EFI_ERROR (Status);

  mFlashNvStorageVariableBase = FixedPcdGet32 (PcdFlashNvStorageVariableBase);

  // Set the index of the first LBA for the FVB
  Instance->StartLba = (PcdGet32 (PcdFlashNvStorageVariableBase) - Instance->RegionBaseAddress) / Instance->Media.BlockSize;

  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    // Determine if there is a valid header at the beginning of the NorFlash
    Status = ValidateFvHeader (Instance);
  }

  // Install the Default FVB header if required
  if (EFI_ERROR(Status)) {
    // There is no valid header, so time to install one.
    DEBUG ((EFI_D_INFO, "%a: The FVB Header is not valid.\n", __FUNCTION__));
    DEBUG ((EFI_D_INFO, "%a: Installing a correct one for this volume.\n",
      __FUNCTION__));

    // Erase all the NorFlash that is reserved for variable storage
    FvbNumLba = (PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) + PcdGet32(PcdFlashNvStorageFtwSpareSize)) / Instance->Media.BlockSize;

    Status = FvbEraseBlocks (&Instance->FvbProtocol, (EFI_LBA)0, FvbNumLba, EFI_LBA_LIST_TERMINATOR);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    // Install all appropriate headers
    Status = InitializeFvAndVariableStoreHeaders (Instance);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // The driver implementing the variable read service can now be dispatched;
  // the varstore headers are in place.
  //
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEdkiiNvVarStoreFormattedGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mFvbVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
