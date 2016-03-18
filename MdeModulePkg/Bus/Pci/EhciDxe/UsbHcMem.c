/** @file

  Routine procedures for memory allocate/free.

Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ehci.h"


/**
  Allocate a block of memory to be used by the buffer pool.

  @param  Pool           The buffer pool to allocate memory for.
  @param  Pages          How many pages to allocate.

  @return The allocated memory block or NULL if failed.

**/
USBHC_MEM_BLOCK *
UsbHcAllocMemBlock (
  IN  USBHC_MEM_POOL      *Pool,
  IN  UINTN               Pages
  )
{
  USBHC_MEM_BLOCK         *Block;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  VOID                    *BufHost;
  VOID                    *Mapping;
  EFI_PHYSICAL_ADDRESS    MappedAddr;
  UINTN                   Bytes;
  EFI_STATUS              Status;

  PciIo = Pool->PciIo;

  Block = AllocateZeroPool (sizeof (USBHC_MEM_BLOCK));
  if (Block == NULL) {
    return NULL;
  }

  //
  // each bit in the bit array represents USBHC_MEM_UNIT
  // bytes of memory in the memory block.
  //
  ASSERT (USBHC_MEM_UNIT * 8 <= EFI_PAGE_SIZE);

  Block->BufLen   = EFI_PAGES_TO_SIZE (Pages);
  Block->BitsLen  = Block->BufLen / (USBHC_MEM_UNIT * 8);
  Block->Bits     = AllocateZeroPool (Block->BitsLen);

  if (Block->Bits == NULL) {
    gBS->FreePool (Block);
    return NULL;
  }

  //
  // Allocate the number of Pages of memory, then map it for
  // bus master read and write.
  //
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    Pages,
                    &BufHost,
                    0
                    );

  if (EFI_ERROR (Status)) {
    goto FREE_BITARRAY;
  }

  Bytes = EFI_PAGES_TO_SIZE (Pages);
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    BufHost,
                    &Bytes,
                    &MappedAddr,
                    &Mapping
                    );

  if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (Pages))) {
    goto FREE_BUFFER;
  }

  //
  // Check whether the data structure used by the host controller
  // should be restricted into the same 4G
  //
  if (Pool->Check4G && (Pool->Which4G != USB_HC_HIGH_32BIT (MappedAddr))) {
    PciIo->Unmap (PciIo, Mapping);
    goto FREE_BUFFER;
  }

  Block->BufHost  = BufHost;
  Block->Buf      = (UINT8 *) ((UINTN) MappedAddr);
  Block->Mapping  = Mapping;

  return Block;

FREE_BUFFER:
  PciIo->FreeBuffer (PciIo, Pages, BufHost);

FREE_BITARRAY:
  gBS->FreePool (Block->Bits);
  gBS->FreePool (Block);
  return NULL;
}


/**
  Free the memory block from the memory pool.

  @param  Pool           The memory pool to free the block from.
  @param  Block          The memory block to free.

**/
VOID
UsbHcFreeMemBlock (
  IN USBHC_MEM_POOL       *Pool,
  IN USBHC_MEM_BLOCK      *Block
  )
{
  EFI_PCI_IO_PROTOCOL     *PciIo;

  ASSERT ((Pool != NULL) && (Block != NULL));

  PciIo = Pool->PciIo;

  //
  // Unmap the common buffer then free the structures
  //
  PciIo->Unmap (PciIo, Block->Mapping);
  PciIo->FreeBuffer (PciIo, EFI_SIZE_TO_PAGES (Block->BufLen), Block->BufHost);

  gBS->FreePool (Block->Bits);
  gBS->FreePool (Block);
}


/**
  Alloc some memory from the block.

  @param  Block          The memory block to allocate memory from.
  @param  Units          Number of memory units to allocate.

  @return The pointer to the allocated memory. If couldn't allocate the needed memory,
          the return value is NULL.

**/
VOID *
UsbHcAllocMemFromBlock (
  IN  USBHC_MEM_BLOCK     *Block,
  IN  UINTN               Units
  )
{
  UINTN                   Byte;
  UINT8                   Bit;
  UINTN                   StartByte;
  UINT8                   StartBit;
  UINTN                   Available;
  UINTN                   Count;

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

    Block->Bits[Byte] = (UINT8) (Block->Bits[Byte] | USB_HC_BIT (Bit));
    NEXT_BIT (Byte, Bit);
  }

  return Block->BufHost + (StartByte * 8 + StartBit) * USBHC_MEM_UNIT;
}

/**
  Calculate the corresponding pci bus address according to the Mem parameter.

  @param  Pool           The memory pool of the host controller.
  @param  Mem            The pointer to host memory.
  @param  Size           The size of the memory region.

  @return the pci memory address
**/
EFI_PHYSICAL_ADDRESS
UsbHcGetPciAddressForHostMem (
  IN USBHC_MEM_POOL       *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  )
{
  USBHC_MEM_BLOCK         *Head;
  USBHC_MEM_BLOCK         *Block;
  UINTN                   AllocSize;
  EFI_PHYSICAL_ADDRESS    PhyAddr;
  UINTN                   Offset;

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
  Offset = (UINT8 *)Mem - Block->BufHost;
  PhyAddr = (EFI_PHYSICAL_ADDRESS)(UINTN) (Block->Buf + Offset);
  return PhyAddr;
}


