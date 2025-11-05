/** @file
Standalone MM CPU specific programming.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/PcdLib.h>
#include "CpuFeaturesLib.h"

/**
  Gets the maximum number of logical processors from the PCD PcdCpuMaxLogicalProcessorNumber.

  This access is abstracted from the PCD services to enforce that the PCD be
  FixedAtBuild in the Standalone MM build of this driver.

  @return  The value of PcdCpuMaxLogicalProcessorNumber.

**/
UINT32
GetCpuMaxLogicalProcessorNumber (
  VOID
  )
{
  return FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber);
}

/**
  The Standalone MM library constructor.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCCESS     This constructor always returns success.

**/
EFI_STATUS
EFIAPI
StandaloneMmCpuFeaturesLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  CpuFeaturesLibInitialization ();

  return EFI_SUCCESS;
}
