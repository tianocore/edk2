/** @file
  HMAC-SHA256 KDF Wrapper Implementation over MbedTLS.

  RFC 5869: HMAC-based Extract-and-Expand Key Derivation Function (HKDF)

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/hkdf.h>

/**
  Derive HMAC-based Extract-and-Expand Key Derivation Function (HKDF).

  @param[in]   MdType           Message Digest Type.
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
STATIC
BOOLEAN
HkdfMdExtractAndExpand (
  IN   mbedtls_md_type_t  MdType,
  IN   CONST UINT8        *Key,
  IN   UINTN              KeySize,
  IN   CONST UINT8        *Salt,
  IN   UINTN              SaltSize,
  IN   CONST UINT8        *Info,
  IN   UINTN              InfoSize,
  OUT  UINT8              *Out,
  IN   UINTN              OutSize
  )
{
  const mbedtls_md_info_t  *md;
  INT32                    Ret;

  if ((Key == NULL) || (Salt == NULL) || (Info == NULL) || (Out == NULL) ||
      (KeySize > INT_MAX) || (SaltSize > INT_MAX) || (InfoSize > INT_MAX) || (OutSize > INT_MAX))
  {
    return FALSE;
  }

  md = mbedtls_md_info_from_type (MdType);
  ASSERT (md != NULL);

  Ret = mbedtls_hkdf (md, Salt, (UINT32)SaltSize, Key, (UINT32)KeySize, Info, (UINT32)InfoSize, Out, (UINT32)OutSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Derive HMAC-based Extract Key Derivation Function (HKDF).

  @param[in]   MdType           Message Digest Type.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
STATIC
BOOLEAN
HkdfMdExtract (
  IN   mbedtls_md_type_t  MdType,
  IN   CONST UINT8        *Key,
  IN   UINTN              KeySize,
  IN   CONST UINT8        *Salt,
  IN   UINTN              SaltSize,
  OUT  UINT8              *PrkOut,
  IN   UINTN              PrkOutSize
  )
{
  const mbedtls_md_info_t  *md;
  INT32                    Ret;
  UINTN                    MdSize;

  if ((Key == NULL) || (Salt == NULL) || (PrkOut == NULL) ||
      (KeySize > INT_MAX) || (SaltSize > INT_MAX) || (PrkOutSize > INT_MAX))
  {
    return FALSE;
  }

  MdSize = 0;
  switch (MdType) {
    case MBEDTLS_MD_SHA256:
      MdSize = SHA256_DIGEST_SIZE;
      break;
    case MBEDTLS_MD_SHA384:
      MdSize = SHA384_DIGEST_SIZE;
      break;
    case MBEDTLS_MD_SHA512:
      MdSize = SHA512_DIGEST_SIZE;
      break;
    default:
      return FALSE;
  }

  if (PrkOutSize != MdSize) {
    return FALSE;
  }

  md = mbedtls_md_info_from_type (MdType);
  ASSERT (md != NULL);

  Ret = mbedtls_hkdf_extract (md, Salt, (UINT32)SaltSize, Key, (UINT32)KeySize, PrkOut);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Derive HMAC-based Expand Key Derivation Function (HKDF).

  @param[in]   MdType           Message Digest Type.
  @param[in]   Prk              Pointer to the user-supplied key.
  @param[in]   PrkSize          Key size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
STATIC
BOOLEAN
HkdfMdExpand (
  IN   mbedtls_md_type_t  MdType,
  IN   CONST UINT8        *Prk,
  IN   UINTN              PrkSize,
  IN   CONST UINT8        *Info,
  IN   UINTN              InfoSize,
  OUT  UINT8              *Out,
  IN   UINTN              OutSize
  )
{
  const mbedtls_md_info_t  *md;
  INT32                    Ret;
  UINTN                    MdSize;

  if ((Prk == NULL) || (Info == NULL) || (Out == NULL) ||
      (PrkSize > INT_MAX) || (InfoSize > INT_MAX) || (OutSize > INT_MAX))
  {
    return FALSE;
  }

  switch (MdType) {
    case MBEDTLS_MD_SHA256:
      MdSize = SHA256_DIGEST_SIZE;
      break;
    case MBEDTLS_MD_SHA384:
      MdSize = SHA384_DIGEST_SIZE;
      break;
    case MBEDTLS_MD_SHA512:
      MdSize = SHA512_DIGEST_SIZE;
      break;
    default:
      return FALSE;
  }

  if (PrkSize != MdSize) {
    return FALSE;
  }

  md = mbedtls_md_info_from_type (MdType);
  ASSERT (md != NULL);

  Ret = mbedtls_hkdf_expand (md, Prk, (UINT32)PrkSize, Info, (UINT32)InfoSize, Out, (UINT32)OutSize);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Derive SHA256 HMAC-based Extract-and-Expand Key Derivation Function (HKDF).

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
  return HkdfMdExtractAndExpand (MBEDTLS_MD_SHA256, Key, KeySize, Salt, SaltSize, Info, InfoSize, Out, OutSize);
}

/**
  Derive SHA256 HMAC-based Extract Key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha256Extract (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  OUT  UINT8        *PrkOut,
  IN   UINTN        PrkOutSize
  )
{
  return HkdfMdExtract (MBEDTLS_MD_SHA256, Key, KeySize, Salt, SaltSize, PrkOut, PrkOutSize);
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
  return HkdfMdExpand (MBEDTLS_MD_SHA256, Prk, PrkSize, Info, InfoSize, Out, OutSize);
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
  return HkdfMdExtractAndExpand (MBEDTLS_MD_SHA384, Key, KeySize, Salt, SaltSize, Info, InfoSize, Out, OutSize);
}

/**
  Derive SHA384 HMAC-based Extract Key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
HkdfSha384Extract (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  OUT  UINT8        *PrkOut,
  IN   UINTN        PrkOutSize
  )
{
  return HkdfMdExtract (MBEDTLS_MD_SHA384, Key, KeySize, Salt, SaltSize, PrkOut, PrkOutSize);
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
  return HkdfMdExpand (MBEDTLS_MD_SHA384, Prk, PrkSize, Info, InfoSize, Out, OutSize);
}
