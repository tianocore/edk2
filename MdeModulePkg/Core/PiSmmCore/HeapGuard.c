/** @file
  UEFI Heap Guard functions.

Copyright (c) 2017-2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HeapGuard.h"

//
// Global to avoid infinite reentrance of memory allocation when updating
// page table attributes, which may need allocating pages for new PDE/PTE.
//
GLOBAL_REMOVE_IF_UNREFERENCED BOOLEAN  mOnGuarding = FALSE;

//
// Pointer to table tracking the Guarded memory with bitmap, in which  '1'
// is used to indicate memory guarded. '0' might be free memory or Guard
// page itself, depending on status of memory adjacent to it.
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64  mGuardedMemoryMap = 0;

//
// Current depth level of map table pointed by mGuardedMemoryMap.
// mMapLevel must be initialized at least by 1. It will be automatically
// updated according to the address of memory just tracked.
//
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mMapLevel = 1;

//
// Shift and mask for each level of map table
//
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mLevelShift[GUARDED_HEAP_MAP_TABLE_DEPTH]
  = GUARDED_HEAP_MAP_TABLE_DEPTH_SHIFTS;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mLevelMask[GUARDED_HEAP_MAP_TABLE_DEPTH]
  = GUARDED_HEAP_MAP_TABLE_DEPTH_MASKS;

//
// SMM memory attribute protocol
//
EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *mSmmMemoryAttribute = NULL;

/**
  Set corresponding bits in bitmap table to 1 according to the address.

  @param[in]  Address     Start address to set for.
  @param[in]  BitNumber   Number of bits to set.
  @param[in]  BitMap      Pointer to bitmap which covers the Address.

  @return VOID
**/
STATIC
VOID
SetBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 BitNumber,
  IN UINT64                *BitMap
  )
{
  UINTN  Lsbs;
  UINTN  Qwords;
  UINTN  Msbs;
  UINTN  StartBit;
  UINTN  EndBit;

  StartBit = (UINTN)GUARDED_HEAP_MAP_ENTRY_BIT_INDEX (Address);
  EndBit   = (StartBit + BitNumber - 1) % GUARDED_HEAP_MAP_ENTRY_BITS;

  if ((StartBit + BitNumber) >= GUARDED_HEAP_MAP_ENTRY_BITS) {
    Msbs = (GUARDED_HEAP_MAP_ENTRY_BITS - StartBit) %
           GUARDED_HEAP_MAP_ENTRY_BITS;
    Lsbs   = (EndBit + 1) % GUARDED_HEAP_MAP_ENTRY_BITS;
    Qwords = (BitNumber - Msbs) / GUARDED_HEAP_MAP_ENTRY_BITS;
  } else {
    Msbs   = BitNumber;
    Lsbs   = 0;
    Qwords = 0;
  }

  if (Msbs > 0) {
    *BitMap |= LShiftU64 (LShiftU64 (1, Msbs) - 1, StartBit);
    BitMap  += 1;
  }

  if (Qwords > 0) {
    SetMem64 (
      (VOID *)BitMap,
      Qwords * GUARDED_HEAP_MAP_ENTRY_BYTES,
      (UINT64)-1
      );
    BitMap += Qwords;
  }

  if (Lsbs > 0) {
    *BitMap |= (LShiftU64 (1, Lsbs) - 1);
  }
}

/**
  Set corresponding bits in bitmap table to 0 according to the address.

  @param[in]  Address     Start address to set for.
  @param[in]  BitNumber   Number of bits to set.
  @param[in]  BitMap      Pointer to bitmap which covers the Address.

  @return VOID.
**/
STATIC
VOID
ClearBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 BitNumber,
  IN UINT64                *BitMap
  )
{
  UINTN  Lsbs;
  UINTN  Qwords;
  UINTN  Msbs;
  UINTN  StartBit;
  UINTN  EndBit;

  StartBit = (UINTN)GUARDED_HEAP_MAP_ENTRY_BIT_INDEX (Address);
  EndBit   = (StartBit + BitNumber - 1) % GUARDED_HEAP_MAP_ENTRY_BITS;

  if ((StartBit + BitNumber) >= GUARDED_HEAP_MAP_ENTRY_BITS) {
    Msbs = (GUARDED_HEAP_MAP_ENTRY_BITS - StartBit) %
           GUARDED_HEAP_MAP_ENTRY_BITS;
    Lsbs   = (EndBit + 1) % GUARDED_HEAP_MAP_ENTRY_BITS;
    Qwords = (BitNumber - Msbs) / GUARDED_HEAP_MAP_ENTRY_BITS;
  } else {
    Msbs   = BitNumber;
    Lsbs   = 0;
    Qwords = 0;
  }

  if (Msbs > 0) {
    *BitMap &= ~LShiftU64 (LShiftU64 (1, Msbs) - 1, StartBit);
    BitMap  += 1;
  }

  if (Qwords > 0) {
    SetMem64 ((VOID *)BitMap, Qwords * GUARDED_HEAP_MAP_ENTRY_BYTES, 0);
    BitMap += Qwords;
  }

  if (Lsbs > 0) {
    *BitMap &= ~(LShiftU64 (1, Lsbs) - 1);
  }
}

