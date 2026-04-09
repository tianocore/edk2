/** @file
  HMAC-SHA256 KDF Wrapper Implementation which does not provide real capabilities.

Copyright (c) 2018 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>

/**
  Derive key data using HMAC-SHA256 based KDF.

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha256ExtractAndExpand (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Derive SHA256 HMAC-based Extract key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       size of hkdf bytes to generate.

  @retval true   Hkdf generated successfully.
  @retval false  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha256Extract (
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Derive SHA256 HMAC-based Expand Key Derivation Function (HKDF).

  @param[in]   Prk              Pointer to the user-supplied key.
  @param[in]   PrkSize          Key size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha256Expand (
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Derive SHA384 HMAC-based Extract-and-Expand Key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha384ExtractAndExpand (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Derive SHA384 HMAC-based Extract key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       size of hkdf bytes to generate.

  @retval true   Hkdf generated successfully.
  @retval false  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha384Extract (
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Derive SHA384 HMAC-based Expand Key Derivation Function (HKDF).

  @param[in]   Prk              Pointer to the user-supplied key.
  @param[in]   PrkSize          Key size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha384Expand (
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
