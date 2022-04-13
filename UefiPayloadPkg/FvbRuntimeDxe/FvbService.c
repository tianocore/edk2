/** @file
Firmware Volume Block Driver to provide FVB service.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvbService.h"

//
// Global variable for this FVB driver  which contains
// the private data of all firmware volume block instances
//
FWB_GLOBAL  mFvbModuleGlobal;

FV_MEMMAP_DEVICE_PATH  mFvMemmapDevicePathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
      }
    },
    EfiMemoryMappedIO,
    (EFI_PHYSICAL_ADDRESS)0,
    (EFI_PHYSICAL_ADDRESS)0,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

FV_PIWG_DEVICE_PATH  mFvPIWGDevicePathTemplate = {
  {
    {
      MEDIA_DEVICE_PATH,
      MEDIA_PIWG_FW_VOL_DP,
      {
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH)),
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH) >> 8)
      }
    },
    { 0 }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

EFI_FW_VOL_BLOCK_DEVICE  mFvbDeviceTemplate = {
  FVB_DEVICE_SIGNATURE,
  NULL,
  0, // Instance
  {
    FvbProtocolGetAttributes,
    FvbProtocolSetAttributes,
    FvbProtocolGetPhysicalAddress,
    FvbProtocolGetBlockSize,
    FvbProtocolRead,
    FvbProtocolWrite,
    FvbProtocolEraseBlocks,
    NULL
  } // FwVolBlockInstance
};

/**
  Get the pointer to EFI_FW_VOL_INSTANCE from the buffer pointed
  by mFvbModuleGlobal.FvInstance based on a index.
  Each EFI_FW_VOL_INSTANCE is  with variable length as
  we have a block map at the end of the EFI_FIRMWARE_VOLUME_HEADER.

  @param[in] Instance The index of the EFI_FW_VOL_INSTANCE.

  @return A pointer to EFI_FW_VOL_INSTANCE.

**/
EFI_FW_VOL_INSTANCE *
GetFvbInstance (
  IN  UINTN  Instance
  )
{
  EFI_FW_VOL_INSTANCE  *FwhRecord;

  if ( Instance >= mFvbModuleGlobal.NumFv ) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return NULL;
  }

  //
  // Find the right instance of the FVB private data
  //
  FwhRecord = mFvbModuleGlobal.FvInstance;
  while ( Instance > 0 ) {
    FwhRecord = (EFI_FW_VOL_INSTANCE *)((UINTN)((UINT8 *)FwhRecord) +
                                        FwhRecord->VolumeHeader.HeaderLength +
                                        (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));
    Instance--;
  }

  return FwhRecord;
}

/**
  Get the EFI_FVB_ATTRIBUTES_2 of a FV.

  @param[in]  Instance    The index of the EFI_FW_VOL_INSTANCE.

  @retval     EFI_FVB_ATTRIBUTES_2 of the FV identified by Instance.

**/
STATIC
EFI_FVB_ATTRIBUTES_2
FvbGetVolumeAttributes (
  IN UINTN  Instance
  )
{
  EFI_FW_VOL_INSTANCE  *FwInstance;

  FwInstance = GetFvbInstance (Instance);
  ASSERT (FwInstance != NULL);

  if (FwInstance == NULL) {
    return 0;
  }

  return FwInstance->VolumeHeader.Attributes;
}

