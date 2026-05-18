/** @file
  Force interrupt handler to return with interrupts still disabled.

  Copyright (C) 2022, Fen Systems Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Protocol/DebugSupport.h>

VOID
DisableInterruptsOnIret (
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );
