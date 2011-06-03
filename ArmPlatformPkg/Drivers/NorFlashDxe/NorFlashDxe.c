/** @file  NorFlashDxe.c

  Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

#include "NorFlashDxe.h"


//
// Global variable declarations
//
NOR_FLASH_INSTANCE **mNorFlashInstances;

NOR_FLASH_INSTANCE  mNorFlashInstanceTemplate = {
  NOR_FLASH_SIGNATURE, // Signature
  NULL, // Handle ... NEED TO BE FILLED

  FALSE, // Initialized
  NULL, // Initialize

  0, // BaseAddress ... NEED TO BE FILLED
  0, // Size ... NEED TO BE FILLED
  0, // StartLba

  {
    EFI_BLOCK_IO_PROTOCOL_REVISION2, // Revision
    NULL, // Media ... NEED TO BE FILLED
    NorFlashBlockIoReset, // Reset;
    NorFlashBlockIoReadBlocks,          // ReadBlocks
    NorFlashBlockIoWriteBlocks,         // WriteBlocks
    NorFlashBlockIoFlushBlocks          // FlushBlocks
  }, // BlockIoProtocol

  {
    0, // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE, // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching;
    0, // BlockSize ... NEED TO BE FILLED
    4, //  IoAlign
    0, // LastBlock ... NEED TO BE FILLED
    0, // LowestAlignedLba
    1, // LogicalBlocksPerPhysicalBlock
  }, //Media;

  FALSE, // SupportFvb ... NEED TO BE FILLED
  {
    FvbGetAttributes, // GetAttributes
    FvbSetAttributes, // SetAttributes
    FvbGetPhysicalAddress,  // GetPhysicalAddress
    FvbGetBlockSize,  // GetBlockSize
    FvbRead,  // Read
    FvbWrite, // Write
    FvbEraseBlocks, // EraseBlocks
    NULL, //ParentHandle
  }, //  FvbProtoccol;

  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        (UINT8)( sizeof(VENDOR_DEVICE_PATH)      ),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8),
      },
      { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, // GUID ... NEED TO BE FILLED
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      0
    }
    } // DevicePath
};

EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                  NorFlashBase,
  IN UINTN                  NorFlashSize,
  IN UINT32                 MediaId,
  IN UINT32                 BlockSize,
  IN BOOLEAN                SupportFvb,
  IN CONST GUID             *NorFlashGuid,
  OUT NOR_FLASH_INSTANCE**  NorFlashInstance
  )
{
  EFI_STATUS Status;
  NOR_FLASH_INSTANCE* Instance;

  ASSERT(NorFlashInstance != NULL);

  Instance = AllocateCopyPool (sizeof(NOR_FLASH_INSTANCE),&mNorFlashInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->BaseAddress = NorFlashBase;
  Instance->Size = NorFlashSize;

  Instance->BlockIoProtocol.Media = &Instance->Media;
  Instance->Media.MediaId = MediaId;
  Instance->Media.BlockSize = BlockSize;
  Instance->Media.LastBlock = (NorFlashSize / BlockSize)-1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid,NorFlashGuid);

  if (SupportFvb) {
    Instance->SupportFvb = TRUE;
    Instance->Initialize = NorFlashFvbInitialize;

    Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->Handle,
                  &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                  &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
                  NULL
                  );
    if (EFI_ERROR(Status)) {
      FreePool(Instance);
      return Status;
    }
  } else {
    Instance->Initialize = NorFlashBlkIoInitialize;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Instance->Handle,
                    &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                    &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
                    NULL
                    );
    if (EFI_ERROR(Status)) {
      FreePool(Instance);
      return Status;
    }
  }

  *NorFlashInstance = Instance;
  return Status;
}

EFI_STATUS
NorFlashReadCfiData (
  IN UINTN   BaseAddress,
  IN UINTN   CfiOffset,
  IN UINT32  NumberOfBytes,
  OUT UINT32 *Data
  )
{
  UINT32          CurrentByte;
  UINTN           ReadAddress;
  UINT32          ReadData;
  UINT32          Byte1;
  UINT32          Byte2;
  UINT32          CombinedData = 0;
  EFI_STATUS      Status = EFI_SUCCESS;


  if (NumberOfBytes > 4) {
    // Using 32 bit variable so can only read 4 bytes
    return EFI_INVALID_PARAMETER;
  }

  // First combine the base address with the offset address to create an absolute read address.
  // However, because we are in little endian, read from the last address down to the first
  ReadAddress = CREATE_NOR_ADDRESS (BaseAddress, CfiOffset) + (NumberOfBytes - 1) * sizeof(UINT32);

  // Although each read returns 32 bits, because of the NOR Flash structure,
  // each 16 bits (16 MSB and 16 LSB) come from two different chips.
  // When in CFI mode, each chip read returns valid data in only the 8 LSBits;
  // the 8 MSBits are invalid and can be ignored.
  // Therefore, each read address returns one byte from each chip.
  //
  // Also note: As we are in little endian notation and we are reading
  // bytes from incremental addresses, we should assemble them in little endian order.
  for (CurrentByte=0; CurrentByte<NumberOfBytes; CurrentByte++) {
    // Read the bytes from the two chips
    ReadData = MmioRead32(ReadAddress);

    // Check the data validity:
    // The 'Dual Data' function means that
    // each chip should return identical data.
    // If that is not the case then we have a problem.
    Byte1 = GET_LOW_BYTE (ReadData);
    Byte2 = GET_HIGH_BYTE(ReadData);

    if(Byte1 != Byte2) {
      // The two bytes should have been identical
      return EFI_DEVICE_ERROR;
    } else {
      // Each successive iteration of the 'for' loop reads a lower address.
      // As we read lower addresses and as we use little endian,
      // we read lower significance bytes. So combine them in the correct order.
      CombinedData = (CombinedData << 8) | Byte1;

      // Decrement down to the next address
      ReadAddress -= sizeof(UINT32);
    }
  }

  *Data = CombinedData;

  return Status;
}

EFI_STATUS
NorFlashReadStatusRegister (
  IN UINTN SR_Address
  )
{
  volatile UINT32       *pStatusRegister;
  UINT32                StatusRegister;
  UINT32                ErrorMask;
  EFI_STATUS            Status = EFI_SUCCESS;

  // Prepare the read address
  pStatusRegister = (UINT32 *) SR_Address;

  do {
    // Prepare to read the status register
    SEND_NOR_COMMAND (SR_Address, 0, P30_CMD_READ_STATUS_REGISTER);
    // Snapshot the status register
    StatusRegister = *pStatusRegister;
  }
  // The chip is busy while the WRITE bit is not asserted
  while ((StatusRegister & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);


  // Perform a full status check:
  // Mask the relevant bits of Status Register.
  // Everything should be zero, if not, we have a problem

  // Prepare the Error Mask by setting bits 5, 4, 3, 1
  ErrorMask = P30_SR_BIT_ERASE | P30_SR_BIT_PROGRAM | P30_SR_BIT_VPP | P30_SR_BIT_BLOCK_LOCKED ;

  if ( (StatusRegister & ErrorMask) != 0 ) {
    if ( (StatusRegister & P30_SR_BIT_VPP) != 0 ) {
      DEBUG((EFI_D_ERROR,"NorFlashReadStatusRegister: VPP Range Error\n"));
    } else if ( (StatusRegister & (P30_SR_BIT_ERASE | P30_SR_BIT_PROGRAM) ) != 0 ) {
      DEBUG((EFI_D_ERROR,"NorFlashReadStatusRegister: Command Sequence Error\n"));
    } else if ( (StatusRegister & P30_SR_BIT_PROGRAM) != 0 ) {
      DEBUG((EFI_D_ERROR,"NorFlashReadStatusRegister: Program Error\n"));
    } else if ( (StatusRegister & P30_SR_BIT_BLOCK_LOCKED) != 0 ) {
      DEBUG((EFI_D_ERROR,"NorFlashReadStatusRegister: Device Protect Error\n"));
    } else {
      DEBUG((EFI_D_ERROR,"NorFlashReadStatusRegister: Error (0x%X)\n",Status));
    }

    // If an error is detected we must clear the Status Register
    SEND_NOR_COMMAND(SR_Address, 0, P30_CMD_CLEAR_STATUS_REGISTER);
    Status = EFI_DEVICE_ERROR;
  }

  SEND_NOR_COMMAND(SR_Address, 0, P30_CMD_READ_ARRAY);

  return Status;
}


BOOLEAN
NorFlashBlockIsLocked (
  IN UINTN BlockAddress
  )
{
  UINT32                LockStatus;
  BOOLEAN               BlockIsLocked = TRUE;

  // Send command for reading device id
  SEND_NOR_COMMAND (BlockAddress, 2, P30_CMD_READ_DEVICE_ID);

  // Read block lock status
  LockStatus = MmioRead32 (CREATE_NOR_ADDRESS( BlockAddress, 2 ));

  // Decode block lock status
  LockStatus = FOLD_32BIT_INTO_16BIT(LockStatus);

  if((LockStatus & 0x2) != 0) {
    DEBUG((EFI_D_ERROR, "UnlockSingleBlock: WARNING: Block LOCKED DOWN\n"));
  }

  if((LockStatus & 0x1) == 0) {
    // This means the block is unlocked
    DEBUG((DEBUG_BLKIO, "UnlockSingleBlock: Block 0x%08x unlocked\n", BlockAddress ));
    BlockIsLocked = FALSE;
  }

  return BlockIsLocked;
}


EFI_STATUS
NorFlashUnlockSingleBlock (
  IN UINTN  BlockAddress
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;

  // Raise the Task Priority Level to TPL_NOTIFY to serialise all its operations
  // and to protect shared data structures.

  // Request a lock setup
  SEND_NOR_COMMAND(BlockAddress, 0, P30_CMD_LOCK_BLOCK_SETUP);

  // Request an unlock
  SEND_NOR_COMMAND(BlockAddress, 0, P30_CMD_UNLOCK_BLOCK);

  // Put device back into Read Array mode
  SEND_NOR_COMMAND(BlockAddress, 0, P30_CMD_READ_ARRAY);

  DEBUG((DEBUG_BLKIO, "UnlockSingleBlock: BlockAddress=0x%08x, Exit Status = \"%r\".\n", BlockAddress, Status));

  return Status;
}


EFI_STATUS
NorFlashUnlockSingleBlockIfNecessary (
  IN UINTN BlockAddress
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if ( NorFlashBlockIsLocked(BlockAddress) == TRUE ) {
    Status = NorFlashUnlockSingleBlock(BlockAddress);
  }

  return Status;
}


/**
 * The following function presumes that the block has already been unlocked.
 **/
