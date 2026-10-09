/** @file
  RSA Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file implements following APIs which provide more capabilities for RSA:
  1) RsaGetKey
  2) RsaGenerateKey
  3) RsaCheckKey

Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/objects.h>

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
  RSA     *RsaKey;
  BIGNUM  *BnKey;
  UINTN   Size;

  //
  // Check input parameters.
  //
  if ((RsaContext == NULL) || (BnSize == NULL)) {
    return FALSE;
  }

  RsaKey  = (RSA *)RsaContext;
  Size    = *BnSize;
  *BnSize = 0;
  BnKey   = NULL;

  switch (KeyTag) {
    //
    // RSA Public Modulus (N)
    //
    case RsaKeyN:
      RSA_get0_key (RsaKey, (const BIGNUM **)&BnKey, NULL, NULL);
      break;

    //
    // RSA Public Exponent (e)
    //
    case RsaKeyE:
      RSA_get0_key (RsaKey, NULL, (const BIGNUM **)&BnKey, NULL);
      break;

    //
    // RSA Private Exponent (d)
    //
    case RsaKeyD:
      RSA_get0_key (RsaKey, NULL, NULL, (const BIGNUM **)&BnKey);
      break;

    //
    // RSA Secret Prime Factor of Modulus (p)
    //
    case RsaKeyP:
      RSA_get0_factors (RsaKey, (const BIGNUM **)&BnKey, NULL);
      break;

    //
    // RSA Secret Prime Factor of Modules (q)
    //
    case RsaKeyQ:
      RSA_get0_factors (RsaKey, NULL, (const BIGNUM **)&BnKey);
      break;

    //
    // p's CRT Exponent (== d mod (p - 1))
    //
    case RsaKeyDp:
      RSA_get0_crt_params (RsaKey, (const BIGNUM **)&BnKey, NULL, NULL);
      break;

    //
    // q's CRT Exponent (== d mod (q - 1))
    //
    case RsaKeyDq:
      RSA_get0_crt_params (RsaKey, NULL, (const BIGNUM **)&BnKey, NULL);
      break;

    //
    // The CRT Coefficient (== 1/q mod p)
    //
    case RsaKeyQInv:
      RSA_get0_crt_params (RsaKey, NULL, NULL, (const BIGNUM **)&BnKey);
      break;

    default:
      return FALSE;
  }

  if (BnKey == NULL) {
    return FALSE;
  }

  *BnSize = Size;
  Size    = BN_num_bytes (BnKey);

  if (*BnSize < Size) {
    *BnSize = Size;
    return FALSE;
  }

  if (BigNumber == NULL) {
    *BnSize = Size;
    return TRUE;
  }

  *BnSize = BN_bn2bin (BnKey, BigNumber);

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
  if ((RsaContext == NULL) || (ModulusLength > INT_MAX) || (PublicExponentSize > INT_MAX)) {
    return FALSE;
  }

  KeyE = BN_new ();
  if (KeyE == NULL) {
    return FALSE;
  }

  RetVal = FALSE;

  if (PublicExponent == NULL) {
    if (BN_set_word (KeyE, 0x10001) == 0) {
      goto _Exit;
    }
  } else {
    if (BN_bin2bn (PublicExponent, (UINT32)PublicExponentSize, KeyE) == NULL) {
      goto _Exit;
    }
  }

  if (RSA_generate_key_ex ((RSA *)RsaContext, (UINT32)ModulusLength, KeyE, NULL) == 1) {
    RetVal = TRUE;
  }

_Exit:
  BN_free (KeyE);
  return RetVal;
}

/**
  Validates key components of RSA context.
  NOTE: This function performs integrity checks on all the RSA key material, so
        the RSA key structure must contain all the private key data.

  This function validates key components of RSA context in following aspects:
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

  if (RSA_check_key ((RSA *)RsaContext) != 1) {
    Reason = ERR_GET_REASON (ERR_peek_last_error ());
    if ((Reason == RSA_R_P_NOT_PRIME) ||
        (Reason == RSA_R_Q_NOT_PRIME) ||
        (Reason == RSA_R_N_DOES_NOT_EQUAL_P_Q) ||
        (Reason == RSA_R_D_E_NOT_CONGRUENT_TO_1))
    {
      return FALSE;
    }
  }

  return TRUE;
}
