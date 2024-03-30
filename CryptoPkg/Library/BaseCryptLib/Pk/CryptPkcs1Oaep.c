/** @file
  This file contains UEFI wrapper functions for RSA PKCS1v2 OAEP encryption routines.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (C) Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

**/

#include "InternalCryptLib.h"
#include <openssl/objects.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <Library/MemoryAllocationLib.h>

/**
  Retrieve a pointer to EVP message digest object.

  @param[in]  DigestLen   Length of the message digest.

**/
STATIC
const
EVP_MD *
GetEvpMD (
  IN UINT16  DigestLen
  )
{
  switch (DigestLen) {
    case SHA1_DIGEST_SIZE:
      return EVP_sha1 ();
      break;
    case SHA256_DIGEST_SIZE:
      return EVP_sha256 ();
      break;
    case SHA384_DIGEST_SIZE:
      return EVP_sha384 ();
      break;
    case SHA512_DIGEST_SIZE:
      return EVP_sha512 ();
      break;
    default:
      return NULL;
  }
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to parse X509 certificate.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size
    and hash digest size).

  @param[in]  Pkey                A pointer to an EVP_PKEY struct that
                                  will be used to encrypt the data.
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
InternalPkcs1v2Encrypt (
  EVP_PKEY          *Pkey,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed   OPTIONAL,
  IN   UINTN        PrngSeedSize   OPTIONAL,
  IN   UINT16       DigestLen   OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  BOOLEAN       Result;
  EVP_PKEY_CTX  *PkeyCtx;
  UINT8         *OutData;
  UINTN         OutDataSize;
  CONST EVP_MD  *HashAlg;

  //
  // Check input parameters.
  //
  if ((Pkey == NULL) || (InData == NULL) ||
      (EncryptedData == NULL) || (EncryptedDataSize == NULL))
  {
    return FALSE;
  }

  *EncryptedData     = NULL;
  *EncryptedDataSize = 0;
  Result             = FALSE;
  PkeyCtx            = NULL;
  OutData            = NULL;
  OutDataSize        = 0;

  //
  // If it provides a seed then use it.
  // Ohterwise, we'll seed with fixed values and hope that the PRNG has already been
  // used enough to generate sufficient entropy.
  //
  if (PrngSeed != NULL) {
    RandomSeed (PrngSeed, PrngSeedSize);
  } else {
    RandomSeed (NULL, 0);
  }

  //
  // Create a context for the public key operation.
  //
  PkeyCtx = EVP_PKEY_CTX_new (Pkey, NULL);
  if (PkeyCtx == NULL) {
    //
    // Fail to create contex.
    //
    goto _Exit;
  }

  //
  // Initialize the context and set the desired padding.
  //
  if ((EVP_PKEY_encrypt_init (PkeyCtx) <= 0) ||
      (EVP_PKEY_CTX_set_rsa_padding (PkeyCtx, RSA_PKCS1_OAEP_PADDING) <= 0))
  {
    //
    // Fail to initialize the context.
    //
    goto _Exit;
  }

  if (DigestLen != 0) {
    HashAlg = GetEvpMD (DigestLen);
    if (HashAlg == NULL) {
      goto _Exit;
    }

    if (EVP_PKEY_CTX_set_rsa_oaep_md (PkeyCtx, HashAlg) <= 0) {
      goto _Exit;
    }

    if (EVP_PKEY_CTX_set_rsa_mgf1_md (PkeyCtx, HashAlg) <= 0) {
      goto _Exit;
    }
  }

  //
  // Determine the required buffer length for malloc'ing.
  //
  if (EVP_PKEY_encrypt (PkeyCtx, NULL, &OutDataSize, InData, InDataSize) <= 0) {
    //
    // Fail to determine output buffer size.
    //
    goto _Exit;
  }

  //
  // Allocate a buffer for the output data.
  //
  OutData = AllocatePool (OutDataSize);
  if (OutData == NULL) {
    //
    // Fail to allocate the output buffer.
    //
    goto _Exit;
  }

  //
  // Encrypt Data.
  //
  if (EVP_PKEY_encrypt (PkeyCtx, OutData, &OutDataSize, InData, InDataSize) <= 0) {
    //
    // Fail to encrypt data, need to free the output buffer.
    //
    FreePool (OutData);
    OutData     = NULL;
    OutDataSize = 0;
    goto _Exit;
  }

  //
  // Encrypt done.
  //
  *EncryptedData     = OutData;
  *EncryptedDataSize = OutDataSize;
  Result             = TRUE;

_Exit:
  //
  // Release Resources
  //
  if (PkeyCtx != NULL) {
    EVP_PKEY_CTX_free (PkeyCtx);
  }

  return Result;
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to parse X509 certificate.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size
    and hash digest size).

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

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
Pkcs1v2Encrypt (
  IN   CONST UINT8  *PublicKey,
  IN   UINTN        PublicKeySize,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed   OPTIONAL,
  IN   UINTN        PrngSeedSize   OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  BOOLEAN      Result;
  CONST UINT8  *TempPointer;
  X509         *CertData;
  EVP_PKEY     *Pkey;

  //
  // Check input parameters.
  //
  if ((PublicKey == NULL) || (InData == NULL) ||
      (EncryptedData == NULL) || (EncryptedDataSize == NULL))
  {
    return FALSE;
  }

  //
  // Check public key size.
  //
  if (PublicKeySize > 0xFFFFFFFF) {
    //
    // Public key size is too large for implementation.
    //
    return FALSE;
  }

  *EncryptedData     = NULL;
  *EncryptedDataSize = 0;
  Result             = FALSE;
  TempPointer        = NULL;
  CertData           = NULL;
  Pkey               = NULL;

  //
  // Parse the X509 cert and extract the public key.
  //
  TempPointer = PublicKey;
  CertData    = d2i_X509 (&CertData, &TempPointer, (UINT32)PublicKeySize);
  if (CertData == NULL) {
    //
    // Fail to parse X509 cert.
    //
    goto _Exit;
  }

  //
  // Extract the public key from the x509 cert in a format that
  // OpenSSL can use.
  //
  Pkey = X509_get_pubkey (CertData);
  if (Pkey == NULL) {
    //
    // Fail to extract public key.
    //
    goto _Exit;
  }

  Result = InternalPkcs1v2Encrypt (Pkey, InData, InDataSize, PrngSeed, PrngSeedSize, 0, EncryptedData, EncryptedDataSize);

_Exit:
  //
  // Release Resources
  //
  if (CertData != NULL) {
    X509_free (CertData);
  }

  if (Pkey != NULL) {
    EVP_PKEY_free (Pkey);
  }

  return Result;
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
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
  IN   UINT16       DigestLen OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  BOOLEAN   Result;
  EVP_PKEY  *Pkey;

  //
  // Check input parameters.
  //
  if (((RsaContext == NULL) || (InData == NULL)) ||
      (EncryptedData == NULL) || (EncryptedDataSize == NULL))
  {
    return FALSE;
  }

  *EncryptedData     = NULL;
  *EncryptedDataSize = 0;
  Result             = FALSE;
  Pkey               = NULL;

  Pkey = EVP_PKEY_new ();
  if (Pkey == NULL) {
    goto _Exit;
  }

  if (EVP_PKEY_set1_RSA (Pkey, (RSA *)RsaContext) == 0) {
    goto _Exit;
  }

  Result = InternalPkcs1v2Encrypt (Pkey, InData, InDataSize, PrngSeed, PrngSeedSize, DigestLen, EncryptedData, EncryptedDataSize);

_Exit:
  //
  // Release Resources
  //
  if (Pkey != NULL) {
    EVP_PKEY_free (Pkey);
  }

  return Result;
}

