/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

/**
  Get 64-bit noise source

  @param[out] Rand         Buffer pointer to store 64-bit noise source

  @retval FALSE            Failed to generate
**/
BOOLEAN
EFIAPI
GetRandomNoise64 (
  OUT UINT64         *Rand
  )
{
  //
  // Return FALSE will fallback to use PerformaceCounter to
  // generate noise.
  //
  return FALSE;
}
