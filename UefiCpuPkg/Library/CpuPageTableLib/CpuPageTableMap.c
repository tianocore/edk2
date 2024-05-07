/** @file
  This library implements CpuPageTableLib that are generic for IA32 family CPU.

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuPageTable.h"

/**
  Set the IA32_PTE_4K.

  @param[in] Pte4K     Pointer to IA32_PTE_4K.
  @param[in] Offset    The offset within the linear address range.
  @param[in] Attribute The attribute of the linear address range.
                       All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                       Page table entry is reset to 0 before set to the new attribute when a new physical base address is set.
  @param[in] Mask      The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
**/
VOID
PageTableLibSetPte4K (
  IN OUT volatile IA32_PTE_4K  *Pte4K,
  IN UINT64                    Offset,
  IN IA32_MAP_ATTRIBUTE        *Attribute,
  IN IA32_MAP_ATTRIBUTE        *Mask
  )
{
  IA32_PTE_4K  LocalPte4K;

  LocalPte4K.Uint64 = Pte4K->Uint64;
  if (Mask->Bits.PageTableBaseAddressLow || Mask->Bits.PageTableBaseAddressHigh) {
    LocalPte4K.Uint64 = (IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (Attribute) + Offset) | (LocalPte4K.Uint64 & ~IA32_PE_BASE_ADDRESS_MASK_40);
  }

  if (Mask->Bits.Present) {
    LocalPte4K.Bits.Present = Attribute->Bits.Present;
  }

  if (Mask->Bits.ReadWrite) {
    LocalPte4K.Bits.ReadWrite = Attribute->Bits.ReadWrite;
  }

  if (Mask->Bits.UserSupervisor) {
    LocalPte4K.Bits.UserSupervisor = Attribute->Bits.UserSupervisor;
  }

  if (Mask->Bits.WriteThrough) {
    LocalPte4K.Bits.WriteThrough = Attribute->Bits.WriteThrough;
  }

  if (Mask->Bits.CacheDisabled) {
    LocalPte4K.Bits.CacheDisabled = Attribute->Bits.CacheDisabled;
  }

  if (Mask->Bits.Accessed) {
    LocalPte4K.Bits.Accessed = Attribute->Bits.Accessed;
  }

  if (Mask->Bits.Dirty) {
    LocalPte4K.Bits.Dirty = Attribute->Bits.Dirty;
  }

  if (Mask->Bits.Pat) {
    LocalPte4K.Bits.Pat = Attribute->Bits.Pat;
  }

  if (Mask->Bits.Global) {
    LocalPte4K.Bits.Global = Attribute->Bits.Global;
  }

  if (Mask->Bits.ProtectionKey) {
    LocalPte4K.Bits.ProtectionKey = Attribute->Bits.ProtectionKey;
  }

  if (Mask->Bits.Nx) {
    LocalPte4K.Bits.Nx = Attribute->Bits.Nx;
  }

  if (Pte4K->Uint64 != LocalPte4K.Uint64) {
    Pte4K->Uint64 = LocalPte4K.Uint64;
  }
}

/**
  Set the IA32_PDPTE_1G or IA32_PDE_2M.

  @param[in] PleB      Pointer to PDPTE_1G or PDE_2M. Both share the same structure definition.
  @param[in] Offset    The offset within the linear address range.
  @param[in] Attribute The attribute of the linear address range.
                       All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                       Page table entry is reset to 0 before set to the new attribute when a new physical base address is set.
  @param[in] Mask      The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
**/
VOID
PageTableLibSetPleB (
  IN OUT volatile IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE  *PleB,
  IN UINT64                                          Offset,
  IN IA32_MAP_ATTRIBUTE                              *Attribute,
  IN IA32_MAP_ATTRIBUTE                              *Mask
  )
{
  IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE  LocalPleB;

  LocalPleB.Uint64 = PleB->Uint64;
  if (Mask->Bits.PageTableBaseAddressLow || Mask->Bits.PageTableBaseAddressHigh) {
    LocalPleB.Uint64 = (IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (Attribute) + Offset) | (LocalPleB.Uint64 & ~IA32_PE_BASE_ADDRESS_MASK_39);
  }

  LocalPleB.Bits.MustBeOne = 1;

  if (Mask->Bits.Present) {
    LocalPleB.Bits.Present = Attribute->Bits.Present;
  }

  if (Mask->Bits.ReadWrite) {
    LocalPleB.Bits.ReadWrite = Attribute->Bits.ReadWrite;
  }

  if (Mask->Bits.UserSupervisor) {
    LocalPleB.Bits.UserSupervisor = Attribute->Bits.UserSupervisor;
  }

  if (Mask->Bits.WriteThrough) {
    LocalPleB.Bits.WriteThrough = Attribute->Bits.WriteThrough;
  }

  if (Mask->Bits.CacheDisabled) {
    LocalPleB.Bits.CacheDisabled = Attribute->Bits.CacheDisabled;
  }

  if (Mask->Bits.Accessed) {
    LocalPleB.Bits.Accessed = Attribute->Bits.Accessed;
  }

  if (Mask->Bits.Dirty) {
    LocalPleB.Bits.Dirty = Attribute->Bits.Dirty;
  }

  if (Mask->Bits.Pat) {
    LocalPleB.Bits.Pat = Attribute->Bits.Pat;
  }

  if (Mask->Bits.Global) {
    LocalPleB.Bits.Global = Attribute->Bits.Global;
  }

  if (Mask->Bits.ProtectionKey) {
    LocalPleB.Bits.ProtectionKey = Attribute->Bits.ProtectionKey;
  }

  if (Mask->Bits.Nx) {
    LocalPleB.Bits.Nx = Attribute->Bits.Nx;
  }

  if (PleB->Uint64 != LocalPleB.Uint64) {
    PleB->Uint64 = LocalPleB.Uint64;
  }
}

