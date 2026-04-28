/** @file
  AES CTR Encryption Wrapper Implementation over OpenSSL.

(c) Copyright 2026 HP Development Company, L.P.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/aes.h>
#include <openssl/evp.h>

/**
  Performs AES encryption in CTR (Counter) mode on a data buffer.

  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  IvSize must be 16, otherwise FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV/nonce value (counter block).
  @param[in]   IvSize      Size of the IV/nonce value in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be encrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the encryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AES-CTR encryption succeeded.
  @retval FALSE  AES-CTR encryption failed.

**/
BOOLEAN
EFIAPI
AesCtrEncrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  EVP_CIPHER_CTX    *Ctx;
  CONST EVP_CIPHER  *Cipher;
  UINTN             TempOutSize;
  UINTN             FinalOutSize;
  BOOLEAN           RetValue;

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != AES_BLOCK_SIZE) {
    return FALSE;
  }

  TempOutSize  = 0;
  FinalOutSize = 0;

  switch (KeySize) {
    case 16:
      Cipher = EVP_aes_128_ctr ();
      break;
    case 24:
      Cipher = EVP_aes_192_ctr ();
      break;
    case 32:
      Cipher = EVP_aes_256_ctr ();
      break;
    default:
      return FALSE;
  }

  if (DataOutSize != NULL) {
    if ((*DataOutSize > INT_MAX) || (*DataOutSize < DataInSize)) {
      return FALSE;
    }
  }

  Ctx = EVP_CIPHER_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetValue = (BOOLEAN)EVP_EncryptInit_ex (Ctx, Cipher, NULL, Key, Iv);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptUpdate (Ctx, DataOut, (INT32 *)&TempOutSize, DataIn, (INT32)DataInSize);
  if (!RetValue) {
    goto Done;
  }

  FinalOutSize = TempOutSize;
  RetValue     = (BOOLEAN)EVP_EncryptFinal_ex (Ctx, DataOut + TempOutSize, (INT32 *)&TempOutSize);
  if (RetValue) {
    FinalOutSize += TempOutSize;
  }

Done:
  EVP_CIPHER_CTX_free (Ctx);
  if (!RetValue) {
    return RetValue;
  }

  if (DataOutSize != NULL) {
    *DataOutSize = FinalOutSize;
  }

  return RetValue;
}

/**
  Performs AES decryption in CTR (Counter) mode on a data buffer.

  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  IvSize must be 16, otherwise FALSE is returned.

  @param[in]   Key         Pointer to the decryption key.
  @param[in]   KeySize     Size of the decryption key in bytes.
  @param[in]   Iv          Pointer to the IV/nonce value (counter block).
  @param[in]   IvSize      Size of the IV/nonce value in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be decrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the decryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AES-CTR decryption succeeded.
  @retval FALSE  AES-CTR decryption failed.

**/
BOOLEAN
EFIAPI
AesCtrDecrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  EVP_CIPHER_CTX    *Ctx;
  CONST EVP_CIPHER  *Cipher;
  UINTN             TempOutSize;
  UINTN             FinalOutSize;
  BOOLEAN           RetValue;

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != AES_BLOCK_SIZE) {
    return FALSE;
  }

  TempOutSize  = 0;
  FinalOutSize = 0;

  switch (KeySize) {
    case 16:
      Cipher = EVP_aes_128_ctr ();
      break;
    case 24:
      Cipher = EVP_aes_192_ctr ();
      break;
    case 32:
      Cipher = EVP_aes_256_ctr ();
      break;
    default:
      return FALSE;
  }

  if (DataOutSize != NULL) {
    if ((*DataOutSize > INT_MAX) || (*DataOutSize < DataInSize)) {
      return FALSE;
    }
  }

  Ctx = EVP_CIPHER_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetValue = (BOOLEAN)EVP_EncryptInit_ex (Ctx, Cipher, NULL, Key, Iv);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptUpdate (Ctx, DataOut, (INT32 *)&TempOutSize, DataIn, (INT32)DataInSize);
  if (!RetValue) {
    goto Done;
  }

  FinalOutSize = TempOutSize;
  RetValue     = (BOOLEAN)EVP_EncryptFinal_ex (Ctx, DataOut + TempOutSize, (INT32 *)&TempOutSize);
  if (RetValue) {
    FinalOutSize += TempOutSize;
  }

Done:
  EVP_CIPHER_CTX_free (Ctx);
  if (!RetValue) {
    return RetValue;
  }

  if (DataOutSize != NULL) {
    *DataOutSize = FinalOutSize;
  }

  return RetValue;
}
