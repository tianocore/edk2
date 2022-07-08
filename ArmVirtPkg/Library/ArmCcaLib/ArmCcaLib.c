/** @file
  Library that implements the Arm CCA helper functions.

  Copyright (c) 2022 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/
#include <Base.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Uefi/UefiMultiPhase.h>
#include <Uefi/UefiSpec.h>
#include <Pi/PiBootMode.h>

#include <Pi/PiHob.h>
#include <Library/HobLib.h>

/**
  Queries to see if running in a Realm.

    @retval TRUE    The execution is within the context of a Realm.
    @retval FALSE   The execution is not within the context of a Realm.
**/
STATIC
BOOLEAN
EFIAPI
ArmCcaReadIsRealm (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINT32         UefiImpl;
  UINT32         RmmImplLow;
  UINT32         RmmImplHigh;

  // Query the RSI_GET_VERSION. If it is implemented then
  // RMM is present and we must be running in a Realm.
  Status = ArmCcaRsiGetVersion (
             &UefiImpl,
             &RmmImplLow,
             &RmmImplHigh
             );
  if (!RETURN_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if running in a Realm.

    @retval TRUE    The execution is within the context of a Realm.
    @retval FALSE   The execution is not within the context of a Realm.
**/
BOOLEAN
EFIAPI
ArmCcaIsRealm (
  VOID
  )
{
  BOOLEAN  RealmWorld;
  VOID     *HobList;

  VOID    *Hob;
  UINT64  *IsRealmHobData;

  if (!ArmHasRme ()) {
    return FALSE;
  }

  HobList = GetHobList ();
  if (HobList == NULL) {
    // HOB not initialised.
    return ArmCcaReadIsRealm ();
  }

  RealmWorld = FALSE;
  Hob        = GetFirstGuidHob (&gArmCcaIsRealmGuid);
  if ((Hob == NULL) ||
      (GET_GUID_HOB_DATA_SIZE (Hob) < sizeof (*IsRealmHobData)))
  {
    // The gArmCcaIsRealmGuid is created in the PlatformPeim() which
    // may not have been called for caching the IsRealm value yet.
    // So, read the Realm value.
    RealmWorld = ArmCcaReadIsRealm ();
  } else {
    // Read the value from the HOB
    IsRealmHobData = GET_GUID_HOB_DATA (Hob);
    RealmWorld     = *IsRealmHobData;
  }

  return RealmWorld;
}

/**
  Read the IPA width of the Realm.

  The IPA width of the Realm is used to configure the protection attribute
  for memory regions, see ArmCcaSetMemoryProtectionAttribute().

    @param [out] IpaWidth  IPA width of the Realm.

    @retval RETURN_SUCCESS            Success.
    @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
    @retval RETURN_OUT_OF_RESOURCES   Out of resources.

**/
STATIC
RETURN_STATUS
EFIAPI
ArmCcaReadIpaWidth (
  OUT UINT64  *IpaWidth
  )
{
  RETURN_STATUS         Status;
  ARM_CCA_REALM_CONFIG  *Config;

  if (IpaWidth == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)),
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (ARM_CCA_REALM_CONFIG));

  Status = ArmCcaRsiGetRealmConfig (Config);
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    goto ExitHandler;
  }

  if (Config->IpaWidth > ArmGetPhysicalAddressBits ()) {
    ASSERT (0);
    Status = RETURN_INVALID_PARAMETER;
    goto ExitHandler;
  }

  *IpaWidth = Config->IpaWidth;

ExitHandler:
  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)));
  return Status;
}

/**
  Return the IPA width of the Realm.

  The IPA width of the Realm is used to configure the protection attribute
  for memory regions, see ArmCcaSetMemoryProtectionAttribute().

    @param [out] IpaWidth  IPA width of the Realm.

    @retval RETURN_SUCCESS            Success.
    @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
    @retval RETURN_UNSUPPORTED        The execution is not within the
                                      context of a Realm.
    @retval RETURN_OUT_OF_RESOURCES   Out of resources.
**/
RETURN_STATUS
EFIAPI
ArmCcaGetIpaWidth (
  OUT UINT64  *IpaWidth
  )
{
  RETURN_STATUS  Status;
  VOID           *HobList;
  VOID           *Hob;
  UINT64         *CcaIpaWidth;

  if (IpaWidth == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (!ArmCcaIsRealm ()) {
    *IpaWidth = 0;
    return RETURN_UNSUPPORTED;
  }

  HobList = GetHobList ();
  if (HobList == NULL) {
    // HOB not initialised.
    return ArmCcaReadIpaWidth (IpaWidth);
  }

  Hob = GetFirstGuidHob (&gArmCcaIpaWidthGuid);
  if ((Hob == NULL) ||
      (GET_GUID_HOB_DATA_SIZE (Hob) < sizeof (UINT64)))
  {
    // The gArmCcaIpaWidthGuid is created in the PlatformPeim() which
    // may not have been called for caching the IpaWidth value yet.
    // So, read the IPA Width value.
    Status = ArmCcaReadIpaWidth (IpaWidth);
    if (RETURN_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } else {
    // Read the value from the HOB.
    CcaIpaWidth = GET_GUID_HOB_DATA (Hob);
    if ((UINT64)*CcaIpaWidth == 0) {
      return RETURN_NOT_FOUND;
    }

    *IpaWidth = *CcaIpaWidth;
  }

  return RETURN_SUCCESS;
}
