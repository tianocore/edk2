/** @file
  PEM (Privacy Enhanced Mail) Format Handler Wrapper Implementation over MbedTLS.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/pem.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>

/**
  Retrieve the RSA Private Key from the password-protected PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA private key component. Use RsaFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.

  @retval  TRUE   RSA Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

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
  INT32                Ret;
  mbedtls_pk_context   Pk;
  mbedtls_rsa_context  *Rsa;
  UINT8                *NewPemData;
  UINTN                PasswordLen;

  if ((PemData == NULL) || (RsaContext == NULL) || (PemSize > INT_MAX)) {
    return FALSE;
  }

  NewPemData = NULL;
  if (PemData[PemSize - 1] != 0) {
    NewPemData = AllocateZeroPool (PemSize + 1);
    if (NewPemData == NULL) {
      return FALSE;
    }

    CopyMem (NewPemData, PemData, PemSize + 1);
    NewPemData[PemSize] = 0;
    PemData             = NewPemData;
    PemSize            += 1;
  }

  mbedtls_pk_init (&Pk);

  if (Password != NULL) {
    PasswordLen = AsciiStrLen (Password);
  } else {
    PasswordLen = 0;
  }

  Ret = mbedtls_pk_parse_key (&Pk, PemData, PemSize, (CONST UINT8 *)Password, PasswordLen, NULL, NULL);

  if (NewPemData != NULL) {
    FreePool (NewPemData);
    NewPemData = NULL;
  }

  if (Ret != 0) {
    mbedtls_pk_free (&Pk);
    return FALSE;
  }

  if (mbedtls_pk_get_type (&Pk) != MBEDTLS_PK_RSA) {
    mbedtls_pk_free (&Pk);
    return FALSE;
  }

  Rsa = RsaNew ();
  if (Rsa == NULL) {
    mbedtls_pk_free (&Pk);
    return FALSE;
  }

  Ret = mbedtls_rsa_copy (Rsa, mbedtls_pk_rsa (Pk));
  if (Ret != 0) {
    RsaFree (Rsa);
    mbedtls_pk_free (&Pk);
    return FALSE;
  }

  mbedtls_pk_free (&Pk);

  *RsaContext = Rsa;
  return TRUE;
}

/**
  Retrieve the EC Private Key from the password-protected PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which contain the retrieved
                           EC private key component. Use EcFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval  TRUE   EC Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

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
