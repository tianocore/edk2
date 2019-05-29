/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RAND_POOL_NOISE_H__
#define __RAND_POOL_NOISE_H__

#include <Uefi/UefiBaseType.h>

/**
   Get 64-bit noise source.

   @param[out] Rand         Buffer pointer to store 64-bit noise source

   @retval TRUE             Get randomness successfully.
   @retval FALSE            Failed to generate
**/
BOOLEAN
EFIAPI
GetRandomNoise64 (
  OUT UINT64         *Rand
  );


#endif // __RAND_POOL_NOISE_H__
