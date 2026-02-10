/** @file
  SLH-DSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/evp.h>
#include <crypto/slh_dsa.h>

/**
 Create the SLH-DSA EVP_PKEY public key object from raw public key bytes.

  If Nid is not NID_SLHDSASHAKE256S, then return FALSE.

  @param[in]  RawPublicKey  Pointer to raw public key bytes.
  @param[in]  Nid           Nid for SLH-DSA Category
  @param[out] PkeyContext   Pointer to the SLH-DSA EVP_PKEY object created.

  @retval TRUE   SLH-DSA EVP_PKEY public key object created successfully.
  @retval FALSE  Invalid input or SLH-DSA EVP_PKEY public key object creation failed.
**/
BOOLEAN
EFIAPI
SlhDsaCreatePublicKeyObject (
  IN     UINT8        *RawPublicKey,
  IN     CONST UINTN  Nid,
  IN OUT VOID         **PkeyContext
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
   Free the SLH-DSA EVP_PKEY object

   @param[in]  PkeyContext  Pointer to the SLHDSA EVP_PKEY object to be freed.

   @retval VOID
**/
VOID
EFIAPI
SlhDsaFreePublicKeyObject (
  IN VOID  *PkeyContext
  )
{
  ASSERT (FALSE);
}

/**
   Verifies the SLH-DSA signature.

   If PkeyContext is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataLen is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigLen is 0, then return FALSE.

   @param[in]  PkeyContext   Pointer to SLH-DSA EVP_PKEY object.
   @param[in]  Nid           Nid for SLH-DSA Category
   @param[in]  Data          Pointer to data being verified.
   @param[in]  DataLen       Length of data.
   @param[in]  Signature     Pointer to SLH-DSA signature.
   @param[in]  SigLen        Length of SLH-DSA signature.

   @retval  TRUE   Valid signature.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
SlhDsaVerify (
  IN CONST VOID   *PkeyContext,
  IN UINTN        Nid
  IN CONST UINT8  *Data,
  IN UINTN        DataLen,
  IN CONST UINT8  *Signature,
  IN UINTN        SigLen
  )
{
  ASSERT (FALSE);
  return FALSE;
}
