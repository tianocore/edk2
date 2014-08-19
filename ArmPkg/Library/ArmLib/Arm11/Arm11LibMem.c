/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Chipset/ARM1176JZ-S.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

VOID
FillTranslationTable (
  IN  UINT32                        *TranslationTable,
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  )
{
  UINT32  *Entry;
  UINTN   Sections;
  UINTN   Index;
  UINT32  Attributes;
  UINT32  PhysicalBase = MemoryRegion->PhysicalBase;

  switch (MemoryRegion->Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH(0);
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
    case ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(1);
      break;
    default:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(0);
      break;
  }

  Entry    = TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS(TranslationTable, MemoryRegion->VirtualBase);
  Sections = ((( MemoryRegion->Length - 1 ) / TT_DESCRIPTOR_SECTION_SIZE ) + 1 );

  for (Index = 0; Index < Sections; Index++)
  {
    *Entry++     =  TT_DESCRIPTOR_SECTION_BASE_ADDRESS(PhysicalBase) | Attributes;
    PhysicalBase += TT_DESCRIPTOR_SECTION_SIZE;
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
  VOID  *TranslationTable;

  // Allocate pages for translation table.
  TranslationTable = AllocatePages (EFI_SIZE_TO_PAGES (TRANSLATION_TABLE_SIZE + TRANSLATION_TABLE_ALIGNMENT));
  if (TranslationTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  TranslationTable = (VOID *)(((UINTN)TranslationTable + TRANSLATION_TABLE_ALIGNMENT_MASK) & ~TRANSLATION_TABLE_ALIGNMENT_MASK);

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = TranslationTable;
  }

  if (TranslationTableBase != NULL) {
    *TranslationTableSize = TRANSLATION_TABLE_SIZE;
  }

  ZeroMem(TranslationTable, TRANSLATION_TABLE_SIZE);

  ArmCleanInvalidateDataCache();
  ArmInvalidateInstructionCache();
  ArmInvalidateTlb();

  ArmDisableDataCache();
  ArmDisableInstructionCache();
  ArmDisableMmu();

  // Make sure nothing sneaked into the cache
  ArmCleanInvalidateDataCache();
  ArmInvalidateInstructionCache();

  while (MemoryTable->Length != 0) {
    FillTranslationTable(TranslationTable, MemoryTable);
    MemoryTable++;
  }

  ArmSetTTBR0(TranslationTable);

  ArmSetDomainAccessControl(DOMAIN_ACCESS_CONTROL_NONE(15) |
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
