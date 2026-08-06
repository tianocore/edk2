/** @file
  Library fills out gDxeMps global

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given DXE_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given DXE_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetDxeMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE              MemoryType,
  IN DXE_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
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
DxeMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if (!gDxeMps.ImageProtectionPolicy.Data &&
      (gDxeMps.NxProtectionPolicy.Fields.EfiLoaderData       ||
       gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData ||
       gDxeMps.NxProtectionPolicy.Fields.EfiRuntimeServicesData))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Image Protection is inactive, but one or more of \
      NxProtectionPolicy.EfiLoaderData \
      NxProtectionPolicy.EfiBootServicesData \
      NxProtectionPolicy.EfiRuntimeServicesData are active. \
      Image data sections could still be non-executable.\n",
      __func__
      ));
  }

  if (gDxeMps.HeapGuardPoolType.Data &&
      (!(gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Pool protections are active, \
      but neither HeapGuardPolicy.UefiPoolGuard nor \
      HeapGuardPolicy.MmPoolGuard are active.\n",
      __func__
      ));
  }

  if (gDxeMps.HeapGuardPageType.Data &&
      (!(gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Page protections are active, \
      but neither HeapGuardPolicy.UefiPageGuard nor \
      HeapGuardPolicy.MmPageGuard are active.\n",
      __func__
      ));
  }

  if (gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData != gDxeMps.NxProtectionPolicy.Fields.EfiConventionalMemory) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - NxProtectionPolicy.EfiBootServicesData \
      and NxProtectionPolicy.EfiConventionalMemory must have the same value. \
      Setting both to ZERO in the memory protection settings global.\n",
      __func__
      ));
    gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData   = 0;
    gDxeMps.NxProtectionPolicy.Fields.EfiConventionalMemory = 0;
  }

  //
  // the heap guard system does not support non-EFI_PAGE_SIZE alignments
  // architectures that require larger RUNTIME_PAGE_ALLOCATION_GRANULARITY
  // cannot have EfiRuntimeServicesCode, EfiRuntimeServicesData, EfiReservedMemoryType,
  // and EfiACPIMemoryNVS guarded. OSes do not map guard pages anyway, so this is a
  // minimal loss. Not guarding prevents alignment mismatches
  //
  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE) {
    if (gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard) {
      if (gDxeMps.HeapGuardPageType.Fields.EfiACPIMemoryNVS != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Page Guard set on \
          EfiACPIMemoryNVS. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPageType.Fields.EfiACPIMemoryNVS = 0;
      }

      if (gDxeMps.HeapGuardPageType.Fields.EfiReservedMemoryType != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Page Guard set on \
          EfiReservedMemoryType. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPageType.Fields.EfiReservedMemoryType = 0;
      }

      if (gDxeMps.HeapGuardPageType.Fields.EfiRuntimeServicesCode != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Page Guard set on \
          EfiRuntimeServicesCode. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPageType.Fields.EfiRuntimeServicesCode = 0;
      }

      if (gDxeMps.HeapGuardPageType.Fields.EfiRuntimeServicesData != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Page Guard set on \
          EfiRuntimeServicesData. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPageType.Fields.EfiRuntimeServicesData = 0;
      }
    }

    if (gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard) {
      if (gDxeMps.HeapGuardPoolType.Fields.EfiACPIMemoryNVS != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Pool Guard set on \
          EfiACPIMemoryNVS. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPoolType.Fields.EfiACPIMemoryNVS = 0;
      }

      if (gDxeMps.HeapGuardPoolType.Fields.EfiReservedMemoryType != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Pool Guard set on \
          EfiReservedMemoryType. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPoolType.Fields.EfiReservedMemoryType = 0;
      }

      if (gDxeMps.HeapGuardPoolType.Fields.EfiRuntimeServicesCode != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Pool Guard set on \
          EfiRuntimeServicesCode. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPoolType.Fields.EfiRuntimeServicesCode = 0;
      }

      if (gDxeMps.HeapGuardPoolType.Fields.EfiRuntimeServicesData != 0) {
        DEBUG ((
          DEBUG_WARN,
          "%a: - RUNTIME_PAGE_ALLOCATION_GRANULARITY != EFI_PAGE_SIZE but Pool Guard set on \
          EfiRuntimeServicesData. This is not supported by Heap Guard system, disabling.\n",
          __func__
          ));
        gDxeMps.HeapGuardPoolType.Fields.EfiRuntimeServicesData = 0;
      }
    }
  }
}

/**
  Translates a memory type bitmask, ordered as defined by the memory protection PCDs
  (UEFI spec memory type order in the low bits, OEM/OS reserved in the high bits), into
  a DXE_HEAP_GUARD_MEMORY_TYPES bitfield.

  @param[in]  Bitmask   The PCD-style memory type bitmask to translate.

  @return The equivalent DXE_HEAP_GUARD_MEMORY_TYPES bitfield.
**/
STATIC
DXE_HEAP_GUARD_MEMORY_TYPES
DxeMemoryTypesFromBitmask (
  IN UINT64  Bitmask
  )
{
  DXE_HEAP_GUARD_MEMORY_TYPES  MemoryTypes;

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
  Populates the gDxeMps global with the memory protection settings derived from the
  platform memory protection PCDs. Used as a fallback when the Memory Protection
  Settings HOB is not present.
**/
STATIC
VOID
PopulateDxeMpsFromPcds (
  VOID
  )
{
  UINT8   NullMask;
  UINT8   HeapGuardMask;
  UINT32  ImageProtectionMask;

  ZeroMem (&gDxeMps, sizeof (gDxeMps));

  gDxeMps.StructVersion = DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION;
  gDxeMps.CpuStackGuard = PcdGetBool (PcdCpuStackGuard);

  NullMask                                                    = PcdGet8 (PcdNullPointerDetectionPropertyMask);
  gDxeMps.NullPointerDetectionPolicy.Fields.UefiNullDetection = (NullMask & BIT0) ? 1 : 0;
  gDxeMps.NullPointerDetectionPolicy.Fields.DisableEndOfDxe   = (NullMask & BIT7) ? 1 : 0;

  HeapGuardMask                                = PcdGet8 (PcdHeapGuardPropertyMask);
  gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard = (HeapGuardMask & BIT0) ? 1 : 0;
  gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard = (HeapGuardMask & BIT1) ? 1 : 0;
  gDxeMps.HeapGuardPolicy.Fields.Direction     = (HeapGuardMask & BIT7) ? 1 : 0;

  ImageProtectionMask                                          = PcdGet32 (PcdImageProtectionPolicy);
  gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown = (ImageProtectionMask & BIT0) ? 1 : 0;
  gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv      = (ImageProtectionMask & BIT1) ? 1 : 0;

  gDxeMps.HeapGuardPoolType  = DxeMemoryTypesFromBitmask (PcdGet64 (PcdHeapGuardPoolType));
  gDxeMps.HeapGuardPageType  = DxeMemoryTypesFromBitmask (PcdGet64 (PcdHeapGuardPageType));
  gDxeMps.NxProtectionPolicy = DxeMemoryTypesFromBitmask (PcdGet64 (PcdDxeNxMemoryProtectionPolicy));

  //
  // Non-stop mode is only active when the corresponding UEFI protection is also enabled.
  //
  gDxeMps.HeapGuardPolicy.Fields.NonStopMode            = ((HeapGuardMask & (BIT6 | BIT4 | BIT1 | BIT0)) > BIT6) ? 1 : 0;
  gDxeMps.NullPointerDetectionPolicy.Fields.NonStopMode = ((NullMask & (BIT6 | BIT0)) > BIT6) ? 1 : 0;
}

/**
  Populates gDxeMps global with the data present in the HOB. If the HOB entry does not exist,
  this constructor will populate the memory protection settings from the memory protection PCDs.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *Ptr;

  Ptr = GetFirstGuidHob (&gDxeMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) != (UINT8)DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_ERROR,
        "\nThe version number of the DXE Memory Protection Settings HOB is invalid! Expected: %d, Actual: %d\n",
        DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,
        *((UINT8 *)GET_GUID_HOB_DATA (Ptr))
        ));
      DEBUG ((DEBUG_ERROR, "This usually happens when the Memory Protection Settings version was incremented\n"));
      DEBUG ((DEBUG_ERROR, "and all modules have not been rebuilt with the new version number. Less likely but\n"));
      DEBUG ((DEBUG_ERROR, "also possible is the HOB entry was corrupted or the producer of the HOB entry\n"));
      DEBUG ((DEBUG_ERROR, "did not set the StructVersion field to DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION.\n"));
      ASSERT (FALSE);
      ZeroMem (&gDxeMps, sizeof (gDxeMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gDxeMps, GET_GUID_HOB_DATA (Ptr), sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    DxeMemoryProtectionSettingsConsistencyCheck ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "DxeMemoryProtectionHobLibConstructor - Unable to fetch memory protection HOB. \
       Falling back to memory protection PCDs\n"
      ));
    PopulateDxeMpsFromPcds ();
    DxeMemoryProtectionSettingsConsistencyCheck ();
  }

  return EFI_SUCCESS;
}
