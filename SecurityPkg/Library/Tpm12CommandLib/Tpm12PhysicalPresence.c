/** @file
  Implement TPM1.2 Physical Presence related command.

Copyright (c) 2016, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm12DeviceLib.h>

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_PHYSICAL_PRESENCE PhysicalPresence;
} TPM_CMD_PHYSICAL_PRESENCE;

#pragma pack()

/**
Send TSC_PhysicalPresence command to TPM.

@param[in] PhysicalPresence   The state to set the TPMs Physical Presence flags.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12PhysicalPresence (
  IN TPM_PHYSICAL_PRESENCE  PhysicalPresence
  )
{
  EFI_STATUS                 Status;
  TPM_CMD_PHYSICAL_PRESENCE  Command;
  TPM_RSP_COMMAND_HDR        Response;
  UINT32                     Length;

  //
  // send Tpm command TSC_ORD_PhysicalPresence
  //
  Command.Hdr.tag          = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize    = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal      = SwapBytes32 (TSC_ORD_PhysicalPresence);
  Command.PhysicalPresence = SwapBytes16 (PhysicalPresence);
  Length = sizeof (Response);

  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32(Response.returnCode) != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm12PhysicalPresence: Response Code error! 0x%08x\r\n", SwapBytes32(Response.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