/**
  Get corresponding bits in bitmap table according to the address.

  The value of bit 0 corresponds to the status of memory at given Address.
  No more than 64 bits can be retrieved in one call.

  @param[in]  Address     Start address to retrieve bits for.
  @param[in]  BitNumber   Number of bits to get.
  @param[in]  BitMap      Pointer to bitmap which covers the Address.

  @return An integer containing the bits information.
**/
STATIC
UINT64
GetBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 BitNumber,
  IN UINT64                *BitMap
  )
{
  UINTN   StartBit;
  UINTN   EndBit;
  UINTN   Lsbs;
  UINTN   Msbs;
  UINT64  Result;

  ASSERT (BitNumber <= GUARDED_HEAP_MAP_ENTRY_BITS);

  StartBit = (UINTN)GUARDED_HEAP_MAP_ENTRY_BIT_INDEX (Address);
  EndBit   = (StartBit + BitNumber - 1) % GUARDED_HEAP_MAP_ENTRY_BITS;

  if ((StartBit + BitNumber) > GUARDED_HEAP_MAP_ENTRY_BITS) {
    Msbs = GUARDED_HEAP_MAP_ENTRY_BITS - StartBit;
    Lsbs = (EndBit + 1) % GUARDED_HEAP_MAP_ENTRY_BITS;
  } else {
    Msbs = BitNumber;
    Lsbs = 0;
  }

  if ((StartBit == 0) && (BitNumber == GUARDED_HEAP_MAP_ENTRY_BITS)) {
    Result = *BitMap;
  } else {
    Result = RShiftU64 ((*BitMap), StartBit) & (LShiftU64 (1, Msbs) - 1);
    if (Lsbs > 0) {
      BitMap += 1;
      Result |= LShiftU64 ((*BitMap) & (LShiftU64 (1, Lsbs) - 1), Msbs);
    }
  }

  return Result;
}

/**
  Helper function to allocate pages without Guard for internal uses.

  @param[in]  Pages       Page number.

  @return Address of memory allocated.
**/
VOID *
PageAlloc (
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;

  Status = SmmInternalAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesData,
             Pages,
             &Memory,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    Memory = 0;
  }

  return (VOID *)(UINTN)Memory;
}

/**
  Locate the pointer of bitmap from the guarded memory bitmap tables, which
  covers the given Address.

  @param[in]  Address       Start address to search the bitmap for.
  @param[in]  AllocMapUnit  Flag to indicate memory allocation for the table.
  @param[out] BitMap        Pointer to bitmap which covers the Address.

  @return The bit number from given Address to the end of current map table.
**/
UINTN
FindGuardedMemoryMap (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  BOOLEAN               AllocMapUnit,
  OUT UINT64                **BitMap
  )
{
  UINTN   Level;
  UINT64  *GuardMap;
  UINT64  MapMemory;
  UINTN   Index;
  UINTN   Size;
  UINTN   BitsToUnitEnd;

  //
  // Adjust current map table depth according to the address to access
  //
  while (AllocMapUnit &&
         mMapLevel < GUARDED_HEAP_MAP_TABLE_DEPTH &&
         RShiftU64 (
           Address,
           mLevelShift[GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel - 1]
           ) != 0)
  {
    if (mGuardedMemoryMap != 0) {
      Size = (mLevelMask[GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel - 1] + 1)
             * GUARDED_HEAP_MAP_ENTRY_BYTES;
      MapMemory = (UINT64)(UINTN)PageAlloc (EFI_SIZE_TO_PAGES (Size));
      ASSERT (MapMemory != 0);

      SetMem ((VOID *)(UINTN)MapMemory, Size, 0);

      *(UINT64 *)(UINTN)MapMemory = mGuardedMemoryMap;
      mGuardedMemoryMap           = MapMemory;
    }

    mMapLevel++;
  }

  GuardMap = &mGuardedMemoryMap;
  for (Level = GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel;
       Level < GUARDED_HEAP_MAP_TABLE_DEPTH;
       ++Level)
  {
    if (*GuardMap == 0) {
      if (!AllocMapUnit) {
        GuardMap = NULL;
        break;
      }

      Size      = (mLevelMask[Level] + 1) * GUARDED_HEAP_MAP_ENTRY_BYTES;
      MapMemory = (UINT64)(UINTN)PageAlloc (EFI_SIZE_TO_PAGES (Size));
      ASSERT (MapMemory != 0);

      SetMem ((VOID *)(UINTN)MapMemory, Size, 0);
      *GuardMap = MapMemory;
    }

    Index    = (UINTN)RShiftU64 (Address, mLevelShift[Level]);
    Index   &= mLevelMask[Level];
    GuardMap = (UINT64 *)(UINTN)((*GuardMap) + Index * sizeof (UINT64));
  }

  BitsToUnitEnd = GUARDED_HEAP_MAP_BITS - GUARDED_HEAP_MAP_BIT_INDEX (Address);
  *BitMap       = GuardMap;

  return BitsToUnitEnd;
}

