/** @file
  Definitions for TPM2 helper functions

Copyright (c), Microsoft Corporation.
Copyright (c) 2013 - 2024, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM2_HELP_LIB_H_
#define TPM2_HELP_LIB_H_

#include <IndustryStandard/Tpm20.h>

/**
  Return size of digest.

  @param[in] HashAlgo  Hash algorithm

  @return size of digest
**/
UINT16
EFIAPI
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  );

/**
  Get hash mask from algorithm.

  @param[in] HashAlgo   Hash algorithm

  @return Hash mask
**/
UINT32
EFIAPI
GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  );

/**
  Copy AuthSessionIn to TPM2 command buffer.

  @param [in]  AuthSessionIn   Input AuthSession data
  @param [out] AuthSessionOut  Output AuthSession data in TPM2 command buffer

  @return AuthSession size
**/
UINT32
EFIAPI
CopyAuthSessionCommand (
  IN      TPMS_AUTH_COMMAND  *AuthSessionIn  OPTIONAL,
  OUT     UINT8              *AuthSessionOut
  );

/**
  Copy AuthSessionIn from TPM2 response buffer.

  @param [in]  AuthSessionIn   Input AuthSession data in TPM2 response buffer
  @param [out] AuthSessionOut  Output AuthSession data

  @return AuthSession size
**/
UINT32
EFIAPI
CopyAuthSessionResponse (
  IN      UINT8               *AuthSessionIn,
  OUT     TPMS_AUTH_RESPONSE  *AuthSessionOut OPTIONAL
  );

/**
  Return if hash alg is supported in HashAlgorithmMask.

  @param HashAlg            Hash algorithm to be checked.
  @param HashAlgorithmMask  Bitfield of allowed hash algorithms.

  @retval TRUE  Hash algorithm is supported.
  @retval FALSE Hash algorithm is not supported.
**/
BOOLEAN
EFIAPI
IsHashAlgSupportedInHashAlgorithmMask (
  IN TPMI_ALG_HASH  HashAlg,
  IN UINT32         HashAlgorithmMask
  );

/**
  Copy TPML_DIGEST_VALUES into a buffer

  @param[in,out] Buffer             Buffer to hold copied TPML_DIGEST_VALUES compact binary.
  @param[in]     DigestList         TPML_DIGEST_VALUES to be copied.
  @param[in]     HashAlgorithmMask  HASH bits corresponding to the desired digests to copy.

  @return The end of buffer to hold TPML_DIGEST_VALUES.
**/
VOID *
EFIAPI
CopyDigestListToBuffer (
  IN OUT VOID            *Buffer,
  IN TPML_DIGEST_VALUES  *DigestList,
  IN UINT32              HashAlgorithmMask
  );

/**
  Copy a buffer into  TPML_DIGEST_VALUES structure.
  This is the opposite to the CopyDigestListToBuffer function.

  @param[in]     Buffer             Buffer to hold TPML_DIGEST_VALUES compact binary.
  @param[in]     BufferSize         Size of Buffer.
  @param[in,out] DigestList         TPML_DIGEST_VALUES.

  @return EFI_STATUS
  @retval EFI_SUCCESS               Buffer was successfully copied to Digest List.
  @retval EFI_BAD_BUFFER_SIZE       Bad buffer size passed to function.
  @retval EFI_INVALID_PARAMETER     Invalid parameter passed to function: NULL pointer or
                                    BufferSize bigger than TPML_DIGEST_VALUES
**/
EFI_STATUS
EFIAPI
CopyBufferToDigestList (
  IN     VOID                *Buffer,
  IN     UINT32              BufferSize,
  IN OUT TPML_DIGEST_VALUES  *DigestList
  );

/**
  Get TPML_DIGEST_VALUES data size.

  @param[in]     DigestList    TPML_DIGEST_VALUES data.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
GetDigestListSize (
  IN TPML_DIGEST_VALUES  *DigestList
  );

/**
  Get TPML_DIGEST_VALUES data size from HashAlgorithmMask

  @param[in]     DigestList    TPML_DIGEST_VALUES data.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
GetDigestListSizeFromHashAlgorithmMask (
  IN UINT32  HashAlgorithmMask
  );

/**
  This function get digest from digest list.

  @param[in]  HashAlg       Digest algorithm
  @param[in]  DigestList    Digest list
  @param[out] Digest        Digest

  @retval EFI_SUCCESS            Digest is found and returned.
  @retval EFI_NOT_FOUND          Digest is not found.
  @retval EFI_INVALID_PARAMETER  DigestList or Digest invalid.
**/
EFI_STATUS
EFIAPI
GetDigestFromDigestList (
  IN TPMI_ALG_HASH       HashAlg,
  IN TPML_DIGEST_VALUES  *DigestList,
  OUT VOID               *Digest
  );

#endif
