/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide more capabilities for RSA:
  1) RsaGetKey
  2) RsaGenerateKey
  3) RsaCheckKey
  4) RsaPkcs1Sign

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

//
// ASN.1 value for Hash Algorithm ID with the Distringuished Encoding Rules (DER)
// Refer to Section 9.2 of PKCS#1 v2.1
//                           
CONST UINT8  Asn1IdMd5[] = {
  0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86,
  0xf7, 0x0d, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10
  };

CONST UINT8  Asn1IdSha1[] = {
  0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
  0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
  };

CONST UINT8  Asn1IdSha256[] = {
  0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
  0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
  0x00, 0x04, 0x20
  };

/**
  Gets the tag-designated RSA key component from the established RSA context.

  This function retrieves the tag-designated RSA key component from the
  established RSA context as a non-negative integer (octet string format
  represented in RSA PKCS#1).
  If specified key component has not been set or has been cleared, then returned
  BnSize is set to 0.
  If the BigNumber buffer is too small to hold the contents of the key, FALSE
  is returned and BnSize is set to the required buffer size to obtain the key.

  If RsaContext is NULL, then return FALSE.
  If BnSize is NULL, then return FALSE.
  If BnSize is large enough but BigNumber is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[out]      BigNumber   Pointer to octet integer buffer.
  @param[in, out]  BnSize      On input, the size of big number buffer in bytes.
                               On output, the size of data returned in big number buffer in bytes.

  @retval  TRUE   RSA key component was retrieved successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  BnSize is too small.

**/
BOOLEAN
EFIAPI
RsaGetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  OUT     UINT8        *BigNumber,
  IN OUT  UINTN        *BnSize
  )
{
  RSA    *RsaKey;
  BIGNUM *BnKey;
  UINTN  Size;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL || BnSize == NULL) {
    return FALSE;
  }

  RsaKey  = (RSA *) RsaContext;
  Size    = *BnSize;
  *BnSize = 0;

  switch (KeyTag) {

  //
  // RSA Public Modulus (N)
  //
  case RsaKeyN:
    if (RsaKey->n == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->n;
    break;

  //
  // RSA Public Exponent (e)
  //
  case RsaKeyE:
    if (RsaKey->e == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->e;
    break;

  //
  // RSA Private Exponent (d)
  //
  case RsaKeyD:
    if (RsaKey->d == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->d;
    break;

  //
  // RSA Secret Prime Factor of Modulus (p)
  //
  case RsaKeyP:
    if (RsaKey->p == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->p;
    break;

  //
  // RSA Secret Prime Factor of Modules (q)
  //
  case RsaKeyQ:
    if (RsaKey->q == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->q;
    break;

  //
  // p's CRT Exponent (== d mod (p - 1))
  //
  case RsaKeyDp:
    if (RsaKey->dmp1 == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->dmp1;
    break;

  //
  // q's CRT Exponent (== d mod (q - 1))
  //
  case RsaKeyDq:
    if (RsaKey->dmq1 == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->dmq1;
    break;

  //
  // The CRT Coefficient (== 1/q mod p)
  //
  case RsaKeyQInv:
    if (RsaKey->iqmp == NULL) {
      return TRUE;
    }
    BnKey = RsaKey->iqmp;
    break;

  default:
    return FALSE;
  }

  *BnSize = Size;
  Size    = BN_num_bytes (BnKey);

  if (*BnSize < Size) {
    *BnSize = Size;
    return FALSE;
  }

  if (BigNumber == NULL) {
    return FALSE;
  }
  *BnSize = BN_bn2bin (BnKey, BigNumber) ;
  
  return TRUE;
}

/**
  Generates RSA key components.

  This function generates RSA key components. It takes RSA public exponent E and
  length in bits of RSA modulus N as input, and generates all key components.
  If PublicExponent is NULL, the default RSA public exponent (0x10001) will be used.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext           Pointer to RSA context being set.
  @param[in]       ModulusLength        Length of RSA modulus N in bits.
  @param[in]       PublicExponent       Pointer to RSA public exponent.
  @param[in]       PublicExponentSize   Size of RSA public exponent buffer in bytes. 

  @retval  TRUE   RSA key component was generated successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaGenerateKey (
  IN OUT  VOID         *RsaContext,
  IN      UINTN        ModulusLength,
  IN      CONST UINT8  *PublicExponent,
  IN      UINTN        PublicExponentSize
  )
{
  BIGNUM   *KeyE;
  BOOLEAN  RetVal;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL) {
    return FALSE;
  }
  
  KeyE = BN_new ();
  if (PublicExponent == NULL) {
    BN_set_word (KeyE, 0x10001);
  } else {
    BN_bin2bn (PublicExponent, (UINT32) PublicExponentSize, KeyE);
  }

  RetVal = FALSE;
  if (RSA_generate_key_ex ((RSA *) RsaContext, (UINT32) ModulusLength, KeyE, NULL) == 1) {
   RetVal = TRUE;
  }

  BN_free (KeyE);
  return RetVal;
}

/**
  Validates key components of RSA context.

  This function validates key compoents of RSA context in following aspects:
  - Whether p is a prime
  - Whether q is a prime
  - Whether n = p * q
  - Whether d*e = 1  mod lcm(p-1,q-1)

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to RSA context to check.

  @retval  TRUE   RSA key components are valid.
  @retval  FALSE  RSA key components are not valid.

**/
BOOLEAN
EFIAPI
RsaCheckKey (
  IN  VOID  *RsaContext
  )
{
  UINTN  Reason;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL) {
    return FALSE;
  }
  
  if  (RSA_check_key ((RSA *) RsaContext) != 1) {
    Reason = ERR_GET_REASON (ERR_peek_last_error ());
    if (Reason == RSA_R_P_NOT_PRIME ||
        Reason == RSA_R_Q_NOT_PRIME ||
        Reason == RSA_R_N_DOES_NOT_EQUAL_P_Q ||
        Reason == RSA_R_D_E_NOT_CONGRUENT_TO_1) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Performs the PKCS1-v1_5 encoding methods defined in RSA PKCS #1.

  @param  Message      Message buffer to be encoded.
  @param  MessageSize  Size of message buffer in bytes.
  @param  DigestInfo   Pointer to buffer of digest info for output.

  @return  Size of DigestInfo in bytes.

**/  
UINTN
DigestInfoEncoding (
  IN   CONST UINT8  *Message,
  IN   UINTN        MessageSize,
  OUT  UINT8        *DigestInfo
  )
{
  CONST UINT8  *HashDer;
  UINTN        DerSize;

  //
  // Check input parameters.
  //
  if (Message == NULL || DigestInfo == NULL) {
    return FALSE;
  }

  //
  // The original message length is used to determine the hash algorithm since
  // message is digest value hashed by the specified algorithm.
  //
  switch (MessageSize) {
  case MD5_DIGEST_SIZE:
    HashDer = Asn1IdMd5;
    DerSize = sizeof (Asn1IdMd5);
    break;
  
  case SHA1_DIGEST_SIZE:
    HashDer = Asn1IdSha1;
    DerSize = sizeof (Asn1IdSha1);
    break;
   
  case SHA256_DIGEST_SIZE:
    HashDer = Asn1IdSha256;
    DerSize = sizeof (Asn1IdSha256);
    break;
  
  default:
    return FALSE;
  }

  CopyMem (DigestInfo, HashDer, DerSize);
  CopyMem (DigestInfo + DerSize, Message, MessageSize);

  return (DerSize + MessageSize);
}

/**
  Carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.

  @param[in]       RsaContext   Pointer to RSA context for signature generation.
  @param[in]       MessageHash  Pointer to octet message hash to be signed.
  @param[in]       HashSize     Size of the message hash in bytes.
  @param[out]      Signature    Pointer to buffer to receive RSA PKCS1-v1_5 signature.
  @param[in, out]  SigSize      On input, the size of Signature buffer in bytes.
                                On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in PKCS1-v1_5.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.

**/
BOOLEAN
EFIAPI
RsaPkcs1Sign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  RSA      *Rsa;
  UINTN    Size;
  INTN     ReturnVal;

  //
  // Check input parameters.
  //
  if (RsaContext == NULL || MessageHash == NULL ||
    (HashSize != MD5_DIGEST_SIZE && HashSize != SHA1_DIGEST_SIZE && HashSize != SHA256_DIGEST_SIZE)) {
    return FALSE;
  }

  Rsa = (RSA *) RsaContext;
  Size = BN_num_bytes (Rsa->n);

  if (*SigSize < Size) {
    *SigSize = Size;
    return FALSE;
  }

  if (Signature == NULL) {
    return FALSE;
  }

  Size = DigestInfoEncoding (MessageHash, HashSize, Signature);

  ReturnVal = RSA_private_encrypt (
                (UINT32) Size,
                Signature,
                Signature,
                Rsa,
                RSA_PKCS1_PADDING
                );

  if (ReturnVal < (INTN) Size) {
    return FALSE;
  }

  *SigSize = (UINTN)ReturnVal;
  return TRUE;
}

