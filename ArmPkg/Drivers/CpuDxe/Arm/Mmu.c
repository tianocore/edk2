/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

EFI_STATUS
SectionToGcdAttributes (
  IN  UINT32  SectionAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch(SectionAttributes & TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK) {
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WT;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE:
      *GcdAttributes |= EFI_MEMORY_WC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  // determine protection attributes
  switch(SectionAttributes & TT_DESCRIPTOR_SECTION_AP_MASK) {
    case TT_DESCRIPTOR_SECTION_AP_NO_NO: // no read, no write
      //*GcdAttributes |= EFI_MEMORY_RO | EFI_MEMORY_RP;
      break;

    case TT_DESCRIPTOR_SECTION_AP_RW_NO:
    case TT_DESCRIPTOR_SECTION_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_SECTION_AP_RO_NO:
    case TT_DESCRIPTOR_SECTION_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_RO;
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // now process eXectue Never attribute
  if ((SectionAttributes & TT_DESCRIPTOR_SECTION_XN_MASK) != 0 ) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PageToGcdAttributes (
  IN  UINT32  PageAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch(PageAttributes & TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK) {
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WT;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_CACHEABLE:
      *GcdAttributes |= EFI_MEMORY_WC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  // determine protection attributes
  switch(PageAttributes & TT_DESCRIPTOR_PAGE_AP_MASK) {
    case TT_DESCRIPTOR_PAGE_AP_NO_NO: // no read, no write
      //*GcdAttributes |= EFI_MEMORY_RO | EFI_MEMORY_RP;
      break;

    case TT_DESCRIPTOR_PAGE_AP_RW_NO:
    case TT_DESCRIPTOR_PAGE_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_PAGE_AP_RO_NO:
    case TT_DESCRIPTOR_PAGE_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_RO;
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // now process eXectue Never attribute
  if ((PageAttributes & TT_DESCRIPTOR_PAGE_XN_MASK) != 0 ) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SyncCacheConfigPage (
  IN     UINT32                             SectionIndex,
  IN     UINT32                             FirstLevelDescriptor,
  IN     UINTN                              NumberOfDescriptors,
  IN     EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap,
  IN OUT EFI_PHYSICAL_ADDRESS               *NextRegionBase,
  IN OUT UINT64                             *NextRegionLength,
  IN OUT UINT32                             *NextSectionAttributes
  )
{
  EFI_STATUS                          Status;
  UINT32                              i;
  volatile ARM_PAGE_TABLE_ENTRY       *SecondLevelTable;
  UINT32                              NextPageAttributes = 0;
  UINT32                              PageAttributes = 0;
  UINT32                              BaseAddress;
  UINT64                              GcdAttributes;

  // Get the Base Address from FirstLevelDescriptor;
  BaseAddress = TT_DESCRIPTOR_PAGE_BASE_ADDRESS(SectionIndex << TT_DESCRIPTOR_SECTION_BASE_SHIFT);

  // Convert SectionAttributes into PageAttributes
  NextPageAttributes =
      TT_DESCRIPTOR_CONVERT_TO_PAGE_CACHE_POLICY(*NextSectionAttributes,0) |
      TT_DESCRIPTOR_CONVERT_TO_PAGE_AP(*NextSectionAttributes);

  // obtain page table base
  SecondLevelTable = (ARM_PAGE_TABLE_ENTRY *)(FirstLevelDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

  for (i=0; i < TRANSLATION_TABLE_PAGE_COUNT; i++) {
    if ((SecondLevelTable[i] & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      // extract attributes (cacheability and permissions)
      PageAttributes = SecondLevelTable[i] & (TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK | TT_DESCRIPTOR_PAGE_AP_MASK);

      if (NextPageAttributes == 0) {
        // start on a new region
        *NextRegionLength = 0;
        *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      } else if (PageAttributes != NextPageAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

        // start on a new region
        *NextRegionLength = 0;
        *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      }
    } else if (NextPageAttributes != 0) {
      // Convert Page Attributes into GCD Attributes
      Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
      ASSERT_EFI_ERROR (Status);

      // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
      SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

      *NextRegionLength = 0;
      *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
      NextPageAttributes = 0;
    }
    *NextRegionLength += TT_DESCRIPTOR_PAGE_SIZE;
  }

  // Convert back PageAttributes into SectionAttributes
  *NextSectionAttributes =
      TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY(NextPageAttributes,0) |
      TT_DESCRIPTOR_CONVERT_TO_SECTION_AP(NextPageAttributes);

  return EFI_SUCCESS;
}

EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL *CpuProtocol
  )
{
  EFI_STATUS                          Status;
  UINT32                              i;
  EFI_PHYSICAL_ADDRESS                NextRegionBase;
  UINT64                              NextRegionLength;
  UINT32                              NextSectionAttributes = 0;
  UINT32                              SectionAttributes = 0;
  UINT64                              GcdAttributes;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;


  DEBUG ((EFI_D_PAGE, "SyncCacheConfig()\n"));

  // This code assumes MMU is enabled and filed with section translations
  ASSERT (ArmMmuEnabled ());

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);


  // The GCD implementation maintains its own copy of the state of memory space attributes.  GCD needs
  // to know what the initial memory space attributes are.  The CPU Arch. Protocol does not provide a
  // GetMemoryAttributes function for GCD to get this so we must resort to calling GCD (as if we were
  // a client) to update its copy of the attributes.  This is bad architecture and should be replaced
  // with a way for GCD to query the CPU Arch. driver of the existing memory space attributes instead.

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)(ArmGetTTBR0BaseAddress ());

  // Get the first region
  NextSectionAttributes = FirstLevelTable[0] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK);

  // iterate through each 1MB descriptor
  NextRegionBase = NextRegionLength = 0;
  for (i=0; i < TRANSLATION_TABLE_SECTION_COUNT; i++) {
    if ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) {
      // extract attributes (cacheability and permissions)
      SectionAttributes = FirstLevelTable[i] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK);

      if (NextSectionAttributes == 0) {
        // start on a new region
        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      } else if (SectionAttributes != NextSectionAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        // start on a new region
        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      }
      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    } else if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(FirstLevelTable[i])) {
      // In this case any bits set in the 'NextSectionAttributes' are garbage and were set from
      // bits that are actually part of the pagetable address.  We clear it out to zero so that
      // the SyncCacheConfigPage will use the page attributes instead of trying to convert the
      // section attributes into page attributes
      NextSectionAttributes = 0;
      Status = SyncCacheConfigPage (
          i,FirstLevelTable[i],
          NumberOfDescriptors, MemorySpaceMap,
          &NextRegionBase,&NextRegionLength,&NextSectionAttributes);
      ASSERT_EFI_ERROR (Status);
    } else {
      // We do not support yet 16MB sections
      ASSERT ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) != TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION);

      // start on a new region
      if (NextSectionAttributes != 0) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = 0;
      }
      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    }
  } // section entry loop

  if (NextSectionAttributes != 0) {
    // Convert Section Attributes into GCD Attributes
    Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
    ASSERT_EFI_ERROR (Status);

    // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
    SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);
  }

  FreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}

