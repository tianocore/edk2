/** @file
  helper file for Unit tests of the CpuPageTableLib instance of the CpuPageTableLib class

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuPageTableLibUnitTest.h"
#include "../CpuPageTable.h"

//
// Global Data to validate if the page table is legal
// mValidMaskNoLeaf[0] is not used
// mValidMaskNoLeaf[1] ... mValidMaskNoLeaf [5] represent PTE ... PML5E
// mValidMaskNoLeaf[Index] means if it is a valid no leaf entry, entry should equal to (entry & mValidMaskNoLeaf[Index])
// mValidMaskLeaf[Index] means if it is a valid leaf entry, entry should equal to (entry & mValidMaskLeaf[Index])
// mValidMaskLeafFlag[Index] means if it is a leaf entry, if and only if ((entry & mValidMaskLeafFlag[Index]) == mValidMaskLeafFlag[Index])
//
IA32_PAGING_ENTRY  mValidMaskNoLeaf[6];
IA32_PAGING_ENTRY  mValidMaskLeaf[6];
IA32_PAGING_ENTRY  mValidMaskLeafFlag[6];

/**
  Init global data.

  @param[in]   MemorySpace    Memory space
**/
VOID
InitGlobalData (
  UINTN  MemorySpace
  )
{
  UINTN  Index;

  ASSERT (MemorySpace <= 52);
  mValidMaskNoLeaf[0].Uint64   = 0;
  mValidMaskLeaf[0].Uint64     = 0;
  mValidMaskLeafFlag[0].Uint64 = 0;

  //
  // Set common part for all kinds of entrys.
  //
  for (Index = 1; Index < 6; Index++) {
    mValidMaskNoLeaf[Index].Uint64 = MAX_UINT64;
    mValidMaskLeaf[Index].Uint64   = MAX_UINT64;

    //
    // bit 51:M is reserved, and should be zero
    //
    if (MemorySpace - 1 < 51) {
      mValidMaskNoLeaf[Index].Uint64 = BitFieldWrite64 (mValidMaskNoLeaf[Index].Uint64, MemorySpace - 1, 51, 0);
      mValidMaskLeaf[Index].Uint64   = BitFieldWrite64 (mValidMaskLeaf[Index].Uint64, MemorySpace - 1, 51, 0);
    }
  }

  //
  // Handle mask for no leaf entry.
  //
  mValidMaskNoLeaf[1].Uint64               = 0; // PTE can't map to page structure.
  mValidMaskNoLeaf[2].Pnle.Bits.MustBeZero = 0; // for PML4E, bit 7 must be zero.
  mValidMaskNoLeaf[3].Pnle.Bits.MustBeZero = 0; // for PML5E, bit 7 must be zero.
  mValidMaskNoLeaf[4].Pml4.Bits.MustBeZero = 0; // for PML4E, bit 7 must be zero.
  mValidMaskNoLeaf[5].Pml4.Bits.MustBeZero = 0; // for PML5E, bit 7 must be zero.

  //
  // Handle mask for leaf entry.
  // No need to modification for PTE, since it doesn't have extra reserved bit
  //
  mValidMaskLeaf[2].Uint64 = BitFieldWrite64 (mValidMaskLeaf[2].Uint64, 13, 20, 0); // bit 13-20 is reserved for PDE
  mValidMaskLeaf[3].Uint64 = BitFieldWrite64 (mValidMaskLeaf[2].Uint64, 13, 29, 0); // bit 13-29 is reserved for PDPTE
  mValidMaskLeaf[4].Uint64 = 0;                                                     // for PML4E, no possible to map to page.
  mValidMaskLeaf[5].Uint64 = 0;                                                     // for PML5E, no possible to map to page.

  //
  // Handle Flags to indicate it is a leaf entry.
  // for PML4E and PML5E, no possible to map to page, so the flag should be MAX_UINT64.
  //
  mValidMaskLeafFlag[1].Pce.Present = 1; // For PTE, as long as it is present, it maps to page
  //
  // For PDE and PDPTE, the bit 7 should be set to map to pages
  //
  mValidMaskLeafFlag[2].Pde2M.Bits.MustBeOne = 1;
  mValidMaskLeafFlag[2].Pde2M.Bits.Present   = 1;
  mValidMaskLeafFlag[3].Pde2M.Bits.MustBeOne = 1;
  mValidMaskLeafFlag[3].Pde2M.Bits.Present   = 1;
  mValidMaskLeafFlag[4].Uint64               = MAX_UINT64;
  mValidMaskLeafFlag[5].Uint64               = MAX_UINT64;
}