EFI_STATUS
NorFlashEraseSingleBlock (
  IN UINTN BlockAddress
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;

  // Request a block erase and then confirm it
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_BLOCK_ERASE_SETUP);
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_BLOCK_ERASE_CONFIRM);
  // Wait until the status register gives us the all clear
  Status = NorFlashReadStatusRegister( BlockAddress );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_BLKIO, "EraseSingleBlock(BlockAddress=0x%08x) = '%r'\n", BlockAddress, Status));
  }
  return Status;
}

/**
 * The following function presumes that the block has already been unlocked.
 **/
EFI_STATUS
NorFlashUnlockAndEraseSingleBlock (
  IN  UINTN   BlockAddress
  )
{
  EFI_STATUS   Status;

  // Unlock the block if we have to
  Status = NorFlashUnlockSingleBlockIfNecessary (BlockAddress);
  if (!EFI_ERROR(Status)) {
    Status = NorFlashEraseSingleBlock(BlockAddress);
  }

  return Status;
}


EFI_STATUS
NorFlashWriteSingleWord (
  IN  UINTN   WordAddress,
  IN  UINT32  WriteData
  )
{
  EFI_STATUS            Status;
  volatile UINT32       *Data;

  // Prepare the read address
  Data = (UINT32 *)WordAddress;

  // Request a write single word command
  SEND_NOR_COMMAND( WordAddress, 0, P30_CMD_WORD_PROGRAM_SETUP );

  // Store the word into NOR Flash;
  *Data = WriteData;

  // Wait for the write to complete and then check for any errors; i.e. check the Status Register
  Status = NorFlashReadStatusRegister( WordAddress );

  return Status;
}

