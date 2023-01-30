/** @file
  Internal ARCH Specific file of MM memory check library.

  MM memory check library implementation. This library consumes MM_ACCESS_PROTOCOL
  to get MMRAM information. In order to use this library instance, the platform should produce
  all MMRAM range via MM_ACCESS_PROTOCOL, including the range for firmware (like MM Core
  and MM driver) and/or specific dedicated hardware.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
//
// Maximum support address used to check input buffer
//
extern EFI_PHYSICAL_ADDRESS  mMmMemLibInternalMaximumSupportAddress;

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibInternalCalculateMaximumSupportAddress (
  VOID
  )
{
  mMmMemLibInternalMaximumSupportAddress = MAX_ALLOC_ADDRESS;

  DEBUG ((DEBUG_INFO, "mMmMemLibInternalMaximumSupportAddress = 0x%lx\n", mMmMemLibInternalMaximumSupportAddress));
}

/**
  Initialize cached Mmram Ranges from HOB.

  @retval EFI_UNSUPPORTED   The routine is unable to extract MMRAM information.
  @retval EFI_SUCCESS       MmRanges are populated successfully.

**/
EFI_STATUS
MmMemLibInternalPopulateMmramRanges (
  VOID
  )
{
  // Not implemented for AARCH64.
  return EFI_SUCCESS;
}

/**
  Deinitialize cached Mmram Ranges.

**/
VOID
MmMemLibInternalFreeMmramRanges (
  VOID
  )
{
  // Not implemented for AARCH64.
}
