/** @file
  Implement TPM1.2 Startup related command.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
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

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_STARTUP_TYPE      TpmSt;
} TPM_CMD_START_UP;

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
} TPM_RSP_START_UP;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
} TPM_CMD_SAVE_STATE;

typedef struct {
  TPM_RSP_COMMAND_HDR   Hdr;
} TPM_RSP_SAVE_STATE;

#pragma pack()

/**
  Send Startup command to TPM1.2.

  @param TpmSt           Startup Type.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12Startup (
  IN TPM_STARTUP_TYPE          TpmSt
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_START_UP                  SendBuffer;
  TPM_RSP_START_UP                  RecvBuffer;
  UINT32                            ReturnCode;

  //
  // send Tpm command TPM_ORD_Startup
  //
  TpmRecvSize               = sizeof (TPM_RSP_START_UP);
  TpmSendSize               = sizeof (TPM_CMD_START_UP);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TpmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_Startup);
  SendBuffer.TpmSt          = SwapBytes16 (TpmSt);

  Status = Tpm12SubmitCommand (TpmSendSize, (UINT8 *)&SendBuffer, &TpmRecvSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReturnCode = SwapBytes32(RecvBuffer.Hdr.returnCode);
  switch (ReturnCode) {
  case TPM_SUCCESS:
  case TPM_INVALID_POSTINIT:
    // In warm reset, TPM may response TPM_INVALID_POSTINIT
    return EFI_SUCCESS;
  default:
    return EFI_DEVICE_ERROR;
  }
}

/**
  Send SaveState command to TPM1.2.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12SaveState (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_SAVE_STATE                SendBuffer;
  TPM_RSP_SAVE_STATE                RecvBuffer;
  UINT32                            ReturnCode;

  //
  // send Tpm command TPM_ORD_SaveState
  //
  TpmRecvSize               = sizeof (TPM_RSP_SAVE_STATE);
  TpmSendSize               = sizeof (TPM_CMD_SAVE_STATE);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TpmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_SaveState);

  Status = Tpm12SubmitCommand (TpmSendSize, (UINT8 *)&SendBuffer, &TpmRecvSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReturnCode = SwapBytes32(RecvBuffer.Hdr.returnCode);
  switch (ReturnCode) {
  case TPM_SUCCESS:
    return EFI_SUCCESS;
  default:
    return EFI_DEVICE_ERROR;
  }
}
