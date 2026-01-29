/** @file
  HMAC-SHA256 Wrapper Implementation over MbedTLS.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/md.h>

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-MD use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacShaMdNew() returns NULL.

**/
STATIC
VOID *
HmacMdNew (
  VOID
  )
{
  VOID  *HmacMdCtx;

  HmacMdCtx = AllocateZeroPool (sizeof (mbedtls_md_context_t));
  if (HmacMdCtx == NULL) {
    return NULL;
  }

  return HmacMdCtx;
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacMdCtx  Pointer to the HMAC_CTX context to be released.

**/
VOID
HmacMdFree (
  IN  VOID  *HmacMdCtx
  )
{
  mbedtls_md_free (HmacMdCtx);
  if (HmacMdCtx != NULL) {
    FreePool (HmacMdCtx);
  }
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacMdUpdate().

  If HmacMdContext is NULL, then return FALSE.

  @param[in]   MdType             Message Digest Type.
  @param[out]  HmacMdContext      Pointer to HMAC-MD context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.

**/
STATIC
BOOLEAN
HmacMdSetKey (
  IN   mbedtls_md_type_t  MdType,
  OUT  VOID               *HmacMdContext,
  IN   CONST UINT8        *Key,
  IN   UINTN              KeySize
  )
{
  const mbedtls_md_info_t  *md_info;
  INT32                    Ret;

  if ((HmacMdContext == NULL) || (KeySize > INT_MAX)) {
    return FALSE;
  }

  ZeroMem (HmacMdContext, sizeof (mbedtls_md_context_t));
  mbedtls_md_init (HmacMdContext);

  md_info = mbedtls_md_info_from_type (MdType);
  ASSERT (md_info != NULL);

  Ret = mbedtls_md_setup (HmacMdContext, md_info, 1);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_md_hmac_starts (HmacMdContext, Key, KeySize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Return block size in md_type.

  @param[in]  MdType            message digest Type.

  @retval blocksize in md_type.

**/
int
HmacMdGetBlockSize (
  mbedtls_md_type_t  MdType
  )
{
  switch (MdType) {
    case MBEDTLS_MD_SHA256:
      return 64;
    case MBEDTLS_MD_SHA384:
      return 128;
    default:
      ASSERT (FALSE);
      return 0;
  }
}

/**
  Makes a copy of an existing HMAC-MD context.

  If HmacMdContext is NULL, then return FALSE.
  If NewHmacMdContext is NULL, then return FALSE.

  @param[in]  MdType            message digest Type.
  @param[in]  HmacMdContext     Pointer to HMAC-MD context being copied.
  @param[out] NewHmacMdContext  Pointer to new HMAC-MD context.

  @retval TRUE   HMAC-MD context copy succeeded.
  @retval FALSE  HMAC-MD context copy failed.

**/
STATIC
BOOLEAN
HmacMdDuplicate (
  IN   CONST mbedtls_md_type_t  MdType,
  IN   CONST VOID               *HmacMdContext,
  OUT  VOID                     *NewHmacMdContext
  )
{
  INT32                    Ret;
  CONST mbedtls_md_info_t  *md_info;
  mbedtls_md_context_t     *MdContext;

  if ((HmacMdContext == NULL) || (NewHmacMdContext == NULL)) {
    return FALSE;
  }

  ZeroMem (NewHmacMdContext, sizeof (mbedtls_md_context_t));
  mbedtls_md_init (NewHmacMdContext);
  md_info = mbedtls_md_info_from_type (MdType);
  ASSERT (md_info != NULL);

  Ret = mbedtls_md_setup (NewHmacMdContext, md_info, 1);
  if (Ret != 0) {
    return FALSE;
  }

  MdContext = (mbedtls_md_context_t *)NewHmacMdContext;

  Ret = mbedtls_md_clone (NewHmacMdContext, HmacMdContext);
  if (Ret != 0) {
    if (MdContext->md_ctx != NULL) {
      mbedtls_free (MdContext->md_ctx);
    }

    if (MdContext->hmac_ctx != NULL) {
      mbedtls_free (MdContext->hmac_ctx);
    }

    return FALSE;
  }

  CopyMem (
    ((mbedtls_md_context_t *)NewHmacMdContext)->hmac_ctx,
    ((CONST mbedtls_md_context_t *)HmacMdContext)->hmac_ctx,
    HmacMdGetBlockSize (MdType) * 2
    );

  return TRUE;
}

/**
  Digests the input data and updates HMAC-MD context.

  This function performs HMAC-MD digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-MD context should be initialized by HmacMdNew(), and should not be finalized
  by HmacMdFinal(). Behavior with invalid context is undefined.

  If HmacMdContext is NULL, then return FALSE.

  @param[in, out]  HmacMdContext     Pointer to the HMAC-MD context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-MD data digest succeeded.
  @retval FALSE  HMAC-MD data digest failed.

**/
STATIC
BOOLEAN
HmacMdUpdate (
  IN OUT  VOID        *HmacMdContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  INT32  Ret;

  if (HmacMdContext == NULL) {
    return FALSE;
  }

  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  if (DataSize > INT_MAX) {
    return FALSE;
  }

  Ret = mbedtls_md_hmac_update (HmacMdContext, Data, DataSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Completes computation of the HMAC-MD digest value.

  This function completes HMAC-MD hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-MD context cannot
  be used again.
  HMAC-MD context should be initialized by HmacMdNew(), and should not be finalized
  by HmacMdFinal(). Behavior with invalid HMAC-MD context is undefined.

  If HmacMdContext is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.

  @param[in, out]  HmacMdContext      Pointer to the HMAC-MD context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-MD digest
                                      value.

  @retval TRUE   HMAC-MD digest computation succeeded.
  @retval FALSE  HMAC-MD digest computation failed.

**/
STATIC
BOOLEAN
HmacMdFinal (
  IN OUT  VOID   *HmacMdContext,
  OUT     UINT8  *HmacValue
  )
{
  INT32  Ret;

  if ((HmacMdContext == NULL) || (HmacValue == NULL)) {
    return FALSE;
  }

  Ret = mbedtls_md_hmac_finish (HmacMdContext, HmacValue);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_md_hmac_reset (HmacMdContext);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Computes the HMAC-MD digest of a input data buffer.

  This function performs the HMAC-MD digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   MdType      Message Digest Type.
  @param[in]   Data        Pointer to the buffer containing the data to be digested.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[in]   Key         Pointer to the user-supplied key.
  @param[in]   KeySize     Key size in bytes.
  @param[out]  HmacValue   Pointer to a buffer that receives the HMAC-MD digest
                           value.

  @retval TRUE   HMAC-MD digest computation succeeded.
  @retval FALSE  HMAC-MD digest computation failed.
  @retval FALSE  This interface is not supported.

**/
STATIC
BOOLEAN
HmacMdAll (
  IN   mbedtls_md_type_t  MdType,
  IN   CONST VOID         *Data,
  IN   UINTN              DataSize,
  IN   CONST UINT8        *Key,
  IN   UINTN              KeySize,
  OUT  UINT8              *HmacValue
  )
{
  const mbedtls_md_info_t  *md_info;
  INT32                    Ret;

  md_info = mbedtls_md_info_from_type (MdType);
  ASSERT (md_info != NULL);

  Ret = mbedtls_md_hmac (md_info, Key, KeySize, Data, DataSize, HmacValue);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

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
  return HmacMdNew ();
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
  HmacMdFree (HmacSha256Ctx);
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
  return HmacMdSetKey (MBEDTLS_MD_SHA256, HmacSha256Context, Key, KeySize);
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
  return HmacMdDuplicate (MBEDTLS_MD_SHA256, HmacSha256Context, NewHmacSha256Context);
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
  return HmacMdUpdate (HmacSha256Context, Data, DataSize);
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
  return HmacMdFinal (HmacSha256Context, HmacValue);
}

/**
  Computes the HMAC-SHA256 digest of a input data buffer.

  This function performs the HMAC-SHA256 digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be digested.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[in]   Key         Pointer to the user-supplied key.
  @param[in]   KeySize     Key size in bytes.
  @param[out]  HmacValue   Pointer to a buffer that receives the HMAC-SHA256 digest
                           value (32 bytes).

  @retval TRUE   HMAC-SHA256 digest computation succeeded.
  @retval FALSE  HMAC-SHA256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256All (
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  )
{
  return HmacMdAll (MBEDTLS_MD_SHA256, Data, DataSize, Key, KeySize, HmacValue);
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA384 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha384New() returns NULL.

**/
VOID *
EFIAPI
HmacSha384New (
  VOID
  )
{
  return HmacMdNew ();
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha384Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha384Free (
  IN  VOID  *HmacSha384Ctx
  )
{
  HmacMdFree (HmacSha384Ctx);
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha384Update().

  If HmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha384Context  Pointer to HMAC-SHA384 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha384SetKey (
  OUT  VOID         *HmacSha384Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return HmacMdSetKey (MBEDTLS_MD_SHA384, HmacSha384Context, Key, KeySize);
}

/**
  Makes a copy of an existing HMAC-SHA384 context.

  If HmacSha384Context is NULL, then return FALSE.
  If NewHmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha384Context     Pointer to HMAC-SHA384 context being copied.
  @param[out] NewHmacSha384Context  Pointer to new HMAC-SHA384 context.

  @retval TRUE   HMAC-SHA384 context copy succeeded.
  @retval FALSE  HMAC-SHA384 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha384Duplicate (
  IN   CONST VOID  *HmacSha384Context,
  OUT  VOID        *NewHmacSha384Context
  )
{
  return HmacMdDuplicate (MBEDTLS_MD_SHA384, HmacSha384Context, NewHmacSha384Context);
}

/**
  Digests the input data and updates HMAC-SHA384 context.

  This function performs HMAC-SHA384 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA384 context should be initialized by HmacSha384New(), and should not be finalized
  by HmacSha384Final(). Behavior with invalid context is undefined.

  If HmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha384Context Pointer to the HMAC-SHA384 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA384 data digest succeeded.
  @retval FALSE  HMAC-SHA384 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha384Update (
  IN OUT  VOID        *HmacSha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return HmacMdUpdate (HmacSha384Context, Data, DataSize);
}

/**
  Completes computation of the HMAC-SHA384 digest value.

  This function completes HMAC-SHA384 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA384 context cannot
  be used again.
  HMAC-SHA384 context should be initialized by HmacSha384New(), and should not be finalized
  by HmacSha384Final(). Behavior with invalid HMAC-SHA384 context is undefined.

  If HmacSha384Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha384Context  Pointer to the HMAC-SHA384 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA384 digest
                                      value (48 bytes).

  @retval TRUE   HMAC-SHA384 digest computation succeeded.
  @retval FALSE  HMAC-SHA384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha384Final (
  IN OUT  VOID   *HmacSha384Context,
  OUT     UINT8  *HmacValue
  )
{
  return HmacMdFinal (HmacSha384Context, HmacValue);
}

/**
  Computes the HMAC-SHA384 digest of a input data buffer.

  This function performs the HMAC-SHA384 digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be digested.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[in]   Key         Pointer to the user-supplied key.
  @param[in]   KeySize     Key size in bytes.
  @param[out]  HmacValue   Pointer to a buffer that receives the HMAC-SHA384 digest
                           value (48 bytes).

  @retval TRUE   HMAC-SHA384 digest computation succeeded.
  @retval FALSE  HMAC-SHA384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha384All (
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  )
{
  return HmacMdAll (MBEDTLS_MD_SHA384, Data, DataSize, Key, KeySize, HmacValue);
}
