/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

#define INVALID_ENTRY   ((UINT32)~0)

#define MIN_T0SZ        16
#define BITS_PER_LEVEL  9

STATIC
VOID
GetRootTranslationTableInfo (
  IN  UINTN     T0SZ,
  OUT UINTN     *RootTableLevel,
  OUT UINTN     *RootTableEntryCount
  )
{
  *RootTableLevel       = (T0SZ - MIN_T0SZ) / BITS_PER_LEVEL;
  *RootTableEntryCount  = TT_ENTRY_COUNT >> (T0SZ - MIN_T0SZ) % BITS_PER_LEVEL;
}

STATIC
UINT64
PageAttributeToGcdAttribute (
  IN UINT64 PageAttributes
  )
{
  UINT64  GcdAttributes;

  switch (PageAttributes & TT_ATTR_INDX_MASK) {
  case TT_ATTR_INDX_DEVICE_MEMORY:
    GcdAttributes = EFI_MEMORY_UC;
    break;
  case TT_ATTR_INDX_MEMORY_NON_CACHEABLE:
    GcdAttributes = EFI_MEMORY_WC;
    break;
  case TT_ATTR_INDX_MEMORY_WRITE_THROUGH:
    GcdAttributes = EFI_MEMORY_WT;
    break;
  case TT_ATTR_INDX_MEMORY_WRITE_BACK:
    GcdAttributes = EFI_MEMORY_WB;
    break;
  default:
    DEBUG ((DEBUG_ERROR,
      "PageAttributeToGcdAttribute: PageAttributes:0x%lX not supported.\n",
      PageAttributes));
    ASSERT (0);
    // The Global Coherency Domain (GCD) value is defined as a bit set.
    // Returning 0 means no attribute has been set.
    GcdAttributes = 0;
  }

  // Determine protection attributes
  if (((PageAttributes & TT_AP_MASK) == TT_AP_NO_RO) ||
      ((PageAttributes & TT_AP_MASK) == TT_AP_RO_RO)) {
    // Read only cases map to write-protect
    GcdAttributes |= EFI_MEMORY_RO;
  }

  // Process eXecute Never attribute
  if ((PageAttributes & (TT_PXN_MASK | TT_UXN_MASK)) != 0) {
    GcdAttributes |= EFI_MEMORY_XP;
  }

  return GcdAttributes;
}

STATIC
UINT64
GetFirstPageAttribute (
  IN UINT64  *FirstLevelTableAddress,
  IN UINTN    TableLevel
  )
{
  UINT64 FirstEntry;

  // Get the first entry of the table
  FirstEntry = *FirstLevelTableAddress;

  if ((TableLevel != 3) && (FirstEntry & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY) {
    // Only valid for Levels 0, 1 and 2

    // Get the attribute of the subsequent table
    return GetFirstPageAttribute ((UINT64*)(FirstEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE), TableLevel + 1);
  } else if (((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY) ||
             ((TableLevel == 3) && ((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3)))
  {
    return FirstEntry & TT_ATTR_INDX_MASK;
  } else {
    return INVALID_ENTRY;
  }
}

STATIC
UINT64
GetNextEntryAttribute (
  IN     UINT64 *TableAddress,
  IN     UINTN   EntryCount,
  IN     UINTN   TableLevel,
  IN     UINT64  BaseAddress,
  IN OUT UINT32 *PrevEntryAttribute,
  IN OUT UINT64 *StartGcdRegion
  )
{
  UINTN                             Index;
  UINT64                            Entry;
  UINT32                            EntryAttribute;
  UINT32                            EntryType;
  EFI_STATUS                        Status;
  UINTN                             NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;

  // Get the memory space map from GCD
  MemorySpaceMap = NULL;
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  // We cannot get more than 3-level page table
  ASSERT (TableLevel <= 3);

  // While the top level table might not contain TT_ENTRY_COUNT entries;
  // the subsequent ones should be filled up
  for (Index = 0; Index < EntryCount; Index++) {
    Entry = TableAddress[Index];
    EntryType = Entry & TT_TYPE_MASK;
    EntryAttribute = Entry  & TT_ATTR_INDX_MASK;

    // If Entry is a Table Descriptor type entry then go through the sub-level table
    if ((EntryType == TT_TYPE_BLOCK_ENTRY) ||
        ((TableLevel == 3) && (EntryType == TT_TYPE_BLOCK_ENTRY_LEVEL3))) {
      if ((*PrevEntryAttribute == INVALID_ENTRY) || (EntryAttribute != *PrevEntryAttribute)) {
        if (*PrevEntryAttribute != INVALID_ENTRY) {
          // Update GCD with the last region
          SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
              *StartGcdRegion,
              (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel))) - *StartGcdRegion,
              PageAttributeToGcdAttribute (*PrevEntryAttribute));
        }

        // Start of the new region
        *StartGcdRegion = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel));
        *PrevEntryAttribute = EntryAttribute;
      } else {
        continue;
      }
    } else if (EntryType == TT_TYPE_TABLE_ENTRY) {
      // Table Entry type is only valid for Level 0, 1, 2
      ASSERT (TableLevel < 3);

      // Increase the level number and scan the sub-level table
      GetNextEntryAttribute ((UINT64*)(Entry & TT_ADDRESS_MASK_DESCRIPTION_TABLE),
                             TT_ENTRY_COUNT, TableLevel + 1,
                             (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel))),
                             PrevEntryAttribute, StartGcdRegion);
    } else {
      if (*PrevEntryAttribute != INVALID_ENTRY) {
        // Update GCD with the last region
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
            *StartGcdRegion,
            (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel))) - *StartGcdRegion,
            PageAttributeToGcdAttribute (*PrevEntryAttribute));

        // Start of the new region
        *StartGcdRegion = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel));
        *PrevEntryAttribute = INVALID_ENTRY;
      }
    }
  }

  FreePool (MemorySpaceMap);

  return BaseAddress + (EntryCount * TT_ADDRESS_AT_LEVEL(TableLevel));
}

EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL *CpuProtocol
  )
{
  EFI_STATUS                          Status;
  UINT32                              PageAttribute;
  UINT64                             *FirstLevelTableAddress;
  UINTN                               TableLevel;
  UINTN                               TableCount;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap;
  UINTN                               Tcr;
  UINTN                               T0SZ;
  UINT64                              BaseAddressGcdRegion;
  UINT64                              EndAddressGcdRegion;

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

  // Obtain page table base
  FirstLevelTableAddress = (UINT64*)(ArmGetTTBR0BaseAddress ());

  // Get Translation Control Register value
  Tcr = ArmGetTCR ();
  // Get Address Region Size
  T0SZ = Tcr & TCR_T0SZ_MASK;

  // Get the level of the first table for the indicated Address Region Size
  GetRootTranslationTableInfo (T0SZ, &TableLevel, &TableCount);

  // First Attribute of the Page Tables
  PageAttribute = GetFirstPageAttribute (FirstLevelTableAddress, TableLevel);

  // We scan from the start of the memory map (ie: at the address 0x0)
  BaseAddressGcdRegion = 0x0;
  EndAddressGcdRegion = GetNextEntryAttribute (FirstLevelTableAddress,
                                               TableCount, TableLevel,
                                               BaseAddressGcdRegion,
                                               &PageAttribute, &BaseAddressGcdRegion);

  // Update GCD with the last region if valid
  if (PageAttribute != INVALID_ENTRY) {
    SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
        BaseAddressGcdRegion,
        EndAddressGcdRegion - BaseAddressGcdRegion,
        PageAttributeToGcdAttribute (PageAttribute));
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
    if (ArmReadCurrentEL () == AARCH64_EL2) {
      ArmAttributes = TT_ATTR_INDX_DEVICE_MEMORY | TT_XN_MASK;
    } else {
      ArmAttributes = TT_ATTR_INDX_DEVICE_MEMORY | TT_UXN_MASK | TT_PXN_MASK;
    }
    break;
  case EFI_MEMORY_WC:
    ArmAttributes = TT_ATTR_INDX_MEMORY_NON_CACHEABLE;
    break;
  case EFI_MEMORY_WT:
    ArmAttributes = TT_ATTR_INDX_MEMORY_WRITE_THROUGH | TT_SH_INNER_SHAREABLE;
    break;
  case EFI_MEMORY_WB:
    ArmAttributes = TT_ATTR_INDX_MEMORY_WRITE_BACK | TT_SH_INNER_SHAREABLE;
    break;
  default:
    ArmAttributes = TT_ATTR_INDX_MASK;
  }

  // Set the access flag to match the block attributes
  ArmAttributes |= TT_AF;

  // Determine protection attributes
  if ((EfiAttributes & EFI_MEMORY_RO) != 0) {
    ArmAttributes |= TT_AP_RO_RO;
  }

  // Process eXecute Never attribute
  if ((EfiAttributes & EFI_MEMORY_XP) != 0) {
    ArmAttributes |= TT_PXN_MASK;
  }

  return ArmAttributes;
}

