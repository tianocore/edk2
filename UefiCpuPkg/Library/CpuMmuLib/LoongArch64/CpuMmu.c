/** @file

  CPU Memory Map Unit Handler Library common functions.

  Copyright (c) 2011-2020, ARM Limited. All rights reserved.
  Copyright (c) 2016, Linaro Limited. All rights reserved.
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuMmuLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DebugSupport.h>
#include <Register/LoongArch64/Csr.h>
#include "TlbInvalid.h"
#include "TlbExceptionHandle.h"
#include "Page.h"

/**
  Check to see if mmu successfully initializes.

  @param  VOID.

  @retval  TRUE  Initialization has been completed.
           FALSE Initialization did not complete.
**/
STATIC
BOOLEAN
MmuIsInit (
  VOID
  )
{
  if (CsrRead (LOONGARCH_CSR_PGDL) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check to see if mmu is enabled.

  @param  VOID.

  @retval  TRUE  MMU has been enabled.
           FALSE MMU did not enabled.
**/
STATIC
BOOLEAN
MmuIsEnabled (
  VOID
  )
{
  if ((CsrRead (LOONGARCH_CSR_CRMD) & BIT4) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Determine if an entry is valid pte.

  @param    Entry   The entry value.

  @retval   TRUE    The entry is a valid pte.
  @retval   FALSE   The entry is not a valid pte.

**/
STATIC
BOOLEAN
IsValidPte (
  IN  UINTN  Entry
  )
{
  if (Entry != INVALID_PAGE) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Determine if an entry is huge page.

  @param    Entry   The entry value.

  @retval   TRUE    The entry is a huge page.
  @retval   FALSE   The entry is not a valid huge page.

**/
STATIC
BOOLEAN
IsValidHugePage (
  IN  UINTN  Entry
  )
{
  if ((Entry & (PAGE_HGLOBAL | PAGE_HUGE)) == (PAGE_HGLOBAL | PAGE_HUGE)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Set an entry to be a valid pte.

  @param  Entry   The entry value.

  @return         The entry value.

**/
STATIC
UINTN
SetValidPte (
  IN  UINTN  Entry
  )
{
  /* Set Valid and Global mapping bits */
  return Entry | PAGE_GLOBAL | PAGE_VALID;
}

/**
  Parse max page table level.

  @param[in]  PageWalkCfg  Page table configure value.

  @return         5   MAX page level is 5
                  4   MAX page level is 4
                  3   MAX page level is 3
                  0   Invalid
**/
STATIC
UINTN
ParseMaxPageTableLevel (
  IN  UINT64  PageWalkCfg
  )
{
  UINT32  Pwctl0;
  UINT32  Pwctl1;

  Pwctl0 = PageWalkCfg & MAX_UINT32;
  Pwctl1 = (PageWalkCfg >> 32) & MAX_UINT32;

  if (((Pwctl1 >> 18) & 0x3F) != 0x0) {
    return LEVEL5;
  } else if (((Pwctl1 >> 6) & 0x3F) != 0x0) {
    return LEVEL4;
  } else if (((Pwctl0 >> 25) & 0x3F) != 0x0) {
    return LEVEL3;
  }

  return 0;
}

/**
  Parse page table bit width.

  Assume that the bit width of the page table that each level is the same to PTwidth.

  @param[in]  PageWalkCfg  Page table configure value.

  @return         page table bit width

**/
STATIC
UINTN
ParsePageTableBitWidth (
  IN  UINT64  PageWalkCfg
  )
{
  //
  // PTwidth
  //
  return ((PageWalkCfg >> 5) & 0x1F);
}

/**
  Determine if an entry is a HUGE PTE or 4K PTE.

  @param    Entry        The entry value.
  @param    Level        The current page table level.
  @param    PageWalkCfg  Page table configure value.

  @retval   TRUE    The entry is a block pte.
  @retval   FALSE   The entry is not a block pte.

**/
STATIC
BOOLEAN
IsBlockEntry (
  IN  UINTN   Entry,
  IN  UINTN   Level,
  IN  UINT64  PageWalkCfg
  )
{
  if (Level == (ParseMaxPageTableLevel (PageWalkCfg) - 1)) {
    return ((Entry & PAGE_VALID) == PAGE_VALID);
  }

  return IsValidHugePage (Entry);
}

/**
  Determine if an entry is a table pte.

  @param    Entry        The entry value.
  @param    Level        The current page table level.
  @param    PageWalkCfg  Page table configure value.

  @retval   TRUE    The entry is a table pte.
  @retval   FALSE   The entry is not a table pte.

**/
STATIC
BOOLEAN
IsTableEntry (
  IN  UINTN   Entry,
  IN  UINTN   Level,
  IN  UINT64  PageWalkCfg
  )
{
  if (Level == (ParseMaxPageTableLevel (PageWalkCfg) - 1)) {
    //
    // The last level is PAGE rather than Table.
    //
    return FALSE;
  }

  //
  // Is DIR4 or DIR3 or DIR2 a Huge Page ?
  //
  return (!IsValidHugePage (Entry)) && (IsValidPte (Entry));
}

/**
  Replace an existing entry with new value.

  @param  Entry               The entry pointer.
  @param  Value               The new entry value.
  @param  RegionStart         The start of region that new value affects.
  @param  IsLiveBlockMapping  TRUE if this is live update, FALSE otherwise.

**/
STATIC
VOID
ReplaceTableEntry (
  IN  UINTN    *Entry,
  IN  UINTN    Value,
  IN  UINTN    RegionStart,
  IN  BOOLEAN  IsLiveBlockMapping
  )
{
  *Entry = Value;

  if (IsLiveBlockMapping && MmuIsInit ()) {
    InvalidTlb (RegionStart);
  }
}

/**
  Get an ppn value from an entry.

  @param  Entry   The entry value.

  @return         The ppn value.

**/
STATIC
UINTN
GetPpnfromPte (
  IN UINTN  Entry
  )
{
  return ((Entry & PTE_PPN_MASK) >> PTE_PPN_SHIFT);
}

/**
  Set an ppn value to a entry.

  @param  Entry   The entry value.
  @param  Address The address.

  @return The new entry value.

**/
STATIC
UINTN
SetPpnToPte (
  UINTN  Entry,
  UINTN  Address
  )
{
  UINTN  Ppn;

  Ppn = ((Address >> LOONGARCH_MMU_PAGE_SHIFT) << PTE_PPN_SHIFT);
  ASSERT (~(Ppn & ~PTE_PPN_MASK));
  Entry &= ~PTE_PPN_MASK;
  return Entry | Ppn;
}

/**
  Free resources of translation table recursively.

  @param  TranslationTable  The pointer of table.
  @param  PageWalkCfg       Page table configure value.
  @param  Level             The current level.

**/
STATIC
VOID
FreePageTablesRecursive (
  IN  UINTN   *TranslationTable,
  IN  UINT64  PageWalkCfg,
  IN  UINTN   Level
  )
{
  UINTN  Index;
  UINTN  TableEntryNum;

  TableEntryNum = (1 << ParsePageTableBitWidth (PageWalkCfg));

  if (Level < (ParseMaxPageTableLevel (PageWalkCfg) - 1)) {
    for (Index = 0; Index < TableEntryNum; Index++) {
      if (IsTableEntry (TranslationTable[Index], Level, PageWalkCfg)) {
        FreePageTablesRecursive (
          (UINTN *)(GetPpnfromPte ((TranslationTable[Index])) <<
                    LOONGARCH_MMU_PAGE_SHIFT),
          PageWalkCfg,
          Level + 1
          );
      }
    }
  }

  FreePages (TranslationTable, 1);
}

/**
  Update region mapping recursively.

  @param  RegionStart           The start address of the region.
  @param  RegionEnd             The end address of the region.
  @param  AttributeSetMask      The attribute mask to be set.
  @param  AttributeClearMask    The attribute mask to be clear.
  @param  PageTable             The pointer of current page table.
  @param  Level                 The current level.
  @param  PageWalkCfg           Page table configure value.
  @param  TableIsLive           TRUE if this is live update, FALSE otherwise.

  @retval EFI_OUT_OF_RESOURCES  Not enough resource.
  @retval EFI_SUCCESS           The operation succesfully.

**/
STATIC
EFI_STATUS
UpdateRegionMappingRecursive (
  IN  UINTN    RegionStart,
  IN  UINTN    RegionEnd,
  IN  UINTN    AttributeSetMask,
  IN  UINTN    AttributeClearMask,
  IN  UINTN    *PageTable,
  IN  UINTN    Level,
  IN  UINT64   PageWalkCfg,
  IN  BOOLEAN  TableIsLive
  )
{
  EFI_STATUS  Status;
  UINTN       BlockShift;
  UINTN       BlockMask;
  UINTN       BlockEnd;
  UINTN       *Entry;
  UINTN       EntryValue;
  UINTN       *TranslationTable;
  UINTN       TableEntryNum;
  UINTN       TableBitWidth;
  BOOLEAN     NextTableIsLive;

  ASSERT (Level < ParseMaxPageTableLevel (PageWalkCfg));
  ASSERT (((RegionStart | RegionEnd) & EFI_PAGE_MASK) == 0);

  TableBitWidth = ParsePageTableBitWidth (PageWalkCfg);
  BlockShift    = (ParseMaxPageTableLevel (PageWalkCfg) - Level - 1) * TableBitWidth + LOONGARCH_MMU_PAGE_SHIFT;
  BlockMask     = MAX_ADDRESS >> (64 - BlockShift);

  DEBUG (
    (
     DEBUG_VERBOSE,
     "%a(%d): %llx - %llx set %lx clr %lx\n",
     __func__,
     Level,
     RegionStart,
     RegionEnd,
     AttributeSetMask,
     AttributeClearMask
    )
    );

  TableEntryNum = (1 << TableBitWidth);
  for ( ; RegionStart < RegionEnd; RegionStart = BlockEnd) {
    BlockEnd = MIN (RegionEnd, (RegionStart | BlockMask) + 1);
    Entry    = &PageTable[(RegionStart >> BlockShift) & (TableEntryNum - 1)];

    //
    // If RegionStart or BlockEnd is not aligned to the block size at this
    // level, we will have to create a table mapping in order to map less
    // than a block, and recurse to create the block or page entries at
    // the next level. No block mappings are allowed at all at level 2,
    // so in that case, we have to recurse unconditionally.
    //
    if ((Level < 2) ||
        (((RegionStart | BlockEnd) & BlockMask) != 0) || IsTableEntry (*Entry, Level, PageWalkCfg))
    {
      ASSERT (Level < (ParseMaxPageTableLevel (PageWalkCfg) - 1));
      if (!IsTableEntry (*Entry, Level, PageWalkCfg)) {
        //
        // No table entry exists yet, so we need to allocate a page table
        // for the next level.
        //
        TranslationTable = AllocatePages (1);
        if (TranslationTable == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        ZeroMem (TranslationTable, EFI_PAGE_SIZE);

        if (IsBlockEntry (*Entry, Level, PageWalkCfg)) {
          //
          // We are splitting an existing block entry, so we have to populate
          // the new table with the attributes of the block entry it replaces.
          //
          Status = UpdateRegionMappingRecursive (
                     RegionStart & ~BlockMask,
                     (RegionStart | BlockMask) + 1,
                     *Entry & PTE_ATTRIBUTES_MASK,
                     PTE_ATTRIBUTES_MASK,
                     TranslationTable,
                     Level + 1,
                     PageWalkCfg,
                     FALSE
                     );
          if (EFI_ERROR (Status)) {
            //
            // The range we passed to UpdateRegionMappingRecursive () is block
            // aligned, so it is guaranteed that no further pages were allocated
            // by it, and so we only have to free the page we allocated here.
            //
            FreePages (TranslationTable, 1);
            return Status;
          }
        }

        NextTableIsLive = FALSE;
      } else {
        TranslationTable = (UINTN *)(GetPpnfromPte (*Entry) << LOONGARCH_MMU_PAGE_SHIFT);
        NextTableIsLive  = TableIsLive;
      }

      //
      // Recurse to the next level
      //
      Status = UpdateRegionMappingRecursive (
                 RegionStart,
                 BlockEnd,
                 AttributeSetMask,
                 AttributeClearMask,
                 TranslationTable,
                 Level + 1,
                 PageWalkCfg,
                 NextTableIsLive
                 );
      if (EFI_ERROR (Status)) {
        if (!IsTableEntry (*Entry, Level, PageWalkCfg)) {
          //
          // We are creating a new table entry, so on failure, we can free all
          // allocations we made recursively, given that the whole subhierarchy
          // has not been wired into the live page tables yet. (This is not
          // possible for existing table entries, since we cannot revert the
          // modifications we made to the subhierarchy it represents.)
          //
          FreePageTablesRecursive (TranslationTable, PageWalkCfg, Level + 1);
        }

        return Status;
      }

      if (!IsTableEntry (*Entry, Level, PageWalkCfg)) {
        EntryValue = SetPpnToPte (0, (UINTN)TranslationTable);
        ReplaceTableEntry (
          Entry,
          EntryValue,
          RegionStart,
          TableIsLive
          );
      }
    } else {
      EntryValue = (*Entry & ~AttributeClearMask) | AttributeSetMask;

      EntryValue = SetPpnToPte (EntryValue, RegionStart);
      EntryValue = SetValidPte (EntryValue);

      if (Level < (ParseMaxPageTableLevel (PageWalkCfg) - 1)) {
        EntryValue |= (PAGE_HGLOBAL | PAGE_HUGE | PAGE_VALID);
      } else {
        EntryValue |= PAGE_GLOBAL | PAGE_VALID;
      }

      ReplaceTableEntry (Entry, EntryValue, RegionStart, TableIsLive);
    }
  }

  return EFI_SUCCESS;
}

/**
  Update region mapping at root table.

  @param  RegionStart           The start address of the region.
  @param  RegionLength          The length of the region.
  @param  PageWalkCfg           Page table configure value.
  @param  AttributeSetMask      The attribute mask to be set.
  @param  AttributeClearMask    The attribute mask to be clear.
  @param  RootTable             The pointer of root table.
  @param  TableIsLive           TRUE if this is live update, FALSE otherwise.

  @retval EFI_INVALID_PARAMETER The RegionStart or RegionLength was not valid.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource.
  @retval EFI_SUCCESS           The operation succesfully.

**/
EFI_STATUS
UpdateRegionMapping (
  IN  UINTN    RegionStart,
  IN  UINTN    RegionLength,
  IN  UINT64   PageWalkCfg,
  IN  UINTN    AttributeSetMask,
  IN  UINTN    AttributeClearMask,
  IN  UINTN    *RootTable,
  IN  BOOLEAN  TableIsLive
  )
{
  if (((RegionStart | RegionLength) & EFI_PAGE_MASK) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return UpdateRegionMappingRecursive (
           RegionStart,
           RegionStart + RegionLength,
           AttributeSetMask,
           AttributeClearMask,
           RootTable,
           0,
           PageWalkCfg,
           TableIsLive
           );
}

/**
  Convert EFI Attributes to Loongarch Attributes.

  @param[in]  EfiAttributes     Efi Attributes.

  @retval  Corresponding architecture attributes.
**/
UINT64
EFIAPI
EfiAttributeConverse (
  IN UINT64  EfiAttributes
  )
{
  UINT64  LoongArchAttributes;

  LoongArchAttributes = PAGE_VALID | PAGE_DIRTY | PLV_KERNEL | PAGE_GLOBAL;

  switch (EfiAttributes & EFI_CACHE_ATTRIBUTE_MASK) {
    case EFI_MEMORY_UC:
      LoongArchAttributes |= CACHE_SUC;
      break;
    case EFI_MEMORY_WC:
      LoongArchAttributes |= CACHE_WUC;
      break;
    case EFI_MEMORY_WT:
    case EFI_MEMORY_WB:
      LoongArchAttributes |= CACHE_CC;
      break;
    case EFI_MEMORY_WP:
      LoongArchAttributes &= ~PAGE_DIRTY;
      break;
    default:
      break;
  }

  // Write protection attributes
  switch (EfiAttributes & EFI_MEMORY_ACCESS_MASK) {
    case EFI_MEMORY_RP:
      LoongArchAttributes |= PAGE_NO_READ;
      break;
    case EFI_MEMORY_XP:
      LoongArchAttributes |= PAGE_NO_EXEC;
      break;
    case EFI_MEMORY_RO:
      LoongArchAttributes &= ~PAGE_DIRTY;
      break;
    default:
      break;
  }

  return LoongArchAttributes;
}

/**
  TLB refill handler configure.

  @param  VOID.

  @retval  EFI_SUCCESS      TLB refill handler configure successfully.
           EFI_UNSUPPORTED  Size not aligned.
**/
EFI_STATUS
TlbRefillHandlerConfigure (
  VOID
  )
{
  UINTN  Length;
  UINTN  TlbReEntry;
  UINTN  TlbReEntryOffset;
  UINTN  Remaining;

  //
  // Set TLB exception handler
  //
  ///
  /// TLB Re-entry address at the end of exception vector, a vector is up to 512 bytes,
  /// so the starting address is: total exception vector size + total interrupt vector size + base.
  /// The total size of TLB handler and exception vector size and interrupt vector size should not
  /// be lager than 64KB.
  ///
  Length           = (UINTN)HandleTlbRefillEnd - (UINTN)HandleTlbRefillStart;
  TlbReEntryOffset = (MAX_LOONGARCH_EXCEPTION + MAX_LOONGARCH_INTERRUPT) * 512;
  Remaining        = TlbReEntryOffset % SIZE_4KB;
  if (Remaining != 0x0) {
    TlbReEntryOffset += (SIZE_4KB - Remaining);
  }

  TlbReEntry = PcdGet64 (PcdLoongArchExceptionVectorBaseAddress) + TlbReEntryOffset;
  if ((TlbReEntryOffset + Length) > SIZE_64KB) {
    return EFI_UNSUPPORTED;
  }

  //
  // Ensure that TLB refill exception base address alignment is equals to 4KB and is valid.
  //
  if (TlbReEntry & (SIZE_4KB - 1)) {
    return EFI_UNSUPPORTED;
  }

  CopyMem ((VOID *)TlbReEntry, HandleTlbRefillStart, Length);
  InvalidateInstructionCacheRange ((VOID *)(UINTN)HandleTlbRefillStart, Length);

  //
  // Set the address of TLB refill exception handler
  //
  SetTlbRebaseAddress ((UINTN)TlbReEntry);

  return EFI_SUCCESS;
}

/**
  Maps the memory region in the page table to the specified attributes.

  @param[in, out] PageTable      The pointer to the page table to update, or pointer to NULL
                                 if a new page table is to be created.
  @param[in]      PageWalkCfg    The page walk controller configure.
  @param[in]      BaseAddress    The base address of the memory region to set the Attributes.
  @param[in]      Length         The length of the memory region to set the Attributes.
  @param[in]      Attributes     The bitmask of attributes to set, which refer to UEFI SPEC
                                 7.2.3(EFI_BOOT_SERVICES.GetMemoryMap()).
  @param[in]      AttributeMask  Mask of memory attributes to take into account.

  @retval EFI_SUCCESS            The Attributes was set successfully or Length is 0.
  @retval EFI_INVALID_PARAMETER  PageTable is NULL, PageWalkCfg is invalid.
  @retval EFI_UNSUPPORTED        *PageTable is NULL.
**/
EFI_STATUS
EFIAPI
MemoryRegionMap (
  IN OUT UINTN                 *PageTable  OPTIONAL,
  IN     UINT64                PageWalkCfg,
  IN     EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN     UINT64                Length,
  IN     UINT64                Attributes,
  IN     UINT64                AttributeMask
  )
{
  EFI_STATUS  Status;
  UINT64      LoongArchAttributes;
  BOOLEAN     Initialization;
  BOOLEAN     CreateNew;
  UINTN       PageTableBitWidth;
  UINTN       MaxLevel;
  UINTN       PgdSize;

  if ((PageTable == NULL) || (PageWalkCfg == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  PageTableBitWidth = ParsePageTableBitWidth (PageWalkCfg);
  MaxLevel          = ParseMaxPageTableLevel (PageWalkCfg);

  if ((!PageTableBitWidth && !MaxLevel) || (PageTableBitWidth > 0x1F) || (MaxLevel > LEVEL5)) {
    return EFI_INVALID_PARAMETER;
  }

  Initialization = FALSE;
  CreateNew      = FALSE;

  //
  // *PageTable is NULL, create a new and return.
  //
  if (*PageTable == 0) {
    CreateNew = TRUE;
    //
    // If the MMU has not been configured yet, configure it later.
    //
    if (!MmuIsInit ()) {
      Initialization = TRUE;
    }
  }

  if (Length == 0) {
    return EFI_SUCCESS;
  }

  if (Initialization == TRUE) {
    Status = TlbRefillHandlerConfigure ();
    ASSERT_EFI_ERROR (Status);
  }

  if (CreateNew == TRUE) {
    //
    // Create a new page table.
    //
    PgdSize    =  (1 << PageTableBitWidth) * sizeof (UINTN);
    *PageTable = (UINTN)AllocatePages (EFI_SIZE_TO_PAGES (PgdSize));
    ZeroMem ((VOID *)*PageTable, PgdSize);

    if ((VOID *)*PageTable == NULL) {
      return EFI_UNSUPPORTED;
    }
  }

  LoongArchAttributes = EfiAttributeConverse (Attributes);

  //
  // Update the page table attributes.
  //
  // If the MMU has been configured and *PageTable == CSR_PGDL, the page table in use will update.
  //
  // If *PageTable != CSR_PGDL, only the page table structure in memory is update, but some TLB
  // region may be invalidated during the mapping process. So at this time the caller must ensure
  // that the execution environment must be safe. It is recommended to use the DA mode!
  //
  Status = UpdateRegionMapping (
             BaseAddress,
             Length,
             PageWalkCfg,
             LoongArchAttributes,
             PTE_ATTRIBUTES_MASK,
             (UINTN *)(*PageTable),
             (MmuIsEnabled () && !CreateNew)
             );

  ASSERT_EFI_ERROR (Status);

  return Status;
}
