/** @file
  Implement TPM2 Miscellanenous related command.

Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#pragma pack(1)

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_HIERARCHY_AUTH    AuthHandle;
  UINT32                    AuthSessionSize;
  TPMS_AUTH_COMMAND         AuthSession;
  UINT32                    AlgorithmSet;
} TPM2_SET_ALGORITHM_SET_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  UINT32                     AuthSessionSize;
  TPMS_AUTH_RESPONSE         AuthSession;
} TPM2_SET_ALGORITHM_SET_RESPONSE;

#pragma pack()

/**
  This command allows the platform to change the set of algorithms that are used by the TPM.
  The algorithmSet setting is a vendor-dependent value.

  @param[in]  AuthHandle              TPM_RH_PLATFORM
  @param[in]  AuthSession             Auth Session context
  @param[in]  AlgorithmSet            A TPM vendor-dependent value indicating the
                                      algorithm set selection

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2SetAlgorithmSet (
  IN  TPMI_RH_PLATFORM          AuthHandle,
  IN  TPMS_AUTH_COMMAND         *AuthSession,
  IN  UINT32                    AlgorithmSet
  )
{
  EFI_STATUS                                 Status;
  TPM2_SET_ALGORITHM_SET_COMMAND             SendBuffer;
  TPM2_SET_ALGORITHM_SET_RESPONSE            RecvBuffer;
  UINT32                                     SendBufferSize;
  UINT32                                     RecvBufferSize;
  UINT8                                      *Buffer;
  UINT32                                     SessionInfoSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_SetAlgorithmSet);

  SendBuffer.AuthHandle = SwapBytes32 (AuthHandle);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32(SessionInfoSize);

  //
  // Real data
  //
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32(AlgorithmSet));
  Buffer += sizeof(UINT32);

  SendBufferSize = (UINT32)((UINTN)Buffer - (UINTN)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2SetAlgorithmSet - RecvBufferSize Error - %x\n", RecvBufferSize));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  if (SwapBytes32(RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2SetAlgorithmSet - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  //
  // Clear AuthSession Content
  //
  ZeroMem (&SendBuffer, sizeof(SendBuffer));
  ZeroMem (&RecvBuffer, sizeof(RecvBuffer));
  return Status;
}