/**
  Set the IA32_PDPTE_1G, IA32_PDE_2M or IA32_PTE_4K.

  @param[in] Level     3, 2 or 1.
  @param[in] Ple       Pointer to PDPTE_1G, PDE_2M or IA32_PTE_4K, depending on the Level.
  @param[in] Offset    The offset within the linear address range.
  @param[in] Attribute The attribute of the linear address range.
                       All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                       Page table entry is reset to 0 before set to the new attribute when a new physical base address is set.
  @param[in] Mask      The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
**/
VOID
PageTableLibSetPle (
  IN UINTN                           Level,
  IN OUT volatile IA32_PAGING_ENTRY  *Ple,
  IN UINT64                          Offset,
  IN IA32_MAP_ATTRIBUTE              *Attribute,
  IN IA32_MAP_ATTRIBUTE              *Mask
  )
{
  if (Level == 1) {
    PageTableLibSetPte4K (&Ple->Pte4K, Offset, Attribute, Mask);
  } else {
    ASSERT (Level == 2 || Level == 3);
    PageTableLibSetPleB (&Ple->PleB, Offset, Attribute, Mask);
  }
}

/**
  Set the IA32_PML5, IA32_PML4, IA32_PDPTE or IA32_PDE.

  @param[in] Pnle      Pointer to IA32_PML5, IA32_PML4, IA32_PDPTE or IA32_PDE. All share the same structure definition.
  @param[in] Attribute The attribute of the page directory referenced by the non-leaf.
  @param[in] Mask      The mask of the page directory referenced by the non-leaf.
**/
VOID
PageTableLibSetPnle (
  IN OUT volatile IA32_PAGE_NON_LEAF_ENTRY  *Pnle,
  IN IA32_MAP_ATTRIBUTE                     *Attribute,
  IN IA32_MAP_ATTRIBUTE                     *Mask
  )
{
  IA32_PAGE_NON_LEAF_ENTRY  LocalPnle;

  LocalPnle.Uint64 = Pnle->Uint64;
  if (Mask->Bits.Present) {
    LocalPnle.Bits.Present = Attribute->Bits.Present;
  }

  if (Mask->Bits.ReadWrite) {
    LocalPnle.Bits.ReadWrite = Attribute->Bits.ReadWrite;
  }

  if (Mask->Bits.UserSupervisor) {
    LocalPnle.Bits.UserSupervisor = Attribute->Bits.UserSupervisor;
  }

  if (Mask->Bits.Nx) {
    LocalPnle.Bits.Nx = Attribute->Bits.Nx;
  }

  LocalPnle.Bits.Accessed   = 0;
  LocalPnle.Bits.MustBeZero = 0;

  //
  // Set the attributes (WT, CD, A) to 0.
  // WT and CD determin the memory type used to access the 4K page directory referenced by this entry.
  // So, it implictly requires PAT[0] is Write Back.
  // Create a new parameter if caller requires to use a different memory type for accessing page directories.
  //
  LocalPnle.Bits.WriteThrough  = 0;
  LocalPnle.Bits.CacheDisabled = 0;
  if (Pnle->Uint64 != LocalPnle.Uint64) {
    Pnle->Uint64 = LocalPnle.Uint64;
  }
}

