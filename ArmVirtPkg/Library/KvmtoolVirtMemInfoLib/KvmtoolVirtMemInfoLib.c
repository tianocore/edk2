/** @file

  Copyright (c) 2018 - 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS          5

#define MAX_REGION_NAME_LEN   32

#define LOG_MEM_MAP_HEADER()                \
  DEBUG ((                                  \
    DEBUG_ERROR,                            \
    "%-*a : %-18a : %-18a - %-18a : %-a\n", \
    MAX_REGION_NAME_LEN,                    \
    "Region",                               \
    "Physical Address",                     \
    "Virtual Address",                      \
    "Size",                                 \
    "Attribute"                             \
    ));

#define LOG_MEM_MAP(Txt)                              \
  DEBUG ((                                            \
    DEBUG_ERROR,                                      \
    "%-*a : 0x%016x : 0x%016x - 0x%016x : 0x%02x\n",  \
    MAX_REGION_NAME_LEN,                              \
    Txt,                                              \
    VirtualMemoryTable[Idx].PhysicalBase,             \
    VirtualMemoryTable[Idx].VirtualBase,              \
    VirtualMemoryTable[Idx].Length,                   \
    VirtualMemoryTable[Idx].Attributes                \
    ));

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
  UINTN                          Idx = 0;
  EFI_PHYSICAL_ADDRESS           TopOfAddressSpace;

  ASSERT (VirtualMemoryMap != NULL);

  TopOfAddressSpace = LShiftU64 (1ULL, ArmGetPhysicalAddressBits ());

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)
                        AllocatePages (
                          EFI_SIZE_TO_PAGES (
                            sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                            MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                            )
                          );
  if (VirtualMemoryTable == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error: Failed to Allocate Pages\n",
      __FUNCTION__
      ));
    return;
  }

  LOG_MEM_MAP_HEADER ();

  // System DRAM
  VirtualMemoryTable[Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  LOG_MEM_MAP ("System DRAM Memory Map");

  // Peripheral space before DRAM
  VirtualMemoryTable[++Idx].PhysicalBase = 0x0;
  VirtualMemoryTable[Idx].VirtualBase    = 0x0;
  VirtualMemoryTable[Idx].Length         = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Idx].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("Peripheral space before DRAM");

  // Peripheral space after DRAM
  VirtualMemoryTable[++Idx].PhysicalBase = PcdGet64 (PcdSystemMemoryBase) +
                                           PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Idx].VirtualBase    = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length         = TopOfAddressSpace -
                                           VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM_MAP ("Peripheral space after DRAM");

  // Map the FV region as normal executable memory
  VirtualMemoryTable[++Idx].PhysicalBase = PcdGet64 (PcdFvBaseAddress);
  VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
  VirtualMemoryTable[Idx].Length       = FixedPcdGet32 (PcdFvSize);
  VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  LOG_MEM_MAP ("FV region");

  // End of Table
  VirtualMemoryTable[++Idx].PhysicalBase  = 0;
  VirtualMemoryTable[Idx].VirtualBase     = 0;
  VirtualMemoryTable[Idx].Length          = 0;
  VirtualMemoryTable[Idx].Attributes      = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT((Idx + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
