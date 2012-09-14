/** @file

  Internal generic functions to operate flash block.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#include "FaultTolerantWrite.h"

/**

  Check whether a flash buffer is erased.

  @param Buffer          Buffer to check
  @param BufferSize      Size of the buffer

  @return A BOOLEAN value indicating erased or not.

**/
BOOLEAN
IsErasedFlashBuffer (
  IN UINT8           *Buffer,
  IN UINTN           BufferSize
  )
{
  BOOLEAN IsEmpty;
  UINT8   *Ptr;
  UINTN   Index;

  Ptr     = Buffer;
  IsEmpty = TRUE;
  for (Index = 0; Index < BufferSize; Index += 1) {
    if (*Ptr++ != FTW_ERASED_BYTE) {
      IsEmpty = FALSE;
      break;
    }
  }

  return IsEmpty;
}

/**
  To erase the block with the spare block size.


  @param FtwDevice       The private data of FTW driver
  @param FvBlock         FVB Protocol interface
  @param Lba             Lba of the firmware block

  @retval  EFI_SUCCESS    Block LBA is Erased successfully
  @retval  Others         Error occurs

**/
EFI_STATUS
FtwEraseBlock (
  IN EFI_FTW_DEVICE                   *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  return FvBlock->EraseBlocks (
                    FvBlock,
                    Lba,
                    FtwDevice->NumberOfSpareBlock,
                    EFI_LBA_LIST_TERMINATOR
                    );
}

/**
  Erase spare block.

  @param FtwDevice        The private data of FTW driver

  @retval EFI_SUCCESS           The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED     The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR      The block device is not functioning
                                correctly and could not be written.
                                The firmware device may have been
                                partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.  


**/
EFI_STATUS
FtwEraseSpareBlock (
  IN EFI_FTW_DEVICE   *FtwDevice
  )
{
  return FtwDevice->FtwBackupFvb->EraseBlocks (
                                    FtwDevice->FtwBackupFvb,
                                    FtwDevice->FtwSpareLba,
                                    FtwDevice->NumberOfSpareBlock,
                                    EFI_LBA_LIST_TERMINATOR
                                    );
}

/**

  Is it in working block?

  @param FtwDevice       The private data of FTW driver
  @param FvBlock         Fvb protocol instance
  @param Lba             The block specified

  @return A BOOLEAN value indicating in working block or not.

**/
BOOLEAN
IsWorkingBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  //
  // If matching the following condition, the target block is in working block.
  // 1. Target block is on the FV of working block (Using the same FVB protocol instance).
  // 2. Lba falls into the range of working block.
  //
  return (BOOLEAN)
    (
      (FvBlock == FtwDevice->FtwFvBlock) &&
      (Lba >= FtwDevice->FtwWorkBlockLba) &&
      (Lba <= FtwDevice->FtwWorkSpaceLba)
    );
}

/**

  Get firmware block by address.


  @param Address         Address specified the block
  @param FvBlock         The block caller wanted

  @retval  EFI_SUCCESS    The protocol instance if found.
  @retval  EFI_NOT_FOUND  Block not found

**/
EFI_HANDLE
GetFvbByAddress (
  IN  EFI_PHYSICAL_ADDRESS               Address,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL **FvBlock
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_HANDLE                          FvbHandle;

  *FvBlock  = NULL;
  FvbHandle = NULL;
  //
  // Locate all handles of Fvb protocol
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  //
  // Get the FVB to access variable store
  //
  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = FtwGetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Compare the address and select the right one
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvbBaseAddress);
    if ((Address >= FvbBaseAddress) && (Address <= (FvbBaseAddress + (FwVolHeader->FvLength - 1)))) {
      *FvBlock  = Fvb;
      FvbHandle  = HandleBuffer[Index];
      break;
    }
  }

  FreePool (HandleBuffer);
  return FvbHandle;
}

