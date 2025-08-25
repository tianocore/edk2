/** @file
  Force interrupt handler to return with interrupts still disabled.

  Copyright (C) 2022, Fen Systems Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "Iret.h"

/**
  Force interrupt handler to return with interrupts still disabled.

  @param SystemContext         A pointer to the system context when the
                               interrupt occurred.
**/
VOID
DisableInterruptsOnIret (
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
 #if defined (MDE_CPU_X64)

  IA32_EFLAGS32  Rflags;

  //
  // Get flags from system context.
  //
  Rflags.UintN = SystemContext.SystemContextX64->Rflags;
  ASSERT (Rflags.Bits.IF);

  //
  // Clear interrupts-enabled flag.
  //
  Rflags.Bits.IF                         = 0;
  SystemContext.SystemContextX64->Rflags = Rflags.UintN;

 #else

  #error "Unsupported CPU"

 #endif
}
