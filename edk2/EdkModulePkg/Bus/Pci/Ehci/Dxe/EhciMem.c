/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    EhciMem.c

Abstract:


Revision History
--*/

#include "Ehci.h"


EFI_STATUS
CreateMemoryBlock (
  IN  USB2_HC_DEV               *HcDev,
  OUT MEMORY_MANAGE_HEADER      **MemoryHeader,
  IN  UINTN                     MemoryBlockSizeInPages
  )
/*++

Routine Description:

  Use PciIo->AllocateBuffer to allocate common buffer for the memory block,
  and use PciIo->Map to map the common buffer for Bus Master Read/Write.

Arguments:

  HcDev                  - USB2_HC_DEV
  MemoryHeader           - MEMORY_MANAGE_HEADER to output
  MemoryBlockSizeInPages - MemoryBlockSizeInPages

Returns:

  EFI_SUCCESS           Success
  EFI_OUT_OF_RESOURCES  Fail for no resources
  EFI_UNSUPPORTED       Unsupported currently

--*/
{
  EFI_STATUS            Status;
  VOID                  *CommonBuffer;
  EFI_PHYSICAL_ADDRESS  MappedAddress;
  UINTN                 MemoryBlockSizeInBytes;
  VOID                  *Mapping;

  //
  // Allocate memory for MemoryHeader
  //
  *MemoryHeader = AllocateZeroPool (sizeof (MEMORY_MANAGE_HEADER));
  if (*MemoryHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*MemoryHeader)->Next = NULL;

  //
  // set Memory block size
  //
  (*MemoryHeader)->MemoryBlockSizeInBytes = EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages);

  //
  // each bit in Bit Array will manage 32 bytes memory in memory block
  //
  (*MemoryHeader)->BitArraySizeInBytes = ((*MemoryHeader)->MemoryBlockSizeInBytes / MEM_UNIT_SIZE) / 8;

  //
  // Allocate memory for BitArray
  //
  (*MemoryHeader)->BitArrayPtr = AllocateZeroPool ((*MemoryHeader)->BitArraySizeInBytes);
  if ((*MemoryHeader)->BitArrayPtr == NULL) {
    gBS->FreePool (*MemoryHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Memory Block uses MemoryBlockSizeInPages pages,
  // and it is allocated as common buffer use.
  //
  Status = HcDev->PciIo->AllocateBuffer (
                           HcDev->PciIo,
                           AllocateAnyPages,
                           EfiBootServicesData,
                           MemoryBlockSizeInPages,
                           &CommonBuffer,
                           0
                           );
  if (EFI_ERROR (Status)) {
    gBS->FreePool ((*MemoryHeader)->BitArrayPtr);
    gBS->FreePool (*MemoryHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  MemoryBlockSizeInBytes = EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages);
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterCommonBuffer,
                           CommonBuffer,
                           &MemoryBlockSizeInBytes,
                           &MappedAddress,
                           &Mapping
                           );
  //
  // If returned Mapped size is less than the size
  // we request,do not support.
  //
  if (EFI_ERROR (Status) || (MemoryBlockSizeInBytes != EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages))) {
    HcDev->PciIo->FreeBuffer (HcDev->PciIo, MemoryBlockSizeInPages, CommonBuffer);
    gBS->FreePool ((*MemoryHeader)->BitArrayPtr);
    gBS->FreePool (*MemoryHeader);
    return EFI_UNSUPPORTED;
  }

  //
  // Data structure involved by host controller
  // should be restricted into the same 4G
  //
  if (HcDev->Is64BitCapable != 0) {
  	if (HcDev->High32BitAddr != GET_32B_TO_63B (MappedAddress)) {
	  HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      HcDev->PciIo->FreeBuffer (HcDev->PciIo, MemoryBlockSizeInPages, CommonBuffer);
      gBS->FreePool ((*MemoryHeader)->BitArrayPtr);
      gBS->FreePool (*MemoryHeader);
      return EFI_UNSUPPORTED;
	}
  }

  //
  // Set Memory block initial address
  //
  (*MemoryHeader)->MemoryBlockPtr = (UINT8 *) ((UINTN) MappedAddress);
  (*MemoryHeader)->Mapping        = Mapping;

  ZeroMem (
    (*MemoryHeader)->MemoryBlockPtr,
    EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages)
    );

  return EFI_SUCCESS;
}

