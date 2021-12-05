/** @file  NorFlash.c

  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>

#include "NorFlash.h"

//
// Global variable declarations
//
extern NOR_FLASH_INSTANCE  **mNorFlashInstances;
extern UINT32              mNorFlashDeviceCount;

UINT32
NorFlashReadStatusRegister (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               SR_Address
  )
{
  // Prepare to read the status register
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_STATUS_REGISTER);
  return MmioRead32 (Instance->DeviceBaseAddress);
}

STATIC
BOOLEAN
NorFlashBlockIsLocked (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
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
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  UINT32  LockStatus;

  // Raise the Task Priority Level to TPL_NOTIFY to serialise all its operations
  // and to protect shared data structures.

  if (FeaturePcdGet (PcdNorFlashCheckBlockLocked) == TRUE) {
    do {
      // Request a lock setup
      SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_LOCK_BLOCK_SETUP);

      // Request an unlock
      SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_UNLOCK_BLOCK);

      // Send command for reading device id
      SEND_NOR_COMMAND (BlockAddress, 2, P30_CMD_READ_DEVICE_ID);

      // Read block lock status
      LockStatus = MmioRead32 (CREATE_NOR_ADDRESS (BlockAddress, 2));

      // Decode block lock status
      LockStatus = FOLD_32BIT_INTO_16BIT (LockStatus);
    } while ((LockStatus & 0x1) == 1);
  } else {
    // Request a lock setup
    SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_LOCK_BLOCK_SETUP);

    // Request an unlock
    SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_UNLOCK_BLOCK);

    // Wait until the status register gives us the all clear
    do {
      LockStatus = NorFlashReadStatusRegister (Instance, BlockAddress);
    } while ((LockStatus & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);
  }

  // Put device back into Read Array mode
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_READ_ARRAY);

  DEBUG ((DEBUG_BLKIO, "UnlockSingleBlock: BlockAddress=0x%08x\n", BlockAddress));

  return EFI_SUCCESS;
}

EFI_STATUS
NorFlashUnlockSingleBlockIfNecessary (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (NorFlashBlockIsLocked (Instance, BlockAddress)) {
    Status = NorFlashUnlockSingleBlock (Instance, BlockAddress);
  }

  return Status;
}

/**
 * The following function presumes that the block has already been unlocked.
 **/
EFI_STATUS
NorFlashEraseSingleBlock (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
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
    StatusRegister = NorFlashReadStatusRegister (Instance, BlockAddress);
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
    SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

  // Put device back into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  return Status;
}

EFI_STATUS
NorFlashWriteSingleWord (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               WordAddress,
  IN UINT32              WriteData
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
    StatusRegister = NorFlashReadStatusRegister (Instance, WordAddress);
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
    SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

  // Put device back into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

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
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               TargetAddress,
  IN UINTN               BufferSizeInBytes,
  IN UINT32              *Buffer
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
    Status = EFI_DEVICE_ERROR;
    goto EXIT;
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
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_BUFFERED_PROGRAM_CONFIRM);

  // Wait for the write to complete and then check for any errors; i.e. check the Status Register
  do {
    StatusRegister = NorFlashReadStatusRegister (Instance, TargetAddress);
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
    SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_CLEAR_STATUS_REGISTER);
  }

EXIT:
  // Put device back into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  return Status;
}

