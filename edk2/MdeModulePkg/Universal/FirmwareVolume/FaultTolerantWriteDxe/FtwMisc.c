/** @file

  Internal generic functions to support fault tolerant write.

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#include "FtwLite.h"

/**

  Check whether a flash buffer is erased.


  @param Polarity        All 1 or all 0
  @param Buffer          Buffer to check
  @param BufferSize      Size of the buffer

  @return A BOOLEAN value indicating erased or not.

**/
BOOLEAN
IsErasedFlashBuffer (
  IN BOOLEAN         Polarity,
  IN UINT8           *Buffer,
  IN UINTN           BufferSize
  )
{
  UINT8 ErasedValue;
  UINT8 *Ptr;

  if (Polarity) {
    ErasedValue = 0xFF;
  } else {
    ErasedValue = 0;
  }

  Ptr = Buffer;
  while ((BufferSize--) != 0) {
    if (*Ptr++ != ErasedValue) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  To Erase one block. The size is FTW_BLOCK_SIZE


  @param FtwLiteDevice   Calling context
  @param FvBlock         FVB Protocol interface
  @param Lba             Lba of the firmware block

  @retval  EFI_SUCCESS    Block LBA is Erased successfully
  @retval  Others         Error occurs

**/
EFI_STATUS
FtwEraseBlock (
  IN EFI_FTW_LITE_DEVICE              *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  return FvBlock->EraseBlocks (
                    FvBlock,
                    Lba,
                    FtwLiteDevice->NumberOfSpareBlock,
                    EFI_LBA_LIST_TERMINATOR
                    );
}

/**

  Erase spare block.


  @param FtwLiteDevice   Calling context

  @retval EFI_SUCCESS The erase request was successfully
                      completed.
  
  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.
  @retval EFI_DEVICE_ERROR  The block device is not functioning
                            correctly and could not be written.
                            The firmware device may have been
                            partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.  


**/
EFI_STATUS
FtwEraseSpareBlock (
  IN EFI_FTW_LITE_DEVICE   *FtwLiteDevice
  )
{
  return FtwLiteDevice->FtwBackupFvb->EraseBlocks (
                                        FtwLiteDevice->FtwBackupFvb,
                                        FtwLiteDevice->FtwSpareLba,
                                        FtwLiteDevice->NumberOfSpareBlock,
                                        EFI_LBA_LIST_TERMINATOR
                                        );
}

/**
  Retrive the proper FVB protocol interface by HANDLE.


  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param FvBlock         The interface of FVB protocol

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwGetFvbByHandle (
  IN EFI_HANDLE                           FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the FVB protocol interface on the handle
  //
  return gBS->HandleProtocol (
                FvBlockHandle,
                &gEfiFirmwareVolumeBlockProtocolGuid,
                (VOID **) FvBlock
                );
}

/**

  Get firmware block by address.


  @param Address         Address specified the block
  @param FvBlock         The block caller wanted

  @retval  EFI_SUCCESS    The protocol instance if found.
  @retval  EFI_NOT_FOUND  Block not found

**/
EFI_STATUS
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

  *FvBlock = NULL;
  //
  // Locate all handles of Fvb protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Search all FVB until find the right one
  //
  for (Index = 0; Index < HandleCount; Index += 1) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &Fvb
                    );
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
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
      Status    = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);
  return Status;
}

/**

  Is it in working block?


  @param FtwLiteDevice   Calling context
  @param FvBlock         Fvb protocol instance
  @param Lba             The block specified

  @return A BOOLEAN value indicating in working block or not.

**/
BOOLEAN
IsInWorkingBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
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
      (FvBlock == FtwLiteDevice->FtwFvBlock) &&
      (Lba >= FtwLiteDevice->FtwWorkBlockLba) &&
      (Lba <= FtwLiteDevice->FtwWorkSpaceLba)
    );
}

