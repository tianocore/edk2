/** @file
  AEAD (AES-GCM) Wrapper Implementation over OpenSSL.

  RFC 5116 - An Interface and Algorithms for Authenticated Encryption
  NIST SP800-38d - Cipher Modes of Operation: Galois / Counter Mode(GCM) and GMAC

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/aes.h>
#include <openssl/evp.h>

/**
  Performs AEAD AES-GCM authenticated encryption on a data buffer and additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be encrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[out]  TagOut      Pointer to a buffer that receives the authentication tag output.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the encryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated encryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated encryption failed.

**/
BOOLEAN
EFIAPI
AeadAesGcmEncrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  OUT  UINT8        *TagOut,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  EVP_CIPHER_CTX    *Ctx;
  CONST EVP_CIPHER  *Cipher;
  UINTN             TempOutSize;
  BOOLEAN           RetValue;

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (ADataSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != 12) {
    return FALSE;
  }

  switch (KeySize) {
    case 16:
      Cipher = EVP_aes_128_gcm ();
      break;
    case 24:
      Cipher = EVP_aes_192_gcm ();
      break;
    case 32:
      Cipher = EVP_aes_256_gcm ();
      break;
    default:
      return FALSE;
  }

  if ((TagSize != 12) && (TagSize != 13) && (TagSize != 14) && (TagSize != 15) && (TagSize != 16)) {
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

  RetValue = (BOOLEAN)EVP_EncryptInit_ex (Ctx, Cipher, NULL, NULL, NULL);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_CIPHER_CTX_ctrl (Ctx, EVP_CTRL_GCM_SET_IVLEN, (INT32)IvSize, NULL);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptInit_ex (Ctx, NULL, NULL, Key, Iv);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptUpdate (Ctx, NULL, (INT32 *)&TempOutSize, AData, (INT32)ADataSize);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptUpdate (Ctx, DataOut, (INT32 *)&TempOutSize, DataIn, (INT32)DataInSize);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_EncryptFinal_ex (Ctx, DataOut, (INT32 *)&TempOutSize);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_CIPHER_CTX_ctrl (Ctx, EVP_CTRL_GCM_GET_TAG, (INT32)TagSize, (VOID *)TagOut);

Done:
  EVP_CIPHER_CTX_free (Ctx);
  if (!RetValue) {
    return RetValue;
  }

  if (DataOutSize != NULL) {
    *DataOutSize = DataInSize;
  }

  return RetValue;
}

/**
  Performs AEAD AES-GCM authenticated decryption on a data buffer and additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.
  If additional authenticated data verification fails, FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be decrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[in]   Tag         Pointer to a buffer that contains the authentication tag.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the decryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated decryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated decryption failed.

**/
BOOLEAN
EFIAPI
AeadAesGcmDecrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  IN   CONST UINT8  *Tag,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  EVP_CIPHER_CTX    *Ctx;
  CONST EVP_CIPHER  *Cipher;
  UINTN             TempOutSize;
  BOOLEAN           RetValue;

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (ADataSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != 12) {
    return FALSE;
  }

  switch (KeySize) {
    case 16:
      Cipher = EVP_aes_128_gcm ();
      break;
    case 24:
      Cipher = EVP_aes_192_gcm ();
      break;
    case 32:
      Cipher = EVP_aes_256_gcm ();
      break;
    default:
      return FALSE;
  }

  if ((TagSize != 12) && (TagSize != 13) && (TagSize != 14) && (TagSize != 15) && (TagSize != 16)) {
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

  RetValue = (BOOLEAN)EVP_DecryptInit_ex (Ctx, Cipher, NULL, NULL, NULL);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_CIPHER_CTX_ctrl (Ctx, EVP_CTRL_GCM_SET_IVLEN, (INT32)IvSize, NULL);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_DecryptInit_ex (Ctx, NULL, NULL, Key, Iv);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_DecryptUpdate (Ctx, NULL, (INT32 *)&TempOutSize, AData, (INT32)ADataSize);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_DecryptUpdate (Ctx, DataOut, (INT32 *)&TempOutSize, DataIn, (INT32)DataInSize);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_CIPHER_CTX_ctrl (Ctx, EVP_CTRL_GCM_SET_TAG, (INT32)TagSize, (VOID *)Tag);
  if (!RetValue) {
    goto Done;
  }

  RetValue = (BOOLEAN)EVP_DecryptFinal_ex (Ctx, DataOut, (INT32 *)&TempOutSize);

Done:
  EVP_CIPHER_CTX_free (Ctx);
  if (!RetValue) {
    return RetValue;
  }

  if (DataOutSize != NULL) {
    *DataOutSize = DataInSize;
  }

  return RetValue;
}
