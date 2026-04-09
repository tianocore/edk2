/** @file

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

STATIC ARM_MEMORY_REGION_DESCRIPTOR  mVirtualMemoryTable[2];

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
EFIAPI
ArmVirtGetMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  EFI_PHYSICAL_ADDRESS  TopOfAddressSpace;

  ASSERT (VirtualMemoryMap != NULL);

  TopOfAddressSpace = LShiftU64 (1ULL, ArmGetPhysicalAddressBits ());

  //
  // Map the entire physical memory space as cached. The only device
  // we care about is the GIC, which will be stage 2 mapped as a device
  // by the hypervisor, overriding the cached mapping we install here.
  //
  mVirtualMemoryTable[0].PhysicalBase = 0x0;
  mVirtualMemoryTable[0].VirtualBase  = 0x0;
  mVirtualMemoryTable[0].Length       = TopOfAddressSpace;
  mVirtualMemoryTable[0].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  mVirtualMemoryTable[1].PhysicalBase = 0x0;
  mVirtualMemoryTable[1].VirtualBase  = 0x0;
  mVirtualMemoryTable[1].Length       = 0x0;
  mVirtualMemoryTable[1].Attributes   = 0x0;

  *VirtualMemoryMap = mVirtualMemoryTable;
}
