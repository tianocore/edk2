/** @file
  Parses the fw_cfg file for the DXE and MM memory protection settings profile.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/SetMemoryProtectionsLib.h>

#define DXE_MEMORY_PROTECTION_PROFILE_FWCFG_FILE \
  "opt/org.tianocore/DxeMemoryProtectionProfile"

#define MM_MEMORY_PROTECTION_PROFILE_FWCFG_FILE \
  "opt/org.tianocore/MmMemoryProtectionProfile"

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
  )
{
  CHAR8  String[100];
  UINTN  StringSize;
  UINTN  Index;

  if (MmSettings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringSize = sizeof (String);

  if (!EFI_ERROR (QemuFwCfgParseString (MM_MEMORY_PROTECTION_PROFILE_FWCFG_FILE, &StringSize, String))) {
    Index = 0;
    do {
      if (AsciiStriCmp (MmMemoryProtectionProfiles[Index].Name, String) == 0) {
        DEBUG ((DEBUG_INFO, "Setting MM Memory Protection Profile: %a\n", String));
        break;
      }
    } while (++Index < MmMemoryProtectionSettingsMax);

    if (Index >= MmMemoryProtectionSettingsMax) {
      DEBUG ((DEBUG_ERROR, "Invalid MM memory protection profile: %a\n", String));
      ASSERT (Index < MmMemoryProtectionSettingsMax);
      return EFI_ABORTED;
    } else {
      CopyMem (MmSettings, &MmMemoryProtectionProfiles[Index].Settings, sizeof (MM_MEMORY_PROTECTION_SETTINGS));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

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
  )
{
  CHAR8  String[100];
  UINTN  StringSize;
  UINTN  Index;

  if (DxeSettings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringSize = sizeof (String);

  if (!EFI_ERROR (QemuFwCfgParseString (DXE_MEMORY_PROTECTION_PROFILE_FWCFG_FILE, &StringSize, String))) {
    Index = 0;
    do {
      if (AsciiStriCmp (DxeMemoryProtectionProfiles[Index].Name, String) == 0) {
        DEBUG ((DEBUG_INFO, "Setting DXE Memory Protection Profile: %a\n", String));
        break;
      }
    } while (++Index < DxeMemoryProtectionSettingsMax);

    if (Index >= DxeMemoryProtectionSettingsMax) {
      DEBUG ((DEBUG_ERROR, "Invalid DXE memory protection profile: %a\n", String));
      ASSERT (Index < DxeMemoryProtectionSettingsMax);
      return EFI_ABORTED;
    } else {
      CopyMem (DxeSettings, &DxeMemoryProtectionProfiles[Index].Settings, sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
