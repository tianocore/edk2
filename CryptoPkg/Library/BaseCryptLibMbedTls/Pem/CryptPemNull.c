/** @file
  PEM (Privacy Enhanced Mail) Format Handler Wrapper Implementation which does
  not provide real capabilities.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Retrieve the RSA Private Key from the password-protected PEM key data.

  Return FALSE to indicate this interface is not supported.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA private key component. Use RsaFree() function to free the
                           resource.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **RsaContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieve the EC Private Key from the password-protected PEM key data.

  Return FALSE to indicate this interface is not supported.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] EcContext    Pointer to new-generated EC context which contain the retrieved
                           EC private key component. Use EcFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
EcGetPrivateKeyFromPem (
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
   Retrieve the EdDSA Private Key from the password-protected PEM key data.

   @param[in]  PemData        Pointer to the PEM-encoded key data to be retrieved.
   @param[in]  PemSize        Size of the PEM key data in bytes.
   @param[in]  Password       NULL-terminated passphrase used for encrypted PEM key data.
   @param[out] EdDsaContext   Pointer to new-generated EdDSA context which contains the retrieved
   EdDSA private key component. Use EdDsaFree() function to free the
   resource.

   If PemData is NULL, then return FALSE.
   If EdDsaContext is NULL, then return FALSE.

   @retval  TRUE   EdDSA Private Key was retrieved successfully.
   @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
EdDsaGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **EdDsaContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieve the ML-DSA Private Key from the password-protected PEM key data.

  If PemData is NULL, then return FALSE.
  If MlDsaContext is NULL, then return FALSE.

  @param[in]  PemData       Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize       Size of the PEM key data in bytes.
  @param[in]  Password      NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] MlDsaContext  Pointer to new-generated ML-DSA context which contains
                            the retrieved ML-DSA private key. Use MlDsaFree() to free.

  @retval  TRUE   ML-DSA Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
MlDsaGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **MlDsaContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieve the EC Public Key from PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted
                           PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which
                           contain the retrieved EC public key component.
                           Use EcFree() function to free the resource.

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

  @param[in]      EcContext   Pointer to EC DSA context.
  @param[out]     PemData     Pointer to the PEM-encoded key data to be
                              retrieved.
  @param[in, out] PemSize     On input, size of PemData in bytes.
                              On output, size of data returned or required size.

  If EcContext is NULL, then return FALSE.
  If PemSize is NULL, then return FALSE.
  If PemData is NULL and *PemSize is zero, then return FALSE and set
  *PemSize to the required size of the PemData buffer.
  If PemData is NULL and *PemSize is not zero, then return FALSE.
  If PemData is not NULL and *PemSize is too small, then return FALSE and
  set *PemSize to the required size of the PemData buffer.

  @retval  TRUE   EC Public Key was converted to the PEM data successfully.
  @retval  FALSE  Invalid EC Context.

**/
BOOLEAN
EFIAPI
EcPublicKeyToPEM (
  IN  VOID      *EcContext,
  OUT UINT8     *PemData,
  IN OUT UINTN  *PemSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
