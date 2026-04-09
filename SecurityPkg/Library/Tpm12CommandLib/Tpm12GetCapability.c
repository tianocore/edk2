/** @file
  Implement TPM1.2 Get Capabilities related commands.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm12DeviceLib.h>

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR    Hdr;
  UINT32                 Capability;
  UINT32                 CapabilityFlagSize;
  UINT32                 CapabilityFlag;
} TPM_CMD_GET_CAPABILITY;

typedef struct {
  TPM_RSP_COMMAND_HDR    Hdr;
  UINT32                 ResponseSize;
  TPM_PERMANENT_FLAGS    Flags;
} TPM_RSP_GET_CAPABILITY_PERMANENT_FLAGS;

typedef struct {
  TPM_RSP_COMMAND_HDR    Hdr;
  UINT32                 ResponseSize;
  TPM_STCLEAR_FLAGS      Flags;
} TPM_RSP_GET_CAPABILITY_STCLEAR_FLAGS;

#pragma pack()

/**
Get TPM capability permanent flags.

@param[out] TpmPermanentFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagPermanent (
  OUT TPM_PERMANENT_FLAGS  *TpmPermanentFlags
  )
{
  EFI_STATUS                              Status;
  TPM_CMD_GET_CAPABILITY                  Command;
  TPM_RSP_GET_CAPABILITY_PERMANENT_FLAGS  Response;
  UINT32                                  Length;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  Command.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize      = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  Command.Capability         = SwapBytes32 (TPM_CAP_FLAG);
  Command.CapabilityFlagSize = SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT));
  Command.CapabilityFlag     = SwapBytes32 (TPM_CAP_FLAG_PERMANENT);
  Length                     = sizeof (Response);
  Status                     = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12GetCapabilityFlagPermanent: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (TpmPermanentFlags, sizeof (*TpmPermanentFlags));
  CopyMem (TpmPermanentFlags, &Response.Flags, MIN (sizeof (*TpmPermanentFlags), SwapBytes32 (Response.ResponseSize)));

  return Status;
}

/**
Get TPM capability volatile flags.

@param[out] VolatileFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS      Operation completed successfully.
@retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagVolatile (
  OUT TPM_STCLEAR_FLAGS  *VolatileFlags
  )
{
  EFI_STATUS                            Status;
  TPM_CMD_GET_CAPABILITY                Command;
  TPM_RSP_GET_CAPABILITY_STCLEAR_FLAGS  Response;
  UINT32                                Length;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  Command.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize      = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  Command.Capability         = SwapBytes32 (TPM_CAP_FLAG);
  Command.CapabilityFlagSize = SwapBytes32 (sizeof (TPM_CAP_FLAG_VOLATILE));
  Command.CapabilityFlag     = SwapBytes32 (TPM_CAP_FLAG_VOLATILE);
  Length                     = sizeof (Response);
  Status                     = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12GetCapabilityFlagVolatile: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (VolatileFlags, sizeof (*VolatileFlags));
  CopyMem (VolatileFlags, &Response.Flags, MIN (sizeof (*VolatileFlags), SwapBytes32 (Response.ResponseSize)));

  return Status;
}
