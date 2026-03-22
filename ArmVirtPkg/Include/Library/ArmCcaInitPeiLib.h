/** @file
  Library that implements the Arm CCA helper functions.

  Copyright (c) 2022 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
**/

#pragma once

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
  );
