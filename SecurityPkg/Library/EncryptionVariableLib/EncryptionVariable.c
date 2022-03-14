/** @file
  Implementation of EncryptionVariableLib with AES algorithm support.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/VariableFormat.h>
#include <Library/EncryptionVariableLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "EncryptionVariable.h"

/**
  Derive encryption key for given variable from variable root key.

  The derivation algorithm is depicted below

    HKDF_Expand(SHA256, RootKey, Name||':'||Guid||':'||Attr||"VAR_ENC_KEY")

  @param[in]    VarEncInfo    Pointer to structure containing detailed
                              information about a variable.
  @param[in]    EncKeySize    Size of key requested.
  @param[out]   EncKey        Buffer of key.

  @retval TRUE    The key was derived successfully.
  @retval FALSE   Failed to generate encryption key.

**/
STATIC
BOOLEAN
EncVarLibGenEncKey (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo,
  IN UINTN                     EncKeySize,
  OUT UINT8                    *EncKey
  )
{
  BOOLEAN  Status;

  struct {
    VOID     *Data;
    UINTN    Size;
  }                 InfoGroup[6];
  UINT8  *Info;
  UINTN  InfoSize;
  UINTN  Index;
  UINT8  Salt[16];

  //
  // info: Name||':'||Guid||':'||Attr||"VAR_ENC_KEY"
  //
  InfoGroup[0].Size = VarEncInfo->Header.NameSize;
  InfoGroup[0].Data = VarEncInfo->Header.VariableName;

  InfoGroup[1].Size = ENC_KEY_SEP_SIZE;
  InfoGroup[1].Data = ENC_KEY_SEP;

  InfoGroup[2].Size = sizeof (*VarEncInfo->Header.VendorGuid);
  InfoGroup[2].Data = VarEncInfo->Header.VendorGuid;

  InfoGroup[3].Size = ENC_KEY_SEP_SIZE;
  InfoGroup[3].Data = ENC_KEY_SEP;

  InfoGroup[4].Size = sizeof (VarEncInfo->Header.Attributes);
  InfoGroup[4].Data = &VarEncInfo->Header.Attributes;

  InfoGroup[5].Size = ENC_KEY_NAME_SIZE;
  InfoGroup[5].Data = ENC_KEY_NAME;

  for (InfoSize = 0, Index = 0; Index < ARRAY_SIZE (InfoGroup); ++Index) {
    InfoSize += InfoGroup[Index].Size;
  }

  Info = AllocateZeroPool (InfoSize);
  if (Info == NULL) {
    ASSERT (Info != NULL);
    return FALSE;
  }

  for (InfoSize = 0, Index = 0; Index < ARRAY_SIZE (InfoGroup); ++Index) {
    CopyMem (Info + InfoSize, InfoGroup[Index].Data, InfoGroup[Index].Size);
    InfoSize += InfoGroup[Index].Size;
  }

  Status = HkdfSha256ExtractAndExpand (
             VarEncInfo->Key,
             VarEncInfo->KeySize,
             Salt,
             0,
             Info,
             InfoSize,
             EncKey,
             EncKeySize
             );

  FreePool (Info);

  return Status;
}

/**
  Generate init-vector for AES encryption.

  @param[out]   InitVector  IVEC buffer.
  @param[in]    Size        Size of IVEC requested.

  @retval TRUE    IVEC was generated successfully.
  @retval FALSE   Failed to generate IVEC.

**/
STATIC
BOOLEAN
EncVarLibGenIvec (
  OUT UINT8  *InitVector,
  IN  UINTN  Size
  )
{
  return RandomBytes (InitVector, Size);
}