/**

  Is it in boot block?

  @param FtwDevice       The private data of FTW driver
  @param FvBlock         Fvb protocol instance
  @param Lba             The block specified

  @return A BOOLEAN value indicating in boot block or not.

**/
BOOLEAN
IsBootBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  EFI_STATUS                          Status;
  EFI_SWAP_ADDRESS_RANGE_PROTOCOL     *SarProtocol;
  EFI_PHYSICAL_ADDRESS                BootBlockBase;
  UINTN                               BootBlockSize;
  EFI_PHYSICAL_ADDRESS                BackupBlockBase;
  UINTN                               BackupBlockSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *BootFvb;
  BOOLEAN                             IsSwapped;
  EFI_HANDLE                          FvbHandle;

  if (!FeaturePcdGet(PcdFullFtwServiceEnable)) {
    return FALSE;
  }

  Status = FtwGetSarProtocol ((VOID **) &SarProtocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Get the boot block range
  //
  Status = SarProtocol->GetRangeLocation (
                          SarProtocol,
                          &BootBlockBase,
                          &BootBlockSize,
                          &BackupBlockBase,
                          &BackupBlockSize
                          );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = SarProtocol->GetSwapState (SarProtocol, &IsSwapped);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Get FVB by address
  //
  if (!IsSwapped) {
    FvbHandle = GetFvbByAddress (BootBlockBase, &BootFvb);
  } else {
    FvbHandle = GetFvbByAddress (BackupBlockBase, &BootFvb);
  }

  if (FvbHandle == NULL) {
    return FALSE;
  }
  //
  // Compare the Fvb
  //
  return (BOOLEAN) (FvBlock == BootFvb);
}

