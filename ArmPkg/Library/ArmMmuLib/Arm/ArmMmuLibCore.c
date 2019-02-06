/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011-2016, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Chipset/ArmV7.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#define ID_MMFR0_SHARELVL_SHIFT       12
#define ID_MMFR0_SHARELVL_MASK       0xf
#define ID_MMFR0_SHARELVL_ONE          0
#define ID_MMFR0_SHARELVL_TWO          1

#define ID_MMFR0_INNERSHR_SHIFT       28
#define ID_MMFR0_INNERSHR_MASK       0xf
#define ID_MMFR0_OUTERSHR_SHIFT        8
#define ID_MMFR0_OUTERSHR_MASK       0xf

#define ID_MMFR0_SHR_IMP_UNCACHED      0
#define ID_MMFR0_SHR_IMP_HW_COHERENT   1
#define ID_MMFR0_SHR_IGNORED         0xf

#define __EFI_MEMORY_RWX               0    // no restrictions

#define CACHE_ATTRIBUTE_MASK   (EFI_MEMORY_UC | \
                                EFI_MEMORY_WC | \
                                EFI_MEMORY_WT | \
                                EFI_MEMORY_WB | \
                                EFI_MEMORY_UCE | \
                                EFI_MEMORY_WP)

UINTN
EFIAPI
ArmReadIdMmfr0 (
  VOID
  );

BOOLEAN
EFIAPI
ArmHasMpExtensions (
  VOID
  );

UINT32
ConvertSectionAttributesToPageAttributes (
  IN UINT32   SectionAttributes,
  IN BOOLEAN  IsLargePage
  )
{
  UINT32 PageAttributes;

  PageAttributes = 0;
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_CACHE_POLICY (SectionAttributes, IsLargePage);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_AP (SectionAttributes);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_XN (SectionAttributes, IsLargePage);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_NG (SectionAttributes);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_S (SectionAttributes);

  return PageAttributes;
}

STATIC
BOOLEAN
PreferNonshareableMemory (
  VOID
  )
{
  UINTN   Mmfr;
  UINTN   Val;

  if (FeaturePcdGet (PcdNormalMemoryNonshareableOverride)) {
    return TRUE;
  }

  //
  // Check whether the innermost level of shareability (the level we will use
  // by default to map normal memory) is implemented with hardware coherency
  // support. Otherwise, revert to mapping as non-shareable.
  //
  Mmfr = ArmReadIdMmfr0 ();
  switch ((Mmfr >> ID_MMFR0_SHARELVL_SHIFT) & ID_MMFR0_SHARELVL_MASK) {
  case ID_MMFR0_SHARELVL_ONE:
    // one level of shareability
    Val = (Mmfr >> ID_MMFR0_OUTERSHR_SHIFT) & ID_MMFR0_OUTERSHR_MASK;
    break;
  case ID_MMFR0_SHARELVL_TWO:
    // two levels of shareability
    Val = (Mmfr >> ID_MMFR0_INNERSHR_SHIFT) & ID_MMFR0_INNERSHR_MASK;
    break;
  default:
    // unexpected value -> shareable is the safe option
    ASSERT (FALSE);
    return FALSE;
  }
  return Val != ID_MMFR0_SHR_IMP_HW_COHERENT;
}

