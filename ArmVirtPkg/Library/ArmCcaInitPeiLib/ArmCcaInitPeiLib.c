/** @file
  Library that implements the Arm CCA initialisation in PEI phase.

  Copyright (c) 2022 2026, Arm Limited. All rights reserved.<BR>
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

**/
VOID
EFIAPI
ArmCcaConfigureSystemMemory (
  VOID
  )
{
  RETURN_STATUS  Status;

  if (!IsRealm ()) {
    return;
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
}

/** Initialise Arm CCA HOBs

  @retval RETURN_SUCCESS             Success.
  @retval RETURN_INVALID_PARAMETER   A parameter is invalid.
  @retval RETURN_OUT_OF_RESOURCES    Out of resources.
**/
RETURN_STATUS
EFIAPI
ArmCcaInitialiseHobs (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINT64         *IsRealmHobData;
  BOOLEAN        RealmWorld;
  UINT64         IpaWidth;
  UINT64         *IpaWidthHobData;

  RealmWorld = IsRealm ();
  if (!RealmWorld) {
    // nothing to do.
    return RETURN_SUCCESS;
  }

  // Create a Guid HOB to cache the IsRealm value.
  IsRealmHobData = BuildGuidHob (
                     &gArmCcaIsRealmGuid,
                     sizeof (*IsRealmHobData)
                     );
  if (IsRealmHobData == NULL) {
    ASSERT (IsRealmHobData != NULL);
    return RETURN_OUT_OF_RESOURCES;
  }

  *IsRealmHobData = RealmWorld;

  // Read the IPA width and Create the Guid HOB, to cache the value in the HOB.
  Status = GetIpaWidth (&IpaWidth);
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  IpaWidthHobData = BuildGuidHob (
                      &gArmCcaIpaWidthGuid,
                      sizeof (*IpaWidthHobData)
                      );
  if (IpaWidthHobData == NULL) {
    ASSERT (IpaWidthHobData != NULL);
    return RETURN_OUT_OF_RESOURCES;
  }

  *IpaWidthHobData = IpaWidth;

  return RETURN_SUCCESS;
}
