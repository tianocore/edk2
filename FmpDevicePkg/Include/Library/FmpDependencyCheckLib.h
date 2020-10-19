/** @file
  Fmp Capsule Dependency check functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FMP_DEPENDENCY_CHECK_LIB__
#define __FMP_DEPENDENCY_CHECK_LIB__

#include <PiDxe.h>
#include <Protocol/FirmwareManagement.h>

/**
  Check dependency for firmware update.

  @param[in]  ImageTypeId        Image Type Id.
  @param[in]  Version            New version.
  @param[in]  Dependencies       Fmp dependency.
  @param[in]  DependenciesSize   Size, in bytes, of the Fmp dependency.
  @param[out] LastAttemptStatus  An optional pointer to a UINT32 that holds the
                                 last attempt status to report back to the caller.
                                 This function will set the value to LAST_ATTEMPT_STATUS_SUCCESS
                                 if an error code is not set.

  @retval  TRUE    Dependencies are satisfied.
  @retval  FALSE   Dependencies are unsatisfied or dependency check fails.

**/
BOOLEAN
EFIAPI
CheckFmpDependency (
  IN  EFI_GUID                ImageTypeId,
  IN  UINT32                  Version,
  IN  EFI_FIRMWARE_IMAGE_DEP  *Dependencies,    OPTIONAL
  IN  UINT32                  DependenciesSize,
  OUT UINT32                  *LastAttemptStatus OPTIONAL
  );

#endif
