/** @file
  Implement TPM1.2 Ownership related command.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/Tpm12DeviceLib.h>

/**
  Send ForceClear command to TPM1.2.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12ForceClear (
  VOID
  )
{
  EFI_STATUS           Status;
  TPM_RQU_COMMAND_HDR  Command;
  TPM_RSP_COMMAND_HDR  Response;
  UINT32               Length;

  //
  // send Tpm command TPM_ORD_ForceClear
  //
  Command.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.paramSize = SwapBytes32 (sizeof (Command));
  Command.ordinal   = SwapBytes32 (TPM_ORD_ForceClear);
  Length            = sizeof (Response);

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
