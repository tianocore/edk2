/** @file

 Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Library/NorFlashPlatformLib.h>

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                 *Count
  )
{
  *NorFlashDescriptions = NULL;
  *Count                = 0;
  return EFI_SUCCESS;
}
