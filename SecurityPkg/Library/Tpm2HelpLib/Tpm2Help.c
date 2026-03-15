/** @file
  Implement TPM2 help.

Copyright (c), Microsoft Corporation.
Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2HelpLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

typedef struct {
  TPMI_ALG_HASH    HashAlgo;
  UINT16           HashSize;
  UINT32           HashMask;
} INTERNAL_HASH_INFO;

STATIC INTERNAL_HASH_INFO  mHashInfo[] = {
  { TPM_ALG_SHA1,    SHA1_DIGEST_SIZE,    HASH_ALG_SHA1    },
  { TPM_ALG_SHA256,  SHA256_DIGEST_SIZE,  HASH_ALG_SHA256  },
  { TPM_ALG_SM3_256, SM3_256_DIGEST_SIZE, HASH_ALG_SM3_256 },
  { TPM_ALG_SHA384,  SHA384_DIGEST_SIZE,  HASH_ALG_SHA384  },
  { TPM_ALG_SHA512,  SHA512_DIGEST_SIZE,  HASH_ALG_SHA512  },
};

/**
  Check if DigestList has an entry for HashAlg.

  @param DigestList         Digest list.
  @param HashAlg            Hash algorithm id.

  @retval TRUE  Match found.
  @retval FALSE No match found.
**/
STATIC
BOOLEAN
CheckDigestListForHashAlg (
  IN TPML_DIGEST_VALUES  *DigestList,
  IN TPM_ALG_ID          HashAlg
  )
{
  UINT32  Index;

  for (Index = 0; Index < DigestList->count; Index++) {
    if (DigestList->digests[Index].hashAlg == HashAlg) {
      DEBUG ((DEBUG_INFO, "Hash alg 0x%x found in DigestList.\n", HashAlg));
      return TRUE;
    }
  }

  DEBUG ((DEBUG_INFO, "Hash alg 0x%x not found in DigestList.\n", HashAlg));
  return FALSE;
}

/**
  Return size of digest.

  @param[in] HashAlgo  Hash algorithm

  @return size of digest
**/
UINT16
EFIAPI
Tpm2GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mHashInfo)/sizeof (mHashInfo[0]); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return mHashInfo[Index].HashSize;
    }
  }

  return 0;
}

/**
  Get hash mask from algorithm.

  @param[in] HashAlgo   Hash algorithm

  @return Hash mask
**/
UINT32
EFIAPI
Tpm2GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mHashInfo)/sizeof (mHashInfo[0]); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return mHashInfo[Index].HashMask;
    }
  }

  return 0;
}

