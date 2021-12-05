/** @file
  This file contains UEFI wrapper functions for RSA PKCS1v2 OAEP encryption routines.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

**/

#include "InternalCryptLib.h"
#include <openssl/objects.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <Library/MemoryAllocationLib.h>

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
  BOOLEAN       Result;
  CONST UINT8   *TempPointer;
  X509          *CertData;
  EVP_PKEY      *InternalPublicKey;
  EVP_PKEY_CTX  *PkeyCtx;
  UINT8         *OutData;
  UINTN         OutDataSize;

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
  InternalPublicKey  = NULL;
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
  InternalPublicKey = X509_get_pubkey (CertData);
  if (InternalPublicKey == NULL) {
    //
    // Fail to extract public key.
    //
    goto _Exit;
  }

  //
  // Create a context for the public key operation.
  //
  PkeyCtx = EVP_PKEY_CTX_new (InternalPublicKey, NULL);
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
  if (CertData != NULL) {
    X509_free (CertData);
  }

  if (InternalPublicKey != NULL) {
    EVP_PKEY_free (InternalPublicKey);
  }

  if (PkeyCtx != NULL) {
    EVP_PKEY_CTX_free (PkeyCtx);
  }

  return Result;
}
