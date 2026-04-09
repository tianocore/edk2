/** @file
  This library implements CpuPageTableLib that are generic for IA32 family CPU.

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuPageTable.h"

/**
  Return the attribute of a 2M/1G page table entry.

  @param[in] PleB               Pointer to a 2M/1G page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the 2M/1G page table entry.
**/
UINT64
PageTableLibGetPleBMapAttribute (
  IN IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE  *PleB,
  IN IA32_MAP_ATTRIBUTE                 *ParentMapAttribute
  )
{
  IA32_MAP_ATTRIBUTE  MapAttribute;

  //
  // PageTableBaseAddress cannot be assigned field to field
  // because their bit positions are different in IA32_MAP_ATTRIBUTE and IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE.
  //
  MapAttribute.Uint64 = IA32_PLEB_PAGE_TABLE_BASE_ADDRESS (PleB);

  MapAttribute.Bits.Present        = ParentMapAttribute->Bits.Present & PleB->Bits.Present;
  MapAttribute.Bits.ReadWrite      = ParentMapAttribute->Bits.ReadWrite & PleB->Bits.ReadWrite;
  MapAttribute.Bits.UserSupervisor = ParentMapAttribute->Bits.UserSupervisor & PleB->Bits.UserSupervisor;
  MapAttribute.Bits.Nx             = ParentMapAttribute->Bits.Nx | PleB->Bits.Nx;
  MapAttribute.Bits.WriteThrough   = PleB->Bits.WriteThrough;
  MapAttribute.Bits.CacheDisabled  = PleB->Bits.CacheDisabled;
  MapAttribute.Bits.Accessed       = PleB->Bits.Accessed;

  MapAttribute.Bits.Pat           = PleB->Bits.Pat;
  MapAttribute.Bits.Dirty         = PleB->Bits.Dirty;
  MapAttribute.Bits.Global        = PleB->Bits.Global;
  MapAttribute.Bits.ProtectionKey = PleB->Bits.ProtectionKey;

  return MapAttribute.Uint64;
}

/**
  Return the attribute of a 4K page table entry.

  @param[in] Pte4K              Pointer to a 4K page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the 4K page table entry.
**/
UINT64
PageTableLibGetPte4KMapAttribute (
  IN IA32_PTE_4K         *Pte4K,
  IN IA32_MAP_ATTRIBUTE  *ParentMapAttribute
  )
{
  IA32_MAP_ATTRIBUTE  MapAttribute;

  MapAttribute.Uint64 = IA32_PTE4K_PAGE_TABLE_BASE_ADDRESS (Pte4K);

  MapAttribute.Bits.Present        = ParentMapAttribute->Bits.Present & Pte4K->Bits.Present;
  MapAttribute.Bits.ReadWrite      = ParentMapAttribute->Bits.ReadWrite & Pte4K->Bits.ReadWrite;
  MapAttribute.Bits.UserSupervisor = ParentMapAttribute->Bits.UserSupervisor & Pte4K->Bits.UserSupervisor;
  MapAttribute.Bits.Nx             = ParentMapAttribute->Bits.Nx | Pte4K->Bits.Nx;
  MapAttribute.Bits.WriteThrough   = Pte4K->Bits.WriteThrough;
  MapAttribute.Bits.CacheDisabled  = Pte4K->Bits.CacheDisabled;
  MapAttribute.Bits.Accessed       = Pte4K->Bits.Accessed;

  MapAttribute.Bits.Pat           = Pte4K->Bits.Pat;
  MapAttribute.Bits.Dirty         = Pte4K->Bits.Dirty;
  MapAttribute.Bits.Global        = Pte4K->Bits.Global;
  MapAttribute.Bits.ProtectionKey = Pte4K->Bits.ProtectionKey;

  return MapAttribute.Uint64;
}

/**
  Return the attribute of a non-leaf page table entry.

  @param[in] Pnle               Pointer to a non-leaf page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the non-leaf page table entry.
**/
UINT64
PageTableLibGetPnleMapAttribute (
  IN IA32_PAGE_NON_LEAF_ENTRY  *Pnle,
  IN IA32_MAP_ATTRIBUTE        *ParentMapAttribute
  )
{
  IA32_MAP_ATTRIBUTE  MapAttribute;

  MapAttribute.Uint64 = Pnle->Uint64;

  MapAttribute.Bits.Present        = ParentMapAttribute->Bits.Present & Pnle->Bits.Present;
  MapAttribute.Bits.ReadWrite      = ParentMapAttribute->Bits.ReadWrite & Pnle->Bits.ReadWrite;
  MapAttribute.Bits.UserSupervisor = ParentMapAttribute->Bits.UserSupervisor & Pnle->Bits.UserSupervisor;
  MapAttribute.Bits.Nx             = ParentMapAttribute->Bits.Nx | Pnle->Bits.Nx;
  MapAttribute.Bits.WriteThrough   = Pnle->Bits.WriteThrough;
  MapAttribute.Bits.CacheDisabled  = Pnle->Bits.CacheDisabled;
  MapAttribute.Bits.Accessed       = Pnle->Bits.Accessed;
  return MapAttribute.Uint64;
}

