/** @file

  Utility functions for serializing (persistently storing) and deserializing
  OVMF's platform configuration.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/OvmfPlatformConfig.h>

#include "PlatformConfig.h"

//
// Name of the UEFI variable that we use for persistent storage.
//
STATIC CHAR16  mVariableName[] = L"PlatformConfig";

/**
  Serialize and persistently save platform configuration.

  @param[in] PlatformConfig  The platform configuration to serialize and save.

  @return  Status codes returned by gRT->SetVariable().
**/
EFI_STATUS
EFIAPI
PlatformConfigSave (
  IN PLATFORM_CONFIG  *PlatformConfig
  )
{
  EFI_STATUS  Status;

  //
  // We could implement any kind of translation here, as part of serialization.
  // For example, we could expose the platform configuration in separate
  // variables with human-readable contents, allowing other tools to access
  // them more easily. For now, just save a binary dump.
  //
  Status = gRT->SetVariable (
                  mVariableName,
                  &gOvmfPlatformConfigGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof *PlatformConfig,
                  PlatformConfig
                  );
  return Status;
}

/**
  Load and deserialize platform configuration.

  When the function fails, output parameters are indeterminate.

  @param[out] PlatformConfig    The platform configuration to receive the
                                loaded data.

  @param[out] OptionalElements  This bitmap describes the presence of optional
                                configuration elements that have been loaded.
                                PLATFORM_CONFIG_F_DOWNGRADE means that some
                                unknown elements, present in the wire format,
                                have been ignored.

  @retval  EFI_SUCCESS         Loading & deserialization successful.
  @return                      Error codes returned by GetVariable2().
**/
EFI_STATUS
EFIAPI
PlatformConfigLoad (
  OUT PLATFORM_CONFIG  *PlatformConfig,
  OUT UINT64           *OptionalElements
  )
{
  VOID        *Data;
  UINTN       DataSize;
  EFI_STATUS  Status;

  //
  // Any translation done in PlatformConfigSave() would have to be mirrored
  // here. For now, just load the binary dump.
  //
  // Versioning of the binary wire format is implemented based on size
  // (only incremental changes, ie. new fields), and on GUID.
  // (Incompatible changes require a GUID change.)
  //
  Status = GetVariable2 (
             mVariableName,
             &gOvmfPlatformConfigGuid,
             &Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *OptionalElements = 0;
  if (DataSize > sizeof *PlatformConfig) {
    //
    // Handle firmware downgrade -- keep only leading part.
    //
    CopyMem (PlatformConfig, Data, sizeof *PlatformConfig);
    *OptionalElements |= PLATFORM_CONFIG_F_DOWNGRADE;
  } else {
    CopyMem (PlatformConfig, Data, DataSize);

    //
    // Handle firmware upgrade -- zero out missing fields.
    //
    ZeroMem (
      (UINT8 *)PlatformConfig + DataSize,
      sizeof *PlatformConfig - DataSize
      );
  }

  //
  // Based on DataSize, report the optional features that we recognize.
  //
  if (DataSize >= (OFFSET_OF (PLATFORM_CONFIG, VerticalResolution) +
                   sizeof PlatformConfig->VerticalResolution))
  {
    *OptionalElements |= PLATFORM_CONFIG_F_GRAPHICS_RESOLUTION;
  }

  FreePool (Data);
  return EFI_SUCCESS;
}
