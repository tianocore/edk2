/** @file
  RSA PSS Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaPssSign

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
  Carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme defined in
  RFC 8017.
  Mask generation function is the same as the message digest algorithm.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If MsgSize is zero or > INT_MAX, then return FALSE.
  If DigestLen is NOT 32, 48 or 64, return FALSE.
  If SaltLen is not equal to DigestLen, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      Message      Pointer to octet message to be signed.
  @param[in]      MsgSize      Size of the message in bytes.
  @param[in]      DigestLen    Length of the digest in bytes to be used for RSA signature operation.
  @param[in]      SaltLen      Length of the salt in bytes to be used for PSS encoding.
  @param[out]     Signature    Pointer to buffer to receive RSA PSS signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in RSASSA-PSS.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaPssSign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *Message,
  IN      UINTN        MsgSize,
  IN      UINT16       DigestLen,
  IN      UINT16       SaltLen,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  BOOLEAN               Result;
  UINTN                 RsaSigSize;
  EVP_PKEY              *EvpRsaKey;
  EVP_MD_CTX            *EvpVerifyCtx;
  EVP_PKEY_CTX          *KeyCtx;
  CONST EVP_MD          *HashAlg;

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

  RsaSigSize = RSA_size (RsaContext);
  if (*SigSize < RsaSigSize) {
    *SigSize = RsaSigSize;
    return FALSE;
  }

  if (Signature == NULL) {
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

  Result = EVP_DigestSignInit(EvpVerifyCtx, &KeyCtx, HashAlg, NULL, EvpRsaKey) > 0;
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
    Result = EVP_DigestSignUpdate(EvpVerifyCtx, Message, (UINT32)MsgSize) > 0;
  }
  if (Result) {
    Result = EVP_DigestSignFinal(EvpVerifyCtx, Signature, SigSize) > 0;
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
