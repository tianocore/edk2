/** @file

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS          5

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
                                    zero-filled entry. The allocated memory
                                    will not be freed.

**/
VOID
ArmVirtGetMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR   **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINT64                        TopOfMemory;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                                     MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  if (VirtualMemoryTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Error: Failed AllocatePool()\n", __FUNCTION__));
    return;
  }

  // System DRAM
  VirtualMemoryTable[0].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[0].VirtualBase  = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[0].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[0].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  DEBUG ((DEBUG_INFO, "%a: Dumping System DRAM Memory Map:\n"
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
  TopOfMemory = MIN (1ULL << FixedPcdGet8 (PcdPrePiCpuMemorySize),
                     ArmGetPhysAddrTop ());
  VirtualMemoryTable[2].PhysicalBase = VirtualMemoryTable[0].Length + VirtualMemoryTable[1].Length;
  VirtualMemoryTable[2].VirtualBase  = VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[2].Length       = TopOfMemory -
                                       VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[2].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // Remap the FD region as normal executable memory
  VirtualMemoryTable[3].PhysicalBase = PcdGet64 (PcdFdBaseAddress);
  VirtualMemoryTable[3].VirtualBase  = VirtualMemoryTable[3].PhysicalBase;
  VirtualMemoryTable[3].Length       = FixedPcdGet32 (PcdFdSize);
  VirtualMemoryTable[3].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  // End of Table
  ZeroMem (&VirtualMemoryTable[4], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  *VirtualMemoryMap = VirtualMemoryTable;
}
