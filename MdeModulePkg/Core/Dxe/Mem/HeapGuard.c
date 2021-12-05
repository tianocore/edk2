/** @file
  UEFI Heap Guard functions.

Copyright (c) 2017-2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Imem.h"
#include "HeapGuard.h"

//
// Global to avoid infinite reentrance of memory allocation when updating
// page table attributes, which may need allocate pages for new PDE/PTE.
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
// Used for promoting freed but not used pages.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_PHYSICAL_ADDRESS  mLastPromotedPage = BASE_4GB;

/**
  Set corresponding bits in bitmap table to 1 according to the address.

  @param[in]  Address     Start address to set for.
  @param[in]  BitNumber   Number of bits to set.
  @param[in]  BitMap      Pointer to bitmap which covers the Address.

  @return VOID.
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
  UINTN       Level;
  UINT64      *GuardMap;
  UINT64      MapMemory;
  UINTN       Index;
  UINTN       Size;
  UINTN       BitsToUnitEnd;
  EFI_STATUS  Status;

  MapMemory = 0;

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
      Status = CoreInternalAllocatePages (
                 AllocateAnyPages,
                 EfiBootServicesData,
                 EFI_SIZE_TO_PAGES (Size),
                 &MapMemory,
                 FALSE
                 );
      ASSERT_EFI_ERROR (Status);
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

      Size   = (mLevelMask[Level] + 1) * GUARDED_HEAP_MAP_ENTRY_BYTES;
      Status = CoreInternalAllocatePages (
                 AllocateAnyPages,
                 EfiBootServicesData,
                 EFI_SIZE_TO_PAGES (Size),
                 &MapMemory,
                 FALSE
                 );
      ASSERT_EFI_ERROR (Status);
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

  @return VOID.
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

  @return VOID.
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
UINT64
GetGuardedMemoryBits (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 NumberOfPages
  )
{
  UINT64  *BitMap;
  UINTN   Bits;
  UINT64  Result;
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
  UINT64  BitMap;

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

  This is done by changing the page table attribute to be NOT PRSENT.

  @param[in]  BaseAddress     Page address to Guard at

  @return VOID
**/
VOID
EFIAPI
SetGuardPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  EFI_STATUS  Status;

  if (gCpu == NULL) {
    return;
  }

  //
  // Set flag to make sure allocating memory without GUARD for page table
  // operation; otherwise infinite loops could be caused.
  //
  mOnGuarding = TRUE;
  //
  // Note: This might overwrite other attributes needed by other features,
  // such as NX memory protection.
  //
  Status = gCpu->SetMemoryAttributes (gCpu, BaseAddress, EFI_PAGE_SIZE, EFI_MEMORY_RP);
  ASSERT_EFI_ERROR (Status);
  mOnGuarding = FALSE;
}

