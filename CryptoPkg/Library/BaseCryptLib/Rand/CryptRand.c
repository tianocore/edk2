/** @file
  Pseudorandom Number Generator Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/rand.h>
#include <openssl/evp.h>

//
// Default seed for UEFI Crypto Library
//
CONST UINT8  DefaultSeed[] = "UEFI Crypto Library default seed";

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
  if (SeedSize > INT_MAX) {
    return FALSE;
  }

  //
  // The software PRNG implementation built in OpenSSL depends on message digest algorithm.
  // Make sure SHA-1 digest algorithm is available here.
  //
  if (EVP_add_digest (EVP_sha1 ()) == 0) {
    return FALSE;
  }

  //
  // Seed the pseudorandom number generator with user-supplied value.
  // NOTE: A cryptographic PRNG must be seeded with unpredictable data.
  //
  if (Seed != NULL) {
    RAND_seed (Seed, (UINT32) SeedSize);
  } else {
    RAND_seed (DefaultSeed, sizeof (DefaultSeed));
  }

  if (RAND_status () == 1) {
    return TRUE;
  }

  return FALSE;
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
  //
  // Check input parameters.
  //
  if (Output == NULL || Size > INT_MAX) {
    return FALSE;
  }

  //
  // Generate random data.
  //
  if (RAND_bytes (Output, (UINT32) Size) != 1) {
    return FALSE;
  }

  return TRUE;
}
