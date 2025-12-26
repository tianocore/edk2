/** @file
  CAMELLIA Wrapper Implementation which does not provide real capabilities..

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Retrieves the size, in bytes, of the context buffer required for CAMELLIA operations.

  @return  The size, in bytes, of the context buffer required for CAMELLIA operations.

**/
UINTN
EFIAPI
CamelliaGetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory as Camellia context for subsequent use.

  This function initializes user-supplied memory pointed by CamelliaContext as
  CAMELLIA context.
  In addition, it sets up all CAMELLIA key materials for subsequent
  encryption and decryption operations.
  There are 3 options for key length, 128 bits, 192 bits, and 256 bits.

  If CamelliaContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.

  @param[out]  CamelliaContext  Pointer to CAMELLIA context being initialized.
  @param[in]   Key              Pointer to the user-supplied CAMELLIA key.
  @param[in]   KeyLength        Length of CAMELLIA key in bits.

  @retval TRUE   CAMELLIA context initialization succeeded.
  @retval FALSE  CAMELLIA context initialization failed.

**/
BOOLEAN
EFIAPI
CamelliaInit (
  OUT  VOID         *CamelliaContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs CAMELLIA encryption on single block (128 bits)

  This function performs CAMELLIA encryption on single block pointed by Input.

  Caller must perform padding, if necessary, to ensure single block size.
  CamelliaContext should be already correctly initialized by CamelliaInit().
  Behavior with invalid Camellia context is undefined.

  If CamelliaContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   CamelliaContext  Pointer to the CAMELLIA context.
  @param[in]   Input            Pointer to the buffer containing single block data
  @param[out]  Output           Pointer to a buffer that receives the CAMELLIA encryption output.

  @retval TRUE   CAMELLIA encryption succeeded.
  @retval FALSE  CAMELLIA encryption failed.

**/
BOOLEAN
EFIAPI
CamelliaEncrypt (
  IN   VOID         *CamelliaContext,
  IN   CONST UINT8  *Input,
  OUT  UINT8        *Output
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs CAMELLIA decryption on single block (128 bits)

  This function performs CAMELLIA decryption on single block pointed by Input.

  Caller must perform padding, if necessary, to ensure single block size.
  CamelliaContext should be already correctly initialized by CamelliaInit().
  Behavior with invalid CAMELLIA context is undefined.

  If CamelliaContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   CamelliaContext  Pointer to the CAMELLIA context.
  @param[in]   Input            Pointer to the buffer containing single block encrpyted data.
  @param[out]  Output           Pointer to a buffer that receives the CAMELLIA decryption output.

  @retval TRUE   CAMELLIA decryption succeeded.
  @retval FALSE  CAMELLIA decryption failed.

**/
BOOLEAN
EFIAPI
CamelliaDecrypt (
  IN   VOID         *CamelliaContext,
  IN   CONST UINT8  *Input,
  OUT  UINT8        *Output
  )
{
  ASSERT (FALSE);
  return FALSE;
}
