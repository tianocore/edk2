/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/ArmLib.h>
#include <Library/PrePiLib.h>
#include <Library/PcdLib.h>

// DDR attributes
#define DDR_ATTRIBUTES_CACHED                ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED              ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

// SoC registers. L3 interconnects
#define SOC_REGISTERS_L3_PHYSICAL_BASE       0x68000000
#define SOC_REGISTERS_L3_PHYSICAL_LENGTH     0x08000000
#define SOC_REGISTERS_L3_ATTRIBUTES          ARM_MEMORY_REGION_ATTRIBUTE_DEVICE

// SoC registers. L4 interconnects
#define SOC_REGISTERS_L4_PHYSICAL_BASE       0x48000000
#define SOC_REGISTERS_L4_PHYSICAL_LENGTH     0x08000000
#define SOC_REGISTERS_L4_ATTRIBUTES          ARM_MEMORY_REGION_ATTRIBUTE_DEVICE

VOID
InitCache (
  IN  UINT32  MemoryBase,
  IN  UINT32  MemoryLength
  )
{
  UINT32                        CacheAttributes;
  ARM_MEMORY_REGION_DESCRIPTOR  MemoryTable[5];
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  // DDR
  MemoryTable[0].PhysicalBase = MemoryBase;
  MemoryTable[0].VirtualBase  = MemoryBase;
  MemoryTable[0].Length       = MemoryLength;
  MemoryTable[0].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

  // SOC Registers. L3 interconnects
  MemoryTable[1].PhysicalBase = SOC_REGISTERS_L3_PHYSICAL_BASE;
  MemoryTable[1].VirtualBase  = SOC_REGISTERS_L3_PHYSICAL_BASE;
  MemoryTable[1].Length       = SOC_REGISTERS_L3_PHYSICAL_LENGTH;
  MemoryTable[1].Attributes   = SOC_REGISTERS_L3_ATTRIBUTES;

  // SOC Registers. L4 interconnects
  MemoryTable[2].PhysicalBase = SOC_REGISTERS_L4_PHYSICAL_BASE;
  MemoryTable[2].VirtualBase  = SOC_REGISTERS_L4_PHYSICAL_BASE;
  MemoryTable[2].Length       = SOC_REGISTERS_L4_PHYSICAL_LENGTH;
  MemoryTable[2].Attributes   = SOC_REGISTERS_L4_ATTRIBUTES;

  // End of Table
  MemoryTable[3].PhysicalBase = 0;
  MemoryTable[3].VirtualBase  = 0;
  MemoryTable[3].Length       = 0;
  MemoryTable[3].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ArmConfigureMmu (MemoryTable, &TranslationTableBase, &TranslationTableSize);

  BuildMemoryAllocationHob((EFI_PHYSICAL_ADDRESS)(UINTN)TranslationTableBase, TranslationTableSize, EfiBootServicesData);
}
