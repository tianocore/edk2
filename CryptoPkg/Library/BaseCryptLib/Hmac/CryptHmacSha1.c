/** @file
  HMAC-SHA1 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA1 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha1New() returns NULL.

**/
VOID *
EFIAPI
HmacSha1New (
  VOID
  )
{
  //
  // Allocates & Initializes HMAC_CTX Context by OpenSSL HMAC_CTX_new()
  //
  return (VOID *) HMAC_CTX_new ();
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha1Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha1Free (
  IN  VOID  *HmacSha1Ctx
  )
{
  //
  // Free OpenSSL HMAC_CTX Context
  //
  HMAC_CTX_free ((HMAC_CTX *)HmacSha1Ctx);
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha1Update().

  If HmacSha1Context is NULL, then return FALSE.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.

**/
BOOLEAN
EFIAPI
HmacSha1SetKey (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  //
  // Check input parameters.
  //
  if (HmacSha1Context == NULL || KeySize > INT_MAX) {
    return FALSE;
  }

  if (HMAC_Init_ex ((HMAC_CTX *)HmacSha1Context, Key, (UINT32) KeySize, EVP_sha1(), NULL) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Makes a copy of an existing HMAC-SHA1 context.

  If HmacSha1Context is NULL, then return FALSE.
  If NewHmacSha1Context is NULL, then return FALSE.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval TRUE   HMAC-SHA1 context copy succeeded.
  @retval FALSE  HMAC-SHA1 context copy failed.

**/
BOOLEAN
EFIAPI
HmacSha1Duplicate (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  )
{
  //
  // Check input parameters.
  //
  if (HmacSha1Context == NULL || NewHmacSha1Context == NULL) {
    return FALSE;
  }

  if (HMAC_CTX_copy ((HMAC_CTX *)NewHmacSha1Context, (HMAC_CTX *)HmacSha1Context) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Digests the input data and updates HMAC-SHA1 context.

  This function performs HMAC-SHA1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA1 context should be initialized by HmacSha1New(), and should not be finalized by
  HmacSha1Final(). Behavior with invalid context is undefined.

  If HmacSha1Context is NULL, then return FALSE.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA1 data digest succeeded.
  @retval FALSE  HMAC-SHA1 data digest failed.

**/
BOOLEAN
EFIAPI
HmacSha1Update (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (HmacSha1Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA1 digest update
  //
  if (HMAC_Update ((HMAC_CTX *)HmacSha1Context, Data, DataSize) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Completes computation of the HMAC-SHA1 digest value.

  This function completes HMAC-SHA1 digest computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA1 context cannot
  be used again.
  HMAC-SHA1 context should be initialized by HmacSha1New(), and should not be finalized by
  HmacSha1Final(). Behavior with invalid HMAC-SHA1 context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HmacValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval TRUE   HMAC-SHA1 digest computation succeeded.
  @retval FALSE  HMAC-SHA1 digest computation failed.

**/
BOOLEAN
EFIAPI
HmacSha1Final (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  )
{
  UINT32  Length;

  //
  // Check input parameters.
  //
  if (HmacSha1Context == NULL || HmacValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA1 digest finalization
  //
  if (HMAC_Final ((HMAC_CTX *)HmacSha1Context, HmacValue, &Length) != 1) {
    return FALSE;
  }
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacSha1Context) != 1) {
    return FALSE;
  }

  return TRUE;
}
