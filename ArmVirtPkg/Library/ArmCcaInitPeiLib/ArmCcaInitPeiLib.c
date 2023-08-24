/** @file
  Library that implements the Arm CCA initialisation in PEI phase.

  Copyright (c) 2022 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/
#include <PiPei.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmVirtMemInfoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

/**
  Configure the System Memory region as Protected RAM.

  When a VMM creates a Realm, a small amount of DRAM (which contains the
  firmware image) and the initial content is configured as Protected RAM.
  The remaining System Memory is in the Protected Empty state. The firmware
  must then initialise the remaining System Memory as Protected RAM before
  it can be accessed.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_UNSUPPORTED        The execution context is not in a Realm.
**/
RETURN_STATUS
EFIAPI
ArmCcaConfigureSystemMemory (
  VOID
  )
{
  RETURN_STATUS  Status;

  if (!IsRealm ()) {
    return RETURN_UNSUPPORTED;
  }

  Status =  RsiSetIpaState (
              (UINT64 *)PcdGet64 (PcdSystemMemoryBase),
              PcdGet64 (PcdSystemMemorySize),
              RipasRam,
              RIPAS_CHANGE_FLAGS_RSI_NO_CHANGE_DESTROYED
              );
  if (RETURN_ERROR (Status)) {
    // Panic
    CpuDeadLoop ();
  }

  return Status;
}

/**
  Perform Arm CCA specific initialisations.

  @retval RETURN_SUCCESS               Success or execution context is not a Realm.
  @retval RETURN_OUT_OF_RESOURCES      Out of resources.
  @retval RETURN_INVALID_PARAMETER     A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaInitialize (
  VOID
  )
{
  EFI_STATUS    Status;
  REALM_CONFIG  *Config;
  UINT64        *IpaWidthHobData;

  if (!IsRealm ()) {
    // Noting to do as the execution context is not a Realm.
    return RETURN_SUCCESS;
  }

  // Read the Realm Config and store the IPA width in a GUID HOB.
  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (REALM_CONFIG)),
             REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (REALM_CONFIG));

  Status = RsiGetRealmConfig (Config);
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (Config->IpaWidth > ArmGetPhysicalAddressBits ()) {
    ASSERT (0);
    return RETURN_INVALID_PARAMETER;
  }

  IpaWidthHobData = BuildGuidHob (
                      &gArmCcaIpaWidthGuid,
                      sizeof (*IpaWidthHobData)
                      );
  if (IpaWidthHobData == NULL) {
    ASSERT (0);
    FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (REALM_CONFIG)));
    return RETURN_OUT_OF_RESOURCES;
  }

  *IpaWidthHobData = Config->IpaWidth;

  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (REALM_CONFIG)));

  // Configure the MMIO memory regions.
  return ArmCcaConfigureMmio (*IpaWidthHobData);
}