/*
 * Writes data to the NOR Flash using the Buffered Programming method.
 *
 * The maximum size of the on-chip buffer is 32-words, because of hardware restrictions.
 * Therefore this function will only handle buffers up to 32 words or 128 bytes.
 * To deal with larger buffers, call this function again.
 *
 * This function presumes that both the TargetAddress and the TargetAddress+BufferSize
 * exist entirely within the NOR Flash. Therefore these conditions will not be checked here.
 *
 * In buffered programming, if the target address not at the beginning of a 32-bit word boundary,
 * then programming time is doubled and power consumption is increased.
 * Therefore, it is a requirement to align buffer writes to 32-bit word boundaries.
 * i.e. the last 4 bits of the target start address must be zero: 0x......00
 */
EFI_STATUS
NorFlashWriteBuffer (
  IN  UINTN   TargetAddress,
  IN  UINTN   BufferSizeInBytes,
  IN  UINT32  *Buffer
  )
{
  EFI_STATUS            Status;
  UINTN                 BufferSizeInWords;
  UINTN                 Count;
  volatile UINT32       *Data;
  UINTN                 WaitForBuffer   = MAX_BUFFERED_PROG_ITERATIONS;
  BOOLEAN               BufferAvailable = FALSE;


  // Check that the target address does not cross a 32-word boundary.
  if ( (TargetAddress & BOUNDARY_OF_32_WORDS) != 0 ) {
    return EFI_INVALID_PARAMETER;
  }

  // Check there are some data to program
  if ( BufferSizeInBytes == 0 ) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Check that the buffer size does not exceed the maximum hardware buffer size on chip.
  if ( BufferSizeInBytes > P30_MAX_BUFFER_SIZE_IN_BYTES ) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Check that the buffer size is a multiple of 32-bit words
  if ( (BufferSizeInBytes % 4) != 0 ) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Pre-programming conditions checked, now start the algorithm.

  // Prepare the data destination address
  Data = (UINT32 *)TargetAddress;

  // Check the availability of the buffer
  do {
    // Issue the Buffered Program Setup command
    SEND_NOR_COMMAND( TargetAddress, 0, P30_CMD_BUFFERED_PROGRAM_SETUP );

    // Read back the status register bit#7 from the same address
    if ( ((*Data) & P30_SR_BIT_WRITE) == P30_SR_BIT_WRITE ) {
      BufferAvailable = TRUE;
    }

    // Update the loop counter
    WaitForBuffer--;

  } while (( WaitForBuffer > 0 ) && ( BufferAvailable == FALSE ));

  // The buffer was not available for writing
  if ( WaitForBuffer == 0 ) {
    return EFI_DEVICE_ERROR;
  }

  // From now on we work in 32-bit words
  BufferSizeInWords = BufferSizeInBytes / (UINTN)4;

  // Write the word count, which is (buffer_size_in_words - 1),
  // because word count 0 means one word.
  SEND_NOR_COMMAND( TargetAddress, 0, (BufferSizeInWords - 1) );

  // Write the data to the NOR Flash, advancing each address by 4 bytes
  for( Count=0; Count<BufferSizeInWords; Count++, Data++, Buffer++ ) {
    *Data = *Buffer;
  }

  // Issue the Buffered Program Confirm command, to start the programming operation
  SEND_NOR_COMMAND( TargetAddress, 0, P30_CMD_BUFFERED_PROGRAM_CONFIRM );

  // Wait for the write to complete and then check for any errors; i.e. check the Status Register
  Status = NorFlashReadStatusRegister( TargetAddress );

  return Status;
}

