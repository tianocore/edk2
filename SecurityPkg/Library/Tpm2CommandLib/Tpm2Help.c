/** @file
  Implement TPM2 help.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2HelpLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Return size of digest.

  @param[in] HashAlgo  Hash algorithm

  @return size of digest
**/
UINT16
EFIAPI
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  return Tpm2GetHashSizeFromAlgo (HashAlgo);
}

/**
  Get hash mask from algorithm.

  @param[in] HashAlgo   Hash algorithm

  @return Hash mask
**/
UINT32
EFIAPI
GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  return Tpm2GetHashMaskFromAlgo (HashAlgo);
}

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
  )
{
  return Tpm2CopyAuthSessionCommand (AuthSessionIn, AuthSessionOut);
}

/**
  Copy AuthSessionIn from TPM2 response buffer.

  @param [in]  AuthSessionIn   Input AuthSession data in TPM2 response buffer
  @param [out] AuthSessionOut  Output AuthSession data

  @return 0    copy failed
          else AuthSession size
**/
UINT32
EFIAPI
CopyAuthSessionResponse (
  IN      UINT8               *AuthSessionIn,
  OUT     TPMS_AUTH_RESPONSE  *AuthSessionOut OPTIONAL
  )
{
  return Tpm2CopyAuthSessionResponse (AuthSessionIn, AuthSessionOut);
}

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
  )
{
  return Tpm2IsHashAlgSupportedInHashAlgorithmMask (HashAlg, HashAlgorithmMask);
}

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
  )
{
  return Tpm2CopyDigestListToBuffer (Buffer, DigestList, HashAlgorithmMask);
}

/**
  Copy a buffer into a TPML_DIGEST_VALUES structure.

  @param[in]     Buffer             Buffer to hold TPML_DIGEST_VALUES compact binary.
  @param[in]     BufferSize         Size of Buffer.
  @param[out]    DigestList         TPML_DIGEST_VALUES.

  @retval EFI_SUCCESS               Buffer was succesfully copied to DigestList.
  @retval EFI_BAD_BUFFER_SIZE       A bad buffer size passed to the function.
  @retval EFI_INVALID_PARAMETER     An invalid parameter passed to the function: NULL pointer or
                                    BufferSize bigger than TPML_DIGEST_VALUES.
**/
EFI_STATUS
EFIAPI
CopyBufferToDigestList (
  IN CONST  VOID                *Buffer,
  IN        UINTN               BufferSize,
  OUT       TPML_DIGEST_VALUES  *DigestList
  )
{
  return Tpm2CopyBufferToDigestList (Buffer, BufferSize, DigestList);
}

/**
  Get TPML_DIGEST_VALUES data size.

  @param[in]     DigestList    TPML_DIGEST_VALUES data.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
GetDigestListSize (
  IN TPML_DIGEST_VALUES  *DigestList
  )
{
  return Tpm2GetDigestListSize (DigestList);
}

/**
  Get the total digest size from a hash algorithm mask.

  @param[in]     HashAlgorithmMask.

  @return Digest size in bytes.
**/
UINT32
EFIAPI
GetDigestListSizeFromHashAlgorithmMask (
  IN UINT32  HashAlgorithmMask
  )
{
  return Tpm2GetDigestListSizeFromHashAlgorithmMask (HashAlgorithmMask);
}

/**
  This function get digest from digest list.

  @param[in]  HashAlg       Digest algorithm
  @param[in]  DigestList    Digest list
  @param[out] Digest        Digest

  @retval EFI_SUCCESS       Digest is found and returned.
  @retval EFI_NOT_FOUND     Digest is not found.
**/
EFI_STATUS
EFIAPI
GetDigestFromDigestList (
  IN TPMI_ALG_HASH       HashAlg,
  IN TPML_DIGEST_VALUES  *DigestList,
  OUT VOID               *Digest
  )
{
  return Tpm2GetDigestFromDigestList (HashAlg, DigestList, Digest);
}
