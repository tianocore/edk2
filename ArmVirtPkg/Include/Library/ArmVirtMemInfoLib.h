/** @file

  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
  Copyright (c) 2017, Linaro, Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ARM_VIRT_MEMINFO_LIB_H_
#define _ARM_VIRT_MEMINFO_LIB_H_

#include <Base.h>
#include <Library/ArmLib.h>

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
  );

#endif
