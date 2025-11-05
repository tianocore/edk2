/** @file
  Implement TPM2 DictionaryAttack related command.

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
  TPM2_COMMAND_HEADER    Header;
  TPMI_RH_LOCKOUT        LockHandle;
  UINT32                 AuthSessionSize;
  TPMS_AUTH_COMMAND      AuthSession;
} TPM2_DICTIONARY_ATTACK_LOCK_RESET_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  UINT32                  AuthSessionSize;
  TPMS_AUTH_RESPONSE      AuthSession;
} TPM2_DICTIONARY_ATTACK_LOCK_RESET_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_RH_LOCKOUT        LockHandle;
  UINT32                 AuthSessionSize;
  TPMS_AUTH_COMMAND      AuthSession;
  UINT32                 NewMaxTries;
  UINT32                 NewRecoveryTime;
  UINT32                 LockoutRecovery;
} TPM2_DICTIONARY_ATTACK_PARAMETERS_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  UINT32                  AuthSessionSize;
  TPMS_AUTH_RESPONSE      AuthSession;
} TPM2_DICTIONARY_ATTACK_PARAMETERS_RESPONSE;

#pragma pack()

/**
  This command cancels the effect of a TPM lockout due to a number of successive authorization failures.
  If this command is properly authorized, the lockout counter is set to zero.

  @param[in]  LockHandle            TPM_RH_LOCKOUT
  @param[in]  AuthSession           Auth Session context

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2DictionaryAttackLockReset (
  IN  TPMI_RH_LOCKOUT    LockHandle,
  IN  TPMS_AUTH_COMMAND  *AuthSession
  )
{
  EFI_STATUS                                  Status;
  TPM2_DICTIONARY_ATTACK_LOCK_RESET_COMMAND   SendBuffer;
  TPM2_DICTIONARY_ATTACK_LOCK_RESET_RESPONSE  RecvBuffer;
  UINT32                                      SendBufferSize;
  UINT32                                      RecvBufferSize;
  UINT8                                       *Buffer;
  UINT32                                      SessionInfoSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_DictionaryAttackLockReset);

  SendBuffer.LockHandle = SwapBytes32 (LockHandle);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize            = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer                    += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32 (SessionInfoSize);

  SendBufferSize              = (UINT32)((UINTN)Buffer - (UINTN)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status         = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Tpm2DictionaryAttackLockReset - RecvBufferSize Error - %x\n", RecvBufferSize));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2DictionaryAttackLockReset - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  //
  // Clear AuthSession Content
  //
  ZeroMem (&SendBuffer, sizeof (SendBuffer));
  ZeroMem (&RecvBuffer, sizeof (RecvBuffer));
  return Status;
}

/**
  This command cancels the effect of a TPM lockout due to a number of successive authorization failures.
  If this command is properly authorized, the lockout counter is set to zero.

  @param[in]  LockHandle            TPM_RH_LOCKOUT
  @param[in]  AuthSession           Auth Session context
  @param[in]  NewMaxTries           Count of authorization failures before the lockout is imposed
  @param[in]  NewRecoveryTime       Time in seconds before the authorization failure count is automatically decremented
  @param[in]  LockoutRecovery       Time in seconds after a lockoutAuth failure before use of lockoutAuth is allowed

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2DictionaryAttackParameters (
  IN  TPMI_RH_LOCKOUT    LockHandle,
  IN  TPMS_AUTH_COMMAND  *AuthSession,
  IN  UINT32             NewMaxTries,
  IN  UINT32             NewRecoveryTime,
  IN  UINT32             LockoutRecovery
  )
{
  EFI_STATUS                                  Status;
  TPM2_DICTIONARY_ATTACK_PARAMETERS_COMMAND   SendBuffer;
  TPM2_DICTIONARY_ATTACK_PARAMETERS_RESPONSE  RecvBuffer;
  UINT32                                      SendBufferSize;
  UINT32                                      RecvBufferSize;
  UINT8                                       *Buffer;
  UINT32                                      SessionInfoSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_DictionaryAttackParameters);

  SendBuffer.LockHandle = SwapBytes32 (LockHandle);

  //
  // Add in Auth session
  //
  Buffer = (UINT8 *)&SendBuffer.AuthSession;

  // sessionInfoSize
  SessionInfoSize            = CopyAuthSessionCommand (AuthSession, Buffer);
  Buffer                    += SessionInfoSize;
  SendBuffer.AuthSessionSize = SwapBytes32 (SessionInfoSize);

  //
  // Real data
  //
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (NewMaxTries));
  Buffer += sizeof (UINT32);
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (NewRecoveryTime));
  Buffer += sizeof (UINT32);
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (LockoutRecovery));
  Buffer += sizeof (UINT32);

  SendBufferSize              = (UINT32)((UINTN)Buffer - (UINTN)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status         = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Tpm2DictionaryAttackParameters - RecvBufferSize Error - %x\n", RecvBufferSize));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2DictionaryAttackParameters - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  //
  // Clear AuthSession Content
  //
  ZeroMem (&SendBufferSize, sizeof (SendBufferSize));
  ZeroMem (&RecvBuffer, sizeof (RecvBuffer));
  return Status;
}
