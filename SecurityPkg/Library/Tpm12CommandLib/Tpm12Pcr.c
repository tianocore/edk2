/** @file
  Implement TPM1.2 PCR related commands.

Copyright (c) 2016, Intel Corporation. All rights reserved. <BR>
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
  TPM_PCRINDEX           PcrIndex;
  TPM_DIGEST             TpmDigest;
} TPM_CMD_EXTEND;

typedef struct {
  TPM_RSP_COMMAND_HDR    Hdr;
  TPM_DIGEST             TpmDigest;
} TPM_RSP_EXTEND;

#pragma pack()

/**
Extend a TPM PCR.

@param[in]  DigestToExtend    The 160 bit value representing the event to be recorded.
@param[in]  PcrIndex          The PCR to be updated.
@param[out] NewPcrValue       New PCR value after extend.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12Extend (
  IN  TPM_DIGEST    *DigestToExtend,
  IN  TPM_PCRINDEX  PcrIndex,
  OUT TPM_DIGEST    *NewPcrValue
  )
{
  EFI_STATUS      Status;
  TPM_CMD_EXTEND  Command;
  TPM_RSP_EXTEND  Response;
  UINT32          Length;

  //
  // send Tpm command TPM_ORD_Extend
  //
  Command.Hdr.tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Command.Hdr.paramSize = SwapBytes32 (sizeof (Command));
  Command.Hdr.ordinal   = SwapBytes32 (TPM_ORD_Extend);
  Command.PcrIndex      = SwapBytes32 (PcrIndex);
  CopyMem (&Command.TpmDigest, DigestToExtend, sizeof (Command.TpmDigest));
  Length = sizeof (Response);
  Status = Tpm12SubmitCommand (sizeof (Command), (UINT8 *)&Command, &Length, (UINT8 *)&Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SwapBytes32 (Response.Hdr.returnCode) != TPM_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm12Extend: Response Code error! 0x%08x\r\n", SwapBytes32 (Response.Hdr.returnCode)));
    return EFI_DEVICE_ERROR;
  }

  if (NewPcrValue != NULL) {
    CopyMem (NewPcrValue, &Response.TpmDigest, sizeof (*NewPcrValue));
  }

  return Status;
}
