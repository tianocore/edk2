/** @file
  HMAC-SHA256 Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA256 use.

  Return NULL to indicate this interface is not supported.

  @return  NULL  This interface is not supported..

**/
VOID *
EFIAPI
HmacSha256New (
  VOID
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Release the specified HMAC_CTX context.

  This function will do nothing.

  @param[in]  HmacSha256Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha256Free (
  IN  VOID  *HmacSha256Ctx
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha256Update().

  Return FALSE to indicate this interface is not supported.

  @param[out]  HmacSha256Context  Pointer to HMAC-SHA256 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256SetKey (
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing HMAC-SHA256 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  HmacSha256Context     Pointer to HMAC-SHA256 context being copied.
  @param[out] NewHmacSha256Context  Pointer to new HMAC-SHA256 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Duplicate (
  IN   CONST VOID  *HmacSha256Context,
  OUT  VOID        *NewHmacSha256Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates HMAC-SHA256 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  HmacSha256Context Pointer to the HMAC-SHA256 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Update (
  IN OUT  VOID        *HmacSha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the HMAC-SHA256 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  HmacSha256Context  Pointer to the HMAC-SHA256 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA256 digest
                                      value (32 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Final (
  IN OUT  VOID   *HmacSha256Context,
  OUT     UINT8  *HmacValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
