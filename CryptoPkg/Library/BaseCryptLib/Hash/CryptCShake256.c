/** @file
  cSHAKE-256 Digest Wrapper Implementations.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

#define  CSHAKE256_SECURITY_STRENGTH  256
#define  CSHAKE256_RATE_IN_BYTES      136

const CHAR8  mZeroPadding[CSHAKE256_RATE_IN_BYTES] = { 0 };

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the beginning of the string by inserting the length of the byte string
  before the byte string representation of input.

  @param[out] EncBuf  Result of left encode.
  @param[in]  Value   Input of left encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
LeftEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  );

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the end of the string by inserting the length of the byte string after
  the byte string representation of input.

  @param[out] EncBuf  Result of right encode.
  @param[in]  Value   Input of right encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
RightEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  );

/**
  Keccak initial fuction.

  Set up state with specified capacity.

  @param[out] Context           Pointer to the context being initialized.
  @param[in]  Pad               Delimited Suffix.
  @param[in]  BlockSize         Size of context block.
  @param[in]  MessageDigestLen  Size of message digest in bytes.

  @retval 1  Initialize successfully.
  @retval 0  Fail to initialize.
**/
UINT8
EFIAPI
KeccakInit (
  OUT Keccak1600_Ctx  *Context,
  IN  UINT8           Pad,
  IN  UINTN           BlockSize,
  IN  UINTN           MessageDigstLen
  );

/**
  Sha3 update fuction.

  This function performs Sha3 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.

  @param[in,out] Context   Pointer to the Keccak context.
  @param[in]     Data      Pointer to the buffer containing the data to be hashed.
  @param[in]     DataSize  Size of Data buffer in bytes.

  @retval 1  Update successfully.
**/
UINT8
EFIAPI
Sha3Update (
  IN OUT Keccak1600_Ctx  *Context,
  IN const VOID          *Data,
  IN UINTN               DataSize
  );

/**
  Completes computation of Sha3 message digest.

  This function completes sha3 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the keccak context cannot
  be used again.

  @param[in, out]  Context        Pointer to the keccak context.
  @param[out]      MessageDigest  Pointer to a buffer that receives the message digest.

  @retval 1   Meaasge digest computation succeeded.
**/
UINT8
EFIAPI
Sha3Final (
  IN OUT Keccak1600_Ctx  *Context,
  OUT    UINT8           *MessageDigest
  );

/**
  Digests the input data and updates cSHAKE-256 context.

  This function performs cSHAKE-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  cSHAKE-256 context should be already correctly initialized by CShake256Initialize(), and should not be finalized
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
  cSHAKE-256 context should be already correctly initialized by CShake256Initialize(), and should not be
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

  //
  // Check input parameters.
  //
  if ((&Ctx == NULL) || (OutputLen == 0) || ((NameLen != 0) && (Name == NULL)) || ((CustomizationLen != 0) && (Customization == NULL))) {
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
                        (Keccak1600_Ctx *)&Ctx,
                        '\x1f',
                        (KECCAK1600_WIDTH - CSHAKE256_SECURITY_STRENGTH * 2) / 8,
                        OutputLen
                        );
  } else {
    UINT8  EncBuf[sizeof (UINTN) + 1];
    UINTN  EncLen;
    UINTN  AbsorbLen;
    UINTN  PadLen;

    Status = (BOOLEAN)KeccakInit (
                        (Keccak1600_Ctx *)&Ctx,
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
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;

    //
    // Absorb encode_string(N).
    //
    EncLen = LeftEncode (EncBuf, NameLen * 8);
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;
    Status     = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, Name, NameLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += NameLen;

    //
    // Absorb encode_string(S).
    //
    EncLen = LeftEncode (EncBuf, CustomizationLen * 8);
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, EncBuf, EncLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += EncLen;
    Status     = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, Customization, CustomizationLen);
    if (!Status) {
      return FALSE;
    }

    AbsorbLen += CustomizationLen;

    //
    // Absorb zero padding up to rate.
    //
    PadLen = CSHAKE256_RATE_IN_BYTES - AbsorbLen % CSHAKE256_RATE_IN_BYTES;
    Status = (BOOLEAN)Sha3Update ((Keccak1600_Ctx *)&Ctx, mZeroPadding, PadLen);
    if (!Status) {
      return FALSE;
    }
  }

  Status = CShake256Update (&Ctx, Data, DataSize);
  if (!Status) {
    return FALSE;
  }

  return CShake256Final (&Ctx, HashValue);
}