/**
  Insert the memory block to the pool's list of the blocks.

  @param  Head           The head of the memory pool's block list.
  @param  Block          The memory block to insert.

**/
VOID
UsbHcInsertMemBlockToPool (
  IN USBHC_MEM_BLOCK      *Head,
  IN USBHC_MEM_BLOCK      *Block
  )
{
  ASSERT ((Head != NULL) && (Block != NULL));
  Block->Next = Head->Next;
  Head->Next  = Block;
}


/**
  Is the memory block empty?

  @param  Block   The memory block to check.

  @retval TRUE    The memory block is empty.
  @retval FALSE   The memory block isn't empty.

**/
BOOLEAN
UsbHcIsMemBlockEmpty (
  IN USBHC_MEM_BLOCK     *Block
  )
{
  UINTN                   Index;

  for (Index = 0; Index < Block->BitsLen; Index++) {
    if (Block->Bits[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Unlink the memory block from the pool's list.

  @param  Head           The block list head of the memory's pool.
  @param  BlockToUnlink  The memory block to unlink.

**/
VOID
UsbHcUnlinkMemBlock (
  IN USBHC_MEM_BLOCK      *Head,
  IN USBHC_MEM_BLOCK      *BlockToUnlink
  )
{
  USBHC_MEM_BLOCK         *Block;

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

  @param  PciIo                The PciIo that can be used to access the host controller.
  @param  Check4G              Whether the host controller requires allocated memory
                               from one 4G address space.
  @param  Which4G              The 4G memory area each memory allocated should be from.

  @retval EFI_SUCCESS          The memory pool is initialized.
  @retval EFI_OUT_OF_RESOURCE  Fail to init the memory pool.

**/
USBHC_MEM_POOL *
UsbHcInitMemPool (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN BOOLEAN              Check4G,
  IN UINT32               Which4G
  )
{
  USBHC_MEM_POOL          *Pool;

  Pool = AllocatePool (sizeof (USBHC_MEM_POOL));

  if (Pool == NULL) {
    return Pool;
  }

  Pool->PciIo   = PciIo;
  Pool->Check4G = Check4G;
  Pool->Which4G = Which4G;
  Pool->Head    = UsbHcAllocMemBlock (Pool, USBHC_MEM_DEFAULT_PAGES);

  if (Pool->Head == NULL) {
    gBS->FreePool (Pool);
    Pool = NULL;
  }

  return Pool;
}


/**
  Release the memory management pool.

  @param  Pool              The USB memory pool to free.

  @retval EFI_SUCCESS       The memory pool is freed.
  @retval EFI_DEVICE_ERROR  Failed to free the memory pool.

**/
EFI_STATUS
UsbHcFreeMemPool (
  IN USBHC_MEM_POOL       *Pool
  )
{
  USBHC_MEM_BLOCK *Block;

  ASSERT (Pool->Head != NULL);

  //
  // Unlink all the memory blocks from the pool, then free them.
  // UsbHcUnlinkMemBlock can't be used to unlink and free the
  // first block.
  //
  for (Block = Pool->Head->Next; Block != NULL; Block = Pool->Head->Next) {
    UsbHcUnlinkMemBlock (Pool->Head, Block);
    UsbHcFreeMemBlock (Pool, Block);
  }

  UsbHcFreeMemBlock (Pool, Pool->Head);
  gBS->FreePool (Pool);
  return EFI_SUCCESS;
}


/**
  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

  @param  Pool           The host controller's memory pool.
  @param  Size           Size of the memory to allocate.

  @return The allocated memory or NULL.

**/
VOID *
UsbHcAllocateMem (
  IN  USBHC_MEM_POOL      *Pool,
  IN  UINTN               Size
  )
{
  USBHC_MEM_BLOCK         *Head;
  USBHC_MEM_BLOCK         *Block;
  USBHC_MEM_BLOCK         *NewBlock;
  VOID                    *Mem;
  UINTN                   AllocSize;
  UINTN                   Pages;

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
    Pages = EFI_SIZE_TO_PAGES (AllocSize) + 1;
  } else {
    Pages = USBHC_MEM_DEFAULT_PAGES;
  }

  NewBlock = UsbHcAllocMemBlock (Pool, Pages);

  if (NewBlock == NULL) {
    DEBUG ((EFI_D_ERROR, "UsbHcAllocateMem: failed to allocate block\n"));
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

  @param  Pool           The memory pool of the host controller.
  @param  Mem            The memory to free.
  @param  Size           The size of the memory to free.

**/
VOID
UsbHcFreeMem (
  IN USBHC_MEM_POOL       *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  )
{
  USBHC_MEM_BLOCK         *Head;
  USBHC_MEM_BLOCK         *Block;
  UINT8                   *ToFree;
  UINTN                   AllocSize;
  UINTN                   Byte;
  UINTN                   Bit;
  UINTN                   Count;

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
  // the caller has passed in a wrong memory point
  //
  ASSERT (Block != NULL);

  //
  // Release the current memory block if it is empty and not the head
  //
  if ((Block != Head) && UsbHcIsMemBlockEmpty (Block)) {
    UsbHcUnlinkMemBlock (Head, Block);
    UsbHcFreeMemBlock (Pool, Block);
  }

  return ;
}
