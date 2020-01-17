/** @file
  HMAC-MD5 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-MD5 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacMd5New() returns NULL.

**/
VOID *
EFIAPI
HmacMd5New (
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

  @param[in]  HmacMd5Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacMd5Free (
  IN  VOID  *HmacMd5Ctx
  )
{
  //
  // Free OpenSSL HMAC_CTX Context
  //
  HMAC_CTX_free ((HMAC_CTX *)HmacMd5Ctx);
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacMd5Update().

  If HmacMd5Context is NULL, then return FALSE.

  @param[out]  HmacMd5Context  Pointer to HMAC-MD5 context.
  @param[in]   Key             Pointer to the user-supplied key.
  @param[in]   KeySize         Key size in bytes.

  @retval TRUE   Key is set successfully.
  @retval FALSE  Key is set unsuccessfully.

**/
BOOLEAN
EFIAPI
HmacMd5SetKey (
  OUT  VOID         *HmacMd5Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  //
  // Check input parameters.
  //
  if (HmacMd5Context == NULL || KeySize > INT_MAX) {
    return FALSE;
  }

  if (HMAC_Init_ex ((HMAC_CTX *)HmacMd5Context, Key, (UINT32) KeySize, EVP_md5(), NULL) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Makes a copy of an existing HMAC-MD5 context.

  If HmacMd5Context is NULL, then return FALSE.
  If NewHmacMd5Context is NULL, then return FALSE.

  @param[in]  HmacMd5Context     Pointer to HMAC-MD5 context being copied.
  @param[out] NewHmacMd5Context  Pointer to new HMAC-MD5 context.

  @retval TRUE   HMAC-MD5 context copy succeeded.
  @retval FALSE  HMAC-MD5 context copy failed.

**/
BOOLEAN
EFIAPI
HmacMd5Duplicate (
  IN   CONST VOID  *HmacMd5Context,
  OUT  VOID        *NewHmacMd5Context
  )
{
  //
  // Check input parameters.
  //
  if (HmacMd5Context == NULL || NewHmacMd5Context == NULL) {
    return FALSE;
  }

  if (HMAC_CTX_copy ((HMAC_CTX *)NewHmacMd5Context, (HMAC_CTX *)HmacMd5Context) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Digests the input data and updates HMAC-MD5 context.

  This function performs HMAC-MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-MD5 context should be initialized by HmacMd5New(), and should not be finalized by
  HmacMd5Final(). Behavior with invalid context is undefined.

  If HmacMd5Context is NULL, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-MD5 data digest succeeded.
  @retval FALSE  HMAC-MD5 data digest failed.

**/
BOOLEAN
EFIAPI
HmacMd5Update (
  IN OUT  VOID        *HmacMd5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (HmacMd5Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL
  //
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-MD5 digest update
  //
  if (HMAC_Update ((HMAC_CTX *)HmacMd5Context, Data, DataSize) != 1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Completes computation of the HMAC-MD5 digest value.

  This function completes HMAC-MD5 digest computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-MD5 context cannot
  be used again.
  HMAC-MD5 context should be initialized by HmacMd5New(), and should not be finalized by
  HmacMd5Final(). Behavior with invalid HMAC-MD5 context is undefined.

  If HmacMd5Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[out]      HmacValue       Pointer to a buffer that receives the HMAC-MD5 digest
                                   value (16 bytes).

  @retval TRUE   HMAC-MD5 digest computation succeeded.
  @retval FALSE  HMAC-MD5 digest computation failed.

**/
BOOLEAN
EFIAPI
HmacMd5Final (
  IN OUT  VOID   *HmacMd5Context,
  OUT     UINT8  *HmacValue
  )
{
  UINT32  Length;

  //
  // Check input parameters.
  //
  if (HmacMd5Context == NULL || HmacValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-MD5 digest finalization
  //
  if (HMAC_Final ((HMAC_CTX *)HmacMd5Context, HmacValue, &Length) != 1) {
    return FALSE;
  }
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacMd5Context) != 1) {
    return FALSE;
  }

  return TRUE;
}
