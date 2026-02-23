/** @file
  ML-DSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/evp.h>
#include <crypto/ml_dsa.h>

/**
  Create the ML-DSA EVP_PKEY public key object from raw public key bytes.

  If Nid is not NID_ML_DSA_87, then return FALSE.

  @param[in]  RawPublicKey      Pointer to raw public key bytes.
  @param[in]  Nid               Nid for ML-DSA Category

  @retval Pointer to new ML-DSA public key object.
**/
VOID *
EFIAPI
MlDsaCreatePublicKeyObject (
  IN UINT8        *RawPublicKey,
  IN CONST UINTN  Nid
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
   Free the ML-DSA EVP_PKEY object

   @param[in]  PkeyContext  Pointer to the MLDSA EVP_PKEY object to be freed.

   @retval VOID
**/
VOID
EFIAPI
MlDsaFreePublicKeyObject (
  IN VOID  *PkeyContext
  )
{
  ASSERT (FALSE);
}

/**
   Verifies the ML-DSA signature.

   If PkeyContext is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataLen is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigLen is 0, then return FALSE.

   @param[in]  PkeyContext    Pointer to ML-DSA EVP_PKEY object.
   @param[in]  Nid            Nid for ML-DSA Category
   @param[in]  Data           Pointer to data being verified.
   @param[in]  DataLen        Length of data.
   @param[in]  Signature      Pointer to ML-DSA signature.
   @param[in]  SigLen         Length of ML-DSA signature.

   @retval  TRUE   Valid signature.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
MlDsaVerify (
  IN  CONST VOID   *PkeyContext,
  IN  UINTN        Nid,
  IN  CONST UINT8  *Data,
  IN  UINTN        DataLen,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigLen
  )
{
  ASSERT (FALSE);
  return FALSE;
}
