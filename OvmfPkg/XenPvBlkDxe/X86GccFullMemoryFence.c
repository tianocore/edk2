/** @file
  Copyright (C) 2022, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FullMemoryFence.h"

//
// Like MemoryFence() but prevent stores from been reorded with loads by
// the CPU on X64.
//
VOID
EFIAPI
FullMemoryFence (
  VOID
  )
{
  __asm__ __volatile__ ("mfence":::"memory");
}
