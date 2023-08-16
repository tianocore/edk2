/** @file
Library for creating the MM and DXE memory protection HOB entries.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SET_MEMORY_PROTECTION_SETTINGS_LIB_H_
#define SET_MEMORY_PROTECTION_SETTINGS_LIB_H_

#include <Guid/MemoryProtectionSettings.h>

typedef struct {
  CHAR8                             *Name;
  CHAR8                             *Description;
  DXE_MEMORY_PROTECTION_SETTINGS    Settings;
} DXE_MEMORY_PROTECTION_PROFILES;

typedef enum {
  DxeMemoryProtectionSettingsDebug = 0,
  DxeMemoryProtectionSettingsRelease,
  DxeMemoryProtectionSettingsReleaseNoPageGuards,
  DxeMemoryProtectionSettingsOff,
  DxeMemoryProtectionSettingsPcd,
  DxeMemoryProtectionSettingsMax
} DXE_MEMORY_PROTECTION_PROFILE_INDEX;

typedef struct {
  CHAR8                            *Name;
  CHAR8                            *Description;
  MM_MEMORY_PROTECTION_SETTINGS    Settings;
} MM_MEMORY_PROTECTION_PROFILES;

typedef enum {
  MmMemoryProtectionSettingsDebug = 0,
  MmMemoryProtectionSettingsRelease,
  MmMemoryProtectionSettingsOff,
  MmMemoryProtectionSettingsPcd,
  MmMemoryProtectionSettingsMax
} MM_MEMORY_PROTECTION_PROFILE_INDEX;

extern DXE_MEMORY_PROTECTION_PROFILES  DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsMax];
extern MM_MEMORY_PROTECTION_PROFILES   MmMemoryProtectionProfiles[MmMemoryProtectionSettingsMax];

/**
  Prevent further changes to the memory protection settings via this
  library API.

  @retval EFI_SUCCESS           The memory protection settings are locked.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
LockMemoryProtectionSettings (
  VOID
  );

/**
  Sets the DXE memory protection settings. If DxeMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] DxeMps        Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if DxeMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input DxeMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetDxeMemoryProtectionSettings (
  IN DXE_MEMORY_PROTECTION_SETTINGS       *DxeMps OPTIONAL,
  IN DXE_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  );

/**
  Sets the MM memory protection HOB entry. If MmMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] MmMps         Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if MmMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to create the HOB.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input MmMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetMmMemoryProtectionSettings (
  IN MM_MEMORY_PROTECTION_SETTINGS       *MmMps OPTIONAL,
  IN MM_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  );

/**
  Copies the current memory protection settings into the input buffer.

  NOTE: The returned settings may not be the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @param[out] Mps  The memory protection settings pointer to populate.

  @retval EFI_SUCCESS           The memory protection settings were copied
                                into the input buffer.
  @retval EFI_INVALID_PARAMETER Mps was NULL.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
GetCurrentMemoryProtectionSettings (
  OUT MEMORY_PROTECTION_SETTINGS  *Mps
  );

/**
  Returns TRUE any form of DXE memory protection is currently active.

  NOTE: The returned value may reflect the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @retval TRUE   DXE Memory protection is active.
  @retval FALSE  DXE Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsDxeMemoryProtectionActive (
  VOID
  );

/**
  Returns TRUE any form of MM memory protection is currently active.

  NOTE: The returned value may reflect the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @retval TRUE   MM Memory protection is active.
  @retval FALSE  MM Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsMmMemoryProtectionActive (
  VOID
  );

#endif
