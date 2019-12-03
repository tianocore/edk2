/** @file
  HMAC-SHA1 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

//
// NOTE: OpenSSL redefines the size of HMAC_CTX at crypto/hmac/hmac_lcl.h
//       #define HMAC_MAX_MD_CBLOCK_SIZE     144
//
//
#define  HMAC_SHA1_CTX_SIZE   (sizeof(void *) * 4 + sizeof(unsigned int) + \
                             sizeof(unsigned char) * 144)

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA1 operations.
  (NOTE: This API is deprecated.
         Use HmacSha1New() / HmacSha1Free() for HMAC-SHA1 Context operations.)

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
  // NOTE: HMAC_CTX object was made opaque in openssl-1.1.x, here we just use the
  //       fixed size as a workaround to make this API work for compatibility.
  //       We should retire HmacSha15GetContextSize() in future, and use HmacSha1New()
  //       and HmacSha1Free() for context allocation and release.
  //
  return (UINTN) HMAC_SHA1_CTX_SIZE;
}

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
  memset(HmacSha1Context, 0, HMAC_SHA1_CTX_SIZE);
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacSha1Context) != 1) {
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
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should not
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
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should
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
  if (HMAC_Final ((HMAC_CTX *)HmacSha1Context, HmacValue, &Length) != 1) {
    return FALSE;
  }
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacSha1Context) != 1) {
    return FALSE;
  }

  return TRUE;
}
