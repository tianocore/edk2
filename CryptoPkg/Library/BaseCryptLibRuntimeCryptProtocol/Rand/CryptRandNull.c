/** @file
  Pseudorandom Number Generator Wrapper Implementation which does not provide
  real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  @param[in]   Size    Size of randome bytes to generate.

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
