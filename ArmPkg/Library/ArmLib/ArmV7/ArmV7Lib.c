/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Chipset/ArmV7.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
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
  UINT32 CLIDR = ReadCLIDR ();

  return CLIDR; // BugBug Fix Me
}

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  )
{
  UINT32 CLIDR = ReadCLIDR ();
  
  if ((CLIDR & 0x2) == 0x2) {
    // Instruction cache exists
    return TRUE;
  }
  if ((CLIDR & 0x7) == 0x4) {
    // Unified cache
    return TRUE;
  }
  
  return FALSE;
}
  
UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  )
{
  UINT32 NumSets;
  UINT32 Associativity;
  UINT32 LineSize;
  UINT32 CCSIDR = ReadCCSIDR (0);
  
  LineSize      = (1 << (CCSIDR + 2));
  Associativity = ((CCSIDR >> 3) & 0x3ff) + 1;
  NumSets       = ((CCSIDR >> 13) & 0x7fff) + 1;

  // LineSize is in words (4 byte chunks)
  return  NumSets * Associativity * LineSize * 4;      
}
  
UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (0);

  return ((CCSIDR >> 3) & 0x3ff) + 1;
}
  
UINTN
ArmDataCacheSets (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (0);
  
  return ((CCSIDR >> 13) & 0x7fff) + 1;
}

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (0) & 7;

  // * 4 converts to bytes
  return (1 << (CCSIDR + 2)) * 4;
}
  
BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  )
{
  UINT32 CLIDR = ReadCLIDR ();
  
  if ((CLIDR & 1) == 1) {
    // Instruction cache exists
    return TRUE;
  }
  if ((CLIDR & 0x7) == 0x4) {
    // Unified cache
    return TRUE;
  }
  
  return FALSE;
}
  
UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  )
{
  UINT32 NumSets;
  UINT32 Associativity;
  UINT32 LineSize;
  UINT32 CCSIDR = ReadCCSIDR (1);
  
  LineSize      = (1 << (CCSIDR + 2));
  Associativity = ((CCSIDR >> 3) & 0x3ff) + 1;
  NumSets       = ((CCSIDR >> 13) & 0x7fff) + 1;

  // LineSize is in words (4 byte chunks)
  return  NumSets * Associativity * LineSize * 4;      
}
  
UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (1);

  return ((CCSIDR >> 3) & 0x3ff) + 1;
//  return 4;
}
  
UINTN
EFIAPI
ArmInstructionCacheSets (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (1);
  
  return ((CCSIDR >> 13) & 0x7fff) + 1;
}

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (1) & 7;

  // * 4 converts to bytes
  return (1 << (CCSIDR + 2)) * 4;

//  return 64;
}


VOID
ArmV7DataCacheOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN     SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();

  ArmV7AllDataCachesOperation (DataCacheOperation);
  
  ArmDrainWriteBuffer ();
  
  if (SavedInterruptState) {
    ArmEnableInterrupts ();
  }
}

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  )
{
  ArmV7DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ArmV7DataCacheOperation (ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ArmV7DataCacheOperation (ArmCleanDataCacheEntryBySetWay);
}
