/** @file
  PEM (Privacy Enhanced Mail) Format Handler Wrapper Implementation
  for EC Public Key functions.

Copyright (c) 2023 - 2026, ARM Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Retrieve the EC Public Key from PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which contain the retrieved
                           EC public key component. Use EcFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval  TRUE   EC Public Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
EcGetPublicKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **EcContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Convert the EC Public Key to PEM key data.

  @param[in]  EcContext    Pointer to EC DSA context.
  @param[out] PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[out] PemSize      Size of the PEM key data in bytes.

  If EcContext is NULL, then return FALSE.
  If PemSize is NULL, then return FALSE.
  If PemData is NULL and PemSize is not zero, then return FALSE and
  set the PemSize to the required size of the PemData buffer.

  @retval  TRUE   EC Public Key was converted to the PEM data successfully.
  @retval  FALSE  Invalid EC Context.

**/
BOOLEAN
EFIAPI
EcPublicKeyToPEM (
  IN  VOID   *EcContext,
  OUT UINT8  *PemData,
  OUT UINTN  *PemSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
