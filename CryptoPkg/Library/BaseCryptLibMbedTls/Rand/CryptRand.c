/** @file
  Pseudorandom Number Generator Wrapper Implementation over MbedTLS.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <Library/RngLib.h>

/**
  Sets up the seed value for the pseudorandom number generator.

  This function sets up the seed value for the pseudorandom number generator.
  If Seed is not NULL, then the seed passed in is used.
  If Seed is NULL, then default seed is used.

  @param[in]  Seed      Pointer to seed value.
                        If NULL, default seed is used.
  @param[in]  SeedSize  Size of seed value.
                        If Seed is NULL, this parameter is ignored.

  @retval TRUE   Pseudorandom number generator has enough entropy for random generation.
  @retval FALSE  Pseudorandom number generator does not have enough entropy for random generation.

**/
BOOLEAN
EFIAPI
RandomSeed (
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  )
{
  return TRUE;
}

/**
  Generates a pseudorandom byte stream of the specified size.

  If Output is NULL, then return FALSE.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval TRUE   Pseudorandom byte stream generated successfully.
  @retval FALSE  Pseudorandom number generator fails to generate due to lack of entropy.

**/
BOOLEAN
EFIAPI
RandomBytes (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  )
{
  BOOLEAN          Ret;
  volatile UINT64  TempRand;

  //
  // Check input parameters.
  //
  if ((Output == NULL) || (Size > INT_MAX)) {
    return FALSE;
  }

  Ret = FALSE;

  while (Size > 0) {
    // Use RngLib to get random number
    Ret = GetRandomNumber64 ((UINT64 *)&TempRand);

    if (!Ret) {
      TempRand = 0;
      return Ret;
    }

    if (Size >= sizeof (TempRand)) {
      *((UINT64 *)Output) = TempRand;
      Output             += sizeof (UINT64);
      Size               -= sizeof (TempRand);
    } else {
      CopyMem (Output, (VOID *)&TempRand, Size);
      Size = 0;
    }
  }

  TempRand = 0;
  return Ret;
}

/**
  The MbedTLS function f_rng, which MbedtlsRand implements.

  @param[in]   RngState Not used, just for compatibility with mbedtls.
  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Len    Size of random bytes to generate.

  @retval 0      Pseudorandom byte stream generated successfully.
  @retval Non-0  Pseudorandom number generator fails to generate due to lack of entropy.
**/
INT32
MbedtlsRand (
  VOID   *RngState,
  UINT8  *Output,
  UINTN  Len
  )
{
  BOOLEAN  Result;

  Result = RandomBytes (Output, Len);

  return Result ? 0 : -1;
}
