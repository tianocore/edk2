/** @file
  AES CTR Encryption Wrapper Implementation over MbedTLS.

(c) Copyright 2026 HP Development Company, L.P.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/cipher.h>

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
  mbedtls_cipher_context_t     Ctx;
  CONST mbedtls_cipher_info_t  *CipherInfo;
  INT32                        Ret;
  UINT8                        IvBuffer[AES_BLOCK_SIZE];

  // Parameter validation
  if ((Key == NULL) || (Iv == NULL) || (DataIn == NULL) || (DataOut == NULL)) {
    return FALSE;
  }

  if (DataOutSize == NULL) {
    return FALSE;
  }

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != AES_BLOCK_SIZE) {
    return FALSE;
  }

  if (*DataOutSize < DataInSize) {
    return FALSE;
  }

  // Select cipher based on key size
  switch (KeySize) {
    case 16:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_128_CTR);
      break;
    case 24:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_192_CTR);
      break;
    case 32:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_256_CTR);
      break;
    default:
      return FALSE;
  }

  if (CipherInfo == NULL) {
    return FALSE;
  }

  // Initialize and setup cipher
  mbedtls_cipher_init (&Ctx);

  Ret = mbedtls_cipher_setup (&Ctx, CipherInfo);
  if (Ret != 0) {
    mbedtls_cipher_free (&Ctx);
    return FALSE;
  }

  // Set encryption key
  Ret = mbedtls_cipher_setkey (&Ctx, Key, (INT32)(KeySize * 8), MBEDTLS_ENCRYPT);
  if (Ret != 0) {
    mbedtls_cipher_free (&Ctx);
    return FALSE;
  }

  // Copy IV to working buffer
  CopyMem (IvBuffer, Iv, AES_BLOCK_SIZE);

  // Perform encryption
  Ret = mbedtls_cipher_crypt (
          &Ctx,
          IvBuffer,
          AES_BLOCK_SIZE,
          DataIn,
          DataInSize,
          DataOut,
          DataOutSize
          );

  mbedtls_cipher_free (&Ctx);

  if (Ret != 0) {
    return FALSE;
  }

  *DataOutSize = DataInSize;
  return TRUE;
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
  mbedtls_cipher_context_t     Ctx;
  CONST mbedtls_cipher_info_t  *CipherInfo;
  INT32                        Ret;
  UINT8                        IvBuffer[AES_BLOCK_SIZE];

  // Parameter validation
  if ((Key == NULL) || (Iv == NULL) || (DataIn == NULL) || (DataOut == NULL)) {
    return FALSE;
  }

  if (DataOutSize == NULL) {
    return FALSE;
  }

  if (DataInSize > INT_MAX) {
    return FALSE;
  }

  if (IvSize != AES_BLOCK_SIZE) {
    return FALSE;
  }

  if (*DataOutSize < DataInSize) {
    return FALSE;
  }

  // Select cipher based on key size
  switch (KeySize) {
    case 16:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_128_CTR);
      break;
    case 24:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_192_CTR);
      break;
    case 32:
      CipherInfo = mbedtls_cipher_info_from_type (MBEDTLS_CIPHER_AES_256_CTR);
      break;
    default:
      return FALSE;
  }

  if (CipherInfo == NULL) {
    return FALSE;
  }

  // Initialize and setup cipher
  mbedtls_cipher_init (&Ctx);

  Ret = mbedtls_cipher_setup (&Ctx, CipherInfo);
  if (Ret != 0) {
    mbedtls_cipher_free (&Ctx);
    return FALSE;
  }

  // Set decryption key (CTR uses same key for both encrypt and decrypt)
  Ret = mbedtls_cipher_setkey (&Ctx, Key, (INT32)(KeySize * 8), MBEDTLS_DECRYPT);
  if (Ret != 0) {
    mbedtls_cipher_free (&Ctx);
    return FALSE;
  }

  // Copy IV to working buffer
  CopyMem (IvBuffer, Iv, AES_BLOCK_SIZE);

  // Perform decryption
  Ret = mbedtls_cipher_crypt (
          &Ctx,
          IvBuffer,
          AES_BLOCK_SIZE,
          DataIn,
          DataInSize,
          DataOut,
          DataOutSize
          );
  mbedtls_cipher_free (&Ctx);

  if (Ret != 0) {
    return FALSE;
  }

  *DataOutSize = DataInSize;
  return TRUE;
}
