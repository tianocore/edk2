/** @file
  SMM Memory pool management functions.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

LIST_ENTRY  mMmPoolLists[MAX_POOL_INDEX];
//
// To cache the MMRAM base since when Loading modules At fixed address feature is enabled,
// all module is assigned an offset relative the MMRAM base in build time.
//
GLOBAL_REMOVE_IF_UNREFERENCED  EFI_PHYSICAL_ADDRESS  gLoadModuleAtFixAddressMmramBase = 0;

/**
  Called to initialize the memory service.

  @param   MmramRangeCount       Number of MMRAM Regions
  @param   MmramRanges           Pointer to MMRAM Descriptors

**/
VOID
MmInitializeMemoryServices (
  IN UINTN                 MmramRangeCount,
  IN EFI_MMRAM_DESCRIPTOR  *MmramRanges
  )
{
  UINTN  Index;

  //
  // Initialize Pool list
  //
  for (Index = sizeof (mMmPoolLists) / sizeof (*mMmPoolLists); Index > 0;) {
    InitializeListHead (&mMmPoolLists[--Index]);
  }

  //
  // Initialize free MMRAM regions
  // Need add Free memory at first, to let mMmMemoryMap record data
  //
  for (Index = 0; Index < MmramRangeCount; Index++) {
    //
    // BUGBUG: Add legacy MMRAM region is buggy.
    //
    if (MmramRanges[Index].CpuStart < BASE_1MB) {
      continue;
    }

    if ((MmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "MmAddMemoryRegion %d : 0x%016lx - 0x%016lx\n",
      Index,
      MmramRanges[Index].CpuStart,
      MmramRanges[Index].PhysicalSize
      ));
    MmAddMemoryRegion (
      MmramRanges[Index].CpuStart,
      MmramRanges[Index].PhysicalSize,
      EfiConventionalMemory,
      MmramRanges[Index].RegionState
      );
  }

  for (Index = 0; Index < MmramRangeCount; Index++) {
    //
    // BUGBUG: Add legacy MMRAM region is buggy.
    //
    if (MmramRanges[Index].CpuStart < BASE_1MB) {
      continue;
    }

    if ((MmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) == 0) {
      continue;
    }

    MmAddMemoryRegion (
      MmramRanges[Index].CpuStart,
      MmramRanges[Index].PhysicalSize,
      EfiConventionalMemory,
      MmramRanges[Index].RegionState
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
  EFI_STATUS            Status;
  FREE_POOL_HEADER      *Hdr;
  EFI_PHYSICAL_ADDRESS  Address;

  ASSERT (PoolIndex <= MAX_POOL_INDEX);
  Status = EFI_SUCCESS;
  Hdr    = NULL;
  if (PoolIndex == MAX_POOL_INDEX) {
    Status = MmInternalAllocatePages (
               AllocateAnyPages,
               EfiRuntimeServicesData,
               EFI_SIZE_TO_PAGES (MAX_POOL_SIZE << 1),
               &Address
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Hdr = (FREE_POOL_HEADER *)(UINTN)Address;
  } else if (!IsListEmpty (&mMmPoolLists[PoolIndex])) {
    Hdr = BASE_CR (GetFirstNode (&mMmPoolLists[PoolIndex]), FREE_POOL_HEADER, Link);
    RemoveEntryList (&Hdr->Link);
  } else {
    Status = InternalAllocPoolByIndex (PoolIndex + 1, &Hdr);
    if (!EFI_ERROR (Status)) {
      Hdr->Header.Size    >>= 1;
      Hdr->Header.Available = TRUE;
      InsertHeadList (&mMmPoolLists[PoolIndex], &Hdr->Link);
      Hdr = (FREE_POOL_HEADER *)((UINT8 *)Hdr + Hdr->Header.Size);
    }
  }

  if (!EFI_ERROR (Status)) {
    Hdr->Header.Size      = MIN_POOL_SIZE << PoolIndex;
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

  PoolIndex                     = (UINTN)(HighBitSet32 ((UINT32)FreePoolHdr->Header.Size) - MIN_POOL_SHIFT);
  FreePoolHdr->Header.Available = TRUE;
  ASSERT (PoolIndex < MAX_POOL_INDEX);
  InsertHeadList (&mMmPoolLists[PoolIndex], &FreePoolHdr->Link);
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
MmInternalAllocatePool (
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

  if ((PoolType != EfiRuntimeServicesCode) &&
      (PoolType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  Size += sizeof (*PoolHdr);
  if (Size > MAX_POOL_SIZE) {
    Size   = EFI_SIZE_TO_PAGES (Size);
    Status = MmInternalAllocatePages (AllocateAnyPages, PoolType, Size, &Address);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PoolHdr            = (POOL_HEADER *)(UINTN)Address;
    PoolHdr->Size      = EFI_PAGES_TO_SIZE (Size);
    PoolHdr->Available = FALSE;
    *Buffer            = PoolHdr + 1;
    return Status;
  }

  Size      = (Size + MIN_POOL_SIZE - 1) >> MIN_POOL_SHIFT;
  PoolIndex = (UINTN)HighBitSet32 ((UINT32)Size);
  if ((Size & (Size - 1)) != 0) {
    PoolIndex++;
  }

  Status = InternalAllocPoolByIndex (PoolIndex, &FreePoolHdr);
  if (!EFI_ERROR (Status)) {
    *Buffer = &FreePoolHdr->Header + 1;
  }

  return Status;
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
MmAllocatePool (
  IN   EFI_MEMORY_TYPE  PoolType,
  IN   UINTN            Size,
  OUT  VOID             **Buffer
  )
{
  EFI_STATUS  Status;

  Status = MmInternalAllocatePool (PoolType, Size, Buffer);
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
MmInternalFreePool (
  IN VOID  *Buffer
  )
{
  FREE_POOL_HEADER  *FreePoolHdr;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreePoolHdr = (FREE_POOL_HEADER *)((POOL_HEADER *)Buffer - 1);
  ASSERT (!FreePoolHdr->Header.Available);

  if (FreePoolHdr->Header.Size > MAX_POOL_SIZE) {
    ASSERT (((UINTN)FreePoolHdr & EFI_PAGE_MASK) == 0);
    ASSERT ((FreePoolHdr->Header.Size & EFI_PAGE_MASK) == 0);
    return MmInternalFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)FreePoolHdr,
             EFI_SIZE_TO_PAGES (FreePoolHdr->Header.Size)
             );
  }

  return InternalFreePoolByIndex (FreePoolHdr);
}

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free.

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
MmFreePool (
  IN VOID  *Buffer
  )
{
  EFI_STATUS  Status;

  Status = MmInternalFreePool (Buffer);
  return Status;
}
