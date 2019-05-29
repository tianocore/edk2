/** @file
  OpenSSL_1_1_1b doesn't implement rand_pool_* functions for UEFI.
  The file implement these functions.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "internal/rand_int.h"
#include <openssl/aes.h>

#include <Uefi.h>
#include <Library/TimerLib.h>

#include "rand_pool_noise.h"

/**
  Get some randomness from low-order bits of GetPerformanceCounter results.
  And combine them to the 64-bit value

  @param[out] Rand    Buffer pointer to store the 64-bit random value.

  @retval TRUE        Random number generated successfully.
  @retval FALSE       Failed to generate.
**/
STATIC
BOOLEAN
EFIAPI
GetRandNoise64FromPerformanceCounter(
  OUT UINT64      *Rand
  )
{
  UINT32 Index;
  UINT32 *RandPtr;

  if (NULL == Rand) {
    return FALSE;
  }

  RandPtr = (UINT32 *) Rand;

  for (Index = 0; Index < 2; Index ++) {
    *RandPtr = (UINT32) (GetPerformanceCounter () & 0xFF);
    MicroSecondDelay (10);
    RandPtr++;
  }

  return TRUE;
}

/**
  Calls RandomNumber64 to fill
  a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
STATIC
BOOLEAN
EFIAPI
RandGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  )
{
  BOOLEAN     Ret;
  UINT64      TempRand;

  Ret = FALSE;

  while (Length > 0) {
    //
    // Get random noise from platform.
    // If it failed, fallback to PerformanceCounter
    // If you really care about security, you must override
    // GetRandomNoise64FromPlatform.
    //
    Ret = GetRandomNoise64 (&TempRand);
    if (Ret == FALSE) {
      Ret = GetRandNoise64FromPerformanceCounter (&TempRand);
    }
    if (!Ret) {
      return Ret;
    }
    if (Length >= sizeof (TempRand)) {
      *((UINT64*) RandBuffer) = TempRand;
      RandBuffer += sizeof (UINT64);
      Length -= sizeof (TempRand);
    } else {
      CopyMem (RandBuffer, &TempRand, Length);
      Length = 0;
    }
  }

  return Ret;
}

/**
  Creates a 128bit random value that is fully forward and backward prediction resistant,
  suitable for seeding a NIST SP800-90 Compliant.
  This function takes multiple random numbers from PerformanceCounter to ensure reseeding
  and performs AES-CBC-MAC over the data to compute the seed value.

  @param[out]  SeedBuffer    Pointer to a 128bit buffer to store the random seed.

  @retval TRUE        Random seed generation succeeded.
  @retval FALSE      Failed to request random bytes.

**/
STATIC
BOOLEAN
EFIAPI
RandGetSeed128 (
  OUT UINT8        *SeedBuffer
  )
{
  BOOLEAN     Ret;
  UINT8       RandByte[16];
  UINT8       Key[16];
  UINT8       Ffv[16];
  UINT8       Xored[16];
  UINT32      Index;
  UINT32      Index2;
  AES_KEY     AESKey;

  //
  // Chose an arbitary key and zero the feed_forward_value (FFV)
  //
  for (Index = 0; Index < 16; Index++) {
    Key[Index] = (UINT8) Index;
    Ffv[Index] = 0;
  }

  AES_set_encrypt_key (Key, 16 * 8, &AESKey);

  //
  // Perform CBC_MAC over 32 * 128 bit values, with 10us gaps between 128 bit value
  // The 10us gaps will ensure multiple reseeds within the system time with a large
  // design margin.
  //
  for (Index = 0; Index < 32; Index++) {
    MicroSecondDelay (10);
    Ret = RandGetBytes (16, RandByte);
    if (!Ret) {
      return Ret;
    }

    //
    // Perform XOR operations on two 128-bit value.
    //
    for (Index2 = 0; Index2 < 16; Index2++) {
      Xored[Index2] = RandByte[Index2] ^ Ffv[Index2];
    }

    AES_encrypt (Xored, Ffv, &AESKey);
  }

  for (Index = 0; Index < 16; Index++) {
    SeedBuffer[Index] = Ffv[Index];
  }

  return Ret;
}

/**
  Generate high-quality entropy source.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
STATIC
BOOLEAN
EFIAPI
RandGenerateEntropy (
  IN UINTN         Length,
  OUT UINT8        *Entropy
  )
{
  BOOLEAN     Ret;
  UINTN       BlockCount;
  UINT8       Seed[16];
  UINT8       *Ptr;

  BlockCount = Length / 16;
  Ptr        = (UINT8 *) Entropy;

  //
  // Generate high-quality seed for DRBG Entropy
  //
  while (BlockCount > 0) {
    Ret = RandGetSeed128 (Seed);
    if (!Ret) {
      return Ret;
    }
    CopyMem (Ptr, Seed, 16);

    BlockCount--;
    Ptr = Ptr + 16;
  }

  //
  // Populate the remained data as request.
  //
  Ret = RandGetSeed128 (Seed);
  if (!Ret) {
    return Ret;
  }
  CopyMem (Ptr, Seed, (Length % 16));

  return Ret;
}

/*
 * Add random bytes to the pool to acquire requested amount of entropy
 *
 * This function is platform specific and tries to acquire the requested
 * amount of entropy by polling platform specific entropy sources.
 *
 * This is OpenSSL required interface.
 */
size_t rand_pool_acquire_entropy(RAND_POOL *pool)
{
  BOOLEAN  Ret;
  size_t bytes_needed;
  unsigned char * buffer;

  bytes_needed = rand_pool_bytes_needed(pool, 1 /*entropy_factor*/);
  if (bytes_needed > 0) {
    buffer = rand_pool_add_begin(pool, bytes_needed);

    if (buffer != NULL) {
      Ret = RandGenerateEntropy(bytes_needed, buffer);
      if (FALSE == Ret) {
        rand_pool_add_end(pool, 0, 0);
      } else {
        rand_pool_add_end(pool, bytes_needed, 8 * bytes_needed);
      }
    }
  }

  return rand_pool_entropy_available(pool);
}

/*
 * Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_add_nonce_data(RAND_POOL *pool)
{
  struct {
    UINT64  Rand;
    UINT64  TimerValue;
  } data = { 0 };

  RandGetBytes(8, (UINT8 *)&(data.Rand));
  data.TimerValue = GetPerformanceCounter();

  return rand_pool_add(pool, (unsigned char*)&data, sizeof(data), 0);
}

/*
 * Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_add_additional_data(RAND_POOL *pool)
{
  struct {
    UINT64  Rand;
    UINT64  TimerValue;
  } data = { 0 };

  RandGetBytes(8, (UINT8 *)&(data.Rand));
  data.TimerValue = GetPerformanceCounter();

  return rand_pool_add(pool, (unsigned char*)&data, sizeof(data), 0);
}

/*
 * Dummy Implememtation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_init(void)
{
  return 1;
}

/*
 * Dummy Implememtation for UEFI
 *
 * This is OpenSSL required interface.
 */
void rand_pool_cleanup(void)
{
}

/*
 * Dummy Implememtation for UEFI
 *
 * This is OpenSSL required interface.
 */
void rand_pool_keep_random_devices_open(int keep)
{
}

