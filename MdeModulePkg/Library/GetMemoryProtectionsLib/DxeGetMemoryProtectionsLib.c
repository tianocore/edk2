/** @file
Library fills out gMps global for accessing the platform memory protection settings

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/GetMemoryProtectionsLib.h>

MEMORY_PROTECTION_SETTINGS_UNION  gMps = { 0 };

/**
  This function checks the memory protection settings for conflicts.

  @param[in]  Mps   Pointer to the memory protection settings to check.

  @retval EFI_SUCCESS           The memory protection settings are consistent.
  @retval EFI_INVALID_PARAMETER The memory protection settings are not consistent.
**/
STATIC
EFI_STATUS
DxeMemoryProtectionSettingsConsistencyCheck (
  IN MEMORY_PROTECTION_SETTINGS  *Mps
  )
{
  if ((Mps->Dxe.HeapGuard.PoolGuardEnabled || Mps->Dxe.HeapGuard.PageGuardEnabled) &&
      Mps->Dxe.HeapGuard.FreedMemoryGuardEnabled)
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - HeapGuard.FreedMemoryGuardEnabled and "
      "UEFI HeapGuard.PoolGuardEnabled/HeapGuard.PageGuardEnabled "
      "cannot be active at the same time. Setting all three to ZERO in "
      "the memory protection settings global.\n",
      __func__
      ));
    ASSERT (
      !(Mps->Dxe.HeapGuard.FreedMemoryGuardEnabled &&
        (Mps->Dxe.HeapGuard.PoolGuardEnabled || Mps->Dxe.HeapGuard.PageGuardEnabled))
      );
    return EFI_INVALID_PARAMETER;
  }

  if (!IsZeroBuffer (&Mps->Dxe.PoolGuard, MPS_MEMORY_TYPE_BUFFER_SIZE) &&
      (!(Mps->Dxe.HeapGuard.PoolGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PoolGuard protections are active "
      "but HeapGuard.PoolGuardEnabled is inactive.\n",
      __func__
      ));
  }

  if (!IsZeroBuffer (&Mps->Dxe.PageGuard, MPS_MEMORY_TYPE_BUFFER_SIZE) &&
      (!(Mps->Dxe.HeapGuard.PageGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PageGuard protections are active "
      "but HeapGuard.PageGuardEnabled is inactive\n",
      __func__
      ));
  }

  if (Mps->Dxe.ExecutionProtection.EnabledForType[EfiBootServicesData] !=
      Mps->Dxe.ExecutionProtection.EnabledForType[EfiConventionalMemory])
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - EfiBootServicesData and EfiConventionalMemory must have the same "
      "ExecutionProtection value. Setting both to ZERO in the memory protection "
      "settings global.\n",
      __func__
      ));
    ASSERT (
      Mps->Dxe.ExecutionProtection.EnabledForType[EfiBootServicesData] ==
      Mps->Dxe.ExecutionProtection.EnabledForType[EfiConventionalMemory]
      );
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

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
  VOID                        *Ptr;
  MEMORY_PROTECTION_SETTINGS  *Mps;

  Ptr = GetFirstGuidHob (&gMemoryProtectionSettingsGuid);

  if (Ptr != NULL) {
    Mps = (MEMORY_PROTECTION_SETTINGS *)GET_GUID_HOB_DATA (Ptr);
    if (Mps->Dxe.StructVersion != DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Version number of the DXE Memory Protection Settings is invalid!\n",
        __func__
        ));
      ASSERT (Mps->Dxe.StructVersion == DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
      return EFI_ABORTED;
    } else if (Mps->Dxe.Signature != DXE_MEMORY_PROTECTION_SIGNATURE) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Signature of the DXE Memory Protection Settings is invalid!\n",
        __func__
        ));
      ASSERT (Mps->Dxe.Signature == DXE_MEMORY_PROTECTION_SIGNATURE);
      return EFI_ABORTED;
    }

    if (!EFI_ERROR (DxeMemoryProtectionSettingsConsistencyCheck (Mps))) {
      CopyMem (&gMps.Dxe, &Mps->Dxe, sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    }
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Memory Protection Settings not found!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Library constructor used to populate gMps global.

  @retval EFI_SUCCESS   Constructor always returns success;
**/
EFI_STATUS
EFIAPI
GetDxeMemoryProtectionSettingsConstructor (
  VOID
  )
{
  PopulateMpsGlobal ();
  return EFI_SUCCESS;
}
