/** @file
  TDES Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

/**
  Retrieves the size, in bytes, of the context buffer required for TDES operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
TdesGetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory as TDES context for subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  TdesContext  Pointer to TDES context being initialized.
  @param[in]   Key          Pointer to the user-supplied TDES key.
  @param[in]   KeyLength    Length of TDES key in bits.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesInit (
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs TDES encryption on a data buffer of the specified size in ECB mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs TDES decryption on a data buffer of the specified size in ECB mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES decryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs TDES encryption on a data buffer of the specified size in CBC mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs TDES decryption on a data buffer of the specified size in CBC mode.

  Return FALSE to indicate this interface is not supported.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

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
  ASSERT (FALSE);
  return FALSE;
}

