/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaNew
  2) RsaFree
  3) RsaSetKey
  4) RsaPkcs1Verify

Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
(c) Copyright 2025 HP Development Company, L.P.
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
  return (VOID *)RSA_new ();
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
  RSA_free ((RSA *)RsaContext);
}

/**
  Install one of the n, e, d components of an RSA key.

  @param[in, out]  RsaKey     OpenSSL RSA object.
  @param[in]       KeyTag     One of RsaKeyN, RsaKeyE, RsaKeyD.
  @param[in]       BigNumber  Octet integer buffer to install.
  @param[in]       BnSize     Size of BigNumber in bytes.

  @retval  TRUE   Component installed.
  @retval  FALSE  Allocation or RSA_set0_key failed; RsaKey is unchanged.

**/
STATIC
BOOLEAN
RsaSetKeyNED (
  IN OUT  RSA          *RsaKey,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  BIGNUM        *PassN;
  BIGNUM        *PassE;
  BIGNUM        *PassD;
  const BIGNUM  *CurN;
  const BIGNUM  *CurE;
  const BIGNUM  *CurD;
  BOOLEAN       Result;

  PassN  = NULL;
  PassE  = NULL;
  PassD  = NULL;
  Result = FALSE;

  //
  // Allocate the slot being set. BN_bin2bn allocates a fresh BIGNUM when
  // its destination argument is NULL, so no RSA-owned BIGNUM is touched.
  //
  switch (KeyTag) {
    case RsaKeyN:
      PassN = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassN == NULL) {
        goto Exit;
      }

      break;
    case RsaKeyE:
      PassE = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassE == NULL) {
        goto Exit;
      }

      break;
    case RsaKeyD:
      PassD = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassD == NULL) {
        goto Exit;
      }

      break;
    default:
      goto Exit;
  }

  //
  // Snapshot the current RSA state. Per the OpenSSL API contract the
  // BIGNUMs that CurN, CurE, CurD point to are owned by RsaKey and must
  // not be modified by the caller.
  //
  RSA_get0_key (RsaKey, &CurN, &CurE, &CurD);

  //
  // For each slot we are not setting:
  //   - leave PassX as NULL when RSA already has a value there
  //     (RSA_set0_key treats NULL as "preserve the current value in
  //     RsaKey"), or
  //   - install a fresh BIGNUM via BN_new() (which returns a BIGNUM with
  //     value 0) when RSA has NULL there.
  //
  if ((PassN == NULL) && (CurN == NULL)) {
    PassN = BN_new ();
    if (PassN == NULL) {
      goto Exit;
    }
  }

  if ((PassE == NULL) && (CurE == NULL)) {
    PassE = BN_new ();
    if (PassE == NULL) {
      goto Exit;
    }
  }

  if ((PassD == NULL) && (CurD == NULL)) {
    PassD = BN_new ();
    if (PassD == NULL) {
      goto Exit;
    }
  }

  //
  // Atomic install. On success ownership of every non-NULL argument
  // transfers to RsaKey; on failure no ownership transfers and the
  // RSA object is left unchanged.
  //
  if (RSA_set0_key (RsaKey, PassN, PassE, PassD) == 0) {
    goto Exit;
  }

  //
  // Ownership transferred to RsaKey. Null our handles so cleanup is a no-op.
  //
  PassN  = NULL;
  PassE  = NULL;
  PassD  = NULL;
  Result = TRUE;

Exit:
  BN_free (PassN);
  BN_free (PassE);
  BN_clear_free (PassD);
  return Result;
}

/**
  Install one of the p, q factors of an RSA key. See RsaSetKeyNED() for the
  shared invariants; the only difference is the setter used and the slot set.

  @param[in, out]  RsaKey     OpenSSL RSA object.
  @param[in]       KeyTag     One of RsaKeyP, RsaKeyQ.
  @param[in]       BigNumber  Octet integer buffer to install.
  @param[in]       BnSize     Size of BigNumber in bytes.

  @retval  TRUE   Component installed.
  @retval  FALSE  Allocation or RSA_set0_factors failed; RsaKey is unchanged.

**/
STATIC
BOOLEAN
RsaSetKeyFactors (
  IN OUT  RSA          *RsaKey,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  BIGNUM        *PassP;
  BIGNUM        *PassQ;
  const BIGNUM  *CurP;
  const BIGNUM  *CurQ;
  BOOLEAN       Result;

  PassP  = NULL;
  PassQ  = NULL;
  Result = FALSE;

  switch (KeyTag) {
    case RsaKeyP:
      PassP = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassP == NULL) {
        goto Exit;
      }

      break;
    case RsaKeyQ:
      PassQ = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassQ == NULL) {
        goto Exit;
      }

      break;
    default:
      goto Exit;
  }

  RSA_get0_factors (RsaKey, &CurP, &CurQ);

  if ((PassP == NULL) && (CurP == NULL)) {
    PassP = BN_new ();
    if (PassP == NULL) {
      goto Exit;
    }
  }

  if ((PassQ == NULL) && (CurQ == NULL)) {
    PassQ = BN_new ();
    if (PassQ == NULL) {
      goto Exit;
    }
  }

  if (RSA_set0_factors (RsaKey, PassP, PassQ) == 0) {
    goto Exit;
  }

  PassP  = NULL;
  PassQ  = NULL;
  Result = TRUE;

