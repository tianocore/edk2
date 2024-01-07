/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2013-2021, Arm Limited. All rights reserved.<BR>
Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

/**
  Convert a set of ARM short descriptor section attributes into a mask
  of EFI_MEMORY_xx constants.

  @param[in]    SectionAttributes   The set of page attributes.
  @param[out]   GcdAttributes       Pointer to the return value.

  @retval EFI_SUCCESS       The attributes were converted successfully.
  @retval EFI_UNSUPPORTED   The section attributes did not have a
                            GCD transation.
**/
STATIC
EFI_STATUS
SectionToGcdAttributes (
  IN  UINT32  SectionAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch (SectionAttributes & TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK) {
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
  switch (SectionAttributes & TT_DESCRIPTOR_SECTION_AP_MASK) {
    case TT_DESCRIPTOR_SECTION_AP_NO_RW:
    case TT_DESCRIPTOR_SECTION_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_SECTION_AP_NO_RO:
    case TT_DESCRIPTOR_SECTION_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_RO;
      break;
  }

  // now process eXectue Never attribute
  if ((SectionAttributes & TT_DESCRIPTOR_SECTION_XN_MASK) != 0) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  if ((SectionAttributes & TT_DESCRIPTOR_SECTION_AF) == 0) {
    *GcdAttributes |= EFI_MEMORY_RP;
  }

  return EFI_SUCCESS;
}

/**
  Convert an arch specific set of page attributes into a mask
  of EFI_MEMORY_xx constants.

  @param[in] PageAttributes  The set of page attributes.

  @retval EFI_SUCCESS       The attributes were converted successfully.
  @retval EFI_UNSUPPORTED   The section attributes did not have a
                            GCD transation.
**/
UINT64
RegionAttributeToGcdAttribute (
  IN UINTN  PageAttributes
  )
{
  UINT64  Result;

  SectionToGcdAttributes (PageAttributes, &Result);
  return Result;
}

/**
  Convert a set of ARM short descriptor page attributes into a mask
  of EFI_MEMORY_xx constants.

  @param[in]    PageAttributes  The set of page attributes.
  @param[out]   GcdAttributes   Pointer to the return value.

  @retval EFI_SUCCESS       The attributes were converted successfully.
  @retval EFI_UNSUPPORTED   The page attributes did not have a GCD transation.
**/
STATIC
EFI_STATUS
PageToGcdAttributes (
  IN  UINT32  PageAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch (PageAttributes & TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK) {
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
  switch (PageAttributes & TT_DESCRIPTOR_PAGE_AP_MASK) {
    case TT_DESCRIPTOR_PAGE_AP_NO_RW:
    case TT_DESCRIPTOR_PAGE_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_PAGE_AP_NO_RO:
    case TT_DESCRIPTOR_PAGE_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_RO;
      break;
  }

  // now process eXectue Never attribute
  if ((PageAttributes & TT_DESCRIPTOR_PAGE_XN_MASK) != 0) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  if ((PageAttributes & TT_DESCRIPTOR_PAGE_AF) == 0) {
    *GcdAttributes |= EFI_MEMORY_RP;
  }

  return EFI_SUCCESS;
}

/**
  Synchronizes the GCD with the translation table for a specified page.

  This function synchronizes cache configuration for a given page based on its section index
  and the first level descriptor. It traverses the second level table entries of the page and
  updates the GCD attributes accordingly for each entry.

  @param[in]        SectionIndex            The index of the section where the page resides.
  @param[in]        FirstLevelDescriptor    The first translation table level of the page.
  @param[in]        NumberOfDescriptors     The number of descriptors in the GCD memory space map.
  @param[in]        MemorySpaceMap          The GCD memory space descriptor.
  @param[in, out]   NextRegionBase          The next region base address.
  @param[in, out]   NextRegionLength        The next region length.
  @param[in, out]   NextSectionAttributes   The next section attributes.

  @retval EFI_STATUS Always return success
**/
EFI_STATUS
SyncCacheConfigPage (
  IN     UINT32                           SectionIndex,
  IN     UINT32                           FirstLevelDescriptor,
  IN     UINTN                            NumberOfDescriptors,
  IN     EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN OUT EFI_PHYSICAL_ADDRESS             *NextRegionBase,
  IN OUT UINT64                           *NextRegionLength,
  IN OUT UINT32                           *NextSectionAttributes
  )
{
  EFI_STATUS                     Status;
  UINT32                         i;
  volatile ARM_PAGE_TABLE_ENTRY  *SecondLevelTable;
  UINT32                         NextPageAttributes;
  UINT32                         PageAttributes;
  UINT32                         BaseAddress;
  UINT64                         GcdAttributes;

  // Get the Base Address from FirstLevelDescriptor;
  BaseAddress = TT_DESCRIPTOR_PAGE_BASE_ADDRESS (SectionIndex << TT_DESCRIPTOR_SECTION_BASE_SHIFT);

  // Convert SectionAttributes into PageAttributes
  NextPageAttributes =
    TT_DESCRIPTOR_CONVERT_TO_PAGE_CACHE_POLICY (*NextSectionAttributes) |
    TT_DESCRIPTOR_CONVERT_TO_PAGE_AF (*NextSectionAttributes) |
    TT_DESCRIPTOR_CONVERT_TO_PAGE_AP (*NextSectionAttributes);

  // obtain page table base
  SecondLevelTable = (ARM_PAGE_TABLE_ENTRY *)(FirstLevelDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

  for (i = 0; i < TRANSLATION_TABLE_PAGE_COUNT; i++) {
    if ((SecondLevelTable[i] & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      // extract attributes (cacheability and permissions)
      PageAttributes = SecondLevelTable[i] & (TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK | TT_DESCRIPTOR_PAGE_AP_MASK | TT_DESCRIPTOR_PAGE_AF);

      if (NextPageAttributes == 0) {
        // start on a new region
        *NextRegionLength  = 0;
        *NextRegionBase    = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      } else if (PageAttributes != NextPageAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          GcdAttributes = 0;
        }

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

        // start on a new region
        *NextRegionLength  = 0;
        *NextRegionBase    = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      }
    } else if (NextPageAttributes != 0) {
      // Convert Page Attributes into GCD Attributes
      Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        GcdAttributes = 0;
      }

      // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
      SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

      *NextRegionLength  = 0;
      *NextRegionBase    = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
      NextPageAttributes = 0;
    }

    *NextRegionLength += TT_DESCRIPTOR_PAGE_SIZE;
  }

  // Convert back PageAttributes into SectionAttributes
  *NextSectionAttributes =
    TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY (NextPageAttributes) |
    TT_DESCRIPTOR_CONVERT_TO_SECTION_AF (NextPageAttributes) |
    TT_DESCRIPTOR_CONVERT_TO_SECTION_AP (NextPageAttributes);

  return EFI_SUCCESS;
}

/**
  Sync the GCD memory space attributes with the translation table.

  @param[in]  CpuProtocol  The CPU architectural protocol instance.

  @retval EFI_SUCCESS   The GCD memory space attributes are synced with the MMU page table.
  @retval Others        The return value of GetMemorySpaceMap().
**/
EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL  *CpuProtocol
  )
{
  EFI_STATUS                           Status;
  UINT32                               i;
  EFI_PHYSICAL_ADDRESS                 NextRegionBase;
  UINT64                               NextRegionLength;
  UINT32                               NextSectionAttributes;
  UINT32                               SectionAttributes;
  UINT64                               GcdAttributes;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR  *FirstLevelTable;
  UINTN                                NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR      *MemorySpaceMap;

  DEBUG ((DEBUG_PAGE, "SyncCacheConfig()\n"));

  // This code assumes MMU is enabled and filed with section translations
  ASSERT (ArmMmuEnabled ());

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status         = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SyncCacheConfig - GetMemorySpaceMap() failed! Status: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // The GCD implementation maintains its own copy of the state of memory space attributes.  GCD needs
  // to know what the initial memory space attributes are.  The CPU Arch. Protocol does not provide a
  // GetMemoryAttributes function for GCD to get this so we must resort to calling GCD (as if we were
  // a client) to update its copy of the attributes.  This is bad architecture and should be replaced
  // with a way for GCD to query the CPU Arch. driver of the existing memory space attributes instead.

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)(ArmGetTTBR0BaseAddress ());

  // Get the first region
  NextSectionAttributes = FirstLevelTable[0] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK | TT_DESCRIPTOR_SECTION_AF);

  // iterate through each 1MB descriptor
  NextRegionBase = NextRegionLength = 0;
  for (i = 0; i < TRANSLATION_TABLE_SECTION_COUNT; i++) {
    if ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) {
      // extract attributes (cacheability and permissions)
      SectionAttributes = FirstLevelTable[i] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK | TT_DESCRIPTOR_SECTION_AF);

      if (NextSectionAttributes == 0) {
        // start on a new region
        NextRegionLength      = 0;
        NextRegionBase        = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      } else if (SectionAttributes != NextSectionAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "SyncCacheConfig - SectionToGcdAttributes() failed! Status: %r\n", Status));
          ASSERT_EFI_ERROR (Status);
          GcdAttributes = 0;
        }

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        // start on a new region
        NextRegionLength      = 0;
        NextRegionBase        = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      }

      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    } else if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (FirstLevelTable[i])) {
      // In this case any bits set in the 'NextSectionAttributes' are garbage and were set from
      // bits that are actually part of the pagetable address.  We clear it out to zero so that
      // the SyncCacheConfigPage will use the page attributes instead of trying to convert the
      // section attributes into page attributes
      NextSectionAttributes = 0;
      Status                = SyncCacheConfigPage (
                                i,
                                FirstLevelTable[i],
                                NumberOfDescriptors,
                                MemorySpaceMap,
                                &NextRegionBase,
                                &NextRegionLength,
                                &NextSectionAttributes
                                );
      ASSERT_EFI_ERROR (Status);
    } else {
      // We do not support yet 16MB sections
      ASSERT ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) != TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION);

      // start on a new region
      if (NextSectionAttributes != 0) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "SyncCacheConfig - SectionToGcdAttributes() failed! Status: %r\n", Status));
          ASSERT_EFI_ERROR (Status);
          GcdAttributes = 0;
        }

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        NextRegionLength      = 0;
        NextRegionBase        = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = 0;
      }

      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    }
  } // section entry loop

  if (NextSectionAttributes != 0) {
    // Convert Section Attributes into GCD Attributes
    Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SyncCacheConfig - SectionToGcdAttributes() failed! Status: %r\n", Status));
      ASSERT_EFI_ERROR (Status);
      GcdAttributes = 0;
    }

    // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
    SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);
  }

  FreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}

