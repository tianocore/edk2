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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Chipset/ArmV7.h>

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
  PageDescriptor    = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (SectionDescriptor, FALSE);

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
  OUT BOOLEAN               *FlushTlbs OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      EntryValue;
  UINT32      EntryMask;
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
  EntryMask = TT_DESCRIPTOR_PAGE_TYPE_MASK | TT_DESCRIPTOR_PAGE_AP_MASK;
  if ((Attributes & EFI_MEMORY_XP) != 0) {
    EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE_XN;
  } else {
    EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE;
  }

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

  if ((Attributes & EFI_MEMORY_RO) != 0) {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RO_RO;
  } else {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RW_RW;
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
  IN UINT64                Attributes
  )
{
  EFI_STATUS                           Status;
  UINT32                               EntryMask;
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
  EntryMask = TT_DESCRIPTOR_SECTION_TYPE_MASK | TT_DESCRIPTOR_SECTION_XN_MASK |
              TT_DESCRIPTOR_SECTION_AP_MASK;
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

EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
{
  EFI_STATUS  Status;
  UINT64      ChunkLength;
  BOOLEAN     FlushTlbs;

  if (BaseAddress > (UINT64)MAX_ADDRESS) {
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

      Status = UpdateSectionEntries (BaseAddress, ChunkLength, Attributes);

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
                 &FlushTlbs
                 );
    }

    if (EFI_ERROR (Status)) {
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

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, EFI_MEMORY_XP);
}

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, __EFI_MEMORY_RWX);
}

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, EFI_MEMORY_RO);
}

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, __EFI_MEMORY_RWX);
}