/**
  Copy the content of spare block to a target block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwLiteDevice->FtwSpareLba.
  Target block is accessed by FvBlock protocol interface. LBA is Lba.


  @param FtwLiteDevice   The private data of FTW_LITE driver
  @param FvBlock         FVB Protocol interface to access target block
  @param Lba             Lba of the target block

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToTargetBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
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

  if ((FtwLiteDevice == NULL) || (FvBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Allocate a memory buffer
  //
  Length  = FtwLiteDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Read all content of spare block to memory buffer
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Read (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
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
  Status = FtwEraseBlock (FtwLiteDevice, FvBlock, Lba);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer to block, using the FvbBlock protocol interface
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Count   = FtwLiteDevice->SizeOfSpareBlock;
    Status  = FvBlock->Write (FvBlock, Lba + Index, 0, &Count, Ptr);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_FTW_LITE, "FtwLite: FVB Write block - %r\n", Status));
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
  FtwLiteDevice->FtwSpareLba.
  Working block is accessed by FTW working FVB protocol interface. LBA is
  FtwLiteDevice->FtwWorkBlockLba.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully
                                     Notes:
                                     Since the working block header is important when FTW initializes, the
                                     state of the operation should be handled carefully. The Crc value is
                                     calculated without STATE element.

**/
EFI_STATUS
FlushSpareBlockToWorkingBlock (
  EFI_FTW_LITE_DEVICE          *FtwLiteDevice
  )
{
  EFI_STATUS                              Status;
  UINTN                                   Length;
  UINT8                                   *Buffer;
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingBlockHeader;
  EFI_LBA                                 WorkSpaceLbaOffset;
  UINTN                                   Count;
  UINT8                                   *Ptr;
  UINTN                                   Index;

  //
  // Allocate a memory buffer
  //
  Length  = FtwLiteDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // To  guarantee that the WorkingBlockValid is set on spare block
  //
  WorkSpaceLbaOffset = FtwLiteDevice->FtwWorkSpaceLba - FtwLiteDevice->FtwWorkBlockLba;
  FtwUpdateFvState (
    FtwLiteDevice->FtwBackupFvb,
    FtwLiteDevice->FtwSpareLba + WorkSpaceLbaOffset,
    FtwLiteDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
    WORKING_BLOCK_VALID
    );
  //
  // Read from spare block to memory buffer
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Read (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
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
  WorkingBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *) (Buffer + (UINTN) WorkSpaceLbaOffset * FtwLiteDevice->SizeOfSpareBlock + FtwLiteDevice->FtwWorkSpaceBase);
  InitWorkSpaceHeader (WorkingBlockHeader);
  WorkingBlockHeader->WorkingBlockValid   = FTW_ERASE_POLARITY;
  WorkingBlockHeader->WorkingBlockInvalid = FTW_ERASE_POLARITY;

  //
  // target block is working block, then
  //   Set WorkingBlockInvalid in EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER
  //   before erase the working block.
  //
  //  Offset = EFI_FIELD_OFFSET(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER,
  //                            WorkingBlockInvalid);
  // To skip Signature and Crc: sizeof(EFI_GUID)+sizeof(UINT32).
  //
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_INVALID
            );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }

  FtwLiteDevice->FtwWorkSpaceHeader->WorkingBlockInvalid = FTW_VALID_STATE;

  //
  // Erase the working block
  //
  Status = FtwEraseBlock (
            FtwLiteDevice,
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkBlockLba
            );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer to working block, using the FvbBlock protocol interface
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwFvBlock->Write (
                                          FtwLiteDevice->FtwFvBlock,
                                          FtwLiteDevice->FtwWorkBlockLba + Index,
                                          0,
                                          &Count,
                                          Ptr
                                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_FTW_LITE, "FtwLite: FVB Write block - %r\n", Status));
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
  // Offset = EFI_FIELD_OFFSET(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER,
  //                           WorkingBlockValid);
  // Hardcode offset sizeof(EFI_GUID)+sizeof(UINT32), to skip Signature and Crc
  //
  Status = FtwUpdateFvState (
            FtwLiteDevice->FtwFvBlock,
            FtwLiteDevice->FtwWorkSpaceLba,
            FtwLiteDevice->FtwWorkSpaceBase + sizeof (EFI_GUID) + sizeof (UINT32),
            WORKING_BLOCK_VALID
            );
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwLiteDevice->FtwWorkSpaceHeader->WorkingBlockValid = FTW_VALID_STATE;

  return EFI_SUCCESS;
}