/**
  Set corresponding bits in bitmap table to 1 according to given memory range.

  @param[in]  Address       Memory address to guard from.
  @param[in]  NumberOfPages Number of pages to guard.

  @return VOID
**/
VOID
EFIAPI
SetGuardedMemoryBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 NumberOfPages
  )
{
  UINT64  *BitMap;
  UINTN   Bits;
  UINTN   BitsToUnitEnd;

  while (NumberOfPages > 0) {
    BitsToUnitEnd = FindGuardedMemoryMap (Address, TRUE, &BitMap);
    ASSERT (BitMap != NULL);

    if (NumberOfPages > BitsToUnitEnd) {
      // Cross map unit
      Bits = BitsToUnitEnd;
    } else {
      Bits = NumberOfPages;
    }

    SetBits (Address, Bits, BitMap);

    NumberOfPages -= Bits;
    Address       += EFI_PAGES_TO_SIZE (Bits);
  }
}

/**
  Clear corresponding bits in bitmap table according to given memory range.

  @param[in]  Address       Memory address to unset from.
  @param[in]  NumberOfPages Number of pages to unset guard.

  @return VOID
**/
VOID
EFIAPI
ClearGuardedMemoryBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 NumberOfPages
  )
{
  UINT64  *BitMap;
  UINTN   Bits;
  UINTN   BitsToUnitEnd;

  while (NumberOfPages > 0) {
    BitsToUnitEnd = FindGuardedMemoryMap (Address, TRUE, &BitMap);
    ASSERT (BitMap != NULL);

    if (NumberOfPages > BitsToUnitEnd) {
      // Cross map unit
      Bits = BitsToUnitEnd;
    } else {
      Bits = NumberOfPages;
    }

    ClearBits (Address, Bits, BitMap);

    NumberOfPages -= Bits;
    Address       += EFI_PAGES_TO_SIZE (Bits);
  }
}

/**
  Retrieve corresponding bits in bitmap table according to given memory range.

  @param[in]  Address       Memory address to retrieve from.
  @param[in]  NumberOfPages Number of pages to retrieve.

  @return An integer containing the guarded memory bitmap.
**/
UINTN
GetGuardedMemoryBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 NumberOfPages
  )
{
  UINT64  *BitMap;
  UINTN   Bits;
  UINTN   Result;
  UINTN   Shift;
  UINTN   BitsToUnitEnd;

  ASSERT (NumberOfPages <= GUARDED_HEAP_MAP_ENTRY_BITS);

  Result = 0;
  Shift  = 0;
  while (NumberOfPages > 0) {
    BitsToUnitEnd = FindGuardedMemoryMap (Address, FALSE, &BitMap);

    if (NumberOfPages > BitsToUnitEnd) {
      // Cross map unit
      Bits = BitsToUnitEnd;
    } else {
      Bits = NumberOfPages;
    }

    if (BitMap != NULL) {
      Result |= LShiftU64 (GetBits (Address, Bits, BitMap), Shift);
    }

    Shift         += Bits;
    NumberOfPages -= Bits;
    Address       += EFI_PAGES_TO_SIZE (Bits);
  }

  return Result;
}

/**
  Get bit value in bitmap table for the given address.

  @param[in]  Address     The address to retrieve for.

  @return 1 or 0.
**/
UINTN
EFIAPI
GetGuardMapBit (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  UINT64  *GuardMap;

  FindGuardedMemoryMap (Address, FALSE, &GuardMap);
  if (GuardMap != NULL) {
    if (RShiftU64 (
          *GuardMap,
          GUARDED_HEAP_MAP_ENTRY_BIT_INDEX (Address)
          ) & 1)
    {
      return 1;
    }
  }

  return 0;
}

/**
  Check to see if the page at the given address is a Guard page or not.

  @param[in]  Address     The address to check for.

  @return TRUE  The page at Address is a Guard page.
  @return FALSE The page at Address is not a Guard page.
**/
BOOLEAN
EFIAPI
IsGuardPage (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  UINTN  BitMap;

  //
  // There must be at least one guarded page before and/or after given
  // address if it's a Guard page. The bitmap pattern should be one of
  // 001, 100 and 101
  //
  BitMap = GetGuardedMemoryBits (Address - EFI_PAGE_SIZE, 3);
  return ((BitMap == BIT0) || (BitMap == BIT2) || (BitMap == (BIT2 | BIT0)));
}

/**
  Check to see if the page at the given address is guarded or not.

  @param[in]  Address     The address to check for.

  @return TRUE  The page at Address is guarded.
  @return FALSE The page at Address is not guarded.
**/
BOOLEAN
EFIAPI
IsMemoryGuarded (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  return (GetGuardMapBit (Address) == 1);
}

/**
  Set the page at the given address to be a Guard page.

  This is done by changing the page table attribute to be NOT PRESENT.

  @param[in]  BaseAddress     Page address to Guard at.

  @return VOID.
**/
VOID
EFIAPI
SetGuardPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  EFI_STATUS  Status;

  if (mSmmMemoryAttribute != NULL) {
    mOnGuarding = TRUE;
    Status      = mSmmMemoryAttribute->SetMemoryAttributes (
                                         mSmmMemoryAttribute,
                                         BaseAddress,
                                         EFI_PAGE_SIZE,
                                         EFI_MEMORY_RP
                                         );
    ASSERT_EFI_ERROR (Status);
    mOnGuarding = FALSE;
  }
}

