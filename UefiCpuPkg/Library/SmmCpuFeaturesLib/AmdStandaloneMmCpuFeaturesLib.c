/** @file
Standalone MM CPU specific programming for AMD based platforms.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include "CpuFeaturesLib.h"

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
