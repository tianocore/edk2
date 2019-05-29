/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

/**
  Get 64-bit noise source

  @param[out] Rand         Buffer pointer to store 64-bit noise source

  @retval TRUE             Get randomness successfully.
  @retval FALSE            Failed to generate
**/
BOOLEAN
EFIAPI
GetRandomNoise64 (
  OUT UINT64         *Rand
  )
{
  UINT32 Index;
  UINT32 *RandPtr;

  if (NULL == Rand) {
    return FALSE;
  }

  RandPtr = (UINT32 *)Rand;

  for (Index = 0; Index < 2; Index ++) {
    *RandPtr = (UINT32) ((AsmReadTsc ()) & 0xFF);
    RandPtr++;
    MicroSecondDelay (10);
  }

  return TRUE;
}
