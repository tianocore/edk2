/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011-2016, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Arm/AArch32.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#define ID_MMFR0_SHARELVL_SHIFT  12
#define ID_MMFR0_SHARELVL_MASK   0xf
#define ID_MMFR0_SHARELVL_ONE    0
#define ID_MMFR0_SHARELVL_TWO    1

#define ID_MMFR0_INNERSHR_SHIFT  28
#define ID_MMFR0_INNERSHR_MASK   0xf
#define ID_MMFR0_OUTERSHR_SHIFT  8
#define ID_MMFR0_OUTERSHR_MASK   0xf

#define ID_MMFR0_SHR_IMP_UNCACHED     0
#define ID_MMFR0_SHR_IMP_HW_COHERENT  1
#define ID_MMFR0_SHR_IGNORED          0xf

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

STATIC
BOOLEAN
PreferNonshareableMemory (
  VOID
  )
{
  UINTN  Mmfr;
  UINTN  Val;

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
  IN UINT32                        *SectionEntry,
  IN UINT32                        PhysicalBase,
  IN UINT32                        RemainLength,
  IN ARM_MEMORY_REGION_ATTRIBUTES  Attributes
  )
{
  UINT32  *PageEntry;
  UINT32  Pages;
  UINT32  Index;
  UINT32  PageAttributes;
  UINT32  SectionDescriptor;
  UINT32  TranslationTable;
  UINT32  BaseSectionAddress;
  UINT32  FirstPageOffset;

  switch (Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_NONSHAREABLE:
      PageAttributes  = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      PageAttributes &= ~TT_DESCRIPTOR_PAGE_S_SHARED;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_RO:
      PageAttributes  = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      PageAttributes |= TT_DESCRIPTOR_PAGE_AP_NO_RO;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_XP:
      PageAttributes  = TT_DESCRIPTOR_PAGE_WRITE_BACK;
      PageAttributes |= TT_DESCRIPTOR_PAGE_XN_MASK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_THROUGH;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
      PageAttributes = TT_DESCRIPTOR_PAGE_DEVICE;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
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
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (*SectionEntry)) {
      TranslationTable = *SectionEntry & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK;
    } else if ((*SectionEntry & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) {
      // Case where a virtual memory map descriptor overlapped a section entry

      // Allocate a Level2 Page Table for this Section
      TranslationTable = (UINTN)AllocateAlignedPages (
                                  EFI_SIZE_TO_PAGES (TRANSLATION_TABLE_PAGE_SIZE),
                                  TRANSLATION_TABLE_PAGE_ALIGNMENT
                                  );

      // Translate the Section Descriptor into Page Descriptor
      SectionDescriptor = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (*SectionEntry);

      BaseSectionAddress = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (*SectionEntry);

      //
      // Make sure we are not inadvertently hitting in the caches
      // when populating the page tables
      //
      InvalidateDataCacheRange (
        (VOID *)TranslationTable,
        TRANSLATION_TABLE_PAGE_SIZE
        );

      // Populate the new Level2 Page Table for the section
      PageEntry = (UINT32 *)TranslationTable;
      for (Index = 0; Index < TRANSLATION_TABLE_PAGE_COUNT; Index++) {
        PageEntry[Index] = TT_DESCRIPTOR_PAGE_BASE_ADDRESS (BaseSectionAddress + (Index << 12)) | SectionDescriptor;
      }

      // Overwrite the section entry to point to the new Level2 Translation Table
      *SectionEntry = (TranslationTable & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) |
                      TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;
    } else {
      // We do not support the other section type (16MB Section)
      ASSERT (0);
      return;
    }
  } else {
    TranslationTable = (UINTN)AllocateAlignedPages (
                                EFI_SIZE_TO_PAGES (TRANSLATION_TABLE_PAGE_SIZE),
                                TRANSLATION_TABLE_PAGE_ALIGNMENT
                                );
    //
    // Make sure we are not inadvertently hitting in the caches
    // when populating the page tables
    //
    InvalidateDataCacheRange (
      (VOID *)TranslationTable,
      TRANSLATION_TABLE_PAGE_SIZE
      );
    ZeroMem ((VOID *)TranslationTable, TRANSLATION_TABLE_PAGE_SIZE);

    *SectionEntry = (TranslationTable & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) |
                    TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;
  }

  FirstPageOffset = (PhysicalBase & TT_DESCRIPTOR_PAGE_INDEX_MASK) >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
  PageEntry       = (UINT32 *)TranslationTable + FirstPageOffset;
  Pages           = RemainLength / TT_DESCRIPTOR_PAGE_SIZE;

  ASSERT (FirstPageOffset + Pages <= TRANSLATION_TABLE_PAGE_COUNT);

  for (Index = 0; Index < Pages; Index++) {
    *PageEntry++  =  TT_DESCRIPTOR_PAGE_BASE_ADDRESS (PhysicalBase) | PageAttributes;
    PhysicalBase += TT_DESCRIPTOR_PAGE_SIZE;
  }

  //
  // Invalidate again to ensure that any line fetches that may have occurred
  // [speculatively] since the previous invalidate are evicted again.
  //
  ArmDataMemoryBarrier ();
  InvalidateDataCacheRange (
    (UINT32 *)TranslationTable + FirstPageOffset,
    RemainLength / TT_DESCRIPTOR_PAGE_SIZE * sizeof (*PageEntry)
    );
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

  ASSERT (MemoryRegion->Length > 0);

  if (MemoryRegion->PhysicalBase >= SIZE_4GB) {
    return;
  }

  PhysicalBase = (UINT32)MemoryRegion->PhysicalBase;
  RemainLength = MIN (MemoryRegion->Length, SIZE_4GB - PhysicalBase);

  switch (MemoryRegion->Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_NONSHAREABLE:
      Attributes  = TT_DESCRIPTOR_SECTION_WRITE_BACK;
      Attributes &= ~TT_DESCRIPTOR_SECTION_S_SHARED;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_RO:
      Attributes  = TT_DESCRIPTOR_SECTION_WRITE_BACK;
      Attributes |= TT_DESCRIPTOR_SECTION_AP_NO_RO;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK_XP:
      Attributes  = TT_DESCRIPTOR_SECTION_WRITE_BACK;
      Attributes |= TT_DESCRIPTOR_SECTION_XN_MASK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED;
      break;
    default:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED;
      break;
  }

  if (PreferNonshareableMemory ()) {
    Attributes &= ~TT_DESCRIPTOR_SECTION_S_SHARED;
  }

  // Get the first section entry for this mapping
  SectionEntry = TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS (TranslationTable, MemoryRegion->VirtualBase);

  while (RemainLength != 0) {
    if ((PhysicalBase % TT_DESCRIPTOR_SECTION_SIZE == 0) &&
        (RemainLength >= TT_DESCRIPTOR_SECTION_SIZE))
    {
      // Case: Physical address aligned on the Section Size (1MB) && the length
      // is greater than the Section Size
      *SectionEntry = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (PhysicalBase) | Attributes;

      //
      // Issue a DMB to ensure that the page table entry update made it to
      // memory before we issue the invalidate, otherwise, a subsequent
      // speculative fetch could observe the old value.
      //
      ArmDataMemoryBarrier ();
      ArmInvalidateDataCacheEntryByMVA ((UINTN)SectionEntry++);

      PhysicalBase += TT_DESCRIPTOR_SECTION_SIZE;
      RemainLength -= TT_DESCRIPTOR_SECTION_SIZE;
    } else {
      PageMapLength = MIN (
                        (UINT32)RemainLength,
                        TT_DESCRIPTOR_SECTION_SIZE -
                        (PhysicalBase % TT_DESCRIPTOR_SECTION_SIZE)
                        );

      // Case: Physical address aligned on the Section Size (1MB) && the length
      //       does not fill a section
      // Case: Physical address NOT aligned on the Section Size (1MB)
      PopulateLevel2PageTable (
        SectionEntry,
        PhysicalBase,
        PageMapLength,
        MemoryRegion->Attributes
        );

      //
      // Issue a DMB to ensure that the page table entry update made it to
      // memory before we issue the invalidate, otherwise, a subsequent
      // speculative fetch could observe the old value.
      //
      ArmDataMemoryBarrier ();
      ArmInvalidateDataCacheEntryByMVA ((UINTN)SectionEntry++);

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
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize OPTIONAL
  )
{
  VOID    *TranslationTable;
  UINT32  TTBRAttributes;

  TranslationTable = AllocateAlignedPages (
                       EFI_SIZE_TO_PAGES (TRANSLATION_TABLE_SECTION_SIZE),
                       TRANSLATION_TABLE_SECTION_ALIGNMENT
                       );
  if (TranslationTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = TranslationTable;
  }

  if (TranslationTableSize != NULL) {
    *TranslationTableSize = TRANSLATION_TABLE_SECTION_SIZE;
  }

  //
  // Make sure we are not inadvertently hitting in the caches
  // when populating the page tables
  //
  InvalidateDataCacheRange (TranslationTable, TRANSLATION_TABLE_SECTION_SIZE);
  ZeroMem (TranslationTable, TRANSLATION_TABLE_SECTION_SIZE);

  while (MemoryTable->Length != 0) {
    FillTranslationTable (TranslationTable, MemoryTable);
    MemoryTable++;
  }

  TTBRAttributes = ArmHasMpExtensions () ? TTBR_MP_WRITE_BACK_ALLOC
                                         : TTBR_WRITE_BACK_ALLOC;
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

  ArmSetTTBR0 ((VOID *)((UINTN)TranslationTable | TTBRAttributes));

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

  ArmSetDomainAccessControl (
    DOMAIN_ACCESS_CONTROL_NONE (15) |
    DOMAIN_ACCESS_CONTROL_NONE (14) |
    DOMAIN_ACCESS_CONTROL_NONE (13) |
    DOMAIN_ACCESS_CONTROL_NONE (12) |
    DOMAIN_ACCESS_CONTROL_NONE (11) |
    DOMAIN_ACCESS_CONTROL_NONE (10) |
    DOMAIN_ACCESS_CONTROL_NONE (9) |
    DOMAIN_ACCESS_CONTROL_NONE (8) |
    DOMAIN_ACCESS_CONTROL_NONE (7) |
    DOMAIN_ACCESS_CONTROL_NONE (6) |
    DOMAIN_ACCESS_CONTROL_NONE (5) |
    DOMAIN_ACCESS_CONTROL_NONE (4) |
    DOMAIN_ACCESS_CONTROL_NONE (3) |
    DOMAIN_ACCESS_CONTROL_NONE (2) |
    DOMAIN_ACCESS_CONTROL_NONE (1) |
    DOMAIN_ACCESS_CONTROL_CLIENT (0)
    );

  ArmEnableInstructionCache ();
  ArmEnableDataCache ();
  ArmEnableMmu ();
  return RETURN_SUCCESS;
}
