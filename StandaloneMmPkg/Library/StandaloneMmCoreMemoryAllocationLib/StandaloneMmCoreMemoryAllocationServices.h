/** @file
  Contains function prototypes for Memory Services in the MM Core.

  This header file borrows the StandaloneMmCore Memory Allocation services as the primitive
  for memory allocation.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
