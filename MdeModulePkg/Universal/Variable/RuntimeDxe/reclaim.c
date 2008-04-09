/** @file
  
  Handles non-volatile variable store garbage collection, using FTW
  (Fault Tolerant Write) protocol.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Variable.h"
#include <VariableFormat.h>

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
  // Get the FVB to access variable store
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
    if ((Address >= FvbBaseAddress) && (Address <= (FvbBaseAddress + FwVolHeader->FvLength))) {
      *FvbHandle  = HandleBuffer[Index];
      Status      = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);
  return Status;
}

STATIC
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
  // Get the proper FVB
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

EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  )
/*++

Routine Description:
    Write a buffer to Variable space, in the working block.

Arguments:
    FvbHandle        - Indicates a handle to FVB to access variable store
    Buffer           - Point to the input buffer
    BufferSize       - The number of bytes of the input Buffer

Returns:
    EFI_SUCCESS            - The function completed successfully
    EFI_ABORTED            - The function could not complete successfully
    EFI_NOT_FOUND          - Locate FVB protocol by handle fails

--*/
{
  EFI_STATUS            Status;
  EFI_HANDLE            FvbHandle;
  EFI_FTW_LITE_PROTOCOL *FtwLiteProtocol;
  EFI_LBA               VarLba;
  UINTN                 VarOffset;
  UINT8                 *FtwBuffer;
  UINTN                 FtwBufferSize;

  //
  // Locate fault tolerant write protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteLiteProtocolGuid,
                  NULL,
                  (VOID **) &FtwLiteProtocol
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Locate Fvb handle by address
  //
  Status = GetFvbHandleByAddress (VariableBase, &FvbHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get LBA and Offset by address
  //
  Status = GetLbaAndOffsetByAddress (VariableBase, &VarLba, &VarOffset);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Prepare for the variable data
  //
  FtwBufferSize = ((VARIABLE_STORE_HEADER *) ((UINTN) VariableBase))->Size;
  FtwBuffer     = AllocateRuntimePool (FtwBufferSize);
  if (FtwBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (FtwBuffer, FtwBufferSize, (UINT8) 0xff);
  CopyMem (FtwBuffer, Buffer, BufferSize);

  //
  // FTW write record
  //
  Status = FtwLiteProtocol->Write (
                              FtwLiteProtocol,
                              FvbHandle,
                              VarLba,         // LBA
                              VarOffset,      // Offset
                              &FtwBufferSize, // NumBytes,
                              FtwBuffer
                              );

  FreePool (FtwBuffer);
  return Status;
}
