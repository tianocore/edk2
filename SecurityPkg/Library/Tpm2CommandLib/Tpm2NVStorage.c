/** @file
  Implement TPM2 NVStorage related command.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#pragma pack(1)

#define RC_NV_ReadPublic_nvIndex            (TPM_RC_H + TPM_RC_1)

#define RC_NV_DefineSpace_authHandle        (TPM_RC_H + TPM_RC_1)
#define RC_NV_DefineSpace_auth              (TPM_RC_P + TPM_RC_1)
#define RC_NV_DefineSpace_publicInfo        (TPM_RC_P + TPM_RC_2)

#define RC_NV_UndefineSpace_authHandle      (TPM_RC_H + TPM_RC_1)
#define RC_NV_UndefineSpace_nvIndex         (TPM_RC_H + TPM_RC_2)

#define RC_NV_Read_authHandle               (TPM_RC_H + TPM_RC_1)
#define RC_NV_Read_nvIndex                  (TPM_RC_H + TPM_RC_2)
#define RC_NV_Read_size                     (TPM_RC_P + TPM_RC_1)
#define RC_NV_Read_offset                   (TPM_RC_P + TPM_RC_2)

#define RC_NV_Write_authHandle              (TPM_RC_H + TPM_RC_1)
#define RC_NV_Write_nvIndex                 (TPM_RC_H + TPM_RC_2)
#define RC_NV_Write_data                    (TPM_RC_P + TPM_RC_1)
#define RC_NV_Write_offset                  (TPM_RC_P + TPM_RC_2)

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_NV_INDEX          NvIndex;
} TPM2_NV_READPUBLIC_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER      Header;
  TPM2B_NV_PUBLIC           NvPublic;
  TPM2B_NAME                NvName;
} TPM2_NV_READPUBLIC_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_PROVISION         AuthHandle;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
  TPM2B_AUTH                Auth;
  TPM2B_NV_PUBLIC           NvPublic;
} TPM2_NV_DEFINESPACE_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_DEFINESPACE_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_PROVISION         AuthHandle;
  TPMI_RH_NV_INDEX          NvIndex;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
} TPM2_NV_UNDEFINESPACE_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_UNDEFINESPACE_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_NV_AUTH           AuthHandle;
  TPMI_RH_NV_INDEX          NvIndex;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
  UINT16                    Size;
  UINT16                    Offset;
} TPM2_NV_READ_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPM2B_MAX_BUFFER           Data;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_READ_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_NV_AUTH           AuthHandle;
  TPMI_RH_NV_INDEX          NvIndex;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
  TPM2B_MAX_BUFFER          Data;
  UINT16                    Offset;
} TPM2_NV_WRITE_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_WRITE_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_NV_AUTH           AuthHandle;
  TPMI_RH_NV_INDEX          NvIndex;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
} TPM2_NV_READLOCK_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_READLOCK_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_NV_AUTH           AuthHandle;
  TPMI_RH_NV_INDEX          NvIndex;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
} TPM2_NV_WRITELOCK_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_WRITELOCK_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_PROVISION         AuthHandle;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
} TPM2_NV_GLOBALWRITELOCK_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_NV_GLOBALWRITELOCK_RESPONSE;

#pragma pack()

/**
  This command is used to read the public area and Name of an NV Index.

  @param[in]  NvIndex            The NV Index.
  @param[out] NvPublic           The public area of the index.
  @param[out] NvName             The Name of the nvIndex.
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvReadPublic (
  IN      TPMI_RH_NV_INDEX          NvIndex,
  OUT     TPM2B_NV_PUBLIC           *NvPublic,
  OUT     TPM2B_NAME                *NvName
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_READPUBLIC_COMMAND        SendBuffer;
  TPM2_NV_READPUBLIC_RESPONSE       RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT16                            NvPublicSize;
  UINT16                            NvNameSize;
  UINT8                             *Buffer;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_ReadPublic);

  SendBuffer.NvIndex = SwapBytes32 (NvIndex);
 
  SendBufferSize = (UINT32) sizeof (SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadPublic - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }
  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadPublic - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  case TPM_RC_HANDLE + RC_NV_ReadPublic_nvIndex: // TPM_RC_NV_DEFINED:
    return EFI_NOT_FOUND;
  case TPM_RC_VALUE + RC_NV_ReadPublic_nvIndex:
    return EFI_INVALID_PARAMETER;
  default:
    return EFI_DEVICE_ERROR;
  }

  if (RecvBufferSize <= sizeof (TPM2_RESPONSE_HEADER) + sizeof (UINT16) + sizeof(UINT16)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadPublic - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_NOT_FOUND;
  }

  //
  // Basic check
  //
  NvPublicSize = SwapBytes16 (RecvBuffer.NvPublic.size);
  NvNameSize = SwapBytes16 (ReadUnaligned16 ((UINT16 *)((UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + NvPublicSize)));

  if (RecvBufferSize != sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + NvPublicSize + sizeof(UINT16) + NvNameSize) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadPublic - RecvBufferSize Error - NvPublicSize %x, NvNameSize %x\n", RecvBufferSize, NvNameSize));
    return EFI_NOT_FOUND;
  }

  //
  // Return the response
  //
  CopyMem (NvPublic, &RecvBuffer.NvPublic, sizeof(UINT16) + NvPublicSize);
  NvPublic->size = NvPublicSize;
  NvPublic->nvPublic.nvIndex = SwapBytes32 (NvPublic->nvPublic.nvIndex);
  NvPublic->nvPublic.nameAlg = SwapBytes16 (NvPublic->nvPublic.nameAlg);
  WriteUnaligned32 ((UINT32 *)&NvPublic->nvPublic.attributes, SwapBytes32 (ReadUnaligned32 ((UINT32 *)&NvPublic->nvPublic.attributes)));
  NvPublic->nvPublic.authPolicy.size = SwapBytes16 (NvPublic->nvPublic.authPolicy.size);
  Buffer = (UINT8 *)&NvPublic->nvPublic.authPolicy;
  Buffer += sizeof(UINT16) + NvPublic->nvPublic.authPolicy.size;
  NvPublic->nvPublic.dataSize = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));

  CopyMem (NvName, (UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + NvPublicSize, NvNameSize);
  NvName->size = NvNameSize;
  
  return EFI_SUCCESS;
}

/**
  This command defines the attributes of an NV Index and causes the TPM to
  reserve space to hold the data associated with the index.
  If a definition already exists at the index, the TPM will return TPM_RC_NV_DEFINED.

  @param[in]  AuthHandle         TPM_RH_OWNER or TPM_RH_PLATFORM+{PP}.
  @param[in]  AuthSession        Auth Session context
  @param[in]  Auth               The authorization data.
  @param[in]  NvPublic           The public area of the index.
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_ALREADY_STARTED    The command was returned successfully, but NvIndex is already defined.
**/
EFI_STATUS
EFIAPI
Tpm2NvDefineSpace (
  IN      TPMI_RH_PROVISION         AuthHandle,
  IN      TPMS_AUTH_COMMAND         *AuthSession, OPTIONAL
  IN      TPM2B_AUTH                *Auth,
  IN      TPM2B_NV_PUBLIC           *NvPublic
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_DEFINESPACE_COMMAND       SendBuffer;
  TPM2_NV_DEFINESPACE_RESPONSE      RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT16                            NvPublicSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_DefineSpace);
  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  //
  // IndexAuth
  //
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16(Auth->size));
  Buffer += sizeof(UINT16);
  CopyMem(Buffer, Auth->buffer, Auth->size);
  Buffer += Auth->size;

  //
  // NvPublic
  //
  NvPublicSize = NvPublic->size;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NvPublicSize));
  Buffer += sizeof(UINT16);
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (NvPublic->nvPublic.nvIndex));
  Buffer += sizeof(UINT32);
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NvPublic->nvPublic.nameAlg));
  Buffer += sizeof(UINT16);
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (ReadUnaligned32 ((UINT32 *)&NvPublic->nvPublic.attributes)));
  Buffer += sizeof(UINT32);
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NvPublic->nvPublic.authPolicy.size));
  Buffer += sizeof(UINT16);
  CopyMem (Buffer, NvPublic->nvPublic.authPolicy.buffer, NvPublic->nvPublic.authPolicy.size);
  Buffer += NvPublic->nvPublic.authPolicy.size;
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NvPublic->nvPublic.dataSize));
  Buffer += sizeof(UINT16);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvDefineSpace - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvDefineSpace - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  case TPM_RC_SIZE + RC_NV_DefineSpace_publicInfo:
  case TPM_RC_SIZE + RC_NV_DefineSpace_auth:
    return EFI_BAD_BUFFER_SIZE;
  case TPM_RC_ATTRIBUTES:
  case TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo:
    return EFI_UNSUPPORTED;
  case TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_authHandle:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_NV_DEFINED:
    return EFI_ALREADY_STARTED;
  case TPM_RC_VALUE + RC_NV_DefineSpace_publicInfo:
  case TPM_RC_VALUE + RC_NV_DefineSpace_authHandle:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_NV_SPACE:
    return EFI_OUT_OF_RESOURCES;
  default:
    return EFI_DEVICE_ERROR;
  }
  
  return EFI_SUCCESS;
}

