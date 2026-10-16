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

#include "ArmCcaLibInternal.h"

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
  if (!ArmHasRme ()) {
    return FALSE;
  }

  return ArmCcaReadIsRealm ();
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
  if (IpaWidth == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (!ArmCcaIsRealm ()) {
    *IpaWidth = 0;
    return RETURN_UNSUPPORTED;
  }

  return ArmCcaReadIpaWidth (IpaWidth);
}