/**
  Unset the Guard page at the given address to the normal memory.

  This is done by changing the page table attribute to be PRESENT.

  @param[in]  BaseAddress     Page address to Guard at.

  @return VOID.
**/
VOID
EFIAPI
UnsetGuardPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  EFI_STATUS  Status;

  if (mSmmMemoryAttribute != NULL) {
    mOnGuarding = TRUE;
    Status      = mSmmMemoryAttribute->ClearMemoryAttributes (
                                         mSmmMemoryAttribute,
                                         BaseAddress,
                                         EFI_PAGE_SIZE,
                                         EFI_MEMORY_RP|EFI_MEMORY_RO|EFI_MEMORY_XP
                                         );
    ASSERT_EFI_ERROR (Status);

    if (gST == NULL) {
      //
      // Make sure EfiConventionalMemory is NX after SmmReadyToLock
      //
      Status = mSmmMemoryAttribute->SetMemoryAttributes (
                                      mSmmMemoryAttribute,
                                      BaseAddress,
                                      EFI_PAGE_SIZE,
                                      EFI_MEMORY_XP
                                      );
      ASSERT_EFI_ERROR (Status);
    }

    mOnGuarding = FALSE;
  }
}

/**
  Check to see if the memory at the given address should be guarded or not.

  @param[in]  MemoryType      Memory type to check.
  @param[in]  AllocateType    Allocation type to check.
  @param[in]  PageOrPool      Indicate a page allocation or pool allocation.


  @return TRUE  The given type of memory should be guarded.
  @return FALSE The given type of memory should not be guarded.
**/
BOOLEAN
IsMemoryTypeToGuard (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN EFI_ALLOCATE_TYPE  AllocateType,
  IN UINT8              PageOrPool
  )
{
  UINT64  TestBit;
  UINT64  ConfigBit;

  if (  ((PcdGet8 (PcdHeapGuardPropertyMask) & PageOrPool) == 0)
     || mOnGuarding
     || (AllocateType == AllocateAddress))
  {
    return FALSE;
  }

  ConfigBit = 0;
  if ((PageOrPool & GUARD_HEAP_TYPE_POOL) != 0) {
    ConfigBit |= PcdGet64 (PcdHeapGuardPoolType);
  }

  if ((PageOrPool & GUARD_HEAP_TYPE_PAGE) != 0) {
    ConfigBit |= PcdGet64 (PcdHeapGuardPageType);
  }

  if ((MemoryType == EfiRuntimeServicesData) ||
      (MemoryType == EfiRuntimeServicesCode))
  {
    TestBit = LShiftU64 (1, MemoryType);
  } else if (MemoryType == EfiMaxMemoryType) {
    TestBit = (UINT64)-1;
  } else {
    TestBit = 0;
  }

  return ((ConfigBit & TestBit) != 0);
}

/**
  Check to see if the pool at the given address should be guarded or not.

  @param[in]  MemoryType      Pool type to check.


  @return TRUE  The given type of pool should be guarded.
  @return FALSE The given type of pool should not be guarded.
**/
BOOLEAN
IsPoolTypeToGuard (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  return IsMemoryTypeToGuard (
           MemoryType,
           AllocateAnyPages,
           GUARD_HEAP_TYPE_POOL
           );
}

/**
  Check to see if the page at the given address should be guarded or not.

  @param[in]  MemoryType      Page type to check.
  @param[in]  AllocateType    Allocation type to check.

  @return TRUE  The given type of page should be guarded.
  @return FALSE The given type of page should not be guarded.
**/
BOOLEAN
IsPageTypeToGuard (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN EFI_ALLOCATE_TYPE  AllocateType
  )
{
  return IsMemoryTypeToGuard (MemoryType, AllocateType, GUARD_HEAP_TYPE_PAGE);
}

/**
  Check to see if the heap guard is enabled for page and/or pool allocation.

  @return TRUE/FALSE.
**/
BOOLEAN
IsHeapGuardEnabled (
  VOID
  )
{
  return IsMemoryTypeToGuard (
           EfiMaxMemoryType,
           AllocateAnyPages,
           GUARD_HEAP_TYPE_POOL|GUARD_HEAP_TYPE_PAGE
           );
}

/**
  Set head Guard and tail Guard for the given memory range.

  @param[in]  Memory          Base address of memory to set guard for.
  @param[in]  NumberOfPages   Memory size in pages.

  @return VOID.
**/
VOID
SetGuardForMemory (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  EFI_PHYSICAL_ADDRESS  GuardPage;

  //
  // Set tail Guard
  //
  GuardPage = Memory + EFI_PAGES_TO_SIZE (NumberOfPages);
  if (!IsGuardPage (GuardPage)) {
    SetGuardPage (GuardPage);
  }

  // Set head Guard
  GuardPage = Memory - EFI_PAGES_TO_SIZE (1);
  if (!IsGuardPage (GuardPage)) {
    SetGuardPage (GuardPage);
  }

  //
  // Mark the memory range as Guarded
  //
  SetGuardedMemoryBits (Memory, NumberOfPages);
}