/**
  Check if the combination for Attribute and Mask is valid for non-present entry.
  1.Mask.Present is 0 but some other attributes is provided. This case should be invalid.
  2.Map non-present range to present. In this case, all attributes should be provided.

  @param[in] Attribute    The attribute of the linear address range.
  @param[in] Mask         The mask used for attribute to check.

  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 1 but some other attributes are not provided.
  @retval RETURN_SUCCESS            The combination for Attribute and Mask is valid.
**/
RETURN_STATUS
IsAttributesAndMaskValidForNonPresentEntry (
  IN     IA32_MAP_ATTRIBUTE  *Attribute,
  IN     IA32_MAP_ATTRIBUTE  *Mask
  )
{
  if ((Mask->Bits.Present == 1) && (Attribute->Bits.Present == 1)) {
    //
    // Creating new page table or remapping non-present range to present.
    //
    if ((Mask->Bits.ReadWrite == 0) || (Mask->Bits.UserSupervisor == 0) || (Mask->Bits.WriteThrough == 0) || (Mask->Bits.CacheDisabled == 0) ||
        (Mask->Bits.Accessed == 0) || (Mask->Bits.Dirty == 0) || (Mask->Bits.Pat == 0) || (Mask->Bits.Global == 0) ||
        ((Mask->Bits.PageTableBaseAddressLow == 0) && (Mask->Bits.PageTableBaseAddressHigh == 0)) || (Mask->Bits.ProtectionKey == 0) || (Mask->Bits.Nx == 0))
    {
      return RETURN_INVALID_PARAMETER;
    }
  } else if ((Mask->Bits.Present == 0) && (Mask->Uint64 > 1)) {
    //
    // Only change other attributes for non-present range is not permitted.
    //
    return RETURN_INVALID_PARAMETER;
  }

  return RETURN_SUCCESS;
}

