/** @file
  RSA PSS Asymmetric Cipher Wrapper Implementation over MbedTLS.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaPssSign

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

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
  IN VOID         *RsaContext,
  IN CONST UINT8  *Message,
  IN UINTN        MsgSize,
  IN UINT16       DigestLen,
  IN UINT16       SaltLen,
  OUT UINT8       *Signature,
  IN OUT UINTN    *SigSize
  )
{
  INT32              Ret;
  mbedtls_md_type_t  MdAlg;
  UINT8              HashValue[SHA512_DIGEST_SIZE];

  if (RsaContext == NULL) {
    return FALSE;
  }

  if (mbedtls_rsa_complete ((mbedtls_rsa_context *)RsaContext) != 0) {
    return FALSE;
  }

  if ((Message == NULL) || (MsgSize == 0) || (MsgSize > INT_MAX)) {
    return FALSE;
  }

  if (SaltLen != DigestLen) {
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

  if (Signature == NULL) {
    //
    // If Signature is NULL, return safe SignatureSize
    //
    *SigSize = MBEDTLS_MPI_MAX_SIZE;
    return FALSE;
  }

  Ret = mbedtls_rsa_set_padding (RsaContext, MBEDTLS_RSA_PKCS_V21, MdAlg);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_rsa_rsassa_pss_sign (
          RsaContext,
          MbedtlsRand,
          NULL,
          MdAlg,
          (UINT32)DigestLen,
          HashValue,
          Signature
          );
  if (Ret != 0) {
    return FALSE;
  }

  *SigSize = ((mbedtls_rsa_context *)RsaContext)->len;
  return TRUE;
}
