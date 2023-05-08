/** @file
  CpuPause function.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"

/**
Provide a hint to the processor that the code sequence is a spin-wait loop.
This can help improve the performance and power consumption of spin-wait loops.

**/
VOID
_mm_pause (
  VOID
  );

#pragma intrinsic(_mm_pause)

/**
  Requests CPU to pause for a short period of time.

  Requests CPU to pause for a short period of time. Typically used in MP
  systems to prevent memory starvation while waiting for a spin lock.

**/
VOID
EFIAPI
CpuPause (
  VOID
  )
{
  _mm_pause ();
}