/**
  Check if the Page table entry is valid

  @param[in]   PagingEntry    The entry in page table to verify
  @param[in]   Level          the level of PagingEntry.
  @param[in]   MaxLeafLevel   Max leaf entry level.
  @param[in]   LinearAddress  The linear address verified.

  @retval  Leaf entry.
**/
UNIT_TEST_STATUS
IsPageTableEntryValid (
  IN IA32_PAGING_ENTRY  *PagingEntry,
  IN UINTN              Level,
  IN UINTN              MaxLeafLevel,
  IN UINT64             Address
  )
{
  UINT64             Index;
  IA32_PAGING_ENTRY  *ChildPageEntry;
  UNIT_TEST_STATUS   Status;

  if (PagingEntry->Pce.Present == 0) {
    return UNIT_TEST_PASSED;
  }

  if ((PagingEntry->Uint64 & mValidMaskLeafFlag[Level].Uint64) == mValidMaskLeafFlag[Level].Uint64) {
    //
    // It is a Leaf
    //
    if (Level > MaxLeafLevel) {
      DEBUG ((DEBUG_ERROR, "ERROR: Level %d entry 0x%lx is a leaf entry, but max leaf level is %d \n", Level, PagingEntry->Uint64, MaxLeafLevel));
      UT_ASSERT_TRUE (Level <= MaxLeafLevel);
    }

    if ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64) != PagingEntry->Uint64) {
      DEBUG ((DEBUG_ERROR, "ERROR: Level %d Leaf entry is 0x%lx, which reserved bit is set \n", Level, PagingEntry->Uint64));
      UT_ASSERT_EQUAL ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64), PagingEntry->Uint64);
    }

    return UNIT_TEST_PASSED;
  }

  //
  // Not a leaf
  //
  UT_ASSERT_NOT_EQUAL (Level, 1);
  if ((PagingEntry->Uint64 & mValidMaskNoLeaf[Level].Uint64) != PagingEntry->Uint64) {
    DEBUG ((DEBUG_ERROR, "ERROR: Level %d no Leaf entry is 0x%lx, which reserved bit is set \n", Level, PagingEntry->Uint64));
    UT_ASSERT_EQUAL ((PagingEntry->Uint64 & mValidMaskNoLeaf[Level].Uint64), PagingEntry->Uint64);
  }

  ChildPageEntry = (IA32_PAGING_ENTRY  *)(UINTN)(((UINTN)(PagingEntry->Pnle.Bits.PageTableBaseAddress)) << 12);
  for (Index = 0; Index < 512; Index++) {
    Status = IsPageTableEntryValid (&ChildPageEntry[Index], Level-1, MaxLeafLevel, Address + (Index<<(9*(Level-1) + 3)));
    if (Status != UNIT_TEST_PASSED) {
      return Status;
    }
  }

  return UNIT_TEST_PASSED;
}

/**
  Check if the Page table is valid

  @param[in]   PageTable      The pointer to the page table.
  @param[in]   PagingMode     The paging mode.

  @retval  UNIT_TEST_PASSED   It is a valid Page Table
**/
UNIT_TEST_STATUS
IsPageTableValid (
  IN     UINTN        PageTable,
  IN     PAGING_MODE  PagingMode
  )
{
  UINTN              MaxLevel;
  UINTN              MaxLeafLevel;
  UINT64             Index;
  UNIT_TEST_STATUS   Status;
  IA32_PAGING_ENTRY  *PagingEntry;

  if ((PagingMode == Paging32bit) || (PagingMode == PagingPae) || (PagingMode >= PagingModeMax)) {
    //
    // 32bit paging is never supported.
    // PAE paging will be supported later.
    //
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  MaxLeafLevel = (UINT8)PagingMode;
  MaxLevel     = (UINT8)(PagingMode >> 8);

  PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)PageTable;
  for (Index = 0; Index < 512; Index++) {
    Status = IsPageTableEntryValid (&PagingEntry[Index], MaxLevel, MaxLeafLevel, Index << (9 * MaxLevel + 3));
    if (Status != UNIT_TEST_PASSED) {
      return Status;
    }
  }

  return Status;
}

