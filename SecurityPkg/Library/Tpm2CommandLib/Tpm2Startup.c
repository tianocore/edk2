/** @file
  Implement TPM2 Startup related command.

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

typedef struct {
  TPM2_COMMAND_HEADER  Header;
  TPM_SU               StartupType;
} TPM2_STARTUP_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER Header;
} TPM2_STARTUP_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER  Header;
  TPM_SU               ShutdownType;
} TPM2_SHUTDOWN_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER Header;
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
  IN      TPM_SU             StartupType
  )
{
  EFI_STATUS                        Status;
  TPM2_STARTUP_COMMAND              Cmd;
  TPM2_STARTUP_RESPONSE             Res;
  UINT32                            ResultBufSize;

  Cmd.Header.tag         = SwapBytes16(TPM_ST_NO_SESSIONS);
  Cmd.Header.paramSize   = SwapBytes32(sizeof(Cmd));
  Cmd.Header.commandCode = SwapBytes32(TPM_CC_Startup);
  Cmd.StartupType        = SwapBytes16(StartupType);

  ResultBufSize = sizeof(Res);
  Status = Tpm2SubmitCommand (sizeof(Cmd), (UINT8 *)&Cmd, &ResultBufSize, (UINT8 *)&Res);

  return Status;
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
  IN      TPM_SU             ShutdownType
  )
{
  EFI_STATUS                        Status;
  TPM2_SHUTDOWN_COMMAND             Cmd;
  TPM2_SHUTDOWN_RESPONSE            Res;
  UINT32                            ResultBufSize;

  Cmd.Header.tag         = SwapBytes16(TPM_ST_NO_SESSIONS);
  Cmd.Header.paramSize   = SwapBytes32(sizeof(Cmd));
  Cmd.Header.commandCode = SwapBytes32(TPM_CC_Shutdown);
  Cmd.ShutdownType       = SwapBytes16(ShutdownType);

  ResultBufSize = sizeof(Res);
  Status = Tpm2SubmitCommand (sizeof(Cmd), (UINT8 *)&Cmd, &ResultBufSize, (UINT8 *)&Res);

  return Status;
}
