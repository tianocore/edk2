/** @file
  Library that implements the Arm CCA helper functions.

  Copyright (c) 2022 - 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/

#pragma once

#include <Base.h>
#include <Uefi/UefiBaseType.h>

/**
  Check if running in a Realm.

    @retval TRUE    The execution is within the context of a Realm.
    @retval FALSE   The execution is not within the context of a Realm.
**/
BOOLEAN
EFIAPI
IsRealm (
  VOID
  );

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
  );
