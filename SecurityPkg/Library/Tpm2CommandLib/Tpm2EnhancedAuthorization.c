/** @file
  Implement TPM2 EnhancedAuthorization related command.

Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved. <BR>
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
  TPMI_DH_ENTITY         AuthHandle;
  TPMI_SH_POLICY         PolicySession;
  UINT32                 AuthSessionSize;
  TPMS_AUTH_COMMAND      AuthSession;
  TPM2B_NONCE            NonceTPM;
  TPM2B_DIGEST           CpHashA;
  TPM2B_NONCE            PolicyRef;
  INT32                  Expiration;
} TPM2_POLICY_SECRET_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  UINT32                  AuthSessionSize;
  TPM2B_TIMEOUT           Timeout;
  TPMT_TK_AUTH            PolicyTicket;
  TPMS_AUTH_RESPONSE      AuthSession;
} TPM2_POLICY_SECRET_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_SH_POLICY         PolicySession;
  TPML_DIGEST            HashList;
} TPM2_POLICY_OR_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
} TPM2_POLICY_OR_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_SH_POLICY         PolicySession;
  TPM_CC                 Code;
} TPM2_POLICY_COMMAND_CODE_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
} TPM2_POLICY_COMMAND_CODE_RESPONSE;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_SH_POLICY         PolicySession;
} TPM2_POLICY_GET_DIGEST_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  TPM2B_DIGEST            PolicyHash;
} TPM2_POLICY_GET_DIGEST_RESPONSE;

#pragma pack()

/**
  This command includes a secret-based authorization to a policy.
  The caller proves knowledge of the secret value using an authorization
  session using the authValue associated with authHandle.

  @param[in]  AuthHandle         Handle for an entity providing the authorization
  @param[in]  PolicySession      Handle for the policy session being extended.
  @param[in]  AuthSession        Auth Session context
  @param[in]  NonceTPM           The policy nonce for the session.
  @param[in]  CpHashA            Digest of the command parameters to which this authorization is limited.
  @param[in]  PolicyRef          A reference to a policy relating to the authorization.
  @param[in]  Expiration         Time when authorization will expire, measured in seconds from the time that nonceTPM was generated.
  @param[out] Timeout            Time value used to indicate to the TPM when the ticket expires.
  @param[out] PolicyTicket       A ticket that includes a value indicating when the authorization expires.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
Tpm2PolicySecret (
  IN      TPMI_DH_ENTITY     AuthHandle,
  IN      TPMI_SH_POLICY     PolicySession,
  IN      TPMS_AUTH_COMMAND  *AuthSession  OPTIONAL,
  IN      TPM2B_NONCE        *NonceTPM,
  IN      TPM2B_DIGEST       *CpHashA,
  IN      TPM2B_NONCE        *PolicyRef,
  IN      INT32              Expiration,
  OUT     TPM2B_TIMEOUT      *Timeout,
  OUT     TPMT_TK_AUTH       *PolicyTicket
  )
{
  EFI_STATUS                   Status;
  TPM2_POLICY_SECRET_COMMAND   SendBuffer;
  TPM2_POLICY_SECRET_RESPONSE  RecvBuffer;
  UINT32                       SendBufferSize;
  UINT32                       RecvBufferSize;
  UINT8                        *Buffer;
  UINT32                       SessionInfoSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_PolicySecret);
  SendBuffer.AuthHandle         = SwapBytes32 (AuthHandle);
  SendBuffer.PolicySession      = SwapBytes32 (PolicySession);

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
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NonceTPM->size));
  Buffer += sizeof (UINT16);
  CopyMem (Buffer, NonceTPM->buffer, NonceTPM->size);
  Buffer += NonceTPM->size;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (CpHashA->size));
  Buffer += sizeof (UINT16);
  CopyMem (Buffer, CpHashA->buffer, CpHashA->size);
  Buffer += CpHashA->size;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (PolicyRef->size));
  Buffer += sizeof (UINT16);
  CopyMem (Buffer, PolicyRef->buffer, PolicyRef->size);
  Buffer += PolicyRef->size;

  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 ((UINT32)Expiration));
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
    DEBUG ((DEBUG_ERROR, "Tpm2PolicySecret - RecvBufferSize Error - %x\n", RecvBufferSize));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicySecret - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Return the response
  //
  Buffer        = (UINT8 *)&RecvBuffer.Timeout;
  Timeout->size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  if (Timeout->size > sizeof (UINT64)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicySecret - Timeout->size error %x\n", Timeout->size));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  Buffer += sizeof (UINT16);
  CopyMem (Timeout->buffer, Buffer, Timeout->size);

  PolicyTicket->tag         = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  Buffer                   += sizeof (UINT16);
  PolicyTicket->hierarchy   = SwapBytes32 (ReadUnaligned32 ((UINT32 *)Buffer));
  Buffer                   += sizeof (UINT32);
  PolicyTicket->digest.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  Buffer                   += sizeof (UINT16);
  if (PolicyTicket->digest.size > sizeof (TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicySecret - digest.size error %x\n", PolicyTicket->digest.size));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  CopyMem (PolicyTicket->digest.buffer, Buffer, PolicyTicket->digest.size);

Done:
  //
  // Clear AuthSession Content
  //
  ZeroMem (&SendBuffer, sizeof (SendBuffer));
  ZeroMem (&RecvBuffer, sizeof (RecvBuffer));
  return Status;
}

/**
  This command allows options in authorizations without requiring that the TPM evaluate all of the options.
  If a policy may be satisfied by different sets of conditions, the TPM need only evaluate one set that
  satisfies the policy. This command will indicate that one of the required sets of conditions has been
  satisfied.

  @param[in] PolicySession      Handle for the policy session being extended.
  @param[in] HashList           the list of hashes to check for a match.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
Tpm2PolicyOR (
  IN TPMI_SH_POLICY  PolicySession,
  IN TPML_DIGEST     *HashList
  )
{
  EFI_STATUS               Status;
  TPM2_POLICY_OR_COMMAND   SendBuffer;
  TPM2_POLICY_OR_RESPONSE  RecvBuffer;
  UINT32                   SendBufferSize;
  UINT32                   RecvBufferSize;
  UINT8                    *Buffer;
  UINTN                    Index;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_PolicyOR);

  SendBuffer.PolicySession = SwapBytes32 (PolicySession);
  Buffer                   = (UINT8 *)&SendBuffer.HashList;
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (HashList->count));
  Buffer += sizeof (UINT32);
  for (Index = 0; Index < HashList->count; Index++) {
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (HashList->digests[Index].size));
    Buffer += sizeof (UINT16);
    CopyMem (Buffer, HashList->digests[Index].buffer, HashList->digests[Index].size);
    Buffer += HashList->digests[Index].size;
  }

  SendBufferSize              = (UINT32)((UINTN)Buffer - (UINTN)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status         = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyOR - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyOR - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This command indicates that the authorization will be limited to a specific command code.

  @param[in]  PolicySession      Handle for the policy session being extended.
  @param[in]  Code               The allowed commandCode.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
Tpm2PolicyCommandCode (
  IN      TPMI_SH_POLICY  PolicySession,
  IN      TPM_CC          Code
  )
{
  EFI_STATUS                         Status;
  TPM2_POLICY_COMMAND_CODE_COMMAND   SendBuffer;
  TPM2_POLICY_COMMAND_CODE_RESPONSE  RecvBuffer;
  UINT32                             SendBufferSize;
  UINT32                             RecvBufferSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_PolicyCommandCode);

  SendBuffer.PolicySession = SwapBytes32 (PolicySession);
  SendBuffer.Code          = SwapBytes32 (Code);

  SendBufferSize              = (UINT32)sizeof (SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status         = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyCommandCode - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyCommandCode - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This command returns the current policyDigest of the session. This command allows the TPM
  to be used to perform the actions required to precompute the authPolicy for an object.

  @param[in]  PolicySession      Handle for the policy session.
  @param[out] PolicyHash         the current value of the policyHash of policySession.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
Tpm2PolicyGetDigest (
  IN      TPMI_SH_POLICY  PolicySession,
  OUT  TPM2B_DIGEST       *PolicyHash
  )
{
  EFI_STATUS                       Status;
  TPM2_POLICY_GET_DIGEST_COMMAND   SendBuffer;
  TPM2_POLICY_GET_DIGEST_RESPONSE  RecvBuffer;
  UINT32                           SendBufferSize;
  UINT32                           RecvBufferSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32 (TPM_CC_PolicyGetDigest);

  SendBuffer.PolicySession = SwapBytes32 (PolicySession);

  SendBufferSize              = (UINT32)sizeof (SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status         = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyGetDigest - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }

  if (SwapBytes32 (RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyGetDigest - responseCode - %x\n", SwapBytes32 (RecvBuffer.Header.responseCode)));
    return EFI_DEVICE_ERROR;
  }

  //
  // Return the response
  //
  PolicyHash->size = SwapBytes16 (RecvBuffer.PolicyHash.size);
  if (PolicyHash->size > sizeof (TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2PolicyGetDigest - PolicyHash->size error %x\n", PolicyHash->size));
    return EFI_DEVICE_ERROR;
  }

  CopyMem (PolicyHash->buffer, &RecvBuffer.PolicyHash.buffer, PolicyHash->size);

  return EFI_SUCCESS;
}
