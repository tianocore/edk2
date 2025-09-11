/** @file  VirtNorFlashDeviceLib.c

  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/VirtNorFlashDeviceLib.h>

STATIC
UINT32
NorFlashReadStatusRegister (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  SR_Address
  )
{
  // Prepare to read the status register
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_STATUS_REGISTER);
  return MmioRead32 (DeviceBaseAddress);
}

STATIC
BOOLEAN
NorFlashBlockIsLocked (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  )
{
  UINT32  LockStatus;

  // Send command for reading device id
  SEND_NOR_COMMAND (BlockAddress, 2, P30_CMD_READ_DEVICE_ID);

  // Read block lock status
  LockStatus = MmioRead32 (CREATE_NOR_ADDRESS (BlockAddress, 2));

  // Decode block lock status
  LockStatus = FOLD_32BIT_INTO_16BIT (LockStatus);

  if ((LockStatus & 0x2) != 0) {
    DEBUG ((DEBUG_ERROR, "NorFlashBlockIsLocked: WARNING: Block LOCKED DOWN\n"));
  }

  return ((LockStatus & 0x1) != 0);
}

STATIC
EFI_STATUS
NorFlashUnlockSingleBlock (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  )
{
  UINT32  LockStatus;

  // Raise the Task Priority Level to TPL_NOTIFY to serialise all its operations
  // and to protect shared data structures.

  // Request a lock setup
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_LOCK_BLOCK_SETUP);

  // Request an unlock
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_UNLOCK_BLOCK);

  // Wait until the status register gives us the all clear
  do {
    LockStatus = NorFlashReadStatusRegister (DeviceBaseAddress, BlockAddress);
  } while ((LockStatus & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);

  // Put device back into Read Array mode
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_READ_ARRAY);

  DEBUG ((DEBUG_BLKIO, "UnlockSingleBlock: BlockAddress=0x%08x\n", BlockAddress));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NorFlashUnlockSingleBlockIfNecessary (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (NorFlashBlockIsLocked (DeviceBaseAddress, BlockAddress)) {
    Status = NorFlashUnlockSingleBlock (DeviceBaseAddress, BlockAddress);
  }

  return Status;
}

/**
 * The following function presumes that the block has already been unlocked.
 **/
