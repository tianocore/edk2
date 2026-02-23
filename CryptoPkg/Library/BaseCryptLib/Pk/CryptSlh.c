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

  If Nid is not NID_SLHDSASHAKE256S, then return FALSE.

  @param[in]  RawPublicKey  Pointer to raw public key bytes.
  @param[in]  Nid           Nid for SLH-DSA Category

  @retval     Pointer to the SLH-DSA EVP_PKEY object created.
**/
VOID *
EFIAPI
SlhDsaCreatePublicKeyObject (
  IN UINT8        *RawPublicKey,
  IN CONST UINTN  Nid
  )
{
  EVP_PKEY  *PublicKey = NULL;
  UINTN     KeySize;
  INT32     NidValue;

  if (!RawPublicKey) {
    return NULL;
  }

  NidValue = CryptoNidToOpensslNid (Nid);
  if (NidValue == NID_undef) {
    return NULL;
  }

  if (!NidToPublicKeySize (NidValue, &KeySize)) {
    return NULL;
  }

  PublicKey = EVP_PKEY_new_raw_public_key (NidValue, NULL, RawPublicKey, KeySize);
  if (PublicKey == NULL) {
    return NULL;
  }

  return (VOID *)PublicKey;
}

/**
   Free the SLH-DSA EVP_PKEY object

   @param[in]  PkeyContext  Pointer to the SLHDSA EVP_PKEY object to be freed.

   @retval VOID
**/
VOID
SlhDsaFreePublicKeyObject (
  IN VOID  *PkeyContext
  )
{
  if (PkeyContext != NULL) {
    EVP_PKEY_free ((EVP_PKEY *)PkeyContext);
  }
}

/**
   Verifies the SLH-DSA signature.

   If PkeyContext is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataSize is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigSize is 0, then return FALSE.

   @param[in]  PkeyContext   Pointer to SLH-DSA EVP_PKEY object.
   @param[in]  Nid           Nid for SLH-DSA Category
   @param[in]  Data          Pointer to data being verified.
   @param[in]  DataSize      Length of data.
   @param[in]  Signature     Pointer to SLH-DSA signature.
   @param[in]  SigSize       Length of SLH-DSA signature.

   @retval  TRUE   Valid signature.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
SlhDsaVerify (
  IN CONST VOID   *PkeyContext,
  IN UINTN        Nid,
  IN CONST UINT8  *Data,
  IN UINTN        DataSize,
  IN CONST UINT8  *Signature,
  IN UINTN        SigSize
  )
{
  EVP_PKEY_CTX   *VerifyCtx = NULL;
  EVP_SIGNATURE  *SigAlg    = NULL;
  CHAR8          *Alg;
  INT32          NidValue;

  if (!PkeyContext || !Data || !DataSize || !Signature || !SigSize) {
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

  VerifyCtx = EVP_PKEY_CTX_new_from_pkey (NULL, (EVP_PKEY *)PkeyContext, NULL);
  if (VerifyCtx == NULL) {
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify_message_init (VerifyCtx, SigAlg, NULL) != 1) {
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  if (EVP_PKEY_verify (VerifyCtx, Signature, SigSize, Data, DataSize) != 1) {
    ReleaseSlhDsaResources (VerifyCtx, SigAlg);
    return FALSE;
  }

  ReleaseSlhDsaResources (VerifyCtx, SigAlg);

  return TRUE;
}
