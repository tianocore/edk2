/** @file
  Copyright (C) 2022, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FullMemoryFence.h"
#include <intrin.h>

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
  _ReadWriteBarrier ();
  _mm_mfence ();
}
