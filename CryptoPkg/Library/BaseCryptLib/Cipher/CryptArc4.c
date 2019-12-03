/** @file
  ARC4 Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/rc4.h>

/**
  Retrieves the size, in bytes, of the context buffer required for ARC4 operations.

  @return  The size, in bytes, of the context buffer required for ARC4 operations.

**/
UINTN
EFIAPI
Arc4GetContextSize (
  VOID
  )
{
  //
  // Memory for 2 copies of RC4_KEY is allocated, one for working copy, and the other
  // for backup copy. When Arc4Reset() is called, we can use the backup copy to restore
  // the working copy to the initial state.
  //
  return (UINTN) (2 * sizeof (RC4_KEY));
}

/**
  Initializes user-supplied memory as ARC4 context for subsequent use.

  This function initializes user-supplied memory pointed by Arc4Context as ARC4 context.
  In addition, it sets up all ARC4 key materials for subsequent encryption and decryption
  operations.

  If Arc4Context is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize does not in the range of [5, 256] bytes, then return FALSE.

  @param[out]  Arc4Context  Pointer to ARC4 context being initialized.
  @param[in]   Key          Pointer to the user-supplied ARC4 key.
  @param[in]   KeySize      Size of ARC4 key in bytes.

  @retval TRUE   ARC4 context initialization succeeded.
  @retval FALSE  ARC4 context initialization failed.

**/
BOOLEAN
EFIAPI
Arc4Init (
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  RC4_KEY  *Rc4Key;

  //
  // Check input parameters.
  //
  if (Arc4Context == NULL || Key == NULL || (KeySize < 5 || KeySize > 256)) {
    return FALSE;
  }

  Rc4Key = (RC4_KEY *) Arc4Context;

  RC4_set_key (Rc4Key, (UINT32) KeySize, Key);

  CopyMem (Rc4Key +  1, Rc4Key, sizeof (RC4_KEY));

  return TRUE;
}

/**
  Performs ARC4 encryption on a data buffer of the specified size.

  This function performs ARC4 encryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 encryption output.

  @retval TRUE   ARC4 encryption succeeded.
  @retval FALSE  ARC4 encryption failed.

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
  RC4_KEY  *Rc4Key;

  //
  // Check input parameters.
  //
  if (Arc4Context == NULL || Input == NULL || Output == NULL || InputSize > INT_MAX) {
    return FALSE;
  }

  Rc4Key = (RC4_KEY *) Arc4Context;

  RC4 (Rc4Key, (UINT32) InputSize, Input, Output);

  return TRUE;
}

/**
  Performs ARC4 decryption on a data buffer of the specified size.

  This function performs ARC4 decryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 decryption output.

  @retval TRUE   ARC4 decryption succeeded.
  @retval FALSE  ARC4 decryption failed.

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
  RC4_KEY  *Rc4Key;

  //
  // Check input parameters.
  //
  if (Arc4Context == NULL || Input == NULL || Output == NULL || InputSize > INT_MAX) {
    return FALSE;
  }

  Rc4Key = (RC4_KEY *) Arc4Context;

  RC4 (Rc4Key, (UINT32) InputSize, Input, Output);

  return TRUE;
}

/**
  Resets the ARC4 context to the initial state.

  The function resets the ARC4 context to the state it had immediately after the
  ARC4Init() function call.
  Contrary to ARC4Init(), Arc4Reset() requires no secret key as input, but ARC4 context
  should be already correctly initialized by ARC4Init().

  If Arc4Context is NULL, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.

  @retval TRUE   ARC4 reset succeeded.
  @retval FALSE  ARC4 reset failed.

**/
BOOLEAN
EFIAPI
Arc4Reset (
  IN OUT  VOID  *Arc4Context
  )
{
  RC4_KEY  *Rc4Key;

  //
  // Check input parameters.
  //
  if (Arc4Context == NULL) {
    return FALSE;
  }

  Rc4Key = (RC4_KEY *) Arc4Context;

  CopyMem (Rc4Key, Rc4Key + 1, sizeof (RC4_KEY));

  return TRUE;
}
