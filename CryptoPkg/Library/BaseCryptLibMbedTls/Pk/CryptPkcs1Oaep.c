/** @file
  This file contains UEFI wrapper functions for RSA PKCS1v2 OAEP encryption routines.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

**/

#include "InternalCryptLib.h"
#include <mbedtls/rsa.h>
#include <mbedtls/x509_crt.h>
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
  IN   CONST UINT8  *PrngSeed  OPTIONAL,
  IN   UINTN        PrngSeedSize  OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  BOOLEAN              Result;
  UINT32               Ret;
  UINT8                *OutData;
  mbedtls_x509_crt     CertContext;
  mbedtls_rsa_context  RsaContext;

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
  if (PublicKeySize > UINT_MAX) {
    //
    // Public key size is too large for implementation.
    //
    return FALSE;
  }

  *EncryptedData     = NULL;
  *EncryptedDataSize = 0;
  Result             = FALSE;
  OutData            = NULL;

  mbedtls_x509_crt_init (&CertContext);

  if (mbedtls_x509_crt_parse_der (&CertContext, PublicKey, (UINT32)PublicKeySize) != 0) {
    goto _Exit;
  }

  if (mbedtls_pk_get_type (&CertContext.pk) != MBEDTLS_PK_RSA) {
    goto _Exit;
  }

  mbedtls_rsa_init (&RsaContext);
  if (mbedtls_rsa_set_padding (&RsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_NONE) != 0) {
    goto _Exit;
  }

  Ret = mbedtls_rsa_copy (&RsaContext, mbedtls_pk_rsa (CertContext.pk));
  if (Ret != 0) {
    goto _Exit;
  }

  *EncryptedDataSize = RsaContext.len;

  //
  // Allocate a buffer for the output data.
  //
  OutData = AllocateZeroPool (*EncryptedDataSize);
  if (OutData == NULL) {
    //
    // Fail to allocate the output buffer.
    //
    goto _Exit;
  }

  Ret = mbedtls_rsa_pkcs1_encrypt (
                                   &RsaContext,
                                   MbedtlsRand,
                                   NULL,
                                   InDataSize,
                                   InData,
                                   OutData
                                   );
  if (Ret != 0) {
    FreePool (OutData);
    OutData = NULL;
    goto _Exit;
  }

  *EncryptedData = OutData;
  Result         = TRUE;

_Exit:
  //
  // Release Resources
  //
  if (&CertContext != NULL) {
    mbedtls_x509_crt_free (&CertContext);
  }

  if (&RsaContext != NULL) {
    mbedtls_rsa_free (&RsaContext);
  }

  return Result;
}
