/** @file
NULL implementation for GetMemoryProtectionsLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/GetMemoryProtectionsLib.h>

MEMORY_PROTECTION_SETTINGS_UNION  gMps = { 0 };

/**
  Populates gMps global. This function is invoked by the library constructor and only needs to be
  called if library contructors have not yet been invoked.

  @retval EFI_SUCCESS       gMps global was populated.
  @retval EFI_NOT_FOUND     The gMemoryProtectionSettingsGuid HOB was not found.
  @retval EFI_ABORTED       The version number of the DXE or MM memory protection settings was invalid.
  @retval EFI_UNSUPPORTED   NULL implementation called.
**/
EFI_STATUS
EFIAPI
PopulateMpsGlobal (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