STATIC
VOID
PopulateLevel2PageTable (
  IN UINT32                         *SectionEntry,
  IN UINT32                         PhysicalBase,
  IN UINT32                         RemainLength,
  IN ARM_MEMORY_REGION_ATTRIBUTES   Attributes
  )
{
  UINT32* PageEntry;
  UINT32  Pages;
  UINT32  Index;
  UINT32  PageAttributes;
  UINT32  SectionDescriptor;
  UINT32  TranslationTable;
  UINT32  BaseSectionAddress;
  UINT32  FirstPageOffset;

  switch (Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_NONSHAREABLE:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK_NONSHAREABLE:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      PageAttributes &= ~TT_DESCRIPTOR_PAGE_S_SHARED;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_THROUGH;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE:
      PageAttributes = TT_DESCRIPTOR_PAGE_DEVICE;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED:
      PageAttributes = TT_DESCRIPTOR_PAGE_UNCACHED;
      break;
    default:
      PageAttributes = TT_DESCRIPTOR_PAGE_UNCACHED;
      break;
  }

  if (PreferNonshareableMemory ()) {
    PageAttributes &= ~TT_DESCRIPTOR_PAGE_S_SHARED;
  }

  // Check if the Section Entry has already been populated. Otherwise attach a
  // Level 2 Translation Table to it
  if (*SectionEntry != 0) {
    // The entry must be a page table. Otherwise it exists an overlapping in the memory map
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(*SectionEntry)) {
      TranslationTable = *SectionEntry & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK;
    } else if ((*SectionEntry & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) {
      // Case where a virtual memory map descriptor overlapped a section entry

      // Allocate a Level2 Page Table for this Section
      TranslationTable = (UINTN)AllocatePages(EFI_SIZE_TO_PAGES(TRANSLATION_TABLE_PAGE_SIZE + TRANSLATION_TABLE_PAGE_ALIGNMENT));
      TranslationTable = ((UINTN)TranslationTable + TRANSLATION_TABLE_PAGE_ALIGNMENT_MASK) & ~TRANSLATION_TABLE_PAGE_ALIGNMENT_MASK;

      // Translate the Section Descriptor into Page Descriptor
      SectionDescriptor = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (*SectionEntry, FALSE);

      BaseSectionAddress = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(*SectionEntry);

      // Populate the new Level2 Page Table for the section
      PageEntry = (UINT32*)TranslationTable;
      for (Index = 0; Index < TRANSLATION_TABLE_PAGE_COUNT; Index++) {
        PageEntry[Index] = TT_DESCRIPTOR_PAGE_BASE_ADDRESS(BaseSectionAddress + (Index << 12)) | SectionDescriptor;
      }

      // Overwrite the section entry to point to the new Level2 Translation Table
      *SectionEntry = (TranslationTable & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) |
          (IS_ARM_MEMORY_REGION_ATTRIBUTES_SECURE(Attributes) ? (1 << 3) : 0) |
          TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;
    } else {
      // We do not support the other section type (16MB Section)
      ASSERT(0);
      return;
    }
  } else {
    TranslationTable = (UINTN)AllocatePages(EFI_SIZE_TO_PAGES(TRANSLATION_TABLE_PAGE_SIZE + TRANSLATION_TABLE_PAGE_ALIGNMENT));
    TranslationTable = ((UINTN)TranslationTable + TRANSLATION_TABLE_PAGE_ALIGNMENT_MASK) & ~TRANSLATION_TABLE_PAGE_ALIGNMENT_MASK;

    ZeroMem ((VOID *)TranslationTable, TRANSLATION_TABLE_PAGE_SIZE);

    *SectionEntry = (TranslationTable & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) |
        (IS_ARM_MEMORY_REGION_ATTRIBUTES_SECURE(Attributes) ? (1 << 3) : 0) |
        TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;
  }

  FirstPageOffset = (PhysicalBase & TT_DESCRIPTOR_PAGE_INDEX_MASK) >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
  PageEntry = (UINT32 *)TranslationTable + FirstPageOffset;
  Pages     = RemainLength / TT_DESCRIPTOR_PAGE_SIZE;

  ASSERT (FirstPageOffset + Pages <= TRANSLATION_TABLE_PAGE_COUNT);

  for (Index = 0; Index < Pages; Index++) {
    *PageEntry++     =  TT_DESCRIPTOR_PAGE_BASE_ADDRESS(PhysicalBase) | PageAttributes;
    PhysicalBase += TT_DESCRIPTOR_PAGE_SIZE;
  }

}