/**
  Unset head Guard and tail Guard for the given memory range.

  @param[in]  Memory          Base address of memory to unset guard for.
  @param[in]  NumberOfPages   Memory size in pages.

  @return VOID.
**/
VOID
UnsetGuardForMemory (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  EFI_PHYSICAL_ADDRESS  GuardPage;
  UINT64                GuardBitmap;

  if (NumberOfPages == 0) {
    return;
  }

  //
  // Head Guard must be one page before, if any.
  //
  //          MSB-> 1     0 <-LSB
  //          -------------------
  //  Head Guard -> 0     1 -> Don't free Head Guard  (shared Guard)
  //  Head Guard -> 0     0 -> Free Head Guard either (not shared Guard)
  //                1     X -> Don't free first page  (need a new Guard)
  //                           (it'll be turned into a Guard page later)
  //          -------------------
  //      Start -> -1    -2
  //
  GuardPage   = Memory - EFI_PAGES_TO_SIZE (1);
  GuardBitmap = GetGuardedMemoryBits (Memory - EFI_PAGES_TO_SIZE (2), 2);
  if ((GuardBitmap & BIT1) == 0) {
    //
    // Head Guard exists.
    //
    if ((GuardBitmap & BIT0) == 0) {
      //
      // If the head Guard is not a tail Guard of adjacent memory block,
      // unset it.
      //
      UnsetGuardPage (GuardPage);
    }
  } else {
    //
    // Pages before memory to free are still in Guard. It's a partial free
    // case. Turn first page of memory block to free into a new Guard.
    //
    SetGuardPage (Memory);
  }

  //
  // Tail Guard must be the page after this memory block to free, if any.
  //
  //   MSB-> 1     0 <-LSB
  //  --------------------
  //         1     0 <- Tail Guard -> Don't free Tail Guard  (shared Guard)
  //         0     0 <- Tail Guard -> Free Tail Guard either (not shared Guard)
  //         X     1               -> Don't free last page   (need a new Guard)
  //                                 (it'll be turned into a Guard page later)
  //  --------------------
  //        +1    +0 <- End
  //
  GuardPage   = Memory + EFI_PAGES_TO_SIZE (NumberOfPages);
  GuardBitmap = GetGuardedMemoryBits (GuardPage, 2);
  if ((GuardBitmap & BIT0) == 0) {
    //
    // Tail Guard exists.
    //
    if ((GuardBitmap & BIT1) == 0) {
      //
      // If the tail Guard is not a head Guard of adjacent memory block,
      // free it; otherwise, keep it.
      //
      UnsetGuardPage (GuardPage);
    }
  } else {
    //
    // Pages after memory to free are still in Guard. It's a partial free
    // case. We need to keep one page to be a head Guard.
    //
    SetGuardPage (GuardPage - EFI_PAGES_TO_SIZE (1));
  }

  //
  // No matter what, we just clear the mark of the Guarded memory.
  //
  ClearGuardedMemoryBits (Memory, NumberOfPages);
}

/**
  Adjust the start address and number of pages to free according to Guard.

  The purpose of this function is to keep the shared Guard page with adjacent
  memory block if it's still in guard, or free it if no more sharing. Another
  is to reserve pages as Guard pages in partial page free situation.

  @param[in,out]  Memory          Base address of memory to free.
  @param[in,out]  NumberOfPages   Size of memory to free.

  @return VOID.
**/
VOID
AdjustMemoryF (
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN OUT UINTN                 *NumberOfPages
  )
{
  EFI_PHYSICAL_ADDRESS  Start;
  EFI_PHYSICAL_ADDRESS  MemoryToTest;
  UINTN                 PagesToFree;
  UINT64                GuardBitmap;
  UINT64                Attributes;

  if ((Memory == NULL) || (NumberOfPages == NULL) || (*NumberOfPages == 0)) {
    return;
  }

  Start       = *Memory;
  PagesToFree = *NumberOfPages;

  //
  // In case the memory to free is marked as read-only (e.g. EfiRuntimeServicesCode).
  //
  if (mSmmMemoryAttribute != NULL) {
    Attributes = 0;
    mSmmMemoryAttribute->GetMemoryAttributes (
                           mSmmMemoryAttribute,
                           Start,
                           EFI_PAGES_TO_SIZE (PagesToFree),
                           &Attributes
                           );
    if ((Attributes & EFI_MEMORY_RO) != 0) {
      mSmmMemoryAttribute->ClearMemoryAttributes (
                             mSmmMemoryAttribute,
                             Start,
                             EFI_PAGES_TO_SIZE (PagesToFree),
                             EFI_MEMORY_RO
                             );
    }
  }

  //
  // Head Guard must be one page before, if any.
  //
  //          MSB-> 1     0 <-LSB
  //          -------------------
  //  Head Guard -> 0     1 -> Don't free Head Guard  (shared Guard)
  //  Head Guard -> 0     0 -> Free Head Guard either (not shared Guard)
  //                1     X -> Don't free first page  (need a new Guard)
  //                           (it'll be turned into a Guard page later)
  //          -------------------
  //      Start -> -1    -2
  //
  MemoryToTest = Start - EFI_PAGES_TO_SIZE (2);
  GuardBitmap  = GetGuardedMemoryBits (MemoryToTest, 2);
  if ((GuardBitmap & BIT1) == 0) {
    //
    // Head Guard exists.
    //
    if ((GuardBitmap & BIT0) == 0) {
      //
      // If the head Guard is not a tail Guard of adjacent memory block,
      // free it; otherwise, keep it.
      //
      Start       -= EFI_PAGES_TO_SIZE (1);
      PagesToFree += 1;
    }
  } else {
    //
    // No Head Guard, and pages before memory to free are still in Guard. It's a
    // partial free case. We need to keep one page to be a tail Guard.
    //
    Start       += EFI_PAGES_TO_SIZE (1);
    PagesToFree -= 1;
  }

  //
  // Tail Guard must be the page after this memory block to free, if any.
  //
  //   MSB-> 1     0 <-LSB
  //  --------------------
  //         1     0 <- Tail Guard -> Don't free Tail Guard  (shared Guard)
  //         0     0 <- Tail Guard -> Free Tail Guard either (not shared Guard)
  //         X     1               -> Don't free last page   (need a new Guard)
  //                                 (it'll be turned into a Guard page later)
  //  --------------------
  //        +1    +0 <- End
  //
  MemoryToTest = Start + EFI_PAGES_TO_SIZE (PagesToFree);
  GuardBitmap  = GetGuardedMemoryBits (MemoryToTest, 2);
  if ((GuardBitmap & BIT0) == 0) {
    //
    // Tail Guard exists.
    //
    if ((GuardBitmap & BIT1) == 0) {
      //
      // If the tail Guard is not a head Guard of adjacent memory block,
      // free it; otherwise, keep it.
      //
      PagesToFree += 1;
    }
  } else if (PagesToFree > 0) {
    //
    // No Tail Guard, and pages after memory to free are still in Guard. It's a
    // partial free case. We need to keep one page to be a head Guard.
    //
    PagesToFree -= 1;
  }

  *Memory        = Start;
  *NumberOfPages = PagesToFree;
}

