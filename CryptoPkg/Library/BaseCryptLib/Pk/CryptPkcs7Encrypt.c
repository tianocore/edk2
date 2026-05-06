/** @file
  PKCS7 Encryption implementation over OpenSSL

  Copyright (c) 2023, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
*/

#include "InternalCryptLib.h"
#include <openssl/evp.h>
#include <openssl/pkcs7.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

#include <Library/MemoryAllocationLib.h>

/**
  Creates a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  that wraps content encrypted for secure transmission to one or more recipients.

  If this interface is not supported, return FALSE.

  @param[in]  X509Stack        Pointer to a stack of X.509 certificates for the
                               intended recipients of this message, created using
                               X509ConstructCertificateStack or similar. Each
                               certificate must provide an RSA public key. Any of the
                               corresponding private keys will be able to decrypt the
                               content of the returned ContentInfo.
  @param[in]  InData           Pointer to the content to be encrypted.
  @param[in]  InDataSize       Size of the content to be encrypted in bytes.
  @param[in]  CipherNid        NID of the symmetric cipher to use for encryption.
                               Supported values are CRYPTO_NID_AES128CBC,
                               CRYPTO_NID_AES192CBC, and CRYPTO_NID_AES256CBC.
  @param[in]  Flags            Flags for the encryption operation. Currently only
                               CRYPTO_PKCS7_DEFAULT is supported, which indicates that
                               the input data is treated as binary data.
  @param[out] ContentInfo      Receives a pointer to the output, which is a PKCS#7
                               DER-encoded ContentInfo that wraps an envelopedData. The
                               caller must free the returned buffer with FreePool().
  @param[out] ContentInfoSize  Receives the size of the output in bytes.

  @retval     TRUE             PKCS#7 data encryption succeeded.
  @retval     FALSE            PKCS#7 data encryption failed.
  @retval     FALSE            This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Encrypt (
  IN   UINT8   *X509Stack,
  IN   UINT8   *InData,
  IN   UINTN   InDataSize,
  IN   UINT32  CipherNid,
  IN   UINT32  Flags,
  OUT  UINT8   **ContentInfo,
  OUT  UINTN   *ContentInfoSize
  )
{
  BOOLEAN           Succeeded;
  EVP_CIPHER const  *pCipher;
  UINT8             *ReturnData = NULL;
  UINTN             ReturnSize  = 0;
  BIO               *pBio       = NULL;
  PKCS7             *pPkcs7     = NULL;
  int               I2dResult;

  if ((X509Stack == NULL) ||
      (InData == NULL) ||
      (InDataSize > INT_MAX) ||
      (Flags != CRYPTO_PKCS7_DEFAULT) ||
      (ContentInfo == NULL) ||
      (ContentInfoSize == NULL))
  {
    Succeeded = FALSE; // Invalid argument.
    goto Done;
  }

  switch (CipherNid) {
    case CRYPTO_NID_AES128CBC:
      pCipher = EVP_aes_128_cbc ();
      break;
    case CRYPTO_NID_AES192CBC:
      pCipher = EVP_aes_192_cbc ();
      break;
    case CRYPTO_NID_AES256CBC:
      pCipher = EVP_aes_256_cbc ();
      break;
    default:
      pCipher = NULL; // Unsupported cipher NID.
      break;
  }

  if (pCipher == NULL) {
    Succeeded = FALSE; // Unsupported cipher NID.
    goto Done;
  }

  pBio = BIO_new_mem_buf (InData, (int)InDataSize);
  if (pBio == NULL) {
    Succeeded = FALSE; // BIO creation failed.
    goto Done;
  }

  // Create a new PKCS#7 structure
  pPkcs7 = PKCS7_encrypt ((struct stack_st_X509 *)X509Stack, pBio, pCipher, PKCS7_BINARY);
  if (pPkcs7 == NULL) {
    Succeeded = FALSE; // PKCS7 creation failed.
    goto Done;
  }

  I2dResult = i2d_PKCS7 (pPkcs7, NULL);
  if (I2dResult <= 0) {
    Succeeded = FALSE; // DER encoding failed.
    goto Done;
  }

  ReturnSize = (UINTN)I2dResult;
  ReturnData = (UINT8 *)AllocateZeroPool (ReturnSize);

  if (ReturnData == NULL) {
    ReturnSize = 0;
    Succeeded  = FALSE; // Memory allocation failed.
    goto Done;
  }

  UINT8  *pPkcs7Data = ReturnData;

  I2dResult = i2d_PKCS7 (pPkcs7, &pPkcs7Data);
  if ((I2dResult <= 0) || (ReturnSize != (UINTN)I2dResult)) {
    FreePool (ReturnData);
    ReturnData = NULL;
    ReturnSize = 0;
    Succeeded  = FALSE; // DER encoding returned an unexpected result.
    goto Done;
  }

  Succeeded = TRUE; // Success, result in ReturnData/ReturnSize.

Done:

  if (pPkcs7) {
    PKCS7_free (pPkcs7);
    pPkcs7 = NULL;
  }

  if (pBio) {
    BIO_free (pBio);
    pBio = NULL;
  }

  if (ContentInfo != NULL) {
    *ContentInfo = ReturnData;
  }

  if (ContentInfoSize != NULL) {
    *ContentInfoSize = ReturnSize;
  }

  return Succeeded;
}
