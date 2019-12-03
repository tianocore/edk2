/** @file
  Implement TPM1.2 NV Self Test related commands.

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved. <BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm12DeviceLib.h>

/**
Send TPM_ContinueSelfTest command to TPM.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12ContinueSelfTest (
  VOID
  )
{
  EFI_STATUS           Status;
  TPM_RQU_COMMAND_HDR  Command;
  TPM_RSP_COMMAND_HDR  Response;
  UINT32               Length;

  //
  // send Tpm command TPM_ORD_ContinueSelfTest
  //
  Command.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.paramSize = SwapBytes32 (sizeof (Command));
  Command.ordinal   = SwapBytes32 (TPM_ORD_ContinueSelfTest);
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12ContinueSelfTest: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  return Status;
}
