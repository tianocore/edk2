/** @file
  SVSM TPM communication

Copyright (C) 2024 James.Bottomley@HansenPartnership.com
Copyright (C) 2024 IBM Corporation

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/AmdSvsmLib.h>

#include "Tpm2Svsm.h"

/**
  Platform commands (MSSIM commands) can be sent through the
  SVSM_VTPM_CMD operation. Each command can have its own
  request and response structures.
**/
#define TPM_SEND_COMMAND  8

/* Max req/resp buffer size */
#define TPM_PLATFORM_MAX_BUFFER  4096

STATIC UINT8  Tpm2SvsmBuffer[TPM_PLATFORM_MAX_BUFFER];

#pragma pack(1)
typedef struct _TPM2_SEND_CMD_REQ {
  UINT32    Cmd;
  UINT8     Locality;
  UINT32    BufSize;
  UINT8     Buf[];
} TPM2_SEND_CMD_REQ;

typedef struct _TPM2_SEND_CMD_RESP {
  UINT32    Size;
  UINT8     Buf[];
} TPM2_SEND_CMD_RESP;
#pragma pack()

/**
  Probe the SVSM vTPM for TPM_SEND_COMMAND support. The
  TPM_SEND_COMMAND platform command can be used to execute a
  TPM command and get the result.

  @retval TRUE    TPM_SEND_COMMAND is supported.
  @retval FALSE   TPM_SEND_COMMAND is not supported.

**/
BOOLEAN
Tpm2SvsmQueryTpmSendCmd (
  VOID
  )
{
  UINT64  PlatformCmdBitmap;
  UINT64  TpmSendMask;

  PlatformCmdBitmap = 0;
  TpmSendMask       = 1 << TPM_SEND_COMMAND;

  if (!AmdSvsmVtpmQuery (&PlatformCmdBitmap, NULL)) {
    return FALSE;
  }

  return ((PlatformCmdBitmap & TpmSendMask) == TpmSendMask);
}

/**
  Send a TPM command to the SVSM vTPM and return the TPM response.

  @param[in]      BufferIn      It should contain the marshaled
                                TPM command.
  @param[in]      SizeIn        Size of the TPM command.
  @param[out]     BufferOut     It will contain the marshaled
                                TPM response.
  @param[in, out] SizeOut       Size of the BufferOut; it will also
                                be used to return the size of the
                                TPM response

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Buffer not provided.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.
  @retval EFI_UNSUPPORTED       Unsupported TPM version

**/
EFI_STATUS
Tpm2SvsmTpmSendCommand (
  IN     UINT8   *BufferIn,
  IN     UINT32  SizeIn,
  OUT UINT8      *BufferOut,
  IN OUT UINT32  *SizeOut
  )
{
  TPM2_SEND_CMD_REQ   *req  = (TPM2_SEND_CMD_REQ *)Tpm2SvsmBuffer;
  TPM2_SEND_CMD_RESP  *resp = (TPM2_SEND_CMD_RESP *)Tpm2SvsmBuffer;

  if ((SizeIn == 0) || !BufferIn || !SizeOut || !BufferOut) {
    return EFI_INVALID_PARAMETER;
  }

  if (SizeIn > sizeof (Tpm2SvsmBuffer) - sizeof (*req)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  req->Cmd      = TPM_SEND_COMMAND;
  req->Locality = 0;
  req->BufSize  = SizeIn;
  CopyMem (req->Buf, BufferIn, SizeIn);

  if (!AmdSvsmVtpmCmd (Tpm2SvsmBuffer)) {
    return EFI_DEVICE_ERROR;
  }

  if (resp->Size > *SizeOut) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (BufferOut, resp->Buf, resp->Size);
  *SizeOut = resp->Size;

  return EFI_SUCCESS;
}
