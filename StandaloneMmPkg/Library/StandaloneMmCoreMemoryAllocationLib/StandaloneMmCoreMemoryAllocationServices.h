/** @file
  Contains function prototypes for Memory Services in the MM Core.

  This header file borrows the StandaloneMmCore Memory Allocation services as the primitive
  for memory allocation.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PI_MM_CORE_MEMORY_ALLOCATION_SERVICES_H_
#define _PI_MM_CORE_MEMORY_ALLOCATION_SERVICES_H_

#include <Guid/MmCoreData.h>

/**
  Called to initialize the memory service.

  @param   MmramRangeCount       Number of MMRAM Regions
  @param   MmramRanges           Pointer to MMRAM Descriptors

**/
VOID
MmInitializeMemoryServices (
  IN UINTN                 MmramRangeCount,
  IN EFI_MMRAM_DESCRIPTOR  *MmramRanges
  );

#endif
