/** @file
  SVSM TPM communication

Copyright (C) 2024 James.Bottomley@HansenPartnership.com
Copyright (C) 2024 IBM Corporation
Copyright (C) 2024 Red Hat

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AmdSvsmLib.h>
#include <Library/PcdLib.h>

#include "Tpm2Svsm.h"

/**
  Platform commands (MSSIM commands) can be sent through the
  SVSM_VTPM_CMD operation. Each command can have its own
  request and response structures.
**/
#define TPM_SEND_COMMAND  8

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

/* Max req/resp buffer size */
#define TPM_PLATFORM_MAX_BUFFER  4096

typedef union {
  TPM2_SEND_CMD_REQ     req;
  TPM2_SEND_CMD_RESP    resp;
} SVSM_TPM_CMD_BUFFER;

STATIC_ASSERT (sizeof (SVSM_TPM_CMD_BUFFER) <= TPM_PLATFORM_MAX_BUFFER, "SVSM_TPM_CMD_BUFFER too large");

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

  return ((PlatformCmdBitmap & TpmSendMask) == TpmSendMask) ? TRUE : FALSE;
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
  @retval EFI_OUT_OF_RESOURCES  Out of memory when allocating internal buffer.
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
  STATIC SVSM_TPM_CMD_BUFFER  *Buffer = NULL;

  if ((SizeIn == 0) || !BufferIn || !SizeOut || !BufferOut) {
    return EFI_INVALID_PARAMETER;
  }

  if (SizeIn > TPM_PLATFORM_MAX_BUFFER - sizeof (TPM2_SEND_CMD_REQ)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Buffer == NULL) {
    STATIC_ASSERT (sizeof (UINT64) >= sizeof (UINTN), "Pointer size larger than 64bit");
    Buffer = (SVSM_TPM_CMD_BUFFER *)(UINTN)PcdGet64 (PcdSvsmVTpmBufferPtr);

    if (Buffer == NULL) {
      Buffer = (SVSM_TPM_CMD_BUFFER *)AllocatePages (EFI_SIZE_TO_PAGES (TPM_PLATFORM_MAX_BUFFER));
      if (Buffer == NULL) {
        DEBUG ((DEBUG_ERROR, "Unable to allocate SVSM vTPM buffer: %r", EFI_OUT_OF_RESOURCES));
        return EFI_OUT_OF_RESOURCES;
      }

      PcdSet64S (PcdSvsmVTpmBufferPtr, (UINTN)(VOID *)Buffer);
    }
  }

  Buffer->req.Cmd      = TPM_SEND_COMMAND;
  Buffer->req.Locality = 0;
  Buffer->req.BufSize  = SizeIn;
  CopyMem (Buffer->req.Buf, BufferIn, SizeIn);

  if (!AmdSvsmVtpmCmd ((UINT8 *)Buffer)) {
    return EFI_DEVICE_ERROR;
  }

  if (Buffer->resp.Size > TPM_PLATFORM_MAX_BUFFER - sizeof (TPM2_SEND_CMD_RESP)) {
    return EFI_DEVICE_ERROR;
  }

  if (Buffer->resp.Size > *SizeOut) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (BufferOut, Buffer->resp.Buf, Buffer->resp.Size);
  *SizeOut = Buffer->resp.Size;
  return EFI_SUCCESS;
}