/**
  Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW working FVB protocol interface. LBA is 1.
  Target block is accessed by FvbBlock protocol interface. LBA is Lba.

  FTW will do extra work on boot block update.
  FTW should depend on a protocol of EFI_ADDRESS_RANGE_SWAP_PROTOCOL,
  which is produced by a chipset driver.
  FTW updating boot block steps may be:
  1. GetRangeLocation(), if the Range is inside the boot block, FTW know
  that boot block will be update. It shall add a FLAG in the working block.
  2. When spare block is ready,
  3. SetSwapState(EFI_SWAPPED)
  4. erasing boot block,
  5. programming boot block until the boot block is ok.
  6. SetSwapState(UNSWAPPED)
  FTW shall not allow to update boot block when battery state is error.

  @param FtwDevice       The private data of FTW driver

  @retval EFI_SUCCESS             Spare block content is copied to boot block
  @retval EFI_INVALID_PARAMETER   Input parameter error
  @retval EFI_OUT_OF_RESOURCES    Allocate memory error
  @retval EFI_ABORTED             The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_DEVICE                      *FtwDevice
  )
{
  EFI_STATUS                          Status;
  UINTN                               Length;
  UINT8                               *Buffer;
  UINTN                               Count;
  UINT8                               *Ptr;
  UINTN                               Index;
  BOOLEAN                             TopSwap;
  EFI_SWAP_ADDRESS_RANGE_PROTOCOL     *SarProtocol;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *BootFvb;
  EFI_LBA                             BootLba;

  if (!FeaturePcdGet(PcdFullFtwServiceEnable)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Locate swap address range protocol
  //
  Status = FtwGetSarProtocol ((VOID **) &SarProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate a memory buffer
  //
  Length = FtwDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Get TopSwap bit state
  //
  Status = SarProtocol->GetSwapState (SarProtocol, &TopSwap);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Ftw: Get Top Swapped status - %r\n", Status));
    FreePool (Buffer);
    return EFI_ABORTED;
  }

  if (TopSwap) {
    //
    // Get FVB of current boot block
    //
    if (GetFvbByAddress (FtwDevice->SpareAreaAddress + FtwDevice->SpareAreaLength, &BootFvb) == NULL) {
      FreePool (Buffer);
      return EFI_ABORTED;
    }
    //
    // Read data from current boot block
    //
    BootLba = 0;
    Ptr     = Buffer;
    for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
      Count = FtwDevice->BlockSize;
      Status = BootFvb->Read (
                          BootFvb,
                          BootLba + Index,
                          0,
                          &Count,
                          Ptr
                          );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      Ptr += Count;
    }
  } else {
    //
    // Read data from spare block
    //
    Ptr = Buffer;
    for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
      Count = FtwDevice->BlockSize;
      Status = FtwDevice->FtwBackupFvb->Read (
                                          FtwDevice->FtwBackupFvb,
                                          FtwDevice->FtwSpareLba + Index,
                                          0,
                                          &Count,
                                          Ptr
                                          );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      Ptr += Count;
    }
    //
    // Set TopSwap bit
    //
    Status = SarProtocol->SetSwapState (SarProtocol, TRUE);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }
  }
  //
  // Erase current spare block
  // Because TopSwap is set, this actually erase the top block (boot block)!
  //
  Status = FtwEraseSpareBlock (FtwDevice);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer currenet spare block. Still top block.
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Write (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
                                        0,
                                        &Count,
                                        Ptr
                                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Ftw: FVB Write boot block - %r\n", Status));
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }

  FreePool (Buffer);

  //
  // Clear TopSwap bit
  //
  Status = SarProtocol->SetSwapState (SarProtocol, FALSE);

  return Status;
}

/**
  Copy the content of spare block to a target block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is 1.
  Target block is accessed by FvbBlock protocol interface. LBA is Lba.


  @param FtwDevice       The private data of FTW driver
  @param FvBlock         FVB Protocol interface to access target block
  @param Lba             Lba of the target block

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToTargetBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  EFI_STATUS  Status;
  UINTN       Length;
  UINT8       *Buffer;
  UINTN       Count;
  UINT8       *Ptr;
  UINTN       Index;

  if ((FtwDevice == NULL) || (FvBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Allocate a memory buffer
  //
  Length = FtwDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Read all content of spare block to memory buffer
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Read (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
                                        0,
                                        &Count,
                                        Ptr
                                        );
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }
  //
  // Erase the target block
  //
  Status = FtwEraseBlock (FtwDevice, FvBlock, Lba);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer to block, using the FvbBlock protocol interface
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Count   = FtwDevice->BlockSize;
    Status  = FvBlock->Write (FvBlock, Lba + Index, 0, &Count, Ptr);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Ftw: FVB Write block - %r\n", Status));
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }

  FreePool (Buffer);

  return Status;
}

/**
  Copy the content of spare block to working block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwDevice->FtwSpareLba.
  Working block is accessed by FTW working FVB protocol interface. LBA is
  FtwDevice->FtwWorkBlockLba.

  Since the working block header is important when FTW initializes, the
  state of the operation should be handled carefully. The Crc value is
  calculated without STATE element.

  @param FtwDevice       The private data of FTW driver

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToWorkingBlock (
  EFI_FTW_DEVICE                      *FtwDevice
  )
{
  EFI_STATUS                              Status;
  UINTN                                   Length;
  UINT8                                   *Buffer;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingBlockHeader;
  UINTN                                   Count;
  UINT8                                   *Ptr;
  UINTN                                   Index;
  EFI_LBA                                 WorkSpaceLbaOffset;

  //
  // Allocate a memory buffer
  //
  Length = FtwDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  WorkSpaceLbaOffset = FtwDevice->FtwWorkSpaceLba - FtwDevice->FtwWorkBlockLba;

  //
  // To guarantee that the WorkingBlockValid is set on spare block
  //
  //  Offset = OFFSET_OF(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER,
  //                            WorkingBlockValid);
  // To skip Signature and Crc: sizeof(EFI_GUID)+sizeof(UINT32).
  //
  FtwUpdateFvState (
    FtwDevice->FtwBackupFvb,
    FtwDevice->FtwSpareLba + WorkSpaceLbaOffset,
    FtwDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
    WORKING_BLOCK_VALID
    );
  //
  // Read from spare block to memory buffer
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwDevice->BlockSize;
    Status = FtwDevice->FtwBackupFvb->Read (
                                        FtwDevice->FtwBackupFvb,
                                        FtwDevice->FtwSpareLba + Index,
                                        0,
                                        &Count,
                                        Ptr
                                        );
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }
  //
  // Clear the CRC and STATE, copy data from spare to working block.
  //
  WorkingBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) (Buffer + (UINTN) WorkSpaceLbaOffset * FtwDevice->BlockSize + FtwDevice->FtwWorkSpaceBase);
  InitWorkSpaceHeader (WorkingBlockHeader);
  WorkingBlockHeader->WorkingBlockValid   = FTW_ERASE_POLARITY;
  WorkingBlockHeader->WorkingBlockInvalid = FTW_ERASE_POLARITY;

  //
  // target block is working block, then
  //   Set WorkingBlockInvalid in EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER
  //   before erase the working block.
  //
  //  Offset = OFFSET_OF(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER,
  //                            WorkingBlockInvalid);
  // So hardcode offset as sizeof(EFI_GUID)+sizeof(UINT32) to
  // skip Signature and Crc.
  //
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
            FtwDevice->FtwWorkSpaceLba,
            FtwDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_INVALID
            );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }

  FtwDevice->FtwWorkSpaceHeader->WorkingBlockInvalid = FTW_VALID_STATE;

  //
  // Erase the working block
  //
  Status = FtwEraseBlock (FtwDevice, FtwDevice->FtwFvBlock, FtwDevice->FtwWorkBlockLba);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer to working block, using the FvbBlock protocol interface
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwDevice->BlockSize;
    Status = FtwDevice->FtwFvBlock->Write (
                                      FtwDevice->FtwFvBlock,
                                      FtwDevice->FtwWorkBlockLba + Index,
                                      0,
                                      &Count,
                                      Ptr
                                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Ftw: FVB Write block - %r\n", Status));
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }
  //
  // Since the memory buffer will not be used, free memory Buffer.
  //
  FreePool (Buffer);

  //
  // Update the VALID of the working block
  //
  // Offset = OFFSET_OF(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER, WorkingBlockValid);
  // So hardcode offset as sizeof(EFI_GUID)+sizeof(UINT32) to skip Signature and Crc.
  //
  Status = FtwUpdateFvState (
            FtwDevice->FtwFvBlock,
            FtwDevice->FtwWorkSpaceLba,
            FtwDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_VALID
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwDevice->FtwWorkSpaceHeader->WorkingBlockValid = FTW_VALID_STATE;

  return EFI_SUCCESS;
}

/**
  Update a bit of state on a block device. The location of the bit is
  calculated by the (Lba, Offset, bit). Here bit is determined by the
  the name of a certain bit.


  @param FvBlock         FVB Protocol interface to access SrcBlock and DestBlock
  @param Lba             Lba of a block
  @param Offset          Offset on the Lba
  @param NewBit          New value that will override the old value if it can be change

  @retval  EFI_SUCCESS    A state bit has been updated successfully
  @retval  Others         Access block device error.
                          Notes:
                          Assume all bits of State are inside the same BYTE.
  @retval  EFI_ABORTED    Read block fail

**/
EFI_STATUS
FtwUpdateFvState (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  IN EFI_LBA                             Lba,
  IN UINTN                               Offset,
  IN UINT8                               NewBit
  )
{
  EFI_STATUS  Status;
  UINT8       State;
  UINTN       Length;

  //
  // Read state from device, assume State is only one byte.
  //
  Length  = sizeof (UINT8);
  Status  = FvBlock->Read (FvBlock, Lba, Offset, &Length, &State);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  State ^= FTW_POLARITY_REVERT;
  State  = (UINT8) (State | NewBit);
  State ^= FTW_POLARITY_REVERT;

  //
  // Write state back to device
  //
  Length  = sizeof (UINT8);
  Status  = FvBlock->Write (FvBlock, Lba, Offset, &Length, &State);

  return Status;
}

