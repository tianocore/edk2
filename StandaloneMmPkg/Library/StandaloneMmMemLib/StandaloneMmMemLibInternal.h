/** @file
  Internal header for StandaloneMmMemLib.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef STANDALONE_MM_MEM_LIB_INTERNAL_H_
#define STANDALONE_MM_MEM_LIB_INTERNAL_H_

#include <PiMm.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibCalculateMaximumSupportAddress (
  VOID
  );

/**
  Initialize valid non-Mmram Ranges from Resource HOB.

**/
VOID
MmMemLibInitializeValidNonMmramRanges (
  VOID
  );

/**
  Deinitialize cached non-Mmram Ranges.

**/
VOID
MmMemLibFreeValidNonMmramRanges (
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
MmMemLibIsValidNonMmramRange (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

#endif
