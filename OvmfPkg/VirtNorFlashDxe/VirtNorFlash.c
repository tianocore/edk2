/** @file  NorFlash.c

  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>

#include "VirtNorFlash.h"

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

  // Request a lock setup
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_LOCK_BLOCK_SETUP);

  // Request an unlock
  SEND_NOR_COMMAND (BlockAddress, 0, P30_CMD_UNLOCK_BLOCK);

  // Wait until the status register gives us the all clear
  do {
    LockStatus = NorFlashReadStatusRegister (Instance, BlockAddress);
  } while ((LockStatus & P30_SR_BIT_WRITE) != P30_SR_BIT_WRITE);

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

  // We must have some bytes to read
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BufferSizeInBytes=0x%x\n", BufferSizeInBytes));
  if (BufferSizeInBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: BlockSize in bytes =0x%x\n", Instance->BlockSize));
  if ((BufferSizeInBytes % Instance->BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->BlockSize;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld.\n", NumBlocks, Instance->LastBlock, Lba));

  if ((Lba + NumBlocks) > (Instance->LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashWriteBlocks: ERROR - Write will exceed last block.\n"));
    return EFI_INVALID_PARAMETER;
  }

  BlockSizeInWords = Instance->BlockSize / 4;

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
    Instance->BlockSize,
    Instance->LastBlock,
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
  if ((BufferSizeInBytes % Instance->BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->BlockSize;

  if ((Lba + NumBlocks) > (Instance->LastBlock + 1)) {
    DEBUG ((DEBUG_ERROR, "NorFlashReadBlocks: ERROR - Read will exceed last block\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  CopyMem (Buffer, (VOID *)StartAddress, BufferSizeInBytes);

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

  if (((Lba * Instance->BlockSize) + Offset + BufferSizeInBytes) > Instance->Size) {
    DEBUG ((DEBUG_ERROR, "NorFlashRead: ERROR - Read will exceed device size.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->BlockSize
                   );

  // Put the device into Read Array mode
  SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

  // Readout the data
  CopyMem (Buffer, (VOID *)(StartAddress + Offset), BufferSizeInBytes);

  return EFI_SUCCESS;
}

/*
  Write a full or portion of a block. It must not span block boundaries; that is,
  Offset + *NumBytes <= Instance->BlockSize.
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
  EFI_STATUS  Status;
  UINTN       CurOffset;
  UINTN       BlockSize;
  UINTN       BlockAddress;
  UINT8       *OrigData;

  DEBUG ((DEBUG_BLKIO, "NorFlashWriteSingleBlock(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n", Lba, Offset, *NumBytes, Buffer));

  // Check we did get some memory. Buffer is BlockSize.
  if (Instance->ShadowBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "FvbWrite: ERROR - Buffer not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // Cache the block size to avoid de-referencing pointers all the time
  BlockSize = Instance->BlockSize;

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

  // Pick P30_MAX_BUFFER_SIZE_IN_BYTES (== 128 bytes) as a good start for word
  // operations as opposed to erasing the block and writing the data regardless
  // if an erase is really needed.  It looks like most individual NV variable
  // writes are smaller than 128 bytes.
  // To avoid pathological cases were a 2 byte write is disregarded because it
  // occurs right at a 128 byte buffered write alignment boundary, permit up to
  // twice the max buffer size, and perform two writes if needed.
  if ((*NumBytes + (Offset & BOUNDARY_OF_32_WORDS)) <= (2 * P30_MAX_BUFFER_SIZE_IN_BYTES)) {
    // Check to see if we need to erase before programming the data into NOR.
    // If the destination bits are only changing from 1s to 0s we can just write.
    // After a block is erased all bits in the block is set to 1.
    // If any byte requires us to erase we just give up and rewrite all of it.

    // Read the old version of the data into the shadow buffer
    Status = NorFlashRead (
               Instance,
               Lba,
               Offset & ~BOUNDARY_OF_32_WORDS,
               (*NumBytes | BOUNDARY_OF_32_WORDS) + 1,
               Instance->ShadowBuffer
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    // Make OrigData point to the start of the old version of the data inside
    // the word aligned buffer
    OrigData = Instance->ShadowBuffer + (Offset & BOUNDARY_OF_32_WORDS);

    // Update the buffer containing the old version of the data with the new
    // contents, while checking whether the old version had any bits cleared
    // that we want to set. In that case, we will need to erase the block first.
    for (CurOffset = 0; CurOffset < *NumBytes; CurOffset++) {
      if (~OrigData[CurOffset] & Buffer[CurOffset]) {
        goto DoErase;
      }

      OrigData[CurOffset] = Buffer[CurOffset];
    }

    //
    // Write the updated buffer to NOR.
    //
    BlockAddress = GET_NOR_BLOCK_ADDRESS (Instance->RegionBaseAddress, Lba, BlockSize);

    // Unlock the block if we have to
    Status = NorFlashUnlockSingleBlockIfNecessary (Instance, BlockAddress);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = NorFlashWriteBuffer (
               Instance,
               BlockAddress + (Offset & ~BOUNDARY_OF_32_WORDS),
               P30_MAX_BUFFER_SIZE_IN_BYTES,
               Instance->ShadowBuffer
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    if ((*NumBytes + (Offset & BOUNDARY_OF_32_WORDS)) > P30_MAX_BUFFER_SIZE_IN_BYTES) {
      BlockAddress += P30_MAX_BUFFER_SIZE_IN_BYTES;

      Status = NorFlashWriteBuffer (
                 Instance,
                 BlockAddress + (Offset & ~BOUNDARY_OF_32_WORDS),
                 P30_MAX_BUFFER_SIZE_IN_BYTES,
                 Instance->ShadowBuffer + P30_MAX_BUFFER_SIZE_IN_BYTES
                 );
    }

Exit:
    // Put device back into Read Array mode
    SEND_NOR_COMMAND (Instance->DeviceBaseAddress, 0, P30_CMD_READ_ARRAY);

    return Status;
  }

DoErase:
  // Read NOR Flash data into shadow buffer
  Status = NorFlashReadBlocks (Instance, Lba, BlockSize, Instance->ShadowBuffer);
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  // Put the data at the appropriate location inside the buffer area
  CopyMem ((VOID *)((UINTN)Instance->ShadowBuffer + Offset), Buffer, *NumBytes);

  // Write the modified buffer back to the NorFlash
  Status = NorFlashWriteBlocks (Instance, Lba, BlockSize, Instance->ShadowBuffer);
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
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
