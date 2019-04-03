/** @file
  Implement TPM2 Object related command.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved. <BR>
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
  TPMI_DH_OBJECT            ObjectHandle;
} TPM2_READ_PUBLIC_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER       Header;
  TPM2B_PUBLIC               OutPublic;
  TPM2B_NAME                 Name;
  TPM2B_NAME                 QualifiedName;
} TPM2_READ_PUBLIC_RESPONSE;

#pragma pack()

/**
  This command allows access to the public area of a loaded object.

  @param[in]  ObjectHandle            TPM handle of an object
  @param[out] OutPublic               Structure containing the public area of an object
  @param[out] Name                    Name of the object
  @param[out] QualifiedName           The Qualified Name of the object

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2ReadPublic (
  IN  TPMI_DH_OBJECT            ObjectHandle,
  OUT TPM2B_PUBLIC              *OutPublic,
  OUT TPM2B_NAME                *Name,
  OUT TPM2B_NAME                *QualifiedName
  )
{
  EFI_STATUS                                 Status;
  TPM2_READ_PUBLIC_COMMAND                   SendBuffer;
  TPM2_READ_PUBLIC_RESPONSE                  RecvBuffer;
  UINT32                                     SendBufferSize;
  UINT32                                     RecvBufferSize;
  TPM_RC                                     ResponseCode;
  UINT8                                      *Buffer;
  UINT16                                     OutPublicSize;
  UINT16                                     NameSize;
  UINT16                                     QualifiedNameSize;

  //
  // Construct command
  //
  SendBuffer.Header.tag = SwapBytes16(TPM_ST_NO_SESSIONS);
  SendBuffer.Header.commandCode = SwapBytes32(TPM_CC_ReadPublic);

  SendBuffer.ObjectHandle = SwapBytes32 (ObjectHandle);

  SendBufferSize = (UINT32) sizeof (SendBuffer);
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
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - RecvBufferSize Error - %x\n", RecvBufferSize));
    return EFI_DEVICE_ERROR;
  }
  ResponseCode = SwapBytes32(RecvBuffer.Header.responseCode);
  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - responseCode - %x\n", SwapBytes32(RecvBuffer.Header.responseCode)));
  }
  switch (ResponseCode) {
  case TPM_RC_SUCCESS:
    // return data
    break;
  case TPM_RC_SEQUENCE:
    // objectHandle references a sequence object
    return EFI_INVALID_PARAMETER;
  default:
    return EFI_DEVICE_ERROR;
  }

  //
  // Basic check
  //
  OutPublicSize = SwapBytes16 (RecvBuffer.OutPublic.size);
  if (OutPublicSize > sizeof(TPMT_PUBLIC)) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - OutPublicSize error %x\n", OutPublicSize));
    return EFI_DEVICE_ERROR;
  }

  NameSize = SwapBytes16 (ReadUnaligned16 ((UINT16 *)((UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) +
                          sizeof(UINT16) + OutPublicSize)));
  if (NameSize > sizeof(TPMU_NAME)) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - NameSize error %x\n", NameSize));
    return EFI_DEVICE_ERROR;
  }

  QualifiedNameSize = SwapBytes16 (ReadUnaligned16 ((UINT16 *)((UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) +
                                   sizeof(UINT16) + OutPublicSize +
                                   sizeof(UINT16) + NameSize)));
  if (QualifiedNameSize > sizeof(TPMU_NAME)) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - QualifiedNameSize error %x\n", QualifiedNameSize));
    return EFI_DEVICE_ERROR;
  }

  if (RecvBufferSize != sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + OutPublicSize + sizeof(UINT16) + NameSize + sizeof(UINT16) + QualifiedNameSize) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - RecvBufferSize %x Error - OutPublicSize %x, NameSize %x, QualifiedNameSize %x\n", RecvBufferSize, OutPublicSize, NameSize, QualifiedNameSize));
    return EFI_DEVICE_ERROR;
  }

  //
  // Return the response
  //
  Buffer = (UINT8 *)&RecvBuffer.OutPublic;
  CopyMem (OutPublic, &RecvBuffer.OutPublic, sizeof(UINT16) + OutPublicSize);
  OutPublic->size = OutPublicSize;
  OutPublic->publicArea.type = SwapBytes16 (OutPublic->publicArea.type);
  OutPublic->publicArea.nameAlg = SwapBytes16 (OutPublic->publicArea.nameAlg);
  WriteUnaligned32 ((UINT32 *)&OutPublic->publicArea.objectAttributes, SwapBytes32 (ReadUnaligned32 ((UINT32 *)&OutPublic->publicArea.objectAttributes)));
  Buffer = (UINT8 *)&RecvBuffer.OutPublic.publicArea.authPolicy;
  OutPublic->publicArea.authPolicy.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
  Buffer += sizeof(UINT16);
  if (OutPublic->publicArea.authPolicy.size > sizeof(TPMU_HA)) {
    DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - authPolicy.size error %x\n", OutPublic->publicArea.authPolicy.size));
    return EFI_DEVICE_ERROR;
  }

  CopyMem (OutPublic->publicArea.authPolicy.buffer, Buffer, OutPublic->publicArea.authPolicy.size);
  Buffer += OutPublic->publicArea.authPolicy.size;

  // TPMU_PUBLIC_PARMS
  switch (OutPublic->publicArea.type) {
  case TPM_ALG_KEYEDHASH:
    OutPublic->publicArea.parameters.keyedHashDetail.scheme.scheme = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.keyedHashDetail.scheme.scheme) {
    case TPM_ALG_HMAC:
      OutPublic->publicArea.parameters.keyedHashDetail.scheme.details.hmac.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_XOR:
      OutPublic->publicArea.parameters.keyedHashDetail.scheme.details.xor.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.keyedHashDetail.scheme.details.xor.kdf = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    default:
      return EFI_UNSUPPORTED;
    }
  case TPM_ALG_SYMCIPHER:
    OutPublic->publicArea.parameters.symDetail.algorithm = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.symDetail.algorithm) {
    case TPM_ALG_AES:
      OutPublic->publicArea.parameters.symDetail.keyBits.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.symDetail.mode.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_SM4:
      OutPublic->publicArea.parameters.symDetail.keyBits.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.symDetail.mode.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_XOR:
      OutPublic->publicArea.parameters.symDetail.keyBits.xor = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    break;
  case TPM_ALG_RSA:
    OutPublic->publicArea.parameters.rsaDetail.symmetric.algorithm = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.rsaDetail.symmetric.algorithm) {
    case TPM_ALG_AES:
      OutPublic->publicArea.parameters.rsaDetail.symmetric.keyBits.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.rsaDetail.symmetric.mode.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_SM4:
      OutPublic->publicArea.parameters.rsaDetail.symmetric.keyBits.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.rsaDetail.symmetric.mode.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    OutPublic->publicArea.parameters.rsaDetail.scheme.scheme = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.rsaDetail.scheme.scheme) {
    case TPM_ALG_RSASSA:
      OutPublic->publicArea.parameters.rsaDetail.scheme.details.rsassa.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_RSAPSS:
      OutPublic->publicArea.parameters.rsaDetail.scheme.details.rsapss.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_RSAES:
      break;
    case TPM_ALG_OAEP:
      OutPublic->publicArea.parameters.rsaDetail.scheme.details.oaep.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    OutPublic->publicArea.parameters.rsaDetail.keyBits = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    OutPublic->publicArea.parameters.rsaDetail.exponent = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT32);
    break;
  case TPM_ALG_ECC:
    OutPublic->publicArea.parameters.eccDetail.symmetric.algorithm = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.eccDetail.symmetric.algorithm) {
    case TPM_ALG_AES:
      OutPublic->publicArea.parameters.eccDetail.symmetric.keyBits.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.eccDetail.symmetric.mode.aes = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_SM4:
      OutPublic->publicArea.parameters.eccDetail.symmetric.keyBits.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      OutPublic->publicArea.parameters.eccDetail.symmetric.mode.SM4 = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    OutPublic->publicArea.parameters.eccDetail.scheme.scheme = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.eccDetail.scheme.scheme) {
    case TPM_ALG_ECDSA:
      OutPublic->publicArea.parameters.eccDetail.scheme.details.ecdsa.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_ECDAA:
      OutPublic->publicArea.parameters.eccDetail.scheme.details.ecdaa.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_ECSCHNORR:
      OutPublic->publicArea.parameters.eccDetail.scheme.details.ecSchnorr.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_ECDH:
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    OutPublic->publicArea.parameters.eccDetail.curveID = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    OutPublic->publicArea.parameters.eccDetail.kdf.scheme = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    switch (OutPublic->publicArea.parameters.eccDetail.kdf.scheme) {
    case TPM_ALG_MGF1:
      OutPublic->publicArea.parameters.eccDetail.kdf.details.mgf1.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_KDF1_SP800_108:
      OutPublic->publicArea.parameters.eccDetail.kdf.details.kdf1_sp800_108.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_KDF1_SP800_56a:
      OutPublic->publicArea.parameters.eccDetail.kdf.details.kdf1_SP800_56a.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_KDF2:
      OutPublic->publicArea.parameters.eccDetail.kdf.details.kdf2.hashAlg = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
      Buffer += sizeof(UINT16);
      break;
    case TPM_ALG_NULL:
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  // TPMU_PUBLIC_ID
  switch (OutPublic->publicArea.type) {
  case TPM_ALG_KEYEDHASH:
    OutPublic->publicArea.unique.keyedHash.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    if(OutPublic->publicArea.unique.keyedHash.size > sizeof(TPMU_HA)) {
      DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - keyedHash.size error %x\n", OutPublic->publicArea.unique.keyedHash.size));
      return EFI_DEVICE_ERROR;
    }
    CopyMem (OutPublic->publicArea.unique.keyedHash.buffer, Buffer, OutPublic->publicArea.unique.keyedHash.size);
    Buffer += OutPublic->publicArea.unique.keyedHash.size;
    break;
  case TPM_ALG_SYMCIPHER:
    OutPublic->publicArea.unique.sym.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    if(OutPublic->publicArea.unique.sym.size > sizeof(TPMU_HA)) {
      DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - sym.size error %x\n", OutPublic->publicArea.unique.sym.size));
      return EFI_DEVICE_ERROR;
    }
    CopyMem (OutPublic->publicArea.unique.sym.buffer, Buffer, OutPublic->publicArea.unique.sym.size);
    Buffer += OutPublic->publicArea.unique.sym.size;
    break;
  case TPM_ALG_RSA:
    OutPublic->publicArea.unique.rsa.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    if(OutPublic->publicArea.unique.rsa.size > MAX_RSA_KEY_BYTES) {
      DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - rsa.size error %x\n", OutPublic->publicArea.unique.rsa.size));
      return EFI_DEVICE_ERROR;
    }
    CopyMem (OutPublic->publicArea.unique.rsa.buffer, Buffer, OutPublic->publicArea.unique.rsa.size);
    Buffer += OutPublic->publicArea.unique.rsa.size;
    break;
  case TPM_ALG_ECC:
    OutPublic->publicArea.unique.ecc.x.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    if (OutPublic->publicArea.unique.ecc.x.size > MAX_ECC_KEY_BYTES) {
      DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - ecc.x.size error %x\n", OutPublic->publicArea.unique.ecc.x.size));
      return EFI_DEVICE_ERROR;
    }
    CopyMem (OutPublic->publicArea.unique.ecc.x.buffer, Buffer, OutPublic->publicArea.unique.ecc.x.size);
    Buffer += OutPublic->publicArea.unique.ecc.x.size;
    OutPublic->publicArea.unique.ecc.y.size = SwapBytes16 (ReadUnaligned16 ((UINT16 *)Buffer));
    Buffer += sizeof(UINT16);
    if (OutPublic->publicArea.unique.ecc.y.size > MAX_ECC_KEY_BYTES) {
      DEBUG ((DEBUG_ERROR, "Tpm2ReadPublic - ecc.y.size error %x\n", OutPublic->publicArea.unique.ecc.y.size));
      return EFI_DEVICE_ERROR;
    }
    CopyMem (OutPublic->publicArea.unique.ecc.y.buffer, Buffer, OutPublic->publicArea.unique.ecc.y.size);
    Buffer += OutPublic->publicArea.unique.ecc.y.size;
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  CopyMem (Name->name, (UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + OutPublicSize + sizeof(UINT16), NameSize);
  Name->size = NameSize;

  CopyMem (QualifiedName->name, (UINT8 *)&RecvBuffer + sizeof(TPM2_RESPONSE_HEADER) + sizeof(UINT16) + OutPublicSize + sizeof(UINT16) + NameSize + sizeof(UINT16), QualifiedNameSize);
  QualifiedName->size = QualifiedNameSize;

  return EFI_SUCCESS;
}
