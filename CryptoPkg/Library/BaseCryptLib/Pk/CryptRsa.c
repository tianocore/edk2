/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/BaseCryptLib.h>
#include <openssl/rsa.h>


/**
  Allocates and Initializes one RSA Context for subsequent use.

  @return  Pointer to the RSA Context that has been initialized.
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
  Release the specified RSA Context.

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
  Sets the tag-designated RSA key component into the established RSA context from
  the user-specified nonnegative integer (octet string format represented in RSA
  PKCS#1).

  If RsaContext is NULL, then ASSERT().

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
  @param[in]       BnLength    Length of big number buffer in bytes.

  @return  TRUE   RSA key component was set successfully.
  @return  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
  IN OUT VOID         *RsaContext,
  IN     RSA_KEY_TAG  KeyTag,
  IN     CONST UINT8  *BigNumber,
  IN     UINTN        BnLength
  )
{
  RSA  *RsaKey;

  //
  // ASSERT if RsaContext is NULL
  //
  ASSERT (RsaContext != NULL);


  RsaKey = (RSA *)RsaContext;
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
    RsaKey->n = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->n);
    break;

  //
  // RSA Public Exponent (e)
  //
  case RsaKeyE:
    if (RsaKey->e != NULL) {
      BN_free (RsaKey->e);
    }
    RsaKey->e = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->e);
    break;

  //
  // RSA Private Exponent (d)
  //
  case RsaKeyD:
    if (RsaKey->d != NULL) {
      BN_free (RsaKey->d);
    }
    RsaKey->d = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->d);
    break;

  //
  // RSA Secret Prime Factor of Modulus (p)
  //
  case RsaKeyP:
    if (RsaKey->p != NULL) {
      BN_free (RsaKey->p);
    }
    RsaKey->p = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->p);
    break;

  //
  // RSA Secret Prime Factor of Modules (q)
  //
  case RsaKeyQ:
    if (RsaKey->q != NULL) {
      BN_free (RsaKey->q);
    }
    RsaKey->q = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->q);
    break;

  //
  // p's CRT Exponent (== d mod (p - 1))
  //
  case RsaKeyDp:
    if (RsaKey->dmp1 != NULL) {
      BN_free (RsaKey->dmp1);
    }
    RsaKey->dmp1 = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->dmp1);
    break;

  //
  // q's CRT Exponent (== d mod (q - 1))
  //
  case RsaKeyDq:
    if (RsaKey->dmq1 != NULL) {
      BN_free (RsaKey->dmq1);
    }
    RsaKey->dmq1 = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->dmq1);
    break;

  //
  // The CRT Coefficient (== 1/q mod p)
  //
  case RsaKeyQInv:
    if (RsaKey->iqmp != NULL) {
      BN_free (RsaKey->iqmp);
    }
    RsaKey->iqmp = BN_bin2bn (BigNumber, (int)BnLength, RsaKey->iqmp);
    break;

  default:
    return FALSE;
  }

  return TRUE;
}


/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then ASSERT().
  If MessageHash is NULL, then ASSERT().
  If Signature is NULL, then ASSERT().
  If HashLength is not equal to the size of MD5, SHA-1 or SHA-256 digest, then ASSERT().

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashLength   Length of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigLength    Length of signature in bytes.

  @return  TRUE   Valid signature encoded in PKCS1-v1_5.
  @return  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashLength,
  IN  UINT8        *Signature,
  IN  UINTN        SigLength
  )
{
  INTN     Length;

  //
  // ASSERT if RsaContext, MessageHash or Signature is NULL
  //
  ASSERT (RsaContext  != NULL);
  ASSERT (MessageHash != NULL);
  ASSERT (Signature   != NULL);

  //
  // ASSERT if unsupported hash length:
  //    Only MD5, SHA-1 or SHA-256 digest size is supported
  //
  ASSERT ((HashLength == MD5_DIGEST_SIZE) || (HashLength == SHA1_DIGEST_SIZE) ||
          (HashLength == SHA256_DIGEST_SIZE));

  //
  // RSA PKCS#1 Signature Decoding using OpenSSL RSA Decryption with Public Key
  //
  Length = RSA_public_decrypt (
             (int)SigLength,
             Signature,
             Signature,
             RsaContext,
             RSA_PKCS1_PADDING
             );

  //
  // Invalid RSA Key or PKCS#1 Padding Checking Failed (if Length < 0)
  // NOTE: Length should be the addition of HashLength and some DER value.
  //       Ignore more strict length checking here.
  //
  if (Length < (INTN) HashLength) {
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
  if (CompareMem (MessageHash, Signature + Length - HashLength, HashLength) == 0) {
    //
    // Valid RSA PKCS#1 Signature
    //
    return TRUE;
  } else {
    //
    // Failed to verification
    //
    return FALSE;
  }
}
