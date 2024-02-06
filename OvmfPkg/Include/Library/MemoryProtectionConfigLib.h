/** @file
  Parses the fw_cfg file for the DXE and MM memory protection settings profile.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MEMORY_PROTECTION_CONFIG_LIB_H_
#define MEMORY_PROTECTION_CONFIG_LIB_H_

#include <Uefi.h>

#include <Library/SetMemoryProtectionsLib.h>

/**
  Parses the fw_cfg file for the MM memory protection settings profile.

  @param[in] MmSettings  The MM memory protection settings profile to populate.

  @retval EFI_SUCCESS             The MM memory protection settings profile was populated.
  @retval EFI_INVALID_PARAMETER   MmSettings is NULL.
  @retval EFI_ABORTED             The MM memory protection settings profile name found in
                                  fw_cfg was invalid.
  @retval EFI_NOT_FOUND           The MM memory protection settings profile was not found.
**/
EFI_STATUS
EFIAPI
ParseFwCfgMmMemoryProtectionSettings (
  IN MM_MEMORY_PROTECTION_SETTINGS  *MmSettings
  );

/**
  Parses the fw_cfg file for the DXE memory protection settings profile.

  @param[in] DxeSettings  The DXE memory protection settings profile to populate.

  @retval EFI_SUCCESS             The DXE memory protection settings profile was populated.
  @retval EFI_INVALID_PARAMETER   DxeSettings is NULL.
  @retval EFI_ABORTED             The DXE memory protection settings profile name found in
                                  fw_cfg was invalid.
  @retval EFI_NOT_FOUND           The DXE memory protection settings profile was not found.
**/
EFI_STATUS
EFIAPI
ParseFwCfgDxeMemoryProtectionSettings (
  IN DXE_MEMORY_PROTECTION_SETTINGS  *DxeSettings
  );

#endif