STATIC
VOID
FillTranslationTable (
  IN  UINT32                        *TranslationTable,
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  )
{
  UINT32  *SectionEntry;
  UINT32  Attributes;
  UINT32  PhysicalBase;
  UINT64  RemainLength;
  UINT32  PageMapLength;

  ASSERT(MemoryRegion->Length > 0);

  if (MemoryRegion->PhysicalBase >= SIZE_4GB) {
    return;
  }

  PhysicalBase = MemoryRegion->PhysicalBase;
  RemainLength = MIN(MemoryRegion->Length, SIZE_4GB - PhysicalBase);

  switch (MemoryRegion->Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_NONSHAREABLE:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
      Attributes &= ~TT_DESCRIPTOR_SECTION_S_SHARED;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK_NONSHAREABLE:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(1);
      Attributes &= ~TT_DESCRIPTOR_SECTION_S_SHARED;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(1);
      break;
    default:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(0);
      break;
  }

  if (PreferNonshareableMemory ()) {
    Attributes &= ~TT_DESCRIPTOR_SECTION_S_SHARED;
  }

  // Get the first section entry for this mapping
  SectionEntry    = TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS(TranslationTable, MemoryRegion->VirtualBase);

  while (RemainLength != 0) {
    if (PhysicalBase % TT_DESCRIPTOR_SECTION_SIZE == 0 &&
        RemainLength >= TT_DESCRIPTOR_SECTION_SIZE) {
      // Case: Physical address aligned on the Section Size (1MB) && the length
      // is greater than the Section Size
      *SectionEntry++ = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(PhysicalBase) | Attributes;
      PhysicalBase += TT_DESCRIPTOR_SECTION_SIZE;
      RemainLength -= TT_DESCRIPTOR_SECTION_SIZE;
    } else {
      PageMapLength = MIN (RemainLength, TT_DESCRIPTOR_SECTION_SIZE -
                                         (PhysicalBase % TT_DESCRIPTOR_SECTION_SIZE));

      // Case: Physical address aligned on the Section Size (1MB) && the length
      //       does not fill a section
      // Case: Physical address NOT aligned on the Section Size (1MB)
      PopulateLevel2PageTable (SectionEntry++, PhysicalBase, PageMapLength,
        MemoryRegion->Attributes);

      // If it is the last entry
      if (RemainLength < TT_DESCRIPTOR_SECTION_SIZE) {
        break;
      }

      PhysicalBase += PageMapLength;
      RemainLength -= PageMapLength;
    }
  }
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
  ARM_MEMORY_REGION_ATTRIBUTES  TranslationTableAttribute;
  UINT32                        TTBRAttributes;

  // Allocate pages for translation table.
  TranslationTable = AllocatePages (EFI_SIZE_TO_PAGES (TRANSLATION_TABLE_SECTION_SIZE + TRANSLATION_TABLE_SECTION_ALIGNMENT));
  if (TranslationTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  TranslationTable = (VOID*)(((UINTN)TranslationTable + TRANSLATION_TABLE_SECTION_ALIGNMENT_MASK) & ~TRANSLATION_TABLE_SECTION_ALIGNMENT_MASK);

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = TranslationTable;
  }

  if (TranslationTableSize != NULL) {
    *TranslationTableSize = TRANSLATION_TABLE_SECTION_SIZE;
  }

  ZeroMem (TranslationTable, TRANSLATION_TABLE_SECTION_SIZE);

  // By default, mark the translation table as belonging to a uncached region
  TranslationTableAttribute = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  while (MemoryTable->Length != 0) {
    // Find the memory attribute for the Translation Table
    if (((UINTN)TranslationTable >= MemoryTable->PhysicalBase) && ((UINTN)TranslationTable <= MemoryTable->PhysicalBase - 1 + MemoryTable->Length)) {
      TranslationTableAttribute = MemoryTable->Attributes;
    }

    FillTranslationTable (TranslationTable, MemoryTable);
    MemoryTable++;
  }

  // Translate the Memory Attributes into Translation Table Register Attributes
  if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK)) {
    TTBRAttributes = ArmHasMpExtensions () ? TTBR_MP_WRITE_BACK_ALLOC : TTBR_WRITE_BACK_ALLOC;
  } else {
    // Page tables must reside in memory mapped as write-back cacheable
    ASSERT (0);
    return RETURN_UNSUPPORTED;
  }

  if (TTBRAttributes & TTBR_SHAREABLE) {
    if (PreferNonshareableMemory ()) {
      TTBRAttributes ^= TTBR_SHAREABLE;
    } else {
      //
      // Unlike the S bit in the short descriptors, which implies inner shareable
      // on an implementation that supports two levels, the meaning of the S bit
      // in the TTBR depends on the NOS bit, which defaults to Outer Shareable.
      // However, we should only set this bit after we have confirmed that the
      // implementation supports multiple levels, or else the NOS bit is UNK/SBZP
      //
      if (((ArmReadIdMmfr0 () >> 12) & 0xf) != 0) {
        TTBRAttributes |= TTBR_NOT_OUTER_SHAREABLE;
      }
    }
  }

  ArmCleanInvalidateDataCache ();
  ArmInvalidateInstructionCache ();

  ArmDisableDataCache ();
  ArmDisableInstructionCache();
  // TLBs are also invalidated when calling ArmDisableMmu()
  ArmDisableMmu ();

  // Make sure nothing sneaked into the cache
  ArmCleanInvalidateDataCache ();
  ArmInvalidateInstructionCache ();

  ArmSetTTBR0 ((VOID *)(UINTN)(((UINTN)TranslationTable & ~TRANSLATION_TABLE_SECTION_ALIGNMENT_MASK) | (TTBRAttributes & 0x7F)));

  //
  // The TTBCR register value is undefined at reset in the Non-Secure world.
  // Writing 0 has the effect of:
  //   Clearing EAE: Use short descriptors, as mandated by specification.
  //   Clearing PD0 and PD1: Translation Table Walk Disable is off.
  //   Clearing N: Perform all translation table walks through TTBR0.
  //               (0 is the default reset value in systems not implementing
  //               the Security Extensions.)
  //
  ArmSetTTBCR (0);

  ArmSetDomainAccessControl (DOMAIN_ACCESS_CONTROL_NONE(15) |
                             DOMAIN_ACCESS_CONTROL_NONE(14) |
                             DOMAIN_ACCESS_CONTROL_NONE(13) |
                             DOMAIN_ACCESS_CONTROL_NONE(12) |
                             DOMAIN_ACCESS_CONTROL_NONE(11) |
                             DOMAIN_ACCESS_CONTROL_NONE(10) |
                             DOMAIN_ACCESS_CONTROL_NONE( 9) |
                             DOMAIN_ACCESS_CONTROL_NONE( 8) |
                             DOMAIN_ACCESS_CONTROL_NONE( 7) |
                             DOMAIN_ACCESS_CONTROL_NONE( 6) |
                             DOMAIN_ACCESS_CONTROL_NONE( 5) |
                             DOMAIN_ACCESS_CONTROL_NONE( 4) |
                             DOMAIN_ACCESS_CONTROL_NONE( 3) |
                             DOMAIN_ACCESS_CONTROL_NONE( 2) |
                             DOMAIN_ACCESS_CONTROL_NONE( 1) |
                             DOMAIN_ACCESS_CONTROL_CLIENT(0));

  ArmEnableInstructionCache();
  ArmEnableDataCache();
  ArmEnableMmu();
  return RETURN_SUCCESS;
}

