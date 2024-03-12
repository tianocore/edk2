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
  mbedtls_pk_context   pk;
  mbedtls_rsa_context  *rsa;
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

  mbedtls_pk_init (&pk);

  if (Password != NULL) {
    PasswordLen = AsciiStrLen (Password);
  } else {
    PasswordLen = 0;
  }

  Ret = mbedtls_pk_parse_key (&pk, PemData, PemSize, (CONST UINT8 *)Password, PasswordLen, NULL, NULL);

  if (NewPemData != NULL) {
    FreePool (NewPemData);
    NewPemData = NULL;
  }

  if (Ret != 0) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  if (mbedtls_pk_get_type (&pk) != MBEDTLS_PK_RSA) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  rsa = RsaNew ();
  if (rsa == NULL) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  Ret = mbedtls_rsa_copy (rsa, mbedtls_pk_rsa (pk));
  if (Ret != 0) {
    RsaFree (rsa);
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  mbedtls_pk_free (&pk);

  *RsaContext = rsa;
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
  INT32                 Ret;
  mbedtls_pk_context    pk;
  mbedtls_ecdh_context  *ecdh;
  UINT8                 *NewPemData;
  UINTN                 PasswordLen;

  if ((PemData == NULL) || (EcContext == NULL) || (PemSize > INT_MAX)) {
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

  mbedtls_pk_init (&pk);

  if (Password != NULL) {
    PasswordLen = AsciiStrLen (Password);
  } else {
    PasswordLen = 0;
  }

  Ret = mbedtls_pk_parse_key (&pk, PemData, PemSize, (CONST UINT8 *)Password, PasswordLen, NULL, NULL);

  if (NewPemData != NULL) {
    FreePool (NewPemData);
    NewPemData = NULL;
  }

  if (Ret != 0) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  if (mbedtls_pk_get_type (&pk) != MBEDTLS_PK_ECKEY) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  ecdh = AllocateZeroPool (sizeof (mbedtls_ecdh_context));
  if (ecdh == NULL) {
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  mbedtls_ecdh_init (ecdh);

  Ret = mbedtls_ecdh_get_params (ecdh, mbedtls_pk_ec (pk), MBEDTLS_ECDH_OURS);
  if (Ret != 0) {
    mbedtls_ecdh_free (ecdh);
    FreePool (ecdh);
    mbedtls_pk_free (&pk);
    return FALSE;
  }

  mbedtls_pk_free (&pk);

  *EcContext = ecdh;
  return TRUE;
}