/**
  Convert EFI memory attributes to ARM translation table attributes.

  @param[in]  EfiAttributes  EFI memory attributes.

  @retval The analogous translation table attributes.
**/
UINT64
EfiAttributeToArmAttribute (
  IN UINT64  EfiAttributes
  )
{
  UINT64  ArmAttributes;

  switch (EfiAttributes & EFI_MEMORY_CACHETYPE_MASK) {
    case EFI_MEMORY_UC:
      // Map to strongly ordered
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // Map to normal non-cacheable
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
  if ((EfiAttributes & EFI_MEMORY_RO) != 0) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RO_RO;
  } else {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RW_RW;
  }

  // Determine eXecute Never attribute
  if ((EfiAttributes & EFI_MEMORY_XP) != 0) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_XN_MASK;
  }

  if ((EfiAttributes & EFI_MEMORY_RP) == 0) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AF;
  }

  return ArmAttributes;
}

/**
  This function finds the end of a memory region in a translation table. A
  memory region is defined as a contiguous set of pages with the same attributes.

  @param[in]    PageTable         The translation table to traverse.
  @param[in]    BaseAddress       The address from which to start the search
  @param[in]    RegionAttributes  The attributes of the start of the region.
  @param[out]   RegionLength      The length of the region found.

  @retval EFI_SUCCESS       The region was found.
  @retval EFI_NOT_FOUND     The end of the region was not found.
  @retval EFI_NO_MAPPING    The region specified by BaseAddress is not mapped
                            in the input translation table.
  @retval EFI_UNSUPPORTED   Large pages are not supported.
**/
STATIC
EFI_STATUS
GetMemoryRegionPage (
  IN     UINT32  *PageTable,
  IN     UINTN   *BaseAddress,
  IN     UINTN   *RegionAttributes,
  OUT    UINTN   *RegionLength
  )
{
  UINT32      PageAttributes;
  UINT32      TableIndex;
  UINT32      PageDescriptor;
  EFI_STATUS  Status;

  // Convert the section attributes into page attributes
  PageAttributes = ConvertSectionAttributesToPageAttributes (*RegionAttributes);
  Status         = EFI_NOT_FOUND;
  *RegionLength  = 0;

  // Calculate index into first level translation table for start of modification
  TableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
  ASSERT (TableIndex < TRANSLATION_TABLE_PAGE_COUNT);

  // Go through the page table to find the end of the section
  for ( ; TableIndex < TRANSLATION_TABLE_PAGE_COUNT; TableIndex++) {
    // Get the section at the given index
    PageDescriptor = PageTable[TableIndex];

    if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_FAULT) {
      Status = (*RegionLength > 0) ? EFI_SUCCESS : EFI_NO_MAPPING;
      break;
    } else if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_PAGE) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      if ((PageDescriptor & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK) != PageAttributes) {
        Status = EFI_SUCCESS;
        break;
      }

      *RegionLength += TT_DESCRIPTOR_PAGE_SIZE;
    } else {
      // Large pages are unsupported.
      Status = EFI_UNSUPPORTED;
      ASSERT (0);
      break;
    }
  }

  return Status;
}