/**
  Retrieves the starting address of an LBA in an FV. It also
  return a few other attribut of the FV.

  @param[in]  Instance        The index of the EFI_FW_VOL_INSTANCE.
  @param[in]  Lba             The logical block address
  @param[out] LbaAddress      On output, contains the physical starting address
                              of the Lba
  @param[out] LbaLength       On output, contains the length of the block
  @param[out] NumOfBlocks     A pointer to a caller allocated UINTN in which the
                              number of consecutive blocks starting with Lba is
                              returned. All blocks in this range have a size of
                              BlockSize

  @retval   EFI_SUCCESS Successfully returns
  @retval   EFI_INVALID_PARAMETER Instance not found

**/
STATIC
EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN    Instance,
  IN  EFI_LBA  Lba,
  OUT UINTN    *LbaAddress,
  OUT UINTN    *LbaLength,
  OUT UINTN    *NumOfBlocks
  )
{
  UINT32                  NumBlocks;
  UINT32                  BlockLength;
  UINTN                   Offset;
  EFI_LBA                 StartLba;
  EFI_LBA                 NextLba;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  EFI_FV_BLOCK_MAP_ENTRY  *BlockMap;

  //
  // Find the right instance of the FVB private data
  //
  FwhInstance = GetFvbInstance (Instance);
  if (FwhInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartLba = 0;
  Offset   = 0;
  BlockMap = &FwhInstance->VolumeHeader.BlockMap[0];
  ASSERT (BlockMap != NULL);

  //
  // Parse the blockmap of the FV to find which map entry the Lba belongs to
  //
  while (TRUE) {
    if ( BlockMap != NULL) {
      NumBlocks   = BlockMap->NumBlocks;
      BlockLength = BlockMap->Length;
    }

    if ((NumBlocks == 0) || (BlockLength == 0)) {
      return EFI_INVALID_PARAMETER;
    }

    NextLba = StartLba + NumBlocks;

    //
    // The map entry found
    //
    if ((Lba >= StartLba) && (Lba < NextLba)) {
      Offset = Offset + (UINTN)MultU64x32 ((Lba - StartLba), BlockLength);
      if (LbaAddress != NULL) {
        *LbaAddress = FwhInstance->FvBase + Offset;
      }

      if (LbaLength != NULL) {
        *LbaLength = BlockLength;
      }

      if (NumOfBlocks != NULL) {
        *NumOfBlocks = (UINTN)(NextLba - Lba);
      }

      return EFI_SUCCESS;
    }

    StartLba = NextLba;
    Offset   = Offset + NumBlocks * BlockLength;
    BlockMap++;
  }
}

/**
  Reads specified number of bytes into a buffer from the specified block

  @param[in]      Instance        The FV instance to be read from
  @param[in]      Lba             The logical block address to be read from
  @param[in]      BlockOffset     Offset into the block at which to begin reading
  @param[in, out] NumBytes        Pointer that on input contains the total size of
                                  the buffer. On output, it contains the total number
                                  of bytes read
  @param[in]      Buffer          Pointer to a caller allocated buffer that will be
                                  used to hold the data read


  @retval         EFI_SUCCESS         The firmware volume was read successfully and
                                      contents are in Buffer
  @retval         EFI_BAD_BUFFER_SIZE Read attempted across a LBA boundary. On output,
                                      NumBytes contains the total number of bytes returned
                                      in Buffer
  @retval         EFI_ACCESS_DENIED   The firmware volume is in the ReadDisabled state
  @retval         EFI_DEVICE_ERROR    The block device is not functioning correctly and
                                      could not be read
  @retval         EFI_INVALID_PARAMETER Instance not found, or NumBytes, Buffer are NULL

**/
STATIC
EFI_STATUS
FvbReadBlock (
  IN UINTN      Instance,
  IN EFI_LBA    Lba,
  IN UINTN      BlockOffset,
  IN OUT UINTN  *NumBytes,
  IN UINT8      *Buffer
  )
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;
  EFI_STATUS            ReadStatus;

  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  if (*NumBytes == 0) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Attributes = FvbGetVolumeAttributes (Instance);

  if ((Attributes & EFI_FVB2_READ_STATUS) == 0) {
    return (EFI_ACCESS_DENIED);
  }

  if (BlockOffset > LbaLength) {
    return (EFI_INVALID_PARAMETER);
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32)(LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  ReadStatus = LibFvbFlashDeviceRead (LbaAddress + BlockOffset, NumBytes, Buffer);
  if (EFI_ERROR (ReadStatus)) {
    return ReadStatus;
  }

  return Status;
}

/**
  Writes specified number of bytes from the input buffer to the block

  @param[in]  Instance          The FV instance to be written to
  @param[in]  Lba               The starting logical block index to write to
  @param[in]  BlockOffset       Offset into the block at which to begin writing
  @param[in, out]  NumBytes     Pointer that on input contains the total size of
                                 the buffer. On output, it contains the total number
                                 of bytes actually written
  @param[in]  Buffer            Pointer to a caller allocated buffer that contains
                                 the source for the write
  @retval     EFI_SUCCESS         The firmware volume was written successfully
  @retval     EFI_BAD_BUFFER_SIZE Write attempted across a LBA boundary. On output,
                                  NumBytes contains the total number of bytes
                                  actually written
  @retval     EFI_ACCESS_DENIED   The firmware volume is in the WriteDisabled state
  @retval     EFI_DEVICE_ERROR    The block device is not functioning correctly and
                                  could not be written
  @retval     EFI_INVALID_PARAMETER Instance not found, or NumBytes, Buffer are NULL

**/
EFI_STATUS
FvbWriteBlock (
  IN UINTN      Instance,
  IN EFI_LBA    Lba,
  IN UINTN      BlockOffset,
  IN OUT UINTN  *NumBytes,
  IN UINT8      *Buffer
  )
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  if (*NumBytes == 0) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the FV is write enabled
  //
  Attributes = FvbGetVolumeAttributes (Instance);
  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if ( LbaLength < (*NumBytes + BlockOffset)) {
    DEBUG ((
      DEBUG_ERROR,
      "FvWriteBlock: Reducing Numbytes from 0x%x to 0x%x\n",
      *NumBytes,
      (UINT32)(LbaLength - BlockOffset)
      ));
    *NumBytes = (UINT32)(LbaLength - BlockOffset);
    return EFI_BAD_BUFFER_SIZE;
  }

  LibFvbFlashDeviceBlockLock (LbaAddress, LbaLength, FALSE);
  Status = LibFvbFlashDeviceWrite (LbaAddress + BlockOffset, NumBytes, Buffer);

  LibFvbFlashDeviceBlockLock (LbaAddress, LbaLength, TRUE);
  WriteBackInvalidateDataCacheRange ((VOID *)(LbaAddress + BlockOffset), *NumBytes);
  return Status;
}