EFI_STATUS
NorFlashWriteBlocks (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               BufferSizeInBytes,
  IN VOID                *Buffer
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

  if (Instance->Media.ReadOnly == TRUE) {
    return EFI_WRITE_PROTECTED;
  }

  // We must have some bytes to read
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BufferSizeInBytes=0x%x\n", BufferSizeInBytes));
  if (BufferSizeInBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BlockSize in bytes =0x%x\n", Instance->Media.BlockSize));
  if ((BufferSizeInBytes % Instance->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->Media.BlockSize;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld.\n", NumBlocks, Instance->Media.LastBlock, Lba));

  if ((Lba + NumBlocks) > (Instance->Media.LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBlocks: ERROR - Write will exceed last block.\n"));
    return EFI_INVALID_PARAMETER;
  }

  BlockSizeInWords = Instance->Media.BlockSize / 4;

  // Because the target *Buffer is a pointer to VOID, we must put all the data into a pointer
  // to a proper data type, so use *ReadBuffer
  pWriteBuffer = (UINT32 *)Buffer;

  CurrentBlock = Lba;
  for (BlockCount = 0; BlockCount < NumBlocks; BlockCount++, CurrentBlock++, pWriteBuffer = pWriteBuffer + BlockSizeInWords) {
    DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: Writing block #%d\n", (UINTN)CurrentBlock));

    Status = NorFlashWriteFullBlock (Instance, CurrentBlock, pWriteBuffer, BlockSizeInWords);

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: Exit Status = \"%r\".\n", Status));
  return Status;
}

#define BOTH_ALIGNED(a, b, align)  ((((UINTN)(a) | (UINTN)(b)) & ((align) - 1)) == 0)

/**
  Copy Length bytes from Source to Destination, using aligned accesses only.
  Note that this implementation uses memcpy() semantics rather then memmove()
  semantics, i.e., SourceBuffer and DestinationBuffer should not overlap.

  @param  DestinationBuffer The target of the copy request.
  @param  SourceBuffer      The place to copy from.
  @param  Length            The number of bytes to copy.

  @return Destination

**/
STATIC
VOID *
AlignedCopyMem (
  OUT     VOID        *DestinationBuffer,
  IN      CONST VOID  *SourceBuffer,
  IN      UINTN       Length
  )
{
  UINT8         *Destination8;
  CONST UINT8   *Source8;
  UINT32        *Destination32;
  CONST UINT32  *Source32;
  UINT64        *Destination64;
  CONST UINT64  *Source64;

  if (BOTH_ALIGNED (DestinationBuffer, SourceBuffer, 8) && (Length >= 8)) {
    Destination64 = DestinationBuffer;
    Source64      = SourceBuffer;
    while (Length >= 8) {
      *Destination64++ = *Source64++;
      Length          -= 8;
    }

    Destination8 = (UINT8 *)Destination64;
    Source8      = (CONST UINT8 *)Source64;
  } else if (BOTH_ALIGNED (DestinationBuffer, SourceBuffer, 4) && (Length >= 4)) {
    Destination32 = DestinationBuffer;
    Source32      = SourceBuffer;
    while (Length >= 4) {
      *Destination32++ = *Source32++;
      Length          -= 4;
    }

    Destination8 = (UINT8 *)Destination32;
    Source8      = (CONST UINT8 *)Source32;
  } else {
    Destination8 = DestinationBuffer;
    Source8      = SourceBuffer;
  }

  while (Length-- != 0) {
    *Destination8++ = *Source8++;
  }

  return DestinationBuffer;
}

EFI_STATUS
NorFlashReadBlocks (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               BufferSizeInBytes,
  OUT VOID               *Buffer
  )
{
  UINT32  NumBlocks;
  UINTN   StartAddress;

  DEBUG ((
    DEBUG_BLKIO,
    "NorFlashReadBlocks: BufferSize=0x%xB BlockSize=0x%xB LastBlock=%ld, Lba=%ld.\n",
    BufferSizeInBytes,
    Instance->Media.BlockSize,
    Instance->Media.LastBlock,
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
  if ((BufferSizeInBytes % Instance->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->Media.BlockSize;

  if ((Lba + NumBlocks) > (Instance->Media.LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashReadBlocks: ERROR - Read will exceed last block\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->Media.BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  AlignedCopyMem (Buffer, (VOID *)StartAddress, BufferSizeInBytes);

  return EFI_SUCCESS;
}

EFI_STATUS
NorFlashRead (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               Offset,
  IN UINTN               BufferSizeInBytes,
  OUT VOID               *Buffer
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

  if (((Lba * Instance->Media.BlockSize) + Offset + BufferSizeInBytes) > Instance->Size) {
    DEBUG ((DEBUG_ERROR, "NorFlashRead: ERROR - Read will exceed device size.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->Media.BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  AlignedCopyMem (Buffer, (VOID *)(StartAddress + Offset), BufferSizeInBytes);

  return EFI_SUCCESS;
}

/*
  Write a full or portion of a block. It must not span block boundaries; that is,
  Offset + *NumBytes <= Instance->Media.BlockSize.
*/
EFI_STATUS
NorFlashWriteSingleBlock (
  IN        NOR_FLASH_INSTANCE  *Instance,
  IN        EFI_LBA             Lba,
  IN        UINTN               Offset,
  IN OUT    UINTN               *NumBytes,
  IN        UINT8               *Buffer
  )
{
  EFI_STATUS  TempStatus;
  UINT32      Tmp;
  UINT32      TmpBuf;
  UINT32      WordToWrite;
  UINT32      Mask;
  BOOLEAN     DoErase;
  UINTN       BytesToWrite;
  UINTN       CurOffset;
  UINTN       WordAddr;
  UINTN       BlockSize;
  UINTN       BlockAddress;
  UINTN       PrevBlockAddress;

  PrevBlockAddress = 0;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteSingleBlock(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n", Lba, Offset, *NumBytes, Buffer));

  // Detect WriteDisabled state
  if (Instance->Media.ReadOnly == TRUE) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteSingleBlock: ERROR - Can not write: Device is in WriteDisabled state.\n"));
    // It is in WriteDisabled state, return an error right away
    return EFI_ACCESS_DENIED;
  }

  // Cache the block size to avoid de-referencing pointers all the time
  BlockSize = Instance->Media.BlockSize;

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

  // Pick 128bytes as a good start for word operations as opposed to erasing the
  // block and writing the data regardless if an erase is really needed.
  // It looks like most individual NV variable writes are smaller than 128bytes.
  if (*NumBytes <= 128) {
    // Check to see if we need to erase before programming the data into NOR.
    // If the destination bits are only changing from 1s to 0s we can just write.
    // After a block is erased all bits in the block is set to 1.
    // If any byte requires us to erase we just give up and rewrite all of it.
    DoErase      = FALSE;
    BytesToWrite = *NumBytes;
    CurOffset    = Offset;

    while (BytesToWrite > 0) {
      // Read full word from NOR, splice as required. A word is the smallest
      // unit we can write.
      TempStatus = NorFlashRead (Instance, Lba, CurOffset & ~(0x3), sizeof (Tmp), &Tmp);
      if (EFI_ERROR (TempStatus)) {
        return EFI_DEVICE_ERROR;
      }

      // Physical address of word in NOR to write.
      WordAddr = (CurOffset & ~(0x3)) + GET_NOR_BLOCK_ADDRESS (
                                          Instance->RegionBaseAddress,
                                          Lba,
                                          BlockSize
                                          );
      // The word of data that is to be written.
      TmpBuf = *((UINT32 *)(Buffer + (*NumBytes - BytesToWrite)));

      // First do word aligned chunks.
      if ((CurOffset & 0x3) == 0) {
        if (BytesToWrite >= 4) {
          // Is the destination still in 'erased' state?
          if (~Tmp != 0) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Write this word to NOR
          WordToWrite   = TmpBuf;
          CurOffset    += sizeof (TmpBuf);
          BytesToWrite -= sizeof (TmpBuf);
        } else {
          // BytesToWrite < 4. Do small writes and left-overs
          Mask = ~((~0) << (BytesToWrite * 8));
          // Mask out the bytes we want.
          TmpBuf &= Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite  = (Tmp & ~Mask) | TmpBuf;
          CurOffset   += BytesToWrite;
          BytesToWrite = 0;
        }
      } else {
        // Do multiple words, but starting unaligned.
        if (BytesToWrite > (4 - (CurOffset & 0x3))) {
          Mask = ((~0) << ((CurOffset & 0x3) * 8));
          // Mask out the bytes we want.
          TmpBuf &= Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite   = (Tmp & ~Mask) | TmpBuf;
          BytesToWrite -= (4 - (CurOffset & 0x3));
          CurOffset    += (4 - (CurOffset & 0x3));
        } else {
          // Unaligned and fits in one word.
          Mask = (~((~0) << (BytesToWrite * 8))) << ((CurOffset & 0x3) * 8);
          // Mask out the bytes we want.
          TmpBuf = (TmpBuf << ((CurOffset & 0x3) * 8)) & Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite  = (Tmp & ~Mask) | TmpBuf;
          CurOffset   += BytesToWrite;
          BytesToWrite = 0;
        }
      }

      //
      // Write the word to NOR.
      //

      BlockAddress = GET_NOR_BLOCK_ADDRESS (Instance->RegionBaseAddress, Lba, BlockSize);
      if (BlockAddress != PrevBlockAddress) {
        TempStatus = NorFlashUnlockSingleBlockIfNecessary (Instance, BlockAddress);
        if (EFI_ERROR (TempStatus)) {
          return EFI_DEVICE_ERROR;
        }

        PrevBlockAddress = BlockAddress;
      }

      TempStatus = NorFlashWriteSingleWord (Instance, WordAddr, WordToWrite);
      if (EFI_ERROR (TempStatus)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Exit if we got here and could write all the data. Otherwise do the
    // Erase-Write cycle.
    if (!DoErase) {
      return EFI_SUCCESS;
    }
  }

  // Check we did get some memory. Buffer is BlockSize.
  if (Instance->ShadowBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "FvbWrite: ERROR - Buffer not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // Read NOR Flash data into shadow buffer
  TempStatus = NorFlashReadBlocks (Instance, Lba, BlockSize, Instance->ShadowBuffer);
  if (EFI_ERROR (TempStatus)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  // Put the data at the appropriate location inside the buffer area
  CopyMem ((VOID *)((UINTN)Instance->ShadowBuffer + Offset), Buffer, *NumBytes);

  // Write the modified buffer back to the NorFlash
  TempStatus = NorFlashWriteBlocks (Instance, Lba, BlockSize, Instance->ShadowBuffer);
  if (EFI_ERROR (TempStatus)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/*
  Although DiskIoDxe will automatically install the DiskIO protocol whenever
  we install the BlockIO protocol, its implementation is sub-optimal as it reads
  and writes entire blocks using the BlockIO protocol. In fact we can access
  NOR flash with a finer granularity than that, so we can improve performance
  by directly producing the DiskIO protocol.
*/

/**
  Read BufferSize bytes from Offset into Buffer.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
NorFlashDiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                DiskOffset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  )
{
  NOR_FLASH_INSTANCE  *Instance;
  UINT32              BlockSize;
  UINT32              BlockOffset;
  EFI_LBA             Lba;

  Instance = INSTANCE_FROM_DISKIO_THIS (This);

  if (MediaId != Instance->Media.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  BlockSize = Instance->Media.BlockSize;
  Lba       = (EFI_LBA)DivU64x32Remainder (DiskOffset, BlockSize, &BlockOffset);

  return NorFlashRead (Instance, Lba, BlockOffset, BufferSize, Buffer);
}

/**
  Writes a specified number of bytes to a device.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    ID of the medium to be written.
  @param  Offset     The starting byte offset on the logical block I/O device to write.
  @param  BufferSize The size in bytes of Buffer. The number of bytes to write to the device.
  @param  Buffer     A pointer to the buffer containing the data to be written.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
NorFlashDiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                DiskOffset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  )
{
  NOR_FLASH_INSTANCE  *Instance;
  UINT32              BlockSize;
  UINT32              BlockOffset;
  EFI_LBA             Lba;
  UINTN               RemainingBytes;
  UINTN               WriteSize;
  EFI_STATUS          Status;

  Instance = INSTANCE_FROM_DISKIO_THIS (This);

  if (MediaId != Instance->Media.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  BlockSize = Instance->Media.BlockSize;
  Lba       = (EFI_LBA)DivU64x32Remainder (DiskOffset, BlockSize, &BlockOffset);

  RemainingBytes = BufferSize;

  // Write either all the remaining bytes, or the number of bytes that bring
  // us up to a block boundary, whichever is less.
  // (DiskOffset | (BlockSize - 1)) + 1) rounds DiskOffset up to the next
  // block boundary (even if it is already on one).
  WriteSize = MIN (RemainingBytes, ((DiskOffset | (BlockSize - 1)) + 1) - DiskOffset);

  do {
    if (WriteSize == BlockSize) {
      // Write a full block
      Status = NorFlashWriteFullBlock (Instance, Lba, Buffer, BlockSize / sizeof (UINT32));
    } else {
      // Write a partial block
      Status = NorFlashWriteSingleBlock (Instance, Lba, BlockOffset, &WriteSize, Buffer);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Now continue writing either all the remaining bytes or single blocks.
    RemainingBytes -= WriteSize;
    Buffer          = (UINT8 *)Buffer + WriteSize;
    Lba++;
    BlockOffset = 0;
    WriteSize   = MIN (RemainingBytes, BlockSize);
  } while (RemainingBytes);

  return Status;
}

EFI_STATUS
NorFlashReset (
  IN  NOR_FLASH_INSTANCE  *Instance
  )
{
  // As there is no specific RESET to perform, ensure that the devices is in the default Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);
  return EFI_SUCCESS;
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
NorFlashVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->DeviceBaseAddress);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->RegionBaseAddress);

    // Convert BlockIo protocol
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.FlushBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.ReadBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.Reset);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.WriteBlocks);

    // Convert Fvb
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.EraseBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetBlockSize);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetPhysicalAddress);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.Read);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.SetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.Write);

    if (mNorFlashInstances[Index]->ShadowBuffer != NULL) {
      EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->ShadowBuffer);
    }
  }

  return;
}
