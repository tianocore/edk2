/** @file
  Library that implements a NULL implementation of the ArmCcaInitPeiLib.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmVirtMemInfoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
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
  return RETURN_UNSUPPORTED;
}

/**
  Perform Arm CCA specific initialisations.

  @retval EFI_SUCCESS               Success or execution context is not a Realm.
  @retval EFI_OUT_OF_RESOURCES      Out of resources.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
EFI_STATUS
EFIAPI
ArmCcaInitialize (
  VOID
  )
{
  // Noting to do as the execution context is not a Realm.
  return EFI_SUCCESS;
}
