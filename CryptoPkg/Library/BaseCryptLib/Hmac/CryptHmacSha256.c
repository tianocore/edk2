/** @file
  HMAC-SHA256 Wrapper Implementation over OpenSSL.

Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA256 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha256New() returns NULL.

**/
VOID *
EFIAPI
HmacSha256New (
  VOID
  )
{
  //
  // Allocates & Initializes HMAC_CTX Context by OpenSSL HMAC_CTX_new()
  //
  return (VOID *)HMAC_CTX_new ();
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha256Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha256Free (
  IN  VOID  *HmacSha256Ctx
  )
{
  //
  // Free OpenSSL HMAC_CTX Context
  //
  HMAC_CTX_free ((HMAC_CTX *)HmacSha256Ctx);
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha256Update().

  If HmacSha256Context is NULL, then return FALSE.

  @param[out]  HmacSha256Context  Pointer to HMAC-SHA256 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.

**/
BOOLEAN
EFIAPI
HmacSha256SetKey (
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  //
  // Check input parameters.
  //
  if ((HmacSha256Context == NULL) || (KeySize > INT_MAX)) {
    return FALSE;
  }

  if (HMAC_Init_ex ((HMAC_CTX *)HmacSha256Context, Key, (UINT32)KeySize, EVP_sha256 (), NULL) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Makes a copy of an existing HMAC-SHA256 context.

  If HmacSha256Context is NULL, then return FALSE.
  If NewHmacSha256Context is NULL, then return FALSE.

  @param[in]  HmacSha256Context     Pointer to HMAC-SHA256 context being copied.
  @param[out] NewHmacSha256Context  Pointer to new HMAC-SHA256 context.

  @retval TRUE   HMAC-SHA256 context copy succeeded.
  @retval FALSE  HMAC-SHA256 context copy failed.

**/
BOOLEAN
EFIAPI
HmacSha256Duplicate (
  IN   CONST VOID  *HmacSha256Context,
  OUT  VOID        *NewHmacSha256Context
  )
{
  //
  // Check input parameters.
  //
  if ((HmacSha256Context == NULL) || (NewHmacSha256Context == NULL)) {
    return FALSE;
  }

  if (HMAC_CTX_copy ((HMAC_CTX *)NewHmacSha256Context, (HMAC_CTX *)HmacSha256Context) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Digests the input data and updates HMAC-SHA256 context.

  This function performs HMAC-SHA256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA256 context should be initialized by HmacSha256New(), and should not be finalized
  by HmacSha256Final(). Behavior with invalid context is undefined.

  If HmacSha256Context is NULL, then return FALSE.

  @param[in, out]  HmacSha256Context Pointer to the HMAC-SHA256 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA256 data digest succeeded.
  @retval FALSE  HMAC-SHA256 data digest failed.

**/
BOOLEAN
EFIAPI
HmacSha256Update (
  IN OUT  VOID        *HmacSha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (HmacSha256Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA256 digest update
  //
  if (HMAC_Update ((HMAC_CTX *)HmacSha256Context, Data, DataSize) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Completes computation of the HMAC-SHA256 digest value.

  This function completes HMAC-SHA256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA256 context cannot
  be used again.
  HMAC-SHA256 context should be initialized by HmacSha256New(), and should not be finalized
  by HmacSha256Final(). Behavior with invalid HMAC-SHA256 context is undefined.

  If HmacSha256Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.

  @param[in, out]  HmacSha256Context  Pointer to the HMAC-SHA256 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA256 digest
                                      value (32 bytes).

  @retval TRUE   HMAC-SHA256 digest computation succeeded.
  @retval FALSE  HMAC-SHA256 digest computation failed.

**/
BOOLEAN
EFIAPI
HmacSha256Final (
  IN OUT  VOID   *HmacSha256Context,
  OUT     UINT8  *HmacValue
  )
{
  UINT32  Length;

  //
  // Check input parameters.
  //
  if ((HmacSha256Context == NULL) || (HmacValue == NULL)) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA256 digest finalization
  //
  if (HMAC_Final ((HMAC_CTX *)HmacSha256Context, HmacValue, &Length) != 1) {
    return FALSE;
  }

  if (HMAC_CTX_reset ((HMAC_CTX *)HmacSha256Context) != 1) {
    return FALSE;
  }

  return TRUE;
}
