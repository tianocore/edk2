/** @file
  EdDSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  Creates a new EdDSA public key object.

  If A is NULL, then return FALSE.
  If Pkey is NULL, then return FALSE.

  @param[in]      A      Pointer to EdDSA Raw public key.
  @param[in]      Nid    Crypto NID of the EdDSA curve.
  @param[in][out] Pkey   Pointer to new EdDSA public key object.

  @retval  TRUE   EdDSA public key object created successfully.
  @retval  FALSE  Invalid input or unsupported Crypto NID.
**/
BOOLEAN
EFIAPI
EdDsaCreatePublicKeyObject (
  IN     VOID   *A,
  IN     UINTN  Nid,
  IN OUT VOID   **Pkey
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Verifies the EdDSA signature. EdDSA signatures are verified via 'pure' implementation.
  Meaning message digest cannot be computed ahead of time. Data is passed into the verification function

  If Pkey is NULL, then return FALSE.
  If Data is NULL, then return FALSE.
  If DataSize is 0, then return FALSE.
  If Sig is NULL, then return FALSE.
  If SigSize is 0, then return FALSE.

  @param[in]  Pkey       Pointer to EdDSA public key object.
  @param[in]  Data       Pointer to data being verified.
  @param[in]  DataSize   Size of data.
  @param[in]  Signature  Pointer to EdDSA signature.
  @param[in]  SigSize    Size of EdDSA signature.

  @retval  TRUE   Valid signature encoded in EdDSA.
  @retval  FALSE  Invalid signature or invalid input.
**/
BOOLEAN
EFIAPI
EdDsaVerify (
  IN       VOID   *Pkey,
  IN CONST UINT8  *Data,
  IN CONST UINTN  DataSize,
  IN CONST UINT8  *Signature,
  IN CONST UINTN  SigSize,
  )
{
  ASSERT (FALSE);
  return FALSE;
}
