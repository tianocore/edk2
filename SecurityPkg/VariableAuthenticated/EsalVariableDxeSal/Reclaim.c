/** @file
  Handles non-volatile variable store garbage collection, using FTW
  (Fault Tolerant Write) protocol.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

/**
  Gets firmware volume block handle by given address.

  This function gets firmware volume block handle whose
  address range contains the parameter Address.

  @param[in]  Address    Address which should be contained
                         by returned FVB handle.
  @param[out] FvbHandle  Pointer to FVB handle for output.

  @retval EFI_SUCCESS    FVB handle successfully returned.
  @retval EFI_NOT_FOUND  Failed to find FVB handle by address.

**/
EFI_STATUS
GetFvbHandleByAddress (
  IN  EFI_PHYSICAL_ADDRESS   Address,
  OUT EFI_HANDLE             *FvbHandle
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;

  *FvbHandle = NULL;
  //
  // Locate all handles with Firmware Volume Block protocol
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
  // Traverse all the handles, searching for the one containing parameter Address
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
    // Checks if the address range of this handle contains parameter Address
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvbBaseAddress);
    if ((Address >= FvbBaseAddress) && (Address <= (FvbBaseAddress + FwVolHeader->FvLength))) {
      *FvbHandle  = HandleBuffer[Index];
      Status      = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);
  return Status;
}

/**
  Gets LBA of block and offset by given address.

  This function gets the Logical Block Address (LBA) of firmware
  volume block containing the given address, and the offset of
  address on the block.

  @param[in]  Address    Address which should be contained
                         by returned FVB handle.
  @param[out] Lba        The pointer to LBA for output.
  @param[out] Offset     The pointer to offset for output.

  @retval EFI_SUCCESS    LBA and offset successfully returned.
  @retval EFI_NOT_FOUND  Failed to find FVB handle by address.
  @retval EFI_ABORTED    Failed to find valid LBA and offset.

**/
EFI_STATUS
GetLbaAndOffsetByAddress (
  IN  EFI_PHYSICAL_ADDRESS   Address,
  OUT EFI_LBA                *Lba,
  OUT UINTN                  *Offset
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          FvbHandle;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_FV_BLOCK_MAP_ENTRY              *FvbMapEntry;
  UINT32                              LbaIndex;

  *Lba    = (EFI_LBA) (-1);
  *Offset = 0;

  //
  // Gets firmware volume block handle by given address.
  //
  Status = GetFvbHandleByAddress (Address, &FvbHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  FvbHandle,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  (VOID **) &Fvb
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the Base Address of FV
  //
  Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvbBaseAddress);

  //
  // Get the (LBA, Offset) of Address
  //
  if ((Address >= FvbBaseAddress) && (Address <= (FvbBaseAddress + FwVolHeader->FvLength))) {
    if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
      //
      // BUGBUG: Assume one FV has one type of BlockLength
      //
      FvbMapEntry = &FwVolHeader->BlockMap[0];
      for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
        if (Address < (FvbBaseAddress + FvbMapEntry->Length * LbaIndex)) {
          //
          // Found the (Lba, Offset)
          //
          *Lba    = LbaIndex - 1;
          *Offset = (UINTN) (Address - (FvbBaseAddress + FvbMapEntry->Length * (LbaIndex - 1)));
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_ABORTED;
}

/**
  Writes a buffer to variable storage space.

  This function writes a buffer to variable storage space into firmware
  volume block device. The destination is specified by parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param[in] VariableBase The base address of the variable to write.
  @param[in] Buffer       Points to the data buffer.
  @param[in] BufferSize   The number of bytes of the data Buffer.

  @retval EFI_SUCCESS     The function completed successfully.
  @retval EFI_NOT_FOUND   Fail to locate Fault Tolerant Write protocol.
  @retval Other           The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            FvbHandle;
  EFI_LBA               VarLba;
  UINTN                 VarOffset;
  UINT8                 *FtwBuffer;
  UINTN                 FtwBufferSize;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *FtwProtocol;

  //
  // Locate Fault Tolerant Write protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteProtocolGuid,
                  NULL,
                  (VOID **) &FtwProtocol
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Gets firmware volume block handle by VariableBase.
  //
  Status = GetFvbHandleByAddress (VariableBase, &FvbHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Gets LBA of block and offset by VariableBase.
  //
  Status = GetLbaAndOffsetByAddress (VariableBase, &VarLba, &VarOffset);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Prepare for the variable data
  //
  FtwBufferSize = ((VARIABLE_STORE_HEADER *) ((UINTN) VariableBase))->Size;
  FtwBuffer     = AllocatePool (FtwBufferSize);
  if (FtwBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (FtwBuffer, FtwBufferSize, (UINT8) 0xff);
  CopyMem (FtwBuffer, Buffer, BufferSize);

  //
  // FTW write record
  //
  Status = FtwProtocol->Write (
                          FtwProtocol,
                          VarLba,         // LBA
                          VarOffset,      // Offset
                          FtwBufferSize,  // NumBytes,
                          NULL,
                          FvbHandle,
                          FtwBuffer
                          );

  FreePool (FtwBuffer);
  return Status;
}