/**
  Adjust the pool head position to make sure the Guard page is adjavent to
  pool tail or pool head.

  @param[in]  Memory    Base address of memory allocated.
  @param[in]  NoPages   Number of pages actually allocated.
  @param[in]  Size      Size of memory requested.
                        (plus pool head/tail overhead)

  @return Address of pool head
**/
VOID *
AdjustPoolHeadA (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NoPages,
  IN UINTN                 Size
  )
{
  if ((Memory == 0) || ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) != 0)) {
    //
    // Pool head is put near the head Guard
    //
    return (VOID *)(UINTN)Memory;
  }

  //
  // Pool head is put near the tail Guard
  //
  Size = ALIGN_VALUE (Size, 8);
  return (VOID *)(UINTN)(Memory + EFI_PAGES_TO_SIZE (NoPages) - Size);
}

/**
  Get the page base address according to pool head address.

  @param[in]  Memory    Head address of pool to free.

  @return Address of pool head.
**/
VOID *
AdjustPoolHeadF (
  IN EFI_PHYSICAL_ADDRESS  Memory
  )
{
  if ((Memory == 0) || ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) != 0)) {
    //
    // Pool head is put near the head Guard
    //
    return (VOID *)(UINTN)Memory;
  }

  //
  // Pool head is put near the tail Guard
  //
  return (VOID *)(UINTN)(Memory & ~EFI_PAGE_MASK);
}

/**
  Helper function of memory allocation with Guard pages.

  @param  FreePageList           The free page node.
  @param  NumberOfPages          Number of pages to be allocated.
  @param  MaxAddress             Request to allocate memory below this address.
  @param  MemoryType             Type of memory requested.

  @return Memory address of allocated pages.
**/
UINTN
InternalAllocMaxAddressWithGuard (
  IN OUT LIST_ENTRY       *FreePageList,
  IN     UINTN            NumberOfPages,
  IN     UINTN            MaxAddress,
  IN     EFI_MEMORY_TYPE  MemoryType

  )
{
  LIST_ENTRY      *Node;
  FREE_PAGE_LIST  *Pages;
  UINTN           PagesToAlloc;
  UINTN           HeadGuard;
  UINTN           TailGuard;
  UINTN           Address;

  for (Node = FreePageList->BackLink; Node != FreePageList;
       Node = Node->BackLink)
  {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    if ((Pages->NumberOfPages >= NumberOfPages) &&
        ((UINTN)Pages + EFI_PAGES_TO_SIZE (NumberOfPages) - 1 <= MaxAddress))
    {
      //
      // We may need 1 or 2 more pages for Guard. Check it out.
      //
      PagesToAlloc = NumberOfPages;
      TailGuard    = (UINTN)Pages + EFI_PAGES_TO_SIZE (Pages->NumberOfPages);
      if (!IsGuardPage (TailGuard)) {
        //
        // Add one if no Guard at the end of current free memory block.
        //
        PagesToAlloc += 1;
        TailGuard     = 0;
      }

      HeadGuard = (UINTN)Pages +
                  EFI_PAGES_TO_SIZE (Pages->NumberOfPages - PagesToAlloc) -
                  EFI_PAGE_SIZE;
      if (!IsGuardPage (HeadGuard)) {
        //
        // Add one if no Guard at the page before the address to allocate
        //
        PagesToAlloc += 1;
        HeadGuard     = 0;
      }

      if (Pages->NumberOfPages < PagesToAlloc) {
        // Not enough space to allocate memory with Guards? Try next block.
        continue;
      }

      Address = InternalAllocPagesOnOneNode (Pages, PagesToAlloc, MaxAddress);
      ConvertSmmMemoryMapEntry (MemoryType, Address, PagesToAlloc, FALSE);
      CoreFreeMemoryMapStack ();
      if (HeadGuard == 0) {
        // Don't pass the Guard page to user.
        Address += EFI_PAGE_SIZE;
      }

      SetGuardForMemory (Address, NumberOfPages);
      return Address;
    }
  }

  return (UINTN)(-1);
}

