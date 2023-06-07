/** @file
Library fills out gMmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/MmMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

MM_MEMORY_PROTECTION_SETTINGS  gMmMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given MM_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given MM_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetMmMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE             MemoryType,
  IN MM_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  )
{
  switch (MemoryType) {
    case EfiReservedMemoryType:
      return HeapGuardMemoryType.Fields.EfiReservedMemoryType;
    case EfiLoaderCode:
      return HeapGuardMemoryType.Fields.EfiLoaderCode;
    case EfiLoaderData:
      return HeapGuardMemoryType.Fields.EfiLoaderData;
    case EfiBootServicesCode:
      return HeapGuardMemoryType.Fields.EfiBootServicesCode;
    case EfiBootServicesData:
      return HeapGuardMemoryType.Fields.EfiBootServicesData;
    case EfiRuntimeServicesCode:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesCode;
    case EfiRuntimeServicesData:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesData;
    case EfiConventionalMemory:
      return HeapGuardMemoryType.Fields.EfiConventionalMemory;
    case EfiUnusableMemory:
      return HeapGuardMemoryType.Fields.EfiUnusableMemory;
    case EfiACPIReclaimMemory:
      return HeapGuardMemoryType.Fields.EfiACPIReclaimMemory;
    case EfiACPIMemoryNVS:
      return HeapGuardMemoryType.Fields.EfiACPIMemoryNVS;
    case EfiMemoryMappedIO:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIO;
    case EfiMemoryMappedIOPortSpace:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIOPortSpace;
    case EfiPalCode:
      return HeapGuardMemoryType.Fields.EfiPalCode;
    case EfiPersistentMemory:
      return HeapGuardMemoryType.Fields.EfiPersistentMemory;
    default:
      return FALSE;
  }
}

/**
  This function checks the memory protection settings and provides warnings of conflicts and/or
  potentially unforseen consequences from the settings. This logic will only ever turn off
  protections to create consistency, never turn others on.
**/
VOID
MmMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if (gMmMps.HeapGuardPoolType.Data &&
      (!(gMmMps.HeapGuardPolicy.Fields.PoolGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Bits set in gMmMps.HeapGuardPoolType, but gMmMps.HeapGuardPolicy.Fields.PoolGuardEnabled is inactive. "
      "No pool guards will be set.\n",
      __func__
      ));
  }

  if (gMmMps.HeapGuardPageType.Data &&
      (!(gMmMps.HeapGuardPolicy.Fields.PageGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Bits are set in gMmMps.HeapGuardPageType, but gMmMps.HeapGuardPolicy.Fields.PageGuardEnabled is inactive. "
      "No page guards will be set.\n",
      __func__
      ));
  }
}

/**
  Abstraction layer for library constructor of Standalone MM and SMM instances.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MmMemoryProtectionHobLibConstructorCommon (
  VOID
  )
{
  VOID  *Ptr;

  Ptr = GetFirstGuidHob (&gMmMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) != (UINT8)MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Version number of the Memory Protection Settings HOB is invalid!\n",
        __func__
        ));
      ASSERT (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) == (UINT8)MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
      ZeroMem (&gMmMps, sizeof (gMmMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gMmMps, GET_GUID_HOB_DATA (Ptr), sizeof (MM_MEMORY_PROTECTION_SETTINGS));
    MmMemoryProtectionSettingsConsistencyCheck ();
  }

  return EFI_SUCCESS;
}