Exit:
  BN_clear_free (PassP);
  BN_clear_free (PassQ);
  return Result;
}

/**
  Install one of the dp, dq, qInv CRT parameters of an RSA key. See
  RsaSetKeyNED() for the shared invariants.

  @param[in, out]  RsaKey     OpenSSL RSA object.
  @param[in]       KeyTag     One of RsaKeyDp, RsaKeyDq, RsaKeyQInv.
  @param[in]       BigNumber  Octet integer buffer to install.
  @param[in]       BnSize     Size of BigNumber in bytes.

  @retval  TRUE   Component installed.
  @retval  FALSE  Allocation or RSA_set0_crt_params failed; RsaKey is unchanged.

**/
STATIC
BOOLEAN
RsaSetKeyCrtParams (
  IN OUT  RSA          *RsaKey,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  BIGNUM        *PassDp;
  BIGNUM        *PassDq;
  BIGNUM        *PassQInv;
  const BIGNUM  *CurDp;
  const BIGNUM  *CurDq;
  const BIGNUM  *CurQInv;
  BOOLEAN       Result;

  PassDp   = NULL;
  PassDq   = NULL;
  PassQInv = NULL;
  Result   = FALSE;

  switch (KeyTag) {
    case RsaKeyDp:
      PassDp = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassDp == NULL) {
        goto Exit;
      }

      break;
    case RsaKeyDq:
      PassDq = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassDq == NULL) {
        goto Exit;
      }

      break;
    case RsaKeyQInv:
      PassQInv = BN_bin2bn (BigNumber, (UINT32)BnSize, NULL);
      if (PassQInv == NULL) {
        goto Exit;
      }

      break;
    default:
      goto Exit;
  }

  RSA_get0_crt_params (RsaKey, &CurDp, &CurDq, &CurQInv);

  if ((PassDp == NULL) && (CurDp == NULL)) {
    PassDp = BN_new ();
    if (PassDp == NULL) {
      goto Exit;
    }
  }

  if ((PassDq == NULL) && (CurDq == NULL)) {
    PassDq = BN_new ();
    if (PassDq == NULL) {
      goto Exit;
    }
  }

  if ((PassQInv == NULL) && (CurQInv == NULL)) {
    PassQInv = BN_new ();
    if (PassQInv == NULL) {
      goto Exit;
    }
  }

  if (RSA_set0_crt_params (RsaKey, PassDp, PassDq, PassQInv) == 0) {
    goto Exit;
  }

  PassDp   = NULL;
  PassDq   = NULL;
  PassQInv = NULL;
  Result   = TRUE;

Exit:
  BN_clear_free (PassDp);
  BN_clear_free (PassDq);
  BN_clear_free (PassQInv);
  return Result;
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
  RSA  *RsaKey;

  //
  // Check input parameters.
  //
  if ((RsaContext == NULL) || (BnSize > INT_MAX)) {
    return FALSE;
  }

  RsaKey = (RSA *)RsaContext;

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
      return RsaSetKeyNED (RsaKey, KeyTag, BigNumber, BnSize);

    //
    // RSA Secret Prime Factor of Modulus (p and q)
    //
    case RsaKeyP:
    case RsaKeyQ:
      return RsaSetKeyFactors (RsaKey, KeyTag, BigNumber, BnSize);

    //
    // p's CRT Exponent (== d mod (p - 1)),  q's CRT Exponent (== d mod (q - 1)),
    // and CRT Coefficient (== 1/q mod p)
    //
    case RsaKeyDp:
    case RsaKeyDq:
    case RsaKeyQInv:
      return RsaSetKeyCrtParams (RsaKey, KeyTag, BigNumber, BnSize);

    default:
      return FALSE;
  }
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
  INT32  DigestType;
  UINT8  *SigBuf;

  //
  // Check input parameters.
  //
  if ((RsaContext == NULL) || (MessageHash == NULL) || (Signature == NULL)) {
    return FALSE;
  }

  if ((SigSize > INT_MAX) || (SigSize == 0)) {
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

  SigBuf = (UINT8 *)Signature;
  return (BOOLEAN)RSA_verify (
                    DigestType,
                    MessageHash,
                    (UINT32)HashSize,
                    SigBuf,
                    (UINT32)SigSize,
                    (RSA *)RsaContext
                    );
}
