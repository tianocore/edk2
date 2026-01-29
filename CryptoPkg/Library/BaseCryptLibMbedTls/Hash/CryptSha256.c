/** @file
  SHA-256 Digest Wrapper Implementation over MbedTLS.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/sha256.h>

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 hash operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
  VOID
  )
{
  return (UINTN)(sizeof (mbedtls_sha256_context));
}

/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[out]  Sha256Context  Pointer to SHA-256 context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha256Init (
  OUT  VOID  *Sha256Context
  )
{
  INT32  Ret;

  if (Sha256Context == NULL) {
    return FALSE;
  }

  mbedtls_sha256_init (Sha256Context);

  Ret = mbedtls_sha256_starts (Sha256Context, FALSE);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Makes a copy of an existing SHA-256 context.

  If Sha256Context is NULL, then return FALSE.
  If NewSha256Context is NULL, then return FALSE.

  @param[in]  Sha256Context     Pointer to SHA-256 context being copied.
  @param[out] NewSha256Context  Pointer to new SHA-256 context.

  @retval TRUE   SHA-256 context copy succeeded.
  @retval FALSE  SHA-256 context copy failed.

**/
BOOLEAN
EFIAPI
Sha256Duplicate (
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  )
{
  if ((Sha256Context == NULL) || (NewSha256Context == NULL)) {
    return FALSE;
  }

  mbedtls_sha256_clone (NewSha256Context, Sha256Context);

  return TRUE;
}

/**
  Digests the input data and updates SHA-256 context.

  This function performs SHA-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be finalized
  by Sha256Final(). Behavior with invalid context is undefined.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  SHA-256 data digest failed.

**/
BOOLEAN
EFIAPI
Sha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  INT32  Ret;

  if (Sha256Context == NULL) {
    return FALSE;
  }

  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  Ret = mbedtls_sha256_update (Sha256Context, Data, DataSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Completes computation of the SHA-256 digest value.

  This function completes SHA-256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-256 context cannot
  be used again.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be
  finalized by Sha256Final(). Behavior with invalid SHA-256 context is undefined.

  If Sha256Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  )
{
  INT32  Ret;

  if ((Sha256Context == NULL) || (HashValue == NULL)) {
    return FALSE;
  }

  Ret = mbedtls_sha256_finish (Sha256Context, HashValue);
  mbedtls_sha256_free (Sha256Context);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Computes the SHA-256 message digest of a input data buffer.

  This function performs the SHA-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-256 digest
                           value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha256HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  INT32  Ret;

  if (HashValue == NULL) {
    return FALSE;
  }

  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  Ret = mbedtls_sha256 (Data, DataSize, HashValue, FALSE);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}
