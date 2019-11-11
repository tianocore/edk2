/** @file
  OpenSSL_1_1_1b doesn't implement rand_pool_* functions for UEFI.
  The file implement these functions.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "internal/rand_int.h"
#include <openssl/aes.h>

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>

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
  BOOLEAN         ret;
  size_t          bytes_needed;
  size_t          len;
  unsigned char   *buffer;
  UINT64          data[2];

  bytes_needed = rand_pool_bytes_needed(pool, 1 /*entropy_factor*/);
  if (bytes_needed > 0) {
    buffer = rand_pool_add_begin(pool, bytes_needed);

    if (buffer != NULL) {
      ret = TRUE;
      while (bytes_needed > 0 && ret) {
        ret = GetRandomNumber128 (data);
        if (ret) {
          len = MIN (bytes_needed, sizeof(data));
          CopyMem (buffer, data, len);

          bytes_needed  -= len;
          buffer        += len;
        }
      }

      if (FALSE == ret) {
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
  UINT64    data[2];

  if (!GetRandomNumber128 (data)) {
    return 0;
  }

  return rand_pool_add(pool, (unsigned char*)&data, sizeof(data), 0);
}

/*
 * Implementation for UEFI
 *
 * This is OpenSSL required interface.
 */
int rand_pool_add_additional_data(RAND_POOL *pool)
{
  UINT64    data[2];

  if (!GetRandomNumber128 (data)) {
    return 0;
  }

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