EFI_STATUS
NorFlashWriteSingleBlock (
  IN  UINTN     DeviceBaseAddress,
  IN  EFI_LBA   Lba,
  IN  UINT32    *DataBuffer,
  IN  UINT32    BlockSizeInWords
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN         WordAddress;
  UINT32        WordIndex;
  UINTN         BufferIndex;
  UINTN         BlockAddress;
  UINTN         BuffersInBlock;
  UINTN         RemainingWords;

  // Get the physical address of the block
  BlockAddress = GET_NOR_BLOCK_ADDRESS(DeviceBaseAddress, Lba, BlockSizeInWords * 4);

  Status = NorFlashUnlockAndEraseSingleBlock( BlockAddress );
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "WriteSingleBlock: ERROR - Failed to Unlock and Erase the single block at 0x%X\n", BlockAddress));
    return Status;
  }

  // To speed up the programming operation, NOR Flash is programmed using the Buffered Programming method.

  // Start writing from the first address at the start of the block
  WordAddress = BlockAddress;

  // Check that the address starts at a 32-word boundary, i.e. last 7 bits must be zero
  if ((WordAddress & BOUNDARY_OF_32_WORDS) == 0x00) {

    // First, break the entire block into buffer-sized chunks.
    BuffersInBlock = (UINTN)BlockSizeInWords / P30_MAX_BUFFER_SIZE_IN_BYTES;

    // Then feed each buffer chunk to the NOR Flash
    for( BufferIndex=0;
         BufferIndex < BuffersInBlock;
         BufferIndex++, WordAddress += P30_MAX_BUFFER_SIZE_IN_BYTES, DataBuffer += P30_MAX_BUFFER_SIZE_IN_WORDS
       ) {
      Status = NorFlashWriteBuffer ( WordAddress, P30_MAX_BUFFER_SIZE_IN_BYTES, DataBuffer );
      if (EFI_ERROR(Status)) {
        goto EXIT;
      }
    }

    // Finally, finish off any remaining words that are less than the maximum size of the buffer
    RemainingWords = BlockSizeInWords % P30_MAX_BUFFER_SIZE_IN_WORDS;

    if( RemainingWords != 0) {
      Status = NorFlashWriteBuffer ( WordAddress, (RemainingWords * 4), DataBuffer );
      if (EFI_ERROR(Status)) {
        goto EXIT;
      }
    }

  } else {
    // For now, use the single word programming algorithm
    // It is unlikely that the NOR Flash will exist in an address which falls within a 32 word boundary range,
    // i.e. which ends in the range 0x......01 - 0x......7F.
    for( WordIndex=0; WordIndex<BlockSizeInWords; WordIndex++, DataBuffer++, WordAddress = WordAddress + 4 ) {
      Status = NorFlashWriteSingleWord( WordAddress, *DataBuffer );
      if (EFI_ERROR(Status)) {
        goto EXIT;
      }
    }
  }

  EXIT:
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "NOR FLASH Programming [WriteSingleBlock] failed at address 0x%08x. Exit Status = \"%r\".\n", WordAddress, Status));
  }
  return Status;
}