/**
  Helper function of memory free with Guard pages.

  @param[in]  Memory                 Base address of memory being freed.
  @param[in]  NumberOfPages          The number of pages to free.
  @param[in]  AddRegion              If this memory is new added region.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.
**/
EFI_STATUS
SmmInternalFreePagesExWithGuard (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               AddRegion
  )
{
  EFI_PHYSICAL_ADDRESS  MemoryToFree;
  UINTN                 PagesToFree;

  if (((Memory & EFI_PAGE_MASK) != 0) || (Memory == 0) || (NumberOfPages == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  MemoryToFree = Memory;
  PagesToFree  = NumberOfPages;

  AdjustMemoryF (&MemoryToFree, &PagesToFree);
  UnsetGuardForMemory (Memory, NumberOfPages);
  if (PagesToFree == 0) {
    return EFI_SUCCESS;
  }

  return SmmInternalFreePagesEx (MemoryToFree, PagesToFree, AddRegion);
}

/**
  Set all Guard pages which cannot be set during the non-SMM mode time.
**/
VOID
SetAllGuardPages (
  VOID
  )
{
  UINTN    Entries[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINTN    Shifts[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINTN    Indices[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64   Tables[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64   Addresses[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64   TableEntry;
  UINT64   Address;
  UINT64   GuardPage;
  INTN     Level;
  UINTN    Index;
  BOOLEAN  OnGuarding;

  if ((mGuardedMemoryMap == 0) ||
      (mMapLevel == 0) ||
      (mMapLevel > GUARDED_HEAP_MAP_TABLE_DEPTH))
  {
    return;
  }

  CopyMem (Entries, mLevelMask, sizeof (Entries));
  CopyMem (Shifts, mLevelShift, sizeof (Shifts));

  SetMem (Tables, sizeof (Tables), 0);
  SetMem (Addresses, sizeof (Addresses), 0);
  SetMem (Indices, sizeof (Indices), 0);

  Level         = GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel;
  Tables[Level] = mGuardedMemoryMap;
  Address       = 0;
  OnGuarding    = FALSE;

  DEBUG_CODE (
    DumpGuardedMemoryBitmap ();
    );

  while (TRUE) {
    if (Indices[Level] > Entries[Level]) {
      Tables[Level] = 0;
      Level        -= 1;
    } else {
      TableEntry = ((UINT64 *)(UINTN)(Tables[Level]))[Indices[Level]];
      Address    = Addresses[Level];

      if (TableEntry == 0) {
        OnGuarding = FALSE;
      } else if (Level < GUARDED_HEAP_MAP_TABLE_DEPTH - 1) {
        Level           += 1;
        Tables[Level]    = TableEntry;
        Addresses[Level] = Address;
        Indices[Level]   = 0;

        continue;
      } else {
        Index = 0;
        while (Index < GUARDED_HEAP_MAP_ENTRY_BITS) {
          if ((TableEntry & 1) == 1) {
            if (OnGuarding) {
              GuardPage = 0;
            } else {
              GuardPage = Address - EFI_PAGE_SIZE;
            }

            OnGuarding = TRUE;
          } else {
            if (OnGuarding) {
              GuardPage = Address;
            } else {
              GuardPage = 0;
            }

            OnGuarding = FALSE;
          }

          if (GuardPage != 0) {
            SetGuardPage (GuardPage);
          }

          if (TableEntry == 0) {
            break;
          }

          TableEntry = RShiftU64 (TableEntry, 1);
          Address   += EFI_PAGE_SIZE;
          Index     += 1;
        }
      }
    }

    if (Level < (GUARDED_HEAP_MAP_TABLE_DEPTH - (INTN)mMapLevel)) {
      break;
    }

    Indices[Level]  += 1;
    Address          = (Level == 0) ? 0 : Addresses[Level - 1];
    Addresses[Level] = Address | LShiftU64 (Indices[Level], Shifts[Level]);
  }
}

/**
  Hook function used to set all Guard pages after entering SMM mode.
**/
VOID
SmmEntryPointMemoryManagementHook (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mSmmMemoryAttribute == NULL) {
    Status = SmmLocateProtocol (
               &gEdkiiSmmMemoryAttributeProtocolGuid,
               NULL,
               (VOID **)&mSmmMemoryAttribute
               );
    if (!EFI_ERROR (Status)) {
      SetAllGuardPages ();
    }
  }
}

/**
  Helper function to convert a UINT64 value in binary to a string.

  @param[in]  Value       Value of a UINT64 integer.
  @param[out] BinString   String buffer to contain the conversion result.

  @return VOID.
**/
VOID
Uint64ToBinString (
  IN  UINT64  Value,
  OUT CHAR8   *BinString
  )
{
  UINTN  Index;

  if (BinString == NULL) {
    return;
  }

  for (Index = 64; Index > 0; --Index) {
    BinString[Index - 1] = '0' + (Value & 1);
    Value                = RShiftU64 (Value, 1);
  }

  BinString[64] = '\0';
}

/**
  Dump the guarded memory bit map.
**/
VOID
EFIAPI
DumpGuardedMemoryBitmap (
  VOID
  )
{
  UINTN   Entries[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINTN   Shifts[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINTN   Indices[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64  Tables[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64  Addresses[GUARDED_HEAP_MAP_TABLE_DEPTH];
  UINT64  TableEntry;
  UINT64  Address;
  INTN    Level;
  UINTN   RepeatZero;
  CHAR8   String[GUARDED_HEAP_MAP_ENTRY_BITS + 1];
  CHAR8   *Ruler1;
  CHAR8   *Ruler2;

  if ((mGuardedMemoryMap == 0) ||
      (mMapLevel == 0) ||
      (mMapLevel > GUARDED_HEAP_MAP_TABLE_DEPTH))
  {
    return;
  }

  Ruler1 = "               3               2               1               0";
  Ruler2 = "FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210";

  DEBUG ((
    HEAP_GUARD_DEBUG_LEVEL,
    "============================="
    " Guarded Memory Bitmap "
    "==============================\r\n"
    ));
  DEBUG ((HEAP_GUARD_DEBUG_LEVEL, "                  %a\r\n", Ruler1));
  DEBUG ((HEAP_GUARD_DEBUG_LEVEL, "                  %a\r\n", Ruler2));

  CopyMem (Entries, mLevelMask, sizeof (Entries));
  CopyMem (Shifts, mLevelShift, sizeof (Shifts));

  SetMem (Indices, sizeof (Indices), 0);
  SetMem (Tables, sizeof (Tables), 0);
  SetMem (Addresses, sizeof (Addresses), 0);

  Level         = GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel;
  Tables[Level] = mGuardedMemoryMap;
  Address       = 0;
  RepeatZero    = 0;

  while (TRUE) {
    if (Indices[Level] > Entries[Level]) {
      Tables[Level] = 0;
      Level        -= 1;
      RepeatZero    = 0;

      DEBUG ((
        HEAP_GUARD_DEBUG_LEVEL,
        "========================================="
        "=========================================\r\n"
        ));
    } else {
      TableEntry = ((UINT64 *)(UINTN)Tables[Level])[Indices[Level]];
      Address    = Addresses[Level];

      if (TableEntry == 0) {
        if (Level == GUARDED_HEAP_MAP_TABLE_DEPTH - 1) {
          if (RepeatZero == 0) {
            Uint64ToBinString (TableEntry, String);
            DEBUG ((HEAP_GUARD_DEBUG_LEVEL, "%016lx: %a\r\n", Address, String));
          } else if (RepeatZero == 1) {
            DEBUG ((HEAP_GUARD_DEBUG_LEVEL, "...             : ...\r\n"));
          }

          RepeatZero += 1;
        }
      } else if (Level < GUARDED_HEAP_MAP_TABLE_DEPTH - 1) {
        Level           += 1;
        Tables[Level]    = TableEntry;
        Addresses[Level] = Address;
        Indices[Level]   = 0;
        RepeatZero       = 0;

        continue;
      } else {
        RepeatZero = 0;
        Uint64ToBinString (TableEntry, String);
        DEBUG ((HEAP_GUARD_DEBUG_LEVEL, "%016lx: %a\r\n", Address, String));
      }
    }

    if (Level < (GUARDED_HEAP_MAP_TABLE_DEPTH - (INTN)mMapLevel)) {
      break;
    }

    Indices[Level]  += 1;
    Address          = (Level == 0) ? 0 : Addresses[Level - 1];
    Addresses[Level] = Address | LShiftU64 (Indices[Level], Shifts[Level]);
  }
}

/**
  Debug function used to verify if the Guard page is well set or not.

  @param[in]  BaseAddress     Address of memory to check.
  @param[in]  NumberOfPages   Size of memory in pages.

  @return TRUE    The head Guard and tail Guard are both well set.
  @return FALSE   The head Guard and/or tail Guard are not well set.
**/
BOOLEAN
VerifyMemoryGuard (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                 NumberOfPages
  )
{
  EFI_STATUS            Status;
  UINT64                Attribute;
  EFI_PHYSICAL_ADDRESS  Address;

  if (mSmmMemoryAttribute == NULL) {
    return TRUE;
  }

  Attribute = 0;
  Address   = BaseAddress - EFI_PAGE_SIZE;
  Status    = mSmmMemoryAttribute->GetMemoryAttributes (
                                     mSmmMemoryAttribute,
                                     Address,
                                     EFI_PAGE_SIZE,
                                     &Attribute
                                     );
  if (EFI_ERROR (Status) || ((Attribute & EFI_MEMORY_RP) == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "Head Guard is not set at: %016lx (%016lX)!!!\r\n",
      Address,
      Attribute
      ));
    DumpGuardedMemoryBitmap ();
    return FALSE;
  }

  Attribute = 0;
  Address   = BaseAddress + EFI_PAGES_TO_SIZE (NumberOfPages);
  Status    = mSmmMemoryAttribute->GetMemoryAttributes (
                                     mSmmMemoryAttribute,
                                     Address,
                                     EFI_PAGE_SIZE,
                                     &Attribute
                                     );
  if (EFI_ERROR (Status) || ((Attribute & EFI_MEMORY_RP) == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "Tail Guard is not set at: %016lx (%016lX)!!!\r\n",
      Address,
      Attribute
      ));
    DumpGuardedMemoryBitmap ();
    return FALSE;
  }

  return TRUE;
}
