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
  RETURN_STATUS  Status;
  UINT32         UefiImpl;
  UINT32         RmmImplLow;
  UINT32         RmmImplHigh;

  if (ArmHasRme ()) {
    Status = RsiGetVersion (
               &UefiImpl,
               &RmmImplLow,
               &RmmImplHigh
               );
    if (!RETURN_ERROR (Status)) {
      return TRUE;
    }
  }

  return FALSE;
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
  REALM_CONFIG   *Config;
  UINT8          ConfigBuffer[sizeof (REALM_CONFIG) * 2];

  if (!IsRealm ()) {
    *IpaWidth = 0;
    return RETURN_UNSUPPORTED;
  }

  Config = ALIGN_POINTER (ConfigBuffer, sizeof (REALM_CONFIG));
  ZeroMem (Config, sizeof (REALM_CONFIG));

  Status = RsiGetRealmConfig (Config);
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    *IpaWidth = 0;
    return Status;
  }

  if (Config->IpaWidth > ArmGetPhysicalAddressBits ()) {
    ASSERT (0);
    *IpaWidth = 0;
    return RETURN_INVALID_PARAMETER;
  }

  *IpaWidth = Config->IpaWidth;
  return RETURN_SUCCESS;
}