/**
  Return TRUE when the page table entry is a leaf entry that points to the physical address memory.
  Return FALSE when the page table entry is a non-leaf entry that points to the page table entries.

  @param[in] PagingEntry Pointer to the page table entry.
  @param[in] Level       Page level where the page table entry resides in.

  @retval TRUE  It's a leaf entry.
  @retval FALSE It's a non-leaf entry.
**/
BOOLEAN
IsPle (
  IN IA32_PAGING_ENTRY  *PagingEntry,
  IN UINTN              Level
  )
{
  //
  // PML5E and PML4E are always non-leaf entries.
  //
  if (Level == 1) {
    return TRUE;
  }

  if (((Level == 3) || (Level == 2))) {
    if (PagingEntry->PleB.Bits.MustBeOne == 1) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Recursively parse the non-leaf page table entries.

  @param[in]      PageTableBaseAddress The base address of the 512 non-leaf page table entries in the specified level.
  @param[in]      Level                Page level. Could be 5, 4, 3, 2, 1.
  @param[in]      RegionStart          The base linear address of the region covered by the non-leaf page table entries.
  @param[in]      ParentMapAttribute   The mapping attribute of the parent entries.
  @param[in, out] Map                  Pointer to an array that describes multiple linear address ranges.
  @param[in, out] MapCount             Pointer to a UINTN that hold the actual number of entries in the Map.
  @param[in]      MapCapacity          The maximum number of entries the Map can hold.
  @param[in]      LastEntry            Pointer to last map entry.
  @param[in]      OneEntry             Pointer to a library internal storage that holds one map entry.
                                       It's used when Map array is used up.
**/
VOID
PageTableLibParsePnle (
  IN     UINT64              PageTableBaseAddress,
  IN     UINTN               Level,
  IN     UINTN               MaxLevel,
  IN     UINT64              RegionStart,
  IN     IA32_MAP_ATTRIBUTE  *ParentMapAttribute,
  IN OUT IA32_MAP_ENTRY      *Map,
  IN OUT UINTN               *MapCount,
  IN     UINTN               MapCapacity,
  IN     IA32_MAP_ENTRY      **LastEntry,
  IN     IA32_MAP_ENTRY      *OneEntry
  )
{
  IA32_PAGING_ENTRY   *PagingEntry;
  UINTN               Index;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  UINT64              RegionLength;
  UINTN               PagingEntryNumber;

  ASSERT (OneEntry != NULL);

  PagingEntry       = (IA32_PAGING_ENTRY *)(UINTN)PageTableBaseAddress;
  RegionLength      = REGION_LENGTH (Level);
  PagingEntryNumber = ((MaxLevel == 3) && (Level == 3)) ? MAX_PAE_PDPTE_NUM : 512;

  for (Index = 0; Index < PagingEntryNumber; Index++, RegionStart += RegionLength) {
    if (PagingEntry[Index].Pce.Present == 0) {
      continue;
    }

    if (IsPle (&PagingEntry[Index], Level)) {
      ASSERT (Level == 1 || Level == 2 || Level == 3);

      if (Level == 1) {
        MapAttribute.Uint64 = PageTableLibGetPte4KMapAttribute (&PagingEntry[Index].Pte4K, ParentMapAttribute);
      } else {
        MapAttribute.Uint64 = PageTableLibGetPleBMapAttribute (&PagingEntry[Index].PleB, ParentMapAttribute);
      }

      if ((*LastEntry != NULL) &&
          ((*LastEntry)->LinearAddress + (*LastEntry)->Length == RegionStart) &&
          (IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&(*LastEntry)->Attribute) + (*LastEntry)->Length
           == IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&MapAttribute)) &&
          (IA32_MAP_ATTRIBUTE_ATTRIBUTES (&(*LastEntry)->Attribute) == IA32_MAP_ATTRIBUTE_ATTRIBUTES (&MapAttribute))
          )
      {
        //
        // Extend LastEntry.
        //
        (*LastEntry)->Length += RegionLength;
      } else {
        if (*MapCount < MapCapacity) {
          //
          // LastEntry points to next map entry in the array.
          //
          *LastEntry = &Map[*MapCount];
        } else {
          //
          // LastEntry points to library internal map entry.
          //
          *LastEntry = OneEntry;
        }

        //
        // Set LastEntry.
        //
        (*LastEntry)->LinearAddress    = RegionStart;
        (*LastEntry)->Length           = RegionLength;
        (*LastEntry)->Attribute.Uint64 = MapAttribute.Uint64;
        (*MapCount)++;
      }
    } else {
      MapAttribute.Uint64 = PageTableLibGetPnleMapAttribute (&PagingEntry[Index].Pnle, ParentMapAttribute);
      PageTableLibParsePnle (
        IA32_PNLE_PAGE_TABLE_BASE_ADDRESS (&PagingEntry[Index].Pnle),
        Level - 1,
        MaxLevel,
        RegionStart,
        &MapAttribute,
        Map,
        MapCount,
        MapCapacity,
        LastEntry,
        OneEntry
        );
    }
  }
}