/**
  Check if there's valid variable information needed by encrypting or decrypting.

  @param[in]    VarEncInfo    Buffer conveying details about a variable.
  @param[in]    CheckForEnc   Flag indicating check for encrypting (TRUE) or
                              decrypting (FALSE).

  @retval TRUE    VarEncInfo is valid.
  @retval FALSE   VarEncInfo is invalid.

**/
STATIC
BOOLEAN
IsValidVariableInfo (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo,
  IN BOOLEAN                   CheckForEnc
  )
{
  BOOLEAN  Valid;

  if (CheckForEnc) {
    Valid = (VarEncInfo->Header.Data != NULL && VarEncInfo->Header.DataSize > 0)
            || (VarEncInfo->PlainData != NULL && VarEncInfo->PlainDataSize > 0);
    if (!Valid) {
      ASSERT (
        (VarEncInfo->Header.Data != NULL && VarEncInfo->Header.DataSize > 0)
             || (VarEncInfo->PlainData != NULL && VarEncInfo->PlainDataSize > 0)
        );
    }
  } else {
    Valid = (VarEncInfo->Header.Data != NULL && VarEncInfo->Header.DataSize > 0)
            || (VarEncInfo->CipherData != NULL && VarEncInfo->CipherDataSize > 0);
    if (!Valid) {
      ASSERT (
        (VarEncInfo->Header.Data != NULL && VarEncInfo->Header.DataSize > 0)
             || (VarEncInfo->CipherData != NULL && VarEncInfo->CipherDataSize > 0)
        );
    }
  }

  Valid = Valid
          && VarEncInfo->Header.VariableName != NULL
          && VarEncInfo->Header.NameSize > 0
          && VarEncInfo->Header.VendorGuid != NULL
          && VarEncInfo->Key != NULL
          && VarEncInfo->KeySize > 0;
  if (!Valid) {
    ASSERT (VarEncInfo->Header.VariableName != NULL);
    ASSERT (VarEncInfo->Header.NameSize != 0);
    ASSERT (VarEncInfo->Header.VendorGuid != NULL);
    ASSERT (VarEncInfo->Key != NULL);
    ASSERT (VarEncInfo->KeySize > 0);
  }

  return Valid;
}

