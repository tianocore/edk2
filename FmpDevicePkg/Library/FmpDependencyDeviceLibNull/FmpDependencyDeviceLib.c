/** @file
  Null instance of FmpDependencyDeviceLib.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/FmpDependencyDeviceLib.h>

/**
  Save dependency to Fmp device.

  @param[in]  Depex       Fmp dependency.
  @param[in]  DepexSize   Size, in bytes, of the Fmp dependency.

  @retval  EFI_SUCCESS       Save Fmp dependency succeeds.
  @retval  EFI_UNSUPPORTED   Save Fmp dependency is not supported.
  @retval  Others            Save Fmp dependency fails.

**/
EFI_STATUS
EFIAPI
SaveFmpDependency (
  IN EFI_FIRMWARE_IMAGE_DEP  *Depex,
  IN UINT32                  DepexSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get dependency from the Fmp device.
  This caller is responsible for freeing the dependency buffer.

  @param[out]  DepexSize   Size, in bytes, of the dependency.

  @retval  The pointer to dependency.
  @retval  NULL

**/
EFI_FIRMWARE_IMAGE_DEP *
EFIAPI
GetFmpDependency (
  OUT UINT32  *DepexSize
  )
{
  return NULL;
}
