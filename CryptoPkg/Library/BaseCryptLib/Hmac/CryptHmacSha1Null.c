/** @file
  HMAC-SHA1 Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA1 use.

  Return NULL to indicate this interface is not supported.

  @return  NULL  This interface is not supported..

**/
VOID *
EFIAPI
HmacSha1New (
  VOID
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Release the specified HMAC_CTX context.

  This function will do nothing.

  @param[in]  HmacSha1Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha1Free (
  IN  VOID  *HmacSha1Ctx
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha1Update().

  Return FALSE to indicate this interface is not supported.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1SetKey (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Makes a copy of an existing HMAC-SHA1 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Duplicate (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Digests the input data and updates HMAC-SHA1 context.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Update (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Completes computation of the HMAC-SHA1 digest value.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HmacValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Final (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  )
{
  ASSERT (FALSE);
  return FALSE;
}