/**
  Get the last Write Header pointer.
  The last write header is the header whose 'complete' state hasn't been set.
  After all, this header may be a EMPTY header entry for next Allocate.


  @param FtwWorkSpaceHeader Pointer of the working block header
  @param FtwWorkSpaceSize   Size of the work space
  @param FtwWriteHeader     Pointer to retrieve the last write header

  @retval  EFI_SUCCESS      Get the last write record successfully
  @retval  EFI_ABORTED      The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER  *FtwWorkSpaceHeader,
  IN UINTN                                    FtwWorkSpaceSize,
  OUT EFI_FAULT_TOLERANT_WRITE_HEADER         **FtwWriteHeader
  )
{
  UINTN                           Offset;
  EFI_FAULT_TOLERANT_WRITE_HEADER *FtwHeader;

  *FtwWriteHeader = NULL;
  FtwHeader       = (EFI_FAULT_TOLERANT_WRITE_HEADER *) (FtwWorkSpaceHeader + 1);
  Offset          = sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER);

  while (FtwHeader->Complete == FTW_VALID_STATE) {
    Offset += WRITE_TOTAL_SIZE (FtwHeader->NumberOfWrites, FtwHeader->PrivateDataSize);
    //
    // If Offset exceed the FTW work space boudary, return error.
    //
    if (Offset >= FtwWorkSpaceSize) {
      *FtwWriteHeader = FtwHeader;
      return EFI_ABORTED;
    }

    FtwHeader = (EFI_FAULT_TOLERANT_WRITE_HEADER *) ((UINT8 *) FtwWorkSpaceHeader + Offset);
  }
  //
  // Last write header is found
  //
  *FtwWriteHeader = FtwHeader;

  return EFI_SUCCESS;
}

/**
  Get the last Write Record pointer. The last write Record is the Record
  whose DestinationCompleted state hasn't been set. After all, this Record
  may be a EMPTY record entry for next write.


  @param FtwWriteHeader  Pointer to the write record header
  @param FtwWriteRecord  Pointer to retrieve the last write record

  @retval EFI_SUCCESS        Get the last write record successfully
  @retval EFI_ABORTED        The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteRecord (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER          *FtwWriteHeader,
  OUT EFI_FAULT_TOLERANT_WRITE_RECORD         **FtwWriteRecord
  )
{
  UINTN                           Index;
  EFI_FAULT_TOLERANT_WRITE_RECORD *FtwRecord;

  *FtwWriteRecord = NULL;
  FtwRecord       = (EFI_FAULT_TOLERANT_WRITE_RECORD *) (FtwWriteHeader + 1);

  //
  // Try to find the last write record "that has not completed"
  //
  for (Index = 0; Index < FtwWriteHeader->NumberOfWrites; Index += 1) {
    if (FtwRecord->DestinationComplete != FTW_VALID_STATE) {
      //
      // The last write record is found
      //
      *FtwWriteRecord = FtwRecord;
      return EFI_SUCCESS;
    }

    FtwRecord++;

    if (FtwWriteHeader->PrivateDataSize != 0) {
      FtwRecord = (EFI_FAULT_TOLERANT_WRITE_RECORD *) ((UINTN) FtwRecord + FtwWriteHeader->PrivateDataSize);
    }
  }
  //
  //  if Index == NumberOfWrites, then
  //  the last record has been written successfully,
  //  but the Header->Complete Flag has not been set.
  //  also return the last record.
  //
  if (Index == FtwWriteHeader->NumberOfWrites) {
    *FtwWriteRecord = (EFI_FAULT_TOLERANT_WRITE_RECORD *) ((UINTN) FtwRecord - RECORD_SIZE (FtwWriteHeader->PrivateDataSize));
    return EFI_SUCCESS;
  }

  return EFI_ABORTED;
}

/**
  To check if FtwRecord is the first record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to the write record

  @retval TRUE      FtwRecord is the first Record of the FtwHeader
  @retval FALSE     FtwRecord is not the first Record of the FtwHeader

**/
BOOLEAN
IsFirstRecordOfWrites (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN EFI_FAULT_TOLERANT_WRITE_RECORD    *FtwRecord
  )
{
  UINT8 *Head;
  UINT8 *Ptr;

  Head  = (UINT8 *) FtwHeader;
  Ptr   = (UINT8 *) FtwRecord;

  Head += sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER);
  return (BOOLEAN) (Head == Ptr);
}

