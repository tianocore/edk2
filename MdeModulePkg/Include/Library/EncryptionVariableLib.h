/** @file
  Provides services to encrypt/decrypt variables.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ENCRYPTION_VARIABLE_LIB_H_
#define ENCRYPTION_VARIABLE_LIB_H_

#include <IndustryStandard/Tpm20.h>

#include <Guid/VariableFormat.h>

#include <Library/AuthVariableLib.h>

#define ENC_TYPE_NULL  0
#define ENC_TYPE_AES   TPM_ALG_AES

typedef struct  _VARIABLE_ENCRYPTION_FLAGS {
  BOOLEAN    Auth;            // Variable is authenticated or not
  BOOLEAN    DecryptInPlace;  // Do decryption in place
  BOOLEAN    Protected;       // Variable is protected or not
} VARIABLE_ENCRYPTION_FLAGS;

typedef struct _VARIABLE_ENCRYPTION_INFO {
  AUTH_VARIABLE_INFO           Header;            // Authenticated varabile header
  VARIABLE_HEADER              *Buffer;           // Pointer to variable buffer
  UINT64                       StoreIndex;        // Variable store index
  VOID                         *PlainData;        // Pointer to plain data
  UINT32                       PlainDataSize;     // Size of plain data
  VOID                         *CipherData;       // Pointer to cipher data
  UINT32                       CipherDataSize;    // Size of cipher data
  UINT32                       CipherHeaderSize;  // Size of cipher header
  UINT32                       CipherDataType;    // Type of cipher data
  VOID                         *Key;              // Pointer to encrypt/decrypt key
  UINT32                       KeySize;           // Size of key
  VARIABLE_ENCRYPTION_FLAGS    Flags;             // Encryption flags
} VARIABLE_ENCRYPTION_INFO;

/**
  Encrypt variable data.

  @param[in, out]   VarInfo   Pointer to structure containing detailed information about a variable.

  @retval EFI_SUCCESS               Function successfully executed.
  @retval EFI_INVALID_PARAMETER     If ProtectedVarLibContextIn == NULL or ProtectedVarLibContextOut == NULL.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
EncryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarInfo
  );

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

  @param[in, out]   VarInfo   Pointer to structure containing detailed
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
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarInfo
  );

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

  @param[in, out]   VarInfo   Pointer to structure containing detailed
                              information about a variable.

  @retval EFI_SUCCESS             The information was retrieved successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_NOT_FOUND           No cipher information recognized.
  @retval EFI_UNSUPPORTED         Unsupported interface.

**/
EFI_STATUS
EFIAPI
GetCipherDataInfo (
  IN  OUT VARIABLE_ENCRYPTION_INFO  *VarInfo
  );

/**
  Force set cipher information for a variable, like plaindata size,
  cipher algorithm type, cipher data etc.

  The destination buffer must be passed via VarEncInfo->Header.Data.

  This method is only used to update and/or change plain data information.

  @param[in, out]   VarInfo   Pointer to structure containing detailed
                              information about a variable.

  @retval EFI_SUCCESS             The information was updated successfully.
  @retval EFI_INVALID_PARAMETER   Variable information in VarEncInfo is invalid.
  @retval EFI_UNSUPPORTED         If this method is not supported.

**/
EFI_STATUS
EFIAPI
SetCipherDataInfo (
  IN  OUT VARIABLE_ENCRYPTION_INFO  *VarInfo
  );

#endif //_ENCRYPTION_VARIABLE_LIB_H_
