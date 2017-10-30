/** @file

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EmmcBlockIoPei.h"

/**
  Allocate a block of memory to be used by the buffer pool.

  @param  Pages          How many pages to allocate.

  @return The allocated memory block or NULL if failed.

**/
EMMC_PEIM_MEM_BLOCK *
EmmcPeimAllocMemBlock (
  IN  UINTN                    Pages
  )
{
  EMMC_PEIM_MEM_BLOCK          *Block;
  VOID                         *BufHost;
  VOID                         *Mapping;
  EFI_PHYSICAL_ADDRESS         MappedAddr;
  EFI_STATUS                   Status;
  VOID                         *TempPtr;

  TempPtr = NULL;
  Block   = NULL;

  Status = PeiServicesAllocatePool (sizeof(EMMC_PEIM_MEM_BLOCK), &TempPtr);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem ((VOID*)(UINTN)TempPtr, sizeof(EMMC_PEIM_MEM_BLOCK));

  //
  // each bit in the bit array represents EMMC_PEIM_MEM_UNIT
  // bytes of memory in the memory block.
  //
  ASSERT (EMMC_PEIM_MEM_UNIT * 8 <= EFI_PAGE_SIZE);

  Block = (EMMC_PEIM_MEM_BLOCK*)(UINTN)TempPtr;
  Block->BufLen   = EFI_PAGES_TO_SIZE (Pages);
  Block->BitsLen  = Block->BufLen / (EMMC_PEIM_MEM_UNIT * 8);

  Status = PeiServicesAllocatePool (Block->BitsLen, &TempPtr);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem ((VOID*)(UINTN)TempPtr, Block->BitsLen);

  Block->Bits = (UINT8*)(UINTN)TempPtr;

  Status = IoMmuAllocateBuffer (
             Pages,
             &BufHost,
             &MappedAddr,
             &Mapping
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem ((VOID*)(UINTN)BufHost, EFI_PAGES_TO_SIZE (Pages));

  Block->BufHost = (UINT8 *) (UINTN) BufHost;
  Block->Buf     = (UINT8 *) (UINTN) MappedAddr;
  Block->Mapping = Mapping;
  Block->Next    = NULL;

  return Block;
}

/**
  Free the memory block from the memory pool.

  @param  Pool           The memory pool to free the block from.
  @param  Block          The memory block to free.

**/
VOID
EmmcPeimFreeMemBlock (
  IN EMMC_PEIM_MEM_POOL       *Pool,
  IN EMMC_PEIM_MEM_BLOCK      *Block
  )
{
  ASSERT ((Pool != NULL) && (Block != NULL));

  IoMmuFreeBuffer (EFI_SIZE_TO_PAGES (Block->BufLen), Block->BufHost, Block->Mapping);
}

/**
  Alloc some memory from the block.

  @param  Block          The memory block to allocate memory from.
  @param  Units          Number of memory units to allocate.

  @return The pointer to the allocated memory. If couldn't allocate the needed memory,
          the return value is NULL.

**/
VOID *
EmmcPeimAllocMemFromBlock (
  IN  EMMC_PEIM_MEM_BLOCK *Block,
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
    if (!EMMC_PEIM_MEM_BIT_IS_SET (Block->Bits[Byte], Bit)) {
      Available++;

      if (Available >= Units) {
        break;
      }

      EMMC_PEIM_NEXT_BIT (Byte, Bit);

    } else {
      EMMC_PEIM_NEXT_BIT (Byte, Bit);

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
    ASSERT (!EMMC_PEIM_MEM_BIT_IS_SET (Block->Bits[Byte], Bit));

    Block->Bits[Byte] = (UINT8) (Block->Bits[Byte] | (UINT8) EMMC_PEIM_MEM_BIT (Bit));
    EMMC_PEIM_NEXT_BIT (Byte, Bit);
  }

  return Block->Buf + (StartByte * 8 + StartBit) * EMMC_PEIM_MEM_UNIT;
}

/**
  Insert the memory block to the pool's list of the blocks.

  @param  Head           The head of the memory pool's block list.
  @param  Block          The memory block to insert.

**/
VOID
EmmcPeimInsertMemBlockToPool (
  IN EMMC_PEIM_MEM_BLOCK      *Head,
  IN EMMC_PEIM_MEM_BLOCK      *Block
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
EmmcPeimIsMemBlockEmpty (
  IN EMMC_PEIM_MEM_BLOCK     *Block
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
EmmcPeimUnlinkMemBlock (
  IN EMMC_PEIM_MEM_BLOCK      *Head,
  IN EMMC_PEIM_MEM_BLOCK      *BlockToUnlink
  )
{
  EMMC_PEIM_MEM_BLOCK         *Block;

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

  @param  Private               The Emmc Peim driver private data.

  @retval EFI_SUCCESS           The memory pool is initialized.
  @retval Others                Fail to init the memory pool.

**/
EFI_STATUS
EmmcPeimInitMemPool (
  IN  EMMC_PEIM_HC_PRIVATE_DATA  *Private
  )
{
  EMMC_PEIM_MEM_POOL         *Pool;
  EFI_STATUS                 Status;
  VOID                       *TempPtr;

  TempPtr = NULL;
  Pool    = NULL;

  Status = PeiServicesAllocatePool (sizeof (EMMC_PEIM_MEM_POOL), &TempPtr);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem ((VOID*)(UINTN)TempPtr, sizeof (EMMC_PEIM_MEM_POOL));

  Pool = (EMMC_PEIM_MEM_POOL *)((UINTN)TempPtr);

  Pool->Head = EmmcPeimAllocMemBlock (EMMC_PEIM_MEM_DEFAULT_PAGES);

  if (Pool->Head == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Pool = Pool;
  return EFI_SUCCESS;
}

/**
  Release the memory management pool.

  @param  Pool                  The memory pool to free.

  @retval EFI_DEVICE_ERROR      Fail to free the memory pool.
  @retval EFI_SUCCESS           The memory pool is freed.

**/
EFI_STATUS
EmmcPeimFreeMemPool (
  IN EMMC_PEIM_MEM_POOL       *Pool
  )
{
  EMMC_PEIM_MEM_BLOCK         *Block;

  ASSERT (Pool->Head != NULL);

  //
  // Unlink all the memory blocks from the pool, then free them.
  // EmmcPeimUnlinkMemBlock can't be used to unlink and free the
  // first block.
  //
  for (Block = Pool->Head->Next; Block != NULL; Block = Pool->Head->Next) {
    EmmcPeimFreeMemBlock (Pool, Block);
  }

  EmmcPeimFreeMemBlock (Pool, Pool->Head);

  return EFI_SUCCESS;
}

/**
  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

  @param  Pool      The host controller's memory pool.
  @param  Size      Size of the memory to allocate.

  @return The allocated memory or NULL.

**/
VOID *
EmmcPeimAllocateMem (
  IN  EMMC_PEIM_MEM_POOL     *Pool,
  IN  UINTN                  Size
  )
{
  EMMC_PEIM_MEM_BLOCK        *Head;
  EMMC_PEIM_MEM_BLOCK        *Block;
  EMMC_PEIM_MEM_BLOCK        *NewBlock;
  VOID                       *Mem;
  UINTN                      AllocSize;
  UINTN                      Pages;

  Mem       = NULL;
  AllocSize = EMMC_PEIM_MEM_ROUND (Size);
  Head      = Pool->Head;
  ASSERT (Head != NULL);

  //
  // First check whether current memory blocks can satisfy the allocation.
  //
  for (Block = Head; Block != NULL; Block = Block->Next) {
    Mem = EmmcPeimAllocMemFromBlock (Block, AllocSize / EMMC_PEIM_MEM_UNIT);

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
  if (AllocSize > EFI_PAGES_TO_SIZE (EMMC_PEIM_MEM_DEFAULT_PAGES)) {
    Pages = EFI_SIZE_TO_PAGES (AllocSize) + 1;
  } else {
    Pages = EMMC_PEIM_MEM_DEFAULT_PAGES;
  }

  NewBlock = EmmcPeimAllocMemBlock (Pages);
  if (NewBlock == NULL) {
    return NULL;
  }

  //
  // Add the new memory block to the pool, then allocate memory from it
  //
  EmmcPeimInsertMemBlockToPool (Head, NewBlock);
  Mem = EmmcPeimAllocMemFromBlock (NewBlock, AllocSize / EMMC_PEIM_MEM_UNIT);

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
EmmcPeimFreeMem (
  IN EMMC_PEIM_MEM_POOL   *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  )
{
  EMMC_PEIM_MEM_BLOCK     *Head;
  EMMC_PEIM_MEM_BLOCK     *Block;
  UINT8                   *ToFree;
  UINTN                   AllocSize;
  UINTN                   Byte;
  UINTN                   Bit;
  UINTN                   Count;

  Head      = Pool->Head;
  AllocSize = EMMC_PEIM_MEM_ROUND (Size);
  ToFree    = (UINT8 *) Mem;

  for (Block = Head; Block != NULL; Block = Block->Next) {
    //
    // scan the memory block list for the memory block that
    // completely contains the memory to free.
    //
    if ((Block->Buf <= ToFree) && ((ToFree + AllocSize) <= (Block->Buf + Block->BufLen))) {
      //
      // compute the start byte and bit in the bit array
      //
      Byte  = ((ToFree - Block->Buf) / EMMC_PEIM_MEM_UNIT) / 8;
      Bit   = ((ToFree - Block->Buf) / EMMC_PEIM_MEM_UNIT) % 8;

      //
      // reset associated bits in bit array
      //
      for (Count = 0; Count < (AllocSize / EMMC_PEIM_MEM_UNIT); Count++) {
        ASSERT (EMMC_PEIM_MEM_BIT_IS_SET (Block->Bits[Byte], Bit));

        Block->Bits[Byte] = (UINT8) (Block->Bits[Byte] ^ EMMC_PEIM_MEM_BIT (Bit));
        EMMC_PEIM_NEXT_BIT (Byte, Bit);
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
  if ((Block != Head) && EmmcPeimIsMemBlockEmpty (Block)) {
    EmmcPeimFreeMemBlock (Pool, Block);
  }

  return ;
}