/**
  To check if FtwRecord is the last record of FtwHeader. Because the
  FtwHeader has NumberOfWrites & PrivateDataSize, the FtwRecord can be
  determined if it is the last record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to the write record

  @retval TRUE      FtwRecord is the last Record of the FtwHeader
  @retval FALSE     FtwRecord is not the last Record of the FtwHeader

**/
BOOLEAN
IsLastRecordOfWrites (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN EFI_FAULT_TOLERANT_WRITE_RECORD    *FtwRecord
  )
{
  UINT8 *Head;
  UINT8 *Ptr;

  Head  = (UINT8 *) FtwHeader;
  Ptr   = (UINT8 *) FtwRecord;

  Head += WRITE_TOTAL_SIZE (FtwHeader->NumberOfWrites - 1, FtwHeader->PrivateDataSize);
  return (BOOLEAN) (Head == Ptr);
}

/**
  To check if FtwRecord is the first record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to retrieve the previous write record

  @retval EFI_ACCESS_DENIED  Input record is the first record, no previous record is return.
  @retval EFI_SUCCESS        The previous write record is found.

**/
EFI_STATUS
GetPreviousRecordOfWrites (
  IN     EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN OUT EFI_FAULT_TOLERANT_WRITE_RECORD    **FtwRecord
  )
{
  UINT8 *Ptr;

  if (IsFirstRecordOfWrites (FtwHeader, *FtwRecord)) {
    *FtwRecord = NULL;
    return EFI_ACCESS_DENIED;
  }

  Ptr = (UINT8 *) (*FtwRecord);
  Ptr -= RECORD_SIZE (FtwHeader->PrivateDataSize);
  *FtwRecord = (EFI_FAULT_TOLERANT_WRITE_RECORD *) Ptr;
  return EFI_SUCCESS;
}