/**
  Decrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  decrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - Fail to parse private key.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.

  @param[in]  Pkey                A pointer to an EVP_PKEY which will decrypt that data.
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
InternalPkcs1v2Decrypt (
  EVP_PKEY     *Pkey,
  IN   UINT8   *EncryptedData,
  IN   UINTN   EncryptedDataSize,
  IN   UINT16  DigestLen   OPTIONAL,
  OUT  UINT8   **OutData,
  OUT  UINTN   *OutDataSize
  )
{
  BOOLEAN       Result;
  EVP_PKEY_CTX  *PkeyCtx;
  UINT8         *TempData;
  UINTN         TempDataSize;
  INTN          ReturnCode;
  CONST EVP_MD  *HashAlg;

  //
  // Check input parameters.
  //
  if ((Pkey == NULL) || (EncryptedData == NULL) ||
      (OutData == NULL) || (OutDataSize == NULL))
  {
    return FALSE;
  }

  Result       = FALSE;
  PkeyCtx      = NULL;
  TempData     = NULL;
  TempDataSize = 0;

  //
  // Create a context for the decryption operation.
  //
  PkeyCtx = EVP_PKEY_CTX_new (Pkey, NULL);
  if (PkeyCtx == NULL) {
    //
    // Fail to create contex.
    //
    DEBUG ((DEBUG_ERROR, "[%a] EVP_PKEY_CTK_new() failed\n", __func__));
    goto _Exit;
  }

  //
  // Initialize the context and set the desired padding.
  //
  if ((EVP_PKEY_decrypt_init (PkeyCtx) <= 0) ||
      (EVP_PKEY_CTX_set_rsa_padding (PkeyCtx, RSA_PKCS1_OAEP_PADDING) <= 0))
  {
    //
    // Fail to initialize the context.
    //
    DEBUG ((DEBUG_ERROR, "[%a] EVP_PKEY_decrypt_init() failed\n", __func__));
    goto _Exit;
  }

  if (DigestLen != 0) {
    HashAlg = GetEvpMD (DigestLen);
    if (HashAlg == NULL) {
      goto _Exit;
    }

    if (EVP_PKEY_CTX_set_rsa_oaep_md (PkeyCtx, HashAlg) <= 0) {
      goto _Exit;
    }

    if (EVP_PKEY_CTX_set_rsa_mgf1_md (PkeyCtx, HashAlg) <= 0) {
      goto _Exit;
    }
  }

  //
  // Determine the required buffer length for malloc'ing.
  //
  ReturnCode = EVP_PKEY_decrypt (PkeyCtx, NULL, &TempDataSize, EncryptedData, EncryptedDataSize);
  if (ReturnCode <= 0) {
    //
    // Fail to determine output buffer size.
    //
    DEBUG ((DEBUG_ERROR, "[%a] EVP_PKEY_decrypt() failed to determine output buffer size (rc=%d)\n", __func__, ReturnCode));
    goto _Exit;
  }

  //
  // Allocate a buffer for the output data.
  //
  TempData = AllocatePool (TempDataSize);
  if (TempData == NULL) {
    //
    // Fail to allocate the output buffer.
    //
    goto _Exit;
  }

  //
  // Decrypt Data.
  //
  ReturnCode = EVP_PKEY_decrypt (PkeyCtx, TempData, &TempDataSize, EncryptedData, EncryptedDataSize);
  if (ReturnCode <= 0) {
    //
    // Fail to decrypt data, need to free the output buffer.
    //
    FreePool (TempData);
    TempData     = NULL;
    TempDataSize = 0;

    DEBUG ((DEBUG_ERROR, "[%a] EVP_PKEY_decrypt(TempData) failed to decrypt (rc=%d)\n", __func__, ReturnCode));
    goto _Exit;
  }

  //
  // Decrypt done.
  //
  *OutData     = TempData;
  *OutDataSize = TempDataSize;
  Result       = TRUE;

_Exit:
  if (PkeyCtx != NULL) {
    EVP_PKEY_CTX_free (PkeyCtx);
  }

  return Result;
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
  BOOLEAN      Result;
  EVP_PKEY     *Pkey;
  CONST UINT8  *TempPointer;

  //
  // Check input parameters.
  //
  if ((PrivateKey == NULL) || (EncryptedData == NULL) ||
      (OutData == NULL) || (OutDataSize == NULL))
  {
    return FALSE;
  }

  Result      = FALSE;
  Pkey        = NULL;
  TempPointer = NULL;

  //
  // Parse the private key.
  //
  TempPointer = PrivateKey;
  Pkey        = d2i_PrivateKey (EVP_PKEY_RSA, &Pkey, &TempPointer, (UINT32)PrivateKeySize);
  if (Pkey == NULL) {
    //
    // Fail to parse private key.
    //
    DEBUG ((DEBUG_ERROR, "[%a] d2i_PrivateKey() failed\n", __func__));
    goto _Exit;
  }

  Result = InternalPkcs1v2Decrypt (Pkey, EncryptedData, EncryptedDataSize, 0, OutData, OutDataSize);

_Exit:
  if (Pkey != NULL) {
    EVP_PKEY_free (Pkey);
  }

  return Result;
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
  IN   UINT16  DigestLen OPTIONAL,
  OUT  UINT8   **OutData,
  OUT  UINTN   *OutDataSize
  )
{
  BOOLEAN   Result;
  EVP_PKEY  *Pkey;

  //
  // Check input parameters.
  //
  if ((RsaContext == NULL) || (EncryptedData == NULL) ||
      (OutData == NULL) || (OutDataSize == NULL))
  {
    return FALSE;
  }

  Result = FALSE;
  Pkey   = NULL;

  //
  // Create a context for the decryption operation.
  //

  Pkey = EVP_PKEY_new ();
  if (Pkey == NULL) {
    goto _Exit;
  }

  if (EVP_PKEY_set1_RSA (Pkey, (RSA *)RsaContext) == 0) {
    goto _Exit;
  }

  Result = InternalPkcs1v2Decrypt (Pkey, EncryptedData, EncryptedDataSize, DigestLen, OutData, OutDataSize);

_Exit:
  if (Pkey != NULL) {
    EVP_PKEY_free (Pkey);
  }

  return Result;
}
