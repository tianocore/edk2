/** @file
  HMAC-MD5 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/hmac.h>

//
// NOTE: OpenSSL redefines the size of HMAC_CTX at crypto/hmac/hmac_lcl.h
//       #define HMAC_MAX_MD_CBLOCK_SIZE     144
//
#define HMAC_MD5_CTX_SIZE    (sizeof(void *) * 4 + sizeof(unsigned int) + \
                             sizeof(unsigned char) * 144)

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-MD5 operations.
  (NOTE: This API is deprecated.
         Use HmacMd5New() / HmacMd5Free() for HMAC-MD5 Context operations.)

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
  // NOTE: HMAC_CTX object was made opaque in openssl-1.1.x, here we just use the
  //       fixed size as a workaround to make this API work for compatibility.
  //       We should retire HmacMd5GetContextSize() in future, and use HmacMd5New()
  //       and HmacMd5Free() for context allocation and release.
  //
  return (UINTN) HMAC_MD5_CTX_SIZE;
}

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
  memset(HmacMd5Context, 0, HMAC_MD5_CTX_SIZE);
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacMd5Context) != 1) {
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
  HMAC-MD5 context should be already correctly initialized by HmacMd5Init(), and should not be
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
  HMAC-MD5 context should be already correctly initialized by HmacMd5Init(), and should not be
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
  if (HMAC_Final ((HMAC_CTX *)HmacMd5Context, HmacValue, &Length) != 1) {
    return FALSE;
  }
  if (HMAC_CTX_reset ((HMAC_CTX *)HmacMd5Context) != 1) {
    return FALSE;
  }

  return TRUE;
}
