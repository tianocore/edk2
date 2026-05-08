/** @file
  AES CTR Wrapper Implementation which does not provide real capabilities.

(c) Copyright 2026 HP Development Company, L.P.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

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
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
}