/**
  Erases and initializes a firmware volume block

  @param[in]    Instance    The FV instance to be erased
  @param[in]    Lba         The logical block index to be erased

  @retval   EFI_SUCCESS       The erase request was successfully completed
  @retval   EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state
  @retval   EFI_DEVICE_ERROR  The block device is not functioning correctly and
                              could not be written. Firmware device may have been
                              partially erased
  @retval   EFI_INVALID_PARAMETER Instance not found

**/
EFI_STATUS
FvbEraseBlock (
  IN UINTN    Instance,
  IN EFI_LBA  Lba
  )
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  //
  // Check if the FV is write enabled
  //
  Attributes = FvbGetVolumeAttributes (Instance);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return (EFI_ACCESS_DENIED);
  }

  //
  // Get the starting address of the block for erase.
  //
  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LibFvbFlashDeviceBlockLock (LbaAddress, LbaLength, FALSE);

  Status = LibFvbFlashDeviceBlockErase (LbaAddress, LbaLength);

  LibFvbFlashDeviceBlockLock (LbaAddress, LbaLength, TRUE);

  WriteBackInvalidateDataCacheRange ((VOID *)LbaAddress, LbaLength);

  return Status;
}

/**
  Modifies the current settings of the firmware volume according to the
  input parameter, and returns the new setting of the volume

  @param[in]  Instance              The FV instance whose attributes is going to be
                                    modified
  @param[in, out]  Attributes       On input, it is a pointer to EFI_FVB_ATTRIBUTES_2
                                    containing the desired firmware volume settings.
                                    On successful return, it contains the new settings
                                    of the firmware volume

  @retval     EFI_SUCCESS           Successfully returns
  @retval     EFI_ACCESS_DENIED     The volume setting is locked and cannot be modified
  @retval     EFI_INVALID_PARAMETER Instance not found, or The attributes requested are
                                    in conflict with the capabilities as declared in the
                                    firmware volume header

**/
STATIC
EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                     Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2  *Attributes
  )
{
  EFI_FW_VOL_INSTANCE   *FwhInstance;
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  *AttribPtr;
  EFI_FVB_ATTRIBUTES_2  UnchangedAttributes;
  UINT32                Capabilities;
  UINT32                OldStatus;
  UINT32                NewStatus;

  //
  // Find the right instance of the FVB private data
  //
  FwhInstance = GetFvbInstance (Instance);
  if (FwhInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AttribPtr = (EFI_FVB_ATTRIBUTES_2 *)&(FwhInstance->VolumeHeader.Attributes);
  ASSERT (AttribPtr != NULL);
  if ( AttribPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldAttributes = *AttribPtr;
  Capabilities  = OldAttributes & EFI_FVB2_CAPABILITIES;
  OldStatus     = OldAttributes & EFI_FVB2_STATUS;
  NewStatus     = *Attributes   & EFI_FVB2_STATUS;

  UnchangedAttributes = EFI_FVB2_READ_DISABLED_CAP  | \
                        EFI_FVB2_READ_ENABLED_CAP   | \
                        EFI_FVB2_WRITE_DISABLED_CAP | \
                        EFI_FVB2_WRITE_ENABLED_CAP  | \
                        EFI_FVB2_LOCK_CAP           | \
                        EFI_FVB2_STICKY_WRITE       | \
                        EFI_FVB2_MEMORY_MAPPED      | \
                        EFI_FVB2_ERASE_POLARITY     | \
                        EFI_FVB2_READ_LOCK_CAP      | \
                        EFI_FVB2_WRITE_LOCK_CAP     | \
                        EFI_FVB2_ALIGNMENT;

  //
  // Some attributes of FV is read only can *not* be set
  //
  if ((OldAttributes & UnchangedAttributes) ^ (*Attributes & UnchangedAttributes)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If firmware volume is locked, no status bit can be updated
  //
  if ((OldAttributes & EFI_FVB2_LOCK_STATUS) != 0) {
    if ((OldStatus ^ NewStatus) != 0) {
      return EFI_ACCESS_DENIED;
    }
  }

  //
  // Test read disable
  //
  if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test read enable
  //
  if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test write disable
  //
  if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test write enable
  //
  if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test lock
  //
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_LOCK_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  *AttribPtr  = (*AttribPtr) & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  *AttribPtr  = (*AttribPtr) | NewStatus;
  *Attributes = *AttribPtr;

  return EFI_SUCCESS;
}

/**
  Retrieves the physical address of the device.

  @param[in]  This    A pointer to EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL.
  @param[out] Address Output buffer containing the address.

  @retval     EFI_SUCCESS The function always return successfully.
  @retval     EFI_INVALID_PARAMETER Instance not found.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_PHYSICAL_ADDRESS                     *Address
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  EFI_FW_VOL_INSTANCE      *FwhInstance;

  FvbDevice   = FVB_DEVICE_FROM_THIS (This);
  FwhInstance = GetFvbInstance (FvbDevice->Instance);
  if (FwhInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Address = FwhInstance->FvBase;
  return EFI_SUCCESS;
}

/**
  Retrieve the size of a logical block

  @param[in]  This        Calling context
  @param[in]  Lba         Indicates which block to return the size for.
  @param[out] BlockSize   A pointer to a caller allocated UINTN in which
                          the size of the block is returned
  @param[out] NumOfBlocks A pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize

  @retval     EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN  EFI_LBA                                  Lba,
  OUT UINTN                                    *BlockSize,
  OUT UINTN                                    *NumOfBlocks
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  return FvbGetLbaAddress (FvbDevice->Instance, Lba, NULL, BlockSize, NumOfBlocks);
}

/**
  Retrieves Volume attributes.  No polarity translations are done.

  @param[in]    This        Calling context
  @param[out]   Attributes  Output buffer which contains attributes

  @retval       EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_FVB_ATTRIBUTES_2                     *Attributes
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice   = FVB_DEVICE_FROM_THIS (This);
  *Attributes = FvbGetVolumeAttributes (FvbDevice->Instance);

  return EFI_SUCCESS;
}

/**
  Sets Volume attributes. No polarity translations are done.

  @param[in]      This        Calling context
  @param[in, out] Attributes  Output buffer which contains attributes

  @retval     EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                  *Attributes
  )
{
  EFI_STATUS               Status;
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  Status    = FvbSetVolumeAttributes (FvbDevice->Instance, Attributes);
  return Status;
}

/**
  This function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param[in] This         Calling context
  @param[in] ...          Starting LBA followed by Number of Lba to erase.
                          a -1 to terminate the list.

  @retval EFI_SUCCESS       The erase request was successfully completed
  @retval EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR  The block device is not functioning correctly and
                            could not be written. Firmware device may have been
                            partially erased

**/
EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  EFI_FW_VOL_INSTANCE      *FwhInstance;
  UINTN                    NumOfBlocks;
  VA_LIST                  args;
  EFI_LBA                  StartingLba;
  UINTN                    NumOfLba;
  EFI_STATUS               Status;

  FvbDevice   = FVB_DEVICE_FROM_THIS (This);
  FwhInstance = GetFvbInstance (FvbDevice->Instance);
  if (FwhInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NumOfBlocks = FwhInstance->NumOfBlocks;
  VA_START (args, This);

  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if ( StartingLba == EFI_LBA_LIST_TERMINATOR ) {
      break;
    }

    NumOfLba = VA_ARG (args, UINT32);

    //
    // Check input parameters
    //
    if (NumOfLba == 0) {
      VA_END (args);
      return EFI_INVALID_PARAMETER;
    }

    if ((StartingLba + NumOfLba) > NumOfBlocks ) {
      return EFI_INVALID_PARAMETER;
    }
  } while (1);

  VA_END (args);

  VA_START (args, This);
  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINT32);

    while ( NumOfLba > 0 ) {
      Status = FvbEraseBlock (FvbDevice->Instance, StartingLba);
      if ( EFI_ERROR (Status)) {
        VA_END (args);
        return Status;
      }

      StartingLba++;
      NumOfLba--;
    }
  } while (1);

  VA_END (args);

  return EFI_SUCCESS;
}