/**
  Allocate private data for FTW driver and initialize it.

  @param[out] FtwData           Pointer to the FTW device structure

  @retval EFI_SUCCESS           Initialize the FTW device successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_INVALID_PARAMETER Workspace or Spare block does not exist

**/
EFI_STATUS
InitFtwDevice (
  OUT EFI_FTW_DEVICE               **FtwData 
  )
{
  EFI_FTW_DEVICE                   *FtwDevice;
  
  //
  // Allocate private data of this driver,
  // Including the FtwWorkSpace[FTW_WORK_SPACE_SIZE].
  //
  FtwDevice = AllocateZeroPool (sizeof (EFI_FTW_DEVICE) + PcdGet32 (PcdFlashNvStorageFtwWorkingSize));
  if (FtwDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize other parameters, and set WorkSpace as FTW_ERASED_BYTE.
  //
  FtwDevice->WorkSpaceLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  FtwDevice->SpareAreaLength  = (UINTN) PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  if ((FtwDevice->WorkSpaceLength == 0) || (FtwDevice->SpareAreaLength == 0)) {
    DEBUG ((EFI_D_ERROR, "Ftw: Workspace or Spare block does not exist!\n"));
    FreePool (FtwDevice);
    return EFI_INVALID_PARAMETER;
  }

  FtwDevice->Signature        = FTW_DEVICE_SIGNATURE;
  FtwDevice->FtwFvBlock       = NULL;
  FtwDevice->FtwBackupFvb     = NULL;
  FtwDevice->FtwWorkSpaceLba  = (EFI_LBA) (-1);
  FtwDevice->FtwSpareLba      = (EFI_LBA) (-1);

  FtwDevice->WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageFtwWorkingBase64);
  if (FtwDevice->WorkSpaceAddress == 0) {
    FtwDevice->WorkSpaceAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  }
  
  FtwDevice->SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageFtwSpareBase64);
  if (FtwDevice->SpareAreaAddress == 0) {
    FtwDevice->SpareAreaAddress = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  }  

  *FtwData = FtwDevice;
  return EFI_SUCCESS;
}


/**
  Find the proper Firmware Volume Block protocol for FTW operation.

  @param[in, out] FtwDevice     Pointer to the FTW device structure

  @retval EFI_SUCCESS           Find the FVB protocol successfully.
  @retval EFI_NOT_FOUND         No proper FVB protocol was found.
  @retval EFI_ABORTED           Some data can not be got or be invalid.
  
**/
EFI_STATUS
FindFvbForFtw (
  IN OUT EFI_FTW_DEVICE               *FtwDevice
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_FVB_ATTRIBUTES_2                Attributes;
  EFI_FV_BLOCK_MAP_ENTRY              *FvbMapEntry;
  UINT32                              LbaIndex;

  //
  // Get all FVB handle.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the FVB to access variable store
  //
  Fvb = NULL;
  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = FtwGetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    // Ensure this FVB protocol support Write operation.
    //
    Status = Fvb->GetAttributes (Fvb, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;     
    }
    //
    // Compare the address and select the right one
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvbBaseAddress);
    if ((FtwDevice->FtwFvBlock == NULL) && (FtwDevice->WorkSpaceAddress >= FvbBaseAddress) &&
      ((FtwDevice->WorkSpaceAddress + FtwDevice->WorkSpaceLength) <= (FvbBaseAddress + FwVolHeader->FvLength))
      ) {
      FtwDevice->FtwFvBlock = Fvb;
      //
      // To get the LBA of work space
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // Now, one FV has one type of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
          if ((FtwDevice->WorkSpaceAddress >= (FvbBaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwDevice->WorkSpaceAddress < (FvbBaseAddress + FvbMapEntry->Length * LbaIndex))) {
            FtwDevice->FtwWorkSpaceLba = LbaIndex - 1;
            //
            // Get the Work space size and Base(Offset)
            //
            FtwDevice->FtwWorkSpaceSize = FtwDevice->WorkSpaceLength;
            FtwDevice->FtwWorkSpaceBase = (UINTN) (FtwDevice->WorkSpaceAddress - (FvbBaseAddress + FvbMapEntry->Length * (LbaIndex - 1)));
            break;
          }
        }
      }
    }
    
    if ((FtwDevice->FtwBackupFvb == NULL) && (FtwDevice->SpareAreaAddress >= FvbBaseAddress) &&
      ((FtwDevice->SpareAreaAddress + FtwDevice->SpareAreaLength) <= (FvbBaseAddress + FwVolHeader->FvLength))
      ) {
      FtwDevice->FtwBackupFvb = Fvb;
      //
      // To get the LBA of spare
      //
      if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
        //
        // Now, one FV has one type of BlockLength
        //
        FvbMapEntry = &FwVolHeader->BlockMap[0];
        for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
          if ((FtwDevice->SpareAreaAddress >= (FvbBaseAddress + FvbMapEntry->Length * (LbaIndex - 1)))
              && (FtwDevice->SpareAreaAddress < (FvbBaseAddress + FvbMapEntry->Length * LbaIndex))) {
            //
            // Get the NumberOfSpareBlock and BlockSize
            //
            FtwDevice->FtwSpareLba   = LbaIndex - 1;
            FtwDevice->BlockSize     = FvbMapEntry->Length;
            FtwDevice->NumberOfSpareBlock = FtwDevice->SpareAreaLength / FtwDevice->BlockSize;
            //
            // Check the range of spare area to make sure that it's in FV range
            //
            if ((FtwDevice->FtwSpareLba + FtwDevice->NumberOfSpareBlock) > FvbMapEntry->NumBlocks) {
              DEBUG ((EFI_D_ERROR, "Ftw: Spare area is out of FV range\n"));
              FreePool (HandleBuffer);
              ASSERT (FALSE);
              return EFI_ABORTED;
            }
            break;
          }
        }
      }
    }
  }
  FreePool (HandleBuffer);
 
  if ((FtwDevice->FtwBackupFvb == NULL) || (FtwDevice->FtwFvBlock == NULL) ||
    (FtwDevice->FtwWorkSpaceLba == (EFI_LBA) (-1)) || (FtwDevice->FtwSpareLba == (EFI_LBA) (-1))) {
    return EFI_ABORTED;
  }
    
  return EFI_SUCCESS;
}


