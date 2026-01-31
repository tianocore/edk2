/** @file
  SLH-DSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <crypto/slh_dsa.h>

/**
  Release SLH-DSA resources

  @param[in]  Ctx     Pointer to EVP_MD_CTX structure.
  @param[in]  SigAlg  Pointer to EVP_SIGNATURE structure.
 **/
STATIC
VOID
ReleaseSlhDsaResources (
  IN EVP_PKEY_CTX   *Ctx,
  IN EVP_SIGNATURE  *SigAlg
  )
{
  if (Ctx != NULL) {
    EVP_PKEY_CTX_free (Ctx);
  }

  if (SigAlg != NULL) {
    EVP_SIGNATURE_free (SigAlg);
  }
}

/**
 Convert Crypto NID to OpenSSL NID

  If Nid is not NID_SLHDSASHAKE256S, then return NID_undef.

  @param[in]  Nid  Numeric identifier value to denote SLHDSA-SHAKE256S

  @return     OpenSSL NID value that corresponds to the Crypto NID value.
              If the Crypto NID is not recognized, then NID_undef is returned.
**/
STATIC
INT32
CryptoNidToOpensslNid (
  IN UINTN  Nid
  )
{
  INT32  OpenSslNid;

  switch (Nid) {
    case CRYPTO_NID_SLH_DSA_SHAKE_256S:
      OpenSslNid = NID_SLH_DSA_SHAKE_256s;
      break;
    default:
      return NID_undef;
  }

  return OpenSslNid;
}

/**
  Convert Crypto NID to SLH-DSA Signature Algorithm Name

  If Nid is not NID_SLHDSASHAKE256S, then return FALSE.

  @param[in]  Nid  Numeric identifier value to denote SLHDSA-SHAKE256S
  @param[out] Alg  Pointer to SLH-DSA Signature Algorithm Name.

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
    case NID_SLH_DSA_SHAKE_256s:
      *Alg = LN_SLH_DSA_SHAKE_256s;
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

/**
 Convert Crypto NID to Key Size

  If Nid is not NID_SLHDSASHAKE256S, then return FALSE.

  @param[in]  Nid      Numeric identifier value to denote SLHDSA-SHAKE256S
  @param[out] KeySize  Pointer to key size in bytes.

  @return     TRUE if the Crypto NID is recognized and KeySize is returned.
              FALSE if the Crypto NID is not recognized.
**/
STATIC
BOOLEAN
NidToPublicKeySize (
  IN UINTN   Nid,
  OUT UINTN  *KeySize
  )
{
  switch (Nid) {
    case NID_SLH_DSA_SHAKE_256s:
      *KeySize = 64;
      break;
    default:
      return FALSE;
  }

  return TRUE;
}

/**
 Create the SLH-DSA EVP_PKEY public key object from raw public key bytes.

  If Pkey is NULL, then return FALSE.
  If RawPublicKey is NULL, then return FALSE.
  If Nid is not NID_SLHDSASHAKE256S, then return FALSE.

  @param[in]  RawPublicKey  Pointer to raw public key bytes.
  @param[out] Pkey          Pointer to the SLH-DSA EVP_PKEY object created.
  @param[in]  Nid           Nid for SLH-DSA Category

  @retval TRUE   SLH-DSA EVP_PKEY public key object created successfully.
  @retval FALSE  Invalid input or SLH-DSA EVP_PKEY public key object creation failed.
**/
BOOLEAN
EFIAPI
SlhDsaCreatePublicKeyObject (
  IN     UINT8        *RawPublicKey,
  IN OUT VOID         **Pkey,
  IN     CONST UINTN  Nid
  )
{
  EVP_PKEY  *PublicKey = NULL;
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

  PublicKey = EVP_PKEY_new_raw_public_key (NidValue, NULL, RawPublicKey, KeySize);
  if (PublicKey == NULL) {
    return FALSE;
  }

  *Pkey = (VOID *)PublicKey;
  return TRUE;
}

/**
   Free the SLH-DSA EVP_PKEY object

   If Pkey is NULL, then return.

   @param[in]  Pkey  Pointer to the SLHDSA EVP_PKEY object to be freed.
**/
VOID
SlhDsaFreePublicKeyObject (
  IN VOID  *Pkey
  )
{
  if (Pkey != NULL) {
    EVP_PKEY_free ((EVP_PKEY *)Pkey);
  }
}

/**
   Verifies the SLH-DSA signature.

   If Pkey is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataLen is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigLen is 0, then return FALSE.

   @param[in]  Pkey       Pointer to SLH-DSA EVP_PKEY object.
   @param[in]  Data       Pointer to data being verified.
   @param[in]  DataLen    Length of data.
   @param[in]  Signature  Pointer to SLH-DSA signature.
   @param[in]  SigLen     Length of SLH-DSA signature.
   @param[in]  Nid        Nid for SLH-DSA Category

   @retval  TRUE   Valid signature.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
SlhDsaVerify (
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
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify_message_init (VerifyCtx, SigAlg, NULL) != 1) {
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify (VerifyCtx, Signature, SigLen, Data, DataLen) != 1) {
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  ReleaseSlhDsaResources (VerifyCtx, SigAlg);

  return TRUE;
}
