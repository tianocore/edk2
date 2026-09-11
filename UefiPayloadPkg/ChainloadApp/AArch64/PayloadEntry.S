/** @file
  AArch64 payload entry stub.  Masks all interrupts, switches
  stack and branches to the payload entry point with the HOB
  list in x0.

  The MMU and caches are left ENABLED.  ChainloadApp installs its
  own translation tables (in EfiReservedMemoryType pages) via
  ArmConfigureMmu() while the outer firmware's boot services are
  still available, and only branches here once TCR/MAIR/TTBR0 point
  at those tables.  There is no cache-off window, so no data-cache
  maintenance is done here; the caller has already invalidated the
  instruction cache over the FV for I/D coherency.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AArch64/AsmMacroLib.h>

// VOID
// EFIAPI
// JumpToPayload (
//   IN UINTN  NewStack,    // x0
//   IN UINTN  HobList,     // x1
//   IN UINTN  EntryPoint   // x2
//   );
ASM_FUNC(JumpToPayload)
  msr  daifset, #0xf
  bic  x0, x0, #0xf
  mov  sp, x0
  mov  x0, x1
  br   x2
