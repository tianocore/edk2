/** @file
  Implement TPM1.2 NV storage related command.

Copyright (c) 2015, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <IndustryStandard/Tpm12.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/DebugLib.h>

//
// Max TPM command/reponse length
//
#define TPMCMDBUFLENGTH             1024

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM12_NV_DATA_PUBLIC  PubInfo;
  TPM_ENCAUTH           EncAuth;
} TPM_CMD_NV_DEFINE_SPACE;

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
} TPM_RSP_NV_DEFINE_SPACE;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_NV_INDEX          NvIndex;
  UINT32                Offset;
  UINT32                DataSize;
} TPM_CMD_NV_READ_VALUE;

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
  UINT32                DataSize;
  UINT8                 Data[TPMCMDBUFLENGTH];
} TPM_RSP_NV_READ_VALUE;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_NV_INDEX          NvIndex;
  UINT32                Offset;
  UINT32                DataSize;
  UINT8                 Data[TPMCMDBUFLENGTH];
} TPM_CMD_NV_WRITE_VALUE;

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
} TPM_RSP_NV_WRITE_VALUE;

#pragma pack()

/**
  Send NV DefineSpace command to TPM1.2.
  
  @param PubInfo           The public parameters of the NV area.
  @param EncAuth           The encrypted AuthData, only valid if the attributes require subsequent authorization.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvDefineSpace (
  IN TPM12_NV_DATA_PUBLIC  *PubInfo,
  IN TPM_ENCAUTH           *EncAuth
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_NV_DEFINE_SPACE           SendBuffer;
  TPM_RSP_NV_DEFINE_SPACE           RecvBuffer;
  UINT32                            ReturnCode;

  //
  // send Tpm command TPM_ORD_NV_DefineSpace
  //
  TpmRecvSize                = sizeof (TPM_RSP_NV_DEFINE_SPACE);
  TpmSendSize                = sizeof (TPM_CMD_NV_DEFINE_SPACE);
  SendBuffer.Hdr.tag         = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize   = SwapBytes32 (sizeof(TPM_CMD_NV_DEFINE_SPACE));
  SendBuffer.Hdr.ordinal     = SwapBytes32 (TPM_ORD_NV_DefineSpace);
  SendBuffer.PubInfo.tag     = SwapBytes16 (PubInfo->tag);
  SendBuffer.PubInfo.nvIndex = SwapBytes32 (PubInfo->nvIndex);
  SendBuffer.PubInfo.pcrInfoRead.pcrSelection.sizeOfSelect  = SwapBytes16 (PubInfo->pcrInfoRead.pcrSelection.sizeOfSelect);
  SendBuffer.PubInfo.pcrInfoRead.pcrSelection.pcrSelect[0]  = PubInfo->pcrInfoRead.pcrSelection.pcrSelect[0];
  SendBuffer.PubInfo.pcrInfoRead.pcrSelection.pcrSelect[1]  = PubInfo->pcrInfoRead.pcrSelection.pcrSelect[1];
  SendBuffer.PubInfo.pcrInfoRead.pcrSelection.pcrSelect[2]  = PubInfo->pcrInfoRead.pcrSelection.pcrSelect[2];
  SendBuffer.PubInfo.pcrInfoRead.localityAtRelease          = PubInfo->pcrInfoRead.localityAtRelease;
  CopyMem (&SendBuffer.PubInfo.pcrInfoRead.digestAtRelease, &PubInfo->pcrInfoRead.digestAtRelease, sizeof(PubInfo->pcrInfoRead.digestAtRelease));
  SendBuffer.PubInfo.pcrInfoWrite.pcrSelection.sizeOfSelect = SwapBytes16 (PubInfo->pcrInfoWrite.pcrSelection.sizeOfSelect);
  SendBuffer.PubInfo.pcrInfoWrite.pcrSelection.pcrSelect[0] = PubInfo->pcrInfoWrite.pcrSelection.pcrSelect[0];
  SendBuffer.PubInfo.pcrInfoWrite.pcrSelection.pcrSelect[1] = PubInfo->pcrInfoWrite.pcrSelection.pcrSelect[1];
  SendBuffer.PubInfo.pcrInfoWrite.pcrSelection.pcrSelect[2] = PubInfo->pcrInfoWrite.pcrSelection.pcrSelect[2];
  SendBuffer.PubInfo.pcrInfoWrite.localityAtRelease         = PubInfo->pcrInfoWrite.localityAtRelease;
  CopyMem (&SendBuffer.PubInfo.pcrInfoWrite.digestAtRelease, &PubInfo->pcrInfoWrite.digestAtRelease, sizeof(PubInfo->pcrInfoWrite.digestAtRelease));
  SendBuffer.PubInfo.permission.tag        = SwapBytes16 (PubInfo->permission.tag);
  SendBuffer.PubInfo.permission.attributes = SwapBytes32 (PubInfo->permission.attributes);
  SendBuffer.PubInfo.bReadSTClear          = PubInfo->bReadSTClear;
  SendBuffer.PubInfo.bWriteSTClear         = PubInfo->bWriteSTClear;
  SendBuffer.PubInfo.bWriteDefine          = PubInfo->bWriteDefine;
  SendBuffer.PubInfo.dataSize              = SwapBytes32 (PubInfo->dataSize);
  CopyMem (&SendBuffer.EncAuth, EncAuth, sizeof(*EncAuth));

  Status = Tpm12SubmitCommand (TpmSendSize, (UINT8 *)&SendBuffer, &TpmRecvSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReturnCode = SwapBytes32(RecvBuffer.Hdr.returnCode);
  DEBUG ((DEBUG_INFO, "Tpm12NvDefineSpace - ReturnCode = %x\n", ReturnCode));
  switch (ReturnCode) {
  case TPM_SUCCESS:
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Send NV ReadValue command to TPM1.2.

  @param NvIndex           The index of the area to set.
  @param Offset            The offset into the area.
  @param DataSize          The size of the data area.
  @param Data              The data to set the area to.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvReadValue (
  IN TPM_NV_INDEX   NvIndex,
  IN UINT32         Offset,
  IN OUT UINT32     *DataSize,
  OUT UINT8         *Data
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_NV_READ_VALUE             SendBuffer;
  TPM_RSP_NV_READ_VALUE             RecvBuffer;
  UINT32                            ReturnCode;

  //
  // send Tpm command TPM_ORD_NV_ReadValue
  //
  TpmRecvSize               = sizeof (TPM_RSP_NV_READ_VALUE);
  TpmSendSize               = sizeof (TPM_CMD_NV_READ_VALUE);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (sizeof(TPM_CMD_NV_READ_VALUE));
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_NV_ReadValue);
  SendBuffer.NvIndex        = SwapBytes32 (NvIndex);
  SendBuffer.Offset         = SwapBytes32 (Offset);
  SendBuffer.DataSize       = SwapBytes32 (*DataSize);

  Status = Tpm12SubmitCommand (TpmSendSize, (UINT8 *)&SendBuffer, &TpmRecvSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReturnCode = SwapBytes32(RecvBuffer.Hdr.returnCode);
  DEBUG ((DEBUG_INFO, "Tpm12NvReadValue - ReturnCode = %x\n", ReturnCode));
  switch (ReturnCode) {
  case TPM_SUCCESS:
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  //
  // Return the response
  //
  *DataSize = SwapBytes32(RecvBuffer.DataSize);
  CopyMem (Data, &RecvBuffer.Data, *DataSize);

  return EFI_SUCCESS;
}

/**
  Send NV WriteValue command to TPM1.2.
  
  @param NvIndex           The index of the area to set.
  @param Offset            The offset into the NV Area.
  @param DataSize          The size of the data parameter.
  @param Data              The data to set the area to.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvWriteValue (
  IN TPM_NV_INDEX   NvIndex,
  IN UINT32         Offset,
  IN UINT32         DataSize,
  IN UINT8          *Data
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_NV_WRITE_VALUE            SendBuffer;
  TPM_RSP_NV_WRITE_VALUE            RecvBuffer;
  UINT32                            ReturnCode;

  if (DataSize > sizeof(SendBuffer.Data)) {
    return EFI_UNSUPPORTED;
  }

  //
  // send Tpm command TPM_ORD_NV_WriteValue
  //
  TpmRecvSize               = sizeof (TPM_RSP_NV_WRITE_VALUE);
  TpmSendSize               = sizeof (TPM_CMD_NV_WRITE_VALUE) - sizeof(SendBuffer.Data) + DataSize;
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (sizeof(TPM_CMD_NV_WRITE_VALUE) - sizeof(SendBuffer.Data) + DataSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_NV_WriteValue);
  SendBuffer.NvIndex        = SwapBytes32 (NvIndex);
  SendBuffer.Offset         = SwapBytes32 (Offset);
  SendBuffer.DataSize       = SwapBytes32 (DataSize);
  CopyMem (SendBuffer.Data, Data, DataSize);

  Status = Tpm12SubmitCommand (TpmSendSize, (UINT8 *)&SendBuffer, &TpmRecvSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReturnCode = SwapBytes32(RecvBuffer.Hdr.returnCode);
  DEBUG ((DEBUG_INFO, "Tpm12NvWritedValue - ReturnCode = %x\n", ReturnCode));
  switch (ReturnCode) {
  case TPM_SUCCESS:
    break;
  default:
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
