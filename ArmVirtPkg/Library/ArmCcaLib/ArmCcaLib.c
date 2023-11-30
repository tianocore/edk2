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
ReadIsRealm (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINT32         UefiImpl;
  UINT32         RmmImplLow;
  UINT32         RmmImplHigh;

  Status = RsiGetVersion (
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
IsRealm (
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
    return ReadIsRealm ();
  }

  RealmWorld = FALSE;
  Hob        = GetFirstGuidHob (&gArmCcaIsRealmGuid);
  if ((Hob == NULL) ||
      (GET_GUID_HOB_DATA_SIZE (Hob) < sizeof (*IsRealmHobData)))
  {
    // The gArmCcaIsRealmGuid is created in the PlatformPeim() which
    // may not have been called for caching the IsRealm value yet.
    // So, read the Realm value.
    RealmWorld = ReadIsRealm ();
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

**/
STATIC
RETURN_STATUS
EFIAPI
ReadIpaWidth (
  OUT UINT64  *IpaWidth
  )
{
  RETURN_STATUS  Status;
  REALM_CONFIG   *Config;
  UINT8          ConfigBuffer[sizeof (REALM_CONFIG) * 2];

  Config = ALIGN_POINTER (ConfigBuffer, sizeof (REALM_CONFIG));
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

  *IpaWidth = Config->IpaWidth;
  return RETURN_SUCCESS;
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
**/
RETURN_STATUS
EFIAPI
GetIpaWidth (
  OUT UINT64  *IpaWidth
  )
{
  RETURN_STATUS  Status;
  VOID           *HobList;
  VOID           *Hob;
  UINT64         *CcaIpaWidth;

  if (!IsRealm ()) {
    *IpaWidth = 0;
    return RETURN_UNSUPPORTED;
  }

  HobList = GetHobList ();
  if (HobList == NULL) {
    // HOB not initialised.
    return ReadIpaWidth (IpaWidth);
  }

  Hob = GetFirstGuidHob (&gArmCcaIpaWidthGuid);
  if ((Hob == NULL) ||
      (GET_GUID_HOB_DATA_SIZE (Hob) < sizeof (UINT64)))
  {
    // The gArmCcaIpaWidthGuid is created in the PlatformPeim() which
    // may not have been called for caching the IpaWidth value yet.
    // So, read the IPA Width value.
    Status = ReadIpaWidth (IpaWidth);
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
