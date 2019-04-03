/** @file

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "FwBlockService.h"

ESAL_FWB_GLOBAL         *mFvbModuleGlobal;

EFI_FW_VOL_BLOCK_DEVICE mFvbDeviceTemplate = {
  FVB_DEVICE_SIGNATURE,  // Signature
  //
  // FV_DEVICE_PATH                      FvDevicePath
  //
  {
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
      (EFI_PHYSICAL_ADDRESS) 0,
      (EFI_PHYSICAL_ADDRESS) 0
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  },
  //
  //   UEFI_FV_DEVICE_PATH                 UefiFvDevicePath
  //
  {
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
  },
  0,      // Instance
  //
  // EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FwVolBlockInstance
  //
  {
    FvbProtocolGetAttributes,
    FvbProtocolSetAttributes,
    FvbProtocolGetPhysicalAddress,
    FvbProtocolGetBlockSize,
    FvbProtocolRead,
    FvbProtocolWrite,
    FvbProtocolEraseBlocks,
    NULL
  }
};

UINT32 mInSmmMode = 0;
EFI_SMM_SYSTEM_TABLE2*   mSmst = NULL;

VOID
PublishFlashDeviceInfo (
  IN SPI_INIT_TABLE   *Found
  )
/*++

Routine Description:

  Publish info on found flash device to other drivers via PcdSpiFlashDeviceSize.

Arguments:
  Found                 - Pointer to entry in mSpiInitTable for found flash part.

Returns:
  None

--*/
{
  EFI_STATUS  Status;

  //
  // Publish Byte Size of found flash device.
  //
  Status = PcdSet32S (PcdSpiFlashDeviceSize, (UINT32)(Found->BiosStartOffset + Found->BiosSize));
  ASSERT_EFI_ERROR (Status);
}

VOID
FvbVirtualddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Fixup internal data so that EFI and SAL can be call in virtual mode.
  Call the passed in Child Notify event and convert the mFvbModuleGlobal
  date items to there virtual address.

  mFvbModuleGlobal->FvInstance[FVB_PHYSICAL]  - Physical copy of instance data
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL]   - Virtual pointer to common
                                                instance data.

Arguments:

  (Standard EFI notify event - EFI_EVENT_NOTIFY)

Returns:

  None

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  UINTN               Index;

  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID **) &mFvbModuleGlobal->FvInstance[FVB_VIRTUAL]);

  //
  // Convert the base address of all the instances
  //
  Index       = 0;
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  while (Index < mFvbModuleGlobal->NumFv) {

  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID **) &FwhInstance->FvBase[FVB_VIRTUAL]);
    //
    // SpiWrite and SpiErase always use Physical Address instead of
    // Virtual Address, even in Runtime. So we need not convert pointer
    // for FvWriteBase[FVB_VIRTUAL]
    //
    // EfiConvertPointer (0, (VOID **) &FwhInstance->FvWriteBase[FVB_VIRTUAL]);
    //
    FwhInstance = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhInstance) + FwhInstance->VolumeHeader.HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
    Index++;
  }

  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID **) &mFvbModuleGlobal->FvbScratchSpace[FVB_VIRTUAL]);
  //
  // Convert SPI_PROTOCOL instance for runtime
  //
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID **) &mFvbModuleGlobal->SpiProtocol);
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID **) &mFvbModuleGlobal);
}

VOID
FvbMemWrite8 (
  IN  UINT64                              Dest,
  IN  UINT8                               Byte
  )
{
  MmioWrite8 ((UINTN)Dest, Byte);

  return ;
}

EFI_STATUS
GetFvbInstance (
  IN  UINTN                               Instance,
  IN  ESAL_FWB_GLOBAL                     *Global,
  OUT EFI_FW_VOL_INSTANCE                 **FwhInstance,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  FwhInstance           - The EFI_FW_VOL_INSTANCE fimrware instance structure
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhRecord;

  if (Instance >= Global->NumFv) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the right instance of the FVB private data
  //
  FwhRecord = Global->FvInstance[Virtual];
  while (Instance > 0) {
    FwhRecord = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhRecord) + FwhRecord->VolumeHeader.HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
    Instance--;
  }

  *FwhInstance = FwhRecord;

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  Address               - Pointer to a caller allocated EFI_PHYSICAL_ADDRESS
                          that on successful return, contains the base address
                          of the firmware volume.
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_STATUS          Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Address = FwhInstance->FvBase[Virtual];

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                  *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  Instance              - The FV instance whose attributes is going to be
                          returned
  Attributes            - Output buffer which contains attributes
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_STATUS          Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Attributes = FwhInstance->VolumeHeader.Attributes;

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN                               Instance,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *LbaAddress,
  OUT UINTN                               *LbaWriteAddress,
  OUT UINTN                               *LbaLength,
  OUT UINTN                               *NumOfBlocks,
  IN  ESAL_FWB_GLOBAL                     *Global,
  IN  BOOLEAN                             Virtual
  )
