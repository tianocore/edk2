/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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
#include "ArmV7Lib.h"
#include "ArmLibPrivate.h"

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
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(0);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_BACK:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE(1);
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_SECURE_UNCACHED_UNBUFFERED:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(1);
      break;
    default:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED(0);
      break;
  }
  
  Entry    = TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS(TranslationTable, MemoryRegion->VirtualBase);
  Sections = MemoryRegion->Length / TT_DESCRIPTOR_SECTION_SIZE;
  
  for (Index = 0; Index < Sections; Index++) {
    *Entry++     =  TT_DESCRIPTOR_SECTION_BASE_ADDRESS(PhysicalBase) | Attributes;
    PhysicalBase += TT_DESCRIPTOR_SECTION_SIZE;
  }
}

VOID
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  )
{
  UINTN                         TranslationTable;
  ARM_MEMORY_REGION_ATTRIBUTES  TranslationTableAttribute;
  UINT32                        TTBRAttributes;

  // Allocate pages for translation table.
  TranslationTable = (UINTN)AllocatePages(EFI_SIZE_TO_PAGES(TRANSLATION_TABLE_SECTION_SIZE + TRANSLATION_TABLE_SECTION_ALIGNMENT));
  TranslationTable = ((UINTN)TranslationTable + TRANSLATION_TABLE_SECTION_ALIGNMENT_MASK) & ~TRANSLATION_TABLE_SECTION_ALIGNMENT_MASK;

  if (TranslationTableBase != NULL) {
    *TranslationTableBase = (VOID *)TranslationTable;
  }
  
  if (TranslationTableBase != NULL) {
    *TranslationTableSize = TRANSLATION_TABLE_SECTION_SIZE;
  }

  ZeroMem ((VOID *)TranslationTable, TRANSLATION_TABLE_SECTION_SIZE);

  ArmCleanInvalidateDataCache();
  ArmInvalidateInstructionCache();
  ArmInvalidateTlb();

  ArmDisableDataCache();
  ArmDisableInstructionCache();
  ArmDisableMmu();

  // Make sure nothing sneaked into the cache
  ArmCleanInvalidateDataCache();
  ArmInvalidateInstructionCache();

  TranslationTableAttribute = 0;
  while (MemoryTable->Length != 0) {
    // Find the memory attribute for the Translation Table
    if ((TranslationTable >= MemoryTable->PhysicalBase) && (TranslationTable < MemoryTable->PhysicalBase + MemoryTable->Length)) {
      TranslationTableAttribute = MemoryTable->Attributes;
    }

    FillTranslationTable ((VOID *)TranslationTable, MemoryTable);
    MemoryTable++;
  }

  // Translate the Memory Attributes into Translation Table Register Attributes
  if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED) || 
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_SECURE_UNCACHED_UNBUFFERED)) {
    TTBRAttributes = TTBR_NON_CACHEABLE;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) || 
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_BACK)) {
    TTBRAttributes = TTBR_WRITE_BACK_ALLOC;
  } else if ((TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH) || 
      (TranslationTableAttribute == ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_THROUGH)) {
    TTBRAttributes = TTBR_WRITE_THROUGH_NO_ALLOC;
  } else {
    //TODO: We should raise an error here
    TTBRAttributes = TTBR_NON_CACHEABLE;
  }

  ArmSetTTBR0 ((VOID *)(UINTN)((TranslationTable & 0xFFFFC000) | (TTBRAttributes & 0x7F)));
    
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
}