EFI_STATUS
FreeMemoryHeader (
  IN USB2_HC_DEV               *HcDev,
  IN MEMORY_MANAGE_HEADER      *MemoryHeader
  )
/*++

Routine Description:

  Free Memory Header

Arguments:

  HcDev         - USB2_HC_DEV
  MemoryHeader  - MemoryHeader to be freed

Returns:

  EFI_SUCCESS            Success
  EFI_INVALID_PARAMETER  Parameter is error

--*/
{
  if ((MemoryHeader == NULL) || (HcDev == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // unmap the common buffer used by the memory block
  //
  HcDev->PciIo->Unmap (HcDev->PciIo, MemoryHeader->Mapping);

  //
  // free common buffer
  //
  HcDev->PciIo->FreeBuffer (
                  HcDev->PciIo,
                  EFI_SIZE_TO_PAGES (MemoryHeader->MemoryBlockSizeInBytes),
                  MemoryHeader->MemoryBlockPtr
                  );
  //
  // free bit array
  //
  gBS->FreePool (MemoryHeader->BitArrayPtr);
  //
  // free memory header
  //
  gBS->FreePool (MemoryHeader);

  return EFI_SUCCESS;
}

EFI_STATUS
EhciAllocatePool (
  IN  USB2_HC_DEV     *HcDev,
  OUT UINT8           **Pool,
  IN  UINTN           AllocSize
  )
/*++

Routine Description:

  Ehci Allocate Pool

Arguments:

  HcDev     - USB2_HC_DEV
  Pool      - Place to store pointer to the memory buffer
  AllocSize - Alloc Size

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  MEMORY_MANAGE_HEADER  *NewMemoryHeader;
  UINTN                 RealAllocSize;
  UINTN                 MemoryBlockSizeInPages;
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;

  *Pool         = NULL;

  MemoryHeader  = HcDev->MemoryHeader;
  ASSERT (MemoryHeader != NULL);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY + 1);

  //
  // allocate unit is 32 bytes (align on 32 byte)
  //
  if (AllocSize & (MEM_UNIT_SIZE - 1)) {
    RealAllocSize = (AllocSize / MEM_UNIT_SIZE + 1) * MEM_UNIT_SIZE;
  } else {
    RealAllocSize = AllocSize;
  }

  //
  // There may be linked MemoryHeaders.
  // To allocate a free pool in Memory blocks,
  // must search in the MemoryHeader link list
  // until enough free pool is found.
  //
  Status = EFI_NOT_FOUND;
  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {

    Status = AllocMemInMemoryBlock (
              TempHeaderPtr,
              (VOID **) Pool,
              RealAllocSize / MEM_UNIT_SIZE
              );
    if (!EFI_ERROR (Status)) {
       break;
    }
  }

  gBS->RestoreTPL (OldTpl);

  if (!EFI_ERROR (Status)) {
     ZeroMem (*Pool, AllocSize);
     return EFI_SUCCESS;
  }


  //
  // There is no enough memory,
  // Create a new Memory Block
  //

  //
  // if pool size is larger than NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES,
  // just allocate a large enough memory block.
  //
  if (RealAllocSize > EFI_PAGES_TO_SIZE (NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES)) {
    MemoryBlockSizeInPages = EFI_SIZE_TO_PAGES (RealAllocSize) + 1;
  } else {
    MemoryBlockSizeInPages = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  }

  Status = CreateMemoryBlock (HcDev, &NewMemoryHeader, MemoryBlockSizeInPages);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY + 1);
  	
  //
  // Link the new Memory Block to the Memory Header list
  //
  InsertMemoryHeaderToList (MemoryHeader, NewMemoryHeader);

  Status = AllocMemInMemoryBlock (
             NewMemoryHeader,
             (VOID **) Pool,
             RealAllocSize / MEM_UNIT_SIZE
             );

  gBS->RestoreTPL (OldTpl);

  if (!EFI_ERROR (Status)) {
    ZeroMem (*Pool, AllocSize);
  }

  return Status;
}

VOID
EhciFreePool (
  IN USB2_HC_DEV     *HcDev,
  IN UINT8           *Pool,
  IN UINTN           AllocSize
  )
/*++

Routine Description:

  Uhci Free Pool

Arguments:

  HcDev     - USB_HC_DEV
  Pool      - Pool to free
  AllocSize - Pool size

Returns:

  VOID

--*/
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  UINTN                 StartBytePos;
  UINTN                 Index;
  UINT8                 StartBitPos;
  UINT8                 Index2;
  UINTN                 Count;
  UINTN                 RealAllocSize;
  EFI_TPL               OldTpl;

  OldTpl        = gBS->RaiseTPL (TPL_NOTIFY + 1);

  MemoryHeader  = HcDev->MemoryHeader;

  //
  // allocate unit is 32 byte (align on 32 byte)
  //
  if (AllocSize & (MEM_UNIT_SIZE - 1)) {
    RealAllocSize = (AllocSize / MEM_UNIT_SIZE + 1) * MEM_UNIT_SIZE;
  } else {
    RealAllocSize = AllocSize;
  }

  //
  // scan the memory header linked list for
  // the asigned memory to free.
  //
  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {

    if ((Pool >= TempHeaderPtr->MemoryBlockPtr) &&
        ((Pool + RealAllocSize) <= (TempHeaderPtr->MemoryBlockPtr + TempHeaderPtr->MemoryBlockSizeInBytes))
        ) {
      //
      // Pool is in the Memory Block area,
      // find the start byte and bit in the bit array
      //
      StartBytePos  = ((Pool - TempHeaderPtr->MemoryBlockPtr) / MEM_UNIT_SIZE) / 8;
      StartBitPos   = (UINT8) (((Pool - TempHeaderPtr->MemoryBlockPtr) / MEM_UNIT_SIZE) & 0x7);

      //
      // reset associated bits in bit arry
      //
      for (Index = StartBytePos, Index2 = StartBitPos, Count = 0; Count < (RealAllocSize / MEM_UNIT_SIZE); Count++) {
        ASSERT ((TempHeaderPtr->BitArrayPtr[Index] & bit (Index2) )== bit (Index2));

        TempHeaderPtr->BitArrayPtr[Index] = (UINT8) (TempHeaderPtr->BitArrayPtr[Index] ^ (bit (Index2)));
        Index2++;
        if (Index2 == 8) {
          Index += 1;
          Index2 = 0;
        }
      }
      //
      // break the loop
      //
      break;
    }
  }

  //
  // Release emptied memory blocks (only if the memory block is not
  // the first one in the memory header list
  //
  for (TempHeaderPtr = MemoryHeader->Next; TempHeaderPtr != NULL;) {

    ASSERT (MemoryHeader->Next != NULL);

    if (IsMemoryBlockEmptied (TempHeaderPtr)) {

      DelinkMemoryBlock (MemoryHeader, TempHeaderPtr);
      //
      // when the TempHeaderPtr is freed in FreeMemoryHeader(),
      // the TempHeaderPtr is pointing to nonsense content.
      //
      gBS->RestoreTPL (OldTpl);
      FreeMemoryHeader (HcDev, TempHeaderPtr);
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY + 1);
      //
      // reset the TempHeaderPtr, continue search for
      // another empty memory block.
      //
      TempHeaderPtr = MemoryHeader->Next;
      continue;
    }

    TempHeaderPtr = TempHeaderPtr->Next;
  }

  gBS->RestoreTPL (OldTpl);
}

VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER     *MemoryHeader,
  IN MEMORY_MANAGE_HEADER     *NewMemoryHeader
  )
/*++

Routine Description:

  Insert Memory Header To List

Arguments:

  MemoryHeader    - MEMORY_MANAGE_HEADER
  NewMemoryHeader - MEMORY_MANAGE_HEADER

Returns:

  VOID

--*/
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {
    if (TempHeaderPtr->Next == NULL) {
      TempHeaderPtr->Next = NewMemoryHeader;
      break;
    }
  }
}

EFI_STATUS
AllocMemInMemoryBlock (
  IN  MEMORY_MANAGE_HEADER     *MemoryHeader,
  OUT VOID                     **Pool,
  IN  UINTN                    NumberOfMemoryUnit
  )
/*++

Routine Description:

  Alloc Memory In MemoryBlock

Arguments:

  MemoryHeader        - MEMORY_MANAGE_HEADER
  Pool                - Place to store pointer to memory
  NumberOfMemoryUnit  - Number Of Memory Unit

Returns:

  EFI_SUCCESS    Success
  EFI_NOT_FOUND  Can't find the free memory

--*/
{
  UINTN TempBytePos;
  UINTN FoundBytePos;
  UINT8 Index;
  UINT8 FoundBitPos;
  UINT8 ByteValue;
  UINT8 BitValue;
  UINTN NumberOfZeros;
  UINTN Count;

  FoundBytePos  = 0;
  FoundBitPos   = 0;
  ByteValue     = MemoryHeader->BitArrayPtr[0];
  NumberOfZeros = 0;
  Index         = 0;

  for (TempBytePos = 0; TempBytePos < MemoryHeader->BitArraySizeInBytes;) {
  	
    //
    // Pop out BitValue from a byte in TempBytePos.
    //
    BitValue = (UINT8) (ByteValue & 0x1);
	
    //
    // right shift the byte
    //
    ByteValue = (UINT8) (ByteValue >> 1);

    if (BitValue == 0) {
      //
      // Found a free bit, the NumberOfZeros only record the number
      // of those consecutive zeros
      //
      NumberOfZeros++;
      //
      // Found enough consecutive free space, break the loop
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        break;
      }
    } else {
      //
      // Encountering a '1', meant the bit is ocupied.
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        //
        // Found enough consecutive free space,break the loop
        //
        break;
      } else {
        //
        // the NumberOfZeros only record the number of those consecutive zeros,
        // so reset the NumberOfZeros to 0 when encountering '1' before finding
        // enough consecutive '0's
        //
        NumberOfZeros = 0;
        //
        // reset the (FoundBytePos,FoundBitPos) to the position of '1'
        //
        FoundBytePos  = TempBytePos;
        FoundBitPos   = Index;
      }
    }
	
    //
    // step forward a bit
    //
    Index++;
    if (Index == 8) {
      //
      // step forward a byte, getting the byte value,
      // and reset the bit pos.
      //
      TempBytePos += 1;
      ByteValue = MemoryHeader->BitArrayPtr[TempBytePos];
      Index     = 0;
    }
  }

  if (NumberOfZeros < NumberOfMemoryUnit) {
    return EFI_NOT_FOUND;
  }

  //
  // Found enough free space.
  //

  //
  // The values recorded in (FoundBytePos,FoundBitPos) have two conditions:
  //  1)(FoundBytePos,FoundBitPos) record the position
  //    of the last '1' before the consecutive '0's, it must
  //    be adjusted to the start position of the consecutive '0's.
  //  2)the start address of the consecutive '0's is just the start of
  //    the bitarray. so no need to adjust the values of
  //    (FoundBytePos,FoundBitPos).
  //
  if ((MemoryHeader->BitArrayPtr[FoundBytePos] & bit (FoundBitPos)) != 0) {
    FoundBitPos += 1;
  }

  //
  // Have the (FoundBytePos,FoundBitPos) make sense.
  //
  if (FoundBitPos > 7) {
    FoundBytePos += 1;
    FoundBitPos -= 8;
  }

  //
  // Set the memory as allocated
  //
  for (TempBytePos = FoundBytePos, Index = FoundBitPos, Count = 0; Count < NumberOfMemoryUnit; Count++) {

    ASSERT ((MemoryHeader->BitArrayPtr[TempBytePos] & bit (Index) )== 0);
    MemoryHeader->BitArrayPtr[TempBytePos] = (UINT8) (MemoryHeader->BitArrayPtr[TempBytePos] | bit (Index));
    Index++;
    if (Index == 8) {
      TempBytePos += 1;
      Index = 0;
    }
  }

  *Pool = MemoryHeader->MemoryBlockPtr + (FoundBytePos * 8 + FoundBitPos) * MEM_UNIT_SIZE;

  return EFI_SUCCESS;
}

