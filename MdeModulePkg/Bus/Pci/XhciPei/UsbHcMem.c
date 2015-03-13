/** @file
PEIM to produce gPeiUsb2HostControllerPpiGuid based on gPeiUsbControllerPpiGuid
which is used to enable recovery function from USB Drivers.

Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "XhcPeim.h"

/**
  Allocate a block of memory to be used by the buffer pool.

  @param  Pages         How many pages to allocate.

  @return Pointer to the allocated memory block or NULL if failed.

**/
USBHC_MEM_BLOCK *
UsbHcAllocMemBlock (
  IN UINTN              Pages
  )
{
  USBHC_MEM_BLOCK       *Block;
  EFI_STATUS            Status;
  UINTN                 PageNumber;
  EFI_PHYSICAL_ADDRESS  TempPtr;

  PageNumber = EFI_SIZE_TO_PAGES (sizeof (USBHC_MEM_BLOCK));
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             PageNumber,
             &TempPtr
             );

  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ZeroMem ((VOID *) (UINTN) TempPtr, EFI_PAGES_TO_SIZE (PageNumber));

  //
  // each bit in the bit array represents USBHC_MEM_UNIT
  // bytes of memory in the memory block.
  //
  ASSERT (USBHC_MEM_UNIT * 8 <= EFI_PAGE_SIZE);

  Block = (USBHC_MEM_BLOCK *) (UINTN) TempPtr;
  Block->BufLen = EFI_PAGES_TO_SIZE (Pages);
  Block->BitsLen = Block->BufLen / (USBHC_MEM_UNIT * 8);

  PageNumber = EFI_SIZE_TO_PAGES (Block->BitsLen);
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             PageNumber,
             &TempPtr
             );

  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ZeroMem ((VOID *) (UINTN) TempPtr, EFI_PAGES_TO_SIZE (PageNumber));

  Block->Bits = (UINT8 *) (UINTN) TempPtr;

  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             Pages,
             &TempPtr
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ZeroMem ((VOID *) (UINTN) TempPtr, EFI_PAGES_TO_SIZE (Pages));

  Block->BufHost = (UINT8 *) (UINTN) TempPtr;;
  Block->Buf = (UINT8 *) (UINTN) TempPtr;
  Block->Next = NULL;

  return Block;
}

/**
  Free the memory block from the memory pool.

  @param  Pool          The memory pool to free the block from.
  @param  Block         The memory block to free.

**/
VOID
UsbHcFreeMemBlock (
  IN USBHC_MEM_POOL     *Pool,
  IN USBHC_MEM_BLOCK    *Block
  )
{
  ASSERT ((Pool != NULL) && (Block != NULL));
  //
  // No free memory in PEI.
  //
}

/**
  Alloc some memory from the block.

  @param  Block         The memory block to allocate memory from.
  @param  Units         Number of memory units to allocate.

  @return The pointer to the allocated memory.
          If couldn't allocate the needed memory, the return value is NULL.

**/
VOID *
UsbHcAllocMemFromBlock (
  IN USBHC_MEM_BLOCK    *Block,
  IN UINTN              Units
  )
{
  UINTN                 Byte;
  UINT8                 Bit;
  UINTN                 StartByte;
  UINT8                 StartBit;
  UINTN                 Available;
  UINTN                 Count;

  ASSERT ((Block != 0) && (Units != 0));

  StartByte  = 0;
  StartBit   = 0;
  Available  = 0;

  for (Byte = 0, Bit = 0; Byte < Block->BitsLen;) {
    //
    // If current bit is zero, the corresponding memory unit is
    // available, otherwise we need to restart our searching.
    // Available counts the consective number of zero bit.
    //
    if (!USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit)) {
      Available++;

      if (Available >= Units) {
        break;
      }

      NEXT_BIT (Byte, Bit);
    } else {
      NEXT_BIT (Byte, Bit);

      Available  = 0;
      StartByte  = Byte;
      StartBit   = Bit;
    }
  }

  if (Available < Units) {
    return NULL;
  }

  //
  // Mark the memory as allocated
  //
  Byte  = StartByte;
  Bit   = StartBit;

  for (Count = 0; Count < Units; Count++) {
    ASSERT (!USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit));

    Block->Bits[Byte] = (UINT8) (Block->Bits[Byte] | (UINT8) USB_HC_BIT (Bit));
    NEXT_BIT (Byte, Bit);
  }

  return Block->BufHost + (StartByte * 8 + StartBit) * USBHC_MEM_UNIT;
}

