/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

#define INVALID_ENTRY  ((UINT64)~0)

#define MIN_T0SZ        16
#define BITS_PER_LEVEL  9

/**
  Parses T0SZ to determine the level and number of entries at the root
  of the translation table.

  @param T0SZ                 The T0SZ value to be parsed.
  @param RootTableLevel       The level of the root table.
  @param RootTableEntryCount  The number of entries in the root table.
**/
STATIC
VOID
GetRootTranslationTableInfo (
  IN  UINTN  T0SZ,
  OUT UINTN  *RootTableLevel,
  OUT UINTN  *RootTableEntryCount
  )
{
  *RootTableLevel      = (T0SZ - MIN_T0SZ) / BITS_PER_LEVEL;
  *RootTableEntryCount = TT_ENTRY_COUNT >> (T0SZ - MIN_T0SZ) % BITS_PER_LEVEL;
}

/**
  Converts ARM translation table attributes to GCD attributes.

  @param PageAttributes The translation table attributes to be converted.

  @retval The analogous GCD attributes.
**/
STATIC
UINT64
PageAttributeToGcdAttribute (
  IN UINT64  PageAttributes
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
      DEBUG ((
        DEBUG_ERROR,
        "PageAttributeToGcdAttribute: PageAttributes:0x%lX not supported.\n",
        PageAttributes
        ));
      ASSERT (0);
      // The Global Coherency Domain (GCD) value is defined as a bit set.
      // Returning 0 means no attribute has been set.
      GcdAttributes = 0;
  }

  // Determine protection attributes
  if ((PageAttributes & TT_AF) == 0) {
    GcdAttributes |= EFI_MEMORY_RP;
  }

  if (((PageAttributes & TT_AP_MASK) == TT_AP_NO_RO) ||
      ((PageAttributes & TT_AP_MASK) == TT_AP_RO_RO))
  {
    // Read only cases map to write-protect
    GcdAttributes |= EFI_MEMORY_RO;
  }

  // Process eXecute Never attribute
  if ((PageAttributes & (TT_PXN_MASK | TT_UXN_MASK)) != 0) {
    GcdAttributes |= EFI_MEMORY_XP;
  }

  return GcdAttributes;
}

/**
  Convert an arch specific set of page attributes into a mask
  of EFI_MEMORY_xx constants.

  @param  PageAttributes  The set of page attributes.

  @retval The mask of EFI_MEMORY_xx constants.

**/
UINT64
RegionAttributeToGcdAttribute (
  IN UINTN  PageAttributes
  )
{
  return PageAttributeToGcdAttribute (PageAttributes);
}

