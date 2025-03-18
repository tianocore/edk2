/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Arm/AArch32.h>

#define __EFI_MEMORY_RWX  0                 // no restrictions

#define CACHE_ATTRIBUTE_MASK  (EFI_MEMORY_UC |  \
                                EFI_MEMORY_WC | \
                                EFI_MEMORY_WT | \
                                EFI_MEMORY_WB | \
                                EFI_MEMORY_UCE | \
                                EFI_MEMORY_WP)

STATIC
EFI_STATUS
ConvertSectionToPages (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  UINT32  FirstLevelIdx;
  UINT32  SectionDescriptor;
  UINT32  PageTableDescriptor;
  UINT32  PageDescriptor;
  UINT32  Index;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR  *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY        *PageTable;

  DEBUG ((DEBUG_PAGE, "Converting section at 0x%x to pages\n", (UINTN)BaseAddress));

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // Get section attributes and convert to page attributes
  SectionDescriptor = FirstLevelTable[FirstLevelIdx];
  PageDescriptor    = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (SectionDescriptor);

  // Allocate a page table for the 4KB entries (we use up a full page even though we only need 1KB)
  PageTable = (volatile ARM_PAGE_TABLE_ENTRY *)AllocatePages (1);
  if (PageTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Write the page table entries out
  for (Index = 0; Index < TRANSLATION_TABLE_PAGE_COUNT; Index++) {
    PageTable[Index] = TT_DESCRIPTOR_PAGE_BASE_ADDRESS (BaseAddress + (Index << 12)) | PageDescriptor;
  }

  // Formulate page table entry, Domain=0, NS=0
  PageTableDescriptor = (((UINTN)PageTable) & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) | TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;

  // Write the page table entry out, replacing section entry
  FirstLevelTable[FirstLevelIdx] = PageTableDescriptor;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdatePageEntries (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes,
  IN  UINT32                EntryMask,
  OUT BOOLEAN               *FlushTlbs OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      EntryValue;
  UINT32      FirstLevelIdx;
  UINT32      Offset;
  UINT32      NumPageEntries;
  UINT32      Descriptor;
  UINT32      p;
  UINT32      PageTableIndex;
  UINT32      PageTableEntry;
  UINT32      CurrentPageTableEntry;
  VOID        *Mva;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR  *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY        *PageTable;

  Status = EFI_SUCCESS;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask
  EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE;

  // Although the PI spec is unclear on this, the GCD guarantees that only
  // one Attribute bit is set at a time, so the order of the conditionals below
  // is irrelevant. If no memory attribute is specified, we preserve whatever
  // memory type is set in the page tables, and update the permission attributes
  // only.
  if ((Attributes & EFI_MEMORY_UC) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // map to strongly ordered
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
  } else if ((Attributes & EFI_MEMORY_WC) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // map to normal non-cacheable
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
  } else if ((Attributes & EFI_MEMORY_WT) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // write through with no-allocate
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
  } else if ((Attributes & EFI_MEMORY_WB) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // write back (with allocate)
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
  } else if ((Attributes & CACHE_ATTRIBUTE_MASK) != 0) {
    // catch unsupported memory type attributes
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_MEMORY_RP) == 0) {
    EntryValue |= TT_DESCRIPTOR_PAGE_AF;
  }

  if ((Attributes & EFI_MEMORY_RO) != 0) {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RO_RO;
  } else {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RW_RW;
  }

  if ((Attributes & EFI_MEMORY_XP) != 0) {
    EntryValue |= TT_DESCRIPTOR_PAGE_XN_MASK;
  }

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate number of 4KB page table entries to change
  NumPageEntries = (UINT32)(Length / TT_DESCRIPTOR_PAGE_SIZE);

  // Iterate for the number of 4KB pages to change
  Offset = 0;
  for (p = 0; p < NumPageEntries; p++) {
    // Calculate index into first level translation table for page table value

    FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (BaseAddress + Offset) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
    ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

    // Read the descriptor from the first level page table
    Descriptor = FirstLevelTable[FirstLevelIdx];

    // Does this descriptor need to be converted from section entry to 4K pages?
    if (!TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (Descriptor)) {
      //
      // If the section mapping covers the requested region with the expected
      // attributes, splitting it is unnecessary, and should be avoided as it
      // may result in unbounded recursion when using a strict NX policy.
      //
      if ((EntryValue & ~TT_DESCRIPTOR_PAGE_TYPE_MASK & EntryMask) ==
          (ConvertSectionAttributesToPageAttributes (Descriptor) & EntryMask))
      {
        continue;
      }

      Status = ConvertSectionToPages (FirstLevelIdx << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
      if (EFI_ERROR (Status)) {
        // Exit for loop
        break;
      }

      // Re-read descriptor
      Descriptor = FirstLevelTable[FirstLevelIdx];
      if (FlushTlbs != NULL) {
        *FlushTlbs = TRUE;
      }
    }

    // Obtain page table base address
    PageTable = (ARM_PAGE_TABLE_ENTRY *)TT_DESCRIPTOR_PAGE_BASE_ADDRESS (Descriptor);

    // Calculate index into the page table
    PageTableIndex = ((BaseAddress + Offset) & TT_DESCRIPTOR_PAGE_INDEX_MASK) >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < TRANSLATION_TABLE_PAGE_COUNT);

    // Get the entry
    CurrentPageTableEntry = PageTable[PageTableIndex];

    // Mask off appropriate fields
    PageTableEntry = CurrentPageTableEntry & ~EntryMask;

    // Mask in new attributes and/or permissions
    PageTableEntry |= EntryValue;

    if (CurrentPageTableEntry  != PageTableEntry) {
      Mva = (VOID *)(UINTN)((((UINTN)FirstLevelIdx) << TT_DESCRIPTOR_SECTION_BASE_SHIFT) + (PageTableIndex << TT_DESCRIPTOR_PAGE_BASE_SHIFT));

      // Only need to update if we are changing the entry
      PageTable[PageTableIndex] = PageTableEntry;
      ArmUpdateTranslationTableEntry ((VOID *)&PageTable[PageTableIndex], Mva);
    }

    Status  = EFI_SUCCESS;
    Offset += TT_DESCRIPTOR_PAGE_SIZE;
  } // End first level translation table loop

  return Status;
}

STATIC
EFI_STATUS
UpdateSectionEntries (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes,
  IN UINT32                EntryMask
  )
{
  EFI_STATUS                           Status;
  UINT32                               EntryValue;
  UINT32                               FirstLevelIdx;
  UINT32                               NumSections;
  UINT32                               i;
  UINT32                               CurrentDescriptor;
  UINT32                               Descriptor;
  VOID                                 *Mva;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR  *FirstLevelTable;

  Status = EFI_SUCCESS;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask

  // Make sure we handle a section range that is unmapped
  EntryValue = TT_DESCRIPTOR_SECTION_TYPE_SECTION;

  // Although the PI spec is unclear on this, the GCD guarantees that only
  // one Attribute bit is set at a time, so the order of the conditionals below
  // is irrelevant. If no memory attribute is specified, we preserve whatever
  // memory type is set in the page tables, and update the permission attributes
  // only.
  if ((Attributes & EFI_MEMORY_UC) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // map to strongly ordered
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
  } else if ((Attributes & EFI_MEMORY_WC) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // map to normal non-cacheable
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
  } else if ((Attributes & EFI_MEMORY_WT) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // write through with no-allocate
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
  } else if ((Attributes & EFI_MEMORY_WB) != 0) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // write back (with allocate)
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
  } else if ((Attributes & CACHE_ATTRIBUTE_MASK) != 0) {
    // catch unsupported memory type attributes
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_MEMORY_RO) != 0) {
    EntryValue |= TT_DESCRIPTOR_SECTION_AP_RO_RO;
  } else {
    EntryValue |= TT_DESCRIPTOR_SECTION_AP_RW_RW;
  }

  if ((Attributes & EFI_MEMORY_XP) != 0) {
    EntryValue |= TT_DESCRIPTOR_SECTION_XN_MASK;
  }

  if ((Attributes & EFI_MEMORY_RP) == 0) {
    EntryValue |= TT_DESCRIPTOR_SECTION_AF;
  }

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // calculate number of 1MB first level entries this applies to
  NumSections = (UINT32)(Length / TT_DESCRIPTOR_SECTION_SIZE);

  // iterate through each descriptor
  for (i = 0; i < NumSections; i++) {
    CurrentDescriptor = FirstLevelTable[FirstLevelIdx + i];

    // has this descriptor already been converted to pages?
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (CurrentDescriptor)) {
      // forward this 1MB range to page table function instead
      Status = UpdatePageEntries (
                 (FirstLevelIdx + i) << TT_DESCRIPTOR_SECTION_BASE_SHIFT,
                 TT_DESCRIPTOR_SECTION_SIZE,
                 Attributes,
                 ConvertSectionAttributesToPageAttributes (EntryMask),
                 NULL
                 );
    } else {
      // still a section entry

      if (CurrentDescriptor != 0) {
        // mask off appropriate fields
        Descriptor = CurrentDescriptor & ~EntryMask;
      } else {
        Descriptor = ((UINTN)FirstLevelIdx + i) << TT_DESCRIPTOR_SECTION_BASE_SHIFT;
      }

      // mask in new attributes and/or permissions
      Descriptor |= EntryValue;

      if (CurrentDescriptor  != Descriptor) {
        Mva = (VOID *)(UINTN)(((UINTN)FirstLevelIdx + i) << TT_DESCRIPTOR_SECTION_BASE_SHIFT);

        // Only need to update if we are changing the descriptor
        FirstLevelTable[FirstLevelIdx + i] = Descriptor;
        ArmUpdateTranslationTableEntry ((VOID *)&FirstLevelTable[FirstLevelIdx + i], Mva);
      }

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

/**
  Update the permission or memory type attributes on a range of memory.

  @param  BaseAddress           The start of the region.
  @param  Length                The size of the region.
  @param  Attributes            A mask of EFI_MEMORY_xx constants.
  @param  SectionMask           A mask of short descriptor section attributes
                                describing which descriptor bits to update.

  @retval EFI_SUCCESS           The attributes were set successfully.
  @retval EFI_OUT_OF_RESOURCES  The operation failed due to insufficient memory.

**/
STATIC
EFI_STATUS
SetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes,
  IN UINT32                SectionMask
  )
{
  EFI_STATUS  Status;
  UINT64      ChunkLength;
  BOOLEAN     FlushTlbs;

  if (BaseAddress > (UINT64)MAX_ADDRESS) {
    DEBUG ((
      DEBUG_ERROR,
      "%a BaseAddress: 0x%llx is greater than MAX_ADDRESS: 0x%llx, fail to apply attributes!\n",
      __func__,
      BaseAddress,
      (UINT64)MAX_ADDRESS
      ));
    return EFI_UNSUPPORTED;
  }

  Length = MIN (Length, (UINT64)MAX_ADDRESS - BaseAddress + 1);
  if (Length == 0) {
    return EFI_SUCCESS;
  }

  FlushTlbs = FALSE;
  while (Length > 0) {
    if ((BaseAddress % TT_DESCRIPTOR_SECTION_SIZE == 0) &&
        (Length >= TT_DESCRIPTOR_SECTION_SIZE))
    {
      ChunkLength = Length - Length % TT_DESCRIPTOR_SECTION_SIZE;

      DEBUG ((
        DEBUG_PAGE,
        "SetMemoryAttributes(): MMU section 0x%lx length 0x%lx to %lx\n",
        BaseAddress,
        ChunkLength,
        Attributes
        ));

      Status = UpdateSectionEntries (
                 BaseAddress,
                 ChunkLength,
                 Attributes,
                 SectionMask
                 );

      FlushTlbs = TRUE;
    } else {
      //
      // Process page by page until the next section boundary, but only if
      // we have more than a section's worth of area to deal with after that.
      //
      ChunkLength = TT_DESCRIPTOR_SECTION_SIZE -
                    (BaseAddress % TT_DESCRIPTOR_SECTION_SIZE);
      if (ChunkLength + TT_DESCRIPTOR_SECTION_SIZE > Length) {
        ChunkLength = Length;
      }

      DEBUG ((
        DEBUG_PAGE,
        "SetMemoryAttributes(): MMU page 0x%lx length 0x%lx to %lx\n",
        BaseAddress,
        ChunkLength,
        Attributes
        ));

      Status = UpdatePageEntries (
                 BaseAddress,
                 ChunkLength,
                 Attributes,
                 ConvertSectionAttributesToPageAttributes (SectionMask),
                 &FlushTlbs
                 );
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed to update attributes with status %r for BaseAddress 0x%llx of length 0x%llx\n",
        __func__,
        Status,
        BaseAddress,
        ChunkLength
        ));
      break;
    }

    BaseAddress += ChunkLength;
    Length      -= ChunkLength;
  }

  if (FlushTlbs) {
    ArmInvalidateTlb ();
  }

  return Status;
}

