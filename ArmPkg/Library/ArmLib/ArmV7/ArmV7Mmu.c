/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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
#include <Chipset/ArmV7.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "ArmV7Lib.h"
#include "ArmLibPrivate.h"

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

  switch (Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK:
      PageAttributes = TT_DESCRIPTOR_PAGE_WRITE_BACK;
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

  PageEntry = ((UINT32 *)(TranslationTable) + ((PhysicalBase & TT_DESCRIPTOR_PAGE_INDEX_MASK) >> TT_DESCRIPTOR_PAGE_BASE_SHIFT));
  Pages     = RemainLength / TT_DESCRIPTOR_PAGE_SIZE;

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
  UINT32  PhysicalBase = MemoryRegion->PhysicalBase;
  UINT32  RemainLength = MemoryRegion->Length;

  ASSERT(MemoryRegion->Length > 0);

  switch (MemoryRegion->Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
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

  // Get the first section entry for this mapping
  SectionEntry    = TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS(TranslationTable, MemoryRegion->VirtualBase);

  while (RemainLength != 0) {
    if (PhysicalBase % TT_DESCRIPTOR_SECTION_SIZE == 0) {
      if (RemainLength >= TT_DESCRIPTOR_SECTION_SIZE) {
        // Case: Physical address aligned on the Section Size (1MB) && the length is greater than the Section Size
        *SectionEntry++ = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(PhysicalBase) | Attributes;
        PhysicalBase += TT_DESCRIPTOR_SECTION_SIZE;
      } else {
        // Case: Physical address aligned on the Section Size (1MB) && the length does not fill a section
        PopulateLevel2PageTable (SectionEntry++, PhysicalBase, RemainLength, MemoryRegion->Attributes);

        // It must be the last entry
        break;
      }
    } else {
      // Case: Physical address NOT aligned on the Section Size (1MB)
      PopulateLevel2PageTable (SectionEntry++, PhysicalBase, RemainLength, MemoryRegion->Attributes);
      // Aligned the address
      PhysicalBase = (PhysicalBase + TT_DESCRIPTOR_SECTION_SIZE) & ~(TT_DESCRIPTOR_SECTION_SIZE-1);

      // If it is the last entry
      if (RemainLength < TT_DESCRIPTOR_SECTION_SIZE) {
        break;
      }
    }
    RemainLength -= TT_DESCRIPTOR_SECTION_SIZE;
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
  if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED)) {
    TTBRAttributes = TTBR_NON_CACHEABLE;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK)) {
    TTBRAttributes = TTBR_WRITE_BACK_ALLOC;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH) ||
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH)) {
    TTBRAttributes = TTBR_WRITE_THROUGH_NO_ALLOC;
  } else {
    ASSERT (0); // No support has been found for the attributes of the memory region that the translation table belongs to.
    return RETURN_UNSUPPORTED;
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
                             DOMAIN_ACCESS_CONTROL_MANAGER(0));

  ArmEnableInstructionCache();
  ArmEnableDataCache();
  ArmEnableMmu();
  return RETURN_SUCCESS;
}
