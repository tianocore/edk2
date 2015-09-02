/** @file
  TDES Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/des.h>

/**
  Retrieves the size, in bytes, of the context buffer required for TDES operations.

  @return  The size, in bytes, of the context buffer required for TDES operations.

**/
UINTN
EFIAPI
TdesGetContextSize (
  VOID
  )
{
  //
  // Memory for 3 copies of DES_key_schedule is allocated, for K1, K2 and K3 each.
  //
  return (UINTN) (3 * sizeof (DES_key_schedule));
}

/**
  Initializes user-supplied memory as TDES context for subsequent use.

  This function initializes user-supplied memory pointed by TdesContext as TDES context.
  In addition, it sets up all TDES key materials for subsequent encryption and decryption
  operations.
  There are 3 key options as follows:
  KeyLength = 64,  Keying option 1: K1 == K2 == K3 (Backward compatibility with DES)
  KeyLength = 128, Keying option 2: K1 != K2 and K3 = K1 (Less Security)
  KeyLength = 192  Keying option 3: K1 != K2 != K3 (Strongest)

  If TdesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.

  @param[out]  TdesContext  Pointer to TDES context being initialized.
  @param[in]   Key          Pointer to the user-supplied TDES key.
  @param[in]   KeyLength    Length of TDES key in bits.

  @retval TRUE   TDES context initialization succeeded.
  @retval FALSE  TDES context initialization failed.

**/
BOOLEAN
EFIAPI
TdesInit (
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  DES_key_schedule  *KeySchedule;

  //
  // Check input parameters.
  //
  if (TdesContext == NULL || Key == NULL || (KeyLength != 64 && KeyLength != 128 && KeyLength != 192)) {
    return FALSE;
  }

  KeySchedule = (DES_key_schedule *) TdesContext;

  //
  // If input Key is a weak key, return error.
  //
  if (DES_is_weak_key ((const_DES_cblock *) Key) == 1) {
    return FALSE;
  }

  DES_set_key_unchecked ((const_DES_cblock *) Key, KeySchedule);

  if (KeyLength == 64) {
    CopyMem (KeySchedule + 1, KeySchedule, sizeof (DES_key_schedule));
    CopyMem (KeySchedule + 2, KeySchedule, sizeof (DES_key_schedule));
    return TRUE;
  }

  if (DES_is_weak_key ((const_DES_cblock *) (Key + 8)) == 1) {
    return FALSE;
  }

  DES_set_key_unchecked ((const_DES_cblock *) (Key + 8), KeySchedule + 1);

  if (KeyLength == 128) {
    CopyMem (KeySchedule + 2, KeySchedule, sizeof (DES_key_schedule));
    return TRUE;
  }

  if (DES_is_weak_key ((const_DES_cblock *) (Key + 16)) == 1) {
    return FALSE;
  }

  DES_set_key_unchecked ((const_DES_cblock *) (Key + 16), KeySchedule + 2);

  return TRUE;
}

/**
  Performs TDES encryption on a data buffer of the specified size in ECB mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.

**/
BOOLEAN
EFIAPI
TdesEcbEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  DES_key_schedule  *KeySchedule;

  //
  // Check input parameters.
  //
  if (TdesContext == NULL || Input == NULL || (InputSize % TDES_BLOCK_SIZE) != 0 || Output == NULL) {
    return FALSE;
  }

  KeySchedule = (DES_key_schedule *) TdesContext;

  while (InputSize > 0) {
    DES_ecb3_encrypt (
      (const_DES_cblock *) Input,
      (DES_cblock *) Output,
      KeySchedule,
      KeySchedule + 1,
      KeySchedule + 2,
      DES_ENCRYPT
      );
    Input     += TDES_BLOCK_SIZE;
    Output    += TDES_BLOCK_SIZE;
    InputSize -= TDES_BLOCK_SIZE;
  }

  return TRUE;
}

/**
  Performs TDES decryption on a data buffer of the specified size in ECB mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES decryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.

**/
BOOLEAN
EFIAPI
TdesEcbDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  DES_key_schedule  *KeySchedule;

  //
  // Check input parameters.
  //
  if (TdesContext == NULL || Input == NULL || (InputSize % TDES_BLOCK_SIZE) != 0 || Output == NULL) {
    return FALSE;
  }

  KeySchedule = (DES_key_schedule *) TdesContext;

  while (InputSize > 0) {
    DES_ecb3_encrypt (
      (const_DES_cblock *) Input,
      (DES_cblock *) Output,
      KeySchedule,
      KeySchedule + 1,
      KeySchedule + 2,
      DES_DECRYPT
      );
    Input     += TDES_BLOCK_SIZE;
    Output    += TDES_BLOCK_SIZE;
    InputSize -= TDES_BLOCK_SIZE;
  }

  return TRUE;
}

/**
  Performs TDES encryption on a data buffer of the specified size in CBC mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.

**/
BOOLEAN
EFIAPI
TdesCbcEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  DES_key_schedule  *KeySchedule;
  UINT8             IvecBuffer[TDES_BLOCK_SIZE];

  //
  // Check input parameters.
  //
  if (TdesContext == NULL || Input == NULL || (InputSize % TDES_BLOCK_SIZE) != 0) {
    return FALSE;
  }

  if (Ivec == NULL || Output == NULL || InputSize > INT_MAX) {
    return FALSE;
  }

  KeySchedule = (DES_key_schedule *) TdesContext;
  CopyMem (IvecBuffer, Ivec, TDES_BLOCK_SIZE);

  DES_ede3_cbc_encrypt (
    Input,
    Output,
    (UINT32) InputSize,
    KeySchedule,
    KeySchedule + 1,
    KeySchedule + 2,
    (DES_cblock *) IvecBuffer,
    DES_ENCRYPT
    );

  return TRUE;
}

/**
  Performs TDES decryption on a data buffer of the specified size in CBC mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.

**/
BOOLEAN
EFIAPI
TdesCbcDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  DES_key_schedule  *KeySchedule;
  UINT8             IvecBuffer[TDES_BLOCK_SIZE];

  //
  // Check input parameters.
  //
  if (TdesContext == NULL || Input == NULL || (InputSize % TDES_BLOCK_SIZE) != 0) {
    return FALSE;
  }

  if (Ivec == NULL || Output == NULL || InputSize > INT_MAX) {
    return FALSE;
  }

  KeySchedule = (DES_key_schedule *) TdesContext;
  CopyMem (IvecBuffer, Ivec, TDES_BLOCK_SIZE);

  DES_ede3_cbc_encrypt (
    Input,
    Output,
    (UINT32) InputSize,
    KeySchedule,
    KeySchedule + 1,
    KeySchedule + 2,
    (DES_cblock *) IvecBuffer,
    DES_DECRYPT
    );

  return TRUE;
}