/**
  Copy AuthSessionIn to TPM2 command buffer.

  @param [in]  AuthSessionIn   Input AuthSession data
  @param [out] AuthSessionOut  Output AuthSession data in TPM2 command buffer

  @return AuthSession size
**/
UINT32
EFIAPI
Tpm2CopyAuthSessionCommand (
  IN      TPMS_AUTH_COMMAND  *AuthSessionIn  OPTIONAL,
  OUT     UINT8              *AuthSessionOut
  )
{
  UINT8  *Buffer;

  if (AuthSessionOut == NULL) {
    return 0;
  }

  Buffer = (UINT8 *)AuthSessionOut;

  //
  // Add in Auth session
  //
  if (AuthSessionIn != NULL) {
    //  sessionHandle
    WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (AuthSessionIn->sessionHandle));
    Buffer += sizeof (UINT32);

    // nonce
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (AuthSessionIn->nonce.size));
    Buffer += sizeof (UINT16);

    CopyMem (Buffer, AuthSessionIn->nonce.buffer, AuthSessionIn->nonce.size);
    Buffer += AuthSessionIn->nonce.size;

    // sessionAttributes
    *(UINT8 *)Buffer = *(UINT8 *)&AuthSessionIn->sessionAttributes;
    Buffer++;

    // hmac
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (AuthSessionIn->hmac.size));
    Buffer += sizeof (UINT16);

    CopyMem (Buffer, AuthSessionIn->hmac.buffer, AuthSessionIn->hmac.size);
    Buffer += AuthSessionIn->hmac.size;
  } else {
    //  sessionHandle
    WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (TPM_RS_PW));
    Buffer += sizeof (UINT32);

    // nonce = nullNonce
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (0));
    Buffer += sizeof (UINT16);

    // sessionAttributes = 0
    *(UINT8 *)Buffer = 0x00;
    Buffer++;

    // hmac = nullAuth
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (0));
    Buffer += sizeof (UINT16);
  }

  return (UINT32)((UINTN)Buffer - (UINTN)AuthSessionOut);
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
Tpm2CopyAuthSessionResponse (
  IN      UINT8               *AuthSessionIn,
  OUT     TPMS_AUTH_RESPONSE  *AuthSessionOut OPTIONAL
  )
{
  UINT8               *Buffer;
  TPMS_AUTH_RESPONSE  LocalAuthSessionOut;

  if (AuthSessionOut == NULL) {
    AuthSessionOut = &LocalAuthSessionOut;
  }

  if (AuthSessionIn == NULL) {
    return 0;
  }

  Buffer = (UINT8 *)AuthSessionIn;

  // nonce
  AuthSessionOut->nonce.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  Buffer                    += sizeof (UINT16);
  if (AuthSessionOut->nonce.size > sizeof (TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2CopyAuthSessionResponse - nonce.size error %x\n", AuthSessionOut->nonce.size));
    return 0;
  }

  CopyMem (AuthSessionOut->nonce.buffer, Buffer, AuthSessionOut->nonce.size);
  Buffer += AuthSessionOut->nonce.size;

  // sessionAttributes
  *(UINT8 *) &AuthSessionOut->sessionAttributes = *(UINT8 *)Buffer;
  Buffer++;

  // hmac
  AuthSessionOut->hmac.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  Buffer                   += sizeof (UINT16);
  if (AuthSessionOut->hmac.size > sizeof (TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2CopyAuthSessionResponse - hmac.size error %x\n", AuthSessionOut->hmac.size));
    return 0;
  }

  CopyMem (AuthSessionOut->hmac.buffer, Buffer, AuthSessionOut->hmac.size);
  Buffer += AuthSessionOut->hmac.size;

  return (UINT32)((UINTN)Buffer - (UINTN)AuthSessionIn);
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
Tpm2IsHashAlgSupportedInHashAlgorithmMask (
  IN TPMI_ALG_HASH  HashAlg,
  IN UINT32         HashAlgorithmMask
  )
{
  switch (HashAlg) {
    case TPM_ALG_SHA1:
      if ((HashAlgorithmMask & HASH_ALG_SHA1) != 0) {
        return TRUE;
      }

      break;
    case TPM_ALG_SHA256:
      if ((HashAlgorithmMask & HASH_ALG_SHA256) != 0) {
        return TRUE;
      }

      break;
    case TPM_ALG_SHA384:
      if ((HashAlgorithmMask & HASH_ALG_SHA384) != 0) {
        return TRUE;
      }

      break;
    case TPM_ALG_SHA512:
      if ((HashAlgorithmMask & HASH_ALG_SHA512) != 0) {
        return TRUE;
      }

      break;
    case TPM_ALG_SM3_256:
      if ((HashAlgorithmMask & HASH_ALG_SM3_256) != 0) {
        return TRUE;
      }

      break;
  }

  return FALSE;
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
Tpm2CopyDigestListToBuffer (
  IN OUT VOID            *Buffer,
  IN TPML_DIGEST_VALUES  *DigestList,
  IN UINT32              HashAlgorithmMask
  )
{
  UINTN   Index;
  UINT16  DigestSize;
  UINT32  DigestListCount;
  UINT32  *DigestListCountPtr;

  if ((Buffer == NULL) || (DigestList == NULL)) {
    return NULL;
  }

  DigestListCountPtr = (UINT32 *)Buffer;
  DigestListCount    = 0;
  Buffer             = (UINT8 *)Buffer + sizeof (DigestList->count);
  for (Index = 0; Index < DigestList->count; Index++) {
    if (!Tpm2IsHashAlgSupportedInHashAlgorithmMask (DigestList->digests[Index].hashAlg, HashAlgorithmMask)) {
      DEBUG ((DEBUG_ERROR, "WARNING: TPM2 Event log has HashAlg unsupported by PCR bank (0x%x)\n", DigestList->digests[Index].hashAlg));
      continue;
    }

    CopyMem (Buffer, &DigestList->digests[Index].hashAlg, sizeof (DigestList->digests[Index].hashAlg));
    Buffer     = (UINT8 *)Buffer + sizeof (DigestList->digests[Index].hashAlg);
    DigestSize = Tpm2GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    CopyMem (Buffer, &DigestList->digests[Index].digest, DigestSize);
    Buffer = (UINT8 *)Buffer + DigestSize;
    DigestListCount++;
  }

  WriteUnaligned32 (DigestListCountPtr, DigestListCount);

  return Buffer;
}

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
Tpm2CopyBufferToDigestList (
  IN     VOID                *Buffer,
  IN     UINT32              BufferSize,
  IN OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  EFI_STATUS    Status = EFI_INVALID_PARAMETER;
  UINTN         Index;
  UINT16        DigestSize;
  UINT8 *CONST  pBuffer = (UINT8 *CONST)Buffer;

  if ((Buffer == NULL) || (DigestList == NULL) || (BufferSize > sizeof (TPML_DIGEST_VALUES))) {
    return EFI_INVALID_PARAMETER;
  }

  DigestList->count = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)Buffer));
  if (DigestList->count > HASH_COUNT) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (UINT8 *)Buffer +  sizeof (UINT32);
  for (Index = 0; Index < DigestList->count; Index++) {
    if ((UINT32)((UINT8 *)Buffer - pBuffer + sizeof (UINT16)) > BufferSize ) {
      Status = EFI_BAD_BUFFER_SIZE;
      break;
    } else {
      DigestList->digests[Index].hashAlg = SwapBytes16 (ReadUnaligned16 ((CONST UINT16 *)Buffer));
    }

    Buffer     = (UINT8 *)Buffer + sizeof (UINT16);
    DigestSize = Tpm2GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    if ((UINT32)((UINT8 *)Buffer - pBuffer + DigestSize) > BufferSize ) {
      Status = EFI_BAD_BUFFER_SIZE;
      break;
    } else {
      CopyMem (&DigestList->digests[Index].digest, Buffer, DigestSize);
    }

    Buffer = (UINT8 *)Buffer + DigestSize;
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Get TPML_DIGEST_VALUES data size.

  @param[in]     DigestList    TPML_DIGEST_VALUES data.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
Tpm2GetDigestListSize (
  IN TPML_DIGEST_VALUES  *DigestList
  )
{
  UINTN   Index;
  UINT16  DigestSize;
  UINT32  TotalSize;

  if (DigestList == NULL) {
    return 0;
  }

  TotalSize = sizeof (DigestList->count);
  for (Index = 0; Index < DigestList->count; Index++) {
    DigestSize = Tpm2GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    TotalSize += sizeof (DigestList->digests[Index].hashAlg) + DigestSize;
  }

  return TotalSize;
}

/**
  Get TPML_DIGEST_VALUES data size from HashAlgorithmMask

  @param[in]     HashAlgorithmMask.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
Tpm2GetDigestListSizeFromHashAlgorithmMask (
  IN UINT32  HashAlgorithmMask
  )
{
  UINTN   Index;
  UINT32  TotalSize;

  TotalSize = sizeof (UINT32);
  for (Index = 0; Index < sizeof (mHashInfo)/sizeof (mHashInfo[0]); Index++) {
    if (mHashInfo[Index].HashMask & HashAlgorithmMask) {
      TotalSize += sizeof (TPMI_ALG_HASH) + mHashInfo[Index].HashSize;
    }
  }

  return TotalSize;
}

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
Tpm2GetDigestFromDigestList (
  IN TPMI_ALG_HASH       HashAlg,
  IN TPML_DIGEST_VALUES  *DigestList,
  OUT VOID               *Digest
  )
{
  UINTN   Index;
  UINT16  DigestSize;

  if ((DigestList == NULL) || (Digest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DigestSize = Tpm2GetHashSizeFromAlgo (HashAlg);
  for (Index = 0; Index < DigestList->count; Index++) {
    if (DigestList->digests[Index].hashAlg == HashAlg) {
      CopyMem (
        Digest,
        &DigestList->digests[Index].digest,
        DigestSize
        );
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Check if all hash algorithms supported in HashAlgorithmMask are
  present in the DigestList.

  @param DigestList         Digest list.
  @param HashAlgorithmMask  Bitfield of allowed hash algorithms.

  @retval TRUE  All hash algorithms present.
  @retval FALSE Some hash algorithms not present.
**/
BOOLEAN
EFIAPI
Tpm2IsDigestListInSyncWithHashAlgorithmMask (
  IN TPML_DIGEST_VALUES  *DigestList,
  IN UINT32              HashAlgorithmMask
  )
{
  if (DigestList == NULL) {
    return FALSE;
  }

  if ((HashAlgorithmMask & HASH_ALG_SHA1) != 0) {
    if (!CheckDigestListForHashAlg (DigestList, TPM_ALG_SHA1)) {
      return FALSE;
    }
  }

  if ((HashAlgorithmMask & HASH_ALG_SHA256) != 0) {
    if (!CheckDigestListForHashAlg (DigestList, TPM_ALG_SHA256)) {
      return FALSE;
    }
  }

  if ((HashAlgorithmMask & HASH_ALG_SHA384) != 0) {
    if (!CheckDigestListForHashAlg (DigestList, TPM_ALG_SHA384)) {
      return FALSE;
    }
  }

  if ((HashAlgorithmMask & HASH_ALG_SHA512) != 0) {
    if (!CheckDigestListForHashAlg (DigestList, TPM_ALG_SHA512)) {
      return FALSE;
    }
  }

  if ((HashAlgorithmMask & HASH_ALG_SM3_256) != 0) {
    if (!CheckDigestListForHashAlg (DigestList, TPM_ALG_SM3_256)) {
      return FALSE;
    }
  }

  return TRUE;
}