/**
  This command removes an index from the TPM.

  @param[in]  AuthHandle         TPM_RH_OWNER or TPM_RH_PLATFORM+{PP}.
  @param[in]  NvIndex            The NV Index.
  @param[in]  AuthSession        Auth Session context
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvUndefineSpace (
  IN      TPMI_RH_PROVISION         AuthHandle,
  IN      TPMI_RH_NV_INDEX          NvIndex,
  IN      TPMS_AUTH_COMMAND         *AuthSession OPTIONAL
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_UNDEFINESPACE_COMMAND     SendBuffer;
  TPM2_NV_UNDEFINESPACE_RESPONSE    RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_UndefineSpace);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);
  SendBuffer.NvIndex = SwapBytes32 (NvIndex);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvUndefineSpace - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvUndefineSpace - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  case TPM_RC_ATTRIBUTES:
  case TPM_RC_ATTRIBUTES + RC_NV_UndefineSpace_nvIndex:
    return EFI_UNSUPPORTED;
  case TPM_RC_NV_AUTHORIZATION:
    return EFI_SECURITY_VIOLATION;
  case TPM_RC_HANDLE + RC_NV_UndefineSpace_nvIndex: // TPM_RC_NV_DEFINED:
    return EFI_NOT_FOUND;
  case TPM_RC_HANDLE + RC_NV_UndefineSpace_authHandle: // TPM_RC_NV_DEFINED:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_VALUE + RC_NV_UndefineSpace_authHandle:
  case TPM_RC_VALUE + RC_NV_UndefineSpace_nvIndex:
    return EFI_INVALID_PARAMETER;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This command reads a value from an area in NV memory previously defined by TPM2_NV_DefineSpace().

  @param[in]     AuthHandle         the handle indicating the source of the authorization value.
  @param[in]     NvIndex            The index to be read.
  @param[in]     AuthSession        Auth Session context
  @param[in]     Size               Number of bytes to read.
  @param[in]     Offset             Byte offset into the area.
  @param[in,out] OutData            The data read.
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvRead (
  IN      TPMI_RH_NV_AUTH           AuthHandle,
  IN      TPMI_RH_NV_INDEX          NvIndex,
  IN      TPMS_AUTH_COMMAND         *AuthSession, OPTIONAL
  IN      UINT16                    Size,
  IN      UINT16                    Offset,
  IN OUT  TPM2B_MAX_BUFFER          *OutData
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_READ_COMMAND              SendBuffer;
  TPM2_NV_READ_RESPONSE             RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_Read);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);
  SendBuffer.NvIndex = SwapBytes32 (NvIndex);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Size));
  Buffer += sizeof(UINT16);
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Offset));
  Buffer += sizeof(UINT16);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvRead - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }
  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvRead - responseCode - %x\n", ResponseCode));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  case TPM_RC_NV_AUTHORIZATION:
    return EFI_SECURITY_VIOLATION;
  case TPM_RC_NV_LOCKED:
    return EFI_ACCESS_DENIED;
  case TPM_RC_NV_RANGE:
    return EFI_BAD_BUFFER_SIZE;
  case TPM_RC_NV_UNINITIALIZED:
    return EFI_NOT_READY;
  case TPM_RC_HANDLE + RC_NV_Read_nvIndex: // TPM_RC_NV_DEFINED:
    return EFI_NOT_FOUND;
  case TPM_RC_HANDLE + RC_NV_Read_authHandle: // TPM_RC_NV_DEFINED:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_VALUE + RC_NV_Read_nvIndex:
  case TPM_RC_VALUE + RC_NV_Read_authHandle:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_BAD_AUTH + RC_NV_Read_authHandle + TPM_RC_S:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_AUTH_UNAVAILABLE:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_AUTH_FAIL + RC_NV_Read_authHandle + TPM_RC_S:
    return EFI_INVALID_PARAMETER;
  default:
    return EFI_DEVICE_ERROR;
  case TPM_RC_ATTRIBUTES + RC_NV_Read_authHandle + TPM_RC_S:
    return EFI_UNSUPPORTED;
  }

  //
  // Return the response
  //
  OutData->size = SwapBytes16 (RecvBuffer.Data.size);
  CopyMem (OutData->buffer, &RecvBuffer.Data.buffer, OutData->size);
  
  return EFI_SUCCESS;
}

/**
  This command writes a value to an area in NV memory that was previously defined by TPM2_NV_DefineSpace().

  @param[in]  AuthHandle         the handle indicating the source of the authorization value.
  @param[in]  NvIndex            The NV Index of the area to write.
  @param[in]  AuthSession        Auth Session context
  @param[in]  InData             The data to write.
  @param[in]  Offset             The offset into the NV Area.
  
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvWrite (
  IN      TPMI_RH_NV_AUTH           AuthHandle,
  IN      TPMI_RH_NV_INDEX          NvIndex,
  IN      TPMS_AUTH_COMMAND         *AuthSession, OPTIONAL
  IN      TPM2B_MAX_BUFFER          *InData,
  IN      UINT16                    Offset
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_WRITE_COMMAND             SendBuffer;
  TPM2_NV_WRITE_RESPONSE            RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_Write);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);
  SendBuffer.NvIndex = SwapBytes32 (NvIndex);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (InData->size));
  Buffer += sizeof(UINT16);
  CopyMem (Buffer, InData->buffer, InData->size);
  Buffer += InData->size;
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Offset));
  Buffer += sizeof(UINT16);

  SendBufferSize = (UINT32) (Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvWrite - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }
  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvWrite - responseCode - %x\n", ResponseCode));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    return EFI_SUCCESS;
  case TPM_RC_ATTRIBUTES:
    return EFI_UNSUPPORTED;
  case TPM_RC_NV_AUTHORIZATION:
    return EFI_SECURITY_VIOLATION;
  case TPM_RC_NV_LOCKED:
    return EFI_ACCESS_DENIED;
  case TPM_RC_NV_RANGE:
    return EFI_BAD_BUFFER_SIZE;
  case TPM_RC_HANDLE + RC_NV_Write_nvIndex: // TPM_RC_NV_DEFINED:
    return EFI_NOT_FOUND;
  case TPM_RC_HANDLE + RC_NV_Write_authHandle: // TPM_RC_NV_DEFINED:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_VALUE + RC_NV_Write_nvIndex:
  case TPM_RC_VALUE + RC_NV_Write_authHandle:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_BAD_AUTH + RC_NV_Write_authHandle + TPM_RC_S:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_AUTH_UNAVAILABLE:
    return EFI_INVALID_PARAMETER;
  case TPM_RC_AUTH_FAIL + RC_NV_Write_authHandle + TPM_RC_S:
    return EFI_INVALID_PARAMETER;
  default:
    return EFI_DEVICE_ERROR;
  case TPM_RC_ATTRIBUTES + RC_NV_Write_authHandle + TPM_RC_S:
    return EFI_UNSUPPORTED;
  }
}

/**
  This command may be used to prevent further reads of the Index until the next TPM2_Startup (TPM_SU_CLEAR).

  @param[in]  AuthHandle         the handle indicating the source of the authorization value.
  @param[in]  NvIndex            The NV Index of the area to lock.
  @param[in]  AuthSession        Auth Session context

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvReadLock (
  IN      TPMI_RH_NV_AUTH           AuthHandle,
  IN      TPMI_RH_NV_INDEX          NvIndex,
  IN      TPMS_AUTH_COMMAND         *AuthSession OPTIONAL
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_READLOCK_COMMAND          SendBuffer;
  TPM2_NV_READLOCK_RESPONSE         RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_ReadLock);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);
  SendBuffer.NvIndex = SwapBytes32 (NvIndex);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadLock - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvReadLock - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This command may be used to inhibit further writes of the Index.

  @param[in]  AuthHandle         the handle indicating the source of the authorization value.
  @param[in]  NvIndex            The NV Index of the area to lock.
  @param[in]  AuthSession        Auth Session context

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvWriteLock (
  IN      TPMI_RH_NV_AUTH           AuthHandle,
  IN      TPMI_RH_NV_INDEX          NvIndex,
  IN      TPMS_AUTH_COMMAND         *AuthSession OPTIONAL
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_WRITELOCK_COMMAND         SendBuffer;
  TPM2_NV_WRITELOCK_RESPONSE        RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_WriteLock);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);
  SendBuffer.NvIndex = SwapBytes32 (NvIndex);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvWriteLock - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvWriteLock - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  The command will SET TPMA_NV_WRITELOCKED for all indexes that have their TPMA_NV_GLOBALLOCK attribute SET.

  @param[in]  AuthHandle         TPM_RH_OWNER or TPM_RH_PLATFORM+{PP}.
  @param[in]  AuthSession        Auth Session context

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2NvGlobalWriteLock (
  IN      TPMI_RH_PROVISION         AuthHandle,
  IN      TPMS_AUTH_COMMAND         *AuthSession OPTIONAL
  )
{
  EFI_STATUS                        Status;
  TPM2_NV_GLOBALWRITELOCK_COMMAND   SendBuffer;
  TPM2_NV_GLOBALWRITELOCK_RESPONSE  RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;
  UINT32                            SessionInfoSize;
  TPM_RC                            ResponseCode;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_NV_GlobalWriteLock);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  SendBufferSize = (UINT32)(Buffer - (UINT8 *)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvGlobalWriteLock - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2NvGlobalWriteLock - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
