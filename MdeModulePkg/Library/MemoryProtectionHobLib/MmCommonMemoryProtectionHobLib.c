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
#include <Library/PcdLib.h>

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
    case EfiUnacceptedMemoryType:
      return HeapGuardMemoryType.Fields.EfiUnacceptedMemoryType;
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
      (!(gMmMps.HeapGuardPolicy.Fields.MmPoolGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Bits set in gMmMps.HeapGuardPoolType, but gMmMps.HeapGuardPolicy.Fields.MmPoolGuard is inactive. "
      "No pool guards will be set.\n",
      __func__
      ));
  }

  if (gMmMps.HeapGuardPageType.Data &&
      (!(gMmMps.HeapGuardPolicy.Fields.MmPageGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Bits are set in gMmMps.HeapGuardPageType, but gMmMps.HeapGuardPolicy.Fields.MmPageGuard is inactive. "
      "No page guards will be set.\n",
      __func__
      ));
  }
}

/**
  Translates a memory type bitmask, ordered as defined by the memory protection PCDs
  (UEFI spec memory type order in the low bits, OEM/OS reserved in the high bits), into
  a MM_HEAP_GUARD_MEMORY_TYPES bitfield.

  @param[in]  Bitmask   The PCD-style memory type bitmask to translate.

  @return The equivalent MM_HEAP_GUARD_MEMORY_TYPES bitfield.
**/
STATIC
MM_HEAP_GUARD_MEMORY_TYPES
MmMemoryTypesFromBitmask (
  IN UINT64  Bitmask
  )
{
  MM_HEAP_GUARD_MEMORY_TYPES  MemoryTypes;

  //
  // Bits 0 - 14 (EfiReservedMemoryType through EfiPersistentMemory) map directly
  // to the low bits of the bitfield. The OEM/OS reserved types live in the high
  // bits of the PCD but immediately after EfiUnacceptedMemoryType in the bitfield.
  //
  MemoryTypes.Data               = (UINT32)(Bitmask & 0x7FFF);
  MemoryTypes.Fields.OEMReserved = ((Bitmask & BIT62) != 0) ? 1 : 0;
  MemoryTypes.Fields.OSReserved  = ((Bitmask & BIT63) != 0) ? 1 : 0;

  return MemoryTypes;
}

/**
  Populates the gMmMps global with the memory protection settings derived from the
  platform memory protection PCDs. Used as a fallback when the Memory Protection
  Settings HOB is not present.
**/
STATIC
VOID
PopulateMmMpsFromPcds (
  VOID
  )
{
  UINT8  NullMask;
  UINT8  HeapGuardMask;

  ZeroMem (&gMmMps, sizeof (gMmMps));

  gMmMps.StructVersion = MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION;

  NullMask                          = PcdGet8 (PcdNullPointerDetectionPropertyMask);
  gMmMps.NullPointerDetectionPolicy = (NullMask & BIT1) ? TRUE : FALSE;

  HeapGuardMask                             = PcdGet8 (PcdHeapGuardPropertyMask);
  gMmMps.HeapGuardPolicy.Fields.MmPageGuard = (HeapGuardMask & BIT2) ? 1 : 0;
  gMmMps.HeapGuardPolicy.Fields.MmPoolGuard = (HeapGuardMask & BIT3) ? 1 : 0;
  gMmMps.HeapGuardPolicy.Fields.Direction   = (HeapGuardMask & BIT7) ? 1 : 0;

  gMmMps.HeapGuardPoolType = MmMemoryTypesFromBitmask (PcdGet64 (PcdHeapGuardPoolType));
  gMmMps.HeapGuardPageType = MmMemoryTypesFromBitmask (PcdGet64 (PcdHeapGuardPageType));

  //
  // Non-stop mode is only active when the corresponding MM protection is also enabled.
  //
  gMmMps.HeapGuardPolicy.Fields.NonStopMode = ((HeapGuardMask & (BIT6 | BIT3 | BIT2)) > BIT6) ? 1 : 0;
  gMmMps.NullDetectionNonStopMode           = ((NullMask & (BIT6 | BIT1)) > BIT6) ? TRUE : FALSE;
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
        "\nThe version number of the MM Memory Protection Settings HOB is invalid! Expected: %d, Actual: %d\n",
        MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,
        *((UINT8 *)GET_GUID_HOB_DATA (Ptr))
        ));
      DEBUG ((DEBUG_ERROR, "This usually happens when the Memory Protection Settings version was incremented\n"));
      DEBUG ((DEBUG_ERROR, "and all modules have not been rebuilt with the new version number. Less likely but\n"));
      DEBUG ((DEBUG_ERROR, "also possible is the HOB entry was corrupted or the producer of the HOB entry\n"));
      DEBUG ((DEBUG_ERROR, "did not set the StructVersion field to MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION.\n"));
      ASSERT (FALSE);
      ZeroMem (&gMmMps, sizeof (gMmMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gMmMps, GET_GUID_HOB_DATA (Ptr), sizeof (MM_MEMORY_PROTECTION_SETTINGS));
    MmMemoryProtectionSettingsConsistencyCheck ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "MmMemoryProtectionHobLibConstructor - Unable to fetch memory protection HOB. \
Falling back to memory protection PCDs\n"
      ));
    PopulateMmMpsFromPcds ();
    MmMemoryProtectionSettingsConsistencyCheck ();
  }

  return EFI_SUCCESS;
}