/**
  Update page table to map [LinearAddress, LinearAddress + Length) with specified attribute in the specified level.

  @param[in]      ParentPagingEntry The pointer to the page table entry to update.
  @param[in]      ParentAttribute   The accumulated attribute of all parents' attribute.
  @param[in]      Modify            FALSE to indicate Buffer is not used and BufferSize is increased by the required buffer size.
  @param[in]      Buffer            The free buffer to be used for page table creation/updating.
                                    When Modify is TRUE, it's used from the end.
                                    When Modify is FALSE, it's ignored.
  @param[in, out] BufferSize        The available buffer size.
                                    Return the remaining buffer size.
  @param[in]      Level             Page table level. Could be 5, 4, 3, 2, or 1.
  @param[in]      MaxLeafLevel      Maximum level that can be a leaf entry. Could be 1, 2 or 3 (if Page 1G is supported).
  @param[in]      LinearAddress     The start of the linear address range.
  @param[in]      Length            The length of the linear address range.
  @param[in]      Offset            The offset within the linear address range.
  @param[in]      Attribute         The attribute of the linear address range.
                                    All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                                    Page table entries that map the linear address range are reset to 0 before set to the new attribute
                                    when a new physical base address is set.
  @param[in]      Mask              The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
  @param[in, out] IsModified        Change IsModified to True if page table is modified and input parameter Modify is TRUE.

  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 1 but some other attributes are not provided.
  @retval RETURN_SUCCESS            PageTable is created/updated successfully.
**/
RETURN_STATUS
PageTableLibMapInLevel (
  IN     IA32_PAGING_ENTRY   *ParentPagingEntry,
  IN     IA32_MAP_ATTRIBUTE  *ParentAttribute,
  IN     BOOLEAN             Modify,
  IN     VOID                *Buffer,
  IN OUT INTN                *BufferSize,
  IN     IA32_PAGE_LEVEL     Level,
  IN     IA32_PAGE_LEVEL     MaxLeafLevel,
  IN     UINT64              LinearAddress,
  IN     UINT64              Length,
  IN     UINT64              Offset,
  IN     IA32_MAP_ATTRIBUTE  *Attribute,
  IN     IA32_MAP_ATTRIBUTE  *Mask,
  IN OUT BOOLEAN             *IsModified
  )
{
  RETURN_STATUS       Status;
  UINTN               BitStart;
  UINTN               Index;
  IA32_PAGING_ENTRY   *PagingEntry;
  UINTN               PagingEntryIndex;
  UINTN               PagingEntryIndexEnd;
  IA32_PAGING_ENTRY   *CurrentPagingEntry;
  UINT64              RegionLength;
  UINT64              SubLength;
  UINT64              SubOffset;
  UINT64              RegionMask;
  UINT64              RegionStart;
  IA32_MAP_ATTRIBUTE  AllOneMask;
  IA32_MAP_ATTRIBUTE  PleBAttribute;
  IA32_MAP_ATTRIBUTE  NopAttribute;
  BOOLEAN             CreateNew;
  IA32_PAGING_ENTRY   OneOfPagingEntry;
  IA32_MAP_ATTRIBUTE  ChildAttribute;
  IA32_MAP_ATTRIBUTE  ChildMask;
  IA32_MAP_ATTRIBUTE  CurrentMask;
  IA32_MAP_ATTRIBUTE  LocalParentAttribute;
  UINT64              PhysicalAddrInEntry;
  UINT64              PhysicalAddrInAttr;
  IA32_PAGING_ENTRY   OriginalParentPagingEntry;
  IA32_PAGING_ENTRY   OriginalCurrentPagingEntry;
  IA32_PAGING_ENTRY   TempPagingEntry;

  ASSERT (Level != 0);
  ASSERT ((Attribute != NULL) && (Mask != NULL));

  CreateNew         = FALSE;
  AllOneMask.Uint64 = ~0ull;

  NopAttribute.Uint64              = 0;
  NopAttribute.Bits.Present        = 1;
  NopAttribute.Bits.ReadWrite      = 1;
  NopAttribute.Bits.UserSupervisor = 1;

  LocalParentAttribute.Uint64 = ParentAttribute->Uint64;
  ParentAttribute             = &LocalParentAttribute;

  OriginalParentPagingEntry.Uint64 = ParentPagingEntry->Uint64;
  OneOfPagingEntry.Uint64          = 0;
  TempPagingEntry.Uint64           = 0;

  //
  // RegionLength: 256T (1 << 48) 512G (1 << 39), 1G (1 << 30), 2M (1 << 21) or 4K (1 << 12).
  //
  BitStart         = 12 + (Level - 1) * 9;
  PagingEntryIndex = (UINTN)BitFieldRead64 (LinearAddress + Offset, BitStart, BitStart + 9 - 1);
  RegionLength     = REGION_LENGTH (Level);
  RegionMask       = RegionLength - 1;

  //
  // ParentPagingEntry ONLY is deferenced for checking Present and MustBeOne bits
  // when Modify is FALSE.
  //
  if ((ParentPagingEntry->Pce.Present == 0) || IsPle (ParentPagingEntry, Level + 1)) {
    //
    // When ParentPagingEntry is non-present, parent entry is CR3 or PML5E/PML4E/PDPTE/PDE.
    // It does NOT point to an existing page directory.
    // When ParentPagingEntry is present, parent entry is leaf PDPTE_1G or PDE_2M. Split to 2M or 4K pages.
    // Note: it's impossible the parent entry is a PTE_4K.
    //
    PleBAttribute.Uint64 = PageTableLibGetPleBMapAttribute (&ParentPagingEntry->PleB, ParentAttribute);
    if (ParentPagingEntry->Pce.Present == 0) {
      //
      // [LinearAddress, LinearAddress + Length] contains non-present range.
      //
      Status = IsAttributesAndMaskValidForNonPresentEntry (Attribute, Mask);
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    } else {
      PageTableLibSetPle (Level, &OneOfPagingEntry, 0, &PleBAttribute, &AllOneMask);
    }

    //
    // Check if the attribute, the physical address calculated by ParentPagingEntry is equal to
    // the attribute, the physical address calculated by input Attribue and Mask.
    //
    if ((IA32_MAP_ATTRIBUTE_ATTRIBUTES (&PleBAttribute) & IA32_MAP_ATTRIBUTE_ATTRIBUTES (Mask))
        == (IA32_MAP_ATTRIBUTE_ATTRIBUTES (Attribute) & IA32_MAP_ATTRIBUTE_ATTRIBUTES (Mask)))
    {
      if ((Mask->Bits.PageTableBaseAddressLow == 0) && (Mask->Bits.PageTableBaseAddressHigh == 0)) {
        return RETURN_SUCCESS;
      }

      //
      // Non-present entry won't reach there since:
      // 1.When map non-present entry to present, the attribute must be different.
      // 2.When still map non-present entry to non-present, PageTableBaseAddressLow and High in Mask must be 0.
      //
      ASSERT (ParentPagingEntry->Pce.Present == 1);
      PhysicalAddrInEntry = IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&PleBAttribute) + MultU64x32 (RegionLength, (UINT32)PagingEntryIndex);
      PhysicalAddrInAttr  = (IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (Attribute) + Offset) & (~RegionMask);
      if (PhysicalAddrInEntry == PhysicalAddrInAttr) {
        return RETURN_SUCCESS;
      }
    }

    ASSERT (Buffer == NULL || *BufferSize >= SIZE_4KB);
    CreateNew    = TRUE;
    *BufferSize -= SIZE_4KB;

    if (Modify) {
      PagingEntry = (IA32_PAGING_ENTRY *)((UINTN)Buffer + *BufferSize);
      ZeroMem (PagingEntry, SIZE_4KB);

      if (ParentPagingEntry->Pce.Present) {
        //
        // Create 512 child-level entries that map to 2M/4K.
        //
        for (SubOffset = 0, Index = 0; Index < 512; Index++) {
          PagingEntry[Index].Uint64 = OneOfPagingEntry.Uint64 + SubOffset;
          SubOffset                += RegionLength;
        }
      }

      //
      // Set NOP attributes
      // Note: Should NOT inherit the attributes from the original entry because a zero RW bit
      //       will make the entire region read-only even the child entries set the RW bit.
      //
      // Non-leaf entry doesn't have PAT bit. So use ~IA32_PE_BASE_ADDRESS_MASK_40 is to make sure PAT bit
      // (bit12) in original big-leaf entry is not assigned to PageTableBaseAddress field of non-leaf entry.
      //
      TempPagingEntry.Uint64 = ParentPagingEntry->Uint64;
      PageTableLibSetPnle (&TempPagingEntry.Pnle, &NopAttribute, &AllOneMask);
      TempPagingEntry.Uint64                           = ((UINTN)(VOID *)PagingEntry) | (TempPagingEntry.Uint64 & (~IA32_PE_BASE_ADDRESS_MASK_40));
      *(volatile UINT64 *)&(ParentPagingEntry->Uint64) = TempPagingEntry.Uint64;
    }
  } else {
    //
    // If (LinearAddress + Length - 1) is not in the same ParentPagingEntry with (LinearAddress + Offset), then the remaining child PagingEntry
    // starting from PagingEntryIndex of ParentPagingEntry is all covered by [LinearAddress + Offset, LinearAddress + Length - 1].
    //
    PagingEntryIndexEnd = (BitFieldRead64 (LinearAddress + Length - 1, BitStart + 9, 63) != BitFieldRead64 (LinearAddress + Offset, BitStart + 9, 63)) ? 511 :
                          (UINTN)BitFieldRead64 (LinearAddress + Length - 1, BitStart, BitStart + 9 - 1);
    PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)IA32_PNLE_PAGE_TABLE_BASE_ADDRESS (&ParentPagingEntry->Pnle);
    for (Index = PagingEntryIndex; Index <= PagingEntryIndexEnd; Index++) {
      if (PagingEntry[Index].Pce.Present == 0) {
        //
        // [LinearAddress, LinearAddress + Length] contains non-present range.
        //
        Status = IsAttributesAndMaskValidForNonPresentEntry (Attribute, Mask);
        if (RETURN_ERROR (Status)) {
          return Status;
        }

        break;
      }
    }

    //
    // It's a non-leaf entry
    //
    ChildAttribute.Uint64 = 0;
    ChildMask.Uint64      = 0;

    //
    // If the inheritable attributes in the parent entry conflicts with the requested attributes,
    //   let the child entries take the parent attributes and
    //   loosen the attribute in the parent entry
    // E.g.: when PDPTE[0].ReadWrite = 0 but caller wants to map [0-2MB] as ReadWrite = 1 (PDE[0].ReadWrite = 1)
    //            we need to change PDPTE[0].ReadWrite = 1 and let all PDE[0-255].ReadWrite = 0 in this step.
    //       when PDPTE[0].Nx = 1 but caller wants to map [0-2MB] as Nx = 0 (PDT[0].Nx = 0)
    //            we need to change PDPTE[0].Nx = 0 and let all PDE[0-255].Nx = 1 in this step.
    if ((ParentPagingEntry->Pnle.Bits.ReadWrite == 0) && (Mask->Bits.ReadWrite == 1) && (Attribute->Bits.ReadWrite == 1)) {
      if (Modify) {
        ParentPagingEntry->Pnle.Bits.ReadWrite = 1;
      }

      ChildAttribute.Bits.ReadWrite = 0;
      ChildMask.Bits.ReadWrite      = 1;
    }

    if ((ParentPagingEntry->Pnle.Bits.UserSupervisor == 0) && (Mask->Bits.UserSupervisor == 1) && (Attribute->Bits.UserSupervisor == 1)) {
      if (Modify) {
        ParentPagingEntry->Pnle.Bits.UserSupervisor = 1;
      }

      ChildAttribute.Bits.UserSupervisor = 0;
      ChildMask.Bits.UserSupervisor      = 1;
    }

    if ((ParentPagingEntry->Pnle.Bits.Nx == 1) && (Mask->Bits.Nx == 1) && (Attribute->Bits.Nx == 0)) {
      if (Modify) {
        ParentPagingEntry->Pnle.Bits.Nx = 0;
      }

      ChildAttribute.Bits.Nx = 1;
      ChildMask.Bits.Nx      = 1;
    }

    if (ChildMask.Uint64 != 0) {
      if (Modify) {
        //
        // Update child entries to use restrictive attribute inherited from parent.
        // e.g.: Set PDE[0-255].ReadWrite = 0
        //
        for (Index = 0; Index < 512; Index++) {
          if (PagingEntry[Index].Pce.Present == 0) {
            continue;
          }

          if (IsPle (&PagingEntry[Index], Level)) {
            PageTableLibSetPle (Level, &PagingEntry[Index], 0, &ChildAttribute, &ChildMask);
          } else {
            PageTableLibSetPnle (&PagingEntry[Index].Pnle, &ChildAttribute, &ChildMask);
          }
        }
      }
    }
  }

  //
  // RegionStart:  points to the linear address that's aligned on RegionLength and lower than (LinearAddress + Offset).
  //
  Index                   = PagingEntryIndex;
  RegionStart             = (LinearAddress + Offset) & ~RegionMask;
  ParentAttribute->Uint64 = PageTableLibGetPnleMapAttribute (&ParentPagingEntry->Pnle, ParentAttribute);

  //
  // Apply the attribute.
  //
  PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)IA32_PNLE_PAGE_TABLE_BASE_ADDRESS (&ParentPagingEntry->Pnle);
  while (Offset < Length && Index < 512) {
    CurrentPagingEntry = (!Modify && CreateNew) ? &OneOfPagingEntry : &PagingEntry[Index];
    SubLength          = MIN (Length - Offset, RegionStart + RegionLength - (LinearAddress + Offset));
    if ((Level <= MaxLeafLevel) &&
        (((LinearAddress + Offset) & RegionMask) == 0) &&
        (((IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (Attribute) + Offset) & RegionMask) == 0) &&
        (SubLength == RegionLength) &&
        ((CurrentPagingEntry->Pce.Present == 0) || IsPle (CurrentPagingEntry, Level))
        )
    {
      //
      // Create one entry mapping the entire region (1G, 2M or 4K).
      //
      if (Modify) {
        //
        // When the inheritable attributes in parent entry could override the child attributes,
        // e.g.: Present/ReadWrite/UserSupervisor is 0 in parent entry, or
        //       Nx is 1 in parent entry,
        // we just skip setting any value to these attributes in child.
        // We add assertion to make sure the requested settings don't conflict with parent attributes in this case.
        //
        CurrentMask.Uint64 = Mask->Uint64;
        if (ParentAttribute->Bits.Present == 0) {
          CurrentMask.Bits.Present = 0;
          ASSERT (CreateNew || (Mask->Bits.Present == 0) || (Attribute->Bits.Present == 0));
        }

        if (ParentAttribute->Bits.ReadWrite == 0) {
          CurrentMask.Bits.ReadWrite = 0;
          ASSERT (CreateNew || (Mask->Bits.ReadWrite == 0) || (Attribute->Bits.ReadWrite == 0));
        }

        if (ParentAttribute->Bits.UserSupervisor == 0) {
          CurrentMask.Bits.UserSupervisor = 0;
          ASSERT (CreateNew || (Mask->Bits.UserSupervisor == 0) || (Attribute->Bits.UserSupervisor == 0));
        }

        if (ParentAttribute->Bits.Nx == 1) {
          CurrentMask.Bits.Nx = 0;
          ASSERT (CreateNew || (Mask->Bits.Nx == 0) || (Attribute->Bits.Nx == 1));
        }

        //
        // Check if any leaf PagingEntry is modified.
        //
        OriginalCurrentPagingEntry.Uint64 = CurrentPagingEntry->Uint64;
        PageTableLibSetPle (Level, CurrentPagingEntry, Offset, Attribute, &CurrentMask);

        if (Modify && (OriginalCurrentPagingEntry.Uint64 != CurrentPagingEntry->Uint64)) {
          //
          // The page table entry can be changed by this function only when Modify is true.
          //
          *IsModified = TRUE;
        }
      }
    } else {
      //
      // Recursively call to create page table.
      // There are 3 cases:
      //   a. Level cannot be a leaf entry which points to physical memory.
      //   a. Level can be a leaf entry but (LinearAddress + Offset) is NOT aligned on the RegionStart.
      //   b. Level can be a leaf entry and (LinearAddress + Offset) is aligned on RegionStart,
      //      but the length is SMALLER than the RegionLength.
      //
      Status = PageTableLibMapInLevel (
                 CurrentPagingEntry,
                 ParentAttribute,
                 Modify,
                 Buffer,
                 BufferSize,
                 Level - 1,
                 MaxLeafLevel,
                 LinearAddress,
                 Length,
                 Offset,
                 Attribute,
                 Mask,
                 IsModified
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }

    Offset      += SubLength;
    RegionStart += RegionLength;
    Index++;
  }

  //
  // Check if ParentPagingEntry entry is modified here is enough. Except the changes happen in leaf PagingEntry during
  // the while loop, if there is any other change happens in page table, the ParentPagingEntry must has been modified.
  //
  if (Modify && (OriginalParentPagingEntry.Uint64 != ParentPagingEntry->Uint64)) {
    //
    // The page table entry can be changed by this function only when Modify is true.
    //
    *IsModified = TRUE;
  }

  return RETURN_SUCCESS;
}