/**
  Calculate the corresponding pci bus address according to the Mem parameter.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The pointer to host memory.
  @param  Size          The size of the memory region.

  @return               The pci memory address

**/
EFI_PHYSICAL_ADDRESS
UsbHcGetPciAddrForHostAddr (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  )
{
  USBHC_MEM_BLOCK       *Head;
  USBHC_MEM_BLOCK       *Block;
  UINTN                 AllocSize;
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  UINTN                 Offset;

  Head      = Pool->Head;
  AllocSize = USBHC_MEM_ROUND (Size);

  if (Mem == NULL) {
    return 0;
  }

  for (Block = Head; Block != NULL; Block = Block->Next) {
    //
    // scan the memory block list for the memory block that
    // completely contains the allocated memory.
    //
    if ((Block->BufHost <= (UINT8 *) Mem) && (((UINT8 *) Mem + AllocSize) <= (Block->BufHost + Block->BufLen))) {
      break;
    }
  }

  ASSERT ((Block != NULL));
  //
  // calculate the pci memory address for host memory address.
  //
  Offset = (UINT8 *) Mem - Block->BufHost;
  PhyAddr = (EFI_PHYSICAL_ADDRESS) (UINTN) (Block->Buf + Offset);
  return PhyAddr;
}

/**
  Calculate the corresponding host address according to the pci address.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The pointer to pci memory.
  @param  Size          The size of the memory region.

  @return               The host memory address

**/
EFI_PHYSICAL_ADDRESS
UsbHcGetHostAddrForPciAddr (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  )
{
  USBHC_MEM_BLOCK       *Head;
  USBHC_MEM_BLOCK       *Block;
  UINTN                 AllocSize;
  EFI_PHYSICAL_ADDRESS  HostAddr;
  UINTN                 Offset;

  Head      = Pool->Head;
  AllocSize = USBHC_MEM_ROUND (Size);

  if (Mem == NULL) {
    return 0;
  }

  for (Block = Head; Block != NULL; Block = Block->Next) {
    //
    // scan the memory block list for the memory block that
    // completely contains the allocated memory.
    //
    if ((Block->Buf <= (UINT8 *) Mem) && (((UINT8 *) Mem + AllocSize) <= (Block->Buf + Block->BufLen))) {
      break;
    }
  }

  ASSERT ((Block != NULL));
  //
  // calculate the host memory address for pci memory address.
  //
  Offset = (UINT8 *) Mem - Block->Buf;
  HostAddr = (EFI_PHYSICAL_ADDRESS) (UINTN) (Block->BufHost + Offset);
  return HostAddr;
}

/**
  Insert the memory block to the pool's list of the blocks.

  @param  Head          The head of the memory pool's block list.
  @param  Block         The memory block to insert.

**/
VOID
UsbHcInsertMemBlockToPool (
  IN USBHC_MEM_BLOCK    *Head,
  IN USBHC_MEM_BLOCK    *Block
  )
{
  ASSERT ((Head != NULL) && (Block != NULL));
  Block->Next = Head->Next;
  Head->Next  = Block;
}