/**
  Unset the Guard page at the given address to the normal memory.

  This is done by changing the page table attribute to be PRSENT.

  @param[in]  BaseAddress     Page address to Guard at.

  @return VOID.
**/
VOID
EFIAPI
UnsetGuardPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  UINT64      Attributes;
  EFI_STATUS  Status;

  if (gCpu == NULL) {
    return;
  }

  //
  // Once the Guard page is unset, it will be freed back to memory pool. NX
  // memory protection must be restored for this page if NX is enabled for free
  // memory.
  //
  Attributes = 0;
  if ((PcdGet64 (PcdDxeNxMemoryProtectionPolicy) & (1 << EfiConventionalMemory)) != 0) {
    Attributes |= EFI_MEMORY_XP;
  }

  //
  // Set flag to make sure allocating memory without GUARD for page table
  // operation; otherwise infinite loops could be caused.
  //
  mOnGuarding = TRUE;
  //
  // Note: This might overwrite other attributes needed by other features,
  // such as memory protection (NX). Please make sure they are not enabled
  // at the same time.
  //
  Status = gCpu->SetMemoryAttributes (gCpu, BaseAddress, EFI_PAGE_SIZE, Attributes);
  ASSERT_EFI_ERROR (Status);
  mOnGuarding = FALSE;
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

  if (AllocateType == AllocateAddress) {
    return FALSE;
  }

  if ((PcdGet8 (PcdHeapGuardPropertyMask) & PageOrPool) == 0) {
    return FALSE;
  }

  if (PageOrPool == GUARD_HEAP_TYPE_POOL) {
    ConfigBit = PcdGet64 (PcdHeapGuardPoolType);
  } else if (PageOrPool == GUARD_HEAP_TYPE_PAGE) {
    ConfigBit = PcdGet64 (PcdHeapGuardPageType);
  } else {
    ConfigBit = (UINT64)-1;
  }

  if ((UINT32)MemoryType >= MEMORY_TYPE_OS_RESERVED_MIN) {
    TestBit = BIT63;
  } else if ((UINT32)MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {
    TestBit = BIT62;
  } else if (MemoryType < EfiMaxMemoryType) {
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

  @param[in]  GuardType   Specify the sub-type(s) of Heap Guard.

  @return TRUE/FALSE.
**/
BOOLEAN
IsHeapGuardEnabled (
  UINT8  GuardType
  )
{
  return IsMemoryTypeToGuard (EfiMaxMemoryType, AllocateAnyPages, GuardType);
}

/**
  Set head Guard and tail Guard for the given memory range.

  @param[in]  Memory          Base address of memory to set guard for.
  @param[in]  NumberOfPages   Memory size in pages.

  @return VOID
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

  @return VOID
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
  Adjust address of free memory according to existing and/or required Guard.

  This function will check if there're existing Guard pages of adjacent
  memory blocks, and try to use it as the Guard page of the memory to be
  allocated.

  @param[in]  Start           Start address of free memory block.
  @param[in]  Size            Size of free memory block.
  @param[in]  SizeRequested   Size of memory to allocate.

  @return The end address of memory block found.
  @return 0 if no enough space for the required size of memory and its Guard.
**/
UINT64
AdjustMemoryS (
  IN UINT64  Start,
  IN UINT64  Size,
  IN UINT64  SizeRequested
  )
{
  UINT64  Target;

  //
  // UEFI spec requires that allocated pool must be 8-byte aligned. If it's
  // indicated to put the pool near the Tail Guard, we need extra bytes to
  // make sure alignment of the returned pool address.
  //
  if ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0) {
    SizeRequested = ALIGN_VALUE (SizeRequested, 8);
  }

  Target = Start + Size - SizeRequested;
  ASSERT (Target >= Start);
  if (Target == 0) {
    return 0;
  }

  if (!IsGuardPage (Start + Size)) {
    // No Guard at tail to share. One more page is needed.
    Target -= EFI_PAGES_TO_SIZE (1);
  }

  // Out of range?
  if (Target < Start) {
    return 0;
  }

  // At the edge?
  if (Target == Start) {
    if (!IsGuardPage (Target - EFI_PAGES_TO_SIZE (1))) {
      // No enough space for a new head Guard if no Guard at head to share.
      return 0;
    }
  }

  // OK, we have enough pages for memory and its Guards. Return the End of the
  // free space.
  return Target + SizeRequested - 1;
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

  if ((Memory == NULL) || (NumberOfPages == NULL) || (*NumberOfPages == 0)) {
    return;
  }

  Start       = *Memory;
  PagesToFree = *NumberOfPages;

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
  Adjust the base and number of pages to really allocate according to Guard.

  @param[in,out]  Memory          Base address of free memory.
  @param[in,out]  NumberOfPages   Size of memory to allocate.

  @return VOID.
**/
VOID
AdjustMemoryA (
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN OUT UINTN                 *NumberOfPages
  )
{
  //
  // FindFreePages() has already taken the Guard into account. It's safe to
  // adjust the start address and/or number of pages here, to make sure that
  // the Guards are also "allocated".
  //
  if (!IsGuardPage (*Memory + EFI_PAGES_TO_SIZE (*NumberOfPages))) {
    // No tail Guard, add one.
    *NumberOfPages += 1;
  }

  if (!IsGuardPage (*Memory - EFI_PAGE_SIZE)) {
    // No head Guard, add one.
    *Memory        -= EFI_PAGE_SIZE;
    *NumberOfPages += 1;
  }
}

/**
  Adjust the pool head position to make sure the Guard page is adjavent to
  pool tail or pool head.

  @param[in]  Memory    Base address of memory allocated.
  @param[in]  NoPages   Number of pages actually allocated.
  @param[in]  Size      Size of memory requested.
                        (plus pool head/tail overhead)

  @return Address of pool head.
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
  Allocate or free guarded memory.

  @param[in]  Start           Start address of memory to allocate or free.
  @param[in]  NumberOfPages   Memory size in pages.
  @param[in]  NewType         Memory type to convert to.

  @return VOID.
**/
EFI_STATUS
CoreConvertPagesWithGuard (
  IN UINT64           Start,
  IN UINTN            NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType
  )
{
  UINT64  OldStart;
  UINTN   OldPages;

  if (NewType == EfiConventionalMemory) {
    OldStart = Start;
    OldPages = NumberOfPages;

    AdjustMemoryF (&Start, &NumberOfPages);
    //
    // It's safe to unset Guard page inside memory lock because there should
    // be no memory allocation occurred in updating memory page attribute at
    // this point. And unsetting Guard page before free will prevent Guard
    // page just freed back to pool from being allocated right away before
    // marking it usable (from non-present to present).
    //
    UnsetGuardForMemory (OldStart, OldPages);
    if (NumberOfPages == 0) {
      return EFI_SUCCESS;
    }
  } else {
    AdjustMemoryA (&Start, &NumberOfPages);
  }

  return CoreConvertPages (Start, NumberOfPages, NewType);
}

/**
  Set all Guard pages which cannot be set before CPU Arch Protocol installed.
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
  Find the address of top-most guarded free page.

  @param[out]  Address    Start address of top-most guarded free page.

  @return VOID.
**/
VOID
GetLastGuardedFreePageAddress (
  OUT EFI_PHYSICAL_ADDRESS  *Address
  )
{
  EFI_PHYSICAL_ADDRESS  AddressGranularity;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINTN                 Level;
  UINT64                Map;
  INTN                  Index;

  ASSERT (mMapLevel >= 1);

  BaseAddress = 0;
  Map         = mGuardedMemoryMap;
  for (Level = GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel;
       Level < GUARDED_HEAP_MAP_TABLE_DEPTH;
       ++Level)
  {
    AddressGranularity = LShiftU64 (1, mLevelShift[Level]);

    //
    // Find the non-NULL entry at largest index.
    //
    for (Index = (INTN)mLevelMask[Level]; Index >= 0; --Index) {
      if (((UINT64 *)(UINTN)Map)[Index] != 0) {
        BaseAddress += MultU64x32 (AddressGranularity, (UINT32)Index);
        Map          = ((UINT64 *)(UINTN)Map)[Index];
        break;
      }
    }
  }

  //
  // Find the non-zero MSB then get the page address.
  //
  while (Map != 0) {
    Map          = RShiftU64 (Map, 1);
    BaseAddress += EFI_PAGES_TO_SIZE (1);
  }

  *Address = BaseAddress;
}

/**
  Record freed pages.

  @param[in]  BaseAddress   Base address of just freed pages.
  @param[in]  Pages         Number of freed pages.

  @return VOID.
**/
VOID
MarkFreedPages (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Pages
  )
{
  SetGuardedMemoryBits (BaseAddress, Pages);
}

/**
  Record freed pages as well as mark them as not-present.

  @param[in]  BaseAddress   Base address of just freed pages.
  @param[in]  Pages         Number of freed pages.

  @return VOID.
**/
VOID
EFIAPI
GuardFreedPages (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                 Pages
  )
{
  EFI_STATUS  Status;

  //
  // Legacy memory lower than 1MB might be accessed with no allocation. Leave
  // them alone.
  //
  if (BaseAddress < BASE_1MB) {
    return;
  }

  MarkFreedPages (BaseAddress, Pages);
  if (gCpu != NULL) {
    //
    // Set flag to make sure allocating memory without GUARD for page table
    // operation; otherwise infinite loops could be caused.
    //
    mOnGuarding = TRUE;
    //
    // Note: This might overwrite other attributes needed by other features,
    // such as NX memory protection.
    //
    Status = gCpu->SetMemoryAttributes (
                     gCpu,
                     BaseAddress,
                     EFI_PAGES_TO_SIZE (Pages),
                     EFI_MEMORY_RP
                     );
    //
    // Normally we should ASSERT the returned Status. But there might be memory
    // alloc/free involved in SetMemoryAttributes(), which might fail this
    // calling. It's rare case so it's OK to let a few tiny holes be not-guarded.
    //
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to guard freed pages: %p (%lu)\n", BaseAddress, (UINT64)Pages));
    }

    mOnGuarding = FALSE;
  }
}

/**
  Record freed pages as well as mark them as not-present, if enabled.

  @param[in]  BaseAddress   Base address of just freed pages.
  @param[in]  Pages         Number of freed pages.

  @return VOID.
**/
VOID
EFIAPI
GuardFreedPagesChecked (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                 Pages
  )
{
  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_FREED)) {
    GuardFreedPages (BaseAddress, Pages);
  }
}

