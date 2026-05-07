/** @file
  Library that implements a NULL implementation of the ArmCcaInitPeiLib.

  Copyright (c) 2022 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/

#include <Base.h>

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
  return RETURN_SUCCESS;
}
