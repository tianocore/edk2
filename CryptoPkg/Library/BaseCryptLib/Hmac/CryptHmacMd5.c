/** @file
  HMAC-MD5 Wrapper Implementation over OpenSSL.

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
  Retrieves the size, in bytes, of the context buffer required for HMAC-MD5 operations.

  @return  The size, in bytes, of the context buffer required for HMAC-MD5 operations.

**/
UINTN
EFIAPI
HmacMd5GetContextSize (
  VOID
  )
{
  //
  // Retrieves the OpenSSL HMAC-MD5 Context Size
  //
  return (UINTN) (sizeof (HMAC_CTX));
}

/**
  Initializes user-supplied memory pointed by HmacMd5Context as HMAC-MD5 context for
  subsequent use.

  If HmacMd5Context is NULL, then return FALSE.

  @param[out]  HmacMd5Context  Pointer to HMAC-MD5 context being initialized.
  @param[in]   Key             Pointer to the user-supplied key.
  @param[in]   KeySize         Key size in bytes.

  @retval TRUE   HMAC-MD5 context initialization succeeded.
  @retval FALSE  HMAC-MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
HmacMd5Init (
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

  //
  // OpenSSL HMAC-MD5 Context Initialization
  //
  HMAC_CTX_init (HmacMd5Context);
  HMAC_Init_ex (HmacMd5Context, Key, (UINT32) KeySize, EVP_md5(), NULL);

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
  
  CopyMem (NewHmacMd5Context, HmacMd5Context, sizeof (HMAC_CTX));

  return TRUE;
}

/**
  Digests the input data and updates HMAC-MD5 context.

  This function performs HMAC-MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-MD5 context should be already correctly intialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid context is undefined.

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
  HMAC_Update (HmacMd5Context, Data, DataSize);

  return TRUE;
}

/**
  Completes computation of the HMAC-MD5 digest value.

  This function completes HMAC-MD5 digest computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-MD5 context cannot
  be used again.
  HMAC-MD5 context should be already correctly intialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid HMAC-MD5 context is undefined.

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
  HMAC_Final (HmacMd5Context, HmacValue, &Length);
  HMAC_CTX_cleanup (HmacMd5Context);

  return TRUE;
}