EFI_STATUS
NorFlashWriteBlocks (
  IN  NOR_FLASH_INSTANCE *Instance,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSizeInBytes,
  IN VOID                   *Buffer
  )
{
  UINT32          *pWriteBuffer;
  EFI_STATUS      Status = EFI_SUCCESS;
  EFI_LBA         CurrentBlock;
  UINT32          BlockSizeInWords;
  UINT32          NumBlocks;
  UINT32          BlockCount;

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if( Instance->Media.ReadOnly == TRUE ) {
    return EFI_WRITE_PROTECTED;
  }

  // We must have some bytes to read
  DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: BufferSizeInBytes=0x%x\n", BufferSizeInBytes));
  if( BufferSizeInBytes == 0 ) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: BlockSize in bytes =0x%x\n", Instance->Media.BlockSize ));
  if ((BufferSizeInBytes % Instance->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->Media.BlockSize ;

  DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld.\n", NumBlocks, Instance->Media.LastBlock, Lba));

  if ( ( Lba + NumBlocks ) > ( Instance->Media.LastBlock + 1 ) ) {
    DEBUG((EFI_D_ERROR, "NorFlashWriteBlocks: ERROR - Write will exceed last block.\n"));
    return EFI_INVALID_PARAMETER;
  }

  BlockSizeInWords = Instance->Media.BlockSize / 4;

  // Because the target *Buffer is a pointer to VOID, we must put all the data into a pointer
  // to a proper data type, so use *ReadBuffer
  pWriteBuffer = (UINT32 *)Buffer;

  CurrentBlock = Lba;
  for( BlockCount=0; BlockCount<NumBlocks; BlockCount++, CurrentBlock++, pWriteBuffer = pWriteBuffer + BlockSizeInWords ) {

    DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: Writing block #%d\n", (UINTN)CurrentBlock ));

    Status = NorFlashWriteSingleBlock( Instance->BaseAddress, CurrentBlock, pWriteBuffer, BlockSizeInWords );

    if (EFI_ERROR(Status)) {
      break;
    }

  }

  DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: Exit Status = \"%r\".\n", Status));
  return Status;
}


