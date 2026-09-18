/** @file
  Library that builds the DXE and MM memory protection settings HOBs based on a
  QEMU fw_cfg selector.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PlatformMemoryProtectionLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Guid/DxeMemoryProtectionSettings.h>
#include <Guid/MmMemoryProtectionSettings.h>

//
// Name of the QEMU fw_cfg file that selects the memory protection profile.
//
#define MEMORY_PROTECTION_FW_CFG_FILE_NAME  "opt/org.tianocore/MemoryProtection"

/**
  Read the memory protection profile name from the QEMU fw_cfg selector.

  @param[out]  ProfileName  Buffer that receives the NULL-terminated profile
                            name.
  @param[in]   ProfileSize  Size, in bytes, of the ProfileName buffer.

  @retval  EFI_SUCCESS          A profile name was read into ProfileName.
  @retval  EFI_NOT_FOUND        The fw_cfg file is not present.
  @retval  EFI_BAD_BUFFER_SIZE  The fw_cfg contents do not fit in ProfileName.
**/
STATIC
EFI_STATUS
GetMemoryProfileName (
  OUT CHAR8  *ProfileName,
  IN  UINTN  ProfileSize
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  EFI_STATUS            Status;

  Status = QemuFwCfgFindFile (
             MEMORY_PROTECTION_FW_CFG_FILE_NAME,
             &FwCfgItem,
             &FwCfgSize
             );
  if (EFI_ERROR (Status)) {
    //
    // No profile selected via fw_cfg.
    //
    return EFI_NOT_FOUND;
  }

  if ((FwCfgSize == 0) || (FwCfgSize >= ProfileSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid MemoryProtection fw_cfg size %u\n",
      __func__,
      (UINT32)FwCfgSize
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, ProfileName);
  ProfileName[FwCfgSize] = '\0';

  return EFI_SUCCESS;
}

/**
  Build the DXE memory protection settings HOB based on a QEMU fw_cfg selector.

  The fw_cfg file "opt/org.tianocore/MemoryProtection" contains the name of the
  memory protection profile to apply (e.g. "DEBUG" selects
  DXE_MEMORY_PROTECTION_SETTINGS_DEBUG). If the fw_cfg file is not present, no
  HOB is produced and the memory protection PCDs are used as a fallback.
**/
VOID
EFIAPI
PlatformBuildMemoryProtectionSettingsHob (
  VOID
  )
{
  STATIC CONST struct {
    CONST CHAR8                       *Name;
    DXE_MEMORY_PROTECTION_SETTINGS    Settings;
  } Profiles[] = {
    { "DEBUG",            DXE_MEMORY_PROTECTION_SETTINGS_DEBUG                  },
    { "RELEASE",          DXE_MEMORY_PROTECTION_SETTINGS_RELEASE                },
    { "RELEASE.NOGUARDS", DXE_MEMORY_PROTECTION_SETTINGS_RELEASE_NO_PAGE_GUARDS },
    { "OFF",              DXE_MEMORY_PROTECTION_SETTINGS_OFF                    },
  };
  EFI_STATUS  Status;
  CHAR8       ProfileName[32];
  UINTN       Index;

  Status = GetMemoryProfileName (ProfileName, sizeof (ProfileName));
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < ARRAY_SIZE (Profiles); Index++) {
    if (AsciiStrCmp (ProfileName, Profiles[Index].Name) == 0) {
      DEBUG ((
        DEBUG_INFO,
        "%a: applying DXE memory protection profile %a\n",
        __func__,
        ProfileName
        ));
      BuildGuidDataHob (
        &gDxeMemoryProtectionSettingsGuid,
        (VOID *)&Profiles[Index].Settings,
        sizeof (DXE_MEMORY_PROTECTION_SETTINGS)
        );
      return;
    }
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: unknown memory protection profile '%a'\n",
    __func__,
    ProfileName
    ));
}

/**
  Build the MM (SMM / Standalone MM) memory protection settings HOB based on a
  QEMU fw_cfg selector.

  The fw_cfg file "opt/org.tianocore/MemoryProtection" contains the name of the
  memory protection profile to apply (e.g. "DEBUG" selects
  MM_MEMORY_PROTECTION_SETTINGS_DEBUG). If the fw_cfg file is not present, no
  HOB is produced and the memory protection PCDs are used as a fallback.

  The MM memory protection settings define only DEBUG and OFF profiles, so the
  production ("RELEASE"/"RELEASE.NOGUARDS") and "OFF" selectors all
  map to MM_MEMORY_PROTECTION_SETTINGS_OFF.
**/
VOID
EFIAPI
PlatformBuildMmMemoryProtectionSettingsHob (
  VOID
  )
{
  STATIC CONST struct {
    CONST CHAR8                      *Name;
    MM_MEMORY_PROTECTION_SETTINGS    Settings;
  } Profiles[] = {
    { "DEBUG",            MM_MEMORY_PROTECTION_SETTINGS_DEBUG },
    { "RELEASE",          MM_MEMORY_PROTECTION_SETTINGS_OFF   },
    { "RELEASE.NOGUARDS", MM_MEMORY_PROTECTION_SETTINGS_OFF   },
    { "OFF",              MM_MEMORY_PROTECTION_SETTINGS_OFF   },
  };
  EFI_STATUS  Status;
  CHAR8       ProfileName[32];
  UINTN       Index;

  Status = GetMemoryProfileName (ProfileName, sizeof (ProfileName));
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < ARRAY_SIZE (Profiles); Index++) {
    if (AsciiStriCmp (ProfileName, Profiles[Index].Name) == 0) {
      DEBUG ((
        DEBUG_INFO,
        "%a: applying MM memory protection profile %a\n",
        __func__,
        ProfileName
        ));
      BuildGuidDataHob (
        &gMmMemoryProtectionSettingsGuid,
        (VOID *)&Profiles[Index].Settings,
        sizeof (MM_MEMORY_PROTECTION_SETTINGS)
        );
      return;
    }
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: unknown memory protection profile '%a'\n",
    __func__,
    ProfileName
    ));
}
