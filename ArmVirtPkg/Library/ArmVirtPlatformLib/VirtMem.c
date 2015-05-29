/** @file
*
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
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
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmPlatformGlobalVariableLib.h>
#include <ArmPlatform.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS          4

// DDR attributes
#define DDR_ATTRIBUTES_CACHED    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED  ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

EFI_PHYSICAL_ADDRESS
ArmGetPhysAddrTop (
  VOID
  );

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePages (
                         EFI_SIZE_TO_PAGES (
                           sizeof (ARM_MEMORY_REGION_DESCRIPTOR)
                           * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                           )
                         );

  if (VirtualMemoryTable == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: Error: Failed AllocatePages()\n", __FUNCTION__));
    return;
  }

  if (FeaturePcdGet (PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  // System DRAM
  VirtualMemoryTable[0].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[0].VirtualBase  = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[0].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[0].Attributes   = CacheAttributes;

  DEBUG ((EFI_D_INFO, "%a: Dumping System DRAM Memory Map:\n"
      "\tPhysicalBase: 0x%lX\n"
      "\tVirtualBase: 0x%lX\n"
      "\tLength: 0x%lX\n",
      __FUNCTION__,
      VirtualMemoryTable[0].PhysicalBase,
      VirtualMemoryTable[0].VirtualBase,
      VirtualMemoryTable[0].Length));

  // Peripheral space before DRAM
  VirtualMemoryTable[1].PhysicalBase = 0x0;
  VirtualMemoryTable[1].VirtualBase  = 0x0;
  VirtualMemoryTable[1].Length       = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[1].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // Peripheral space after DRAM
  VirtualMemoryTable[2].PhysicalBase = VirtualMemoryTable[0].Length + VirtualMemoryTable[1].Length;
  VirtualMemoryTable[2].VirtualBase  = VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[2].Length       = ArmGetPhysAddrTop () - VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[2].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // End of Table
  ZeroMem (&VirtualMemoryTable[3], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  *VirtualMemoryMap = VirtualMemoryTable;
}