/*++

Routine Description:
  Retrieves the starting address of an LBA in an FV

Arguments:
  Instance              - The FV instance which the Lba belongs to
  Lba                   - The logical block address
  LbaAddress            - On output, contains the physical starting address
                          of the Lba
  LbaWriteAddress       - On output, contains the physical starting address
                          of the Lba for writing
  LbaLength             - On output, contains the length of the block
  NumOfBlocks           - A pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  UINT32                  NumBlocks;
  UINT32                  BlockLength;
  UINTN                   Offset;
  EFI_LBA                 StartLba;
  EFI_LBA                 NextLba;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  EFI_FV_BLOCK_MAP_ENTRY  *BlockMap;
  EFI_STATUS              Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  StartLba  = 0;
  Offset    = 0;
  BlockMap  = &(FwhInstance->VolumeHeader.BlockMap[0]);

  //
  // Parse the blockmap of the FV to find which map entry the Lba belongs to
  //
  while (TRUE) {
    NumBlocks   = BlockMap->NumBlocks;
    BlockLength = BlockMap->Length;

    if ((NumBlocks == 0) || (BlockLength == 0)) {
      return EFI_INVALID_PARAMETER;
    }

    NextLba = StartLba + NumBlocks;

    //
    // The map entry found
    //
    if (Lba >= StartLba && Lba < NextLba) {
      Offset = Offset + (UINTN) MultU64x32 ((Lba - StartLba), BlockLength);
      if (LbaAddress) {
        *LbaAddress = FwhInstance->FvBase[Virtual] + Offset;
      }

      if (LbaWriteAddress) {
        *LbaWriteAddress = FwhInstance->FvWriteBase[Virtual] + Offset;
      }

      if (LbaLength) {
        *LbaLength = BlockLength;
      }

      if (NumOfBlocks) {
        *NumOfBlocks = (UINTN) (NextLba - Lba);
      }

      return EFI_SUCCESS;
    }

    StartLba  = NextLba;
    Offset    = Offset + NumBlocks * BlockLength;
    BlockMap++;
  }
}

EFI_STATUS
FvbReadBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Reads specified number of bytes into a buffer from the specified block

Arguments:
  Instance              - The FV instance to be read from
  Lba                   - The logical block address to be read from
  BlockOffset           - Offset into the block at which to begin reading
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes read
  Buffer                - Pointer to a caller allocated buffer that will be
                          used to hold the data read
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - The firmware volume was read successfully and
                          contents are in Buffer
  EFI_BAD_BUFFER_SIZE   - Read attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes returned
                          in Buffer
  EFI_ACCESS_DENIED     - The firmware volume is in the ReadDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be read
  EFI_INVALID_PARAMETER - Instance not found, or NumBytes, Buffer are NULL

--*/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN               LbaAddress;
  UINTN               LbaLength;
  EFI_STATUS          Status;

  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, NULL, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check if the FV is read enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32) (LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  MmioReadBuffer8 (LbaAddress + BlockOffset, (UINTN) *NumBytes, Buffer);

  return Status;
}

EFI_STATUS
FlashFdWrite (
  IN  UINTN                               WriteAddress,
  IN  UINTN                               Address,
  IN OUT UINTN                            *NumBytes,
  IN  UINT8                               *Buffer,
  IN  UINTN                               LbaLength
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the address

Arguments:

Returns:

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // TODO:  Suggested that this code be "critical section"
  //
  WriteAddress -= ( PcdGet32 (PcdFlashAreaBaseAddress) );
  if (mInSmmMode == 0) { // !(EfiInManagementInterrupt ())) {
    Status = mFvbModuleGlobal->SpiProtocol->Execute (
                                            mFvbModuleGlobal->SpiProtocol,
                                            SPI_OPCODE_WRITE_INDEX, // OpcodeIndex
                                            0,                      // PrefixOpcodeIndex
                                            TRUE,                   // DataCycle
                                            TRUE,                   // Atomic
                                            TRUE,                   // ShiftOut
                                            WriteAddress,           // Address
                                            (UINT32) (*NumBytes),   // Data Number
                                            Buffer,
                                            EnumSpiRegionBios
                                            );

  } else {
    Status = mFvbModuleGlobal->SmmSpiProtocol->Execute (
                                            mFvbModuleGlobal->SmmSpiProtocol,
                                            SPI_OPCODE_WRITE_INDEX, // OpcodeIndex
                                            0,                      // PrefixOpcodeIndex
                                            TRUE,                   // DataCycle
                                            TRUE,                   // Atomic
                                            TRUE,                   // ShiftOut
                                            WriteAddress,           // Address
                                            (UINT32) (*NumBytes),   // Data Number
                                            Buffer,
                                            EnumSpiRegionBios
                                            );
  }

    AsmWbinvd ();

  return Status;
}

EFI_STATUS
FlashFdErase (
  IN UINTN                                WriteAddress,
  IN UINTN                                Address,
  IN UINTN                                LbaLength
  )
/*++

Routine Description:
  Erase a certain block from address LbaWriteAddress

Arguments:

Returns:

--*/
{
  EFI_STATUS  Status;

  WriteAddress -= (PcdGet32 (PcdFlashAreaBaseAddress));
  if (mInSmmMode == 0 ) { // !(EfiInManagementInterrupt ())) {
    Status = mFvbModuleGlobal->SpiProtocol->Execute (
                                            mFvbModuleGlobal->SpiProtocol,
                                            SPI_OPCODE_ERASE_INDEX, // OpcodeIndex
                                            0,                      // PrefixOpcodeIndex
                                            FALSE,                  // DataCycle
                                            TRUE,                   // Atomic
                                            FALSE,                  // ShiftOut
                                            WriteAddress,           // Address
                                            0,                      // Data Number
                                            NULL,
                                            EnumSpiRegionBios       // SPI_REGION_TYPE
                                            );
  } else {
    Status = mFvbModuleGlobal->SmmSpiProtocol->Execute (
                                            mFvbModuleGlobal->SmmSpiProtocol,
                                            SPI_OPCODE_ERASE_INDEX, // OpcodeIndex
                                            0,                      // PrefixOpcodeIndex
                                            FALSE,                  // DataCycle
                                            TRUE,                   // Atomic
                                            FALSE,                  // ShiftOut
                                            WriteAddress,           // Address
                                            0,                      // Data Number
                                            NULL,
                                            EnumSpiRegionBios       // SPI_REGION_TYPE
                                            );
  }

  AsmWbinvd ();

  return Status;
}