// This function will recursively go down the page table to find the first block address linked to 'BaseAddress'.
// And then the function will identify the size of the region that has the same page table attribute.
EFI_STATUS
GetMemoryRegionRec (
  IN     UINT64                  *TranslationTable,
  IN     UINTN                    TableLevel,
  IN     UINT64                  *LastBlockEntry,
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  EFI_STATUS Status;
  UINT64    *NextTranslationTable;
  UINT64    *BlockEntry;
  UINT64     BlockEntryType;
  UINT64     EntryType;

  if (TableLevel != 3) {
    BlockEntryType = TT_TYPE_BLOCK_ENTRY;
  } else {
    BlockEntryType = TT_TYPE_BLOCK_ENTRY_LEVEL3;
  }

  // Find the block entry linked to the Base Address
  BlockEntry = (UINT64*)TT_GET_ENTRY_FOR_ADDRESS (TranslationTable, TableLevel, *BaseAddress);
  EntryType = *BlockEntry & TT_TYPE_MASK;

  if ((TableLevel < 3) && (EntryType == TT_TYPE_TABLE_ENTRY)) {
    NextTranslationTable = (UINT64*)(*BlockEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE);

    // The entry is a page table, so we go to the next level
    Status = GetMemoryRegionRec (
        NextTranslationTable, // Address of the next level page table
        TableLevel + 1, // Next Page Table level
        (UINTN*)TT_LAST_BLOCK_ADDRESS(NextTranslationTable, TT_ENTRY_COUNT),
        BaseAddress, RegionLength, RegionAttributes);

    // In case of 'Success', it means the end of the block region has been found into the upper
    // level translation table
    if (!EFI_ERROR(Status)) {
      return EFI_SUCCESS;
    }

    // Now we processed the table move to the next entry
    BlockEntry++;
  } else if (EntryType == BlockEntryType) {
    // We have found the BlockEntry attached to the address. We save its start address (the start
    // address might be before the 'BaseAddress') and attributes
    *BaseAddress      = *BaseAddress & ~(TT_ADDRESS_AT_LEVEL(TableLevel) - 1);
    *RegionLength     = 0;
    *RegionAttributes = *BlockEntry & TT_ATTRIBUTES_MASK;
  } else {
    // We have an 'Invalid' entry
    return EFI_UNSUPPORTED;
  }

  while (BlockEntry <= LastBlockEntry) {
    if ((*BlockEntry & TT_ATTRIBUTES_MASK) == *RegionAttributes) {
      *RegionLength = *RegionLength + TT_BLOCK_ENTRY_SIZE_AT_LEVEL(TableLevel);
    } else {
      // In case we have found the end of the region we return success
      return EFI_SUCCESS;
    }
    BlockEntry++;
  }

  // If we have reached the end of the TranslationTable and we have not found the end of the region then
  // we return EFI_NOT_FOUND.
  // The caller will continue to look for the memory region at its level
  return EFI_NOT_FOUND;
}

EFI_STATUS
GetMemoryRegion (
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  EFI_STATUS  Status;
  UINT64     *TranslationTable;
  UINTN       TableLevel;
  UINTN       EntryCount;
  UINTN       T0SZ;

  ASSERT ((BaseAddress != NULL) && (RegionLength != NULL) && (RegionAttributes != NULL));

  TranslationTable = ArmGetTTBR0BaseAddress ();

  T0SZ = ArmGetTCR () & TCR_T0SZ_MASK;
  // Get the Table info from T0SZ
  GetRootTranslationTableInfo (T0SZ, &TableLevel, &EntryCount);

  Status = GetMemoryRegionRec (TranslationTable, TableLevel,
      (UINTN*)TT_LAST_BLOCK_ADDRESS(TranslationTable, EntryCount),
      BaseAddress, RegionLength, RegionAttributes);

  // If the region continues up to the end of the root table then GetMemoryRegionRec()
  // will return EFI_NOT_FOUND
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  } else {
    return Status;
  }
}