/**
  Set the requested memory permission attributes on a region of memory.

  BaseAddress and Length must be aligned to EFI_PAGE_SIZE.

  If Attributes contains a memory type attribute (EFI_MEMORY_UC/WC/WT/WB), the
  region is mapped according to this memory type, and additional memory
  permission attributes (EFI_MEMORY_RP/RO/XP) are taken into account as well,
  discarding any permission attributes that are currently set for the region.
  AttributeMask is ignored in this case, and must be set to 0x0.

  If Attributes contains only a combination of memory permission attributes
  (EFI_MEMORY_RP/RO/XP), each page in the region will retain its existing
  memory type, even if it is not uniformly set across the region. In this case,
  AttributesMask may be set to a mask of permission attributes, and memory
  permissions omitted from this mask will not be updated for any page in the
  region. All attributes appearing in Attributes must appear in AttributeMask
  as well. (Attributes & ~AttributeMask must produce 0x0)

  @param[in]  BaseAddress     The physical address that is the start address of
                              a memory region.
  @param[in]  Length          The size in bytes of the memory region.
  @param[in]  Attributes      Mask of memory attributes to set.
  @param[in]  AttributeMask   Mask of memory attributes to take into account.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER BaseAddress or Length is not suitably aligned.
                                Invalid combination of Attributes and
                                AttributeMask.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.

**/
EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes,
  IN UINT64                AttributeMask
  )
{
  UINT32  TtEntryMask;

  if (((BaseAddress | Length) & EFI_PAGE_MASK) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_MEMORY_CACHETYPE_MASK) == 0) {
    //
    // No memory type was set in Attributes, so we are going to update the
    // permissions only.
    //
    if (AttributeMask != 0) {
      if (((AttributeMask & ~(UINT64)(EFI_MEMORY_RP|EFI_MEMORY_RO|EFI_MEMORY_XP)) != 0) ||
          ((Attributes & ~AttributeMask) != 0))
      {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      AttributeMask = EFI_MEMORY_RP | EFI_MEMORY_RO | EFI_MEMORY_XP;
    }

    TtEntryMask = 0;
    if ((AttributeMask & EFI_MEMORY_RP) != 0) {
      TtEntryMask |= TT_DESCRIPTOR_SECTION_AF;
    }

    if ((AttributeMask & EFI_MEMORY_RO) != 0) {
      TtEntryMask |= TT_DESCRIPTOR_SECTION_AP_MASK;
    }

    if ((AttributeMask & EFI_MEMORY_XP) != 0) {
      TtEntryMask |= TT_DESCRIPTOR_SECTION_XN_MASK;
    }
  } else {
    ASSERT (AttributeMask == 0);
    if (AttributeMask != 0) {
      return EFI_INVALID_PARAMETER;
    }

    TtEntryMask = TT_DESCRIPTOR_SECTION_TYPE_MASK |
                  TT_DESCRIPTOR_SECTION_XN_MASK |
                  TT_DESCRIPTOR_SECTION_AP_MASK |
                  TT_DESCRIPTOR_SECTION_AF;
  }

  return SetMemoryAttributes (
           BaseAddress,
           Length,
           Attributes,
           TtEntryMask
           );
}
