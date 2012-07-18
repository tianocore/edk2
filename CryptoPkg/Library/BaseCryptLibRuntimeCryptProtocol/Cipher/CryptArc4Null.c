/** @file 
  ARC4 Wrapper Implementation which does not provide real capabilities.

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
  Retrieves the size, in bytes, of the context buffer required for ARC4 operations.

  Return zero to indicate this interface is not supported.

  @retval  0   This interface is not supported.


**/
UINTN
EFIAPI
Arc4GetContextSize (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Initializes user-supplied memory as ARC4 context for subsequent use.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Arc4Context  Pointer to ARC4 context being initialized.
  @param[in]   Key          Pointer to the user-supplied ARC4 key.
  @param[in]   KeySize      Size of ARC4 key in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Init (
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs ARC4 encryption on a data buffer of the specified size.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Encrypt (
  IN OUT  VOID         *Arc4Context,
  IN      CONST UINT8  *Input,
  IN      UINTN        InputSize,
  OUT     UINT8        *Output
  )
{ 
  ASSERT (FALSE);
  return FALSE;
}

/**
  Performs ARC4 decryption on a data buffer of the specified size.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 decryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Decrypt (
  IN OUT  VOID   *Arc4Context,
  IN      UINT8  *Input,
  IN      UINTN  InputSize,
  OUT     UINT8  *Output
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Resets the ARC4 context to the initial state.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Reset (
  IN OUT  VOID  *Arc4Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}