BOOLEAN
IsMemoryBlockEmptied (
  IN MEMORY_MANAGE_HEADER     *MemoryHeaderPtr
  )
/*++

Routine Description:

  Is Memory Block Emptied

Arguments:

  MemoryHeaderPtr - MEMORY_MANAGE_HEADER

Returns:

  TRUE    Empty
  FALSE   Not Empty

--*/
{
  UINTN Index;

  for (Index = 0; Index < MemoryHeaderPtr->BitArraySizeInBytes; Index++) {
    if (MemoryHeaderPtr->BitArrayPtr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

VOID
DelinkMemoryBlock (
  IN MEMORY_MANAGE_HEADER     *FirstMemoryHeader,
  IN MEMORY_MANAGE_HEADER     *NeedFreeMemoryHeader
  )
/*++

Routine Description:

  Delink Memory Block

Arguments:

  FirstMemoryHeader     - MEMORY_MANAGE_HEADER
  NeedFreeMemoryHeader  - MEMORY_MANAGE_HEADER

Returns:

  VOID

--*/
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  if ((FirstMemoryHeader == NULL) || (NeedFreeMemoryHeader == NULL)) {
    return ;
  }

  for (TempHeaderPtr = FirstMemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {

    if (TempHeaderPtr->Next == NeedFreeMemoryHeader) {
      //
      // Link the before and after
      //
      TempHeaderPtr->Next = NeedFreeMemoryHeader->Next;
      NeedFreeMemoryHeader->Next = NULL;
      break;
    }
  }
}

EFI_STATUS
InitialMemoryManagement (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Initialize Memory Management

Arguments:

  HcDev  - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS            Status;
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  UINTN                 MemPages;

  MemPages  = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  Status    = CreateMemoryBlock (HcDev, &MemoryHeader, MemPages);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  HcDev->MemoryHeader = MemoryHeader;

exit:
  return Status;
}

EFI_STATUS
DeinitialMemoryManagement (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Deinitialize Memory Management

Arguments:

  HcDev   - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  for (TempHeaderPtr = HcDev->MemoryHeader->Next; TempHeaderPtr != NULL;) {

    DelinkMemoryBlock (HcDev->MemoryHeader, TempHeaderPtr);
    //
    // when the TempHeaderPtr is freed in FreeMemoryHeader(),
    // the TempHeaderPtr is pointing to nonsense content.
    //
    FreeMemoryHeader (HcDev, TempHeaderPtr);
    //
    // reset the TempHeaderPtr,continue free another memory block.
    //
    TempHeaderPtr = HcDev->MemoryHeader->Next;
  }

  FreeMemoryHeader (HcDev, HcDev->MemoryHeader);

  return EFI_SUCCESS;
}
