/** @file
*  File managing the MMU for ARMv8 architecture
*
*  Copyright (c) 2011-2020, ARM Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Chipset/AArch64.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// We use this index definition to define an invalid block entry
#define TT_ATTR_INDX_INVALID    ((UINT32)~0)

STATIC
UINT64
ArmMemoryAttributeToPageAttribute (
  IN ARM_MEMORY_REGION_ATTRIBUTES  Attributes
  )
{
  switch (Attributes) {
  case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_NONSHAREABLE:
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK_NONSHAREABLE:
    return TT_ATTR_INDX_MEMORY_WRITE_BACK;

  case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK:
    return TT_ATTR_INDX_MEMORY_WRITE_BACK | TT_SH_INNER_SHAREABLE;

  case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH:
    return TT_ATTR_INDX_MEMORY_WRITE_THROUGH | TT_SH_INNER_SHAREABLE;

  // Uncached and device mappings are treated as outer shareable by default,
  case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED:
    return TT_ATTR_INDX_MEMORY_NON_CACHEABLE;

  default:
    ASSERT (0);
  case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
  case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE:
    if (ArmReadCurrentEL () == AARCH64_EL2)
      return TT_ATTR_INDX_DEVICE_MEMORY | TT_XN_MASK;
    else
      return TT_ATTR_INDX_DEVICE_MEMORY | TT_UXN_MASK | TT_PXN_MASK;
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

#define MIN_T0SZ        16
#define BITS_PER_LEVEL  9

VOID
GetRootTranslationTableInfo (
  IN UINTN     T0SZ,
  OUT UINTN   *TableLevel,
  OUT UINTN   *TableEntryCount
  )
{
  // Get the level of the root table
  if (TableLevel) {
    *TableLevel = (T0SZ - MIN_T0SZ) / BITS_PER_LEVEL;
  }

  if (TableEntryCount) {
    *TableEntryCount = 1UL << (BITS_PER_LEVEL - (T0SZ - MIN_T0SZ) % BITS_PER_LEVEL);
  }
}

STATIC
VOID
ReplaceTableEntry (
  IN  UINT64  *Entry,
  IN  UINT64  Value,
  IN  UINT64  RegionStart,
  IN  BOOLEAN IsLiveBlockMapping
  )
{
  if (!ArmMmuEnabled () || !IsLiveBlockMapping) {
    *Entry = Value;
    ArmUpdateTranslationTableEntry (Entry, (VOID *)(UINTN)RegionStart);
  } else {
    ArmReplaceLiveTranslationEntry (Entry, Value, RegionStart);
  }
}

STATIC
VOID
FreePageTablesRecursive (
  IN  UINT64  *TranslationTable
  )
{
  UINTN   Index;

  for (Index = 0; Index < TT_ENTRY_COUNT; Index++) {
    if ((TranslationTable[Index] & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY) {
      FreePageTablesRecursive ((VOID *)(UINTN)(TranslationTable[Index] &
                                               TT_ADDRESS_MASK_BLOCK_ENTRY));
    }
  }
  FreePages (TranslationTable, 1);
}

STATIC
EFI_STATUS
UpdateRegionMappingRecursive (
  IN  UINT64      RegionStart,
  IN  UINT64      RegionEnd,
  IN  UINT64      AttributeSetMask,
  IN  UINT64      AttributeClearMask,
  IN  UINT64      *PageTable,
  IN  UINTN       Level
  )
{
  UINTN           BlockShift;
  UINT64          BlockMask;
  UINT64          BlockEnd;
  UINT64          *Entry;
  UINT64          EntryValue;
  VOID            *TranslationTable;
  EFI_STATUS      Status;

  ASSERT (((RegionStart | RegionEnd) & EFI_PAGE_MASK) == 0);

  BlockShift = (Level + 1) * BITS_PER_LEVEL + MIN_T0SZ;
  BlockMask = MAX_UINT64 >> BlockShift;

  DEBUG ((DEBUG_VERBOSE, "%a(%d): %llx - %llx set %lx clr %lx\n", __FUNCTION__,
    Level, RegionStart, RegionEnd, AttributeSetMask, AttributeClearMask));

  for (; RegionStart < RegionEnd; RegionStart = BlockEnd) {
    BlockEnd = MIN (RegionEnd, (RegionStart | BlockMask) + 1);
    Entry = &PageTable[(RegionStart >> (64 - BlockShift)) & (TT_ENTRY_COUNT - 1)];

    //
    // If RegionStart or BlockEnd is not aligned to the block size at this
    // level, we will have to create a table mapping in order to map less
    // than a block, and recurse to create the block or page entries at
    // the next level. No block mappings are allowed at all at level 0,
    // so in that case, we have to recurse unconditionally.
    //
    if (Level == 0 || ((RegionStart | BlockEnd) & BlockMask) != 0) {
      ASSERT (Level < 3);

      if ((*Entry & TT_TYPE_MASK) != TT_TYPE_TABLE_ENTRY) {
        //
        // No table entry exists yet, so we need to allocate a page table
        // for the next level.
        //
        TranslationTable = AllocatePages (1);
        if (TranslationTable == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        if (!ArmMmuEnabled ()) {
          //
          // Make sure we are not inadvertently hitting in the caches
          // when populating the page tables.
          //
          InvalidateDataCacheRange (TranslationTable, EFI_PAGE_SIZE);
        }

        if ((*Entry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY) {
          //
          // We are splitting an existing block entry, so we have to populate
          // the new table with the attributes of the block entry it replaces.
          //
          Status = UpdateRegionMappingRecursive (RegionStart & ~BlockMask,
                     (RegionStart | BlockMask) + 1, *Entry & TT_ATTRIBUTES_MASK,
                     0, TranslationTable, Level + 1);
          if (EFI_ERROR (Status)) {
            //
            // The range we passed to UpdateRegionMappingRecursive () is block
            // aligned, so it is guaranteed that no further pages were allocated
            // by it, and so we only have to free the page we allocated here.
            //
            FreePages (TranslationTable, 1);
            return Status;
          }
        } else {
          ZeroMem (TranslationTable, EFI_PAGE_SIZE);
        }
      } else {
        TranslationTable = (VOID *)(UINTN)(*Entry & TT_ADDRESS_MASK_BLOCK_ENTRY);
      }

      //
      // Recurse to the next level
      //
      Status = UpdateRegionMappingRecursive (RegionStart, BlockEnd,
                 AttributeSetMask, AttributeClearMask, TranslationTable,
                 Level + 1);
      if (EFI_ERROR (Status)) {
        if ((*Entry & TT_TYPE_MASK) != TT_TYPE_TABLE_ENTRY) {
          //
          // We are creating a new table entry, so on failure, we can free all
          // allocations we made recursively, given that the whole subhierarchy
          // has not been wired into the live page tables yet. (This is not
          // possible for existing table entries, since we cannot revert the
          // modifications we made to the subhierarchy it represents.)
          //
          FreePageTablesRecursive (TranslationTable);
        }
        return Status;
      }

      if ((*Entry & TT_TYPE_MASK) != TT_TYPE_TABLE_ENTRY) {
        EntryValue = (UINTN)TranslationTable | TT_TYPE_TABLE_ENTRY;
        ReplaceTableEntry (Entry, EntryValue, RegionStart,
                           (*Entry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY);
      }
    } else {
      EntryValue = (*Entry & AttributeClearMask) | AttributeSetMask;
      EntryValue |= RegionStart;
      EntryValue |= (Level == 3) ? TT_TYPE_BLOCK_ENTRY_LEVEL3
                                 : TT_TYPE_BLOCK_ENTRY;

      ReplaceTableEntry (Entry, EntryValue, RegionStart, FALSE);
    }
  }
  return EFI_SUCCESS;
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
EFI_STATUS
UpdateRegionMapping (
  IN  UINT64  RegionStart,
  IN  UINT64  RegionLength,
  IN  UINT64  AttributeSetMask,
  IN  UINT64  AttributeClearMask
  )
{
  UINTN     RootTableLevel;
  UINTN     T0SZ;

  if (((RegionStart | RegionLength) & EFI_PAGE_MASK)) {
    return EFI_INVALID_PARAMETER;
  }

  T0SZ = ArmGetTCR () & TCR_T0SZ_MASK;
  GetRootTranslationTableInfo (T0SZ, &RootTableLevel, NULL);

  return UpdateRegionMappingRecursive (RegionStart, RegionStart + RegionLength,
           AttributeSetMask, AttributeClearMask, ArmGetTTBR0BaseAddress (),
           RootTableLevel);
}

STATIC
EFI_STATUS
FillTranslationTable (
  IN  UINT64                        *RootTable,
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  )
{
  return UpdateRegionMapping (
           MemoryRegion->VirtualBase,
           MemoryRegion->Length,
           ArmMemoryAttributeToPageAttribute (MemoryRegion->Attributes) | TT_AF,
           0
           );
}

STATIC
UINT64
GcdAttributeToPageAttribute (
  IN UINT64 GcdAttributes
  )
{
  UINT64 PageAttributes;

  switch (GcdAttributes & EFI_MEMORY_CACHETYPE_MASK) {
  case EFI_MEMORY_UC:
    PageAttributes = TT_ATTR_INDX_DEVICE_MEMORY;
    break;
  case EFI_MEMORY_WC:
    PageAttributes = TT_ATTR_INDX_MEMORY_NON_CACHEABLE;
    break;
  case EFI_MEMORY_WT:
    PageAttributes = TT_ATTR_INDX_MEMORY_WRITE_THROUGH | TT_SH_INNER_SHAREABLE;
    break;
  case EFI_MEMORY_WB:
    PageAttributes = TT_ATTR_INDX_MEMORY_WRITE_BACK | TT_SH_INNER_SHAREABLE;
    break;
  default:
    PageAttributes = TT_ATTR_INDX_MASK;
    break;
  }

  if ((GcdAttributes & EFI_MEMORY_XP) != 0 ||
      (GcdAttributes & EFI_MEMORY_CACHETYPE_MASK) == EFI_MEMORY_UC) {
    if (ArmReadCurrentEL () == AARCH64_EL2) {
      PageAttributes |= TT_XN_MASK;
    } else {
      PageAttributes |= TT_UXN_MASK | TT_PXN_MASK;
    }
  }

  if ((GcdAttributes & EFI_MEMORY_RO) != 0) {
    PageAttributes |= TT_AP_RO_RO;
  }

  return PageAttributes | TT_AF;
}

EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  UINT64                       PageAttributes;
  UINT64                       PageAttributeMask;

  PageAttributes = GcdAttributeToPageAttribute (Attributes);
  PageAttributeMask = 0;

  if ((Attributes & EFI_MEMORY_CACHETYPE_MASK) == 0) {
    //
    // No memory type was set in Attributes, so we are going to update the
    // permissions only.
    //
    PageAttributes &= TT_AP_MASK | TT_UXN_MASK | TT_PXN_MASK;
    PageAttributeMask = ~(TT_ADDRESS_MASK_BLOCK_ENTRY | TT_AP_MASK |
                          TT_PXN_MASK | TT_XN_MASK);
  }

  return UpdateRegionMapping (BaseAddress, Length, PageAttributes,
           PageAttributeMask);
}

STATIC
EFI_STATUS
SetMemoryRegionAttribute (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINT64                    Attributes,
  IN  UINT64                    BlockEntryMask
  )
{
  return UpdateRegionMapping (BaseAddress, Length, Attributes, BlockEntryMask);
}

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  UINT64    Val;

  if (ArmReadCurrentEL () == AARCH64_EL1) {
    Val = TT_PXN_MASK | TT_UXN_MASK;
  } else {
    Val = TT_XN_MASK;
  }

  return SetMemoryRegionAttribute (
           BaseAddress,
           Length,
           Val,
           ~TT_ADDRESS_MASK_BLOCK_ENTRY);
}

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  UINT64 Mask;

  // XN maps to UXN in the EL1&0 translation regime
  Mask = ~(TT_ADDRESS_MASK_BLOCK_ENTRY | TT_PXN_MASK | TT_XN_MASK);

  return SetMemoryRegionAttribute (
           BaseAddress,
           Length,
           0,
           Mask);
}

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return SetMemoryRegionAttribute (
           BaseAddress,
           Length,
           TT_AP_RO_RO,
           ~TT_ADDRESS_MASK_BLOCK_ENTRY);
}

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return SetMemoryRegionAttribute (
           BaseAddress,
           Length,
           TT_AP_RW_RW,
           ~(TT_ADDRESS_MASK_BLOCK_ENTRY | TT_AP_MASK));
}

EFI_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                         **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize OPTIONAL
  )
{
  VOID*                         TranslationTable;
  UINT64                        MaxAddress;
  UINTN                         T0SZ;
  UINTN                         RootTableEntryCount;
  UINT64                        TCR;
  EFI_STATUS                    Status;

  if (MemoryTable == NULL) {
    ASSERT (MemoryTable != NULL);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Limit the virtual address space to what we can actually use: UEFI
  // mandates a 1:1 mapping, so no point in making the virtual address
  // space larger than the physical address space. We also have to take
  // into account the architectural limitations that result from UEFI's
  // use of 4 KB pages.
  //
  MaxAddress = MIN (LShiftU64 (1ULL, ArmGetPhysicalAddressBits ()) - 1,
                    MAX_ALLOC_ADDRESS);

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
      DEBUG ((DEBUG_ERROR,
        "ArmConfigureMmu: The MaxAddress 0x%lX is not supported by this MMU configuration.\n",
        MaxAddress));
      ASSERT (0); // Bigger than 48-bit memory space are not supported
      return EFI_UNSUPPORTED;
    }
  } else if (ArmReadCurrentEL () == AARCH64_EL1) {
    // Due to Cortex-A57 erratum #822227 we must set TG1[1] == 1, regardless of EPD1.
    TCR = T0SZ | TCR_TG0_4KB | TCR_TG1_4KB | TCR_EPD1;

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
      DEBUG ((DEBUG_ERROR,
        "ArmConfigureMmu: The MaxAddress 0x%lX is not supported by this MMU configuration.\n",
        MaxAddress));
      ASSERT (0); // Bigger than 48-bit memory space are not supported
      return EFI_UNSUPPORTED;
    }
  } else {
    ASSERT (0); // UEFI is only expected to run at EL2 and EL1, not EL3.
    return EFI_UNSUPPORTED;
  }

  //
  // Translation table walks are always cache coherent on ARMv8-A, so cache
  // maintenance on page tables is never needed. Since there is a risk of
  // loss of coherency when using mismatched attributes, and given that memory
  // is mapped cacheable except for extraordinary cases (such as non-coherent
  // DMA), have the page table walker perform cached accesses as well, and
  // assert below that that matches the attributes we use for CPU accesses to
  // the region.
  //
  TCR |= TCR_SH_INNER_SHAREABLE |
         TCR_RGN_OUTER_WRITE_BACK_ALLOC |
         TCR_RGN_INNER_WRITE_BACK_ALLOC;

  // Set TCR
  ArmSetTCR (TCR);

  // Allocate pages for translation table
  TranslationTable = AllocatePages (1);
  if (TranslationTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // We set TTBR0 just after allocating the table to retrieve its location from
  // the subsequent functions without needing to pass this value across the
  // functions. The MMU is only enabled after the translation tables are
  // populated.
  //
  ArmSetTTBR0 (TranslationTable);

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = TranslationTable;
  }

  if (TranslationTableSize != NULL) {
    *TranslationTableSize = RootTableEntryCount * sizeof (UINT64);
  }

  //
  // Make sure we are not inadvertently hitting in the caches
  // when populating the page tables.
  //
  InvalidateDataCacheRange (TranslationTable,
    RootTableEntryCount * sizeof (UINT64));
  ZeroMem (TranslationTable, RootTableEntryCount * sizeof (UINT64));

  while (MemoryTable->Length != 0) {
    Status = FillTranslationTable (TranslationTable, MemoryTable);
    if (EFI_ERROR (Status)) {
      goto FreeTranslationTable;
    }
    MemoryTable++;
  }

  //
  // EFI_MEMORY_UC ==> MAIR_ATTR_DEVICE_MEMORY
  // EFI_MEMORY_WC ==> MAIR_ATTR_NORMAL_MEMORY_NON_CACHEABLE
  // EFI_MEMORY_WT ==> MAIR_ATTR_NORMAL_MEMORY_WRITE_THROUGH
  // EFI_MEMORY_WB ==> MAIR_ATTR_NORMAL_MEMORY_WRITE_BACK
  //
  ArmSetMAIR (
    MAIR_ATTR (TT_ATTR_INDX_DEVICE_MEMORY,        MAIR_ATTR_DEVICE_MEMORY)               |
    MAIR_ATTR (TT_ATTR_INDX_MEMORY_NON_CACHEABLE, MAIR_ATTR_NORMAL_MEMORY_NON_CACHEABLE) |
    MAIR_ATTR (TT_ATTR_INDX_MEMORY_WRITE_THROUGH, MAIR_ATTR_NORMAL_MEMORY_WRITE_THROUGH) |
    MAIR_ATTR (TT_ATTR_INDX_MEMORY_WRITE_BACK,    MAIR_ATTR_NORMAL_MEMORY_WRITE_BACK)
    );

  ArmDisableAlignmentCheck ();
  ArmEnableStackAlignmentCheck ();
  ArmEnableInstructionCache ();
  ArmEnableDataCache ();

  ArmEnableMmu ();
  return EFI_SUCCESS;

FreeTranslationTable:
  FreePages (TranslationTable, 1);
  return Status;
}

RETURN_STATUS
EFIAPI
ArmMmuBaseLibConstructor (
  VOID
  )
{
  extern UINT32 ArmReplaceLiveTranslationEntrySize;

  //
  // The ArmReplaceLiveTranslationEntry () helper function may be invoked
  // with the MMU off so we have to ensure that it gets cleaned to the PoC
  //
  WriteBackDataCacheRange (ArmReplaceLiveTranslationEntry,
    ArmReplaceLiveTranslationEntrySize);

  return RETURN_SUCCESS;
}