EFI_STATUS
EFIAPI
NorFlashEraseSingleBlock (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  )
{
  EFI_STATUS  Status;
  UINT32      StatusRegister;

  Status = EFI_SUCCESS;

  // Request a block erase and then confirm it
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_BLOCK_ERASE_SETUP);
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_BLOCK_ERASE_CONFIRM);

  // Wait until the status register gives us the all clear
  do {
    StatusRegister = NorFlashReadStatusRegister (DeviceBaseAddress, BlockAddress);
  } while ((StatusRegister & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);

  if (StatusRegister & P30_SR_BIT_VPP) {
    DEBUG ((DEBUG_ERROR, "EraseSingleBlock(BlockAddress=0x%08x: VPP Range Error\n", BlockAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if ((StatusRegister & (P30_SR_BIT_ERASE | P30_SR_BIT_PROGRAM)) == (P30_SR_BIT_ERASE | P30_SR_BIT_PROGRAM)) {
    DEBUG ((DEBUG_ERROR, "EraseSingleBlock(BlockAddress=0x%08x: Command Sequence Error\n", BlockAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_ERASE) {
    DEBUG ((DEBUG_ERROR, "EraseSingleBlock(BlockAddress=0x%08x: Block Erase Error StatusRegister:0x%X\n", BlockAddress, StatusRegister));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_BLOCK_LOCKED) {
    // The debug level message has been reduced because a device lock might happen. In this case we just retry it ...
    DEBUG ((DEBUG_INFO, "EraseSingleBlock(BlockAddress=0x%08x: Block Locked Error\n", BlockAddress));
    Status = EFI_WRITE_PROTECTED;
  }

  if (EFI_ERROR (Status)) {
    // Clear the Status Register
    SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

  // Put device back into Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashWriteSingleWord (
  IN  UINTN   DeviceBaseAddress,
  IN  UINTN   WordAddress,
  IN  UINT32  WriteData
  )
{
  EFI_STATUS  Status;
  UINT32      StatusRegister;

  Status = EFI_SUCCESS;

  // Request a write single word command
  SEND_NOR_COMMAND (WordAddress, 0, P30_CMD_WORD_PROGRAM_SETUP);

  // Store the word into NOR Flash;
  MmioWrite32 (WordAddress, WriteData);

  // Wait for the write to complete and then check for any errors; i.e. check the Status Register
  do {
    // Prepare to read the status register
    StatusRegister = NorFlashReadStatusRegister (DeviceBaseAddress, WordAddress);
    // The chip is busy while the WRITE bit is not asserted
  } while ((StatusRegister & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);

  // Perform a full status check:
  // Mask the relevant bits of Status Register.
  // Everything should be zero, if not, we have a problem

  if (StatusRegister & P30_SR_BIT_VPP) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleWord(WordAddress:0x%X): VPP Range Error\n", WordAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_PROGRAM) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleWord(WordAddress:0x%X): Program Error\n", WordAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_BLOCK_LOCKED) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleWord(WordAddress:0x%X): Device Protect Error\n", WordAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (!EFI_ERROR (Status)) {
    // Clear the Status Register
    SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

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
EFIAPI
NorFlashWriteBuffer (
  IN  UINTN   DeviceBaseAddress,
  IN  UINTN   TargetAddress,
  IN  UINTN   BufferSizeInBytes,
  IN  UINT32  *Buffer
  )
{
  EFI_STATUS       Status;
  UINTN            BufferSizeInWords;
  UINTN            Count;
  volatile UINT32  *Data;
  UINTN            WaitForBuffer;
  BOOLEAN          BufferAvailable;
  UINT32           StatusRegister;

  WaitForBuffer   = MAX_BUFFERED_PROG_ITERATIONS;
  BufferAvailable = FALSE;

  // Check that the target address does not cross a 32-word boundary.
  if ((TargetAddress & BOUNDARY_OF_32_WORDS) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Check there are some data to program
  if (BufferSizeInBytes == 0) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Check that the buffer size does not exceed the maximum hardware buffer size on chip.
  if (BufferSizeInBytes > P30_MAX_BUFFER_SIZE_IN_BYTES) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Check that the buffer size is a multiple of 32-bit words
  if ((BufferSizeInBytes % 4) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Pre-programming conditions checked, now start the algorithm.

  // Prepare the data destination address
  Data = (UINT32 *)TargetAddress;

  // Check the availability of the buffer
  do {
    // Issue the Buffered Program Setup command
    SEND_NOR_COMMAND (TargetAddress, 0, P30_CMD_BUFFERED_PROGRAM_SETUP);

    // Read back the status register bit#7 from the same address
    if (((*Data) & P30_SR_BIT_WRITE) == P30_SR_BIT_WRITE) {
      BufferAvailable = TRUE;
    }

    // Update the loop counter
    WaitForBuffer--;
  } while ((WaitForBuffer > 0) && (BufferAvailable == FALSE));

  // The buffer was not available for writing
  if (WaitForBuffer == 0) {
    return EFI_DEVICE_ERROR;
  }

  // From now on we work in 32-bit words
  BufferSizeInWords = BufferSizeInBytes / (UINTN)4;

  // Write the word count, which is (buffer_size_in_words - 1),
  // because word count 0 means one word.
  SEND_NOR_COMMAND (TargetAddress, 0, (BufferSizeInWords - 1));

  // Write the data to the NOR Flash, advancing each address by 4 bytes
  for (Count = 0; Count < BufferSizeInWords; Count++, Data++, Buffer++) {
    MmioWrite32 ((UINTN)Data, *Buffer);
  }

  // Issue the Buffered Program Confirm command, to start the programming operation
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_BUFFERED_PROGRAM_CONFIRM);

  // Wait for the write to complete and then check for any errors; i.e. check the Status Register
  do {
    StatusRegister = NorFlashReadStatusRegister (DeviceBaseAddress, TargetAddress);
    // The chip is busy while the WRITE bit is not asserted
  } while ((StatusRegister & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);

  // Perform a full status check:
  // Mask the relevant bits of Status Register.
  // Everything should be zero, if not, we have a problem

  Status = EFI_SUCCESS;

  if (StatusRegister & P30_SR_BIT_VPP) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBuffer(TargetAddress:0x%X): VPP Range Error\n", TargetAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_PROGRAM) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBuffer(TargetAddress:0x%X): Program Error\n", TargetAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (StatusRegister & P30_SR_BIT_BLOCK_LOCKED) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBuffer(TargetAddress:0x%X): Device Protect Error\n", TargetAddress));
    Status = EFI_DEVICE_ERROR;
  }

  if (!EFI_ERROR (Status)) {
    // Clear the Status Register
    SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

  return Status;
}

/**
 * This function unlock and erase an entire NOR Flash block.
 **/
EFI_STATUS
NorFlashUnlockAndEraseSingleBlock (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Index = 0;
  // The block erase might fail a first time (SW bug ?). Retry it ...
  do {
    // Unlock the block if we have to
    Status = NorFlashUnlockSingleBlockIfNecessary (DeviceBaseAddress, BlockAddress);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = NorFlashEraseSingleBlock (DeviceBaseAddress, BlockAddress);
    Index++;
  } while ((Index < NOR_FLASH_ERASE_RETRY) && (Status == EFI_WRITE_PROTECTED));

  if (Index == NOR_FLASH_ERASE_RETRY) {
    DEBUG ((DEBUG_ERROR, "EraseSingleBlock(BlockAddress=0x%08x: Block Locked Error (try to erase %d times)\n", BlockAddress, Index));
  }

  return Status;
}

EFI_STATUS
NorFlashWriteFullBlock (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  UINT32   *DataBuffer,
  IN  UINT32   BlockSizeInWords
  )
{
  EFI_STATUS  Status;
  UINTN       WordAddress;
  UINT32      WordIndex;
  UINTN       BufferIndex;
  UINTN       BlockAddress;
  UINTN       BuffersInBlock;
  UINTN       RemainingWords;
  UINTN       Cnt;

  Status = EFI_SUCCESS;

  // Get the physical address of the block
  BlockAddress = GET_NOR_BLOCK_ADDRESS (RegionBaseAddress, Lba, BlockSizeInWords * 4);

  // Start writing from the first address at the start of the block
  WordAddress = BlockAddress;
  Status      = NorFlashUnlockAndEraseSingleBlock (DeviceBaseAddress, BlockAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "WriteSingleBlock: ERROR - Failed to Unlock and Erase the single block at 0x%X\n", BlockAddress));
    goto EXIT;
  }

  // To speed up the programming operation, NOR Flash is programmed using the Buffered Programming method.

  // Check that the address starts at a 32-word boundary, i.e. last 7 bits must be zero
  if ((WordAddress & BOUNDARY_OF_32_WORDS) == 0x00) {
    // First, break the entire block into buffer-sized chunks.
    BuffersInBlock = (UINTN)(BlockSizeInWords * 4) / P30_MAX_BUFFER_SIZE_IN_BYTES;

    // Then feed each buffer chunk to the NOR Flash
    // If a buffer does not contain any data, don't write it.
    for (BufferIndex = 0;
         BufferIndex < BuffersInBlock;
         BufferIndex++, WordAddress += P30_MAX_BUFFER_SIZE_IN_BYTES, DataBuffer += P30_MAX_BUFFER_SIZE_IN_WORDS
         )
    {
      // Check the buffer to see if it contains any data (not set all 1s).
      for (Cnt = 0; Cnt < P30_MAX_BUFFER_SIZE_IN_WORDS; Cnt++) {
        if (~DataBuffer[Cnt] != 0 ) {
          // Some data found, write the buffer.
          Status = NorFlashWriteBuffer (
                     DeviceBaseAddress,
                     WordAddress,
                     P30_MAX_BUFFER_SIZE_IN_BYTES,
                     DataBuffer
                     );
          if (EFI_ERROR (Status)) {
            goto EXIT;
          }

          break;
        }
      }
    }

    // Finally, finish off any remaining words that are less than the maximum size of the buffer
    RemainingWords = BlockSizeInWords % P30_MAX_BUFFER_SIZE_IN_WORDS;

    if (RemainingWords != 0) {
      Status = NorFlashWriteBuffer (DeviceBaseAddress, WordAddress, (RemainingWords * 4), DataBuffer);
      if (EFI_ERROR (Status)) {
        goto EXIT;
      }
    }
  } else {
    // For now, use the single word programming algorithm
    // It is unlikely that the NOR Flash will exist in an address which falls within a 32 word boundary range,
    // i.e. which ends in the range 0x......01 - 0x......7F.
    for (WordIndex = 0; WordIndex < BlockSizeInWords; WordIndex++, DataBuffer++, WordAddress = WordAddress + 4) {
      Status = NorFlashWriteSingleWord (DeviceBaseAddress, WordAddress, *DataBuffer);
      if (EFI_ERROR (Status)) {
        goto EXIT;
      }
    }
  }

EXIT:
  // Put device back into Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NOR FLASH Programming [WriteSingleBlock] failed at address 0x%08x. Exit Status = \"%r\".\n", WordAddress, Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashWriteBlocks (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  EFI_LBA  LastBlock,
  IN  UINT32   BlockSize,
  IN  UINTN    BufferSizeInBytes,
  IN  VOID     *Buffer
  )
{
  UINT32      *pWriteBuffer;
  EFI_STATUS  Status;
  EFI_LBA     CurrentBlock;
  UINT32      BlockSizeInWords;
  UINT32      NumBlocks;
  UINT32      BlockCount;

  Status = EFI_SUCCESS;

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We must have some bytes to read
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BufferSizeInBytes=0x%x\n", BufferSizeInBytes));
  if (BufferSizeInBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BlockSize in bytes =0x%x\n", BlockSize));
  if ((BufferSizeInBytes % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / BlockSize;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld.\n", NumBlocks, LastBlock, Lba));

  if ((Lba + NumBlocks) > (LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBlocks: ERROR - Write will exceed last block.\n"));
    return EFI_INVALID_PARAMETER;
  }

  BlockSizeInWords = BlockSize / 4;

  // Because the target *Buffer is a pointer to VOID, we must put all the data into a pointer
  // to a proper data type, so use *ReadBuffer
  pWriteBuffer = (UINT32 *)Buffer;

  CurrentBlock = Lba;
  for (BlockCount = 0; BlockCount < NumBlocks; BlockCount++, CurrentBlock++, pWriteBuffer = pWriteBuffer + BlockSizeInWords) {
    DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: Writing block #%d\n", (UINTN)CurrentBlock));

    Status = NorFlashWriteFullBlock (
               DeviceBaseAddress,
               RegionBaseAddress,
               CurrentBlock,
               pWriteBuffer,
               BlockSizeInWords
               );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: Exit Status = \"%r\".\n", Status));
  return Status;
}

EFI_STATUS
EFIAPI
NorFlashReadBlocks (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  EFI_LBA  LastBlock,
  IN  UINT32   BlockSize,
  IN  UINTN    BufferSizeInBytes,
  OUT  VOID    *Buffer
  )
{
  UINT32  NumBlocks;
  UINTN   StartAddress;

  DEBUG ((
    DEBUG_BLKIO,
    "NorFlashReadBlocks: BufferSize=0x%xB BlockSize=0x%xB LastBlock=%ld, Lba=%ld.\n",
    BufferSizeInBytes,
    BlockSize,
    LastBlock,
    Lba
    ));

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Return if we have not any byte to read
  if (BufferSizeInBytes == 0) {
    return EFI_SUCCESS;
  }

  // The size of the buffer must be a multiple of the block size
  if ((BufferSizeInBytes % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / BlockSize;

  if ((Lba + NumBlocks) > (LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashReadBlocks: ERROR - Read will exceed last block\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   RegionBaseAddress,
                   Lba,
                   BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  CopyMem (Buffer, (VOID *)StartAddress, BufferSizeInBytes);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NorFlashRead (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  UINT32   BlockSize,
  IN  UINTN    Size,
  IN  UINTN    Offset,
  IN  UINTN    BufferSizeInBytes,
  OUT  VOID    *Buffer
  )
{
  UINTN  StartAddress;

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Return if we have not any byte to read
  if (BufferSizeInBytes == 0) {
    return EFI_SUCCESS;
  }

  if (((Lba * BlockSize) + Offset + BufferSizeInBytes) > Size) {
    DEBUG ((DEBUG_ERROR, "NorFlashRead: ERROR - Read will exceed device size.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   RegionBaseAddress,
                   Lba,
                   BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  CopyMem (Buffer, (VOID *)(StartAddress + Offset), BufferSizeInBytes);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
NorFlashWriteSingleBlockWithErase (
  IN  UINTN      DeviceBaseAddress,
  IN  UINTN      RegionBaseAddress,
  IN  EFI_LBA    Lba,
  IN  UINT32     LastBlock,
  IN  UINT32     BlockSize,
  IN  UINTN      Offset,
  IN OUT  UINTN  *NumBytes,
  IN  UINT8      *Buffer,
  IN  VOID       *ShadowBuffer
  )
{
  EFI_STATUS  Status;

  // Read NOR Flash data into shadow buffer
  Status = NorFlashReadBlocks (
             DeviceBaseAddress,
             RegionBaseAddress,
             Lba,
             LastBlock,
             BlockSize,
             BlockSize,
             ShadowBuffer
             );
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  // Put the data at the appropriate location inside the buffer area
  CopyMem ((VOID *)((UINTN)ShadowBuffer + Offset), Buffer, *NumBytes);

  // Write the modified buffer back to the NorFlash
  Status = NorFlashWriteBlocks (
             DeviceBaseAddress,
             RegionBaseAddress,
             Lba,
             LastBlock,
             BlockSize,
             BlockSize,
             ShadowBuffer
             );
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/*
  Write a full or portion of a block. It must not span block boundaries; that is,
  Offset + *NumBytes <= Instance->BlockSize.
*/
EFI_STATUS
EFIAPI
NorFlashWriteSingleBlock (
  IN  UINTN      DeviceBaseAddress,
  IN  UINTN      RegionBaseAddress,
  IN  EFI_LBA    Lba,
  IN  UINT32     LastBlock,
  IN  UINT32     BlockSize,
  IN  UINTN      Size,
  IN  UINTN      Offset,
  IN OUT  UINTN  *NumBytes,
  IN  UINT8      *Buffer,
  IN  VOID       *ShadowBuffer
  )
{
  EFI_STATUS  Status;
  UINTN       CurOffset;
  UINTN       BlockAddress;
  UINT8       *OrigData;
  UINTN       Start, End;
  UINT32      Index, Count;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteSingleBlock(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n", Lba, Offset, *NumBytes, Buffer));

  // Check we did get some memory. Buffer is BlockSize.
  if (ShadowBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "FvbWrite: ERROR - Buffer not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // The write must not span block boundaries.
  // We need to check each variable individually because adding two large values together overflows.
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize))
  {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleBlock: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n", Offset, *NumBytes, BlockSize));
    return EFI_BAD_BUFFER_SIZE;
  }

  // We must have some bytes to write
  if (*NumBytes == 0) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleBlock: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n", Offset, *NumBytes, BlockSize));
    return EFI_BAD_BUFFER_SIZE;
  }

  // Pick 4 * P30_MAX_BUFFER_SIZE_IN_BYTES (== 512 bytes) as a good
  // start for word operations as opposed to erasing the block and
  // writing the data regardless if an erase is really needed.
  //
  // Many NV variable updates are small enough for a a single
  // P30_MAX_BUFFER_SIZE_IN_BYTES block write.  In case the update is
  // larger than a single block, or the update crosses a
  // P30_MAX_BUFFER_SIZE_IN_BYTES boundary (as shown in the diagram
  // below), or both, we might have to write two or more blocks.
  //
  //    0               128              256
  //    [----------------|----------------]
  //    ^         ^             ^         ^
  //    |         |             |         |
  //    |         |             |        End, the next "word" boundary beyond
  //    |         |             |        the (logical) update
  //    |         |             |
  //    |         |     (Offset & BOUNDARY_OF_32_WORDS) + NumBytes;
  //    |         |     i.e., the relative offset inside (or just past)
  //    |         |     the *double-word* such that it is the
  //    |         |     *exclusive* end of the (logical) update.
  //    |         |
  //    |         Offset & BOUNDARY_OF_32_WORDS; i.e., Offset within the "word";
  //    |         this is where the (logical) update is supposed to start
  //    |
  //    Start = Offset & ~BOUNDARY_OF_32_WORDS; i.e., Offset truncated to "word" boundary

  Start = Offset & ~BOUNDARY_OF_32_WORDS;
  End   = ALIGN_VALUE (Offset + *NumBytes, P30_MAX_BUFFER_SIZE_IN_BYTES);

  if ((End - Start) <= (4 * P30_MAX_BUFFER_SIZE_IN_BYTES)) {
    // Check to see if we need to erase before programming the data into NOR.
    // If the destination bits are only changing from 1s to 0s we can just write.
    // After a block is erased all bits in the block is set to 1.
    // If any byte requires us to erase we just give up and rewrite all of it.

    // Read the old version of the data into the shadow buffer
    Status = NorFlashRead (
               DeviceBaseAddress,
               RegionBaseAddress,
               Lba,
               BlockSize,
               Size,
               Start,
               End - Start,
               ShadowBuffer
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    // Make OrigData point to the start of the old version of the data inside
    // the word aligned buffer
    OrigData = ShadowBuffer + (Offset & BOUNDARY_OF_32_WORDS);

    // Update the buffer containing the old version of the data with the new
    // contents, while checking whether the old version had any bits cleared
    // that we want to set. In that case, we will need to erase the block first.
    for (CurOffset = 0; CurOffset < *NumBytes; CurOffset++) {
      if (~(UINT32)OrigData[CurOffset] & (UINT32)Buffer[CurOffset]) {
        Status = NorFlashWriteSingleBlockWithErase (
                   DeviceBaseAddress,
                   RegionBaseAddress,
                   Lba,
                   LastBlock,
                   BlockSize,
                   Offset,
                   NumBytes,
                   Buffer,
                   ShadowBuffer
                   );
        return Status;
      }

      OrigData[CurOffset] = Buffer[CurOffset];
    }

    //
    // Write the updated buffer to NOR.
    //
    BlockAddress = GET_NOR_BLOCK_ADDRESS (RegionBaseAddress, Lba, BlockSize);

    // Unlock the block if we have to
    Status = NorFlashUnlockSingleBlockIfNecessary (DeviceBaseAddress, BlockAddress);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Count = (End - Start) / P30_MAX_BUFFER_SIZE_IN_BYTES;
    for (Index = 0; Index < Count; Index++) {
      Status = NorFlashWriteBuffer (
                 DeviceBaseAddress,
                 BlockAddress + Start + Index * P30_MAX_BUFFER_SIZE_IN_BYTES,
                 P30_MAX_BUFFER_SIZE_IN_BYTES,
                 ShadowBuffer + Index * P30_MAX_BUFFER_SIZE_IN_BYTES
                 );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
    }
  } else {
    Status = NorFlashWriteSingleBlockWithErase (
               DeviceBaseAddress,
               RegionBaseAddress,
               Lba,
               LastBlock,
               BlockSize,
               Offset,
               NumBytes,
               Buffer,
               ShadowBuffer
               );
    return Status;
  }

Exit:
  // Put device back into Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashReset (
  IN  UINTN  DeviceBaseAddress
  )
{
  // As there is no specific RESET to perform, ensure that the devices is in the default Read Array mode
  SEND_NOR_COMMAND (DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);
  return EFI_SUCCESS;
}
