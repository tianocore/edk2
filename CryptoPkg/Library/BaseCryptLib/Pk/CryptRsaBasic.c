/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaNew
  2) RsaFree
  3) RsaSetKey
  4) RsaPkcs1Verify

Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RsaNew (
  VOID
  )
{
  //
  // Allocates & Initializes RSA Context by OpenSSL RSA_new()
  //
  return (VOID *) RSA_new ();
}

/**
  Release the specified RSA context.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RsaFree (
  IN  VOID  *RsaContext
  )
{
  //
  // Free OpenSSL RSA Context
  //
  RSA_free ((RSA *) RsaContext);
}

/**
  Sets the tag-designated key component into the established RSA context.

  This function sets the tag-designated RSA key component into the established
  RSA context from the user-specified non-negative integer (octet string format
  represented in RSA PKCS#1).
  If BigNumber is NULL, then the specified key component in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key component in RSA
                               context is cleared.
  @param[in]       BnSize      Size of big number buffer in bytes.
                               If BigNumber is NULL, then it is ignored.

  @retval  TRUE   RSA key component was set successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  RSA     *RsaKey;
  BIGNUM  *BnN;
  BIGNUM  *BnE;
  BIGNUM  *BnD;
  BIGNUM  *BnP;
  BIGNUM  *BnQ;
  BIGNUM  *BnDp;
  BIGNUM  *BnDq;
  BIGNUM  *BnQInv;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL || BnSize > INT_MAX) {
    return FALSE;
  }

  BnN    = NULL;
  BnE    = NULL;
  BnD    = NULL;
  BnP    = NULL;
  BnQ    = NULL;
  BnDp   = NULL;
  BnDq   = NULL;
  BnQInv = NULL;

  //
  // Retrieve the components from RSA object.
  //
  RsaKey = (RSA *) RsaContext;
  RSA_get0_key (RsaKey, (const BIGNUM **)&BnN, (const BIGNUM **)&BnE, (const BIGNUM **)&BnD);
  RSA_get0_factors (RsaKey, (const BIGNUM **)&BnP, (const BIGNUM **)&BnQ);
  RSA_get0_crt_params (RsaKey, (const BIGNUM **)&BnDp, (const BIGNUM **)&BnDq, (const BIGNUM **)&BnQInv);

  //
  // Set RSA Key Components by converting octet string to OpenSSL BN representation.
  // NOTE: For RSA public key (used in signature verification), only public components
  //       (N, e) are needed.
  //
  switch (KeyTag) {

  //
  // RSA Public Modulus (N), Public Exponent (e) and Private Exponent (d)
  //
  case RsaKeyN:
  case RsaKeyE:
  case RsaKeyD:
    if (BnN == NULL) {
      BnN = BN_new ();
    }
    if (BnE == NULL) {
      BnE = BN_new ();
    }
    if (BnD == NULL) {
      BnD = BN_new ();
    }

    if ((BnN == NULL) || (BnE == NULL) || (BnD == NULL)) {
      return FALSE;
    }

    switch (KeyTag) {
    case RsaKeyN:
      BnN = BN_bin2bn (BigNumber, (UINT32)BnSize, BnN);
      break;
    case RsaKeyE:
      BnE = BN_bin2bn (BigNumber, (UINT32)BnSize, BnE);
      break;
    case RsaKeyD:
      BnD = BN_bin2bn (BigNumber, (UINT32)BnSize, BnD);
      break;
    default:
      return FALSE;
    }
    if (RSA_set0_key (RsaKey, BN_dup(BnN), BN_dup(BnE), BN_dup(BnD)) == 0) {
      return FALSE;
    }

    break;

  //
  // RSA Secret Prime Factor of Modulus (p and q)
  //
  case RsaKeyP:
  case RsaKeyQ:
    if (BnP == NULL) {
      BnP = BN_new ();
    }
    if (BnQ == NULL) {
      BnQ = BN_new ();
    }
    if ((BnP == NULL) || (BnQ == NULL)) {
      return FALSE;
    }

    switch (KeyTag) {
    case RsaKeyP:
      BnP = BN_bin2bn (BigNumber, (UINT32)BnSize, BnP);
      break;
    case RsaKeyQ:
      BnQ = BN_bin2bn (BigNumber, (UINT32)BnSize, BnQ);
      break;
    default:
      return FALSE;
    }
    if (RSA_set0_factors (RsaKey, BN_dup(BnP), BN_dup(BnQ)) == 0) {
      return FALSE;
    }

    break;

  //
  // p's CRT Exponent (== d mod (p - 1)),  q's CRT Exponent (== d mod (q - 1)),
  // and CRT Coefficient (== 1/q mod p)
  //
  case RsaKeyDp:
  case RsaKeyDq:
  case RsaKeyQInv:
    if (BnDp == NULL) {
      BnDp = BN_new ();
    }
    if (BnDq == NULL) {
      BnDq = BN_new ();
    }
    if (BnQInv == NULL) {
      BnQInv = BN_new ();
    }
    if ((BnDp == NULL) || (BnDq == NULL) || (BnQInv == NULL)) {
      return FALSE;
    }

    switch (KeyTag) {
    case RsaKeyDp:
      BnDp = BN_bin2bn (BigNumber, (UINT32)BnSize, BnDp);
      break;
    case RsaKeyDq:
      BnDq = BN_bin2bn (BigNumber, (UINT32)BnSize, BnDq);
      break;
    case RsaKeyQInv:
      BnQInv = BN_bin2bn (BigNumber, (UINT32)BnSize, BnQInv);
      break;
    default:
      return FALSE;
    }
    if (RSA_set0_crt_params (RsaKey, BN_dup(BnDp), BN_dup(BnDq), BN_dup(BnQInv)) == 0) {
      return FALSE;
    }

    break;

  default:
    return FALSE;
  }

  return TRUE;
}

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1, SHA-256, SHA-384 or SHA-512 digest, then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in PKCS1-v1_5.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  INT32    DigestType;
  UINT8    *SigBuf;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL || MessageHash == NULL || Signature == NULL) {
    return FALSE;
  }

  if (SigSize > INT_MAX || SigSize == 0) {
    return FALSE;
  }

  //
  // Determine the message digest algorithm according to digest size.
  //   Only MD5, SHA-1, SHA-256, SHA-384 or SHA-512 algorithm is supported.
  //
  switch (HashSize) {
  case MD5_DIGEST_SIZE:
    DigestType = NID_md5;
    break;

  case SHA1_DIGEST_SIZE:
    DigestType = NID_sha1;
    break;

  case SHA256_DIGEST_SIZE:
    DigestType = NID_sha256;
    break;

  case SHA384_DIGEST_SIZE:
    DigestType = NID_sha384;
    break;

  case SHA512_DIGEST_SIZE:
    DigestType = NID_sha512;
    break;

  default:
    return FALSE;
  }

  SigBuf = (UINT8 *) Signature;
  return (BOOLEAN) RSA_verify (
                     DigestType,
                     MessageHash,
                     (UINT32) HashSize,
                     SigBuf,
                     (UINT32) SigSize,
                     (RSA *) RsaContext
                     );
}
