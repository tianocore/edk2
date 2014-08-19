/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ArmPlatform.h>

// Number of Virtual Memory Map Descriptors
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
  ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
  EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;
  UINTN                         Index = 0;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINT32                        SysId;
  BOOLEAN                       HasSparseMemory;
  EFI_VIRTUAL_ADDRESS           SparseMemoryBase;
  UINT64                        SparseMemorySize;

  ASSERT (VirtualMemoryMap != NULL);

  // The FVP model has Sparse memory
  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId != ARM_RTSM_SYS_ID) {
    HasSparseMemory = TRUE;

    ResourceAttributes =
        EFI_RESOURCE_ATTRIBUTE_PRESENT |
        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
        EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_TESTED;

    // Declared the additional DRAM from 2GB to 4GB
    SparseMemoryBase = 0x0880000000;
    SparseMemorySize = SIZE_2GB;

    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        ResourceAttributes,
        SparseMemoryBase,
        SparseMemorySize);
  } else {
    HasSparseMemory = FALSE;
    SparseMemoryBase = 0x0;
    SparseMemorySize = 0x0;
  }

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
      return;
  }

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
      CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
      CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  // ReMap (Either NOR Flash or DRAM)
  VirtualMemoryTable[Index].PhysicalBase = ARM_VE_REMAP_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_REMAP_BASE;
  VirtualMemoryTable[Index].Length       = ARM_VE_REMAP_SZ;

  if (FeaturePcdGet(PcdNorFlashRemapping) == FALSE) {
    // Map the NOR Flash as Secure Memory
    if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
      VirtualMemoryTable[Index].Attributes   = DDR_ATTRIBUTES_CACHED;
    } else {
      VirtualMemoryTable[Index].Attributes   = DDR_ATTRIBUTES_UNCACHED;
    }
  } else {
    // DRAM mapping
    VirtualMemoryTable[Index].Attributes   = CacheAttributes;
  }

  // DDR
  VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_DRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_DRAM_BASE;
  VirtualMemoryTable[Index].Length       = ARM_VE_DRAM_SZ;
  VirtualMemoryTable[Index].Attributes   = CacheAttributes;

  // CPU peripherals. TRM. Manual says not all of them are implemented.
  VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_ON_CHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_ON_CHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].Length       = ARM_VE_ON_CHIP_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS0-CS1 - NOR Flash 1 & 2
  VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_NOR0_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_NOR0_BASE;
  VirtualMemoryTable[Index].Length       = ARM_VE_SMB_NOR0_SZ + ARM_VE_SMB_NOR1_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS2 - SRAM
  VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].Length       = ARM_VE_SMB_SRAM_SZ;
  VirtualMemoryTable[Index].Attributes   = CacheAttributes;

  // Peripheral CS2 and CS3
  VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].Length       = 2 * ARM_VE_SMB_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // Map sparse memory region if present
  if (HasSparseMemory) {
    VirtualMemoryTable[++Index].PhysicalBase = SparseMemoryBase;
    VirtualMemoryTable[Index].VirtualBase    = SparseMemoryBase;
    VirtualMemoryTable[Index].Length         = SparseMemorySize;
    VirtualMemoryTable[Index].Attributes     = CacheAttributes;
  }

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase  = 0;
  VirtualMemoryTable[Index].Length       = 0;
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  *VirtualMemoryMap = VirtualMemoryTable;
}
