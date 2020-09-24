/** @file
  Support routines for RDRAND instruction access.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/RngLib.h>

#include "RdRand.h"
#include "AesCore.h"

/**
  Calls RDRAND to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RdRandGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  )
{
  BOOLEAN     IsRandom;
  UINT64      TempRand[2];

  while (Length > 0) {
    IsRandom = GetRandomNumber128 (TempRand);
    if (!IsRandom) {
      return EFI_NOT_READY;
    }
    if (Length >= sizeof (TempRand)) {
      WriteUnaligned64 ((UINT64*)RandBuffer, TempRand[0]);
      RandBuffer += sizeof (UINT64);
      WriteUnaligned64 ((UINT64*)RandBuffer, TempRand[1]);
      RandBuffer += sizeof (UINT64);
      Length -= sizeof (TempRand);
    } else {
      CopyMem (RandBuffer, TempRand, Length);
      Length = 0;
    }
  }

  return EFI_SUCCESS;
}

/**
  Creates a 128bit random value that is fully forward and backward prediction resistant,
  suitable for seeding a NIST SP800-90 Compliant, FIPS 1402-2 certifiable SW DRBG.
  This function takes multiple random numbers through RDRAND without intervening
  delays to ensure reseeding and performs AES-CBC-MAC over the data to compute the
  seed value.

  @param[out]  SeedBuffer    Pointer to a 128bit buffer to store the random seed.

  @retval EFI_SUCCESS        Random seed generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RdRandGetSeed128 (
  OUT UINT8        *SeedBuffer
  )
{
  EFI_STATUS  Status;
  UINT8       RandByte[16];
  UINT8       Key[16];
  UINT8       Ffv[16];
  UINT8       Xored[16];
  UINT32      Index;
  UINT32      Index2;

  //
  // Chose an arbitrary key and zero the feed_forward_value (FFV)
  //
  for (Index = 0; Index < 16; Index++) {
    Key[Index] = (UINT8) Index;
    Ffv[Index] = 0;
  }

  //
  // Perform CBC_MAC over 32 * 128 bit values, with 10us gaps between 128 bit value
  // The 10us gaps will ensure multiple reseeds within the HW RNG with a large design margin.
  //
  for (Index = 0; Index < 32; Index++) {
    MicroSecondDelay (10);
    Status = RdRandGetBytes (16, RandByte);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Perform XOR operations on two 128-bit value.
    //
    for (Index2 = 0; Index2 < 16; Index2++) {
      Xored[Index2] = RandByte[Index2] ^ Ffv[Index2];
    }

    AesEncrypt (Key, Xored, Ffv);
  }

  for (Index = 0; Index < 16; Index++) {
    SeedBuffer[Index] = Ffv[Index];
  }

  return EFI_SUCCESS;
}

/**
  Generate high-quality entropy source through RDRAND.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
EFI_STATUS
EFIAPI
RdRandGenerateEntropy (
  IN UINTN         Length,
  OUT UINT8        *Entropy
  )
{
  EFI_STATUS  Status;
  UINTN       BlockCount;
  UINT8       Seed[16];
  UINT8       *Ptr;

  Status     = EFI_NOT_READY;
  BlockCount = Length / 16;
  Ptr        = (UINT8 *)Entropy;

  //
  // Generate high-quality seed for DRBG Entropy
  //
  while (BlockCount > 0) {
    Status = RdRandGetSeed128 (Seed);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (Ptr, Seed, 16);

    BlockCount--;
    Ptr = Ptr + 16;
  }

  //
  // Populate the remained data as request.
  //
  Status = RdRandGetSeed128 (Seed);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  CopyMem (Ptr, Seed, (Length % 16));

  return Status;
}