EFI_STATUS
FvbWriteBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the block

Arguments:
  Instance              - The FV instance to be written to
  Lba                   - The starting logical block index to write to
  BlockOffset           - Offset into the block at which to begin writing
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  Buffer                - Pointer to a caller allocated buffer that contains
                          the source for the write
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - The firmware volume was written successfully
  EFI_BAD_BUFFER_SIZE   - Write attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes
                          actually written
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be written
  EFI_INVALID_PARAMETER - Instance not found, or NumBytes, Buffer are NULL

--*/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN               LbaAddress;
  UINTN               LbaWriteAddress;
  UINTN               LbaLength;
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_STATUS          Status;
  EFI_STATUS          ReturnStatus;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  //
  // Writes are enabled in the init routine itself
  //
  if (!FwhInstance->WriteEnabled) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaWriteAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32) (LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  ReturnStatus = FlashFdWrite (
                  LbaWriteAddress + BlockOffset,
                  LbaAddress,
                  NumBytes,
                  Buffer,
                  LbaLength
                  );
  if (EFI_ERROR (ReturnStatus)) {
    return ReturnStatus;
  }

  return Status;
}

EFI_STATUS
FvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Erases and initializes a firmware volume block

Arguments:
  Instance              - The FV instance to be erased
  Lba                   - The logical block index to be erased
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - The erase request was successfully completed
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be written. Firmware device may have been
                          partially erased
  EFI_INVALID_PARAMETER - Instance not found

--*/
{

  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN               LbaAddress;
  UINTN               LbaWriteAddress;
  EFI_FW_VOL_INSTANCE *FwhInstance;
  UINTN               LbaLength;
  EFI_STATUS          Status;
  UINTN               SectorNum;
  UINTN               Index;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  //
  // Writes are enabled in the init routine itself
  //
  if (!FwhInstance->WriteEnabled) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Get the starting address of the block for erase. For debug reasons,
  // LbaWriteAddress may not be the same as LbaAddress.
  //
  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaWriteAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SectorNum = LbaLength / SPI_ERASE_SECTOR_SIZE;
  for (Index = 0; Index < SectorNum; Index++){
    Status = FlashFdErase (
               LbaWriteAddress + Index * SPI_ERASE_SECTOR_SIZE,
               LbaAddress,
               SPI_ERASE_SECTOR_SIZE
               );
    if (Status != EFI_SUCCESS){
      break;
    }
  }

  return Status;
}

EFI_STATUS
FvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
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
  OffsetStartLba        - Offset into the last block at which to end erasing
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - The firmware volume was erased successfully
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be written. Firmware device may have been
                          partially erased
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_LBA Index;
  UINTN   LbaSize;
  UINTN   ScratchLbaSizeData;

  //
  // First LBA.
  //
  FvbGetLbaAddress (Instance, StartLba, NULL, NULL, &LbaSize, NULL, Global, Virtual);

  //
  // Use the scratch space as the intermediate buffer to transfer data
  // Back up the first LBA in scratch space.
  //
  FvbReadBlock (Instance, StartLba, 0, &LbaSize, Global->FvbScratchSpace[Virtual], Global, Virtual);

  //
  // erase now
  //
  FvbEraseBlock (Instance, StartLba, Global, Virtual);
  ScratchLbaSizeData = OffsetStartLba;

  //
  // write the data back to the first block
  //
  if (ScratchLbaSizeData > 0) {
    FvbWriteBlock (Instance, StartLba, 0, &ScratchLbaSizeData, Global->FvbScratchSpace[Virtual], Global, Virtual);
  }
  //
  // Middle LBAs
  //
  if (LastLba > (StartLba + 1)) {
    for (Index = (StartLba + 1); Index <= (LastLba - 1); Index++) {
      FvbEraseBlock (Instance, Index, Global, Virtual);
    }
  }
  //
  // Last LBAs, the same as first LBAs
  //
  if (LastLba > StartLba) {
    FvbGetLbaAddress (Instance, LastLba, NULL, NULL, &LbaSize, NULL, Global, Virtual);
    FvbReadBlock (Instance, LastLba, 0, &LbaSize, Global->FvbScratchSpace[Virtual], Global, Virtual);
    FvbEraseBlock (Instance, LastLba, Global, Virtual);
  }

  ScratchLbaSizeData = LbaSize - (OffsetStartLba + 1);

  return FvbWriteBlock (
          Instance,
          LastLba,
          (OffsetLastLba + 1),
          &ScratchLbaSizeData,
          Global->FvbScratchSpace[Virtual],
          Global,
          Virtual
          );
}

EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2               *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the
  input parameter, and returns the new setting of the volume

