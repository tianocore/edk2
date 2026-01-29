/** @file
  AES Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/aes.h>

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  @return  The size, in bytes, of the context buffer required for AES operations.

**/
UINTN
EFIAPI
AesGetContextSize (
  VOID
  )
{
  //
  // AES uses different key contexts for encryption and decryption, so here memory
  // for 2 copies of AES_KEY is allocated.
  //
  return (UINTN)(2 * sizeof (AES_KEY));
}

/**
  Initializes user-supplied memory as AES context for subsequent use.

  This function initializes user-supplied memory pointed by AesContext as AES context.
  In addition, it sets up all AES key materials for subsequent encryption and decryption
  operations.
  There are 3 options for key length, 128 bits, 192 bits, and 256 bits.

  If AesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.

  @param[out]  AesContext  Pointer to AES context being initialized.
  @param[in]   Key         Pointer to the user-supplied AES key.
  @param[in]   KeyLength   Length of AES key in bits.

  @retval TRUE   AES context initialization succeeded.
  @retval FALSE  AES context initialization failed.

**/
BOOLEAN
EFIAPI
AesInit (
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  AES_KEY  *AesKey;

  //
  // Check input parameters.
  //
  if ((AesContext == NULL) || (Key == NULL) || ((KeyLength != 128) && (KeyLength != 192) && (KeyLength != 256))) {
    return FALSE;
  }

  //
  // Initialize AES encryption & decryption key schedule.
  //
  AesKey = (AES_KEY *)AesContext;
  if (AES_set_encrypt_key (Key, (UINT32)KeyLength, AesKey) != 0) {
    return FALSE;
  }

  if (AES_set_decrypt_key (Key, (UINT32)KeyLength, AesKey + 1) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Performs AES encryption on a data buffer of the specified size in CBC mode.

  This function performs AES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.

**/
BOOLEAN
EFIAPI
AesCbcEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  AES_KEY  *AesKey;
  UINT8    IvecBuffer[AES_BLOCK_SIZE];

  //
  // Check input parameters.
  //
  if ((AesContext == NULL) || (Input == NULL) || ((InputSize % AES_BLOCK_SIZE) != 0)) {
    return FALSE;
  }

  if ((Ivec == NULL) || (Output == NULL) || (InputSize > INT_MAX)) {
    return FALSE;
  }

  AesKey = (AES_KEY *)AesContext;
  CopyMem (IvecBuffer, Ivec, AES_BLOCK_SIZE);

  //
  // Perform AES data encryption with CBC mode
  //
  AES_cbc_encrypt (Input, Output, (UINT32)InputSize, AesKey, IvecBuffer, AES_ENCRYPT);

  return TRUE;
}

/**
  Performs AES decryption on a data buffer of the specified size in CBC mode.

  This function performs AES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.

**/
BOOLEAN
EFIAPI
AesCbcDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  AES_KEY  *AesKey;
  UINT8    IvecBuffer[AES_BLOCK_SIZE];

  //
  // Check input parameters.
  //
  if ((AesContext == NULL) || (Input == NULL) || ((InputSize % AES_BLOCK_SIZE) != 0)) {
    return FALSE;
  }

  if ((Ivec == NULL) || (Output == NULL) || (InputSize > INT_MAX)) {
    return FALSE;
  }

  AesKey = (AES_KEY *)AesContext;
  CopyMem (IvecBuffer, Ivec, AES_BLOCK_SIZE);

  //
  // Perform AES data decryption with CBC mode
  //
  AES_cbc_encrypt (Input, Output, (UINT32)InputSize, AesKey + 1, IvecBuffer, AES_DECRYPT);

  return TRUE;
}
