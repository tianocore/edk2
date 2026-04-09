/** @file
  RSA Asymmetric Cipher Wrapper Implementation over MbedTLS.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaPssVerify
  2) RsaPssVerifyDigest

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
(c) Copyright 2026 HP Development Company, L.P.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

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
  INT32                Ret;
  mbedtls_md_type_t    MdAlg;
  UINT8                HashValue[SHA512_DIGEST_SIZE];
  mbedtls_rsa_context  *RsaKey;

  if (RsaContext == NULL) {
    return FALSE;
  }

  if ((Message == NULL) || (MsgSize == 0) || (MsgSize > INT_MAX)) {
    return FALSE;
  }

  if (SaltLen != DigestLen) {
    return FALSE;
  }

  if ((Signature == NULL) || (SigSize == 0) || (SigSize > INT_MAX)) {
    return FALSE;
  }

  RsaKey = (mbedtls_rsa_context *)RsaContext;
  if (mbedtls_rsa_complete (RsaKey) != 0) {
    return FALSE;
  }

  ZeroMem (HashValue, DigestLen);

  switch (DigestLen) {
    case SHA256_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA256;
      if (mbedtls_sha256 (Message, MsgSize, HashValue, FALSE) != 0) {
        return FALSE;
      }

      break;

    case SHA384_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA384;
      if (mbedtls_sha512 (Message, MsgSize, HashValue, TRUE) != 0) {
        return FALSE;
      }

      break;

    case SHA512_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA512;
      if (mbedtls_sha512 (Message, MsgSize, HashValue, FALSE) != 0) {
        return FALSE;
      }

      break;

    default:
      return FALSE;
  }

  if (mbedtls_rsa_get_len (RsaContext) != SigSize) {
    return FALSE;
  }

  mbedtls_rsa_set_padding (RsaContext, MBEDTLS_RSA_PKCS_V21, MdAlg);

  Ret = mbedtls_rsa_rsassa_pss_verify (
          RsaContext,
          MdAlg,
          (UINT32)DigestLen,
          HashValue,
          Signature
          );
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Verifies an RSA-PSS signature over a precomputed message digest.
  Mask generation function is the same as the message digest algorithm.

  If RsaContext is NULL, then return FALSE.
  If Digest is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If DigestSize is not one of SHA-256, SHA-384 or SHA-512 digest sizes,
  then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  Digest       Pointer to the message digest.
  @param[in]  DigestSize   Digest size in bytes
                           SHA256_DIGEST_SIZE
                           SHA384_DIGEST_SIZE
                           SHA512_DIGEST_SIZE
  @param[in]  Signature    Pointer to RSASSA-PSS signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in RSASSA-PSS.
  @retval  FALSE  Invalid signature or invalid RSA context.
**/
BOOLEAN
EFIAPI
RsaPssVerifyDigest (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *Digest,
  IN  UINTN        DigestSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  INT32                Ret;
  mbedtls_md_type_t    MdAlg;
  mbedtls_rsa_context  *RsaKey;

  if (RsaContext == NULL) {
    return FALSE;
  }

  if ((Digest == NULL) || (DigestSize == 0) || (DigestSize > INT_MAX) || (DigestSize > MAX_UINT16)) {
    return FALSE;
  }

  if ((Signature == NULL) || (SigSize == 0) || (SigSize > INT_MAX)) {
    return FALSE;
  }

  RsaKey = (mbedtls_rsa_context *)RsaContext;
  if (mbedtls_rsa_complete (RsaKey) != 0) {
    return FALSE;
  }

  switch (DigestSize) {
    case SHA256_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA256;
      break;

    case SHA384_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA384;
      break;

    case SHA512_DIGEST_SIZE:
      MdAlg = MBEDTLS_MD_SHA512;
      break;

    default:
      return FALSE;
  }

  if (mbedtls_rsa_get_len (RsaContext) != SigSize) {
    return FALSE;
  }

  mbedtls_rsa_set_padding (RsaContext, MBEDTLS_RSA_PKCS_V21, MdAlg);

  Ret = mbedtls_rsa_rsassa_pss_verify (
          RsaContext,
          MdAlg,
          (UINT32)DigestSize,
          Digest,
          Signature
          );
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}