Arguments:
  Instance              - The FV instance whose attributes is going to be
                          modified
  Attributes            - On input, it is a pointer to EFI_FVB_ATTRIBUTES_2
                          containing the desired firmware volume settings.
                          On successful return, it contains the new settings
                          of the firmware volume
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns:
  EFI_SUCCESS           - Successfully returns
  EFI_ACCESS_DENIED     - The volume setting is locked and cannot be modified
  EFI_INVALID_PARAMETER - Instance not found, or The attributes requested are
                          in conflict with the capabilities as declared in the
                          firmware volume header

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  *AttribPtr;
  UINT32              Capabilities;
  UINT32              OldStatus;
  UINT32              NewStatus;
  EFI_STATUS          Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  AttribPtr     = (EFI_FVB_ATTRIBUTES_2 *) &(FwhInstance->VolumeHeader.Attributes);
  OldAttributes = *AttribPtr;
  Capabilities  = OldAttributes & EFI_FVB2_CAPABILITIES;
  OldStatus     = OldAttributes & EFI_FVB2_STATUS;
  NewStatus     = *Attributes & EFI_FVB2_STATUS;

  //
  // If firmware volume is locked, no status bit can be updated
  //
  if (OldAttributes & EFI_FVB2_LOCK_STATUS) {
    if (OldStatus ^ NewStatus) {
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
    if (NewStatus & EFI_FVB2_READ_STATUS) {
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
    if (NewStatus & EFI_FVB2_WRITE_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test lock
  //
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if (NewStatus & EFI_FVB2_LOCK_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  *AttribPtr  = (*AttribPtr) & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  *AttribPtr  = (*AttribPtr) | NewStatus;
  *Attributes = *AttribPtr;

  return EFI_SUCCESS;
}
//
// FVB protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  OUT EFI_PHYSICAL_ADDRESS                          *Address
  )
/*++

Routine Description:

  Retrieves the physical address of the device.

Arguments:

  This                  - Calling context
  Address               - Output buffer containing the address.

Returns:

Returns:
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetPhysicalAddress (FvbDevice->Instance, Address, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN  EFI_LBA                                       Lba,
  OUT UINTN                                         *BlockSize,
  OUT UINTN                                         *NumOfBlocks
  )
/*++

Routine Description:
  Retrieve the size of a logical block

Arguments:
  This                  - Calling context
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

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetLbaAddress (
          FvbDevice->Instance,
          Lba,
          NULL,
          NULL,
          BlockSize,
          NumOfBlocks,
          mFvbModuleGlobal,
          EfiGoneVirtual ()
          );
}

EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  OUT EFI_FVB_ATTRIBUTES_2                            *Attributes
  )
/*++

Routine Description:
    Retrieves Volume attributes.  No polarity translations are done.

Arguments:
    This                - Calling context
    Attributes          - output buffer which contains attributes

Returns:
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                         *Attributes
  )
/*++

Routine Description:
  Sets Volume attributes. No polarity translations are done.

Arguments:
  This                  - Calling context
  Attributes            - output buffer which contains attributes

Returns:
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbSetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
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
  This                  - Calling context
  ...                   - Starting LBA followed by Number of Lba to erase.
                          a -1 to terminate the list.

Returns:
  EFI_SUCCESS           - The erase request was successfully completed
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be written. Firmware device may have been
                          partially erased

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  UINTN                   NumOfBlocks;
  VA_LIST                 args;
  EFI_LBA                 StartingLba;
  UINTN                   NumOfLba;
  EFI_STATUS              Status;

  FwhInstance = NULL;
  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  Status    = GetFvbInstance (FvbDevice->Instance, mFvbModuleGlobal, &FwhInstance, EfiGoneVirtual ());
  ASSERT_EFI_ERROR (Status);

  NumOfBlocks = FwhInstance->NumOfBlocks;

  VA_START (args, This);

  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINTN);

    //
    // Check input parameters
    //
    if (NumOfLba == 0) {
      VA_END (args);
      return EFI_INVALID_PARAMETER;
    }

    if ((StartingLba + NumOfLba) > NumOfBlocks) {
      return EFI_INVALID_PARAMETER;
    }
  } while (TRUE);

  VA_END (args);

  VA_START (args, This);
  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINTN);

    while (NumOfLba > 0) {
      Status = FvbEraseBlock (FvbDevice->Instance, StartingLba, mFvbModuleGlobal, EfiGoneVirtual ());
      if (EFI_ERROR (Status)) {
        VA_END (args);
        return Status;
      }

      StartingLba++;
      NumOfLba--;
    }

  } while (TRUE);

  VA_END (args);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN EFI_LBA                                        Lba,
  IN UINTN                                          Offset,
  IN OUT UINTN                                      *NumBytes,
  IN UINT8                                          *Buffer
  )
/*++

Routine Description:

  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

Arguments:
  This                  - Calling context
  Lba                   - Block in which to begin write
  Offset                - Offset in the block at which to begin write
  NumBytes              - On input, indicates the requested write size. On
                          output, indicates the actual number of bytes written
  Buffer                - Buffer containing source data for the write.

Returns:
  EFI_SUCCESS           - The firmware volume was written successfully
  EFI_BAD_BUFFER_SIZE   - Write attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes
                          actually written
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be written
  EFI_INVALID_PARAMETER - NumBytes or Buffer are NULL

--*/
{

  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbWriteBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN EFI_LBA                                        Lba,
  IN UINTN                                          Offset,
  IN OUT UINTN                                      *NumBytes,
  IN UINT8                                          *Buffer
  )
/*++

Routine Description:

  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

Arguments:
  This                  - Calling context
  Lba                   - Block in which to begin Read
  Offset                - Offset in the block at which to begin Read
  NumBytes              - On input, indicates the requested write size. On
                          output, indicates the actual number of bytes Read
  Buffer                - Buffer containing source data for the Read.

Returns:
  EFI_SUCCESS           - The firmware volume was read successfully and
                          contents are in Buffer
  EFI_BAD_BUFFER_SIZE   - Read attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes returned
                          in Buffer
  EFI_ACCESS_DENIED     - The firmware volume is in the ReadDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and
                          could not be read
  EFI_INVALID_PARAMETER - NumBytes or Buffer are NULL

--*/
{

  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;
  EFI_STATUS              Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  Status    = FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, EfiGoneVirtual ());

  return Status;
}

EFI_STATUS
ValidateFvHeader (
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader
  )
/*++

Routine Description:
  Check the integrity of firmware volume header

Arguments:
  FwVolHeader           - A pointer to a firmware volume header

Returns:
  EFI_SUCCESS           - The firmware volume is consistent
  EFI_NOT_FOUND         - The firmware volume has corrupted. So it is not an FV

--*/
{
  UINT16  *Ptr;
  UINT16  HeaderLength;
  UINT16  Checksum;

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  #ifndef R864_BUILD
  if (((FwVolHeader->Revision != EFI_FVH_REVISION) && (FwVolHeader->Revision != EFI_FVH_REVISION)) ||
  #else
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
  #endif
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINTN) -1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      ) {
    return EFI_NOT_FOUND;
  }
  //
  // Verify the header checksum
  //
  HeaderLength  = (UINT16) (FwVolHeader->HeaderLength / 2);
  Ptr           = (UINT16 *) FwVolHeader;
  Checksum      = 0;
  while (HeaderLength > 0) {
    Checksum = Checksum + (*Ptr);
    Ptr++;
    HeaderLength--;
  }

  if (Checksum != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetFvbHeader (
  VOID                                **HobList,
  OUT EFI_FIRMWARE_VOLUME_HEADER      **FwVolHeader,
  OUT EFI_PHYSICAL_ADDRESS            *BaseAddress,
  OUT BOOLEAN                         *WriteBack
  )
{
  EFI_STATUS                Status;

  Status        = EFI_SUCCESS;
  *WriteBack    = FALSE;

  if (*FwVolHeader == NULL) {
    *BaseAddress = PcdGet32 (PcdFlashFvRecoveryBase);
  } else if (*FwVolHeader == (VOID *)(UINTN)PcdGet32 (PcdFlashFvRecoveryBase)) {
    *BaseAddress = PcdGet32 (PcdFlashFvMainBase);
  } else if (*FwVolHeader == (VOID *)(UINTN)PcdGet32 (PcdFlashFvMainBase)) {
    *BaseAddress = PcdGet32 (PcdFlashNvStorageVariableBase);
  } else {
    return EFI_NOT_FOUND;
  }

  DEBUG((EFI_D_INFO, "Fvb base : %08x\n",*BaseAddress));

  *FwVolHeader  = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (*BaseAddress);
  Status        = ValidateFvHeader (*FwVolHeader);
  if (EFI_ERROR (Status)) {
    //
    // Get FvbInfo
    //
    *WriteBack  = TRUE;

    Status      = GetFvbInfo (*BaseAddress, FwVolHeader);
    DEBUG(( DEBUG_ERROR, "Through GetFvbInfo: %08x!\n",*BaseAddress));

    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
SmmSpiInit (
  VOID
  )
{
  UINT8       SpiStatus;
  UINT8       FlashIndex;
  UINT8       FlashID[3];
  EFI_STATUS  Status;

  //
  // Obtain a handle for ICH SPI Protocol
  //
  ASSERT(mSmst != NULL);
  if (mFvbModuleGlobal->SmmSpiProtocol == NULL){
    Status = mSmst->SmmLocateProtocol (&gEfiSmmSpiProtocolGuid, NULL, (VOID **) &mFvbModuleGlobal->SmmSpiProtocol);
    ASSERT_EFI_ERROR(Status);
  }
  //
  // attempt to identify flash part and initialize spi table
  //
  for (FlashIndex = 0; FlashIndex < EnumSpiFlashMax; FlashIndex++) {
    Status = mFvbModuleGlobal->SmmSpiProtocol->Init (
                                                mFvbModuleGlobal->SmmSpiProtocol,
                                                &(mSpiInitTable[FlashIndex])
                                                );
    if (!EFI_ERROR (Status)) {
      //
      // read vendor/device IDs to check if flash device is supported
      //
      Status = mFvbModuleGlobal->SmmSpiProtocol->Execute (
                                                  mFvbModuleGlobal->SmmSpiProtocol,
                                                  SPI_OPCODE_JEDEC_ID_INDEX,
                                                  SPI_WREN_INDEX,
                                                  TRUE,
                                                  FALSE,
                                                  FALSE,
                                                  0,
                                                  3,
                                                  FlashID,
                                                  EnumSpiRegionAll
                                                  );
      if (!EFI_ERROR (Status)) {
        if (((FlashID[0] == mSpiInitTable[FlashIndex].VendorId) &&
               (FlashID[2] == mSpiInitTable[FlashIndex].DeviceId1)) ||
              ((FlashID[0] == SPI_AT26DF321_ID1) &&
               (FlashID[0] == mSpiInitTable[FlashIndex].VendorId) &&
               (FlashID[1] == mSpiInitTable[FlashIndex].DeviceId0))) {
          //
          // Supported SPI device found
          //
          DEBUG (
              ((EFI_D_INFO),
              "Smm Mode: Supported SPI Flash device found, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
              FlashID[0],
              FlashID[1],
              FlashID[2])
              );
          break;
        }
      }
    }
  }

  if (FlashIndex >= EnumSpiFlashMax) {
    Status = EFI_UNSUPPORTED;
    DEBUG (
        (EFI_D_ERROR,
        "ERROR - Unknown SPI Flash Device, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
        FlashID[0],
        FlashID[1],
        FlashID[2])
        );
    ASSERT_EFI_ERROR (Status);
  }

  SpiStatus = 0;
  Status = mFvbModuleGlobal->SmmSpiProtocol->Execute (
                                            mFvbModuleGlobal->SmmSpiProtocol,
                                            SPI_OPCODE_WRITE_S_INDEX, // OpcodeIndex
                                            1,                        // PrefixOpcodeIndex
                                            TRUE,                     // DataCycle
                                            TRUE,                     // Atomic
                                            TRUE,                     // ShiftOut
                                            0,                        // Address
                                            1,                        // Data Number
                                            &SpiStatus,
                                            EnumSpiRegionAll          // SPI_REGION_TYPE
                                            );
  return Status;
}

EFI_STATUS
SmmSpiNotificationFunction (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  return SmmSpiInit();
}


VOID
EFIAPI
GetFullDriverPath (
  IN  EFI_HANDLE                  ImageHandle,
  IN  EFI_SYSTEM_TABLE            *SystemTable,
  OUT EFI_DEVICE_PATH_PROTOCOL    **CompleteFilePath
  )
/*++

Routine Description:

  Function is used to get the full device path for this driver.

Arguments:

  ImageHandle        - The loaded image handle of this driver.
  SystemTable        - The pointer of system table.
  CompleteFilePath   - The pointer of returned full file path

Returns:

  none

--*/
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;


  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->HandleProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *) &ImageDevicePath
                  );
  ASSERT_EFI_ERROR (Status);

  *CompleteFilePath = AppendDevicePath (
                        ImageDevicePath,
                        LoadedImage->FilePath
                        );

  return ;
}



EFI_STATUS
FvbInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  This function does common initialization for FVB services

Arguments:

Returns:

--*/
{
  EFI_STATUS                          Status;
  EFI_FW_VOL_INSTANCE                 *FwhInstance;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_FIRMWARE_VOLUME_HEADER          *TempFwVolHeader;
  VOID                                *HobList;
  VOID                                *FirmwareVolumeHobList;
  UINT32                              BufferSize;
  EFI_FV_BLOCK_MAP_ENTRY              *PtrBlockMapEntry;
  BOOLEAN                             WriteEnabled;
  BOOLEAN                             WriteLocked;
  EFI_HANDLE                          FwbHandle;
  EFI_FW_VOL_BLOCK_DEVICE             *FvbDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *OldFwbInterface;
  EFI_DEVICE_PATH_PROTOCOL            *FwbDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *TempFwbDevicePath;
  UINT32                              MaxLbaSize;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  BOOLEAN                             WriteBack;
  UINTN                               NumOfBlocks;
  UINTN                               HeaderLength;
  UINT8                               SpiStatus;
  UINT8                               FlashIndex;
  UINT8                               FlashID[3];
  EFI_DEVICE_PATH_PROTOCOL            *CompleteFilePath;
  UINT8                               PrefixOpcodeIndex;
  BOOLEAN                             InSmm;
  EFI_SMM_BASE2_PROTOCOL              *mSmmBase2;
  EFI_HANDLE                          Handle;

  VOID                                *Registration;
  EFI_EVENT                           Event;

  CompleteFilePath = NULL;
  GetFullDriverPath (ImageHandle, SystemTable, &CompleteFilePath);

 Status = EfiGetSystemConfigurationTable (&gEfiHobListGuid, &HobList);

  //
  // No FV HOBs found
  //
  ASSERT_EFI_ERROR (Status);


  //
  // Allocate runtime services data for global variable, which contains
  // the private data of all firmware volume block instances
  //
  mFvbModuleGlobal = (ESAL_FWB_GLOBAL *)AllocateRuntimeZeroPool(sizeof (ESAL_FWB_GLOBAL  ));
  ASSERT(mFvbModuleGlobal);
  mSmmBase2 = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiSmmBase2ProtocolGuid,
                  NULL,
                  (VOID **) &mSmmBase2
                  );

  if (mSmmBase2 == NULL) {
    InSmm = FALSE;
  } else {
    mSmmBase2->InSmm (mSmmBase2, &InSmm);
    mSmmBase2->GetSmstLocation (mSmmBase2, &mSmst);

  }

  if (!InSmm) {
    mInSmmMode = 0;
    //
    // Obtain a handle for ICH SPI Protocol
    //
    Status = gBS->LocateProtocol (&gEfiSpiProtocolGuid, NULL, (VOID **) &mFvbModuleGlobal->SpiProtocol);
    ASSERT_EFI_ERROR (Status);

    //
    // attempt to identify flash part and initialize spi table
    //
    for (FlashIndex = 0; FlashIndex < EnumSpiFlashMax; FlashIndex++) {
      Status = mFvbModuleGlobal->SpiProtocol->Init (
                                                mFvbModuleGlobal->SpiProtocol,
                                                &(mSpiInitTable[FlashIndex])
                                                );
      if (!EFI_ERROR (Status)) {
        //
        // read vendor/device IDs to check if flash device is supported
        //
        Status = mFvbModuleGlobal->SpiProtocol->Execute (
                                                  mFvbModuleGlobal->SpiProtocol,
                                                  SPI_OPCODE_JEDEC_ID_INDEX,
                                                  SPI_WREN_INDEX,
                                                  TRUE,
                                                  FALSE,
                                                  FALSE,
                                                  0,
                                                  3,
                                                  FlashID,
                                                  EnumSpiRegionAll
                                                  );
        if (!EFI_ERROR (Status)) {
          if (((FlashID[0] == mSpiInitTable[FlashIndex].VendorId) &&
               (FlashID[2] == mSpiInitTable[FlashIndex].DeviceId1)) ||
              ((FlashID[0] == SPI_AT26DF321_ID1) &&
               (FlashID[0] == mSpiInitTable[FlashIndex].VendorId) &&
               (FlashID[1] == mSpiInitTable[FlashIndex].DeviceId0))) {
            //
            // Supported SPI device found
            //
            DEBUG (
              ((EFI_D_INFO),
              "Supported SPI Flash device found, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
              FlashID[0],
              FlashID[1],
              FlashID[2])
              );

            PublishFlashDeviceInfo (&mSpiInitTable[FlashIndex]);
            break;
          }
        }
      }
    }

    if (FlashIndex >= EnumSpiFlashMax) {
      Status = EFI_UNSUPPORTED;
      DEBUG (
        (DEBUG_ERROR,
        "ERROR - Unknown SPI Flash Device, Vendor Id: 0x%02x, Device ID: 0x%02x%02x!\n",
        FlashID[0],
        FlashID[1],
        FlashID[2])
        );
      ASSERT_EFI_ERROR (Status);
    }

    //
    // Unlock all regions by writing to status register
    // This could be SPI device specific, need to follow the datasheet
    // To write to Write Status Register the Spi PrefixOpcode needs to be:
    //   0 for Atmel parts
    //   0 for Intel parts
    //   0 for Macronix parts
    //   0 for Winbond parts
    //   1 for SST parts
    SpiStatus = 0;
    if (FlashID[0] == SPI_SST25VF016B_ID1) {
      PrefixOpcodeIndex = 1;
    } else {
      PrefixOpcodeIndex = 0;
    }
    Status = mFvbModuleGlobal->SpiProtocol->Execute (
                                              mFvbModuleGlobal->SpiProtocol,
                                              SPI_OPCODE_WRITE_S_INDEX, // OpcodeIndex
                                              PrefixOpcodeIndex,        // PrefixOpcodeIndex
                                              TRUE,                     // DataCycle
                                              TRUE,                     // Atomic
                                              TRUE,                     // ShiftOut
                                              0,                        // Address
                                              1,                        // Data Number
                                              &SpiStatus,
                                              EnumSpiRegionAll          // SPI_REGION_TYPE
                                              );


  } else  {
    mInSmmMode = 1;

    Status = mSmst->SmmLocateProtocol (&gEfiSmmSpiProtocolGuid, NULL, (VOID **) &mFvbModuleGlobal->SmmSpiProtocol);
    if (EFI_ERROR(Status)) {
      Registration = NULL;
      Status = mSmst->SmmRegisterProtocolNotify (
                   &gEfiSmmSpiProtocolGuid,
                   SmmSpiNotificationFunction,
                   &Registration
                   );
    } else  {
      Status  = SmmSpiInit();
    }

  }

  //
  // Calculate the total size for all firmware volume block instances
  //
  BufferSize            = 0;
  FirmwareVolumeHobList = HobList;
  FwVolHeader           = NULL;
  do {
    Status = GetFvbHeader (&FirmwareVolumeHobList, &FwVolHeader, &BaseAddress, &WriteBack);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (FwVolHeader) {
      BufferSize += (FwVolHeader->HeaderLength + sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER));
    }
  } while (TRUE);

  //
  // Only need to allocate once. There is only one copy of physical memory for
  // the private data of each FV instance. But in virtual mode or in physical
  // mode, the address of the the physical memory may be different.
  //
  mFvbModuleGlobal->FvInstance[FVB_PHYSICAL] = (EFI_FW_VOL_INSTANCE *) AllocateRuntimeZeroPool (BufferSize);
  ASSERT(mFvbModuleGlobal->FvInstance[FVB_PHYSICAL]);
  //
  // Make a virtual copy of the FvInstance pointer.
  //
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL] = FwhInstance;

  mFvbModuleGlobal->NumFv                   = 0;
  FirmwareVolumeHobList                     = HobList;
  TempFwVolHeader                           = NULL;

  MaxLbaSize = 0;

  //
  // Fill in the private data of each firmware volume block instance
  //
  // Foreach Fv HOB in the FirmwareVolumeHobList, loop
  //
  do {
    Status = GetFvbHeader (&FirmwareVolumeHobList, &TempFwVolHeader, &BaseAddress, &WriteBack);
    if (EFI_ERROR (Status)) {
      break;
    }
    FwVolHeader = TempFwVolHeader;

    if (!FwVolHeader) {
      continue;
    }


    CopyMem ((UINTN *) &(FwhInstance->VolumeHeader), (UINTN *) FwVolHeader, FwVolHeader->HeaderLength);
    FwVolHeader                       = &(FwhInstance->VolumeHeader);

    FwhInstance->FvBase[FVB_PHYSICAL] = (UINTN) BaseAddress;
    FwhInstance->FvBase[FVB_VIRTUAL]  = (UINTN) BaseAddress;

    //
    // FwhInstance->FvWriteBase may not be the same as FwhInstance->FvBase
    //
    FwhInstance->FvWriteBase[FVB_PHYSICAL]  = (UINTN) BaseAddress;
    WriteEnabled = TRUE;

    //
    // Every pointer should have a virtual copy.
    //
    FwhInstance->FvWriteBase[FVB_VIRTUAL] = FwhInstance->FvWriteBase[FVB_PHYSICAL];

    FwhInstance->WriteEnabled             = WriteEnabled;
    EfiInitializeLock (&(FwhInstance->FvbDevLock), TPL_HIGH_LEVEL);

    NumOfBlocks = 0;
    WriteLocked = FALSE;

    if (WriteEnabled) {
      for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
        //
        // Get the maximum size of a block. The size will be used to allocate
        // buffer for Scratch space, the intermediate buffer for FVB extension
        // protocol
        //
        if (MaxLbaSize < PtrBlockMapEntry->Length) {
          MaxLbaSize = PtrBlockMapEntry->Length;
        }

        NumOfBlocks = NumOfBlocks + PtrBlockMapEntry->NumBlocks;
      }
      //
      //  Write back a healthy FV header
      //
      if (WriteBack && (!WriteLocked)) {

        Status = FlashFdErase (
                  (UINTN) FwhInstance->FvWriteBase[0],
                  (UINTN) BaseAddress,
                  FwVolHeader->BlockMap->Length
                  );

        HeaderLength = (UINTN) FwVolHeader->HeaderLength;
        Status = FlashFdWrite (
                  (UINTN) FwhInstance->FvWriteBase[0],
                  (UINTN) BaseAddress,
                  &HeaderLength,
                  (UINT8 *) FwVolHeader,
                  FwVolHeader->BlockMap->Length
                  );

      }
    }
    //
    // The total number of blocks in the FV.
    //
    FwhInstance->NumOfBlocks = NumOfBlocks;

    //
    // If the FV is write locked, set the appropriate attributes
    //
    if (WriteLocked) {
      //
      // write disabled
      //
      FwhInstance->VolumeHeader.Attributes &= ~EFI_FVB2_WRITE_STATUS;
      //
      // lock enabled
      //
      FwhInstance->VolumeHeader.Attributes |= EFI_FVB2_LOCK_STATUS;
    }

    //
    // Allocate and initialize FVB Device in a runtime data buffer
    //
    FvbDevice = AllocateRuntimeCopyPool (sizeof (EFI_FW_VOL_BLOCK_DEVICE), &mFvbDeviceTemplate);
    ASSERT (FvbDevice);

    FvbDevice->Instance = mFvbModuleGlobal->NumFv;
    mFvbModuleGlobal->NumFv++;

    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
    //
    if (FwVolHeader->ExtHeaderOffset == 0) {
      FvbDevice->FvDevicePath.MemMapDevPath.StartingAddress = BaseAddress;
      FvbDevice->FvDevicePath.MemMapDevPath.EndingAddress   = BaseAddress + (FwVolHeader->FvLength - 1);
      FwbDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&FvbDevice->FvDevicePath;
    } else {
      CopyGuid (
        &FvbDevice->UefiFvDevicePath.FvDevPath.FvName,
        (EFI_GUID *)(UINTN)(BaseAddress + FwVolHeader->ExtHeaderOffset)
        );
      FwbDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&FvbDevice->UefiFvDevicePath;
    }

    if (!InSmm) {
      //
      // Find a handle with a matching device path that has supports FW Block protocol
      //
      TempFwbDevicePath = FwbDevicePath;
      Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid, &TempFwbDevicePath, &FwbHandle);
      if (EFI_ERROR (Status)) {
        //
        // LocateDevicePath fails so install a new interface and device path
        //
        FwbHandle = NULL;
        Status = gBS->InstallMultipleProtocolInterfaces (
                        &FwbHandle,
                        &gEfiFirmwareVolumeBlockProtocolGuid,
                        &FvbDevice->FwVolBlockInstance,
                        &gEfiDevicePathProtocolGuid,
                        FwbDevicePath,
                        NULL
                        );
        ASSERT_EFI_ERROR (Status);
      } else if (EfiIsDevicePathEnd (TempFwbDevicePath)) {
        //
        // Device already exists, so reinstall the FVB protocol
        //
        Status = gBS->HandleProtocol (
                        FwbHandle,
                        &gEfiFirmwareVolumeBlockProtocolGuid,
                        (VOID **) &OldFwbInterface
                        );
        ASSERT_EFI_ERROR (Status);

        Status = gBS->ReinstallProtocolInterface (
                        FwbHandle,
                        &gEfiFirmwareVolumeBlockProtocolGuid,
                        OldFwbInterface,
                        &FvbDevice->FwVolBlockInstance
                        );
        ASSERT_EFI_ERROR (Status);

      } else {
        //
        // There was a FVB protocol on an End Device Path node
        //
        ASSERT (FALSE);
      }
    } else {
      FwbHandle = NULL;
      Status = mSmst->SmmInstallProtocolInterface (
                &FwbHandle,
                &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &FvbDevice->FwVolBlockInstance
                );
      ASSERT_EFI_ERROR (Status);
    }

    FwhInstance = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhInstance) + FwVolHeader->HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
  } while (TRUE);

  //
  // Allocate for scratch space, an intermediate buffer for FVB extention
  //

  mFvbModuleGlobal->FvbScratchSpace[FVB_PHYSICAL] = AllocateRuntimeZeroPool (MaxLbaSize);

  ASSERT (mFvbModuleGlobal->FvbScratchSpace[FVB_PHYSICAL]);

  mFvbModuleGlobal->FvbScratchSpace[FVB_VIRTUAL] = mFvbModuleGlobal->FvbScratchSpace[FVB_PHYSICAL];

  if (!InSmm) {
    Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                 FvbVirtualddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // Inform other platform drivers that SPI device discovered and
    // SPI interface ready for use.
    //
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gEfiSmmSpiReadyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
  }
  return EFI_SUCCESS;
}