UINT64
EfiAttributeToArmAttribute (
  IN UINT64                    EfiAttributes
  )
{
  UINT64 ArmAttributes;

  switch (EfiAttributes & EFI_MEMORY_CACHETYPE_MASK) {
    case EFI_MEMORY_UC:
      // Map to strongly ordered
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // Map to normal non-cachable
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // Write through with no-allocate
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // Write back (with allocate)
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_UCE:
    default:
      ArmAttributes = TT_DESCRIPTOR_SECTION_TYPE_FAULT;
      break;
  }

  // Determine protection attributes
  if (EfiAttributes & EFI_MEMORY_RO) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RO_RO;
  } else {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RW_RW;
  }

  // Determine eXecute Never attribute
  if (EfiAttributes & EFI_MEMORY_XP) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_XN_MASK;
  }

  return ArmAttributes;
}

EFI_STATUS
GetMemoryRegionPage (
  IN     UINT32                  *PageTable,
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  UINT32      PageAttributes;
  UINT32      TableIndex;
  UINT32      PageDescriptor;

  // Convert the section attributes into page attributes
  PageAttributes = ConvertSectionAttributesToPageAttributes (*RegionAttributes, 0);

  // Calculate index into first level translation table for start of modification
  TableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
  ASSERT (TableIndex < TRANSLATION_TABLE_PAGE_COUNT);

  // Go through the page table to find the end of the section
  for (; TableIndex < TRANSLATION_TABLE_PAGE_COUNT; TableIndex++) {
    // Get the section at the given index
    PageDescriptor = PageTable[TableIndex];

    if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_FAULT) {
      // Case: End of the boundary of the region
      return EFI_SUCCESS;
    } else if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_PAGE) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      if ((PageDescriptor & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK) == PageAttributes) {
        *RegionLength = *RegionLength + TT_DESCRIPTOR_PAGE_SIZE;
      } else {
        // Case: End of the boundary of the region
        return EFI_SUCCESS;
      }
    } else {
      // We do not support Large Page yet. We return EFI_SUCCESS that means end of the region.
      ASSERT(0);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GetMemoryRegion (
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  EFI_STATUS                  Status;
  UINT32                      TableIndex;
  UINT32                      PageAttributes;
  UINT32                      PageTableIndex;
  UINT32                      SectionDescriptor;
  ARM_FIRST_LEVEL_DESCRIPTOR *FirstLevelTable;
  UINT32                     *PageTable;

  // Initialize the arguments
  *RegionLength = 0;

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  TableIndex = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (*BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (TableIndex < TRANSLATION_TABLE_SECTION_COUNT);

  // Get the section at the given index
  SectionDescriptor = FirstLevelTable[TableIndex];
  if (!SectionDescriptor) {
    return EFI_NOT_FOUND;
  }

  // If 'BaseAddress' belongs to the section then round it to the section boundary
  if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
      ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION))
  {
    *BaseAddress = (*BaseAddress) & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK;
    *RegionAttributes = SectionDescriptor & TT_DESCRIPTOR_SECTION_ATTRIBUTE_MASK;
  } else {
    // Otherwise, we round it to the page boundary
    *BaseAddress = (*BaseAddress) & TT_DESCRIPTOR_PAGE_BASE_ADDRESS_MASK;

    // Get the attribute at the page table level (Level 2)
    PageTable = (UINT32*)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

    // Calculate index into first level translation table for start of modification
    PageTableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < TRANSLATION_TABLE_PAGE_COUNT);

    PageAttributes = PageTable[PageTableIndex] & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK;
    *RegionAttributes = TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY (PageAttributes, 0) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_AP (PageAttributes);
  }

  for (;TableIndex < TRANSLATION_TABLE_SECTION_COUNT; TableIndex++) {
    // Get the section at the given index
    SectionDescriptor = FirstLevelTable[TableIndex];

    // If the entry is a level-2 page table then we scan it to find the end of the region
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (SectionDescriptor)) {
      // Extract the page table location from the descriptor
      PageTable = (UINT32*)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

      // Scan the page table to find the end of the region.
      Status = GetMemoryRegionPage (PageTable, BaseAddress, RegionLength, RegionAttributes);

      // If we have found the end of the region (Status == EFI_SUCCESS) then we exit the for-loop
      if (Status == EFI_SUCCESS) {
        break;
      }
    } else if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
               ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION)) {
      if ((SectionDescriptor & TT_DESCRIPTOR_SECTION_ATTRIBUTE_MASK) != *RegionAttributes) {
        // If the attributes of the section differ from the one targeted then we exit the loop
        break;
      } else {
        *RegionLength = *RegionLength + TT_DESCRIPTOR_SECTION_SIZE;
      }
    } else {
      // If we are on an invalid section then it means it is the end of our section.
      break;
    }
  }

  return EFI_SUCCESS;
}