/**
  Get the leaf entry for a given linear address from one entry in page table

  @param[in]       PagingEntry    The entry in page table which covers the linear address
  @param[in, out]  Level          On input, is the level of PagingEntry.
                                  On outout, is the level of the leaf entry
  @param[in]       MaxLeafLevel   Max leaf entry level.
  @param[in]       LinearAddress  The linear address.

  @retval  Leaf entry.
**/
UINT64
GetEntryFromSubPageTable (
  IN     IA32_PAGING_ENTRY  *PagingEntry,
  IN OUT UINTN              *Level,
  IN     UINTN              MaxLeafLevel,
  IN     UINT64             Address
  )
{
  UINT64             Index;
  IA32_PAGING_ENTRY  *ChildPageEntry;

  if (PagingEntry->Pce.Present == 0) {
    return 0;
  }

  if ((PagingEntry->Uint64 & mValidMaskLeafFlag[*Level].Uint64) == mValidMaskLeafFlag[*Level].Uint64) {
    //
    // It is a Leaf
    //
    return PagingEntry->Uint64;
  }

  //
  // Not a leaf
  //
  ChildPageEntry = (IA32_PAGING_ENTRY  *)(UINTN)(((UINTN)(PagingEntry->Pnle.Bits.PageTableBaseAddress)) << 12);
  *Level         = *Level -1;
  Index          = Address >> (*Level * 9 + 3);
  ASSERT (Index == (Index & ((1<< 9) - 1)));

  return GetEntryFromSubPageTable (&ChildPageEntry[Index], Level, MaxLeafLevel, Address - (Index << (9 * *Level + 3)));
}

/**
  Get the leaf entry for a given linear address from a page table

  @param[in]   PageTable      The pointer to the page table.
  @param[in]   PagingMode     The paging mode.
  @param[in]   LinearAddress  The linear address.
  @param[out]  Level          leaf entry's level.

  @retval  Leaf entry.
**/
UINT64
GetEntryFromPageTable (
  IN     UINTN        PageTable,
  IN     PAGING_MODE  PagingMode,
  IN     UINT64       Address,
  OUT    UINTN        *Level
  )
{
  UINTN              MaxLevel;
  UINTN              MaxLeafLevel;
  UINT64             Index;
  IA32_PAGING_ENTRY  *PagingEntry;

  if ((PagingMode == Paging32bit) || (PagingMode == PagingPae) || (PagingMode >= PagingModeMax)) {
    //
    // 32bit paging is never supported.
    // PAE paging will be supported later.
    //
    return 0;
  }

  MaxLeafLevel = (UINT8)PagingMode;
  MaxLevel     = (UINT8)(PagingMode >> 8);

  Index = Address >> (MaxLevel * 9 + 3);
  ASSERT (Index == (Index & ((1<< 9) - 1)));
  PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)PageTable;
  *Level      = MaxLevel;

  return GetEntryFromSubPageTable (&PagingEntry[Index], Level, MaxLeafLevel, Address - (Index << (9 * MaxLevel + 3)));
}

/**
  Get max physical adrress supported by specific page mode

  @param[in]  Mode           The paging mode.

  @retval  max address.
**/
UINT64
GetMaxAddress (
  IN PAGING_MODE  Mode
  )
{
  switch (Mode) {
    case Paging32bit:
    case PagingPae:
      return SIZE_4GB;

    case Paging4Level:
    case Paging4Level1GB:
    case Paging5Level:
    case Paging5Level1GB:
      return 1ull << MIN (12 + (Mode >> 8) * 9, 52);

    default:
      ASSERT (0);
      return 0;
  }
}