/**
  Retrieves the attribute of the first page entry in the translation table.

  @param[in] FirstLevelTableAddress   The base address of the translation table.
  @param[in] TableLevel               The current level being traversed.

  @retval The attributes of the first page entry found, or INVALID_ENTRY.
**/
STATIC
UINT64
GetFirstPageAttribute (
  IN UINT64  *FirstLevelTableAddress,
  IN UINTN   TableLevel
  )
{
  UINT64  FirstEntry;

  // Get the first entry of the table
  FirstEntry = *FirstLevelTableAddress;

  if ((TableLevel != 3) && ((FirstEntry & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY)) {
    // Only valid for Levels 0, 1 and 2

    // Get the attribute of the subsequent table
    return GetFirstPageAttribute ((UINT64 *)(FirstEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE), TableLevel + 1);
  } else if (((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY) ||
             ((TableLevel == 3) && ((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3)))
  {
    return FirstEntry & TT_ATTRIBUTES_MASK;
  } else {
    return INVALID_ENTRY;
  }
}

/**
  This function recursively traverses the translation table heirarchy to
  synchronise the GCD with the translation table.

  @param[in]        TableAddress        The address of the table being processed.
  @param[in]        EntryCount          The number of entries in the current level of the table.
  @param[in]        TableLevel          The current level of the memory table being processed.
  @param[in]        BaseAddress         The starting address of the region.
  @param[in, out]   PrevEntryAttribute  The attributes of the previous region.
  @param[in, out]   StartGcdRegion      The start of the GCD region.

  @retval The address at the end of the last region processed.
**/
STATIC
UINT64
GetNextEntryAttribute (
  IN     UINT64  *TableAddress,
  IN     UINTN   EntryCount,
  IN     UINTN   TableLevel,
  IN     UINT64  BaseAddress,
  IN OUT UINT64  *PrevEntryAttribute,
  IN OUT UINT64  *StartGcdRegion
  )
{
  UINTN                            Index;
  UINT64                           Entry;
  UINT64                           EntryAttribute;
  UINT64                           EntryType;
  EFI_STATUS                       Status;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;

  // Get the memory space map from GCD
  MemorySpaceMap = NULL;
  Status         = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status) || (TableLevel > 3)) {
    ASSERT_EFI_ERROR (Status);
    ASSERT (TableLevel <= 3);
    return 0;
  }

  // While the top level table might not contain TT_ENTRY_COUNT entries;
  // the subsequent ones should be filled up
  for (Index = 0; Index < EntryCount; Index++) {
    Entry          = TableAddress[Index];
    EntryType      = Entry & TT_TYPE_MASK;
    EntryAttribute = Entry & TT_ATTRIBUTES_MASK;

    // If Entry is a Table Descriptor type entry then go through the sub-level table
    if ((EntryType == TT_TYPE_BLOCK_ENTRY) ||
        ((TableLevel == 3) && (EntryType == TT_TYPE_BLOCK_ENTRY_LEVEL3)))
    {
      if ((*PrevEntryAttribute == INVALID_ENTRY) || (EntryAttribute != *PrevEntryAttribute)) {
        if (*PrevEntryAttribute != INVALID_ENTRY) {
          // Update GCD with the last region
          SetGcdMemorySpaceAttributes (
            MemorySpaceMap,
            NumberOfDescriptors,
            *StartGcdRegion,
            (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL (TableLevel))) - *StartGcdRegion,
            PageAttributeToGcdAttribute (*PrevEntryAttribute)
            );
        }

        // Start of the new region
        *StartGcdRegion     = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL (TableLevel));
        *PrevEntryAttribute = EntryAttribute;
      } else {
        continue;
      }
    } else if (EntryType == TT_TYPE_TABLE_ENTRY) {
      // Table Entry type is only valid for Level 0, 1, 2
      ASSERT (TableLevel < 3);

      // Increase the level number and scan the sub-level table
      GetNextEntryAttribute (
        (UINT64 *)(Entry & TT_ADDRESS_MASK_DESCRIPTION_TABLE),
        TT_ENTRY_COUNT,
        TableLevel + 1,
        (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL (TableLevel))),
        PrevEntryAttribute,
        StartGcdRegion
        );
    } else {
      if (*PrevEntryAttribute != INVALID_ENTRY) {
        // Update GCD with the last region
        SetGcdMemorySpaceAttributes (
          MemorySpaceMap,
          NumberOfDescriptors,
          *StartGcdRegion,
          (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL (TableLevel))) - *StartGcdRegion,
          PageAttributeToGcdAttribute (*PrevEntryAttribute)
          );

        // Start of the new region
        *StartGcdRegion     = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL (TableLevel));
        *PrevEntryAttribute = INVALID_ENTRY;
      }
    }
  }

  FreePool (MemorySpaceMap);

  return BaseAddress + (EntryCount * TT_ADDRESS_AT_LEVEL (TableLevel));
}

