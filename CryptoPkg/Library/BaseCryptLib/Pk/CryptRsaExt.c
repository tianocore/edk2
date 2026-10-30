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
      //
      // Deprecated: RSA private exponent (d) is no longer supported.
      //
      break;

    //
    // RSA Secret Prime Factor of Modulus (p)
    //
    case RsaKeyP:
      //
      // Deprecated: RSA secret prime factor p is no longer supported.
      //
      break;

    //
    // RSA Secret Prime Factor of Modules (q)
    //
    case RsaKeyQ:
      //
      // Deprecated: RSA secret prime factor q is no longer supported.
      //
      break;

    //
    // p's CRT Exponent (== d mod (p - 1))
    //
    case RsaKeyDp:
      //
      // Deprecated: RSA CRT exponent dp is no longer supported.
      //
      break;

    //
    // q's CRT Exponent (== d mod (q - 1))
    //
    case RsaKeyDq:
      //
      // Deprecated: RSA CRT exponent dq is no longer supported.
      //
      break;

    //
    // The CRT Coefficient (== 1/q mod p)
    //
    case RsaKeyQInv:
      //
      // Deprecated: RSA CRT coefficient qInv is no longer supported.
      //
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