/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This      Calling context
  @param[in]      Lba       Block in which to begin write
  @param[in]      Offset    Offset in the block at which to begin write
  @param[in,out]  NumBytes  On input, indicates the requested write size. On
                            output, indicates the actual number of bytes written
  @param[in]      Buffer    Buffer containing source data for the write.

  @retval EFI_SUCCESS           The firmware volume was written successfully
  @retval EFI_BAD_BUFFER_SIZE   Write attempted across a LBA boundary. On output,
                                NumBytes contains the total number of bytes
                                actually written
  @retval EFI_ACCESS_DENIED     The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR      The block device is not functioning correctly and
                                could not be written
  @retval EFI_INVALID_PARAMETER NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN EFI_LBA                                   Lba,
  IN UINTN                                     Offset,
  IN OUT UINTN                                 *NumBytes,
  IN UINT8                                     *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  EFI_STATUS               Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  Status    = FvbWriteBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer);
  DEBUG ((
    DEBUG_VERBOSE,
    "FvbWrite: Lba: 0x%lx Offset: 0x%x NumBytes: 0x%x, Buffer: 0x%x Status:%r\n",
    Lba,
    Offset,
    *NumBytes,
    Buffer,
    Status
    ));

  return Status;
}

/**
  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This      Calling context
  @param[in]      Lba       Block in which to begin write
  @param[in]      Offset    Offset in the block at which to begin write
  @param[in,out]  NumBytes  On input, indicates the requested write size. On
                            output, indicates the actual number of bytes written
  @param[out]     Buffer    Buffer containing source data for the write.


Returns:
  @retval EFI_SUCCESS           The firmware volume was read successfully and
                                contents are in Buffer
  @retval EFI_BAD_BUFFER_SIZE   Read attempted across a LBA boundary. On output,
                                NumBytes contains the total number of bytes returned
                                in Buffer
  @retval EFI_ACCESS_DENIED     The firmware volume is in the ReadDisabled state
  @retval EFI_DEVICE_ERROR      The block device is not functioning correctly and
                                could not be read
  @retval EFI_INVALID_PARAMETER NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN EFI_LBA                                   Lba,
  IN UINTN                                     Offset,
  IN OUT UINTN                                 *NumBytes,
  OUT UINT8                                    *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  EFI_STATUS               Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  Status    = FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer);
  DEBUG ((
    DEBUG_VERBOSE,
    "FvbRead: Lba: 0x%lx Offset: 0x%x NumBytes: 0x%x, Buffer: 0x%x, Status:%r\n",
    Lba,
    Offset,
    *NumBytes,
    Buffer,
    Status
    ));

  return Status;
}

/**
  Check the integrity of firmware volume header in FvBase

  @param[in]  FvBase        A pointer to firmware volume base address.

  @retval     TRUE          The firmware volume is consistent
  @retval     FALSE         The firmware volume has corrupted.

**/
BOOLEAN
IsFvHeaderValid (
  IN       EFI_PHYSICAL_ADDRESS  FvBase
  )
{
  UINT16                      Sum;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if (FvBase == PcdGet32 (PcdFlashNvStorageVariableBase)) {
    if (CompareMem (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid, sizeof (EFI_GUID)) != 0 ) {
      DEBUG ((DEBUG_INFO, "  --FileSystemGuid not match: %g\n", &FwVolHeader->FileSystemGuid));
      return FALSE;
    }
  } else {
    if (CompareMem (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem2Guid, sizeof (EFI_GUID)) != 0 ) {
      DEBUG ((DEBUG_INFO, "  --not expected guid.\n"));
      return FALSE;
    }
  }

  if ((FwVolHeader->Revision != EFI_FVH_REVISION)   ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINTN)-1))       ||
      ((FwVolHeader->HeaderLength & 0x01) != 0))
  {
    DEBUG ((DEBUG_INFO, "  -- >Revision = 0x%x, Signature = 0x%x\n", FwVolHeader->Revision, FwVolHeader->Signature));
    DEBUG ((DEBUG_INFO, "  -- >FvLength = 0x%lx, HeaderLength = 0x%x\n", FwVolHeader->FvLength, FwVolHeader->HeaderLength));
    return FALSE;
  }

  Sum = CalculateSum16 ((UINT16 *)FwVolHeader, FwVolHeader->HeaderLength);
  if (Sum != 0) {
    DEBUG ((DEBUG_INFO, "error: checksum: 0x%04X (expect 0x0)\n", Sum));
    return FALSE;
  }

  return TRUE;
}