/**
  Create or update page table to map [LinearAddress, LinearAddress + Length) with specified attribute.

  @param[in, out] PageTable      The pointer to the page table to update, or pointer to NULL if a new page table is to be created.
                                 If not pointer to NULL, the value it points to won't be changed in this function.
  @param[in]      PagingMode     The paging mode.
  @param[in]      Buffer         The free buffer to be used for page table creation/updating.
  @param[in, out] BufferSize     The buffer size.
                                 On return, the remaining buffer size.
                                 The free buffer is used from the end so caller can supply the same Buffer pointer with an updated
                                 BufferSize in the second call to this API.
  @param[in]      LinearAddress  The start of the linear address range.
  @param[in]      Length         The length of the linear address range.
  @param[in]      Attribute      The attribute of the linear address range.
                                 All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                                 Page table entries that map the linear address range are reset to 0 before set to the new attribute
                                 when a new physical base address is set.
  @param[in]      Mask           The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
  @param[out]     IsModified     TRUE means page table is modified by software or hardware. FALSE means page table is not modified by software.
                                 If the output IsModified is FALSE, there is possibility that the page table is changed by hardware. It is ok
                                 because page table can be changed by hardware anytime, and caller don't need to Flush TLB.

  @retval RETURN_UNSUPPORTED        PagingMode is not supported.
  @retval RETURN_INVALID_PARAMETER  PageTable, BufferSize, Attribute or Mask is NULL.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 1 but some other attributes are not provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  *BufferSize is not multiple of 4KB.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for page table creation/updating.
                                    BufferSize is updated to indicate the expected buffer size.
                                    Caller may still get RETURN_BUFFER_TOO_SMALL with the new BufferSize.
  @retval RETURN_SUCCESS            PageTable is created/updated successfully or the input Length is 0.
**/
RETURN_STATUS
EFIAPI
PageTableMap (
  IN OUT UINTN               *PageTable  OPTIONAL,
  IN     PAGING_MODE         PagingMode,
  IN     VOID                *Buffer,
  IN OUT UINTN               *BufferSize,
  IN     UINT64              LinearAddress,
  IN     UINT64              Length,
  IN     IA32_MAP_ATTRIBUTE  *Attribute,
  IN     IA32_MAP_ATTRIBUTE  *Mask,
  OUT    BOOLEAN             *IsModified   OPTIONAL
  )
{
  RETURN_STATUS       Status;
  IA32_PAGING_ENTRY   TopPagingEntry;
  INTN                RequiredSize;
  UINT64              MaxLinearAddress;
  IA32_PAGE_LEVEL     MaxLevel;
  IA32_PAGE_LEVEL     MaxLeafLevel;
  IA32_MAP_ATTRIBUTE  ParentAttribute;
  BOOLEAN             LocalIsModified;
  UINTN               Index;
  IA32_PAGING_ENTRY   *PagingEntry;
  UINT8               BufferInStack[SIZE_4KB - 1 + MAX_PAE_PDPTE_NUM * sizeof (IA32_PAGING_ENTRY)];

  if (Length == 0) {
    return RETURN_SUCCESS;
  }

  if ((PagingMode == Paging32bit) || (PagingMode >= PagingModeMax)) {
    //
    // 32bit paging is never supported.
    //
    return RETURN_UNSUPPORTED;
  }

  if ((PageTable == NULL) || (BufferSize == NULL) || (Attribute == NULL) || (Mask == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (*BufferSize % SIZE_4KB != 0) {
    //
    // BufferSize should be multiple of 4K.
    //
    return RETURN_INVALID_PARAMETER;
  }

  if (((UINTN)LinearAddress % SIZE_4KB != 0) || ((UINTN)Length % SIZE_4KB != 0)) {
    //
    // LinearAddress and Length should be multiple of 4K.
    //
    return RETURN_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // If to map [LinearAddress, LinearAddress + Length] as non-present,
  // all attributes except Present should not be provided.
  //
  if ((Attribute->Bits.Present == 0) && (Mask->Bits.Present == 1) && (Mask->Uint64 > 1)) {
    return RETURN_INVALID_PARAMETER;
  }

  MaxLeafLevel     = (IA32_PAGE_LEVEL)(UINT8)PagingMode;
  MaxLevel         = (IA32_PAGE_LEVEL)(UINT8)(PagingMode >> 8);
  MaxLinearAddress = (PagingMode == PagingPae) ? LShiftU64 (1, 32) : LShiftU64 (1, 12 + MaxLevel * 9);

  if ((LinearAddress > MaxLinearAddress) || (Length > MaxLinearAddress - LinearAddress)) {
    //
    // Maximum linear address is (1 << 32), (1 << 48) or (1 << 57)
    //
    return RETURN_INVALID_PARAMETER;
  }

  TopPagingEntry.Uintn = *PageTable;
  if (TopPagingEntry.Uintn != 0) {
    if (PagingMode == PagingPae) {
      //
      // Create 4 temporary PDPTE at a 4k-aligned address.
      // Copy the original PDPTE content and set ReadWrite, UserSupervisor to 1, set Nx to 0.
      //
      TopPagingEntry.Uintn = ALIGN_VALUE ((UINTN)BufferInStack, BASE_4KB);
      PagingEntry          = (IA32_PAGING_ENTRY *)(TopPagingEntry.Uintn);
      CopyMem (PagingEntry, (VOID *)(*PageTable), MAX_PAE_PDPTE_NUM * sizeof (IA32_PAGING_ENTRY));
      for (Index = 0; Index < MAX_PAE_PDPTE_NUM; Index++) {
        PagingEntry[Index].Pnle.Bits.ReadWrite      = 1;
        PagingEntry[Index].Pnle.Bits.UserSupervisor = 1;
        PagingEntry[Index].Pnle.Bits.Nx             = 0;
      }
    }

    TopPagingEntry.Pce.Present        = 1;
    TopPagingEntry.Pce.ReadWrite      = 1;
    TopPagingEntry.Pce.UserSupervisor = 1;
    TopPagingEntry.Pce.Nx             = 0;
  }

  if (IsModified == NULL) {
    IsModified = &LocalIsModified;
  }

  *IsModified = FALSE;

  ParentAttribute.Uint64                       = 0;
  ParentAttribute.Bits.PageTableBaseAddressLow = 1;
  ParentAttribute.Bits.Present                 = 1;
  ParentAttribute.Bits.ReadWrite               = 1;
  ParentAttribute.Bits.UserSupervisor          = 1;
  ParentAttribute.Bits.Nx                      = 0;

  //
  // Query the required buffer size without modifying the page table.
  //
  RequiredSize = 0;
  Status       = PageTableLibMapInLevel (
                   &TopPagingEntry,
                   &ParentAttribute,
                   FALSE,
                   NULL,
                   &RequiredSize,
                   MaxLevel,
                   MaxLeafLevel,
                   LinearAddress,
                   Length,
                   0,
                   Attribute,
                   Mask,
                   IsModified
                   );
  ASSERT (*IsModified == FALSE);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  RequiredSize = -RequiredSize;

  if ((UINTN)RequiredSize > *BufferSize) {
    *BufferSize = RequiredSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if ((RequiredSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Update the page table when the supplied buffer is sufficient.
  //
  Status = PageTableLibMapInLevel (
             &TopPagingEntry,
             &ParentAttribute,
             TRUE,
             Buffer,
             (INTN *)BufferSize,
             MaxLevel,
             MaxLeafLevel,
             LinearAddress,
             Length,
             0,
             Attribute,
             Mask,
             IsModified
             );

  if (!RETURN_ERROR (Status)) {
    PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)(TopPagingEntry.Uintn & IA32_PE_BASE_ADDRESS_MASK_40);

    if (PagingMode == PagingPae) {
      //
      // These MustBeZero fields are treated as RW and other attributes by the common map logic. So they might be set to 1.
      //
      for (Index = 0; Index < MAX_PAE_PDPTE_NUM; Index++) {
        PagingEntry[Index].PdptePae.Bits.MustBeZero  = 0;
        PagingEntry[Index].PdptePae.Bits.MustBeZero2 = 0;
        PagingEntry[Index].PdptePae.Bits.MustBeZero3 = 0;
      }

      if (*PageTable != 0) {
        //
        // Copy temp PDPTE to original PDPTE.
        //
        CopyMem ((VOID *)(*PageTable), PagingEntry, MAX_PAE_PDPTE_NUM * sizeof (IA32_PAGING_ENTRY));
      }
    }

    if (*PageTable == 0) {
      //
      // Do not assign the *PageTable when it's an existing page table.
      // If it's an existing PAE page table, PagingEntry is the temp buffer in stack.
      //
      *PageTable = (UINTN)PagingEntry;
    }
  }

  return Status;
}
