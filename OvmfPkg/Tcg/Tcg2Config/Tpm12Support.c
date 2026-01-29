/** @file
  Implement the InternalTpm12Detect() function on top of the Tpm12DeviceLib
  class.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/Tpm12DeviceLib.h>

#include "Tpm12Support.h"

#pragma pack (1)
typedef struct {
  TPM_RSP_COMMAND_HDR    Hdr;
  TPM_CURRENT_TICKS      CurrentTicks;
} TPM_RSP_GET_TICKS;
#pragma pack ()

/**
  Probe for the TPM for 1.2 version, by sending TPM1.2 GetTicks

  Sending a TPM1.2 command to a TPM2 should return a TPM1.2
  header (tag = 0xc4) and error code (TPM_BADTAG = 0x1e)

  @retval EFI_SUCCESS  TPM version 1.2 probing successful.

  @return              Error codes propagated from Tpm12SubmitCommand().
**/
STATIC
EFI_STATUS
TestTpm12 (
  )
{
  EFI_STATUS           Status;
  TPM_RQU_COMMAND_HDR  Command;
  TPM_RSP_GET_TICKS    Response;
  UINT32               Length;

  Command.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.paramSize = SwapBytes32 (sizeof (Command));
  Command.ordinal   = SwapBytes32 (TPM_ORD_GetTicks);

  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (
             sizeof (Command),
             (UINT8 *)&Command,
             &Length,
             (UINT8 *)&Response
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Detect the presence of a TPM with interface version 1.2.

  @retval EFI_SUCCESS      TPM-1.2 available. The Tpm12RequestUseTpm() and
                           Tpm12SubmitCommand(TPM_ORD_GetTicks) operations
                           (from the Tpm12DeviceLib class) have succeeded.

  @return                  Error codes propagated from Tpm12RequestUseTpm() and
                           Tpm12SubmitCommand().
**/
EFI_STATUS
InternalTpm12Detect (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = Tpm12RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return TestTpm12 ();
}