/**
  Sanity check of encrption header prefixed to encrypted data.

  @param[in]    EncHeader   Pointer to VARIABLE_ENCRYPTION_HEADER.
  @param[in]    DataSize    Size of variable data payload.

  @retval TRUE    EncHeader is valid.
  @retval FALSE   EncHeader is invalid.

**/
STATIC
BOOLEAN
IsValidEncrptionHeader (
  IN VARIABLE_ENCRYPTION_HEADER  *EncHeader,
  IN UINT32                      DataSize
  )
{
  if (  (DataSize > sizeof (VARIABLE_ENCRYPTION_HEADER))
     && ((EncHeader->DataType == ENC_TYPE_AES) || (EncHeader->DataType == ENC_TYPE_NULL))
     && (EncHeader->HeaderSize >= sizeof (VARIABLE_ENCRYPTION_HEADER))
     && (EncHeader->CipherDataSize > 0)
     && ((EncHeader->CipherDataSize % ENC_BLOCK_SIZE) == 0)
     && (EncHeader->PlainDataSize > 0)
     && (EncHeader->PlainDataSize <= EncHeader->CipherDataSize)
     && ((EncHeader->CipherDataSize + EncHeader->HeaderSize) <= DataSize))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Encrypt variable data.

  If VarEncInfo->PlainData is not NULL, VarEncInfo->PlainData holds the plain
  data. Otherwise, VarEncInfo->Headr.Data is supposed to be the plain data.

  If VarEncInfo->CipherData is not NULL, The encrypted data is stored in
  VarEncInfo->CipherData. Otherwise, the encrypted data is stored directly
  in variable data buffer, i.e. VarEncInfo->Headr.Data.

  @param[in, out]   VarEncInfo   Pointer to structure containing detailed
                                 information about a variable.

  @retval EFI_SUCCESS             Variable was encrypted successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_BUFFER_TOO_SMALL    VarEncInfo->CipherData is not NULL but
                                  VarEncInfo->CipherDataSize is too small.
  @retval EFI_ABORTED             Uknown error occurred during encrypting.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED         Unsupported to encrypt variable.

**/
EFI_STATUS
EFIAPI
EncryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  EFI_STATUS                  Status;
  VOID                        *AesContext;
  UINT8                       EncKey[ENC_KEY_SIZE];
  UINT8                       Ivec[ENC_IVEC_SIZE];
  UINT8                       *PlainBuffer;
  UINT8                       *CipherBuffer;
  UINT8                       *PlainData;
  UINT32                      PlainDataSize;
  VARIABLE_ENCRYPTION_HEADER  *CipherData;
  UINT32                      CipherDataSize;
  UINT32                      PaddingBytes;

  Status       = EFI_ABORTED;
  AesContext   = NULL;
  PlainBuffer  = NULL;
  CipherBuffer = NULL;

  if (!IsValidVariableInfo (VarEncInfo, TRUE)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VarEncInfo->PlainData != NULL) {
    PlainData     = VarEncInfo->PlainData;
    PlainDataSize = VarEncInfo->PlainDataSize;
  } else {
    PlainData     = VarEncInfo->Header.Data;
    PlainDataSize = (UINT32)VarEncInfo->Header.DataSize;
  }

  CipherDataSize = AES_CIPHER_DATA_SIZE (PlainDataSize);
  if (VarEncInfo->CipherData != NULL) {
    if (VarEncInfo->CipherDataSize
        < (CipherDataSize + sizeof (VARIABLE_ENCRYPTION_HEADER)))
    {
      VarEncInfo->CipherDataSize = CipherDataSize
                                   + sizeof (VARIABLE_ENCRYPTION_HEADER);
      return EFI_BUFFER_TOO_SMALL;
    }

    CipherData = VarEncInfo->CipherData;
  } else {
    CipherData = VarEncInfo->Header.Data;
  }

  //
  // Prepare buffer for encrypted data.
  //
  if ((UINTN)CipherData == (UINTN)PlainData) {
    //
    // Need buffer to store the encrypted data temporarily.
    //
    CipherBuffer = (UINT8 *)AllocateZeroPool (
                              CipherDataSize
                              + sizeof (VARIABLE_ENCRYPTION_HEADER)
                              );
    if (CipherBuffer == NULL) {
      ASSERT (CipherBuffer != NULL);
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    CipherBuffer = (UINT8 *)CipherData;
  }

  //
  // Plain variable data must also be multiple of ENC_BLOCK_SIZE.
  //
  PaddingBytes = ALIGN_VALUE (PlainDataSize, ENC_BLOCK_SIZE) - PlainDataSize;
  if (PaddingBytes != 0) {
    //
    // Since the plain data size will be saved in the VARIABLE_ENCRYPTION_HEADER,
    // there's no need to do PKCS way of padding. To save space, just padding
    // the plain data to be of the nearest n*ENC_BLOCK_SIZE.
    //
    PlainBuffer = AllocateZeroPool (PlainDataSize + PaddingBytes);
    if (PlainBuffer == NULL) {
      ASSERT (PlainBuffer != NULL);
      goto Done;
    }

    CopyMem (PlainBuffer, PlainData, PlainDataSize);
    SetMem (PlainBuffer + PlainDataSize, PaddingBytes, ENC_PADDING_BYTE);
  } else {
    PlainBuffer = PlainData;
  }

  //
  // Skip EFI_VARIABLE_APPEND_WRITE bit in generating encryption key.
  //
  VarEncInfo->Header.Attributes &= (~EFI_VARIABLE_APPEND_WRITE);
  if (!EncVarLibGenEncKey (VarEncInfo, ENC_KEY_SIZE, EncKey)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  if (!EncVarLibGenIvec (Ivec, ENC_IVEC_SIZE)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  AesContext = AllocateZeroPool (AesGetContextSize ());
  if (AesContext == NULL) {
    ASSERT (AesContext != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  if (!AesInit (AesContext, EncKey, ENC_KEY_SIZE * 8)) {
    ASSERT (FALSE);
    goto Done;
  }

  if (AesCbcEncrypt (
        AesContext,
        PlainBuffer,
        PlainDataSize + PaddingBytes,
        Ivec,
        CipherBuffer + sizeof (VARIABLE_ENCRYPTION_HEADER)
        ))
  {
    //
    // Keep the IV for decryption.
    //
    CopyMem (CipherData->KeyIvec, Ivec, ENC_BLOCK_SIZE);

    if ((UINTN)CipherBuffer != (UINTN)CipherData) {
      CopyMem (
        CipherData + 1,
        CipherBuffer + sizeof (VARIABLE_ENCRYPTION_HEADER),
        CipherDataSize
        );
    }

    CipherData->CipherDataSize = CipherDataSize;
    CipherData->PlainDataSize  = PlainDataSize;
    CipherData->DataType       = ENC_TYPE_AES;
    CipherData->HeaderSize     = sizeof (VARIABLE_ENCRYPTION_HEADER);

    VarEncInfo->CipherData       = CipherData;
    VarEncInfo->CipherDataSize   = CipherDataSize + sizeof (VARIABLE_ENCRYPTION_HEADER);
    VarEncInfo->CipherHeaderSize = sizeof (VARIABLE_ENCRYPTION_HEADER);
    VarEncInfo->CipherDataType   = ENC_TYPE_AES;

    Status = EFI_SUCCESS;
  } else {
    VarEncInfo->CipherData       = NULL;
    VarEncInfo->CipherDataSize   = 0;
    VarEncInfo->CipherHeaderSize = 0;
    VarEncInfo->CipherDataType   = ENC_TYPE_NULL;

    ASSERT (FALSE);
  }

Done:
  FREE_POOL (AesContext);
  if (PlainBuffer != PlainData) {
    FREE_POOL (PlainBuffer);
  }

  if (CipherBuffer != (UINT8 *)CipherData) {
    FREE_POOL (CipherBuffer);
  }

  return Status;
}

/**
  Decrypt variable data.

  If VarEncInfo->CipherData is not NULL, it must holds the cipher data to be
  decrypted. Otherwise, assume the cipher data from variable data buffer, i.e.
  VarEncInfo->Header.Data.

  If VarEncInfo->Flags.DecryptInPlace is TRUE, the decrypted data will be put
  back in the same buffer as cipher buffer got above, after encryption header,
  which helps to identify later if the data in buffer is decrypted or not. This
  can avoid repeat decryption when accessing the same variable more than once.

  If VarEncInfo->Flags.DecryptInPlace is FALSE, VarEncInfo->PlainData must be
  passed in with a valid buffer with VarEncInfo->PlainDataSize set correctly
  with its size.

  Note the VarEncInfo->PlainData is always pointing to the buffer address with
  decrypted data without encryption header, and VarEncInfo->PlainDataSize is
  always the size of original variable data, if this function returned
  successfully.

  @param[in, out]   VarEncInfo   Pointer to structure containing detailed
                                 information about a variable.

  @retval EFI_SUCCESS             Variable was decrypted successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_BUFFER_TOO_SMALL    VarEncInfo->PlainData is not NULL but
                                  VarEncInfo->PlainDataSize is too small.
  @retval EFI_ABORTED             Uknown error occurred during decrypting.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate enough resource.
  @retval EFI_COMPROMISED_DATA    The cipher header is not valid.
  @retval EFI_UNSUPPORTED         Unsupported to encrypt variable.

**/
EFI_STATUS
EFIAPI
DecryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  VOID                        *AesContext;
  UINT8                       EncKey[ENC_KEY_SIZE];
  UINT8                       *PlainBuffer;
  UINT8                       *PlainData;
  VARIABLE_ENCRYPTION_HEADER  *CipherData;
  UINT32                      CipherDataSize;
  EFI_STATUS                  Status;

  Status      = EFI_ABORTED;
  AesContext  = NULL;
  PlainBuffer = NULL;

  if (!IsValidVariableInfo (VarEncInfo, FALSE)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VarEncInfo->CipherData != NULL) {
    CipherData     = VarEncInfo->CipherData;
    CipherDataSize = VarEncInfo->CipherDataSize;
  } else {
    CipherData     = VarEncInfo->Header.Data;
    CipherDataSize = (UINT32)VarEncInfo->Header.DataSize;
  }

  //
  // Sanity check of cipher header.
  //
  if (!IsValidEncrptionHeader (CipherData, CipherDataSize)) {
    return EFI_COMPROMISED_DATA;
  }

  if (  (VarEncInfo->PlainData != NULL)
     && (VarEncInfo->PlainDataSize < CipherData->PlainDataSize))
  {
    VarEncInfo->PlainDataSize = CipherData->PlainDataSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (CipherData->DataType == ENC_TYPE_AES) {
    if (VarEncInfo->Flags.DecryptInPlace) {
      //
      // Reusing cipher data buffer needs to keep the encryption header.
      //
      PlainData = (UINT8 *)CipherData + CipherData->HeaderSize;
    } else {
      PlainData = VarEncInfo->PlainData;
    }

    if (PlainData == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Always need buffer to store the decrypted data temporarily, due to
    // padding bytes or buffer reuse. Then the buffer must be larger than
    // CipherData->PlainDataSize.
    //
    PlainBuffer = AllocatePages (EFI_SIZE_TO_PAGES (CipherDataSize));
    if (PlainBuffer == NULL) {
      ASSERT (PlainBuffer != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    if (!EncVarLibGenEncKey (VarEncInfo, ENC_KEY_SIZE, EncKey)) {
      ASSERT (FALSE);
      goto Done;
    }

    AesContext = AllocatePages (EFI_SIZE_TO_PAGES (AesGetContextSize ()));
    if (AesContext == NULL) {
      ASSERT (AesContext != NULL);
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    if (!AesInit (AesContext, EncKey, ENC_KEY_SIZE * 8)) {
      ASSERT (FALSE);
      goto Done;
    }

    if (AesCbcDecrypt (
          AesContext,
          (UINT8 *)CipherData + CipherData->HeaderSize,
          CipherDataSize - CipherData->HeaderSize,
          CipherData->KeyIvec,
          PlainBuffer
          ))
    {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_COMPROMISED_DATA;
    }
  } else {
    //
    // The data has been decrypted already.
    //
    PlainBuffer = (UINT8 *)CipherData + CipherData->HeaderSize;

    if (VarEncInfo->PlainData != NULL) {
      PlainData = VarEncInfo->PlainData;
    } else {
      PlainData = PlainBuffer;
    }

    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    if (PlainBuffer != PlainData) {
      CopyMem (PlainData, PlainBuffer, CipherData->PlainDataSize);
    }

    if (VarEncInfo->PlainData != NULL) {
      if (VarEncInfo->PlainData != PlainBuffer) {
        CopyMem (VarEncInfo->PlainData, PlainBuffer, CipherData->PlainDataSize);
      }
    } else {
      VarEncInfo->PlainData = PlainData;
    }

    VarEncInfo->PlainDataSize    = CipherData->PlainDataSize;
    VarEncInfo->CipherHeaderSize = CipherData->HeaderSize;
    VarEncInfo->CipherDataType   = CipherData->DataType;

    if (VarEncInfo->Flags.DecryptInPlace) {
      CipherData->DataType = ENC_TYPE_NULL;
    }
  }

Done:
  FreePages (PlainBuffer, EFI_SIZE_TO_PAGES ((CipherDataSize)));
  FreePages (AesContext, EFI_SIZE_TO_PAGES (AesGetContextSize ()));
  return Status;
}

/**
  Get cipher information about a variable, including plaindata size,
  cipher algorithm type, etc.

  For data passed in with VarEncInfo,

    VarEncInfo->Header.Data
      - The variable data in normal variable structure.
    VarEncInfo->Header.DataSize
      - The size of variable data.

  For data passed out with VarEncInfo (valid only if EFI_SUCCESS is returned),

    VarEncInfo->CipherDataType
      - ENC_TYPE_NULL, if the variable is not encrypted or has been decrypted;
      - ENC_TYPE_AES, if the variable is encrypted.
    VarEncInfo->CipherHeaderSize
      - Size of cipher header put before encrypted or decrypted data.
    VarEncInfo->PlainData
      - NULL, if the variable is encrypted; Or
      - pointer to original variable data, if the variable has been decrypted.
    VarEncInfo->PlainDataSize
      - The size of original variable data
    VarEncInfo->CipherData
      - NULL, if the variable is decrypted; Or
      - pointer to start of encrypted variable data, including encryption header;
    VarEncInfo->CipherDataSize
      - The size of encrypted variable data, including encryption header.

  @param[in]   VarEncInfo   Pointer to structure containing detailed
                            information about a variable.

  @retval EFI_SUCCESS             The information was retrieved successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_NOT_FOUND           No cipher information recognized.
  @retval EFI_UNSUPPORTED         Unsupported interface.

**/
EFI_STATUS
EFIAPI
GetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  VARIABLE_ENCRYPTION_HEADER  *EncHeader;

  if ((VarEncInfo->Header.Data == NULL) || (VarEncInfo->Header.DataSize == 0)) {
    ASSERT (VarEncInfo->Header.Data != NULL);
    ASSERT (VarEncInfo->Header.DataSize != 0);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate encryption header.
  //
  EncHeader = (VARIABLE_ENCRYPTION_HEADER *)VarEncInfo->Header.Data;
  if (!IsValidEncrptionHeader (EncHeader, (UINT32)VarEncInfo->Header.DataSize)) {
    //
    // Not an encrypted variable.
    //
    return EFI_NOT_FOUND;
  }

  if (EncHeader->DataType == ENC_TYPE_NULL) {
    //
    // The data must have been decrypted.
    //
    VarEncInfo->PlainData            = (UINT8 *)VarEncInfo->Header.Data + EncHeader->HeaderSize;
    VarEncInfo->CipherData           = NULL;
    VarEncInfo->Flags.DecryptInPlace = TRUE;
  } else {
    //
    // The data is encrypted.
    //
    VarEncInfo->CipherData           = VarEncInfo->Header.Data;
    VarEncInfo->PlainData            = NULL;
    VarEncInfo->Flags.DecryptInPlace = FALSE;
  }

  VarEncInfo->PlainDataSize    = EncHeader->PlainDataSize;
  VarEncInfo->CipherDataSize   = EncHeader->CipherDataSize + EncHeader->HeaderSize;
  VarEncInfo->CipherDataType   = EncHeader->DataType;
  VarEncInfo->CipherHeaderSize = EncHeader->HeaderSize;

  return EFI_SUCCESS;
}

/**
  Force set cipher information for a variable, like plaindata size,
  cipher algorithm type, cipher data etc.

  The destination buffer must be passed via VarEncInfo->Header.Data.

  This method is only used to update and/or change plain data information.

  @param[in]   VarEncInfo   Pointer to structure containing detailed
                            information about a variable.

  @retval EFI_SUCCESS             The information was updated successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_UNSUPPORTED         If this method is not supported.

**/
EFI_STATUS
EFIAPI
SetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  VARIABLE_ENCRYPTION_HEADER  *EncHeader;
  UINT8                       *Data;

  if (  (VarEncInfo->Header.Data == NULL)
     || (VarEncInfo->Header.DataSize < sizeof (VARIABLE_ENCRYPTION_HEADER))
     || (VarEncInfo->CipherDataType != ENC_TYPE_NULL))
  {
    ASSERT (VarEncInfo->Header.Data != NULL);
    ASSERT (VarEncInfo->Header.DataSize >= sizeof (VARIABLE_ENCRYPTION_HEADER));
    ASSERT (VarEncInfo->CipherDataType == ENC_TYPE_NULL);
    return EFI_INVALID_PARAMETER;
  }

  Data      = VarEncInfo->Header.Data;
  EncHeader = (VARIABLE_ENCRYPTION_HEADER *)Data;

  if (  !IsValidEncrptionHeader (EncHeader, (UINT32)VarEncInfo->Header.DataSize)
     || (VarEncInfo->PlainDataSize > EncHeader->CipherDataSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((VarEncInfo->PlainData != NULL) && (VarEncInfo->PlainDataSize > 0)) {
    CopyMem (
      Data + EncHeader->HeaderSize,
      VarEncInfo->PlainData,
      VarEncInfo->PlainDataSize
      );
  }

  EncHeader->DataType = VarEncInfo->CipherDataType;
  if (VarEncInfo->PlainDataSize != 0) {
    EncHeader->PlainDataSize = VarEncInfo->PlainDataSize;
  }

  return EFI_SUCCESS;
}
