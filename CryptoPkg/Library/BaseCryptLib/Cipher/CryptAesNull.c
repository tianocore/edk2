/** @file
  AES Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
AesGetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory as AES context for subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  AesContext  Pointer to AES context being initialized.
  @param[in]   Key         Pointer to the user-supplied AES key.
  @param[in]   KeyLength   Length of AES key in bits.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesInit (
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs AES encryption on a data buffer of the specified size in CBC mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs AES decryption on a data buffer of the specified size in CBC mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs AES encryption on single block (AES_BLOCK_SIZE)

  This function performs AES encryption on single block pointed by Input.

  Caller must perform padding, if necessary, to ensure single block size.
  AesContext should be already correctly initialized by AesInit().
  Behavior with invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing single block data
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.

**/
BOOLEAN
EFIAPI
AesEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  OUT  UINT8        *Output
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs AES decryption on single block (AES_BLOCK_SIZE)

  This function performs AES decryption on single block pointed by Input.

  Caller must perform padding, if necessary, to ensure single block size.
  AesContext should be already correctly initialized by AesInit().
  Behavior with invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing single block encrpyted data.
  @param[out]  Output      Pointer to a buffer that receives the AES decryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.

**/
BOOLEAN
EFIAPI
AesDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  OUT  UINT8        *Output
  )
{
  ASSERT (FALSE);
  return FALSE;
}
