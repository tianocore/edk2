/** @file
  Fmp Capsule Dependency support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FMP_DEPENDENCY_LIB__
#define __FMP_DEPENDENCY_LIB__

#include <PiDxe.h>
#include <Protocol/FirmwareManagement.h>

//
// Data struct to store FMP ImageType and version for dependency check.
//
typedef struct {
  EFI_GUID ImageTypeId;
  UINT32   Version;
} FMP_DEPEX_CHECK_VERSION_DATA;

/**
  Validate the dependency expression and output its size.

  @param[in]   Dependencies       Pointer to the EFI_FIRMWARE_IMAGE_DEP.
  @param[in]   MaxDepexSize       Max size of the dependency.
  @param[out]  DepexSize          Size of dependency.
  @param[out]  LastAttemptStatus  An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  If a last attempt status error code is not returned,
                                  this function will not modify the LastAttemptStatus value.

  @retval TRUE    The dependency expression is valid.
  @retval FALSE   The dependency expression is invalid.

**/
BOOLEAN
EFIAPI
ValidateDependency (
  IN  EFI_FIRMWARE_IMAGE_DEP  *Dependencies,
  IN  UINTN                   MaxDepexSize,
  OUT UINT32                  *DepexSize,
  OUT UINT32                  *LastAttemptStatus OPTIONAL
  );

/**
  Get dependency from firmware image.

  @param[in]  Image               Points to the firmware image.
  @param[in]  ImageSize           Size, in bytes, of the firmware image.
  @param[out] DepexSize           Size, in bytes, of the dependency.
  @param[out] LastAttemptStatus   An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  If a last attempt status error code is not returned,
                                  this function will not modify the LastAttemptStatus value.
  @retval  The pointer to dependency.
  @retval  Null

**/
EFI_FIRMWARE_IMAGE_DEP*
EFIAPI
GetImageDependency (
  IN  EFI_FIRMWARE_IMAGE_AUTHENTICATION *Image,
  IN  UINTN                             ImageSize,
  OUT UINT32                            *DepexSize,
  OUT UINT32                            *LastAttemptStatus  OPTIONAL
  );

/**
  Evaluate the dependencies. The caller must search all the Fmp instances and
  gather their versions into FmpVersions parameter. If there is PUSH_GUID opcode
  in dependency expression with no FmpVersions provided, the dependency will
  evaluate to FALSE.

  @param[in]   Dependencies       Dependency expressions.
  @param[in]   DependenciesSize   Size of Dependency expressions.
  @param[in]   FmpVersions        Array of Fmp ImageTypeId and version. This
                                  parameter is optional and can be set to NULL.
  @param[in]   FmpVersionsCount   Element count of the array. When FmpVersions
                                  is NULL, FmpVersionsCount must be 0.
  @param[out]  LastAttemptStatus  An optional pointer to a UINT32 that holds the
                                  last attempt status to report back to the caller.
                                  This function will set the value to LAST_ATTEMPT_STATUS_SUCCESS
                                  if an error code is not set.

  @retval TRUE    Dependency expressions evaluate to TRUE.
  @retval FALSE   Dependency expressions evaluate to FALSE.

**/
BOOLEAN
EFIAPI
EvaluateDependency (
  IN  EFI_FIRMWARE_IMAGE_DEP        *Dependencies,
  IN  UINTN                         DependenciesSize,
  IN  FMP_DEPEX_CHECK_VERSION_DATA  *FmpVersions,      OPTIONAL
  IN  UINTN                         FmpVersionsCount,
  OUT UINT32                        *LastAttemptStatus OPTIONAL
  );

#endif