EFI_STATUS
NorFlashReadBlocks (
  IN NOR_FLASH_INSTANCE   *Instance,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSizeInBytes,
  OUT VOID                  *Buffer
  )
{
  UINT32              NumBlocks;
  UINTN               StartAddress;

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We must have some bytes to read
  DEBUG((DEBUG_BLKIO, "NorFlashReadBlocks: BufferSize=0x%x bytes.\n", BufferSizeInBytes));
  if( BufferSizeInBytes == 0 ) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG((DEBUG_BLKIO, "NorFlashReadBlocks: BlockSize=0x%x bytes.\n", Instance->Media.BlockSize ));
  if ((BufferSizeInBytes % Instance->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->Media.BlockSize ;

  DEBUG((DEBUG_BLKIO, "NorFlashReadBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld\n", NumBlocks, Instance->Media.LastBlock, Lba));

  if ( ( Lba + NumBlocks ) > (Instance->Media.LastBlock + 1) ) {
    DEBUG((EFI_D_ERROR, "NorFlashReadBlocks: ERROR - Read will exceed last block\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (Instance->BaseAddress,
                                        Lba,
                                        Instance->Media.BlockSize
                                        );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (Instance->BaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  CopyMem(Buffer, (UINTN *)StartAddress, BufferSizeInBytes);

  return EFI_SUCCESS;
}


EFI_STATUS
NorFlashReset (
  IN  NOR_FLASH_INSTANCE *Instance
  )
{
  // As there is no specific RESET to perform, ensure that the devices is in the default Read Array mode
  SEND_NOR_COMMAND( Instance->BaseAddress, 0, P30_CMD_READ_ARRAY);
  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
NorFlashInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  NOR_FLASH_DESCRIPTION*  NorFlashDevices;
  UINT32                  NorFlashDeviceCount;
  BOOLEAN                 ContainVariableStorage;

  Status = NorFlashPlatformInitialization ();
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to initialize Nor Flash devices\n"));
    return Status;
  }

  Status = NorFlashPlatformGetDevices (&NorFlashDevices,&NorFlashDeviceCount);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to get Nor Flash devices\n"));
    return Status;
  }

  mNorFlashInstances = AllocatePool(sizeof(NOR_FLASH_INSTANCE*) * NorFlashDeviceCount);

  for (Index = 0; Index < NorFlashDeviceCount; Index++) {
    // Check if this NOR Flash device contain the variable storage region
    ContainVariableStorage =
        (NorFlashDevices[Index].BaseAddress <= PcdGet32 (PcdFlashNvStorageVariableBase)) &&
        (PcdGet32 (PcdFlashNvStorageVariableBase) + PcdGet32 (PcdFlashNvStorageVariableSize) <= NorFlashDevices[Index].BaseAddress + NorFlashDevices[Index].Size);

    Status = NorFlashCreateInstance (
      NorFlashDevices[Index].BaseAddress,
      NorFlashDevices[Index].Size,
      Index,
      NorFlashDevices[Index].BlockSize,
      ContainVariableStorage,
      &NorFlashDevices[Index].Guid,
      &mNorFlashInstances[Index]
    );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to create instance for NorFlash[%d]\n",Index));
    }
  }

  return Status;
}