STATIC
EFI_STATUS
ConvertSectionToPages (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  UINT32                  FirstLevelIdx;
  UINT32                  SectionDescriptor;
  UINT32                  PageTableDescriptor;
  UINT32                  PageDescriptor;
  UINT32                  Index;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  DEBUG ((EFI_D_PAGE, "Converting section at 0x%x to pages\n", (UINTN)BaseAddress));

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // Get section attributes and convert to page attributes
  SectionDescriptor = FirstLevelTable[FirstLevelIdx];
  PageDescriptor = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (SectionDescriptor, FALSE);

  // Allocate a page table for the 4KB entries (we use up a full page even though we only need 1KB)
  PageTable = (volatile ARM_PAGE_TABLE_ENTRY *)AllocatePages (1);
  if (PageTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Write the page table entries out
  for (Index = 0; Index < TRANSLATION_TABLE_PAGE_COUNT; Index++) {
    PageTable[Index] = TT_DESCRIPTOR_PAGE_BASE_ADDRESS(BaseAddress + (Index << 12)) | PageDescriptor;
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
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINT64                    Attributes,
  OUT BOOLEAN                   *FlushTlbs OPTIONAL
  )
{
  EFI_STATUS    Status;
  UINT32        EntryValue;
  UINT32        EntryMask;
  UINT32        FirstLevelIdx;
  UINT32        Offset;
  UINT32        NumPageEntries;
  UINT32        Descriptor;
  UINT32        p;
  UINT32        PageTableIndex;
  UINT32        PageTableEntry;
  UINT32        CurrentPageTableEntry;
  VOID          *Mva;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  Status = EFI_SUCCESS;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask
  EntryMask = TT_DESCRIPTOR_PAGE_TYPE_MASK | TT_DESCRIPTOR_PAGE_AP_MASK;
  if (Attributes & EFI_MEMORY_XP) {
    EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE_XN;
  } else {
    EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE;
  }

  // Although the PI spec is unclear on this, the GCD guarantees that only
  // one Attribute bit is set at a time, so the order of the conditionals below
  // is irrelevant. If no memory attribute is specified, we preserve whatever
  // memory type is set in the page tables, and update the permission attributes
  // only.
  if (Attributes & EFI_MEMORY_UC) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // map to strongly ordered
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
  } else if (Attributes & EFI_MEMORY_WC) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // map to normal non-cachable
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
  } else if (Attributes & EFI_MEMORY_WT) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // write through with no-allocate
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
  } else if (Attributes & EFI_MEMORY_WB) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
    // write back (with allocate)
    EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
  } else if (Attributes & CACHE_ATTRIBUTE_MASK) {
    // catch unsupported memory type attributes
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (Attributes & EFI_MEMORY_RO) {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RO_RO;
  } else {
    EntryValue |= TT_DESCRIPTOR_PAGE_AP_RW_RW;
  }

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate number of 4KB page table entries to change
  NumPageEntries = Length / TT_DESCRIPTOR_PAGE_SIZE;

  // Iterate for the number of 4KB pages to change
  Offset = 0;
  for(p = 0; p < NumPageEntries; p++) {
    // Calculate index into first level translation table for page table value

    FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress + Offset) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
    ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

    // Read the descriptor from the first level page table
    Descriptor = FirstLevelTable[FirstLevelIdx];

    // Does this descriptor need to be converted from section entry to 4K pages?
    if (!TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(Descriptor)) {
      Status = ConvertSectionToPages (FirstLevelIdx << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
      if (EFI_ERROR(Status)) {
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
    PageTable = (ARM_PAGE_TABLE_ENTRY *)TT_DESCRIPTOR_PAGE_BASE_ADDRESS(Descriptor);

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

    Status = EFI_SUCCESS;
    Offset += TT_DESCRIPTOR_PAGE_SIZE;

  } // End first level translation table loop

  return Status;
}

STATIC
EFI_STATUS
UpdateSectionEntries (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT32        EntryMask;
  UINT32        EntryValue;
  UINT32        FirstLevelIdx;
  UINT32        NumSections;
  UINT32        i;
  UINT32        CurrentDescriptor;
  UINT32        Descriptor;
  VOID          *Mva;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;

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
  if (Attributes & EFI_MEMORY_UC) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // map to strongly ordered
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
  } else if (Attributes & EFI_MEMORY_WC) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // map to normal non-cachable
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
  } else if (Attributes & EFI_MEMORY_WT) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // write through with no-allocate
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
  } else if (Attributes & EFI_MEMORY_WB) {
    // modify cacheability attributes
    EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
    // write back (with allocate)
    EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
  } else if (Attributes & CACHE_ATTRIBUTE_MASK) {
    // catch unsupported memory type attributes
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (Attributes & EFI_MEMORY_RO) {
    EntryValue |= TT_DESCRIPTOR_SECTION_AP_RO_RO;
  } else {
    EntryValue |= TT_DESCRIPTOR_SECTION_AP_RW_RW;
  }

  if (Attributes & EFI_MEMORY_XP) {
    EntryValue |= TT_DESCRIPTOR_SECTION_XN_MASK;
  }

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // calculate number of 1MB first level entries this applies to
  NumSections = Length / TT_DESCRIPTOR_SECTION_SIZE;

  // iterate through each descriptor
  for(i=0; i<NumSections; i++) {
    CurrentDescriptor = FirstLevelTable[FirstLevelIdx + i];

    // has this descriptor already been converted to pages?
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(CurrentDescriptor)) {
      // forward this 1MB range to page table function instead
      Status = UpdatePageEntries (
                 (FirstLevelIdx + i) << TT_DESCRIPTOR_SECTION_BASE_SHIFT,
                 TT_DESCRIPTOR_SECTION_SIZE,
                 Attributes,
                 NULL);
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
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  EFI_STATUS    Status;
  UINT64        ChunkLength;
  BOOLEAN       FlushTlbs;

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
        Length >= TT_DESCRIPTOR_SECTION_SIZE) {

      ChunkLength = Length - Length % TT_DESCRIPTOR_SECTION_SIZE;

      DEBUG ((DEBUG_PAGE,
        "SetMemoryAttributes(): MMU section 0x%lx length 0x%lx to %lx\n",
        BaseAddress, ChunkLength, Attributes));

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

      DEBUG ((DEBUG_PAGE,
        "SetMemoryAttributes(): MMU page 0x%lx length 0x%lx to %lx\n",
        BaseAddress, ChunkLength, Attributes));

      Status = UpdatePageEntries (BaseAddress, ChunkLength, Attributes,
                 &FlushTlbs);
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    BaseAddress += ChunkLength;
    Length -= ChunkLength;
  }

  if (FlushTlbs) {
    ArmInvalidateTlb ();
  }
  return Status;
}

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, EFI_MEMORY_XP);
}

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, __EFI_MEMORY_RWX);
}

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, EFI_MEMORY_RO);
}

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return ArmSetMemoryAttributes (BaseAddress, Length, __EFI_MEMORY_RWX);
}

RETURN_STATUS
EFIAPI
ArmMmuBaseLibConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}
