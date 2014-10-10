/** @file
*  File managing the MMU for ARMv8 architecture
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Uefi.h>
#include <Chipset/AArch64.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "AArch64Lib.h"
#include "ArmLibPrivate.h"

// We use this index definition to define an invalid block entry
#define TT_ATTR_INDX_INVALID    ((UINT32)~0)

STATIC
UINT64
ArmMemoryAttributeToPageAttribute (
  IN ARM_MEMORY_REGION_ATTRIBUTES  Attributes
  )
{
  switch (Attributes) {
  case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
    return TT_ATTR_INDX_MEMORY_WRITE_BACK;
  case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
    return TT_ATTR_INDX_MEMORY_WRITE_THROUGH;
  case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
    return TT_ATTR_INDX_DEVICE_MEMORY;
  case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
    return TT_ATTR_INDX_MEMORY_NON_CACHEABLE;
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK:
    return TT_ATTR_INDX_MEMORY_WRITE_BACK;
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH:
    return TT_ATTR_INDX_MEMORY_WRITE_THROUGH;
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE:
    return TT_ATTR_INDX_DEVICE_MEMORY;
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED:
    return TT_ATTR_INDX_MEMORY_NON_CACHEABLE;
  default:
    ASSERT(0);
    return TT_ATTR_INDX_DEVICE_MEMORY;
  }
}

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
    DEBUG ((EFI_D_ERROR, "PageAttributeToGcdAttribute: PageAttributes:0x%lX not supported.\n", PageAttributes));
    ASSERT (0);
    // The Global Coherency Domain (GCD) value is defined as a bit set.
    // Returning 0 means no attribute has been set.
    GcdAttributes = 0;
  }

  // Determine protection attributes
  if (((PageAttributes & TT_AP_MASK) == TT_AP_NO_RO) || ((PageAttributes & TT_AP_MASK) == TT_AP_RO_RO)) {
    // Read only cases map to write-protect
    GcdAttributes |= EFI_MEMORY_WP;
  }

  // Process eXecute Never attribute
  if ((PageAttributes & (TT_PXN_MASK | TT_UXN_MASK)) != 0 ) {
    GcdAttributes |= EFI_MEMORY_XP;
  }

  return GcdAttributes;
}

UINT64
GcdAttributeToPageAttribute (
  IN UINT64 GcdAttributes
  )
{
  UINT64  PageAttributes;

  switch (GcdAttributes & 0xFF) {
  case EFI_MEMORY_UC:
    PageAttributes = TT_ATTR_INDX_DEVICE_MEMORY;
    break;
  case EFI_MEMORY_WC:
    PageAttributes = TT_ATTR_INDX_MEMORY_NON_CACHEABLE;
    break;
  case EFI_MEMORY_WT:
    PageAttributes = TT_ATTR_INDX_MEMORY_WRITE_THROUGH;
    break;
  case EFI_MEMORY_WB:
    PageAttributes = TT_ATTR_INDX_MEMORY_WRITE_BACK;
    break;
  default:
    DEBUG ((EFI_D_ERROR, "GcdAttributeToPageAttribute: 0x%X attributes is not supported.\n", GcdAttributes));
    ASSERT (0);
    // If no match has been found then we mark the memory as device memory.
    // The only side effect of using device memory should be a slow down in the performance.
    PageAttributes = TT_ATTR_INDX_DEVICE_MEMORY;
  }

  // Determine protection attributes
  if (GcdAttributes & EFI_MEMORY_WP) {
    // Read only cases map to write-protect
    PageAttributes |= TT_AP_RO_RO;
  }

  // Process eXecute Never attribute
  if (GcdAttributes & EFI_MEMORY_XP) {
    PageAttributes |= (TT_PXN_MASK | TT_UXN_MASK);
  }

  return PageAttributes;
}

ARM_MEMORY_REGION_ATTRIBUTES
GcdAttributeToArmAttribute (
  IN UINT64 GcdAttributes
  )
{
  switch (GcdAttributes & 0xFF) {
  case EFI_MEMORY_UC:
    return ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  case EFI_MEMORY_WC:
    return ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  case EFI_MEMORY_WT:
    return ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH;
  case EFI_MEMORY_WB:
    return ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  default:
    DEBUG ((EFI_D_ERROR, "GcdAttributeToArmAttribute: 0x%lX attributes is not supported.\n", GcdAttributes));
    ASSERT (0);
    return ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  }
}

// Describe the T0SZ values for each translation table level
typedef struct {
  UINTN     MinT0SZ;
  UINTN     MaxT0SZ;
  UINTN     LargestT0SZ; // Generally (MaxT0SZ == LargestT0SZ) but at the Level3 Table
                         // the MaxT0SZ is not at the boundary of the table
} T0SZ_DESCRIPTION_PER_LEVEL;

// Map table for the corresponding Level of Table
STATIC CONST T0SZ_DESCRIPTION_PER_LEVEL T0SZPerTableLevel[] = {
    { 16, 24, 24 }, // Table Level 0
    { 25, 33, 33 }, // Table Level 1
    { 34, 39, 42 }  // Table Level 2
};

VOID
GetRootTranslationTableInfo (
  IN UINTN     T0SZ,
  OUT UINTN   *TableLevel,
  OUT UINTN   *TableEntryCount
  )
{
  UINTN Index;

  // Identify the level of the root table from the given T0SZ
  for (Index = 0; Index < sizeof (T0SZPerTableLevel) / sizeof (T0SZ_DESCRIPTION_PER_LEVEL); Index++) {
    if (T0SZ <= T0SZPerTableLevel[Index].MaxT0SZ) {
      break;
    }
  }

  // If we have not found the corresponding maximum T0SZ then we use the last one
  if (Index == sizeof (T0SZPerTableLevel) / sizeof (T0SZ_DESCRIPTION_PER_LEVEL)) {
    Index--;
  }

  // Get the level of the root table
  if (TableLevel) {
    *TableLevel = Index;
  }

  // The Size of the Table is 2^(T0SZ-LargestT0SZ)
  if (TableEntryCount) {
    *TableEntryCount = 1 << (T0SZPerTableLevel[Index].LargestT0SZ - T0SZ + 1);
  }
}

STATIC
VOID
LookupAddresstoRootTable (
  IN  UINT64  MaxAddress,
  OUT UINTN  *T0SZ,
  OUT UINTN  *TableEntryCount
  )
{
  UINTN TopBit;

  // Check the parameters are not NULL
  ASSERT ((T0SZ != NULL) && (TableEntryCount != NULL));

  // Look for the highest bit set in MaxAddress
  for (TopBit = 63; TopBit != 0; TopBit--) {
    if ((1ULL << TopBit) & MaxAddress) {
      // MaxAddress top bit is found
      TopBit = TopBit + 1;
      break;
    }
  }
  ASSERT (TopBit != 0);

  // Calculate T0SZ from the top bit of the MaxAddress
  *T0SZ = 64 - TopBit;

  // Get the Table info from T0SZ
  GetRootTranslationTableInfo (*T0SZ, NULL, TableEntryCount);
}

STATIC
UINT64*
GetBlockEntryListFromAddress (
  IN  UINT64       *RootTable,
  IN  UINT64        RegionStart,
  OUT UINTN        *TableLevel,
  IN OUT UINT64    *BlockEntrySize,
  IN OUT UINT64   **LastBlockEntry
  )
{
  UINTN   RootTableLevel;
  UINTN   RootTableEntryCount;
  UINT64 *TranslationTable;
  UINT64 *BlockEntry;
  UINT64 *SubTableBlockEntry;
  UINT64  BlockEntryAddress;
  UINTN   BaseAddressAlignment;
  UINTN   PageLevel;
  UINTN   Index;
  UINTN   IndexLevel;
  UINTN   T0SZ;
  UINT64  Attributes;
  UINT64  TableAttributes;

  // Initialize variable
  BlockEntry = NULL;

  // Ensure the parameters are valid
  if (!(TableLevel && BlockEntrySize && LastBlockEntry)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return NULL;
  }

  // Ensure the Region is aligned on 4KB boundary
  if ((RegionStart & (SIZE_4KB - 1)) != 0) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return NULL;
  }

  // Ensure the required size is aligned on 4KB boundary
  if ((*BlockEntrySize & (SIZE_4KB - 1)) != 0) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return NULL;
  }

  //
  // Calculate LastBlockEntry from T0SZ - this is the last block entry of the root Translation table
  //
  T0SZ = ArmGetTCR () & TCR_T0SZ_MASK;
  // Get the Table info from T0SZ
  GetRootTranslationTableInfo (T0SZ, &RootTableLevel, &RootTableEntryCount);
  // The last block of the root table depends on the number of entry in this table
  *LastBlockEntry = TT_LAST_BLOCK_ADDRESS(RootTable, RootTableEntryCount);

  // If the start address is 0x0 then we use the size of the region to identify the alignment
  if (RegionStart == 0) {
    // Identify the highest possible alignment for the Region Size
    for (BaseAddressAlignment = 0; BaseAddressAlignment < 64; BaseAddressAlignment++) {
      if ((1 << BaseAddressAlignment) & *BlockEntrySize) {
        break;
      }
    }
  } else {
    // Identify the highest possible alignment for the Base Address
    for (BaseAddressAlignment = 0; BaseAddressAlignment < 64; BaseAddressAlignment++) {
      if ((1 << BaseAddressAlignment) & RegionStart) {
        break;
      }
    }
  }

  // Identify the Page Level the RegionStart must belongs to
  PageLevel = 3 - ((BaseAddressAlignment - 12) / 9);

  // If the required size is smaller than the current block size then we need to go to the page below.
  // The PageLevel was calculated on the Base Address alignment but did not take in account the alignment
  // of the allocation size
  if (*BlockEntrySize < TT_BLOCK_ENTRY_SIZE_AT_LEVEL (PageLevel)) {
    // It does not fit so we need to go a page level above
    PageLevel++;
  }

  // Expose the found PageLevel to the caller
  *TableLevel = PageLevel;

  // Now, we have the Table Level we can get the Block Size associated to this table
  *BlockEntrySize = TT_BLOCK_ENTRY_SIZE_AT_LEVEL (PageLevel);

  //
  // Get the Table Descriptor for the corresponding PageLevel. We need to decompose RegionStart to get appropriate entries
  //

  TranslationTable = RootTable;
  for (IndexLevel = RootTableLevel; IndexLevel <= PageLevel; IndexLevel++) {
    BlockEntry = (UINT64*)TT_GET_ENTRY_FOR_ADDRESS (TranslationTable, IndexLevel, RegionStart);

    if ((IndexLevel != 3) && ((*BlockEntry & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY)) {
      // Go to the next table
      TranslationTable = (UINT64*)(*BlockEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE);

      // If we are at the last level then update the output
      if (IndexLevel == PageLevel) {
        // And get the appropriate BlockEntry at the next level
        BlockEntry = (UINT64*)TT_GET_ENTRY_FOR_ADDRESS (TranslationTable, IndexLevel + 1, RegionStart);

        // Set the last block for this new table
        *LastBlockEntry = TT_LAST_BLOCK_ADDRESS(TranslationTable, TT_ENTRY_COUNT);
      }
    } else if ((*BlockEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY) {
      // If we are not at the last level then we need to split this BlockEntry
      if (IndexLevel != PageLevel) {
        // Retrieve the attributes from the block entry
        Attributes = *BlockEntry & TT_ATTRIBUTES_MASK;

        // Convert the block entry attributes into Table descriptor attributes
        TableAttributes = TT_TABLE_AP_NO_PERMISSION;
        if (Attributes & TT_PXN_MASK) {
          TableAttributes = TT_TABLE_PXN;
        }
        if (Attributes & TT_UXN_MASK) {
          TableAttributes = TT_TABLE_XN;
        }
        if (Attributes & TT_NS) {
          TableAttributes = TT_TABLE_NS;
        }

        // Get the address corresponding at this entry
        BlockEntryAddress = RegionStart;
        BlockEntryAddress = BlockEntryAddress >> TT_ADDRESS_OFFSET_AT_LEVEL(IndexLevel);
        // Shift back to right to set zero before the effective address
        BlockEntryAddress = BlockEntryAddress << TT_ADDRESS_OFFSET_AT_LEVEL(IndexLevel);

        // Set the correct entry type for the next page level
        if ((IndexLevel + 1) == 3) {
          Attributes |= TT_TYPE_BLOCK_ENTRY_LEVEL3;
        } else {
          Attributes |= TT_TYPE_BLOCK_ENTRY;
        }

        // Create a new translation table
        TranslationTable = (UINT64*)AllocatePages (EFI_SIZE_TO_PAGES((TT_ENTRY_COUNT * sizeof(UINT64)) + TT_ALIGNMENT_DESCRIPTION_TABLE));
        if (TranslationTable == NULL) {
          return NULL;
        }
        TranslationTable = (UINT64*)((UINTN)TranslationTable & TT_ADDRESS_MASK_DESCRIPTION_TABLE);

        // Populate the newly created lower level table
        SubTableBlockEntry = TranslationTable;
        for (Index = 0; Index < TT_ENTRY_COUNT; Index++) {
          *SubTableBlockEntry = Attributes | (BlockEntryAddress + (Index << TT_ADDRESS_OFFSET_AT_LEVEL(IndexLevel + 1)));
          SubTableBlockEntry++;
        }

        // Fill the BlockEntry with the new TranslationTable
        *BlockEntry = ((UINTN)TranslationTable & TT_ADDRESS_MASK_DESCRIPTION_TABLE) | TableAttributes | TT_TYPE_TABLE_ENTRY;
        // Update the last block entry with the newly created translation table
        *LastBlockEntry = TT_LAST_BLOCK_ADDRESS(TranslationTable, TT_ENTRY_COUNT);

        // Block Entry points at the beginning of the Translation Table
        BlockEntry = TranslationTable;
      }
    } else {
      if (IndexLevel != PageLevel) {
        //
        // Case when we have an Invalid Entry and we are at a page level above of the one targetted.
        //

        // Create a new translation table
        TranslationTable = (UINT64*)AllocatePages (EFI_SIZE_TO_PAGES((TT_ENTRY_COUNT * sizeof(UINT64)) + TT_ALIGNMENT_DESCRIPTION_TABLE));
        if (TranslationTable == NULL) {
          return NULL;
        }
        TranslationTable = (UINT64*)((UINTN)TranslationTable & TT_ADDRESS_MASK_DESCRIPTION_TABLE);

        ZeroMem (TranslationTable, TT_ENTRY_COUNT * sizeof(UINT64));

        // Fill the new BlockEntry with the TranslationTable
        *BlockEntry = ((UINTN)TranslationTable & TT_ADDRESS_MASK_DESCRIPTION_TABLE) | TT_TYPE_TABLE_ENTRY;
        // Update the last block entry with the newly created translation table
        *LastBlockEntry = TT_LAST_BLOCK_ADDRESS(TranslationTable, TT_ENTRY_COUNT);
      } else {
        //
        // Case when the new region is part of an existing page table
        //
        *LastBlockEntry = TT_LAST_BLOCK_ADDRESS(TranslationTable, TT_ENTRY_COUNT);
      }
    }
  }

  return BlockEntry;
}

STATIC
RETURN_STATUS
FillTranslationTable (
  IN  UINT64                        *RootTable,
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  )
{
  UINT64  Attributes;
  UINT32  Type;
  UINT64  RegionStart;
  UINT64  RemainingRegionLength;
  UINT64 *BlockEntry;
  UINT64 *LastBlockEntry;
  UINT64  BlockEntrySize;
  UINTN   TableLevel;

  // Ensure the Length is aligned on 4KB boundary
  if ((MemoryRegion->Length == 0) || ((MemoryRegion->Length & (SIZE_4KB - 1)) != 0)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return RETURN_INVALID_PARAMETER;
  }

  // Variable initialization
  Attributes = ArmMemoryAttributeToPageAttribute (MemoryRegion->Attributes) | TT_AF;
  RemainingRegionLength = MemoryRegion->Length;
  RegionStart = MemoryRegion->VirtualBase;

  do {
    // Get the first Block Entry that matches the Virtual Address and also the information on the Table Descriptor
    // such as the the size of the Block Entry and the address of the last BlockEntry of the Table Descriptor
    BlockEntrySize = RemainingRegionLength;
    BlockEntry = GetBlockEntryListFromAddress (RootTable, RegionStart, &TableLevel, &BlockEntrySize, &LastBlockEntry);
    if (BlockEntry == NULL) {
      // GetBlockEntryListFromAddress() return NULL when it fails to allocate new pages from the Translation Tables
      return RETURN_OUT_OF_RESOURCES;
    }

    if (TableLevel != 3) {
      Type = TT_TYPE_BLOCK_ENTRY;
    } else {
      Type = TT_TYPE_BLOCK_ENTRY_LEVEL3;
    }

    do {
      // Fill the Block Entry with attribute and output block address
      *BlockEntry = (RegionStart & TT_ADDRESS_MASK_BLOCK_ENTRY) | Attributes | Type;

      // Go to the next BlockEntry
      RegionStart += BlockEntrySize;
      RemainingRegionLength -= BlockEntrySize;
      BlockEntry++;
    } while ((RemainingRegionLength >= BlockEntrySize) && (BlockEntry <= LastBlockEntry));
  } while (RemainingRegionLength != 0);

  return RETURN_SUCCESS;
}

RETURN_STATUS
SetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes,
  IN EFI_PHYSICAL_ADDRESS      VirtualMask
  )
{
  RETURN_STATUS                Status;
  ARM_MEMORY_REGION_DESCRIPTOR MemoryRegion;
  UINT64                      *TranslationTable;

  MemoryRegion.PhysicalBase = BaseAddress;
  MemoryRegion.VirtualBase = BaseAddress;
  MemoryRegion.Length = Length;
  MemoryRegion.Attributes = GcdAttributeToArmAttribute (Attributes);

  TranslationTable = ArmGetTTBR0BaseAddress ();

  Status = FillTranslationTable (TranslationTable, &MemoryRegion);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  // Flush d-cache so descriptors make it back to uncached memory for subsequent table walks
  // flush and invalidate pages
  ArmCleanInvalidateDataCache ();

  ArmInvalidateInstructionCache ();

  // Invalidate all TLB entries so changes are synced
  ArmInvalidateTlb ();

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                         **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize OPTIONAL
  )
{
  VOID*                         TranslationTable;
  UINTN                         TranslationTablePageCount;
  UINT32                        TranslationTableAttribute;
  ARM_MEMORY_REGION_DESCRIPTOR *MemoryTableEntry;
  UINT64                        MaxAddress;
  UINT64                        TopAddress;
  UINTN                         T0SZ;
  UINTN                         RootTableEntryCount;
  UINT64                        TCR;
  RETURN_STATUS                 Status;

  if(MemoryTable == NULL) {
    ASSERT (MemoryTable != NULL);
    return RETURN_INVALID_PARAMETER;
  }

  // Identify the highest address of the memory table
  MaxAddress = MemoryTable->PhysicalBase + MemoryTable->Length - 1;
  MemoryTableEntry = MemoryTable;
  while (MemoryTableEntry->Length != 0) {
    TopAddress = MemoryTableEntry->PhysicalBase + MemoryTableEntry->Length - 1;
    if (TopAddress > MaxAddress) {
      MaxAddress = TopAddress;
    }
    MemoryTableEntry++;
  }

  // Lookup the Table Level to get the information
  LookupAddresstoRootTable (MaxAddress, &T0SZ, &RootTableEntryCount);

  //
  // Set TCR that allows us to retrieve T0SZ in the subsequent functions
  //
  // Ideally we will be running at EL2, but should support EL1 as well.
  // UEFI should not run at EL3.
  if (ArmReadCurrentEL () == AARCH64_EL2) {
    //Note: Bits 23 and 31 are reserved(RES1) bits in TCR_EL2
    TCR = T0SZ | (1UL << 31) | (1UL << 23) | TCR_TG0_4KB;

    // Set the Physical Address Size using MaxAddress
    if (MaxAddress < SIZE_4GB) {
      TCR |= TCR_PS_4GB;
    } else if (MaxAddress < SIZE_64GB) {
      TCR |= TCR_PS_64GB;
    } else if (MaxAddress < SIZE_1TB) {
      TCR |= TCR_PS_1TB;
    } else if (MaxAddress < SIZE_4TB) {
      TCR |= TCR_PS_4TB;
    } else if (MaxAddress < SIZE_16TB) {
      TCR |= TCR_PS_16TB;
    } else if (MaxAddress < SIZE_256TB) {
      TCR |= TCR_PS_256TB;
    } else {
      DEBUG ((EFI_D_ERROR, "ArmConfigureMmu: The MaxAddress 0x%lX is not supported by this MMU configuration.\n", MaxAddress));
      ASSERT (0); // Bigger than 48-bit memory space are not supported
      return RETURN_UNSUPPORTED;
    }
  } else if (ArmReadCurrentEL () == AARCH64_EL1) {
    TCR = T0SZ | TCR_TG0_4KB;

    // Set the Physical Address Size using MaxAddress
    if (MaxAddress < SIZE_4GB) {
      TCR |= TCR_IPS_4GB;
    } else if (MaxAddress < SIZE_64GB) {
      TCR |= TCR_IPS_64GB;
    } else if (MaxAddress < SIZE_1TB) {
      TCR |= TCR_IPS_1TB;
    } else if (MaxAddress < SIZE_4TB) {
      TCR |= TCR_IPS_4TB;
    } else if (MaxAddress < SIZE_16TB) {
      TCR |= TCR_IPS_16TB;
    } else if (MaxAddress < SIZE_256TB) {
      TCR |= TCR_IPS_256TB;
    } else {
      DEBUG ((EFI_D_ERROR, "ArmConfigureMmu: The MaxAddress 0x%lX is not supported by this MMU configuration.\n", MaxAddress));
      ASSERT (0); // Bigger than 48-bit memory space are not supported
      return RETURN_UNSUPPORTED;
    }
  } else {
    ASSERT (0); // UEFI is only expected to run at EL2 and EL1, not EL3.
    return RETURN_UNSUPPORTED;
  }

  // Set TCR
  ArmSetTCR (TCR);

  // Allocate pages for translation table
  TranslationTablePageCount = EFI_SIZE_TO_PAGES((RootTableEntryCount * sizeof(UINT64)) + TT_ALIGNMENT_DESCRIPTION_TABLE);
  TranslationTable = AllocatePages (TranslationTablePageCount);
  if (TranslationTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  TranslationTable = (VOID*)((UINTN)TranslationTable & TT_ADDRESS_MASK_DESCRIPTION_TABLE);
  // We set TTBR0 just after allocating the table to retrieve its location from the subsequent
  // functions without needing to pass this value across the functions. The MMU is only enabled
  // after the translation tables are populated.
  ArmSetTTBR0 (TranslationTable);

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = TranslationTable;
  }

  if (TranslationTableSize != NULL) {
    *TranslationTableSize = RootTableEntryCount * sizeof(UINT64);
  }

  ZeroMem (TranslationTable, RootTableEntryCount * sizeof(UINT64));

  // Disable MMU and caches. ArmDisableMmu() also invalidates the TLBs
  ArmDisableMmu ();
  ArmDisableDataCache ();
  ArmDisableInstructionCache ();

  // Make sure nothing sneaked into the cache
  ArmCleanInvalidateDataCache ();
  ArmInvalidateInstructionCache ();

  TranslationTableAttribute = TT_ATTR_INDX_INVALID;
  while (MemoryTable->Length != 0) {
    // Find the memory attribute for the Translation Table
    if (((UINTN)TranslationTable >= MemoryTable->PhysicalBase) &&
        ((UINTN)TranslationTable <= MemoryTable->PhysicalBase - 1 + MemoryTable->Length)) {
      TranslationTableAttribute = MemoryTable->Attributes;
    }

    Status = FillTranslationTable (TranslationTable, MemoryTable);
    if (RETURN_ERROR (Status)) {
      goto FREE_TRANSLATION_TABLE;
    }
    MemoryTable++;
  }

  // Translate the Memory Attributes into Translation Table Register Attributes
  if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED)) {
    TCR |= TCR_SH_NON_SHAREABLE | TCR_RGN_OUTER_NON_CACHEABLE | TCR_RGN_INNER_NON_CACHEABLE;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK)) {
    TCR |= TCR_SH_INNER_SHAREABLE | TCR_RGN_OUTER_WRITE_BACK_ALLOC | TCR_RGN_INNER_WRITE_BACK_ALLOC;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH)) {
    TCR |= TCR_SH_NON_SHAREABLE | TCR_RGN_OUTER_WRITE_THROUGH | TCR_RGN_INNER_WRITE_THROUGH;
  } else {
    // If we failed to find a mapping that contains the root translation table then it probably means the translation table
    // is not mapped in the given memory map.
    ASSERT (0);
    Status = RETURN_UNSUPPORTED;
    goto FREE_TRANSLATION_TABLE;
  }

  // Set again TCR after getting the Translation Table attributes
  ArmSetTCR (TCR);

  ArmSetMAIR (MAIR_ATTR(TT_ATTR_INDX_DEVICE_MEMORY, MAIR_ATTR_DEVICE_MEMORY) |                      // mapped to EFI_MEMORY_UC
              MAIR_ATTR(TT_ATTR_INDX_MEMORY_NON_CACHEABLE, MAIR_ATTR_NORMAL_MEMORY_NON_CACHEABLE) | // mapped to EFI_MEMORY_WC
              MAIR_ATTR(TT_ATTR_INDX_MEMORY_WRITE_THROUGH, MAIR_ATTR_NORMAL_MEMORY_WRITE_THROUGH) | // mapped to EFI_MEMORY_WT
              MAIR_ATTR(TT_ATTR_INDX_MEMORY_WRITE_BACK, MAIR_ATTR_NORMAL_MEMORY_WRITE_BACK));       // mapped to EFI_MEMORY_WB

  ArmDisableAlignmentCheck ();
  ArmEnableInstructionCache ();
  ArmEnableDataCache ();

  ArmEnableMmu ();
  return RETURN_SUCCESS;

FREE_TRANSLATION_TABLE:
  FreePages (TranslationTable, TranslationTablePageCount);
  return Status;
}
