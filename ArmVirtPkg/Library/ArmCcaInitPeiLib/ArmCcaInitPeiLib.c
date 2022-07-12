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
#include <Library/BaseLib.h>

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

  if (!ArmCcaIsRealm ()) {
    return;
  }

  Status =  ArmCcaRsiSetIpaState (
              (UINT64 *)PcdGet64 (PcdSystemMemoryBase),
              PcdGet64 (PcdSystemMemorySize),
              RipasRam,
              ARM_CCA_RIPAS_CHANGE_FLAGS_RSI_NO_CHANGE_DESTROYED
              );
  if (RETURN_ERROR (Status)) {
    // Panic
    CpuDeadLoop ();
  }
}
