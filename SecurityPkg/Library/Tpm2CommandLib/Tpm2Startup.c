/** @file
  Implement TPM2 Startup related command.

Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved. <BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#pragma pack(1)

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPM_SU                 StartupType;
} TPM2_STARTUP_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
} TPM2_STARTUP_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPM_SU                 ShutdownType;
} TPM2_SHUTDOWN_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
} TPM2_SHUTDOWN_RESPONSE;

#pragma pack()

/**
  Send Startup command to TPM2.

  @param[in] StartupType           TPM_SU_CLEAR or TPM_SU_STATE

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2Startup (
  IN      TPM_SU  StartupType
  )
{
  EFI_STATUS             Status;
  TPM2_STARTUP_COMMAND   Cmd;
  TPM2_STARTUP_RESPONSE  Res;
  UINT32                 ResultBufSize;
  TPM_RC                 ResponseCode;

  Cmd.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  Cmd.Header.paramSize   = SwapBytes32 (sizeof (Cmd));
  Cmd.Header.commandCode = SwapBytes32 (TPM_CC_Startup);
  Cmd.StartupType        = SwapBytes16 (StartupType);

  ResultBufSize = sizeof (Res);
  Status        = Tpm2SubmitCommand (sizeof (Cmd), (UINT8 *)&Cmd, &ResultBufSize, (UINT8 *)&Res);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ResponseCode = SwapBytes32 (Res.Header.responseCode);
  switch (ResponseCode) {
    case TPM_RC_SUCCESS:
      DEBUG ((DEBUG_INFO, "TPM2Startup: TPM_RC_SUCCESS\n"));
      return EFI_SUCCESS;
    case TPM_RC_INITIALIZE:
      // TPM_RC_INITIALIZE can be returned if Tpm2Startup is not required.
      DEBUG ((DEBUG_INFO, "TPM2Startup: TPM_RC_INITIALIZE\n"));
      return EFI_SUCCESS;
    default:
      DEBUG ((DEBUG_ERROR, "Tpm2Startup: Response Code error! 0x%08x\r\n", ResponseCode));
      return EFI_DEVICE_ERROR;
  }
}

/**
  Send Shutdown command to TPM2.

  @param[in] ShutdownType           TPM_SU_CLEAR or TPM_SU_STATE.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2Shutdown (
  IN      TPM_SU  ShutdownType
  )
{
  EFI_STATUS              Status;
  TPM2_SHUTDOWN_COMMAND   Cmd;
  TPM2_SHUTDOWN_RESPONSE  Res;
  UINT32                  ResultBufSize;

  Cmd.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  Cmd.Header.paramSize   = SwapBytes32 (sizeof (Cmd));
  Cmd.Header.commandCode = SwapBytes32 (TPM_CC_Shutdown);
  Cmd.ShutdownType       = SwapBytes16 (ShutdownType);

  ResultBufSize = sizeof (Res);
  Status        = Tpm2SubmitCommand (sizeof (Cmd), (UINT8 *)&Cmd, &ResultBufSize, (UINT8 *)&Res);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Res.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2Shutdown: Response Code error! 0x%08x\r\n", SwapBytes32 (Res.Header.responseCode)));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
