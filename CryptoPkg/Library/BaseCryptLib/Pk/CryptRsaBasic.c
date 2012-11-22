/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaNew
  2) RsaFree
  3) RsaSetKey
  4) RsaPkcs1Verify

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

#include <openssl/rsa.h>
#include <openssl/err.h>


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
  If BigNumber is NULL, then the specified key componenet in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key componenet in RSA
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
  if (RsaContext == NULL || BnSize > INT_MAX) {
    return FALSE;
  }

  RsaKey = (RSA *) RsaContext;
  //
  // Set RSA Key Components by converting octet string to OpenSSL BN representation.
  // NOTE: For RSA public key (used in signature verification), only public components
  //       (N, e) are needed.
  //
  switch (KeyTag) {

  //
  // RSA Public Modulus (N)
  //
  case RsaKeyN:
    if (RsaKey->n != NULL) {
      BN_free (RsaKey->n);
    }
    RsaKey->n = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->n = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->n);
    if (RsaKey->n == NULL) {
      return FALSE;
    }

    break;

  //
  // RSA Public Exponent (e)
  //
  case RsaKeyE:
    if (RsaKey->e != NULL) {
      BN_free (RsaKey->e);
    }
    RsaKey->e = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->e = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->e);
    if (RsaKey->e == NULL) {
      return FALSE;
    }

    break;

  //
  // RSA Private Exponent (d)
  //
  case RsaKeyD:
    if (RsaKey->d != NULL) {
      BN_free (RsaKey->d);
    }
    RsaKey->d = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->d = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->d);
    if (RsaKey->d == NULL) {
      return FALSE;
    }

    break;

  //
  // RSA Secret Prime Factor of Modulus (p)
  //
  case RsaKeyP:
    if (RsaKey->p != NULL) {
      BN_free (RsaKey->p);
    }
    RsaKey->p = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->p = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->p);
    if (RsaKey->p == NULL) {
      return FALSE;
    }

    break;

  //
  // RSA Secret Prime Factor of Modules (q)
  //
  case RsaKeyQ:
    if (RsaKey->q != NULL) {
      BN_free (RsaKey->q);
    }
    RsaKey->q = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->q = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->q);
    if (RsaKey->q == NULL) {
      return FALSE;
    }

    break;

  //
  // p's CRT Exponent (== d mod (p - 1))
  //
  case RsaKeyDp:
    if (RsaKey->dmp1 != NULL) {
      BN_free (RsaKey->dmp1);
    }
    RsaKey->dmp1 = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->dmp1 = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->dmp1);
    if (RsaKey->dmp1 == NULL) {
      return FALSE;
    }

    break;

  //
  // q's CRT Exponent (== d mod (q - 1))
  //
  case RsaKeyDq:
    if (RsaKey->dmq1 != NULL) {
      BN_free (RsaKey->dmq1);
    }
    RsaKey->dmq1 = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->dmq1 = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->dmq1);
    if (RsaKey->dmq1 == NULL) {
      return FALSE;
    }

    break;

  //
  // The CRT Coefficient (== 1/q mod p)
  //
  case RsaKeyQInv:
    if (RsaKey->iqmp != NULL) {
      BN_free (RsaKey->iqmp);
    }
    RsaKey->iqmp = NULL;
    if (BigNumber == NULL) {
      break;
    }
    RsaKey->iqmp = BN_bin2bn (BigNumber, (UINT32) BnSize, RsaKey->iqmp);
    if (RsaKey->iqmp == NULL) {
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
  If HashSize is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.

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
  INTN     Length;
  UINT8    *DecryptedSigature;

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
  // Check for unsupported hash size:
  //    Only MD5, SHA-1 or SHA-256 digest size is supported
  //
  if (HashSize != MD5_DIGEST_SIZE && HashSize != SHA1_DIGEST_SIZE && HashSize != SHA256_DIGEST_SIZE) {
    return FALSE;
  }

  //
  // Prepare buffer to store decrypted signature.
  //
  DecryptedSigature = (UINT8 *) malloc (SigSize);
  if (DecryptedSigature == NULL) {
    return FALSE;
  }

  //
  // RSA PKCS#1 Signature Decoding using OpenSSL RSA Decryption with Public Key
  //
  Length = RSA_public_decrypt (
             (UINT32) SigSize,
             Signature,
             DecryptedSigature,
             RsaContext,
             RSA_PKCS1_PADDING
             );

  //
  // Invalid RSA Key or PKCS#1 Padding Checking Failed (if Length < 0)
  // NOTE: Length should be the addition of HashSize and some DER value.
  //       Ignore more strict length checking here.
  //
  if (Length < (INTN) HashSize) {
    free (DecryptedSigature);
    return FALSE;
  }

  //
  // Validate the MessageHash and Decoded Signature
  // NOTE: The decoded Signature should be the DER encoding of the DigestInfo value
  //       DigestInfo ::= SEQUENCE {
  //           digestAlgorithm AlgorithmIdentifier
  //           digest OCTET STRING
  //       }
  //       Then Memory Comparing should skip the DER value of the underlying SEQUENCE
  //       type and AlgorithmIdentifier.
  //
  if (CompareMem (MessageHash, DecryptedSigature + Length - HashSize, HashSize) == 0) {
    //
    // Valid RSA PKCS#1 Signature
    //
    free (DecryptedSigature);
    return TRUE;
  } else {
    //
    // Failed to verification
    //
    free (DecryptedSigature);
    return FALSE;
  }
}