/**
  Sync the GCD memory space attributes with the translation table.

  @param[in]  CpuProtocol The CPU architectural protocol instance.

  @retval EFI_SUCCESS The GCD memory space attributes are synced with
                      the MMU page table.
  @retval Others      The return value of GetMemorySpaceMap().
**/
EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL  *CpuProtocol
  )
{
  EFI_STATUS                       Status;
  UINT64                           PageAttribute;
  UINT64                           *FirstLevelTableAddress;
  UINTN                            TableLevel;
  UINTN                            TableCount;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  UINTN                            Tcr;
  UINTN                            T0SZ;
  UINT64                           BaseAddressGcdRegion;
  UINT64                           EndAddressGcdRegion;

  // This code assumes MMU is enabled and filed with section translations
  ASSERT (ArmMmuEnabled ());

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status         = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // The GCD implementation maintains its own copy of the state of memory space attributes.  GCD needs
  // to know what the initial memory space attributes are.  The CPU Arch. Protocol does not provide a
  // GetMemoryAttributes function for GCD to get this so we must resort to calling GCD (as if we were
  // a client) to update its copy of the attributes.  This is bad architecture and should be replaced
  // with a way for GCD to query the CPU Arch. driver of the existing memory space attributes instead.

  // Obtain page table base
  FirstLevelTableAddress = (UINT64 *)(ArmGetTTBR0BaseAddress ());

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
  EndAddressGcdRegion  = GetNextEntryAttribute (
                           FirstLevelTableAddress,
                           TableCount,
                           TableLevel,
                           BaseAddressGcdRegion,
                           &PageAttribute,
                           &BaseAddressGcdRegion
                           );

  // Update GCD with the last region if valid
  if ((PageAttribute != INVALID_ENTRY) && (EndAddressGcdRegion > BaseAddressGcdRegion)) {
    SetGcdMemorySpaceAttributes (
      MemorySpaceMap,
      NumberOfDescriptors,
      BaseAddressGcdRegion,
      EndAddressGcdRegion - BaseAddressGcdRegion,
      PageAttributeToGcdAttribute (PageAttribute)
      );
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
  if ((EfiAttributes & EFI_MEMORY_RP) == 0) {
    ArmAttributes |= TT_AF;
  }

  // Determine protection attributes
  if ((EfiAttributes & EFI_MEMORY_RO) != 0) {
    ArmAttributes |= TT_AP_NO_RO;
  }

  // Process eXecute Never attribute
  if ((EfiAttributes & EFI_MEMORY_XP) != 0) {
    ArmAttributes |= TT_PXN_MASK;
  }

  return ArmAttributes;
}

/**
  This function returns the attributes of the memory region containing the
  specified address.

  RegionLength and RegionAttributes are only valid if the result is EFI_SUCCESS.

  @param[in]        TranslationTable  The translation table base address.
  @param[in]        TableLevel        The level of the translation table.
  @param[in]        LastBlockEntry    The last block address of the table level.
  @param[in, out]   BaseAddress       The base address of the memory region.
  @param[out]       RegionLength      The length of the memory region.
  @param[out]       RegionAttributes  The attributes of the memory region.

  @retval EFI_SUCCESS     The attributes of the memory region were
                          returned successfully.
  @retval EFI_NOT_FOUND   The memory region was not found.
  @retval EFI_NO_MAPPING  The translation table entry associated with
                          BaseAddress is invalid.
**/
EFI_STATUS
GetMemoryRegionRec (
  IN     UINT64  *TranslationTable,
  IN     UINTN   TableLevel,
  IN     UINT64  *LastBlockEntry,
  IN OUT UINTN   *BaseAddress,
  OUT    UINTN   *RegionLength,
  OUT    UINTN   *RegionAttributes
  )
{
  EFI_STATUS  Status;
  UINT64      *NextTranslationTable;
  UINT64      *BlockEntry;
  UINT64      BlockEntryType;
  UINT64      EntryType;

  if (TableLevel != 3) {
    BlockEntryType = TT_TYPE_BLOCK_ENTRY;
  } else {
    BlockEntryType = TT_TYPE_BLOCK_ENTRY_LEVEL3;
  }

  // Find the block entry linked to the Base Address
  BlockEntry = (UINT64 *)TT_GET_ENTRY_FOR_ADDRESS (TranslationTable, TableLevel, *BaseAddress);
  EntryType  = *BlockEntry & TT_TYPE_MASK;

  if ((TableLevel < 3) && (EntryType == TT_TYPE_TABLE_ENTRY)) {
    NextTranslationTable = (UINT64 *)(*BlockEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE);

    // The entry is a page table, so we go to the next level
    Status = GetMemoryRegionRec (
               NextTranslationTable, // Address of the next level page table
               TableLevel + 1,       // Next Page Table level
               (UINTN *)TT_LAST_BLOCK_ADDRESS (NextTranslationTable, TT_ENTRY_COUNT),
               BaseAddress,
               RegionLength,
               RegionAttributes
               );

    // EFI_SUCCESS:     The end of the end of the region was found.
    // EFI_NO_MAPPING:  The translation entry associated with BaseAddress is invalid.
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    // Now we processed the table move to the next entry
    BlockEntry++;
  } else if (EntryType == BlockEntryType) {
    // We have found the BlockEntry attached to the address. We save its start address (the start
    // address might be before the 'BaseAddress') and attributes
    *BaseAddress      = *BaseAddress & ~(TT_ADDRESS_AT_LEVEL (TableLevel) - 1);
    *RegionLength     = 0;
    *RegionAttributes = *BlockEntry & TT_ATTRIBUTES_MASK;
  } else {
    return EFI_NO_MAPPING;
  }

  while (BlockEntry <= LastBlockEntry) {
    if (((*BlockEntry & TT_TYPE_MASK) == BlockEntryType) &&
        ((*BlockEntry & TT_ATTRIBUTES_MASK) == *RegionAttributes))
    {
      *RegionLength = *RegionLength + TT_BLOCK_ENTRY_SIZE_AT_LEVEL (TableLevel);
    } else {
      // In case we have found the end of the region we return success
      return EFI_SUCCESS;
    }

    BlockEntry++;
  }

  // If we have reached the end of the TranslationTable and we have not found the end of the region then
  // we return EFI_NOT_FOUND.
  // The caller will continue to look for the memory region at its level.
  return EFI_NOT_FOUND;
}

/**
  Retrieves a memory region from a given base address.

  This function retrieves a memory region starting from a given base address.

  @param[in, out] BaseAddress       The base address from which to retrieve
                                    the memory region. On successful return, this is
                                    updated to the end address of the retrieved region.
  @param[out]     RegionLength      The length of the retrieved memory region.
  @param[out]     RegionAttributes  The attributes of the retrieved memory region.

  @retval EFI_STATUS              Returns EFI_SUCCESS if the memory region is
                                  retrieved successfully, or the status of the
                                  recursive call to GetMemoryRegionRec.
  @retval EFI_NOT_FOUND           The memory region was not found.
  @retval EFI_NO_MAPPING          The translation table entry associated with
                                  BaseAddress is invalid.
  @retval EFI_INVALID_PARAMETER   One of the input parameters was NULL.
**/
EFI_STATUS
GetMemoryRegion (
  IN OUT UINTN  *BaseAddress,
  OUT    UINTN  *RegionLength,
  OUT    UINTN  *RegionAttributes
  )
{
  EFI_STATUS  Status;
  UINT64      *TranslationTable;
  UINTN       TableLevel;
  UINTN       EntryCount;
  UINTN       T0SZ;

  if ((BaseAddress == NULL) || (RegionLength == NULL) || (RegionAttributes == NULL)) {
    ASSERT ((BaseAddress != NULL) && (RegionLength != NULL) && (RegionAttributes != NULL));
    return EFI_INVALID_PARAMETER;
  }

  TranslationTable = ArmGetTTBR0BaseAddress ();

  // Initialize the output parameters. These paramaters are only valid if the
  // result is EFI_SUCCESS.
  *RegionLength     = 0;
  *RegionAttributes = 0;

  T0SZ = ArmGetTCR () & TCR_T0SZ_MASK;
  // Get the Table info from T0SZ
  GetRootTranslationTableInfo (T0SZ, &TableLevel, &EntryCount);

  Status = GetMemoryRegionRec (
             TranslationTable,
             TableLevel,
             (UINTN *)TT_LAST_BLOCK_ADDRESS (TranslationTable, EntryCount),
             BaseAddress,
             RegionLength,
             RegionAttributes
             );

  // If the region continues up to the end of the root table then GetMemoryRegionRec()
  // will return EFI_NOT_FOUND. Check if the region length was updated.
  if ((Status == EFI_NOT_FOUND) && (*RegionLength > 0)) {
    return EFI_SUCCESS;
  }

  return Status;
}
