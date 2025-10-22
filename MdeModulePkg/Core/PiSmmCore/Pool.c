/** @file
  SMM Memory pool management functions.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCore.h"

LIST_ENTRY  mSmmPoolLists[SmmPoolTypeMax][MAX_POOL_INDEX];
//
// To cache the SMRAM base since when Loading modules At fixed address feature is enabled,
// all module is assigned an offset relative the SMRAM base in build time.
//
GLOBAL_REMOVE_IF_UNREFERENCED  EFI_PHYSICAL_ADDRESS  gLoadModuleAtFixAddressSmramBase = 0;

/**
  Convert a UEFI memory type to SMM pool type.

  @param[in]  MemoryType              Type of pool to allocate.

  @return SMM pool type
**/
SMM_POOL_TYPE
UefiMemoryTypeToSmmPoolType (
  IN  EFI_MEMORY_TYPE  MemoryType
  )
{
  ASSERT ((MemoryType == EfiRuntimeServicesCode) || (MemoryType == EfiRuntimeServicesData));
  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      return SmmPoolTypeCode;
    case EfiRuntimeServicesData:
      return SmmPoolTypeData;
    default:
      return SmmPoolTypeMax;
  }
}

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
  UINTN                                       Index;
  EFI_STATUS                                  Status;
  UINTN                                       SmmPoolTypeIndex;
  EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE  *LMFAConfigurationTable;

  //
  // Initialize Pool list
  //
  for (SmmPoolTypeIndex = 0; SmmPoolTypeIndex < SmmPoolTypeMax; SmmPoolTypeIndex++) {
    for (Index = 0; Index < ARRAY_SIZE (mSmmPoolLists[SmmPoolTypeIndex]); Index++) {
      InitializeListHead (&mSmmPoolLists[SmmPoolTypeIndex][Index]);
    }
  }

  Status = EfiGetSystemConfigurationTable (
             &gLoadFixedAddressConfigurationTableGuid,
             (VOID **)&LMFAConfigurationTable
             );
  if (!EFI_ERROR (Status) && (LMFAConfigurationTable != NULL)) {
    gLoadModuleAtFixAddressSmramBase = LMFAConfigurationTable->SmramBase;
  }

  //
  // Add Free SMRAM regions
  // Need add Free memory at first, to let gSmmMemoryMap record data
  //
  for (Index = 0; Index < SmramRangeCount; Index++) {
    if ((SmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    SmmAddMemoryRegion (
      SmramRanges[Index].CpuStart,
      SmramRanges[Index].PhysicalSize,
      EfiConventionalMemory,
      SmramRanges[Index].RegionState
      );
  }

  //
  // Add the allocated SMRAM regions
  //
  for (Index = 0; Index < SmramRangeCount; Index++) {
    if ((SmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) == 0) {
      continue;
    }

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

  @param  PoolType              Type of pool to allocate.
  @param  PoolIndex             Index which indicate the Pool size.
  @param  FreePoolHdr           The returned Free pool.

  @retval EFI_OUT_OF_RESOURCES   Allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
InternalAllocPoolByIndex (
  IN  EFI_MEMORY_TYPE   PoolType,
  IN  UINTN             PoolIndex,
  OUT FREE_POOL_HEADER  **FreePoolHdr
  )
{
  EFI_STATUS            Status;
  FREE_POOL_HEADER      *Hdr;
  POOL_TAIL             *Tail;
  EFI_PHYSICAL_ADDRESS  Address;
  SMM_POOL_TYPE         SmmPoolType;

  Address     = 0;
  SmmPoolType = UefiMemoryTypeToSmmPoolType (PoolType);

  ASSERT (PoolIndex <= MAX_POOL_INDEX);
  Status = EFI_SUCCESS;
  Hdr    = NULL;
  if (PoolIndex == MAX_POOL_INDEX) {
    Status = SmmInternalAllocatePages (
               AllocateAnyPages,
               PoolType,
               EFI_SIZE_TO_PAGES (MAX_POOL_SIZE << 1),
               &Address,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Hdr = (FREE_POOL_HEADER *)(UINTN)Address;
  } else if (!IsListEmpty (&mSmmPoolLists[SmmPoolType][PoolIndex])) {
    Hdr = BASE_CR (GetFirstNode (&mSmmPoolLists[SmmPoolType][PoolIndex]), FREE_POOL_HEADER, Link);
    RemoveEntryList (&Hdr->Link);
  } else {
    Status = InternalAllocPoolByIndex (PoolType, PoolIndex + 1, &Hdr);
    if (!EFI_ERROR (Status)) {
      Hdr->Header.Signature = 0;
      Hdr->Header.Size    >>= 1;
      Hdr->Header.Available = TRUE;
      Hdr->Header.Type      = 0;
      Tail                  = HEAD_TO_TAIL (&Hdr->Header);
      Tail->Signature       = 0;
      Tail->Size            = 0;
      InsertHeadList (&mSmmPoolLists[SmmPoolType][PoolIndex], &Hdr->Link);
      Hdr = (FREE_POOL_HEADER *)((UINT8 *)Hdr + Hdr->Header.Size);
    }
  }

  if (!EFI_ERROR (Status)) {
    Hdr->Header.Signature = POOL_HEAD_SIGNATURE;
    Hdr->Header.Size      = MIN_POOL_SIZE << PoolIndex;
    Hdr->Header.Available = FALSE;
    Hdr->Header.Type      = PoolType;
    Tail                  = HEAD_TO_TAIL (&Hdr->Header);
    Tail->Signature       = POOL_TAIL_SIGNATURE;
    Tail->Size            = Hdr->Header.Size;
  }

  *FreePoolHdr = Hdr;
  return Status;
}

/**
  Internal Function. Free a pool by specified PoolIndex.

  @param  FreePoolHdr           The pool to free.
  @param  PoolTail              The pointer to the pool tail.

  @retval EFI_SUCCESS           Pool successfully freed.

**/
EFI_STATUS
InternalFreePoolByIndex (
  IN FREE_POOL_HEADER  *FreePoolHdr,
  IN POOL_TAIL         *PoolTail
  )
{
  UINTN          PoolIndex;
  SMM_POOL_TYPE  SmmPoolType;

  ASSERT ((FreePoolHdr->Header.Size & (FreePoolHdr->Header.Size - 1)) == 0);
  ASSERT (((UINTN)FreePoolHdr & (FreePoolHdr->Header.Size - 1)) == 0);
  ASSERT (FreePoolHdr->Header.Size >= MIN_POOL_SIZE);

  SmmPoolType = UefiMemoryTypeToSmmPoolType (FreePoolHdr->Header.Type);

  PoolIndex                     = (UINTN)(HighBitSet32 ((UINT32)FreePoolHdr->Header.Size) - MIN_POOL_SHIFT);
  FreePoolHdr->Header.Signature = 0;
  FreePoolHdr->Header.Available = TRUE;
  FreePoolHdr->Header.Type      = 0;
  if (PoolTail != NULL) {
    PoolTail->Signature = 0;
    PoolTail->Size      = 0;
  }

  ASSERT (PoolIndex < MAX_POOL_INDEX);
  InsertHeadList (&mSmmPoolLists[SmmPoolType][PoolIndex], &FreePoolHdr->Link);
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
SmmInternalAllocatePool (
  IN   EFI_MEMORY_TYPE  PoolType,
  IN   UINTN            Size,
  OUT  VOID             **Buffer
  )
{
  POOL_HEADER           *PoolHdr;
  POOL_TAIL             *PoolTail;
  FREE_POOL_HEADER      *FreePoolHdr;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 PoolIndex;
  BOOLEAN               HasPoolTail;
  BOOLEAN               NeedGuard;
  UINTN                 NoPages;

  Address = 0;

  if ((PoolType != EfiRuntimeServicesCode) &&
      (PoolType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  NeedGuard   = IsPoolTypeToGuard (PoolType);
  HasPoolTail = !(NeedGuard &&
                  ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0));

  //
  // Adjust the size by the pool header & tail overhead
  //
  Size += POOL_OVERHEAD;
  if ((Size > MAX_POOL_SIZE) || NeedGuard) {
    if (!HasPoolTail) {
      Size -= sizeof (POOL_TAIL);
    }

    NoPages = EFI_SIZE_TO_PAGES (Size);
    Status  = SmmInternalAllocatePages (
                AllocateAnyPages,
                PoolType,
                NoPages,
                &Address,
                NeedGuard
                );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (NeedGuard) {
      ASSERT (VerifyMemoryGuard (Address, NoPages) == TRUE);
      Address = (EFI_PHYSICAL_ADDRESS)(UINTN)AdjustPoolHeadA (
                                               Address,
                                               NoPages,
                                               Size
                                               );
    }

    PoolHdr            = (POOL_HEADER *)(UINTN)Address;
    PoolHdr->Signature = POOL_HEAD_SIGNATURE;
    PoolHdr->Size      = EFI_PAGES_TO_SIZE (NoPages);
    PoolHdr->Available = FALSE;
    PoolHdr->Type      = PoolType;

    if (HasPoolTail) {
      PoolTail            = HEAD_TO_TAIL (PoolHdr);
      PoolTail->Signature = POOL_TAIL_SIGNATURE;
      PoolTail->Size      = PoolHdr->Size;
    }

    *Buffer = PoolHdr + 1;
    return Status;
  }

  Size      = (Size + MIN_POOL_SIZE - 1) >> MIN_POOL_SHIFT;
  PoolIndex = (UINTN)HighBitSet32 ((UINT32)Size);
  if ((Size & (Size - 1)) != 0) {
    PoolIndex++;
  }

  Status = InternalAllocPoolByIndex (PoolType, PoolIndex, &FreePoolHdr);
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
SmmAllocatePool (
  IN   EFI_MEMORY_TYPE  PoolType,
  IN   UINTN            Size,
  OUT  VOID             **Buffer
  )
{
  EFI_STATUS  Status;

  Status = SmmInternalAllocatePool (PoolType, Size, Buffer);
  if (!EFI_ERROR (Status)) {
    SmmCoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MemoryProfileActionAllocatePool,
      PoolType,
      Size,
      *Buffer,
      NULL
      );
  }

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
SmmInternalFreePool (
  IN VOID  *Buffer
  )
{
  FREE_POOL_HEADER  *FreePoolHdr;
  POOL_TAIL         *PoolTail;
  BOOLEAN           HasPoolTail;
  BOOLEAN           MemoryGuarded;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreePoolHdr = (FREE_POOL_HEADER *)((POOL_HEADER *)Buffer - 1);
  ASSERT (FreePoolHdr->Header.Signature == POOL_HEAD_SIGNATURE);
  ASSERT (!FreePoolHdr->Header.Available);
  if (FreePoolHdr->Header.Signature != POOL_HEAD_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  MemoryGuarded = IsHeapGuardEnabled () &&
                  IsMemoryGuarded ((EFI_PHYSICAL_ADDRESS)(UINTN)FreePoolHdr);
  HasPoolTail = !(MemoryGuarded &&
                  ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0));

  if (HasPoolTail) {
    PoolTail = HEAD_TO_TAIL (&FreePoolHdr->Header);
    ASSERT (PoolTail->Signature == POOL_TAIL_SIGNATURE);
    ASSERT (FreePoolHdr->Header.Size == PoolTail->Size);
    if (PoolTail->Signature != POOL_TAIL_SIGNATURE) {
      return EFI_INVALID_PARAMETER;
    }

    if (FreePoolHdr->Header.Size != PoolTail->Size) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    PoolTail = NULL;
  }

  if (MemoryGuarded) {
    Buffer = AdjustPoolHeadF ((EFI_PHYSICAL_ADDRESS)(UINTN)FreePoolHdr);
    return SmmInternalFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
             EFI_SIZE_TO_PAGES (FreePoolHdr->Header.Size),
             TRUE
             );
  }

  if (FreePoolHdr->Header.Size > MAX_POOL_SIZE) {
    ASSERT (((UINTN)FreePoolHdr & EFI_PAGE_MASK) == 0);
    ASSERT ((FreePoolHdr->Header.Size & EFI_PAGE_MASK) == 0);
    return SmmInternalFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)FreePoolHdr,
             EFI_SIZE_TO_PAGES (FreePoolHdr->Header.Size),
             FALSE
             );
  }

  return InternalFreePoolByIndex (FreePoolHdr, PoolTail);
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
  EFI_STATUS  Status;

  Status = SmmInternalFreePool (Buffer);
  if (!EFI_ERROR (Status)) {
    SmmCoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MemoryProfileActionFreePool,
      EfiMaxMemoryType,
      0,
      Buffer,
      NULL
      );
  }

  return Status;
}
