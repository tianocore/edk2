/** @file
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

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>

#include <ArmPlatform.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS          6

// DDR attributes
#define DDR_ATTRIBUTES_CACHED           ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED         ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
  UINT32                        CacheAttributes;
  UINTN                         Index = 0;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

  ASSERT(VirtualMemoryMap != NULL);

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages (EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
      return;
  }

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  // ReMap (Either NOR Flash or DRAM)
  VirtualMemoryTable[Index].PhysicalBase = ARM_EB_REMAP_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_EB_REMAP_BASE;
  VirtualMemoryTable[Index].Length       = ARM_EB_REMAP_SZ;
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

  // DDR
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase  = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

  // SMC CS7
  VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].Length       = ARM_EB_SMB_MB_ON_CHIP_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS0-CS1 - NOR Flash 1 & 2
  VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_NOR_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_NOR_BASE;
  VirtualMemoryTable[Index].Length       = ARM_EB_SMB_NOR_SZ + ARM_EB_SMB_DOC_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS2 - SRAM
  VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].Length       = ARM_EB_SMB_SRAM_SZ;
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

  // SMB CS3-CS6 - Motherboard Peripherals
  VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].Length       = ARM_EB_SMB_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // If a Logic Tile is connected to The ARM Versatile Express Motherboard
  if (MmioRead32(ARM_EB_SYS_PROCID1_REG) != 0) {
      VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_LOGIC_TILE_BASE;
      VirtualMemoryTable[Index].VirtualBase  = ARM_EB_LOGIC_TILE_BASE;
      VirtualMemoryTable[Index].Length       = ARM_EB_LOGIC_TILE_SZ;
      VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

      ASSERT((Index + 1) == (MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS + 1));
  } else {
    ASSERT((Index + 1) == MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);
  }

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase  = 0;
  VirtualMemoryTable[Index].Length       = 0;
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  *VirtualMemoryMap = VirtualMemoryTable;
}