/**
  Parse page table.

  @param[in]      PageTable  Pointer to the page table.
  @param[in]      PagingMode The paging mode.
  @param[out]     Map        Return an array that describes multiple linear address ranges.
  @param[in, out] MapCount   On input, the maximum number of entries that Map can hold.
                             On output, the number of entries in Map.

  @retval RETURN_UNSUPPORTED       PageLevel is not 5 or 4.
  @retval RETURN_INVALID_PARAMETER MapCount is NULL.
  @retval RETURN_INVALID_PARAMETER *MapCount is not 0 but Map is NULL.
  @retval RETURN_BUFFER_TOO_SMALL  *MapCount is too small.
  @retval RETURN_SUCCESS           Page table is parsed successfully.
**/
RETURN_STATUS
EFIAPI
PageTableParse (
  IN     UINTN         PageTable,
  IN     PAGING_MODE   PagingMode,
  OUT  IA32_MAP_ENTRY  *Map,
  IN OUT UINTN         *MapCount
  )
{
  UINTN               MapCapacity;
  IA32_MAP_ATTRIBUTE  NopAttribute;
  IA32_MAP_ENTRY      *LastEntry;
  IA32_MAP_ENTRY      OneEntry;
  UINTN               MaxLevel;
  UINTN               Index;
  IA32_PAGING_ENTRY   BufferInStack[MAX_PAE_PDPTE_NUM];

  if ((PagingMode == Paging32bit) || (PagingMode >= PagingModeMax)) {
    //
    // 32bit paging is never supported.
    //
    return RETURN_UNSUPPORTED;
  }

  if (MapCount == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*MapCount != 0) && (Map == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (PageTable == 0) {
    *MapCount = 0;
    return RETURN_SUCCESS;
  }

  if (PagingMode == PagingPae) {
    CopyMem (BufferInStack, (VOID *)PageTable, sizeof (BufferInStack));
    for (Index = 0; Index < MAX_PAE_PDPTE_NUM; Index++) {
      BufferInStack[Index].Pnle.Bits.ReadWrite      = 1;
      BufferInStack[Index].Pnle.Bits.UserSupervisor = 1;
      BufferInStack[Index].Pnle.Bits.Nx             = 0;
    }

    PageTable = (UINTN)BufferInStack;
  }

  //
  // Page table layout is as below:
  //
  // [IA32_CR3]
  //     |
  //     |
  //     V
  // [IA32_PML5E]
  // ...
  // [IA32_PML5E] --> [IA32_PML4E]
  //                  ...
  //                  [IA32_PML4E] --> [IA32_PDPTE_1G] --> 1G aligned physical address
  //                                   ...
  //                                   [IA32_PDPTE] --> [IA32_PDE_2M] --> 2M aligned physical address
  //                                                    ...
  //                                                    [IA32_PDE] --> [IA32_PTE_4K]  --> 4K aligned physical address
  //                                                                   ...
  //                                                                   [IA32_PTE_4K]  --> 4K aligned physical address
  //

  NopAttribute.Uint64              = 0;
  NopAttribute.Bits.Present        = 1;
  NopAttribute.Bits.ReadWrite      = 1;
  NopAttribute.Bits.UserSupervisor = 1;

  MaxLevel    = (UINT8)(PagingMode >> 8);
  MapCapacity = *MapCount;
  *MapCount   = 0;
  LastEntry   = NULL;
  PageTableLibParsePnle ((UINT64)PageTable, MaxLevel, MaxLevel, 0, &NopAttribute, Map, MapCount, MapCapacity, &LastEntry, &OneEntry);

  if (*MapCount > MapCapacity) {
    return RETURN_BUFFER_TOO_SMALL;
  }

  return RETURN_SUCCESS;
}
