/** @file
  Explicit platform initialization for IPMI providers.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

/**
  Prepare platform hardware before an IPMI provider initializes its transport.

  Each PEI, DXE, SMM or Standalone MM IPMI provider calls this function once
  during driver initialization, before transport acquisition or initialization
  and before publishing its PPI or protocol. PEI transport recovery and command
  submission do not call this function again.

  Implementations must use only services available in the consuming phase and
  must not issue IPMI commands through the provider being initialized: it is not
  ready yet. PEI implementations must be safe for execute-in-place operation,
  must not write image globals, and must not retain temporary-memory pointers
  for use after memory migration.

  An implementation must clean up its own resources and disable any callbacks
  it enabled before returning an error. On error the provider logs and returns
  that status without acquiring a transport or publishing its interface.

  @retval EFI_SUCCESS  Platform preparation completed, or no preparation is needed.
  @retval Others       Platform preparation failed; provider initialization stops.
**/
EFI_STATUS
EFIAPI
IpmiPlatformInitialize (
  VOID
  );
