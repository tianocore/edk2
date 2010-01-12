/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Chipset/Cortex-A8.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "ArmCortexALib.h"

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
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_BACK;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH:
      Attributes = TT_DESCRIPTOR_SECTION_WRITE_THROUGH;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
      Attributes = TT_DESCRIPTOR_SECTION_DEVICE;
      break;
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
    default:
      Attributes = TT_DESCRIPTOR_SECTION_UNCACHED;
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
  VOID  *TranslationTable;

  // Allocate pages for translation table.
  TranslationTable = AllocatePages(EFI_SIZE_TO_PAGES(TRANSLATION_TABLE_SIZE + TRANSLATION_TABLE_ALIGNMENT));
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

  ArmSetTranslationTableBaseAddress(TranslationTable);
    
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
}

ARM_CACHE_TYPE
EFIAPI
ArmCacheType (
  VOID
  )
{
  return ARM_CACHE_TYPE_WRITE_BACK;
}

ARM_CACHE_ARCHITECTURE
EFIAPI
ArmCacheArchitecture (
  VOID
  )
{
  return ARM_CACHE_ARCHITECTURE_SEPARATE;
}

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  )
{
  return TRUE;
}
  
UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  )
{
  return  16 * 1024;      
}
  
UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  )
{
  return 4;
}
  
UINTN
ArmDataCacheSets (
  VOID
  )
{
  return 64;
}

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  return 64;
}
  
BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  )
{
  return TRUE;
}
  
UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  )
{
  return  16 * 1024;      
}
  
UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  )
{
  return 4;
}
  
UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  return 64;
}

VOID
ArmCortexADataCacheOperation (
  IN  ARM_CORTEX_A_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN     Set;
  UINTN     SetCount;
  UINTN     SetShift;
  UINTN     Way;
  UINTN     WayCount;
  UINTN     WayShift;
  UINT32    SetWayFormat;
  UINTN     SavedInterruptState;

  SetCount = ArmDataCacheSets();
  WayCount = ArmDataCacheAssociativity();

  // Cortex-A8 Manual, System Control Coprocessor chapter
  SetShift = 6;
  WayShift = 32 - LowBitSet32 ((UINT32)WayCount);
  
  SavedInterruptState = ArmDisableInterrupts();
      
  for (Way = 0; Way < WayCount; Way++) {
    for (Set = 0; Set < SetCount; Set++) {      
      // Build the format that the CP15 instruction can understand
      SetWayFormat = (Way << WayShift) | (Set << SetShift);

      // Pass it through
      (*DataCacheOperation)(SetWayFormat);
    }
  }
  
  ArmDrainWriteBuffer();
  
  if (SavedInterruptState) {
    ArmEnableInterrupts();
  }
}

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  )
{
  ArmCortexADataCacheOperation(ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ArmCortexADataCacheOperation(ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ArmCortexADataCacheOperation(ArmCleanDataCacheEntryBySetWay);
}
