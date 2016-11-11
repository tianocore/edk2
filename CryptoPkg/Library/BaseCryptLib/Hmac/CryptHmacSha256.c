/** @file
  HMAC-SHA256 Wrapper Implementation over OpenSSL.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA256 operations.

  @return  The size, in bytes, of the context buffer required for HMAC-SHA256 operations.

**/
UINTN
EFIAPI
HmacSha256GetContextSize (
  VOID
  )
{
  //
  // Retrieves the OpenSSL HMAC-SHA256 Context Size
  //
  return (UINTN) (sizeof (HMAC_CTX));
}

/**
  Initializes user-supplied memory pointed by HmacSha256Context as HMAC-SHA256 context for
  subsequent use.

  If HmacSha256Context is NULL, then return FALSE.

  @param[out]  HmacSha256Context  Pointer to HMAC-SHA256 context being initialized.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   HMAC-SHA256 context initialization succeeded.
  @retval FALSE  HMAC-SHA256 context initialization failed.

**/
BOOLEAN
EFIAPI
HmacSha256Init (
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  //
  // Check input parameters.
  //
  if (HmacSha256Context == NULL || KeySize > INT_MAX) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA256 Context Initialization
  //
  HMAC_CTX_init (HmacSha256Context);
  HMAC_Init_ex (HmacSha256Context, Key, (UINT32) KeySize, EVP_sha256(), NULL);

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
  if (HmacSha256Context == NULL || NewHmacSha256Context == NULL) {
    return FALSE;
  }

  CopyMem (NewHmacSha256Context, HmacSha256Context, sizeof (HMAC_CTX));

  return TRUE;
}

/**
  Digests the input data and updates HMAC-SHA256 context.

  This function performs HMAC-SHA256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA256 context should be already correctly initialized by HmacSha256Init(), and should not
  be finalized by HmacSha256Final(). Behavior with invalid context is undefined.

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
  if (Data == NULL && DataSize != 0) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA256 digest update
  //
  HMAC_Update (HmacSha256Context, Data, DataSize);

  return TRUE;
}

/**
  Completes computation of the HMAC-SHA256 digest value.

  This function completes HMAC-SHA256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA256 context cannot
  be used again.
  HMAC-SHA256 context should be already correctly initialized by HmacSha256Init(), and should
  not be finalized by HmacSha256Final(). Behavior with invalid HMAC-SHA256 context is undefined.

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
  if (HmacSha256Context == NULL || HmacValue == NULL) {
    return FALSE;
  }

  //
  // OpenSSL HMAC-SHA256 digest finalization
  //
  HMAC_Final (HmacSha256Context, HmacValue, &Length);
  HMAC_CTX_cleanup (HmacSha256Context);

  return TRUE;
}
