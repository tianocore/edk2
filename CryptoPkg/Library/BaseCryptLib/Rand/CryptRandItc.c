/** @file
  Pseudorandom Number Generator Wrapper Implementation over OpenSSL.

Copyright (c) 2012 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <Library/PrintLib.h>

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
  CHAR8  DefaultSeed[128];

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
    //
    // Retrieve current time.
    //
    AsciiSPrint (
      DefaultSeed,
      sizeof (DefaultSeed),
      "UEFI Crypto Library default seed (%ld)",
      AsmReadItc ()
      ); 

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
  @param[in]   Size    Size of randome bytes to generate.

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