/**
  Get intial variable data.

  @param[out]  VarData          Valid variable data.
  @param[out]  VarSize          Valid variable size.

  @retval RETURN_SUCCESS        Successfully found initial variable data.
  @retval RETURN_NOT_FOUND      Failed to find the variable data file from FV.
  @retval EFI_INVALID_PARAMETER VarData or VarSize is null.

**/
EFI_STATUS
GetInitialVariableData (
  OUT VOID   **VarData,
  OUT UINTN  *VarSize
  )
{
  EFI_STATUS                     Status;
  VOID                           *ImageData;
  UINTN                          ImageSize;
  EFI_FIRMWARE_VOLUME_HEADER     *FvHeader;
  VARIABLE_STORE_HEADER          *VariableStore;
  AUTHENTICATED_VARIABLE_HEADER  *Variable;
  UINTN                          VariableSize;
  UINTN                          VarEndAddr;

  if ((VarData == NULL) || (VarSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetSectionFromAnyFv (PcdGetPtr (PcdNvsDataFile), EFI_SECTION_RAW, 0, &ImageData, &ImageSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FvHeader      = (EFI_FIRMWARE_VOLUME_HEADER *)ImageData;
  VariableStore = (VARIABLE_STORE_HEADER *)((UINT8 *)ImageData + FvHeader->HeaderLength);
  VarEndAddr    = (UINTN)VariableStore + VariableStore->Size;
  Variable      = (AUTHENTICATED_VARIABLE_HEADER *)HEADER_ALIGN (VariableStore + 1);
  *VarData      = (VOID *)Variable;
  while (((UINTN)Variable < VarEndAddr)) {
    if (Variable->StartId != VARIABLE_DATA) {
      break;
    }

    VariableSize = sizeof (AUTHENTICATED_VARIABLE_HEADER) + Variable->DataSize + Variable->NameSize;
    Variable     = (AUTHENTICATED_VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)Variable + VariableSize);
  }

  *VarSize = (UINTN)Variable - HEADER_ALIGN (VariableStore + 1);

  return EFI_SUCCESS;
}

/**
  The function does the necessary initialization work for
  Firmware Volume Block Driver.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.
                                It will ASSERT on errors.

**/
EFI_STATUS
FvbInitialize (
  VOID
  )
{
  EFI_FW_VOL_INSTANCE         *FwVolInstance;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FV_BLOCK_MAP_ENTRY      *BlockMap;
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  UINTN                       WriteAddr;
  EFI_STATUS                  Status;
  UINTN                       BufferSize;
  UINTN                       Length;
  VARIABLE_STORE_HEADER       VariableStore;
  VOID                        *VarData;

  InitVariableStore ();
  BaseAddress = PcdGet32 (PcdFlashNvStorageVariableBase);
  FvHeader    = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BaseAddress;

  //
  // Check FV header and variable store header
  //
  if (!IsFvHeaderValid (BaseAddress)) {
    //
    //  Write back a healthy FV header
    //
    DEBUG ((DEBUG_ERROR, "Fvb: Writing back a healthy FV header: 0x%lx\n", BaseAddress));
    FvHeader = GetFvHeaderTemplate ();
    LibFvbFlashDeviceBlockLock ((UINTN)BaseAddress, FvHeader->BlockMap->Length, FALSE);

    Status = LibFvbFlashDeviceBlockErase ((UINTN)BaseAddress, FvHeader->BlockMap->Length);
    ASSERT_EFI_ERROR (Status);

    Length     = FvHeader->HeaderLength;
    WriteAddr  = (UINTN)BaseAddress;
    Status     = LibFvbFlashDeviceWrite (WriteAddr, &Length, (UINT8 *)FvHeader);
    WriteAddr += Length;
    ASSERT_EFI_ERROR (Status);

    //
    // Write back variable store header
    //
    VariableStore.Size   = PcdGet32 (PcdFlashNvStorageVariableSize) - FvHeader->HeaderLength;
    VariableStore.Format = VARIABLE_STORE_FORMATTED;
    VariableStore.State  = VARIABLE_STORE_HEALTHY;
    CopyGuid (&VariableStore.Signature, &gEfiAuthenticatedVariableGuid);
    BufferSize = sizeof (VARIABLE_STORE_HEADER);
    Status     = LibFvbFlashDeviceWrite (WriteAddr, &BufferSize, (UINT8 *)&VariableStore);
    WriteAddr += BufferSize;
    ASSERT_EFI_ERROR (Status);

    //
    // Write initial variable data if found
    //
    Status = GetInitialVariableData (&VarData, &Length);
    if (!EFI_ERROR (Status)) {
      Status = LibFvbFlashDeviceWrite (WriteAddr, &Length, (UINT8 *)VarData);
      ASSERT_EFI_ERROR (Status);
    }

    LibFvbFlashDeviceBlockLock ((UINTN)BaseAddress, FvHeader->BlockMap->Length, TRUE);
    WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)BaseAddress, FvHeader->BlockMap->Length);
  }

  //
  // Create a new FW volume instance for NVS variable
  //
  BufferSize    = FvHeader->HeaderLength + sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER);
  FwVolInstance = (EFI_FW_VOL_INSTANCE *)AllocateRuntimeZeroPool (BufferSize);
  if (FwVolInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FwVolInstance->FvBase = (UINTN)BaseAddress;
  CopyMem (&FwVolInstance->VolumeHeader, FvHeader, FvHeader->HeaderLength);

  //
  // Process the block map for each FV. Assume it has same block size.
  //
  FwVolInstance->NumOfBlocks = 0;
  FvHeader                   = &FwVolInstance->VolumeHeader;
  for (BlockMap = FvHeader->BlockMap; BlockMap->NumBlocks != 0; BlockMap++) {
    FwVolInstance->NumOfBlocks += BlockMap->NumBlocks;
  }

  //
  // Add a FVB Protocol Instance
  //
  Status = InstallFvbProtocol (FwVolInstance, mFvbModuleGlobal.NumFv);
  mFvbModuleGlobal.NumFv++;
  mFvbModuleGlobal.FvInstance = FwVolInstance;

  return Status;
}
