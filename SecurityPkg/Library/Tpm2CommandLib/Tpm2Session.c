/** @file
  Implement TPM2 Session related command.

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
  TPM2_COMMAND_HEADER       Header;
  TPMI_DH_OBJECT            TpmKey;
  TPMI_DH_ENTITY            Bind;
  TPM2B_NONCE               NonceCaller;
  TPM2B_ENCRYPTED_SECRET    Salt;
  TPM_SE                    SessionType;
  TPMT_SYM_DEF              Symmetric;
  TPMI_ALG_HASH             AuthHash;
} TPM2_START_AUTH_SESSION_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER      Header;
  TPMI_SH_AUTH_SESSION      SessionHandle;
  TPM2B_NONCE               NonceTPM;
} TPM2_START_AUTH_SESSION_RESPONSE;

#pragma pack()

/**
  This command is used to start an authorization session using alternative methods of
  establishing the session key (sessionKey) that is used for authorization and encrypting value.

  @param[in]  TpmKey             Handle of a loaded decrypt key used to encrypt salt.
  @param[in]  Bind               Entity providing the authValue.
  @param[in]  NonceCaller        Initial nonceCaller, sets nonce size for the session.
  @param[in]  Salt               Value encrypted according to the type of tpmKey.
  @param[in]  SessionType        Indicates the type of the session.
  @param[in]  Symmetric          The algorithm and key size for parameter encryption.
  @param[in]  AuthHash           Hash algorithm to use for the session.
  @param[out] SessionHandle      Handle for the newly created session.
  @param[out] NonceTPM           The initial nonce from the TPM, used in the computation of the sessionKey.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
**/
EFI_STATUS
EFIAPI
Tpm2StartAuthSession (
  IN      TPMI_DH_OBJECT            TpmKey,
  IN      TPMI_DH_ENTITY            Bind,
  IN      TPM2B_NONCE               *NonceCaller,
  IN      TPM2B_ENCRYPTED_SECRET    *Salt,
  IN      TPM_SE                    SessionType,
  IN      TPMT_SYM_DEF              *Symmetric,
  IN      TPMI_ALG_HASH             AuthHash,
     OUT  TPMI_SH_AUTH_SESSION      *SessionHandle,
     OUT  TPM2B_NONCE               *NonceTPM
  )
{
  EFI_STATUS                        Status;
  TPM2_START_AUTH_SESSION_COMMAND   SendBuffer;
  TPM2_START_AUTH_SESSION_RESPONSE  RecvBuffer;
  UINT32                            SendBufferSize;
  UINT32                            RecvBufferSize;
  UINT8                             *Buffer;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_StartAuthSession);

  SendBuffer.TpmKey = SwapBytes32 (TpmKey);
  SendBuffer.Bind = SwapBytes32 (Bind);
  Buffer = (UINT8 *)&SendBuffer.NonceCaller;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (NonceCaller->size));
  Buffer += sizeof(UINT16);
  CopyMem (Buffer, NonceCaller->buffer, NonceCaller->size);
  Buffer += NonceCaller->size;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Salt->size));
  Buffer += sizeof(UINT16);
  CopyMem (Buffer, Salt->secret, Salt->size);
  Buffer += Salt->size;

  *(TPM_SE *)Buffer = SessionType;
  Buffer++;

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->algorithm));
  Buffer += sizeof(UINT16);
  switch (Symmetric->algorithm) {
  case TPM_ALG_NULL:
    break;
  case TPM_ALG_AES:
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->keyBits.aes));
    Buffer += sizeof(UINT16);
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->mode.aes));
    Buffer += sizeof(UINT16);
    break;
  case TPM_ALG_SM4:
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->keyBits.SM4));
    Buffer += sizeof(UINT16);
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->mode.SM4));
    Buffer += sizeof(UINT16);
    break;
  case TPM_ALG_SYMCIPHER:
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->keyBits.sym));
    Buffer += sizeof(UINT16);
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->mode.sym));
    Buffer += sizeof(UINT16);
    break;
  case TPM_ALG_XOR:
    WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (Symmetric->keyBits.xor));
    Buffer += sizeof(UINT16);
    break;
  default:
    ASSERT (FALSE);
    DEBUG ((EFI_D_ERROR, "Tpm2StartAuthSession - Symmetric->algorithm - %x\n", Symmetric->algorithm));
    return EFI_UNSUPPORTED;
  }

  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (AuthHash));
  Buffer += sizeof(UINT16);

  SendBufferSize = (UINT32) ((UINTN)Buffer - (UINTN)&SendBuffer);
  SendBuffer.Header.paramSize = SwapBytes32 (SendBufferSize);

  //
  // send Tpm command
  //
  RecvBufferSize = sizeof (RecvBuffer);
  Status = Tpm2SubmitCommand (SendBufferSize, (UINT8 *)&SendBuffer, &RecvBufferSize, (UINT8 *)&RecvBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)) {
    DEBUG ((EFI_D_ERROR, "Tpm2StartAuthSession - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }
  if (SwapBytes32(RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Tpm2StartAuthSession - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
    return EFI_DEVICE_ERROR;
  }

  //
  // Return the response
  //
  *SessionHandle = SwapBytes32 (RecvBuffer.SessionHandle);
  NonceTPM->size = SwapBytes16 (RecvBuffer.NonceTPM.size);
  if (NonceTPM->size > sizeof(TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2StartAuthSession - NonceTPM->size error %x\n", NonceTPM->size));
    return EFI_DEVICE_ERROR;
  }

  CopyMem (NonceTPM->buffer, &RecvBuffer.NonceTPM.buffer, NonceTPM->size);

  return EFI_SUCCESS;
}
