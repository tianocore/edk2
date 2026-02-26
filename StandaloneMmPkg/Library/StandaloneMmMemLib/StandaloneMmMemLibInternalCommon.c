/** @file
  Internal ARCH Specific file of MM memory check library.

  MM memory check library implementation. This library consumes MM_ACCESS_PROTOCOL
  to get MMRAM information. In order to use this library instance, the platform should produce
  all MMRAM range via MM_ACCESS_PROTOCOL, including the range for firmware (like MM Core
  and MM driver) and/or specific dedicated hardware.

  Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "StandaloneMmMemLibInternal.h"
//
// Maximum support address used to check input buffer
//
extern EFI_PHYSICAL_ADDRESS  mMmMemLibInternalMaximumSupportAddress;

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibCalculateMaximumSupportAddress (
  VOID
  )
{
  mMmMemLibInternalMaximumSupportAddress = MAX_ALLOC_ADDRESS;

  DEBUG ((DEBUG_INFO, "mMmMemLibInternalMaximumSupportAddress = 0x%lx\n", mMmMemLibInternalMaximumSupportAddress));
}

/**
  Initialize valid non-Mmram Ranges from Resource HOB.

**/
VOID
MmMemLibInitializeValidNonMmramRanges (
  VOID
  )
{
  // Not implemented for AARCH64.
}

/**
  Deinitialize cached non-Mmram Ranges.

**/
VOID
MmMemLibFreeValidNonMmramRanges (
  VOID
  )
{
  // Not implemented for AARCH64.
}

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
  )
{
  return TRUE;
}
