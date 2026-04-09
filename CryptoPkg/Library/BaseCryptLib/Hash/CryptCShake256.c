/** @file
  cSHAKE-256 Digest Wrapper Implementations.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CryptParallelHash.h"

#define  CSHAKE256_SECURITY_STRENGTH  256
#define  CSHAKE256_RATE_IN_BYTES      136

CONST CHAR8  mZeroPadding[CSHAKE256_RATE_IN_BYTES] = { 0 };

/**
  CShake256 initial function.

  Initializes user-supplied memory pointed by CShake256Context as cSHAKE-256 hash context for
  subsequent use.

  @param[out] CShake256Context  Pointer to cSHAKE-256 context being initialized.
  @param[in]  OutputLen         The desired number of output length in bytes.
  @param[in]  Name              Pointer to the function name string.
  @param[in]  NameLen           The length of the function name in bytes.
  @param[in]  Customization     Pointer to the customization string.
  @param[in]  CustomizationLen  The length of the customization string in bytes.

  @retval TRUE   cSHAKE-256 context initialization succeeded.
  @retval FALSE  cSHAKE-256 context initialization failed.
  @retval FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
CShake256Init (
  OUT  VOID        *CShake256Context,
  IN   UINTN       OutputLen,
  IN   CONST VOID  *Name,
  IN   UINTN       NameLen,
  IN   CONST VOID  *Customization,
  IN   UINTN       CustomizationLen
  )
{
  BOOLEAN  Status;
  UINT8    EncBuf[sizeof (UINTN) + 1];
  UINTN    EncLen;
  UINTN    AbsorbLen;
  UINTN    PadLen;

  //
  // Check input parameters.
  //
  if ((CShake256Context == NULL) || (OutputLen == 0) || ((NameLen != 0) && (Name == NULL)) || ((CustomizationLen != 0) && (Customization == NULL))) {
    return FALSE;
  }

  //
  // Initialize KECCAK context with pad value and block size.
  //
  if ((NameLen == 0) && (CustomizationLen == 0)) {
    //
    // When N and S are both empty strings, cSHAKE(X, L, N, S) is equivalent to
    // SHAKE as defined in FIPS 202.
    //
    Status = (BOOLEAN)KeccakInit (
                        (Keccak1600_Ctx *)CShake256Context,
                        '\x1f',
                        (KECCAK1600_WIDTH - CSHAKE256_SECURITY_STRENGTH * 2) / 8,
                        OutputLen
                        );

    return Status;
  } else {
    Status = (BOOLEAN)KeccakInit (
                        (Keccak1600_Ctx *)CShake256Context,
                        '\x04',
                        (KECCAK1600_WIDTH - CSHAKE256_SECURITY_STRENGTH * 2) / 8,
                        OutputLen
                        );
    if (!Status) {
      return FALSE;
    }

    AbsorbLen = 0;
    //
    // Absorb Absorb bytepad(.., rate).
    //
    EncLen = LeftEncode (EncBuf, CSHAKE256_RATE_IN_BYTES);
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;

    //
    // Absorb encode_string(N).
    //
    EncLen = LeftEncode (EncBuf, NameLen * 8);
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;
    Status     = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, Name, NameLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += NameLen;

    //
    // Absorb encode_string(S).
    //
    EncLen = LeftEncode (EncBuf, CustomizationLen * 8);
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;
    Status     = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, Customization, CustomizationLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += CustomizationLen;

    //
    // Absorb zero padding up to rate.
    //
    PadLen = CSHAKE256_RATE_IN_BYTES - AbsorbLen % CSHAKE256_RATE_IN_BYTES;
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)CShake256Context, mZeroPadding, PadLen);
    if (!Status) {
      return FALSE;
    }

    return TRUE;
  }
}

/**
  Digests the input data and updates cSHAKE-256 context.

  This function performs cSHAKE-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  cSHAKE-256 context should be already correctly initialized by CShake256Init(), and should not be finalized
  by CShake256Final(). Behavior with invalid context is undefined.

  @param[in, out]  CShake256Context   Pointer to the cSHAKE-256 context.
  @param[in]       Data               Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize           Size of Data buffer in bytes.

  @retval TRUE   cSHAKE-256 data digest succeeded.
  @retval FALSE  cSHAKE-256 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CShake256Update (
  IN OUT  VOID        *CShake256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  //
  // Check input parameters.
  //
  if (CShake256Context == NULL) {
    return FALSE;
  }

  //
  // Check invalid parameters, in case that only DataLength was checked in OpenSSL.
  //
  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  return (BOOLEAN)(Sha3Update ((Keccak1600_Ctx *)CShake256Context, Data, DataSize));
}

/**
  Completes computation of the cSHAKE-256 digest value.

  This function completes cSHAKE-256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the cSHAKE-256 context cannot
  be used again.
  cSHAKE-256 context should be already correctly initialized by CShake256Init(), and should not be
  finalized by CShake256Final(). Behavior with invalid cSHAKE-256 context is undefined.

  @param[in, out]  CShake256Context  Pointer to the cSHAKE-256 context.
  @param[out]      HashValue         Pointer to a buffer that receives the cSHAKE-256 digest
                                     value.

  @retval TRUE   cSHAKE-256 digest computation succeeded.
  @retval FALSE  cSHAKE-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CShake256Final (
  IN OUT  VOID   *CShake256Context,
  OUT     UINT8  *HashValue
  )
{
  //
  // Check input parameters.
  //
  if ((CShake256Context == NULL) || (HashValue == NULL)) {
    return FALSE;
  }

  //
  // cSHAKE-256 Hash Finalization.
  //
  return (BOOLEAN)(Sha3Final ((Keccak1600_Ctx *)CShake256Context, HashValue));
}

/**
  Computes the CSHAKE-256 message digest of a input data buffer.

  This function performs the CSHAKE-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  @param[in]   Data               Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize           Size of Data buffer in bytes.
  @param[in]   OutputLen          Size of output in bytes.
  @param[in]   Name               Pointer to the function name string.
  @param[in]   NameLen            Size of the function name in bytes.
  @param[in]   Customization      Pointer to the customization string.
  @param[in]   CustomizationLen   Size of the customization string in bytes.
  @param[out]  HashValue          Pointer to a buffer that receives the CSHAKE-256 digest
                                  value.

  @retval TRUE   CSHAKE-256 digest computation succeeded.
  @retval FALSE  CSHAKE-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CShake256HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  IN   UINTN       OutputLen,
  IN   CONST VOID  *Name,
  IN   UINTN       NameLen,
  IN   CONST VOID  *Customization,
  IN   UINTN       CustomizationLen,
  OUT  UINT8       *HashValue
  )
{
  BOOLEAN         Status;
  Keccak1600_Ctx  Ctx;

  //
  // Check input parameters.
  //
  if (HashValue == NULL) {
    return FALSE;
  }

  if ((Data == NULL) && (DataSize != 0)) {
    return FALSE;
  }

  Status = CShake256Init (&Ctx, OutputLen, Name, NameLen, Customization, CustomizationLen);
  if (!Status) {
    return FALSE;
  }

  Status = CShake256Update (&Ctx, Data, DataSize);
  if (!Status) {
    return FALSE;
  }

  return CShake256Final (&Ctx, HashValue);
}
