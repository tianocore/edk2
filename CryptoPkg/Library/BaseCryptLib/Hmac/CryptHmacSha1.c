/** @file
  HMAC-SHA1 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
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
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA1 operations.

  @return  The size, in bytes, of the context buffer required for HMAC-SHA1 operations.

**/
UINTN
EFIAPI
HmacSha1GetContextSize (
  VOID
  )
{
  //
  // Retrieves the OpenSSL HMAC-SHA1 Context Size
  //
  return (UINTN) (sizeof (HMAC_CTX));
}

/**
  Initializes user-supplied memory pointed by HmacSha1Context as HMAC-SHA1 context for
  subsequent use.

  If HmacSha1Context is NULL, then return FALSE.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context being initialized.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval TRUE   HMAC-SHA1 context initialization succeeded.
  @retval FALSE  HMAC-SHA1 context initialization failed.

**/
BOOLEAN
EFIAPI
HmacSha1Init (
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

  //
  // OpenSSL HMAC-SHA1 Context Initialization
  //
  HMAC_CTX_init (HmacSha1Context);
  HMAC_Init_ex (HmacSha1Context, Key, (UINT32) KeySize, EVP_sha1(), NULL);

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

  CopyMem (NewHmacSha1Context, HmacSha1Context, sizeof (HMAC_CTX));

  return TRUE;
}

/**
  Digests the input data and updates HMAC-SHA1 context.

  This function performs HMAC-SHA1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA1 context should be already correctly intialized by HmacSha1Init(), and should not
  be finalized by HmacSha1Final(). Behavior with invalid context is undefined.

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
  HMAC_Update (HmacSha1Context, Data, DataSize);

  return TRUE;
}

/**
  Completes computation of the HMAC-SHA1 digest value.

  This function completes HMAC-SHA1 digest computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA1 context cannot
  be used again.
  HMAC-SHA1 context should be already correctly intialized by HmacSha1Init(), and should
  not be finalized by HmacSha1Final(). Behavior with invalid HMAC-SHA1 context is undefined.

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
  HMAC_Final (HmacSha1Context, HmacValue, &Length);
  HMAC_CTX_cleanup (HmacSha1Context);

  return TRUE;
}
