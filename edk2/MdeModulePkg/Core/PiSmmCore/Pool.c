/** @file
  SMM Memory pool management functions.

  Copyright (c) 2009 - 2010, Intel Corporation.  All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PiSmmCore.h"

//
// MIN_POOL_SHIFT must not be less than 5
//
#define MIN_POOL_SHIFT  6
#define MIN_POOL_SIZE   (1 << MIN_POOL_SHIFT)

//
// MAX_POOL_SHIFT must not be less than EFI_PAGE_SHIFT - 1
//
#define MAX_POOL_SHIFT  (EFI_PAGE_SHIFT - 1)
#define MAX_POOL_SIZE   (1 << MAX_POOL_SHIFT)

//
// MAX_POOL_INDEX are calculated by maximum and minimum pool sizes
//
#define MAX_POOL_INDEX  (MAX_POOL_SHIFT - MIN_POOL_SHIFT + 1)

typedef struct {
  UINTN        Size;
  BOOLEAN      Available;
} POOL_HEADER;

typedef struct {
  POOL_HEADER  Header;
  LIST_ENTRY   Link;
} FREE_POOL_HEADER;

LIST_ENTRY  mSmmPoolLists[MAX_POOL_INDEX];

/**
  Called to initialize the memory service.

  @param   SmramRangeCount       Number of SMRAM Regions
  @param   SmramRanges           Pointer to SMRAM Descriptors

**/
VOID
SmmInitializeMemoryServices (
  IN UINTN                 SmramRangeCount,
  IN EFI_SMRAM_DESCRIPTOR  *SmramRanges
  )
{
  UINTN  Index;

  //
  // Initialize Pool list
  //
  for (Index = sizeof (mSmmPoolLists) / sizeof (*mSmmPoolLists); Index > 0;) {
    InitializeListHead (&mSmmPoolLists[--Index]);
  }

  //
  // Initialize free SMRAM regions
  //
  for (Index = 0; Index < SmramRangeCount; Index++) {
    SmmAddMemoryRegion (
      SmramRanges[Index].CpuStart,
      SmramRanges[Index].PhysicalSize,
      EfiConventionalMemory,
      SmramRanges[Index].RegionState
      );
  }
}

/**
  Internal Function. Allocate a pool by specified PoolIndex.

  @param  PoolIndex             Index which indicate the Pool size.
  @param  FreePoolHdr           The returned Free pool.

  @retval EFI_OUT_OF_RESOURCES   Allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
InternalAllocPoolByIndex (
  IN  UINTN             PoolIndex,
  OUT FREE_POOL_HEADER  **FreePoolHdr
  )
{
  EFI_STATUS        Status;
  FREE_POOL_HEADER  *Hdr;

  Status = EFI_SUCCESS;
  if (PoolIndex == MAX_POOL_INDEX) {
    Hdr = (FREE_POOL_HEADER *)AllocatePages (EFI_SIZE_TO_PAGES (MAX_POOL_SIZE << 1));
    if (Hdr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (!IsListEmpty (&mSmmPoolLists[PoolIndex])) {
    Hdr = BASE_CR (GetFirstNode (&mSmmPoolLists[PoolIndex]), FREE_POOL_HEADER, Link);
    RemoveEntryList (&Hdr->Link);
  } else {
    Status = InternalAllocPoolByIndex (PoolIndex + 1, &Hdr);
    if (!EFI_ERROR (Status)) {
      Hdr->Header.Size >>= 1;
      Hdr->Header.Available = TRUE;
      InsertHeadList (&mSmmPoolLists[PoolIndex], &Hdr->Link);
      Hdr = (FREE_POOL_HEADER*)((UINT8*)Hdr + Hdr->Header.Size);
    }
  }

  if (!EFI_ERROR (Status)) {
    Hdr->Header.Size = MIN_POOL_SIZE << PoolIndex;
    Hdr->Header.Available = FALSE;
  }

  *FreePoolHdr = Hdr;
  return Status;
}

/**
  Internal Function. Free a pool by specified PoolIndex.

  @param  FreePoolHdr           The pool to free.

  @retval EFI_SUCCESS           Pool successfully freed.

**/
EFI_STATUS
InternalFreePoolByIndex (
  IN FREE_POOL_HEADER  *FreePoolHdr
  )
{
  UINTN  PoolIndex;

  ASSERT ((FreePoolHdr->Header.Size & (FreePoolHdr->Header.Size - 1)) == 0);
  ASSERT (((UINTN)FreePoolHdr & (FreePoolHdr->Header.Size - 1)) == 0);
  ASSERT (FreePoolHdr->Header.Size >= MIN_POOL_SIZE);

  PoolIndex = HighBitSet32 ((UINT32)FreePoolHdr->Header.Size) - MIN_POOL_SHIFT;
  FreePoolHdr->Header.Available = TRUE;
  ASSERT (PoolIndex < MAX_POOL_INDEX);
  InsertHeadList (&mSmmPoolLists[PoolIndex], &FreePoolHdr->Link);
  return EFI_SUCCESS;
}

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate.
  @param  Size                   The amount of pool to allocate.
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool.

  @retval EFI_INVALID_PARAMETER  PoolType not valid.
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmAllocatePool (
  IN   EFI_MEMORY_TYPE  PoolType,
  IN   UINTN            Size,
  OUT  VOID             **Buffer
  )
{
  POOL_HEADER           *PoolHdr;
  FREE_POOL_HEADER      *FreePoolHdr;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 PoolIndex;

  if (PoolType != EfiRuntimeServicesCode &&
      PoolType != EfiRuntimeServicesData) {
    return EFI_INVALID_PARAMETER;
  }

  if (Size == 0) {
    *Buffer = NULL;
    return EFI_SUCCESS;
  }

  Size += sizeof (*PoolHdr);
  if (Size > MAX_POOL_SIZE) {
    Size = EFI_SIZE_TO_PAGES (Size);
    Status = SmmAllocatePages (AllocateAnyPages, PoolType, Size, &Address);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PoolHdr = (POOL_HEADER*)(UINTN)Address;
    PoolHdr->Size = EFI_PAGES_TO_SIZE (Size);
    PoolHdr->Available = FALSE;
    *Buffer = PoolHdr + 1;
    return Status;
  }

  Size = (Size + MIN_POOL_SIZE - 1) >> MIN_POOL_SHIFT;
  PoolIndex = HighBitSet32 ((UINT32)Size);
  if ((Size & (Size - 1)) != 0) {
    PoolIndex++;
  }

  Status = InternalAllocPoolByIndex (PoolIndex, &FreePoolHdr);
  *Buffer = &FreePoolHdr->Header + 1;
  return Status;
}

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free.

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePool (
  IN VOID  *Buffer
  )
{
  FREE_POOL_HEADER  *FreePoolHdr;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreePoolHdr = (FREE_POOL_HEADER*)((POOL_HEADER*)Buffer - 1);
  ASSERT (!FreePoolHdr->Header.Available);

  if (FreePoolHdr->Header.Size > MAX_POOL_SIZE) {
    ASSERT (((UINTN)FreePoolHdr & EFI_PAGE_MASK) == 0);
    ASSERT ((FreePoolHdr->Header.Size & EFI_PAGE_MASK) == 0);
    return SmmFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)FreePoolHdr,
             EFI_SIZE_TO_PAGES (FreePoolHdr->Header.Size)
             );
  }
  return InternalFreePoolByIndex (FreePoolHdr);
}