/**
  Get the memory region that contains the specified address. A memory region is defined
  as a contiguous set of pages with the same attributes.

  RegionLength and RegionAttributes are only valid if EFI_SUCCESS is returned.

  @param[in, out]   BaseAddress       On input, the address to search for.
                                      On output, the base address of the region found.
  @param[out]       RegionLength      The length of the region found.
  @param[out]       RegionAttributes  The attributes of the region found.

  @retval   EFI_SUCCESS             Region found
  @retval   EFI_NOT_FOUND           Region not found
  @retval   EFI_UNSUPPORTED         Large pages are unsupported
  @retval   EFI_NO_MAPPING          The page specified by BaseAddress is unmapped
  @retval   EFI_INVALID_PARAMETER   The BaseAddress exceeds the addressable range of
                                    the translation table.
**/
EFI_STATUS
GetMemoryRegion (
  IN OUT UINTN  *BaseAddress,
  OUT    UINTN  *RegionLength,
  OUT    UINTN  *RegionAttributes
  )
{
  EFI_STATUS                  Status;
  UINT32                      TableIndex;
  UINT32                      PageAttributes;
  UINT32                      PageTableIndex;
  UINT32                      SectionDescriptor;
  ARM_FIRST_LEVEL_DESCRIPTOR  *FirstLevelTable;
  UINT32                      *PageTable;
  UINTN                       Length;

  // Initialize the arguments
  *RegionLength = 0;

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  TableIndex = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (*BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;

  if (TableIndex >= TRANSLATION_TABLE_SECTION_COUNT) {
    ASSERT (TableIndex < TRANSLATION_TABLE_SECTION_COUNT);
    return EFI_INVALID_PARAMETER;
  }

  // Get the section at the given index
  SectionDescriptor = FirstLevelTable[TableIndex];
  if (!SectionDescriptor) {
    return EFI_NOT_FOUND;
  }

  // If 'BaseAddress' belongs to the section then round it to the section boundary
  if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
      ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION))
  {
    *BaseAddress      = (*BaseAddress) & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK;
    *RegionAttributes = SectionDescriptor & TT_DESCRIPTOR_SECTION_ATTRIBUTE_MASK;
  } else {
    // Otherwise, we round it to the page boundary
    *BaseAddress = (*BaseAddress) & TT_DESCRIPTOR_PAGE_BASE_ADDRESS_MASK;

    // Get the attribute at the page table level (Level 2)
    PageTable = (UINT32 *)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

    // Calculate index into first level translation table for start of modification
    PageTableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < TRANSLATION_TABLE_PAGE_COUNT);

    PageAttributes    = PageTable[PageTableIndex] & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK;
    *RegionAttributes = TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY (PageAttributes) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_S (PageAttributes) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_XN (PageAttributes) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_AF (PageAttributes) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_AP (PageAttributes);
  }

  Status = EFI_NOT_FOUND;

  for ( ; TableIndex < TRANSLATION_TABLE_SECTION_COUNT; TableIndex++) {
    // Get the section at the given index
    SectionDescriptor = FirstLevelTable[TableIndex];

    // If the entry is a level-2 page table then we scan it to find the end of the region
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (SectionDescriptor)) {
      // Extract the page table location from the descriptor
      PageTable = (UINT32 *)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);
      Length    = 0;

      // Scan the page table to find the end of the region.
      Status         = GetMemoryRegionPage (PageTable, BaseAddress, RegionAttributes, &Length);
      *RegionLength += Length;

      // Status == EFI_NOT_FOUND implies we have not reached the end of the region.
      if ((Status == EFI_NOT_FOUND) && (Length > 0)) {
        continue;
      }

      break;
    } else if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
               ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION))
    {
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

  // Check if the region length was updated.
  if (*RegionLength > 0) {
    Status = EFI_SUCCESS;
  }

  return Status;
}
