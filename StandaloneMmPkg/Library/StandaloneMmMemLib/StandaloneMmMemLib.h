/** @file
  Internal header for StandaloneMmMemLib.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef STANDALONE_MM_MEM_LIB_H_
#define STANDALONE_MM_MEM_LIB_H_

#include <PiMm.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Length;
} NON_MM_MEMORY_MAP;

#define NEXT_NON_MM_MEMORY_MAP(MemoryDescriptor, Size) \
  ((NON_MM_MEMORY_MAP *)((UINT8 *)(MemoryDescriptor) + (Size)))

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibInternalCalculateMaximumSupportAddress (
  VOID
  );

/**
  Initialize cached Mmram Ranges from HOB.

  @retval EFI_UNSUPPORTED   The routine is unable to extract MMRAM information.
  @retval EFI_SUCCESS       MmRanges are populated successfully.

**/
EFI_STATUS
MmMemLibInternalPopulateMmramRanges (
  VOID
  );

/**
  Deinitialize cached Mmram Ranges.

**/
VOID
MmMemLibInternalFreeMmramRanges (
  VOID
  );

/**
  Initialize valid non-Mmram Ranges from Resource HOB.

**/
VOID
MmMemLibInternalPopulateValidNonMmramRanges (
  VOID
  );

/**
  Deinitialize cached non-Mmram Ranges.

**/
VOID
MmMemLibInternalFreeNonMmramRanges (
  VOID
  );

/**
  This function check if the buffer is valid non-MMRAM memory range.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid non-MMRAM memory range.
  @retval FALSE This buffer is not valid non-MMRAM memory range.
**/
BOOLEAN
MmMemLibInternalIsValidNonMmramRange (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

#endif
