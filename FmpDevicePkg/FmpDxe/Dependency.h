/** @file
  Fmp Capsule Dependency support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEPENDENCY_H__
#define __DEPENDENCY_H__

#include <Library/UefiLib.h>
#include <Protocol/FirmwareManagement.h>

#define DEPENDENCIES_SATISFIED    0
#define DEPENDENCIES_UNSATISFIED  1
#define DEPENDENCIES_INVALID      2

extern UINT8  mDependenciesCheckStatus;

/**
  Validate the dependency expression and output its size.

  @param[in]   ImageDepex      Pointer to the EFI_FIRMWARE_IMAGE_DEP.
  @param[in]   MaxDepexSize    Max size of the dependency.
  @param[out]  DepexSize       Size of dependency.

  @retval TRUE           The capsule is valid.
  @retval FALSE          The capsule is invalid.

**/
BOOLEAN
ValidateImageDepex (
  IN  EFI_FIRMWARE_IMAGE_DEP             *ImageDepex,
  IN  CONST UINTN                        MaxDepexSize,
  OUT UINT32                             *DepexSize
  );

/**
  Check dependency for firmware update.

  @param[in]   ImageTypeId         Image Type Id.
  @param[in]   Version             New version.
  @param[in]   Dependencies        The dependencies.
  @param[in]   DepexSize           Size of the dependencies
  @param[out]  IsSatisfied         Indicate the dependencies is satisfied or not.

  @retval  EFI_SUCCESS             Dependency Evaluation is successful.
  @retval  Others                  Dependency Evaluation fails with unexpected error.

**/
EFI_STATUS
EvaluateImageDependencies (
  IN CONST EFI_GUID                ImageTypeId,
  IN CONST UINT32                  Version,
  IN CONST EFI_FIRMWARE_IMAGE_DEP  *Dependencies,
  IN CONST UINT32                  DepexSize,
  OUT BOOLEAN                      *IsSatisfied
  );

#endif
