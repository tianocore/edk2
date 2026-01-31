/** @file
  ML-DSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/objects.h>
#include <crypto/ml_dsa.h>

/**
  Release all MLDSA resources

  @param[in]  LibCtx      Pointer to OSSL_LIB_CTX object.
  @param[in]  PublicKey   Pointer to MLDSA EVP_PKEY object.
  @param[in]  mctx        Pointer to EVP_MD_CTX object.
  @param[in]  SigAlg      Pointer to EVP_SIGNATURE object.
**/
STATIC
VOID
ReleaseMlDsaResources (
  EVP_PKEY_CTX   *vctx,
  EVP_SIGNATURE  *SigAlg
  )
{
  if (SigAlg != NULL) {
    EVP_SIGNATURE_free (SigAlg);
  }

  if (vctx != NULL) {
    EVP_PKEY_CTX_free (vctx);
  }
}

/**
  Convert Crypto NID to OpenSSL NID.

  @param[in]  Nid   Crypto NID.

  @return OpenSSL NID.
**/
STATIC
INT32
CryptoNidToOpensslNid (
  IN UINTN  Nid
  )
{
  INT32  NidValue;

  switch (Nid) {
    case CRYPTO_NID_ML_DSA_87:
      NidValue = NID_ML_DSA_87;
      break;
    default:
      NidValue = NID_undef;
      break;
  }

  return NidValue;
}

/**
  Get MLDSA public key length by NID.

  @param[in]  Nid            Crypto NID.
  @param[out] PublicKeyLen   Pointer to receive MLDSA public key length.

  @retval TRUE   Public key length is retrieved.
  @retval FALSE  Unsupported NID.
**/
STATIC
BOOLEAN
NidToPublicKeySize (
  IN UINTN   Nid,
  OUT UINTN  *KeySize
  )
{
  switch (Nid) {
    case NID_ML_DSA_87:
      *KeySize = ML_DSA_87_PUB_LEN;
      break;
    default:
      *KeySize = 0;
      return FALSE;
  }

  return TRUE;
}

/**
 Convert NID to ML-DSA Signature Algorithm Name

  If Nid is not NID_MLDSA_87, then return FALSE.

  @param[in]  Nid  Numeric identifier value to denote ML-DSA Signature Algorithm.
  @param[out] Alg  Pointer to ML-DSA Signature Algorithm Name.

  @return     TRUE if the Crypto NID is recognized and Alg is returned.
              FALSE if the Crypto NID is not recognized.
**/
STATIC
BOOLEAN
NidToSignatureAlgorithmName (
  IN     UINTN  Nid,
  OUT    CHAR8  **Alg
  )
{
  switch (Nid) {
    case NID_ML_DSA_87:
      *Alg = LN_ML_DSA_87;
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

/**
  Create the ML-DSA EVP_PKEY public key object from raw public key bytes.

  If Pkey is NULL, then return FALSE.
  If RawPublicKey is NULL, then return FALSE.
  If Nid is not NID_ML_DSA_87, then return FALSE.

  @param[in]  RawPublicKey  Pointer to raw public key bytes.
  @param[out] Pkey          Pointer to the ML-DSA EVP_PKEY object created.
  @param[in]  Nid           Nid for ML-DSA Category

  @retval TRUE   ML-DSA EVP_PKEY public key object created successfully.
  @retval FALSE  Invalid input or ML-DSA EVP_PKEY public key object creation failed.
**/
BOOLEAN
EFIAPI
MlDsaCreatePublicKeyObject (
  IN     UINT8        *RawPublicKey,
  IN OUT VOID         **Pkey,
  IN     CONST UINTN  Nid
  )
{
  EVP_PKEY  *PublicKey = NULL;
  CHAR8     *Alg;
  UINTN     KeySize;
  INT32     NidValue;

  if (!RawPublicKey) {
    return FALSE;
  }

  NidValue = CryptoNidToOpensslNid (Nid);
  if (NidValue == NID_undef) {
    return FALSE;
  }

  if (!NidToPublicKeySize (NidValue, &KeySize)) {
    return FALSE;
  }

  if (!NidToSignatureAlgorithmName (NidValue, &Alg)) {
    return FALSE;
  }

  PublicKey = EVP_PKEY_new_raw_public_key (NidValue, NULL, RawPublicKey, KeySize);
  if (PublicKey == NULL) {
    return FALSE;
  }

  *Pkey = (VOID *)PublicKey;
  return TRUE;
}

/**
   Free the ML-DSA EVP_PKEY object

   If Pkey is NULL, then return.

   @param[in]  Pkey  Pointer to the MLDSA EVP_PKEY object to be freed.
**/
VOID
MlDsaFreePublicKeyObject (
  IN VOID  *Pkey
  )
{
  if (Pkey != NULL) {
    EVP_PKEY_free ((EVP_PKEY *)Pkey);
  }
}

/**
   Verifies the ML-DSA signature.

   If Pkey is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataLen is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigLen is 0, then return FALSE.

   @param[in]  Pkey       Pointer to ML-DSA EVP_PKEY object.
   @param[in]  Data       Pointer to data being verified.
   @param[in]  DataLen    Length of data.
   @param[in]  Signature  Pointer to ML-DSA signature.
   @param[in]  SigLen     Length of ML-DSA signature.
   @param[in]  Nid        Nid for ML-DSA Category

   @retval  TRUE   Valid signature.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
MlDsaVerify (
  IN     CONST VOID   *Pkey,
  IN     CONST UINT8  *Data,
  IN     CONST UINTN  DataLen,
  IN     CONST UINT8  *Signature,
  IN     CONST UINTN  SigLen,
  IN     CONST UINTN  Nid
  )
{
  EVP_PKEY_CTX   *VerifyCtx = NULL;
  EVP_SIGNATURE  *SigAlg    = NULL;
  CHAR8          *Alg;
  INT32          NidValue;

  if ((Pkey == NULL) || (Data == NULL) || (DataLen == 0) || (Signature == NULL) || (SigLen == 0)) {
    return FALSE;
  }

  NidValue = CryptoNidToOpensslNid (Nid);
  if (NidValue == NID_undef) {
    return FALSE;
  }

  if (!NidToSignatureAlgorithmName (NidValue, &Alg)) {
    return FALSE;
  }

  SigAlg = EVP_SIGNATURE_fetch (NULL, Alg, NULL);
  if (SigAlg == NULL) {
    return FALSE;
  }

  VerifyCtx = EVP_PKEY_CTX_new_from_pkey (NULL, (EVP_PKEY *)Pkey, NULL);
  if (VerifyCtx == NULL) {
    ReleaseMlDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify_message_init (VerifyCtx, SigAlg, NULL) != 1) {
    ReleaseMlDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify (VerifyCtx, Signature, SigLen, Data, DataLen) != 1) {
    ReleaseMlDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  ReleaseMlDsaResources (VerifyCtx, SigAlg);

  return TRUE;
}
