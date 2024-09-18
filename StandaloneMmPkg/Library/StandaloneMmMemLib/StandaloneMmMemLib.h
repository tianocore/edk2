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

#endif
