/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaPssVerify

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/evp.h>


/**
  Retrieve a pointer to EVP message digest object.

  @param[in]  DigestLen   Length of the message digest.

**/
STATIC
const
EVP_MD*
GetEvpMD (
  IN UINT16 DigestLen
  )
{
  switch (DigestLen){
    case SHA256_DIGEST_SIZE:
      return EVP_sha256();
      break;
    case SHA384_DIGEST_SIZE:
      return EVP_sha384();
      break;
    case SHA512_DIGEST_SIZE:
      return EVP_sha512();
      break;
    default:
      return NULL;
  }
}


/**
  Verifies the RSA signature with RSASSA-PSS signature scheme defined in RFC 8017.
  Implementation determines salt length automatically from the signature encoding.
  Mask generation function is the same as the message digest algorithm.
  Salt length should be equal to digest length.

  @param[in]  RsaContext      Pointer to RSA context for signature verification.
  @param[in]  Message         Pointer to octet message to be verified.
  @param[in]  MsgSize         Size of the message in bytes.
  @param[in]  Signature       Pointer to RSASSA-PSS signature to be verified.
  @param[in]  SigSize         Size of signature in bytes.
  @param[in]  DigestLen       Length of digest for RSA operation.
  @param[in]  SaltLen         Salt length for PSS encoding.

  @retval  TRUE   Valid signature encoded in RSASSA-PSS.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPssVerify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *Message,
  IN  UINTN        MsgSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize,
  IN  UINT16       DigestLen,
  IN  UINT16       SaltLen
  )
{
  BOOLEAN Result;
  EVP_PKEY *EvpRsaKey;
  EVP_MD_CTX *EvpVerifyCtx;
  EVP_PKEY_CTX *KeyCtx;
  CONST EVP_MD  *HashAlg;

  Result = FALSE;
  EvpRsaKey = NULL;
  EvpVerifyCtx = NULL;
  KeyCtx = NULL;
  HashAlg = NULL;

  if (RsaContext == NULL) {
    return FALSE;
  }
  if (Message == NULL || MsgSize == 0 || MsgSize > INT_MAX) {
    return FALSE;
  }
  if (Signature == NULL || SigSize == 0 || SigSize > INT_MAX) {
    return FALSE;
  }
  if (SaltLen != DigestLen) {
    return FALSE;
  }

  HashAlg = GetEvpMD(DigestLen);

  if (HashAlg == NULL) {
    return FALSE;
  }

  EvpRsaKey = EVP_PKEY_new();
  if (EvpRsaKey == NULL) {
    goto _Exit;
  }

  EVP_PKEY_set1_RSA(EvpRsaKey, RsaContext);

  EvpVerifyCtx = EVP_MD_CTX_create();
  if (EvpVerifyCtx == NULL) {
    goto _Exit;
  }

  Result = EVP_DigestVerifyInit(EvpVerifyCtx, &KeyCtx, HashAlg, NULL, EvpRsaKey) > 0;
  if (KeyCtx == NULL) {
    goto _Exit;
  }

  if (Result) {
    Result = EVP_PKEY_CTX_set_rsa_padding(KeyCtx, RSA_PKCS1_PSS_PADDING) > 0;
  }
  if (Result) {
    Result = EVP_PKEY_CTX_set_rsa_pss_saltlen(KeyCtx, SaltLen) > 0;
  }
  if (Result) {
    Result = EVP_PKEY_CTX_set_rsa_mgf1_md(KeyCtx, HashAlg) > 0;
  }
  if (Result) {
    Result = EVP_DigestVerifyUpdate(EvpVerifyCtx, Message, (UINT32)MsgSize) > 0;
  }
  if (Result) {
    Result = EVP_DigestVerifyFinal(EvpVerifyCtx, Signature, (UINT32)SigSize) > 0;
  }

_Exit :
  if (EvpRsaKey != NULL) {
    EVP_PKEY_free(EvpRsaKey);
  }
  if (EvpVerifyCtx != NULL) {
    EVP_MD_CTX_destroy(EvpVerifyCtx);
  }

  return Result;
}