/**
  Mark all pages freed before CPU Arch Protocol as not-present.

**/
VOID
GuardAllFreedPages (
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
  UINT64  GuardPage;
  INTN    Level;
  UINT64  BitIndex;
  UINTN   GuardPageNumber;

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

  Level           = GUARDED_HEAP_MAP_TABLE_DEPTH - mMapLevel;
  Tables[Level]   = mGuardedMemoryMap;
  Address         = 0;
  GuardPage       = (UINT64)-1;
  GuardPageNumber = 0;

  while (TRUE) {
    if (Indices[Level] > Entries[Level]) {
      Tables[Level] = 0;
      Level        -= 1;
    } else {
      TableEntry = ((UINT64 *)(UINTN)(Tables[Level]))[Indices[Level]];
      Address    = Addresses[Level];

      if (Level < GUARDED_HEAP_MAP_TABLE_DEPTH - 1) {
        Level           += 1;
        Tables[Level]    = TableEntry;
        Addresses[Level] = Address;
        Indices[Level]   = 0;

        continue;
      } else {
        BitIndex = 1;
        while (BitIndex != 0) {
          if ((TableEntry & BitIndex) != 0) {
            if (GuardPage == (UINT64)-1) {
              GuardPage = Address;
            }

            ++GuardPageNumber;
          } else if (GuardPageNumber > 0) {
            GuardFreedPages (GuardPage, GuardPageNumber);
            GuardPageNumber = 0;
            GuardPage       = (UINT64)-1;
          }

          if (TableEntry == 0) {
            break;
          }

          Address += EFI_PAGES_TO_SIZE (1);
          BitIndex = LShiftU64 (BitIndex, 1);
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

  //
  // Update the maximum address of freed page which can be used for memory
  // promotion upon out-of-memory-space.
  //
  GetLastGuardedFreePageAddress (&Address);
  if (Address != 0) {
    mLastPromotedPage = Address;
  }
}

/**
  This function checks to see if the given memory map descriptor in a memory map
  can be merged with any guarded free pages.

  @param  MemoryMapEntry    A pointer to a descriptor in MemoryMap.
  @param  MaxAddress        Maximum address to stop the merge.

  @return VOID

**/
VOID
MergeGuardPages (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry,
  IN EFI_PHYSICAL_ADDRESS   MaxAddress
  )
{
  EFI_PHYSICAL_ADDRESS  EndAddress;
  UINT64                Bitmap;
  INTN                  Pages;

  if (!IsHeapGuardEnabled (GUARD_HEAP_TYPE_FREED) ||
      (MemoryMapEntry->Type >= EfiMemoryMappedIO))
  {
    return;
  }

  Bitmap = 0;
  Pages  = EFI_SIZE_TO_PAGES ((UINTN)(MaxAddress - MemoryMapEntry->PhysicalStart));
  Pages -= (INTN)MemoryMapEntry->NumberOfPages;
  while (Pages > 0) {
    if (Bitmap == 0) {
      EndAddress = MemoryMapEntry->PhysicalStart +
                   EFI_PAGES_TO_SIZE ((UINTN)MemoryMapEntry->NumberOfPages);
      Bitmap = GetGuardedMemoryBits (EndAddress, GUARDED_HEAP_MAP_ENTRY_BITS);
    }

    if ((Bitmap & 1) == 0) {
      break;
    }

    Pages--;
    MemoryMapEntry->NumberOfPages++;
    Bitmap = RShiftU64 (Bitmap, 1);
  }
}

/**
  Put part (at most 64 pages a time) guarded free pages back to free page pool.

  Freed memory guard is used to detect Use-After-Free (UAF) memory issue, which
  makes use of 'Used then throw away' way to detect any illegal access to freed
  memory. The thrown-away memory will be marked as not-present so that any access
  to those memory (after free) will be caught by page-fault exception.

  The problem is that this will consume lots of memory space. Once no memory
  left in pool to allocate, we have to restore part of the freed pages to their
  normal function. Otherwise the whole system will stop functioning.

  @param  StartAddress    Start address of promoted memory.
  @param  EndAddress      End address of promoted memory.

  @return TRUE    Succeeded to promote memory.
  @return FALSE   No free memory found.

**/
BOOLEAN
PromoteGuardedFreePages (
  OUT EFI_PHYSICAL_ADDRESS  *StartAddress,
  OUT EFI_PHYSICAL_ADDRESS  *EndAddress
  )
{
  EFI_STATUS            Status;
  UINTN                 AvailablePages;
  UINT64                Bitmap;
  EFI_PHYSICAL_ADDRESS  Start;

  if (!IsHeapGuardEnabled (GUARD_HEAP_TYPE_FREED)) {
    return FALSE;
  }

  //
  // Similar to memory allocation service, always search the freed pages in
  // descending direction.
  //
  Start          = mLastPromotedPage;
  AvailablePages = 0;
  while (AvailablePages == 0) {
    Start -= EFI_PAGES_TO_SIZE (GUARDED_HEAP_MAP_ENTRY_BITS);
    //
    // If the address wraps around, try the really freed pages at top.
    //
    if (Start > mLastPromotedPage) {
      GetLastGuardedFreePageAddress (&Start);
      ASSERT (Start != 0);
      Start -= EFI_PAGES_TO_SIZE (GUARDED_HEAP_MAP_ENTRY_BITS);
    }

    Bitmap = GetGuardedMemoryBits (Start, GUARDED_HEAP_MAP_ENTRY_BITS);
    while (Bitmap > 0) {
      if ((Bitmap & 1) != 0) {
        ++AvailablePages;
      } else if (AvailablePages == 0) {
        Start += EFI_PAGES_TO_SIZE (1);
      } else {
        break;
      }

      Bitmap = RShiftU64 (Bitmap, 1);
    }
  }

  if (AvailablePages != 0) {
    DEBUG ((DEBUG_INFO, "Promoted pages: %lX (%lx)\r\n", Start, (UINT64)AvailablePages));
    ClearGuardedMemoryBits (Start, AvailablePages);

    if (gCpu != NULL) {
      //
      // Set flag to make sure allocating memory without GUARD for page table
      // operation; otherwise infinite loops could be caused.
      //
      mOnGuarding = TRUE;
      Status      = gCpu->SetMemoryAttributes (gCpu, Start, EFI_PAGES_TO_SIZE (AvailablePages), 0);
      ASSERT_EFI_ERROR (Status);
      mOnGuarding = FALSE;
    }

    mLastPromotedPage = Start;
    *StartAddress     = Start;
    *EndAddress       = Start + EFI_PAGES_TO_SIZE (AvailablePages) - 1;
    return TRUE;
  }

  return FALSE;
}

/**
  Notify function used to set all Guard pages before CPU Arch Protocol installed.
**/
VOID
HeapGuardCpuArchProtocolNotify (
  VOID
  )
{
  ASSERT (gCpu != NULL);

  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_PAGE|GUARD_HEAP_TYPE_POOL) &&
      IsHeapGuardEnabled (GUARD_HEAP_TYPE_FREED))
  {
    DEBUG ((DEBUG_ERROR, "Heap guard and freed memory guard cannot be enabled at the same time.\n"));
    CpuDeadLoop ();
  }

  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_PAGE|GUARD_HEAP_TYPE_POOL)) {
    SetAllGuardPages ();
  }

  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_FREED)) {
    GuardAllFreedPages ();
  }
}

/**
  Helper function to convert a UINT64 value in binary to a string.

  @param[in]  Value       Value of a UINT64 integer.
  @param[out]  BinString   String buffer to contain the conversion result.

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

  if (!IsHeapGuardEnabled (GUARD_HEAP_TYPE_ALL)) {
    return;
  }

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
