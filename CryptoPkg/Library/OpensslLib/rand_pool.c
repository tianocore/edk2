/** @file
  OpenSSL_1_1_1b doesn't implement rand_pool_* functions for UEFI.
  The file implement these functions.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "crypto/rand.h"
#include <openssl/aes.h>

#include <Uefi.h>
#include <Library/RngLib.h>

/**
  Calls RandomNumber64 to fill
  a buffer of arbitrary size with random bytes.
  This is a shim layer to RngLib.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval True        Random bytes generation succeeded.
  @retval False       Failed to request random bytes.

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

  if (RandBuffer == NULL) {
    DEBUG((DEBUG_ERROR, "[OPENSSL_RAND_POOL] NULL RandBuffer. No random numbers are generated and your system is not secure\n"));
    ASSERT(FALSE); // Since we can't generate random numbers, we should assert. Otherwise we will just blow up later.
    return Ret;
  }


  while (Length > 0) {
    // Use RngLib to get random number
    Ret = GetRandomNumber64(&TempRand);

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
      Ret = RandGetBytes(bytes_needed, buffer);
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
  UINT8 data[16];
  RandGetBytes(sizeof(data), data);

  return rand_pool_add(pool, (unsigned char*)&data, sizeof(data), 0);
}

/*
 * Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_add_additional_data(RAND_POOL *pool)
{
  UINT8 data[16];
  RandGetBytes(sizeof(data), data);

  return rand_pool_add(pool, (unsigned char*)&data, sizeof(data), 0);
}

/*
 * Dummy Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_init(void)
{
  return 1;
}

/*
 * Dummy Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
void rand_pool_cleanup(void)
{
}

/*
 * Dummy Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
void rand_pool_keep_random_devices_open(int keep)
{
}
