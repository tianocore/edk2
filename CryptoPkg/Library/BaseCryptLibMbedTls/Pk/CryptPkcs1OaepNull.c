/** @file
  This file contains UEFI wrapper functions for RSA PKCS1v2 OAEP encryption routines.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "InternalCryptLib.h"

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Return FALSE to indicate this interface is not supported.

  @param[in]  PublicKey           A pointer to the DER-encoded X509 certificate that
                                  will be used to encrypt the data.
  @param[in]  PublicKeySize       Size of the X509 cert buffer.
  @param[in]  InData              Data to be encrypted.
  @param[in]  InDataSize          Size of the data buffer.
  @param[in]  PrngSeed            [Optional] If provided, a pointer to a random seed buffer
                                  to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize        [Optional] If provided, size of the random seed buffer.
                                  0 otherwise.
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval FALSE                   This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs1v2Encrypt (
  IN   CONST UINT8  *PublicKey,
  IN   UINTN        PublicKeySize,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed  OPTIONAL,
  IN   UINTN        PrngSeedSize  OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size
    and hash digest size).

  @param[in]  RsaContext          A pointer to an RSA context created by RsaNew() and
                                  provisioned with a public key using RsaSetKey().
  @param[in]  InData              Data to be encrypted.
  @param[in]  InDataSize          Size of the data buffer.
  @param[in]  PrngSeed            [Optional] If provided, a pointer to a random seed buffer
                                  to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize        [Optional] If provided, size of the random seed buffer.
                                  0 otherwise.
  @param[in]  DigestLen           [Optional] If provided, size of the hash used:
                                  SHA1_DIGEST_SIZE
                                  SHA256_DIGEST_SIZE
                                  SHA384_DIGEST_SIZE
                                  SHA512_DIGEST_SIZE
                                  0 to use default (SHA1)
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
RsaOaepEncrypt (
  IN   VOID         *RsaContext,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed   OPTIONAL,
  IN   UINTN        PrngSeedSize   OPTIONAL,
  IN   UINT16       DigestLen   OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Decrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  decrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - Fail to parse private key.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.

  @param[in]  PrivateKey          A pointer to the DER-encoded private key.
  @param[in]  PrivateKeySize      Size of the private key buffer.
  @param[in]  EncryptedData       Data to be decrypted.
  @param[in]  EncryptedDataSize   Size of the encrypted buffer.
  @param[out] OutData             Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] OutDataSize         Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
Pkcs1v2Decrypt (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   UINT8        *EncryptedData,
  IN   UINTN        EncryptedDataSize,
  OUT  UINT8        **OutData,
  OUT  UINTN        *OutDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Decrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  decrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - Fail to parse private key.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.

  @param[in]  RsaContext          A pointer to an RSA context created by RsaNew() and
                                  provisioned with a private key using RsaSetKey().
  @param[in]  EncryptedData       Data to be decrypted.
  @param[in]  EncryptedDataSize   Size of the encrypted buffer.
  @param[in]  DigestLen           [Optional] If provided, size of the hash used:
                                  SHA1_DIGEST_SIZE
                                  SHA256_DIGEST_SIZE
                                  SHA384_DIGEST_SIZE
                                  SHA512_DIGEST_SIZE
                                  0 to use default (SHA1)
  @param[out] OutData             Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] OutDataSize         Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
RsaOaepDecrypt (
  IN   VOID    *RsaContext,
  IN   UINT8   *EncryptedData,
  IN   UINTN   EncryptedDataSize,
  IN   UINT16  DigestLen   OPTIONAL,
  OUT  UINT8   **OutData,
  OUT  UINTN   *OutDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
