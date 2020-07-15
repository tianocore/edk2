/** @file
  Implement TPM1.2 Startup related command.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/DebugLib.h>

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_STARTUP_TYPE      TpmSt;
} TPM_CMD_START_UP;

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
  IN TPM_STARTUP_TYPE  TpmSt
  )
{
  EFI_STATUS           Status;
  TPM_CMD_START_UP     Command;
  TPM_RSP_COMMAND_HDR  Response;
  UINT32               Length;

  //
  // send Tpm command TPM_ORD_Startup
  //
  Command.Hdr.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal   = SwapBytes32 (TPM_ORD_Startup);
  Command.TpmSt         = SwapBytes16 (TpmSt);
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  switch (SwapBytes32(Response.returnCode)) {
  case TPM_SUCCESS:
    DEBUG ((DEBUG_INFO, "TPM12Startup: TPM_SUCCESS\n"));
    return EFI_SUCCESS;
  case TPM_INVALID_POSTINIT:
    // In warm reset, TPM may response TPM_INVALID_POSTINIT
    DEBUG ((DEBUG_INFO, "TPM12Startup: TPM_INVALID_POSTINIT\n"));
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
  EFI_STATUS           Status;
  TPM_RQU_COMMAND_HDR  Command;
  TPM_RSP_COMMAND_HDR  Response;
  UINT32               Length;

  //
  // send Tpm command TPM_ORD_SaveState
  //
  Command.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.paramSize  = SwapBytes32 (sizeof (Command));
  Command.ordinal    = SwapBytes32 (TPM_ORD_SaveState);
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  switch (SwapBytes32 (Response.returnCode)) {
  case TPM_SUCCESS:
    return EFI_SUCCESS;
  default:
    return EFI_DEVICE_ERROR;
  }
}
