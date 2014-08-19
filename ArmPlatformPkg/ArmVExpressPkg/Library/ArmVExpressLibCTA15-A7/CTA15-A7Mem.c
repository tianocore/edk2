/** @file
*
*  Copyright (c) 2012, ARM Limited. All rights reserved.
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
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <ArmPlatform.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS 14

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
  UINTN                         Index = 0;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
    return;
  }

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

#ifdef ARM_BIGLITTLE_TC2
  // Secure NOR0 Flash
  VirtualMemoryTable[Index].PhysicalBase    = ARM_VE_SEC_NOR0_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SEC_NOR0_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_SEC_NOR0_SZ;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  // Secure RAM
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SEC_RAM0_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SEC_RAM0_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_SEC_RAM0_SZ;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
#endif

  // SMB CS0 - NOR0 Flash
  VirtualMemoryTable[Index].PhysicalBase    = ARM_VE_SMB_NOR0_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_NOR0_BASE;
  VirtualMemoryTable[Index].Length          = SIZE_256KB * 255;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  // Environment Variables region
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SMB_NOR0_BASE + (SIZE_256KB * 255);
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_NOR0_BASE + (SIZE_256KB * 255);
  VirtualMemoryTable[Index].Length          = SIZE_64KB * 4;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS1 or CS4 - NOR1 Flash
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SMB_NOR1_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_NOR1_BASE;
  VirtualMemoryTable[Index].Length          = SIZE_256KB * 255;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  // Environment Variables region
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SMB_NOR1_BASE + (SIZE_256KB * 255);
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_NOR1_BASE + (SIZE_256KB * 255);
  VirtualMemoryTable[Index].Length          = SIZE_64KB * 4;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SMB CS3 or CS1 - PSRAM
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_SRAM_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_SMB_SRAM_SZ;
  VirtualMemoryTable[Index].Attributes      = CacheAttributes;

  // Motherboard peripherals
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_SMB_PERIPH_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_SMB_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

#ifdef ARM_BIGLITTLE_TC2
  // Non-secure ROM
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_TC2_NON_SECURE_ROM_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_TC2_NON_SECURE_ROM_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_TC2_NON_SECURE_ROM_SZ;
  VirtualMemoryTable[Index].Attributes      = CacheAttributes;
#endif

  // OnChip peripherals
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_ONCHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_ONCHIP_PERIPH_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_ONCHIP_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // SCC Region
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_CTA15A7_SCC_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_CTA15A7_SCC_BASE;
  VirtualMemoryTable[Index].Length          = SIZE_64KB;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

#ifdef ARM_BIGLITTLE_TC2
  // TC2 OnChip non-secure SRAM
  VirtualMemoryTable[++Index].PhysicalBase  = ARM_VE_TC2_NON_SECURE_SRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase     = ARM_VE_TC2_NON_SECURE_SRAM_BASE;
  VirtualMemoryTable[Index].Length          = ARM_VE_TC2_NON_SECURE_SRAM_SZ;
  VirtualMemoryTable[Index].Attributes      = CacheAttributes;
#endif

#ifndef ARM_BIGLITTLE_TC2
  // Workaround for SRAM bug in RTSM
  if (PcdGet64 (PcdSystemMemoryBase) != 0x80000000) {
    VirtualMemoryTable[++Index].PhysicalBase  = 0x80000000;
    VirtualMemoryTable[Index].VirtualBase     = 0x80000000;
    VirtualMemoryTable[Index].Length          = PcdGet64 (PcdSystemMemoryBase) - 0x80000000;
    VirtualMemoryTable[Index].Attributes      = CacheAttributes;
  }
#endif

  // DDR
  VirtualMemoryTable[++Index].PhysicalBase  = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase     = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length          = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes      = CacheAttributes;

  // Detect if it is a 1GB or 2GB Test Chip
  //   [16:19]: 0=1GB TC2, 1=2GB TC2
  if (MmioRead32(ARM_VE_SYS_PROCID0_REG) & (0xF << 16)) {
    DEBUG((EFI_D_ERROR,"Info: 2GB Test Chip 2 detected.\n"));
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
          EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE | EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
          EFI_RESOURCE_ATTRIBUTE_TESTED,
        PcdGet64 (PcdSystemMemoryBase) + PcdGet64 (PcdSystemMemorySize),
        SIZE_1GB
    );

    // Map the additional 1GB into the MMU
    VirtualMemoryTable[++Index].PhysicalBase  = PcdGet64 (PcdSystemMemoryBase) + PcdGet64 (PcdSystemMemorySize);
    VirtualMemoryTable[Index].VirtualBase     = PcdGet64 (PcdSystemMemoryBase) + PcdGet64 (PcdSystemMemorySize);
    VirtualMemoryTable[Index].Length          = SIZE_1GB;
    VirtualMemoryTable[Index].Attributes      = CacheAttributes;
  }

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase  = 0;
  VirtualMemoryTable[Index].VirtualBase     = 0;
  VirtualMemoryTable[Index].Length          = 0;
  VirtualMemoryTable[Index].Attributes      = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
