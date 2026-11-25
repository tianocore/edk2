/** @file
  RSA Asymmetric Cipher Wrapper Implementation over MbedTLS.

  This file implements following APIs which provide more capabilities for RSA:
  1) RsaGetKey
  2) RsaGenerateKey
  3) RsaCheckKey

  RFC 8017 - PKCS #1: RSA Cryptography Specifications Version 2.2

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/rsa.h>

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
  IN OUT VOID     *RsaContext,
  IN RSA_KEY_TAG  KeyTag,
  OUT UINT8       *BigNumber,
  IN OUT UINTN    *BnSize
  )
{
  mbedtls_rsa_context  *RsaKey;
  INT32                Ret;
  mbedtls_mpi          Value;
  UINTN                Size;

  //
  // Check input parameters.
  //
  if ((RsaContext == NULL) || (*BnSize > INT_MAX)) {
    return FALSE;
  }

  //
  // Init mbedtls_mpi
  //
  mbedtls_mpi_init (&Value);
  Size    = *BnSize;
  *BnSize = 0;

  RsaKey = (mbedtls_rsa_context *)RsaContext;

  switch (KeyTag) {
    //
    // RSA Public Modulus (N)
    //
    case RsaKeyN:
      Ret = mbedtls_rsa_export (RsaKey, &Value, NULL, NULL, NULL, NULL);
      break;
    //
    // RSA Public Exponent (e)
    //
    case RsaKeyE:
      Ret = mbedtls_rsa_export (RsaKey, NULL, NULL, NULL, NULL, &Value);
      break;
    //
    // Deprecated: Private RSA key components are no longer supported.
    //
    case RsaKeyD:
    case RsaKeyQ:
    case RsaKeyP:
    case RsaKeyDp:
    case RsaKeyDq:
    case RsaKeyQInv:
    default:
      Ret = -1;
      break;
  }

  if (Ret != 0) {
    goto End;
  }

  if (mbedtls_mpi_size (&Value) == 0) {
    Ret = 0;
    goto End;
  }

  *BnSize = Size;

  Size = mbedtls_mpi_size (&Value);
  if (*BnSize < Size) {
    Ret     = 1;
    *BnSize = Size;
    goto End;
  }

  if (BigNumber == NULL) {
    Ret     = 0;
    *BnSize = Size;
    goto End;
  }

  if ((BigNumber != NULL) && (Ret == 0)) {
    Ret     = mbedtls_mpi_write_binary (&Value, BigNumber, Size);
    *BnSize = Size;
  }

End:
  mbedtls_mpi_free (&Value);
  return Ret == 0;
}