/**
  Is the memory block empty?

  @param  Block         The memory block to check.

  @retval TRUE          The memory block is empty.
  @retval FALSE         The memory block isn't empty.

**/
BOOLEAN
UsbHcIsMemBlockEmpty (
  IN USBHC_MEM_BLOCK    *Block
  )
{
  UINTN Index;

  for (Index = 0; Index < Block->BitsLen; Index++) {
    if (Block->Bits[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Unlink the memory block from the pool's list.

  @param  Head          The block list head of the memory's pool.
  @param  BlockToUnlink The memory block to unlink.

**/
VOID
UsbHcUnlinkMemBlock (
  IN USBHC_MEM_BLOCK    *Head,
  IN USBHC_MEM_BLOCK    *BlockToUnlink
  )
{
  USBHC_MEM_BLOCK       *Block;

  ASSERT ((Head != NULL) && (BlockToUnlink != NULL));

  for (Block = Head; Block != NULL; Block = Block->Next) {
    if (Block->Next == BlockToUnlink) {
      Block->Next         = BlockToUnlink->Next;
      BlockToUnlink->Next = NULL;
      break;
    }
  }
}

/**
  Initialize the memory management pool for the host controller.

  @return Pointer to the allocated memory pool or NULL if failed.

**/
USBHC_MEM_POOL *
UsbHcInitMemPool (
  VOID
  )
{
  USBHC_MEM_POOL        *Pool;
  UINTN                 PageNumber;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  TempPtr;

  PageNumber = EFI_SIZE_TO_PAGES (sizeof (USBHC_MEM_POOL));
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             PageNumber,
             &TempPtr
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ZeroMem ((VOID *) (UINTN) TempPtr, EFI_PAGES_TO_SIZE (PageNumber));

  Pool = (USBHC_MEM_POOL *) ((UINTN) TempPtr);
  Pool->Head = UsbHcAllocMemBlock (USBHC_MEM_DEFAULT_PAGES);

  if (Pool->Head == NULL) {
    //
    // No free memory in PEI.
    //
    Pool = NULL;
  }

  return Pool;
}

/**
  Release the memory management pool.

  @param  Pool          The USB memory pool to free.

**/
VOID
UsbHcFreeMemPool (
  IN USBHC_MEM_POOL     *Pool
  )
{
  USBHC_MEM_BLOCK       *Block;

  ASSERT (Pool->Head != NULL);

  //
  // Unlink all the memory blocks from the pool, then free them.
  // UsbHcUnlinkMemBlock can't be used to unlink and free the
  // first block.
  //
  for (Block = Pool->Head->Next; Block != NULL; Block = Pool->Head->Next) {
    //UsbHcUnlinkMemBlock (Pool->Head, Block);
    UsbHcFreeMemBlock (Pool, Block);
  }

  UsbHcFreeMemBlock (Pool, Pool->Head);
}

/**
  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

  @param  Pool          The host controller's memory pool.
  @param  Size          Size of the memory to allocate.

  @return The allocated memory or NULL.

**/
VOID *
UsbHcAllocateMem (
  IN USBHC_MEM_POOL     *Pool,
  IN UINTN              Size
  )
{
  USBHC_MEM_BLOCK       *Head;
  USBHC_MEM_BLOCK       *Block;
  USBHC_MEM_BLOCK       *NewBlock;
  VOID                  *Mem;
  UINTN                 AllocSize;
  UINTN                 Pages;

  Mem       = NULL;
  AllocSize = USBHC_MEM_ROUND (Size);
  Head      = Pool->Head;
  ASSERT (Head != NULL);

  //
  // First check whether current memory blocks can satisfy the allocation.
  //
  for (Block = Head; Block != NULL; Block = Block->Next) {
    Mem = UsbHcAllocMemFromBlock (Block, AllocSize / USBHC_MEM_UNIT);

    if (Mem != NULL) {
      ZeroMem (Mem, Size);
      break;
    }
  }

  if (Mem != NULL) {
    return Mem;
  }

  //
  // Create a new memory block if there is not enough memory
  // in the pool. If the allocation size is larger than the
  // default page number, just allocate a large enough memory
  // block. Otherwise allocate default pages.
  //
  if (AllocSize > EFI_PAGES_TO_SIZE (USBHC_MEM_DEFAULT_PAGES)) {
    Pages = EFI_SIZE_TO_PAGES (AllocSize);
  } else {
    Pages = USBHC_MEM_DEFAULT_PAGES;
  }
  NewBlock = UsbHcAllocMemBlock (Pages);

  if (NewBlock == NULL) {
    return NULL;
  }

  //
  // Add the new memory block to the pool, then allocate memory from it
  //
  UsbHcInsertMemBlockToPool (Head, NewBlock);
  Mem = UsbHcAllocMemFromBlock (NewBlock, AllocSize / USBHC_MEM_UNIT);

  if (Mem != NULL) {
    ZeroMem (Mem, Size);
  }

  return Mem;
}

/**
  Free the allocated memory back to the memory pool.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The memory to free.
  @param  Size          The size of the memory to free.

**/
VOID
UsbHcFreeMem (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  )
{
  USBHC_MEM_BLOCK       *Head;
  USBHC_MEM_BLOCK       *Block;
  UINT8                 *ToFree;
  UINTN                 AllocSize;
  UINTN                 Byte;
  UINTN                 Bit;
  UINTN                 Count;

  Head      = Pool->Head;
  AllocSize = USBHC_MEM_ROUND (Size);
  ToFree    = (UINT8 *) Mem;

  for (Block = Head; Block != NULL; Block = Block->Next) {
    //
    // scan the memory block list for the memory block that
    // completely contains the memory to free.
    //
    if ((Block->BufHost <= ToFree) && ((ToFree + AllocSize) <= (Block->BufHost + Block->BufLen))) {
      //
      // compute the start byte and bit in the bit array
      //
      Byte  = ((ToFree - Block->BufHost) / USBHC_MEM_UNIT) / 8;
      Bit   = ((ToFree - Block->BufHost) / USBHC_MEM_UNIT) % 8;

      //
      // reset associated bits in bit arry
      //
      for (Count = 0; Count < (AllocSize / USBHC_MEM_UNIT); Count++) {
        ASSERT (USB_HC_BIT_IS_SET (Block->Bits[Byte], Bit));

        Block->Bits[Byte] = (UINT8) (Block->Bits[Byte] ^ USB_HC_BIT (Bit));
        NEXT_BIT (Byte, Bit);
      }

      break;
    }
  }

  //
  // If Block == NULL, it means that the current memory isn't
  // in the host controller's pool. This is critical because
  // the caller has passed in a wrong memory pointer
  //
  ASSERT (Block != NULL);

  //
  // Release the current memory block if it is empty and not the head
  //
  if ((Block != Head) && UsbHcIsMemBlockEmpty (Block)) {
    //UsbHcUnlinkMemBlock (Head, Block);
    UsbHcFreeMemBlock (Pool, Block);
  }
}

/**
  Allocates pages at a specified alignment.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  Pages                 The number of pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to
                                use to access the hosts HostAddress.

  @retval EFI_SUCCESS           Success to allocate aligned pages.
  @retval EFI_INVALID_PARAMETER Pages or Alignment is not valid.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to allocate memory.

**/
EFI_STATUS
UsbHcAllocateAlignedPages (
  IN UINTN                      Pages,
  IN UINTN                      Alignment,
  OUT VOID                      **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS      *DeviceAddress
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 AlignedMemory;
  UINTN                 AlignmentMask;
  UINTN                 RealPages;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if ((Alignment & (Alignment - 1)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Pages == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Alignment > EFI_PAGE_SIZE) {
    //
    // Calculate the total number of pages since alignment is larger than page size.
    //
    AlignmentMask  = Alignment - 1;
    RealPages      = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
    //
    ASSERT (RealPages > Pages);

    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               Pages,
               &Memory
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    AlignedMemory = ((UINTN) Memory + AlignmentMask) & ~AlignmentMask;
  } else {
    //
    // Do not over-allocate pages in this case.
    //
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               Pages,
               &Memory
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    AlignedMemory = (UINTN) Memory;
  }

  *HostAddress = (VOID *) AlignedMemory;
  *DeviceAddress = (EFI_PHYSICAL_ADDRESS) AlignedMemory;

  return EFI_SUCCESS;
}

/**
  Frees memory that was allocated with UsbHcAllocateAlignedPages().

  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  Pages                 The number of pages to free.

**/
VOID
UsbHcFreeAlignedPages (
  IN VOID               *HostAddress,
  IN UINTN              Pages
  )
{
  ASSERT (Pages != 0);
  //
  // No free memory in PEI.
  //
}

