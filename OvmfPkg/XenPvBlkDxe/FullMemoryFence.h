/** @file
  Copyright (C) 2022, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)

//
// Like MemoryFence() but prevent stores from been reorded with loads by
// the CPU on X64.
//
VOID
EFIAPI
FullMemoryFence (
  VOID
  );

#else

//
// Only implement FullMemoryFence() on X86 as MemoryFence() is probably
// fine on other platform.
//
#define FullMemoryFence()  MemoryFence()

#endif
