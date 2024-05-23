/** @file
  Library that implements the Arm CCA helper functions.

  Copyright (c) 2022 2023, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/

#ifndef ARM_CCA_INIT_PEI_LIB_
#define ARM_CCA_INIT_PEI_LIB_

#include <Base.h>

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
  );

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
  );

#endif // ARM_CCA_LIB_