/**
  Initialization for Fault Tolerant Write protocol.

  @param[in, out] FtwDevice     Pointer to the FTW device structure

  @retval EFI_SUCCESS           Initialize the FTW protocol successfully.
  @retval EFI_NOT_FOUND         No proper FVB protocol was found.
  
**/
EFI_STATUS
InitFtwProtocol (
  IN OUT EFI_FTW_DEVICE               *FtwDevice
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINTN                               Length;
  EFI_FAULT_TOLERANT_WRITE_HEADER     *FtwHeader;
  UINTN                               Offset;
  EFI_HANDLE                          FvbHandle;

  //
  // Find the right SMM Fvb protocol instance for FTW.
  //
  Status = FindFvbForFtw (FtwDevice);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }  
  
  //
  // Calculate the start LBA of working block. Working block is an area which
  // contains working space in its last block and has the same size as spare
  // block, unless there are not enough blocks before the block that contains
  // working space.
  //
  FtwDevice->FtwWorkBlockLba = FtwDevice->FtwWorkSpaceLba - FtwDevice->NumberOfSpareBlock + 1;
  ASSERT ((INT64) (FtwDevice->FtwWorkBlockLba) >= 0); 

  //
  // Initialize other parameters, and set WorkSpace as FTW_ERASED_BYTE.
  //
  FtwDevice->FtwWorkSpace = (UINT8 *) (FtwDevice + 1);
  FtwDevice->FtwWorkSpaceHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) FtwDevice->FtwWorkSpace;

  FtwDevice->FtwLastWriteHeader = NULL;
  FtwDevice->FtwLastWriteRecord = NULL;

  //
  // Refresh the working space data from working block
  //
  Status = WorkSpaceRefresh (FtwDevice);
  ASSERT_EFI_ERROR (Status);
  //
  // If the working block workspace is not valid, try the spare block
  //
  if (!IsValidWorkSpace (FtwDevice->FtwWorkSpaceHeader)) {
    //
    // Read from spare block
    //
    Length = FtwDevice->FtwWorkSpaceSize;
    Status = FtwDevice->FtwBackupFvb->Read (
                    FtwDevice->FtwBackupFvb,
                    FtwDevice->FtwSpareLba,
                    FtwDevice->FtwWorkSpaceBase,
                    &Length,
                    FtwDevice->FtwWorkSpace
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // If spare block is valid, then replace working block content.
    //
    if (IsValidWorkSpace (FtwDevice->FtwWorkSpaceHeader)) {
      Status = FlushSpareBlockToWorkingBlock (FtwDevice);
      DEBUG ((EFI_D_ERROR, "Ftw: Restart working block update in InitFtwProtocol() - %r\n", Status));
      FtwAbort (&FtwDevice->FtwInstance);
      //
      // Refresh work space.
      //
      Status = WorkSpaceRefresh (FtwDevice);
      ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG ((EFI_D_ERROR, "Ftw: Both are invalid, init workspace\n"));
      //
      // If both are invalid, then initialize work space.
      //
      SetMem (
        FtwDevice->FtwWorkSpace,
        FtwDevice->FtwWorkSpaceSize,
        FTW_ERASED_BYTE
        );
      InitWorkSpaceHeader (FtwDevice->FtwWorkSpaceHeader);
      //
      // Initialize the work space
      //
      Status = FtwReclaimWorkSpace (FtwDevice, FALSE);
      ASSERT_EFI_ERROR (Status);
    }
  }
  //
  // If the FtwDevice->FtwLastWriteRecord is 1st record of write header &&
  // (! SpareComplete) THEN call Abort().
  //
  if ((FtwDevice->FtwLastWriteHeader->HeaderAllocated == FTW_VALID_STATE) &&
    (FtwDevice->FtwLastWriteRecord->SpareComplete != FTW_VALID_STATE) &&
    IsFirstRecordOfWrites (FtwDevice->FtwLastWriteHeader, FtwDevice->FtwLastWriteRecord)
    ) {
    DEBUG ((EFI_D_ERROR, "Ftw: Init.. find first record not SpareCompleted, abort()\n"));
    FtwAbort (&FtwDevice->FtwInstance);
  }
  //
  // If Header is incompleted and the last record has completed, then
  // call Abort() to set the Header->Complete FLAG.
  //
  if ((FtwDevice->FtwLastWriteHeader->Complete != FTW_VALID_STATE) &&
    (FtwDevice->FtwLastWriteRecord->DestinationComplete == FTW_VALID_STATE) &&
    IsLastRecordOfWrites (FtwDevice->FtwLastWriteHeader, FtwDevice->FtwLastWriteRecord)
    ) {
    DEBUG ((EFI_D_ERROR, "Ftw: Init.. find last record completed but header not, abort()\n"));
    FtwAbort (&FtwDevice->FtwInstance);
  }
  //
  // To check the workspace buffer following last Write header/records is EMPTY or not.
  // If it's not EMPTY, FTW also need to call reclaim().
  //
  FtwHeader = FtwDevice->FtwLastWriteHeader;
  Offset    = (UINT8 *) FtwHeader - FtwDevice->FtwWorkSpace;
  if (FtwDevice->FtwWorkSpace[Offset] != FTW_ERASED_BYTE) {
    Offset += WRITE_TOTAL_SIZE (FtwHeader->NumberOfWrites, FtwHeader->PrivateDataSize);
  }
  
  if (!IsErasedFlashBuffer (FtwDevice->FtwWorkSpace + Offset, FtwDevice->FtwWorkSpaceSize - Offset)) {
    Status = FtwReclaimWorkSpace (FtwDevice, TRUE);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Restart if it's boot block
  //
  if ((FtwDevice->FtwLastWriteHeader->Complete != FTW_VALID_STATE) &&
    (FtwDevice->FtwLastWriteRecord->SpareComplete == FTW_VALID_STATE)
    ) {
    if (FtwDevice->FtwLastWriteRecord->BootBlockUpdate == FTW_VALID_STATE) {
      Status = FlushSpareBlockToBootBlock (FtwDevice);
      DEBUG ((EFI_D_ERROR, "Ftw: Restart boot block update - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
      FtwAbort (&FtwDevice->FtwInstance);
    } else {
      //
      // if (SpareCompleted) THEN  Restart to fault tolerant write.
      //
      FvbHandle = NULL;
      FvbHandle = GetFvbByAddress (FtwDevice->FtwLastWriteRecord->FvBaseAddress, &Fvb);
      if (FvbHandle != NULL) {
        Status = FtwRestart (&FtwDevice->FtwInstance, FvbHandle);
        DEBUG ((EFI_D_ERROR, "FtwLite: Restart last write - %r\n", Status));
        ASSERT_EFI_ERROR (Status);
      }
      FtwAbort (&FtwDevice->FtwInstance);
    }
  }
  //
  // Hook the protocol API
  //
  FtwDevice->FtwInstance.GetMaxBlockSize = FtwGetMaxBlockSize;
  FtwDevice->FtwInstance.Allocate        = FtwAllocate;
  FtwDevice->FtwInstance.Write           = FtwWrite;
  FtwDevice->FtwInstance.Restart         = FtwRestart;
  FtwDevice->FtwInstance.Abort           = FtwAbort;
  FtwDevice->FtwInstance.GetLastWrite    = FtwGetLastWrite;
    
  return EFI_SUCCESS;
}

