/** @file
  Pseudorandom Number Generator Wrapper Implementation which does not provide
  real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"


/**
  Sets up the seed value for the pseudorandom number generator.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Seed      Pointer to seed value.
                        If NULL, default seed is used.
  @param[in]  SeedSize  Size of seed value.
                        If Seed is NULL, this parameter is ignored.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RandomSeed (
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Generates a pseudorandom byte stream of the specified size.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RandomBytes (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  )
{
  ASSERT (FALSE);
  return FALSE;
}
