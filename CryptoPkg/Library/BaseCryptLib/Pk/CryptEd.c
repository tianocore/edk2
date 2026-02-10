/** @file
  EdDSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/provider.h>

OSSL_LIB_CTX  *LibCtx = NULL;

/**
   Create OpenSSL library context.

   @return Pointer to OSSL_LIB_CTX structure.
**/
STATIC
OSSL_LIB_CTX *
OpenSslCreateLibCtx (
  )
{
  if (LibCtx == NULL) {
    LibCtx = OSSL_LIB_CTX_new ();
  }

  return LibCtx;
}

/**
   Free OpenSSL library context.

   @param[in]  LibCtx   Pointer to OSSL_LIB_CTX structure.
**/
STATIC
VOID
OpenSslFreeLibCtx (
  )
{
  if (LibCtx != NULL) {
    OSSL_LIB_CTX_free (LibCtx);
  }
}

/**
  Free EVP_MD_CTX for EdDSA.

  @param[in]  Ctx     Pointer to EVP_MD_CTX structure.
 **/
STATIC
VOID
FreeEvpMdCtx (
  IN EVP_MD_CTX  *Ctx
  )
{
  if (Ctx != NULL) {
    EVP_MD_CTX_free (Ctx);
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
  INT32  NidPublicKey;

  switch (Nid) {
    case CRYPTO_NID_ED448:
      NidPublicKey = NID_ED448;
      break;
    default:
      NidPublicKey = NID_undef;
      break;
  }

  return NidPublicKey;
}

/**
  Get coordinate length for EdDSA curve.

  @param[in]  Nid        Crypto NID of the EdDSA curve.
  @param[out] CoordLen   Pointer to coordinate length.

  @retval  TRUE   Coordinate length retrieved successfully.
  @retval  FALSE  Unsupported Crypto NID.
**/
STATIC
BOOLEAN
NidToCoordLen (
  IN UINTN   Nid,
  OUT UINTN  *CoordLen
  )
{
  switch (Nid) {
    case NID_ED448:
      *CoordLen = 57;
      break;
    default:
      *CoordLen = 0;
      return FALSE;
  }

  return TRUE;
}

/**
  Get key type for EdDSA curve.

  @param[in]  Nid       Crypto NID of the EdDSA curve.
  @param[out] KeyType   Pointer to key type string.

  @retval  TRUE   Key type retrieved successfully.
  @retval  FALSE  Unsupported Crypto NID.
**/
STATIC
BOOLEAN
NidToKeyType (
  IN UINTN         Nid,
  OUT CONST CHAR8  **KeyType
  )
{
  switch (Nid) {
    case NID_ED448:
      *KeyType = SN_ED448;
      break;
    default:
      *KeyType = NULL;
      return FALSE;
  }

  return TRUE;
}

/**
  Creates a new EdDSA public key object.

  If A is NULL, then return FALSE.

  @param[in]      A             Pointer to EdDSA Raw public key.
  @param[in]      Nid           Crypto NID of the EdDSA curve.

  @retval Pointer to new EdDSA public key object.
**/
VOID *
EFIAPI
EdDsaCreatePublicKeyObject (
  IN     UINT8  *A,
  IN     UINTN  Nid
  )
{
  EVP_PKEY     *PkeyContext = NULL;
  UINTN        CoordLen     = 0;
  INT32        NidValue;
  CONST CHAR8  *KeyType;

  if (!A) {
    return NULL;
  }

  NidValue = CryptoNidToOpensslNid (Nid);
  if (NidValue == NID_undef) {
    return NULL;
  }

  if (!NidToCoordLen (NidValue, &CoordLen)) {
    return NULL;
  }

  if (!NidToKeyType (NidValue, &KeyType)) {
    return NULL;
  }

  LibCtx = OpenSslCreateLibCtx ();

  PkeyContext = EVP_PKEY_new_raw_public_key_ex (LibCtx, KeyType, NULL, A, CoordLen);
  if (PkeyContext == NULL) {
    OpenSslFreeLibCtx ();
    return NULL;
  }

  return (VOID *)PkeyContext;
}

/**
  Free EdDSA public key object.

  If PkeyContext is NULL, then return.

  @param[in]  PkeyContext   Pointer to EdDSA public key object.
**/
VOID
EFIAPI
EdDsaFreePublicKeyObject (
  IN VOID  *PkeyContext
  )
{
  if (PkeyContext != NULL) {
    EVP_PKEY_free ((EVP_PKEY *)PkeyContext);
  }
}

/**
  Verifies the EdDSA signature. EdDSA signatures are verified via 'pure' implementation.
  Meaning message digest cannot be computed ahead of time. Data is passed into the verification function

  If PkeyContext is NULL, then return FALSE.
  If Data is NULL, then return FALSE.
  If DataSize is 0, then return FALSE.
  If Sig is NULL, then return FALSE.
  If SigSize is 0, then return FALSE.

  @param[in]  PkeyContext       Pointer to EdDSA public key object.
  @param[in]  Data              Pointer to data being verified.
  @param[in]  DataSize          Size of data.
  @param[in]  Signature         Pointer to EdDSA signature.
  @param[in]  SigSize           Size of EdDSA signature.

  @retval  TRUE   Valid signature encoded in EdDSA.
  @retval  FALSE  Invalid signature or invalid input.
**/
BOOLEAN
EFIAPI
EdDsaVerify (
  IN VOID         *PkeyContext,
  IN CONST UINT8  *Data,
  IN UINTN        DataSize,
  IN CONST UINT8  *Signature,
  IN UINTN        SigSize
  )
{
  EVP_MD_CTX  *VerifyCtx = NULL;

  if (LibCtx == NULL) {
    return FALSE;
  }

  if (!PkeyContext || !Data || !DataSize || !Signature || !SigSize) {
    OpenSslFreeLibCtx ();
    return FALSE;
  }

  VerifyCtx = EVP_MD_CTX_new ();
  if (!VerifyCtx) {
    OpenSslFreeLibCtx ();
    return FALSE;
  }

  if (EVP_DigestVerifyInit_ex (VerifyCtx, NULL, NULL, LibCtx, NULL, (EVP_PKEY *)PkeyContext, NULL) != 1) {
    OpenSslFreeLibCtx ();
    FreeEvpMdCtx (VerifyCtx);
    return FALSE;
  }

  if (EVP_DigestVerify (VerifyCtx, Signature, (UINT32)SigSize, Data, (UINT32)DataSize) != 1) {
    OpenSslFreeLibCtx ();
    FreeEvpMdCtx (VerifyCtx);
    return FALSE;
  }

  OpenSslFreeLibCtx ();
  FreeEvpMdCtx (VerifyCtx);

  return TRUE;
}
