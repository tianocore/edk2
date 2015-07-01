/** @file
  The implementation of Payloads Creation.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Utility.h"
#include "IpSecDebug.h"
#include "IpSecConfigImpl.h"
#include "IpSecCryptIo.h"

//
// The Constant String of "Key Pad for IKEv2" for Authentication Payload generation.
//
#define CONSTANT_KEY_SIZE     17
GLOBAL_REMOVE_IF_UNREFERENCED CHAR8 mConstantKey[CONSTANT_KEY_SIZE] =
{
  'K', 'e', 'y', ' ', 'P', 'a', 'd', ' ', 'f', 'o', 'r', ' ', 'I', 'K', 'E', 'v', '2'
};

/**
  Generate Ikev2 SA payload according to SessionSaData

  @param[in] SessionSaData   The data used in SA payload.
  @param[in] NextPayload     The payload type presented in NextPayload field of
                             SA Payload header.
  @param[in] Type            The SA type. It MUST be neither (1) for IKE_SA or
                             (2) for CHILD_SA or (3) for INFO.

  @retval a Pointer to SA IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateSaPayload (
  IN IKEV2_SA_DATA    *SessionSaData,
  IN UINT8            NextPayload,
  IN IKE_SESSION_TYPE Type
  )
{
  IKE_PAYLOAD   *SaPayload;
  IKEV2_SA_DATA *SaData;
  UINTN         SaDataSize;

  SaPayload = IkePayloadAlloc ();
  ASSERT (SaPayload != NULL);
  //
  // TODO: Get the Proposal Number and Transform Number from IPsec Config,
  // after the Ipsecconfig Application is support it.
  //

  if (Type == IkeSessionTypeIkeSa) {
    SaDataSize = sizeof (IKEV2_SA_DATA) +
                 SessionSaData->NumProposals * sizeof (IKEV2_PROPOSAL_DATA) +
                 sizeof (IKEV2_TRANSFORM_DATA) * SessionSaData->NumProposals * 4;
  } else {
    SaDataSize = sizeof (IKEV2_SA_DATA) +
                 SessionSaData->NumProposals * sizeof (IKEV2_PROPOSAL_DATA) +
                 sizeof (IKEV2_TRANSFORM_DATA) * SessionSaData->NumProposals * 3;

  }

  SaData = AllocateZeroPool (SaDataSize);
  ASSERT (SaData != NULL);

  CopyMem (SaData, SessionSaData, SaDataSize);
  SaData->SaHeader.Header.NextPayload = NextPayload;
  SaPayload->PayloadType              = IKEV2_PAYLOAD_TYPE_SA;
  SaPayload->PayloadBuf               = (UINT8 *) SaData;

  return SaPayload;
}

/**
  Generate a Nonce payload containing the input parameter NonceBuf.

  @param[in]  NonceBuf      The nonce buffer contains the whole Nonce payload block
                            except the payload header.
  @param[in]  NonceSize     The buffer size of the NonceBuf
  @param[in]  NextPayload   The payload type presented in the NextPayload field
                            of Nonce Payload header.

  @retval Pointer to Nonce IKE paload.

**/
IKE_PAYLOAD *
Ikev2GenerateNoncePayload (
  IN UINT8            *NonceBuf,
  IN UINTN            NonceSize,
  IN UINT8            NextPayload
  )
{
  IKE_PAYLOAD *NoncePayload;
  IKEV2_NONCE *Nonce;
  UINTN       Size;
  UINT8       *NonceBlock;

  //                           1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !                                                               !
  //    ~                            Nonce Data                         ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  Size        = sizeof (IKEV2_NONCE) + NonceSize;
  NonceBlock  = NonceBuf;

  Nonce       = AllocateZeroPool (Size);
  ASSERT (Nonce != NULL);
  CopyMem (Nonce + 1, NonceBlock, Size - sizeof (IKEV2_NONCE));

  Nonce->Header.NextPayload   = NextPayload;
  Nonce->Header.PayloadLength = (UINT16) Size;
  NoncePayload                = IkePayloadAlloc ();

  ASSERT (NoncePayload != NULL);
  NoncePayload->PayloadType = IKEV2_PAYLOAD_TYPE_NONCE;
  NoncePayload->PayloadBuf  = (UINT8 *) Nonce;
  NoncePayload->PayloadSize = Size;

  return NoncePayload;
}

/**
  Generate a Key Exchange payload according to the DH group type and save the
  public Key into IkeSaSession IkeKey field.

  @param[in, out] IkeSaSession    Pointer of the IKE_SA_SESSION.
  @param[in]      NextPayload     The payload type presented in the NextPayload field of Key
                                  Exchange Payload header.

  @retval Pointer to Key IKE payload.

**/
IKE_PAYLOAD*
Ikev2GenerateKePayload (
  IN OUT IKEV2_SA_SESSION *IkeSaSession,
  IN     UINT8            NextPayload
  )
{
  IKE_PAYLOAD         *KePayload;
  IKEV2_KEY_EXCHANGE  *Ke;
  UINTN               KeSize;
  IKEV2_SESSION_KEYS  *IkeKeys;

  //
  //                        1                   2                   3
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !          DH Group #           !           RESERVED            !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !                                                               !
  //   ~                       Key Exchange Data                       ~
  //   !                                                               !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  IkeKeys = IkeSaSession->IkeKeys;

  if (IkeSaSession->SessionCommon.IsInitiator) {
    KeSize = sizeof (IKEV2_KEY_EXCHANGE) + IkeKeys->DhBuffer->GxSize;
  } else {
    KeSize = sizeof (IKEV2_KEY_EXCHANGE) + IkeKeys->DhBuffer->GxSize;
  }

  //
  // Allocate buffer for Key Exchange
  //
  Ke = AllocateZeroPool (KeSize);
  ASSERT (Ke != NULL);

  Ke->Header.NextPayload    = NextPayload;
  Ke->Header.PayloadLength  = (UINT16) KeSize;
  Ke->DhGroup               = IkeSaSession->SessionCommon.PreferDhGroup;

  CopyMem (Ke + 1, IkeKeys->DhBuffer->GxBuffer, IkeKeys->DhBuffer->GxSize);

  //
  // Create IKE_PAYLOAD to point to Key Exchange payload
  //
  KePayload = IkePayloadAlloc ();
  ASSERT (KePayload != NULL);

  KePayload->PayloadType = IKEV2_PAYLOAD_TYPE_KE;
  KePayload->PayloadBuf  = (UINT8 *) Ke;
  KePayload->PayloadSize = KeSize;
  return KePayload;
}

/**
  Generate a ID payload.

  @param[in] CommonSession   Pointer to IKEV2_SESSION_COMMON related to ID payload.
  @param[in] NextPayload     The payload type presented in the NextPayload field
                             of ID Payload header.

  @retval Pointer to ID IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateIdPayload (
  IN IKEV2_SESSION_COMMON *CommonSession,
  IN UINT8                NextPayload
  )
{
  IKE_PAYLOAD    *IdPayload;
  IKEV2_ID       *Id;
  UINTN          IdSize;
  UINT8          IpVersion;
  UINT8          AddrSize;

  //
  // ID payload
  //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   ! Next Payload  !   RESERVED    !         Payload Length        !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !   ID Type     !             RESERVED                          !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !                                                               !
  //   ~                   Identification Data                         ~
  //   !                                                               !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  IpVersion = CommonSession->UdpService->IpVersion;
  AddrSize  = (UINT8) ((IpVersion == IP_VERSION_4) ? sizeof(EFI_IPv4_ADDRESS) : sizeof(EFI_IPv6_ADDRESS));
  IdSize    = sizeof (IKEV2_ID) + AddrSize;

  Id = (IKEV2_ID *) AllocateZeroPool (IdSize);
  ASSERT (Id != NULL);

  IdPayload = IkePayloadAlloc ();
  ASSERT (IdPayload != NULL);

  IdPayload->PayloadType  = (UINT8) ((CommonSession->IsInitiator) ? IKEV2_PAYLOAD_TYPE_ID_INIT : IKEV2_PAYLOAD_TYPE_ID_RSP);
  IdPayload->PayloadBuf   = (UINT8 *) Id;
  IdPayload->PayloadSize  = IdSize;

  //
  // Set generic header of identification payload
  //
  Id->Header.NextPayload    = NextPayload;
  Id->Header.PayloadLength  = (UINT16) IdSize;
  Id->IdType                = (UINT8) ((IpVersion == IP_VERSION_4) ? IKEV2_ID_TYPE_IPV4_ADDR : IKEV2_ID_TYPE_IPV6_ADDR);
  CopyMem (Id + 1, &CommonSession->LocalPeerIp, AddrSize);

  return IdPayload;
}

/**
  Generate a ID payload.

  @param[in] CommonSession   Pointer to IKEV2_SESSION_COMMON related to ID payload.
  @param[in] NextPayload     The payload type presented in the NextPayload field
                             of ID Payload header.
  @param[in] InCert          Pointer to the Certificate which distinguished name
                             will be added into the Id payload.
  @param[in] CertSize        Size of the Certificate.

  @retval Pointer to ID IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCertIdPayload (
  IN IKEV2_SESSION_COMMON *CommonSession,
  IN UINT8                NextPayload,
  IN UINT8                *InCert,
  IN UINTN                CertSize
  )
{
  IKE_PAYLOAD    *IdPayload;
  IKEV2_ID       *Id;
  UINTN          IdSize;
  UINTN          SubjectSize;
  UINT8          *CertSubject;

  //
  // ID payload
  //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   ! Next Payload  !   RESERVED    !         Payload Length        !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !   ID Type     !             RESERVED                          !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !                                                               !
  //   ~                   Identification Data                         ~
  //   !                                                               !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  SubjectSize = 0;
  CertSubject = NULL;
  IpSecCryptoIoGetSubjectFromCert (
    InCert,
    CertSize,
    &CertSubject,
    &SubjectSize
    );
  if (SubjectSize != 0) {
    ASSERT (CertSubject != NULL);
  }

  IdSize = sizeof (IKEV2_ID) + SubjectSize;

  Id = (IKEV2_ID *) AllocateZeroPool (IdSize);
  ASSERT (Id != NULL);

  IdPayload = IkePayloadAlloc ();
  ASSERT (IdPayload != NULL);

  IdPayload->PayloadType  = (UINT8) ((CommonSession->IsInitiator) ? IKEV2_PAYLOAD_TYPE_ID_INIT : IKEV2_PAYLOAD_TYPE_ID_RSP);
  IdPayload->PayloadBuf   = (UINT8 *) Id;
  IdPayload->PayloadSize  = IdSize;

  //
  // Set generic header of identification payload
  //
  Id->Header.NextPayload    = NextPayload;
  Id->Header.PayloadLength  = (UINT16) IdSize;
  Id->IdType                = 9;
  CopyMem (Id + 1, CertSubject, SubjectSize);

  if (CertSubject != NULL) {
    FreePool (CertSubject);
  }
  return IdPayload;
}

/**
  Generate a Authentication Payload.

  This function is used for both Authentication generation and verification. When the
  IsVerify is TRUE, it create a Auth Data for verification. This function choose the
  related IKE_SA_INIT Message for Auth data creation according to the IKE Session's type
  and the value of IsVerify parameter.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION related to.
  @param[in]  IdPayload     Pointer to the ID payload to be used for Authentication
                            payload generation.
  @param[in]  NextPayload   The type filled into the Authentication Payload next
                            payload field.
  @param[in]  IsVerify      If it is TURE, the Authentication payload is used for
                            verification.

  @return pointer to IKE Authentication payload for Pre-shared key method.

**/
IKE_PAYLOAD *
Ikev2PskGenerateAuthPayload (
  IN IKEV2_SA_SESSION *IkeSaSession,
  IN IKE_PAYLOAD      *IdPayload,
  IN UINT8            NextPayload,
  IN BOOLEAN          IsVerify
  )
{
  UINT8              *Digest;
  UINTN              DigestSize;
  PRF_DATA_FRAGMENT  Fragments[3];
  UINT8              *KeyBuf;
  UINTN              KeySize;
  IKE_PAYLOAD        *AuthPayload;
  IKEV2_AUTH         *PayloadBuf;
  EFI_STATUS         Status;

  //
  // Auth = Prf(Prf(Secret,"Key Pad for IKEv2),IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r))
  //
  //                           1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Auth Method   !                RESERVED                       !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !                                                               !
  //    ~                      Authentication Data                      ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  KeyBuf      = NULL;
  AuthPayload = NULL;
  Digest      = NULL;

  DigestSize = IpSecGetHmacDigestLength ((UINT8)IkeSaSession->SessionCommon.SaParams->Prf);
  Digest     = AllocateZeroPool (DigestSize);

  if (Digest == NULL) {
    return NULL;
  }
  if (IdPayload == NULL) {
    return NULL;
  }
  //
  // Calcualte Prf(Seceret, "Key Pad for IKEv2");
  //
  Fragments[0].Data     = (UINT8 *) mConstantKey;
  Fragments[0].DataSize = CONSTANT_KEY_SIZE;

  Status = IpSecCryptoIoHmac (
             (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
             IkeSaSession->Pad->Data->AuthData,
             IkeSaSession->Pad->Data->AuthDataSize,
             (HASH_DATA_FRAGMENT *)Fragments,
             1,
             Digest,
             DigestSize
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Store the AuthKey into KeyBuf
  //
  KeyBuf = AllocateZeroPool (DigestSize);
  ASSERT (KeyBuf != NULL);
  CopyMem (KeyBuf, Digest, DigestSize);
  KeySize = DigestSize;

  //
  // Calculate Prf(SK_Pi/r, IDi/r)
  //
  Fragments[0].Data     = IdPayload->PayloadBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER);
  Fragments[0].DataSize = IdPayload->PayloadSize - sizeof (IKEV2_COMMON_PAYLOAD_HEADER);

  if ((IkeSaSession->SessionCommon.IsInitiator && IsVerify) ||
      (!IkeSaSession->SessionCommon.IsInitiator && !IsVerify)
     ) {
     Status = IpSecCryptoIoHmac (
                (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
                IkeSaSession->IkeKeys->SkPrKey,
                IkeSaSession->IkeKeys->SkPrKeySize,
                (HASH_DATA_FRAGMENT *) Fragments,
                1,
                Digest,
                DigestSize
                );
  } else {
    Status = IpSecCryptoIoHmac (
               (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
               IkeSaSession->IkeKeys->SkPiKey,
               IkeSaSession->IkeKeys->SkPiKeySize,
               (HASH_DATA_FRAGMENT *) Fragments,
               1,
               Digest,
               DigestSize
               );
  }
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Copy data to Fragments.
  //
  if ((IkeSaSession->SessionCommon.IsInitiator && IsVerify) ||
      (!IkeSaSession->SessionCommon.IsInitiator && !IsVerify)
     )  {
    Fragments[0].Data     = IkeSaSession->RespPacket;
    Fragments[0].DataSize = IkeSaSession->RespPacketSize;
    Fragments[1].Data     = IkeSaSession->NiBlock;
    Fragments[1].DataSize = IkeSaSession->NiBlkSize;
  } else {
    Fragments[0].Data     = IkeSaSession->InitPacket;
    Fragments[0].DataSize = IkeSaSession->InitPacketSize;
    Fragments[1].Data     = IkeSaSession->NrBlock;
    Fragments[1].DataSize = IkeSaSession->NrBlkSize;
  }

  //
  // Copy the result of Prf(SK_Pr, IDi/r) to Fragments[2].
  //
  Fragments[2].Data     = AllocateZeroPool (DigestSize);
  Fragments[2].DataSize = DigestSize;
  CopyMem (Fragments[2].Data, Digest, DigestSize);

  //
  // Calculate Prf(Key,IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r))
  //
  Status = IpSecCryptoIoHmac (
             (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
             KeyBuf,
             KeySize,
             (HASH_DATA_FRAGMENT *) Fragments,
             3,
             Digest,
             DigestSize
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Allocate buffer for Auth Payload
  //
  AuthPayload               = IkePayloadAlloc ();
  ASSERT (AuthPayload != NULL);

  AuthPayload->PayloadSize  = sizeof (IKEV2_AUTH) + DigestSize;
  PayloadBuf                = (IKEV2_AUTH *) AllocateZeroPool (AuthPayload->PayloadSize);
  ASSERT (PayloadBuf != NULL);
  //
  // Fill in Auth payload.
  //
  PayloadBuf->Header.NextPayload   = NextPayload;
  PayloadBuf->Header.PayloadLength = (UINT16) (AuthPayload->PayloadSize);
  if (IkeSaSession->Pad->Data->AuthMethod == EfiIPsecAuthMethodPreSharedSecret) {
    //
    // Only support Shared Key Message Integrity
    //
    PayloadBuf->AuthMethod = IKEV2_AUTH_METHOD_SKMI;
  } else {
    //
    // Not support other Auth method.
    //
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  //
  // Copy the result of Prf(Key,IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r)) to Auth
  // payload block.
  //
  CopyMem (
    PayloadBuf + 1,
    Digest,
    DigestSize
    );

  //
  // Fill in IKE_PACKET
  //
  AuthPayload->PayloadBuf   = (UINT8 *) PayloadBuf;
  AuthPayload->PayloadType  = IKEV2_PAYLOAD_TYPE_AUTH;

EXIT:
  if (KeyBuf != NULL) {
    FreePool (KeyBuf);
  }
  if (Digest != NULL) {
    FreePool (Digest);
  }
  if (Fragments[2].Data != NULL) {
    //
    // Free the buffer which contains the result of Prf(SK_Pr, IDi/r)
    //
    FreePool (Fragments[2].Data);
  }

  if (EFI_ERROR (Status)) {
    if (AuthPayload != NULL) {
      IkePayloadFree (AuthPayload);
    }
    return NULL;
  } else {
    return AuthPayload;
  }
}

/**
  Generate a Authentication Payload for Certificate Auth method.

  This function has two functions. One is creating a local Authentication
  Payload for sending and other is creating the remote Authentication data
  for verification when the IsVerify is TURE.

  @param[in]  IkeSaSession      Pointer to IKEV2_SA_SESSION related to.
  @param[in]  IdPayload         Pointer to the ID payload to be used for Authentication
                                payload generation.
  @param[in]  NextPayload       The type filled into the Authentication Payload
                                next payload field.
  @param[in]  IsVerify          If it is TURE, the Authentication payload is used
                                for verification.
  @param[in]  UefiPrivateKey    Pointer to the UEFI private key. Ignore it when
                                verify the authenticate payload.
  @param[in]  UefiPrivateKeyLen The size of UefiPrivateKey in bytes. Ignore it
                                when verify the authenticate payload.
  @param[in]  UefiKeyPwd        Pointer to the password of UEFI private key.
                                Ignore it when verify the authenticate payload.
  @param[in]  UefiKeyPwdLen     The size of UefiKeyPwd in bytes.Ignore it when
                                verify the authenticate payload.

  @return pointer to IKE Authentication payload for Cerifitcation method.

**/
IKE_PAYLOAD *
Ikev2CertGenerateAuthPayload (
  IN IKEV2_SA_SESSION *IkeSaSession,
  IN IKE_PAYLOAD      *IdPayload,
  IN UINT8            NextPayload,
  IN BOOLEAN          IsVerify,
  IN UINT8            *UefiPrivateKey,
  IN UINTN            UefiPrivateKeyLen,
  IN UINT8            *UefiKeyPwd,
  IN UINTN            UefiKeyPwdLen
  )
{
  UINT8              *Digest;
  UINTN              DigestSize;
  PRF_DATA_FRAGMENT  Fragments[3];
  UINT8              *KeyBuf;
  IKE_PAYLOAD        *AuthPayload;
  IKEV2_AUTH         *PayloadBuf;
  EFI_STATUS         Status;
  UINT8              *Signature;
  UINTN              SigSize;

  //
  // Auth = Prf(Scert,IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r))
  //
  //                           1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Auth Method   !                RESERVED                       !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !                                                               !
  //    ~                      Authentication Data                      ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  //
  // Initial point
  //
  KeyBuf      = NULL;
  AuthPayload = NULL;
  Digest      = NULL;
  Signature   = NULL;
  SigSize     = 0;

  if (IdPayload == NULL) {
    return NULL;
  }
  DigestSize = IpSecGetHmacDigestLength ((UINT8)IkeSaSession->SessionCommon.SaParams->Prf);
  Digest     = AllocateZeroPool (DigestSize);

  if (Digest == NULL) {
    return NULL;
  }

  //
  // Store the AuthKey into KeyBuf
  //
  KeyBuf  = AllocateZeroPool (DigestSize);
  ASSERT (KeyBuf != NULL);

  CopyMem (KeyBuf, Digest, DigestSize);

  //
  // Calculate Prf(SK_Pi/r, IDi/r)
  //
  Fragments[0].Data     = IdPayload->PayloadBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER);
  Fragments[0].DataSize = IdPayload->PayloadSize - sizeof (IKEV2_COMMON_PAYLOAD_HEADER);

  IpSecDumpBuf ("RestofIDPayload", Fragments[0].Data, Fragments[0].DataSize);

  if ((IkeSaSession->SessionCommon.IsInitiator && IsVerify) ||
      (!IkeSaSession->SessionCommon.IsInitiator && !IsVerify)
     ) {
     Status = IpSecCryptoIoHmac(
                (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
                IkeSaSession->IkeKeys->SkPrKey,
                IkeSaSession->IkeKeys->SkPrKeySize,
                (HASH_DATA_FRAGMENT *) Fragments,
                1,
                Digest,
                DigestSize
                );
    IpSecDumpBuf ("MACedIDForR", Digest, DigestSize);
  } else {
    Status = IpSecCryptoIoHmac (
               (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
               IkeSaSession->IkeKeys->SkPiKey,
               IkeSaSession->IkeKeys->SkPiKeySize,
               (HASH_DATA_FRAGMENT *) Fragments,
               1,
               Digest,
               DigestSize
               );
    IpSecDumpBuf ("MACedIDForI", Digest, DigestSize);
  }
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Copy data to Fragments.
  //
  if ((IkeSaSession->SessionCommon.IsInitiator && IsVerify) ||
      (!IkeSaSession->SessionCommon.IsInitiator && !IsVerify)
     )  {
    Fragments[0].Data     = IkeSaSession->RespPacket;
    Fragments[0].DataSize = IkeSaSession->RespPacketSize;
    Fragments[1].Data     = IkeSaSession->NiBlock;
    Fragments[1].DataSize = IkeSaSession->NiBlkSize;
    IpSecDumpBuf ("RealMessage2", Fragments[0].Data, Fragments[0].DataSize);
    IpSecDumpBuf ("NonceIDdata", Fragments[1].Data, Fragments[1].DataSize);
  } else {
    Fragments[0].Data     = IkeSaSession->InitPacket;
    Fragments[0].DataSize = IkeSaSession->InitPacketSize;
    Fragments[1].Data     = IkeSaSession->NrBlock;
    Fragments[1].DataSize = IkeSaSession->NrBlkSize;
    IpSecDumpBuf ("RealMessage1", Fragments[0].Data, Fragments[0].DataSize);
    IpSecDumpBuf ("NonceRDdata", Fragments[1].Data, Fragments[1].DataSize);
  }

  //
  // Copy the result of Prf(SK_Pr, IDi/r) to Fragments[2].
  //
  Fragments[2].Data     = AllocateZeroPool (DigestSize);
  Fragments[2].DataSize = DigestSize;
  CopyMem (Fragments[2].Data, Digest, DigestSize);

  //
  // Calculate Prf(Key,IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r))
  //
  Status = IpSecCryptoIoHash (
             (UINT8)IkeSaSession->SessionCommon.SaParams->Prf,
             (HASH_DATA_FRAGMENT *) Fragments,
             3,
             Digest,
             DigestSize
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  IpSecDumpBuf ("HashSignedOctects", Digest, DigestSize);
  //
  // Sign the data by the private Key
  //
  if (!IsVerify) {
    IpSecCryptoIoAuthDataWithCertificate (
      Digest,
      DigestSize,
      UefiPrivateKey,
      UefiPrivateKeyLen,
      UefiKeyPwd,
      UefiKeyPwdLen,
      &Signature,
      &SigSize
      );

    if (SigSize == 0 || Signature == NULL) {
      goto EXIT;
    }
  }

  //
  // Allocate buffer for Auth Payload
  //
  AuthPayload = IkePayloadAlloc ();
  ASSERT (AuthPayload != NULL);

  if (!IsVerify) {
    AuthPayload->PayloadSize  = sizeof (IKEV2_AUTH) + SigSize;
  } else {
    AuthPayload->PayloadSize  = sizeof (IKEV2_AUTH) + DigestSize;
  }

  PayloadBuf = (IKEV2_AUTH *) AllocateZeroPool (AuthPayload->PayloadSize);
  ASSERT (PayloadBuf != NULL);
  //
  // Fill in Auth payload.
  //
  PayloadBuf->Header.NextPayload   = NextPayload;
  PayloadBuf->Header.PayloadLength = (UINT16) (AuthPayload->PayloadSize);
  if (IkeSaSession->Pad->Data->AuthMethod == EfiIPsecAuthMethodCertificates) {
      PayloadBuf->AuthMethod = IKEV2_AUTH_METHOD_RSA;
  } else {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  //
  // Copy the result of Prf(Key,IKE_SA_INIi/r|Ni/r|Prf(SK_Pr, IDi/r)) to Auth
  // payload block.
  //
  if (!IsVerify) {
    CopyMem (PayloadBuf + 1, Signature, SigSize);
  } else {
    CopyMem (PayloadBuf + 1, Digest, DigestSize);
  }

  //
  // Fill in IKE_PACKET
  //
  AuthPayload->PayloadBuf   = (UINT8 *) PayloadBuf;
  AuthPayload->PayloadType  = IKEV2_PAYLOAD_TYPE_AUTH;

EXIT:
  if (KeyBuf != NULL) {
    FreePool (KeyBuf);
  }
  if (Digest != NULL) {
    FreePool (Digest);
  }
  if (Signature != NULL) {
    FreePool (Signature);
  }
  if (Fragments[2].Data != NULL) {
    //
    // Free the buffer which contains the result of Prf(SK_Pr, IDi/r)
    //
    FreePool (Fragments[2].Data);
  }

  if (EFI_ERROR (Status)) {
    if (AuthPayload != NULL) {
      IkePayloadFree (AuthPayload);
    }
    return NULL;
  } else {
    return AuthPayload;
  }
}

/**
  Generate TS payload.

  This function generates TSi or TSr payload according to type of next payload.
  If the next payload is Responder TS, gereate TSi Payload. Otherwise, generate
  TSr payload.

  @param[in] ChildSa        Pointer to IKEV2_CHILD_SA_SESSION related to this TS payload.
  @param[in] NextPayload    The payload type presented in the NextPayload field
                            of ID Payload header.
  @param[in] IsTunnel       It indicates that if the Ts Payload is after the CP payload.
                            If yes, it means the Tsi and Tsr payload should be with
                            Max port range and address range and protocol is marked
                            as zero.

  @retval Pointer to Ts IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateTsPayload (
  IN IKEV2_CHILD_SA_SESSION *ChildSa,
  IN UINT8                  NextPayload,
  IN BOOLEAN                IsTunnel
  )
{
  IKE_PAYLOAD        *TsPayload;
  IKEV2_TS           *TsPayloadBuf;
  TRAFFIC_SELECTOR   *TsSelector;
  UINTN              SelectorSize;
  UINTN              TsPayloadSize;
  UINT8              IpVersion;
  UINT8              AddrSize;

  //
  //                           1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Number of TSs !                 RESERVED                      !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !                                                               !
  //    ~                       <Traffic Selectors>                     ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  TsPayload    = IkePayloadAlloc();
  ASSERT (TsPayload != NULL);

  IpVersion    = ChildSa->SessionCommon.UdpService->IpVersion;
  //
  // The Starting Address and Ending Address is variable length depends on
  // is IPv4 or IPv6
  //
  AddrSize      = (UINT8)((IpVersion == IP_VERSION_4) ? sizeof (EFI_IPv4_ADDRESS) : sizeof (EFI_IPv6_ADDRESS));
  SelectorSize  = sizeof (TRAFFIC_SELECTOR) + 2 * AddrSize;
  TsPayloadSize = sizeof (IKEV2_TS) + SelectorSize;
  TsPayloadBuf = AllocateZeroPool (TsPayloadSize);
  ASSERT (TsPayloadBuf != NULL);

  TsPayload->PayloadBuf = (UINT8 *) TsPayloadBuf;
  TsSelector            = (TRAFFIC_SELECTOR*)(TsPayloadBuf + 1);

  TsSelector->TSType = (UINT8)((IpVersion == IP_VERSION_4) ? IKEV2_TS_TYPE_IPV4_ADDR_RANGE : IKEV2_TS_TYPS_IPV6_ADDR_RANGE);

  //
  // For tunnel mode
  //
  if (IsTunnel) {
    TsSelector->IpProtocolId = IKEV2_TS_ANY_PROTOCOL;
    TsSelector->SelecorLen   = (UINT16) SelectorSize;
    TsSelector->StartPort    = 0;
    TsSelector->EndPort      = IKEV2_TS_ANY_PORT;
    ZeroMem ((UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR), AddrSize);
    SetMem  ((UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR) + AddrSize, AddrSize, 0xff);

  } else {
    //
    // TODO: Support port range and address range
    //
    if (NextPayload == IKEV2_PAYLOAD_TYPE_TS_RSP){
      //
      // Create initiator Traffic Selector
      //
      TsSelector->SelecorLen   = (UINT16)SelectorSize;

      //
      // Currently only support the port range from 0~0xffff. Don't support other
      // port range.
      // TODO: support Port range
      //
      if (ChildSa->SessionCommon.IsInitiator) {
        if (ChildSa->Spd->Selector->LocalPort != 0 &&
            ChildSa->Spd->Selector->LocalPortRange == 0) {
          //
          // For not port range.
          //
          TsSelector->StartPort = ChildSa->Spd->Selector->LocalPort;
          TsSelector->EndPort   = ChildSa->Spd->Selector->LocalPort;
        } else if (ChildSa->Spd->Selector->LocalPort == 0){
          //
          // For port from 0~0xffff
          //
          TsSelector->StartPort = 0;
          TsSelector->EndPort   = IKEV2_TS_ANY_PORT;
        } else {
          //
          // Not support now.
          //
          goto ON_ERROR;
        }
      } else {
        if (ChildSa->Spd->Selector->RemotePort != 0 &&
            ChildSa->Spd->Selector->RemotePortRange == 0) {
          //
          // For not port range.
          //
          TsSelector->StartPort = ChildSa->Spd->Selector->RemotePort;
          TsSelector->EndPort   = ChildSa->Spd->Selector->RemotePort;
        } else if (ChildSa->Spd->Selector->RemotePort == 0) {
          //
          // For port from 0~0xffff
          //
          TsSelector->StartPort = 0;
          TsSelector->EndPort   = IKEV2_TS_ANY_PORT;
        } else {
          //
          // Not support now.
          //
          goto ON_ERROR;
        }
      }
      //
      // Copy Address.Currently the address range is not supported.
      // The Starting address is same as Ending address
      // TODO: Support Address Range.
      //
      CopyMem (
        (UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR),
        ChildSa->SessionCommon.IsInitiator ?
        ChildSa->Spd->Selector->LocalAddress :
        ChildSa->Spd->Selector->RemoteAddress,
        AddrSize
        );
      CopyMem (
        (UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR) + AddrSize,
        ChildSa->SessionCommon.IsInitiator ?
        ChildSa->Spd->Selector->LocalAddress :
        ChildSa->Spd->Selector->RemoteAddress,
        AddrSize
        );
      //
      // If the Next Payload is not TS responder, this TS payload type is the TS responder.
      //
      TsPayload->PayloadType             = IKEV2_PAYLOAD_TYPE_TS_INIT;
    }else{
        //
        // Create responder Traffic Selector
        //
        TsSelector->SelecorLen   = (UINT16)SelectorSize;

        //
        // Currently only support the port range from 0~0xffff. Don't support other
        // port range.
        // TODO: support Port range
        //
        if (!ChildSa->SessionCommon.IsInitiator) {
          if (ChildSa->Spd->Selector->LocalPort != 0 &&
              ChildSa->Spd->Selector->LocalPortRange == 0) {
            //
            // For not port range.
            //
            TsSelector->StartPort = ChildSa->Spd->Selector->LocalPort;
            TsSelector->EndPort   = ChildSa->Spd->Selector->LocalPort;
          } else if (ChildSa->Spd->Selector->LocalPort == 0){
            //
            // For port from 0~0xffff
            //
            TsSelector->StartPort = 0;
            TsSelector->EndPort   = IKEV2_TS_ANY_PORT;
          } else {
            //
            // Not support now.
            //
            goto ON_ERROR;
          }
        } else {
          if (ChildSa->Spd->Selector->RemotePort != 0 &&
              ChildSa->Spd->Selector->RemotePortRange == 0) {
            //
            // For not port range.
            //
            TsSelector->StartPort = ChildSa->Spd->Selector->RemotePort;
            TsSelector->EndPort   = ChildSa->Spd->Selector->RemotePort;
          } else if (ChildSa->Spd->Selector->RemotePort == 0){
            //
            // For port from 0~0xffff
            //
            TsSelector->StartPort = 0;
            TsSelector->EndPort   = IKEV2_TS_ANY_PORT;
          } else {
            //
            // Not support now.
            //
            goto ON_ERROR;
          }
        }
        //
        // Copy Address.Currently the address range is not supported.
        // The Starting address is same as Ending address
        // TODO: Support Address Range.
        //
        CopyMem (
          (UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR),
          ChildSa->SessionCommon.IsInitiator ?
          ChildSa->Spd->Selector->RemoteAddress :
          ChildSa->Spd->Selector->LocalAddress,
          AddrSize
          );
        CopyMem (
          (UINT8*)TsSelector + sizeof(TRAFFIC_SELECTOR) + AddrSize,
          ChildSa->SessionCommon.IsInitiator ?
          ChildSa->Spd->Selector->RemoteAddress :
          ChildSa->Spd->Selector->LocalAddress,
          AddrSize
          );
        //
        // If the Next Payload is not TS responder, this TS payload type is the TS responder.
        //
        TsPayload->PayloadType          = IKEV2_PAYLOAD_TYPE_TS_RSP;
      }
    }

    if (ChildSa->Spd->Selector->NextLayerProtocol != 0xffff) {
      TsSelector->IpProtocolId = (UINT8)ChildSa->Spd->Selector->NextLayerProtocol;
    } else {
      TsSelector->IpProtocolId = IKEV2_TS_ANY_PROTOCOL;
    }

  TsPayloadBuf->Header.NextPayload    = NextPayload;
  TsPayloadBuf->Header.PayloadLength  = (UINT16)TsPayloadSize;
  TsPayloadBuf->TSNumbers             = 1;
  TsPayload->PayloadSize              = TsPayloadSize;
  goto ON_EXIT;

ON_ERROR:
  if (TsPayload != NULL) {
    IkePayloadFree (TsPayload);
    TsPayload = NULL;
  }
ON_EXIT:
  return TsPayload;
}

/**
  Generate the Notify payload.

  Since the structure of Notify payload which defined in RFC 4306 is simple, so
  there is no internal data structure for Notify payload. This function generate
  Notify payload defined in RFC 4306, but all the fields in this payload are still
  in host order and need call Ikev2EncodePayload() to convert those fields from
  the host order to network order beforing sending it.

  @param[in]  ProtocolId        The protocol type ID. For IKE_SA it MUST be one (1).
                                For IPsec SAs it MUST be neither (2) for AH or (3)
                                for ESP.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of
                                the Notify payload.
  @param[in]  SpiSize           Size of the SPI in SPI size field of the Notify Payload.
  @param[in]  MessageType       The message type in NotifyMessageType field of the
                                Notify Payload.
  @param[in]  SpiBuf            Pointer to buffer contains the SPI value.
  @param[in]  NotifyData        Pointer to buffer contains the notification data.
  @param[in]  NotifyDataSize    The size of NotifyData in bytes.


  @retval Pointer to IKE Notify Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateNotifyPayload (
  IN UINT8            ProtocolId,
  IN UINT8            NextPayload,
  IN UINT8            SpiSize,
  IN UINT16           MessageType,
  IN UINT8            *SpiBuf,
  IN UINT8            *NotifyData,
  IN UINTN            NotifyDataSize
  )
{
  IKE_PAYLOAD         *NotifyPayload;
  IKEV2_NOTIFY        *Notify;
  UINT16              NotifyPayloadLen;
  UINT8               *MessageData;

  //                       1                   2                   3
  //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  !  Protocol ID  !   SPI Size    !      Notify Message Type      !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  !                                                               !
  //  ~                Security Parameter Index (SPI)                 ~
  //  !                                                               !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  !                                                               !
  //  ~                       Notification Data                       ~
  //  !                                                               !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  //
  NotifyPayloadLen  = (UINT16) (sizeof (IKEV2_NOTIFY) + NotifyDataSize + SpiSize);
  Notify            = (IKEV2_NOTIFY *) AllocateZeroPool (NotifyPayloadLen);
  ASSERT (Notify != NULL);

  //
  // Set Delete Payload's Generic Header
  //
  Notify->Header.NextPayload    = NextPayload;
  Notify->Header.PayloadLength  = NotifyPayloadLen;
  Notify->SpiSize               = SpiSize;
  Notify->ProtocolId            = ProtocolId;
  Notify->MessageType           = MessageType;

  //
  // Copy Spi , for Cookie Notify, there is no SPI.
  //
  if (SpiBuf != NULL && SpiSize != 0 ) {
    CopyMem (Notify + 1, SpiBuf, SpiSize);
  }

  MessageData = ((UINT8 *) (Notify + 1)) + SpiSize;

  //
  // Copy Notification Data
  //
  if (NotifyDataSize != 0) {
    CopyMem (MessageData, NotifyData, NotifyDataSize);
  }

  //
  // Create Payload for and set type as IKEV2_PAYLOAD_TYPE_NOTIFY
  //
  NotifyPayload = IkePayloadAlloc ();
  ASSERT (NotifyPayload != NULL);
  NotifyPayload->PayloadType  = IKEV2_PAYLOAD_TYPE_NOTIFY;
  NotifyPayload->PayloadBuf   = (UINT8 *) Notify;
  NotifyPayload->PayloadSize  = NotifyPayloadLen;
  return NotifyPayload;
}

/**
  Generate the Delete payload.

  Since the structure of Delete payload which defined in RFC 4306 is simple,
  there is no internal data structure for Delete payload. This function generate
  Delete payload defined in RFC 4306, but all the fields in this payload are still
  in host order and need call Ikev2EncodePayload() to convert those fields from
  the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used of Delete payload generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of
                                the Delete payload.
  @param[in]  SpiSize           Size of the SPI in SPI size field of the Delete Payload.
  @param[in]  SpiNum            Number of SPI in NumofSPIs field of the Delete Payload.
  @param[in]  SpiBuf            Pointer to buffer contains the SPI value.

  @retval a Pointer of IKE Delete Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateDeletePayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             SpiSize,
  IN UINT16            SpiNum,
  IN UINT8             *SpiBuf

  )
{
  IKE_PAYLOAD  *DelPayload;
  IKEV2_DELETE *Del;
  UINT16       SpiBufSize;
  UINT16       DelPayloadLen;

  //                         1                   2                   3
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  ! Protocol ID   !   SPI Size    !           # of SPIs           !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  !                                                               !
  //  ~               Security Parameter Index(es) (SPI)              ~
  //  !                                                               !
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  SpiBufSize    = (UINT16) (SpiSize * SpiNum);
  if (SpiBufSize != 0 && SpiBuf == NULL) {
    return NULL;
  }

  DelPayloadLen = (UINT16) (sizeof (IKEV2_DELETE) + SpiBufSize);

  Del           = AllocateZeroPool (DelPayloadLen);
  ASSERT (Del != NULL);

  //
  // Set Delete Payload's Generic Header
  //
  Del->Header.NextPayload   = NextPayload;
  Del->Header.PayloadLength = DelPayloadLen;
  Del->NumSpis              = SpiNum;
  Del->SpiSize              = SpiSize;

  if (SpiSize == 4) {
    //
    // TODO: should consider the AH if needs to support.
    //
    Del->ProtocolId = IPSEC_PROTO_IPSEC_ESP;
  } else {
    Del->ProtocolId = IPSEC_PROTO_ISAKMP;
  }

  //
  // Set Del Payload's Idntification Data
  //
  CopyMem (Del + 1, SpiBuf, SpiBufSize);
  DelPayload = IkePayloadAlloc ();
  ASSERT (DelPayload != NULL);
  DelPayload->PayloadType = IKEV2_PAYLOAD_TYPE_DELETE;
  DelPayload->PayloadBuf  = (UINT8 *) Del;
  DelPayload->PayloadSize = DelPayloadLen;
  return DelPayload;
}

/**
  Generate the Configuration payload.

  This function generate configuration payload defined in RFC 4306, but all the
  fields in this payload are still in host order and need call Ikev2EncodePayload()
  to convert those fields from the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used for Delete payload
                                generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of
                                the Delete payload.
  @param[in]  CfgType           The attribute type in the Configuration attribute.

  @retval Pointer to IKE CP Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCpPayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             CfgType
  )
{
  IKE_PAYLOAD           *CpPayload;
  IKEV2_CFG             *Cfg;
  UINT16                PayloadLen;
  IKEV2_CFG_ATTRIBUTES  *CfgAttributes;

  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C! RESERVED    !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !   CFG Type    !                    RESERVED                   !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    !                                                               !
  //    ~                   Configuration Attributes                    ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  PayloadLen = (UINT16) (sizeof (IKEV2_CFG) + sizeof (IKEV2_CFG_ATTRIBUTES));
  Cfg        = (IKEV2_CFG *) AllocateZeroPool (PayloadLen);

  if (Cfg == NULL) {
    return NULL;
  }

  CfgAttributes = (IKEV2_CFG_ATTRIBUTES *)((UINT8 *)Cfg + sizeof (IKEV2_CFG));

  //
  // Only generate the configuration payload with an empty INTERNAL_IP4_ADDRESS
  // or INTERNAL_IP6_ADDRESS.
  //

  Cfg->Header.NextPayload   = NextPayload;
  Cfg->Header.PayloadLength = PayloadLen;
  Cfg->CfgType              = IKEV2_CFG_TYPE_REQUEST;

  CfgAttributes->AttritType  = CfgType;
  CfgAttributes->ValueLength = 0;

  CpPayload = IkePayloadAlloc ();
  if (CpPayload == NULL) {
    if (Cfg != NULL) {
      FreePool (Cfg);
    }
    return NULL;
  }

  CpPayload->PayloadType = IKEV2_PAYLOAD_TYPE_CP;
  CpPayload->PayloadBuf  = (UINT8 *) Cfg;
  CpPayload->PayloadSize = PayloadLen;
  return CpPayload;
}

/**
  Parser the Notify Cookie payload.

  This function parses the Notify Cookie payload.If the Notify ProtocolId is not
  IPSEC_PROTO_ISAKMP or if the SpiSize is not zero or if the MessageType is not
  the COOKIE, return EFI_INVALID_PARAMETER.

  @param[in]      IkeNCookie    Pointer to the IKE_PAYLOAD which contians the
                                Notify Cookie payload.
                                the Notify payload.
  @param[in, out] IkeSaSession  Pointer to the relevant IKE SA Session.

  @retval EFI_SUCCESS           The Notify Cookie Payload is valid.
  @retval EFI_INVALID_PARAMETER The Notify Cookie Payload is invalid.
  @retval EFI_OUT_OF_RESOURCE   The required resource can't be allocated.

**/
EFI_STATUS
Ikev2ParserNotifyCookiePayload (
  IN     IKE_PAYLOAD      *IkeNCookie,
  IN OUT IKEV2_SA_SESSION *IkeSaSession
  )
{
  IKEV2_NOTIFY      *NotifyPayload;
  UINTN             NotifyDataSize;

  NotifyPayload = (IKEV2_NOTIFY *)IkeNCookie->PayloadBuf;

  if ((NotifyPayload->ProtocolId != IPSEC_PROTO_ISAKMP) ||
      (NotifyPayload->SpiSize != 0) ||
      (NotifyPayload->MessageType != IKEV2_NOTIFICATION_COOKIE)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  NotifyDataSize        = NotifyPayload->Header.PayloadLength - sizeof (IKEV2_NOTIFY);
  IkeSaSession->NCookie = AllocateZeroPool (NotifyDataSize);
  if (IkeSaSession->NCookie == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IkeSaSession->NCookieSize = NotifyDataSize;

  CopyMem (
    IkeSaSession->NCookie,
    (UINT8 *)NotifyPayload + sizeof (IKEV2_NOTIFY),
    NotifyDataSize
    );

  return EFI_SUCCESS;
}


/**
  Generate the Certificate payload or Certificate Request Payload.

  Since the Certificate Payload structure is same with Certificate Request Payload,
  the only difference is that one contains the Certificate Data, other contains
  the acceptable certificateion CA. This function generate Certificate payload
  or Certificate Request Payload defined in RFC 4306, but all the fields
  in the payload are still in host order and need call Ikev2EncodePayload()
  to convert those fields from the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used of Delete payload
                                generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of
                                the Delete payload.
  @param[in]  Certificate       Pointer of buffer contains the certification data.
  @param[in]  CertificateLen    The length of Certificate in byte.
  @param[in]  EncodeType        Specified the Certificate Encodeing which is defined
                                in RFC 4306.
  @param[in]  IsRequest         To indicate create Certificate Payload or Certificate
                                Request Payload. If it is TURE, create Certificate
                                Payload. Otherwise, create Certificate Request Payload.

  @retval  a Pointer to IKE Payload whose payload buffer containing the Certificate
           payload or Certificated Request payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCertificatePayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             *Certificate,
  IN UINTN             CertificateLen,
  IN UINT8             EncodeType,
  IN BOOLEAN           IsRequest
  )
{
  IKE_PAYLOAD           *CertPayload;
  IKEV2_CERT            *Cert;
  UINT16                PayloadLen;
  UINT8                 *PublicKey;
  UINTN                 PublicKeyLen;
  HASH_DATA_FRAGMENT    Fragment[1];
  UINT8                 *HashData;
  UINTN                 HashDataSize;
  EFI_STATUS            Status;

  //
  //                         1                   2                   3
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Next Payload  !C!  RESERVED   !         Payload Length        !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    ! Cert Encoding !                                               !
  //    +-+-+-+-+-+-+-+-+                                               !
  //    ~                       Certificate Data/Authority              ~
  //    !                                                               !
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  Status       = EFI_SUCCESS;
  PublicKey    = NULL;
  PublicKeyLen = 0;

  if (!IsRequest) {
    PayloadLen = (UINT16) (sizeof (IKEV2_CERT) + CertificateLen);
  } else {
    //
    // SHA1 Hash length is 20.
    //
    PayloadLen = (UINT16) (sizeof (IKEV2_CERT) + 20);
  }

  Cert = AllocateZeroPool (PayloadLen);
  if (Cert == NULL) {
    return NULL;
  }

  //
  // Generate Certificate Payload or Certificate Request Payload.
  //
  Cert->Header.NextPayload   = NextPayload;
  Cert->Header.PayloadLength = PayloadLen;
  Cert->CertEncoding         = EncodeType;
  if (!IsRequest) {
    CopyMem (
      ((UINT8 *)Cert) + sizeof (IKEV2_CERT),
      Certificate,
      CertificateLen
      );
  } else {
    Status = IpSecCryptoIoGetPublicKeyFromCert (
               Certificate,
               CertificateLen,
               &PublicKey,
               &PublicKeyLen
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Fragment[0].Data     = PublicKey;
    Fragment[0].DataSize = PublicKeyLen;
    HashDataSize      = IpSecGetHmacDigestLength (IKE_AALG_SHA1HMAC);
    HashData          = AllocateZeroPool (HashDataSize);
    if (HashData == NULL) {
      goto ON_EXIT;
    }

    Status = IpSecCryptoIoHash (
               IKE_AALG_SHA1HMAC,
               Fragment,
               1,
               HashData,
               HashDataSize
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    CopyMem (
      ((UINT8 *)Cert) + sizeof (IKEV2_CERT),
      HashData,
      HashDataSize
      );
  }

  CertPayload = IkePayloadAlloc ();
  if (CertPayload == NULL) {
    goto ON_EXIT;
  }

  if (!IsRequest) {
    CertPayload->PayloadType = IKEV2_PAYLOAD_TYPE_CERT;
  } else {
    CertPayload->PayloadType = IKEV2_PAYLOAD_TYPE_CERTREQ;
  }

  CertPayload->PayloadBuf  = (UINT8 *) Cert;
  CertPayload->PayloadSize = PayloadLen;
  return CertPayload;

ON_EXIT:
  if (Cert != NULL) {
    FreePool (Cert);
  }
  if (PublicKey != NULL) {
    FreePool (PublicKey);
  }
  return NULL;
}

/**
  Remove and free all IkePayloads in the specified IkePacket.

  @param[in] IkePacket   The pointer of IKE_PACKET.

**/
VOID
ClearAllPayloads (
  IN IKE_PACKET     *IkePacket
  )
{
  LIST_ENTRY      *PayloadEntry;
  IKE_PAYLOAD     *IkePayload;
  //
  // remove all payloads from list and free each payload.
  //
  while (!IsListEmpty (&IkePacket->PayloadList)) {
    PayloadEntry  = IkePacket->PayloadList.ForwardLink;
    IkePayload    = IKE_PAYLOAD_BY_PACKET (PayloadEntry);
    IKE_PACKET_REMOVE_PAYLOAD (IkePacket, IkePayload);
    IkePayloadFree (IkePayload);
  }
}

/**
  Transfer the intrnal data structure IKEV2_SA_DATA to IKEV2_SA structure defined in RFC.

  @param[in] SessionCommon Pointer to IKEV2_SESSION_COMMON related to the SA Session.
  @param[in] SaData        Pointer to IKEV2_SA_DATA to be transfered.

  @retval  return the pointer of IKEV2_SA.

**/
IKEV2_SA*
Ikev2EncodeSa (
  IN IKEV2_SESSION_COMMON *SessionCommon,
  IN IKEV2_SA_DATA        *SaData
  )
{
  IKEV2_SA              *Sa;
  UINTN                 SaSize;
  IKEV2_PROPOSAL_DATA   *ProposalData;
  IKEV2_TRANSFORM_DATA  *TransformData;
  UINTN                 TotalTransforms;
  UINTN                 SaAttrsSize;
  UINTN                 TransformsSize;
  UINTN                 TransformSize;
  UINTN                 ProposalsSize;
  UINTN                 ProposalSize;
  UINTN                 ProposalIndex;
  UINTN                 TransformIndex;
  IKE_SA_ATTRIBUTE      *SaAttribute;
  IKEV2_PROPOSAL        *Proposal;
  IKEV2_TRANSFORM       *Transform;

  //
  // Transform IKE_SA_DATA structure to IKE_SA Payload.
  // Header length is host order.
  // The returned IKE_SA struct should be freed by caller.
  //
  TotalTransforms = 0;
  //
  // Calculate the Proposal numbers and Transform numbers.
  //
  for (ProposalIndex = 0; ProposalIndex < SaData->NumProposals; ProposalIndex++) {

    ProposalData     = (IKEV2_PROPOSAL_DATA *) (SaData + 1) + ProposalIndex;
    TotalTransforms += ProposalData->NumTransforms;

  }
  SaSize = sizeof (IKEV2_SA) +
           SaData->NumProposals * sizeof (IKEV2_PROPOSAL) +
           TotalTransforms * (sizeof (IKEV2_TRANSFORM) + MAX_SA_ATTRS_SIZE);
  //
  // Allocate buffer for IKE_SA.
  //
  Sa = AllocateZeroPool (SaSize);
  ASSERT (Sa != NULL);
  CopyMem (Sa, SaData, sizeof (IKEV2_SA));
  Sa->Header.PayloadLength  = (UINT16) sizeof (IKEV2_SA);
  ProposalsSize             = 0;
  Proposal                  = (IKEV2_PROPOSAL *) (Sa + 1);

  //
  // Set IKE_PROPOSAL
  //
  ProposalData  = (IKEV2_PROPOSAL_DATA *) (SaData + 1);
  for (ProposalIndex = 0; ProposalIndex < SaData->NumProposals; ProposalIndex++) {
    Proposal->ProposalIndex   = ProposalData->ProposalIndex;
    Proposal->ProtocolId      = ProposalData->ProtocolId;
    Proposal->NumTransforms   = ProposalData->NumTransforms;

    if (ProposalData->Spi == 0) {
      Proposal->SpiSize = 0;
    } else {
      Proposal->SpiSize           = 4;
      *(UINT32 *) (Proposal + 1)  = HTONL (*((UINT32*)ProposalData->Spi));
    }

    TransformsSize  = 0;
    Transform       = (IKEV2_TRANSFORM *) ((UINT8 *) (Proposal + 1) + Proposal->SpiSize);

    //
    // Set IKE_TRANSFORM
    //
    for (TransformIndex = 0; TransformIndex < ProposalData->NumTransforms; TransformIndex++) {
      TransformData               = (IKEV2_TRANSFORM_DATA *) (ProposalData + 1) + TransformIndex;
      Transform->TransformType    = TransformData->TransformType;
      Transform->TransformId      = HTONS (TransformData->TransformId);
      SaAttrsSize                 = 0;

      //
      // If the Encryption Algorithm is variable key length set the key length in attribute.
      // Note that only a single attribute type (Key Length) is defined and it is fixed length.
      //
      if (Transform->TransformType == IKEV2_TRANSFORM_TYPE_ENCR && TransformData->Attribute.Attr.AttrValue != 0) {
        SaAttribute                 = (IKE_SA_ATTRIBUTE *) (Transform + 1);
        SaAttribute->AttrType       = HTONS (IKEV2_ATTRIBUTE_TYPE_KEYLEN | SA_ATTR_FORMAT_BIT);
        SaAttribute->Attr.AttrValue = HTONS (TransformData->Attribute.Attr.AttrValue);
        SaAttrsSize                 = sizeof (IKE_SA_ATTRIBUTE);
      }

      //
      // If the Integrity Algorithm is variable key length set the key length in attribute.
      //
      if (Transform->TransformType == IKEV2_TRANSFORM_TYPE_INTEG && TransformData->Attribute.Attr.AttrValue != 0) {
        SaAttribute                 = (IKE_SA_ATTRIBUTE *) (Transform + 1);
        SaAttribute->AttrType       = HTONS (IKEV2_ATTRIBUTE_TYPE_KEYLEN | SA_ATTR_FORMAT_BIT);
        SaAttribute->Attr.AttrValue = HTONS (TransformData->Attribute.Attr.AttrValue);
        SaAttrsSize                 = sizeof (IKE_SA_ATTRIBUTE);
      }

      TransformSize                 = sizeof (IKEV2_TRANSFORM) + SaAttrsSize;
      TransformsSize               += TransformSize;

      Transform->Header.NextPayload   = IKE_TRANSFORM_NEXT_PAYLOAD_MORE;
      Transform->Header.PayloadLength = HTONS ((UINT16)TransformSize);

      if (TransformIndex == (UINTN)(ProposalData->NumTransforms - 1)) {
        Transform->Header.NextPayload = IKE_TRANSFORM_NEXT_PAYLOAD_NONE;
      }

      Transform     = (IKEV2_TRANSFORM *)((UINT8 *) Transform + TransformSize);
    }

    //
    // Set Proposal's Generic Header.
    //
    ProposalSize                   = sizeof (IKEV2_PROPOSAL) + Proposal->SpiSize + TransformsSize;
    ProposalsSize                 += ProposalSize;
    Proposal->Header.NextPayload   = IKE_PROPOSAL_NEXT_PAYLOAD_MORE;
    Proposal->Header.PayloadLength = HTONS ((UINT16)ProposalSize);

    if (ProposalIndex == (UINTN)(SaData->NumProposals - 1)) {
      Proposal->Header.NextPayload = IKE_PROPOSAL_NEXT_PAYLOAD_NONE;
    }

    //
    // Point to next Proposal Payload
    //
    Proposal     = (IKEV2_PROPOSAL *) ((UINT8 *) Proposal + ProposalSize);
    ProposalData = (IKEV2_PROPOSAL_DATA *)(((UINT8 *)ProposalData) + sizeof (IKEV2_PROPOSAL_DATA) + (TransformIndex * sizeof (IKEV2_TRANSFORM_DATA)));
  }
  //
  // Set SA's Generic Header.
  //
  Sa->Header.PayloadLength = (UINT16) (Sa->Header.PayloadLength + ProposalsSize);
  return Sa;
}

/**
  Decode SA payload.

  This function converts the received SA payload to internal data structure.

  @param[in]  SessionCommon       Pointer to IKE Common Session used to decode the SA
                                  Payload.
  @param[in]  Sa                  Pointer to SA Payload

  @return a Pointer to internal data structure for SA payload.

**/
IKEV2_SA_DATA *
Ikev2DecodeSa (
  IN IKEV2_SESSION_COMMON *SessionCommon,
  IN IKEV2_SA             *Sa
  )
{
  IKEV2_SA_DATA         *SaData;
  EFI_STATUS            Status;
  IKEV2_PROPOSAL        *Proposal;
  IKEV2_TRANSFORM       *Transform;
  UINTN                 TotalProposals;
  UINTN                 TotalTransforms;
  UINTN                 ProposalNextPayloadSum;
  UINTN                 ProposalIndex;
  UINTN                 TransformIndex;
  UINTN                 SaRemaining;
  UINT16                ProposalSize;
  UINTN                 ProposalRemaining;
  UINT16                TransformSize;
  UINTN                 SaAttrRemaining;
  IKE_SA_ATTRIBUTE      *SaAttribute;
  IKEV2_PROPOSAL_DATA   *ProposalData;
  IKEV2_TRANSFORM_DATA  *TransformData;
  UINT8                 *Spi;

  //
  // Transfrom from IKE_SA payload to IKE_SA_DATA structure.
  // Header length NTOH is already done
  // The returned IKE_SA_DATA should be freed by caller
  //
  SaData    = NULL;
  Status    = EFI_SUCCESS;

  //
  // First round sanity check and size calculae
  //
  TotalProposals         = 0;
  TotalTransforms        = 0;
  ProposalNextPayloadSum = 0;
  SaRemaining            = Sa->Header.PayloadLength - sizeof (IKEV2_SA);// Point to current position in SA
  Proposal               = (IKEV2_PROPOSAL *)((IKEV2_SA *)(Sa)+1);

  //
  // Calculate the number of Proposal payload and the total numbers of
  // Transforms payload (the transforms in all proposal payload).
  //
  while (SaRemaining > sizeof (IKEV2_PROPOSAL)) {
    ProposalSize = NTOHS (Proposal->Header.PayloadLength);
    if (SaRemaining < ProposalSize) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    if (Proposal->SpiSize != 0 && Proposal->SpiSize != 4) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    TotalProposals++;
    TotalTransforms        += Proposal->NumTransforms;
    SaRemaining            -= ProposalSize;
    ProposalNextPayloadSum += Proposal->Header.NextPayload;
    Proposal                = IKEV2_NEXT_PROPOSAL_WITH_SIZE (Proposal, ProposalSize);
  }

  //
  // Check the proposal number.
  // The proposal Substructure, the NextPayLoad field indicates : 0 (last) or 2 (more)
  // which Specifies whether this is the last Proposal Substructure in the SA.
  // Here suming all Proposal NextPayLoad field to check the proposal number is correct
  // or not.
  //
  if (TotalProposals == 0 ||
      (TotalProposals - 1) * IKE_PROPOSAL_NEXT_PAYLOAD_MORE != ProposalNextPayloadSum
      ) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Second round sanity check and decode. Transform the SA payload into
  // a IKE_SA_DATA structure.
  //
  SaData = (IKEV2_SA_DATA *) AllocateZeroPool (
                               sizeof (IKEV2_SA_DATA) +
                               TotalProposals * sizeof (IKEV2_PROPOSAL_DATA) +
                               TotalTransforms * sizeof (IKEV2_TRANSFORM_DATA)
                               );
  ASSERT (SaData != NULL);
  CopyMem (SaData, Sa, sizeof (IKEV2_SA));
  SaData->NumProposals        = TotalProposals;
  ProposalData                = (IKEV2_PROPOSAL_DATA *) (SaData + 1);

  //
  // Proposal Payload
  //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   ! Next Payload  !   RESERVED    !         Payload Length        !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !  Proposal #   !  Protocol-Id  !    SPI Size   !# of Transforms!
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   !                        SPI (variable)                         !
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //
  for (ProposalIndex = 0, Proposal = IKEV2_SA_FIRST_PROPOSAL (Sa);
       ProposalIndex < TotalProposals;
       ProposalIndex++
       ) {

    //
    // TODO: check ProposalId
    //
    ProposalData->ProposalIndex   = Proposal->ProposalIndex;
    ProposalData->ProtocolId      = Proposal->ProtocolId;
    if (Proposal->SpiSize == 0) {
      ProposalData->Spi = 0;
    } else {
      //
      // SpiSize == 4
      //
      Spi = AllocateZeroPool (Proposal->SpiSize);
      ASSERT (Spi != NULL);
      CopyMem (Spi, (UINT32 *) (Proposal + 1), Proposal->SpiSize);
      *((UINT32*) Spi) = NTOHL (*((UINT32*) Spi));
      ProposalData->Spi = Spi;
    }

    ProposalData->NumTransforms = Proposal->NumTransforms;
    ProposalSize                = NTOHS (Proposal->Header.PayloadLength);
    ProposalRemaining           = ProposalSize;
    //
    // Transform Payload
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //   ! Next Payload  !   RESERVED    !         Payload Length        !
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //   !Transform Type !   RESERVED    !         Transform ID          !
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //   !                                                               !
    //   ~                        SA Attributes                          ~
    //   !                                                               !
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    Transform = IKEV2_PROPOSAL_FIRST_TRANSFORM (Proposal);
    for (TransformIndex = 0; TransformIndex < Proposal->NumTransforms; TransformIndex++) {

      //
      // Transfer the IKEV2_TRANSFORM structure into internal IKEV2_TRANSFORM_DATA struture.
      //
      TransformData                   = (IKEV2_TRANSFORM_DATA *) (ProposalData + 1) + TransformIndex;
      TransformData->TransformId      = NTOHS (Transform->TransformId);
      TransformData->TransformType    = Transform->TransformType;
      TransformSize                   = NTOHS (Transform->Header.PayloadLength);
      //
      // Check the Proposal Data is correct.
      //
      if (ProposalRemaining < TransformSize) {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
      }

      //
      // Check if the Transform payload includes Attribution.
      //
      SaAttrRemaining = TransformSize - sizeof (IKEV2_TRANSFORM);

      //
      // According to RFC 4603, currently only the Key length attribute type is
      // supported. For each Transform, there is only one attributeion.
      //
      if (SaAttrRemaining > 0) {
        if (SaAttrRemaining != sizeof (IKE_SA_ATTRIBUTE)) {
          Status = EFI_INVALID_PARAMETER;
          goto Exit;
        }
        SaAttribute                             = (IKE_SA_ATTRIBUTE *) ((IKEV2_TRANSFORM *)(Transform) + 1);
        TransformData->Attribute.AttrType       = (UINT16)((NTOHS (SaAttribute->AttrType))  & ~SA_ATTR_FORMAT_BIT);
        TransformData->Attribute.Attr.AttrValue = NTOHS (SaAttribute->Attr.AttrValue);

        //
        // Currently, only supports the Key Length Attribution.
        //
        if (TransformData->Attribute.AttrType != IKEV2_ATTRIBUTE_TYPE_KEYLEN) {
          Status = EFI_INVALID_PARAMETER;
          goto Exit;
        }
      }

      //
      // Move to next Transform
      //
      Transform = IKEV2_NEXT_TRANSFORM_WITH_SIZE (Transform, TransformSize);
    }
    Proposal     = IKEV2_NEXT_PROPOSAL_WITH_SIZE (Proposal, ProposalSize);
    ProposalData = (IKEV2_PROPOSAL_DATA *) ((UINT8 *)(ProposalData + 1) +
                                                ProposalData->NumTransforms *
                                                sizeof (IKEV2_TRANSFORM_DATA));
  }

Exit:
  if (EFI_ERROR (Status) && SaData != NULL) {
    FreePool (SaData);
    SaData = NULL;
  }
  return SaData;
}

/**
  General interface of payload encoding.

  This function encodes the internal data structure into payload which
  is defined in RFC 4306. The IkePayload->PayloadBuf is used to store both the input
  payload and converted payload. Only the SA payload use the interal structure
  to store the attribute. Other payload use structure which is same with the RFC
  defined, for this kind payloads just do host order to network order change of
  some fields.

  @param[in]      SessionCommon       Pointer to IKE Session Common used to encode the payload.
  @param[in, out] IkePayload          Pointer to IKE payload to be encoded as input, and
                                      store the encoded result as output.

  @retval EFI_INVALID_PARAMETER  Meet error when encoding the SA payload.
  @retval EFI_SUCCESS            Encoded successfully.

**/
EFI_STATUS
Ikev2EncodePayload (
  IN     UINT8               *SessionCommon,
  IN OUT IKE_PAYLOAD         *IkePayload
  )
{
  IKEV2_SA_DATA               *SaData;
  IKEV2_SA                    *SaPayload;
  IKEV2_COMMON_PAYLOAD_HEADER *PayloadHdr;
  IKEV2_NOTIFY                *NotifyPayload;
  IKEV2_DELETE                *DeletePayload;
  IKEV2_KEY_EXCHANGE          *KeyPayload;
  IKEV2_TS                    *TsPayload;
  IKEV2_CFG_ATTRIBUTES        *CfgAttribute;
  UINT8                       *TsBuffer;
  UINT8                       Index;
  TRAFFIC_SELECTOR            *TrafficSelector;

  //
  // Transform the Internal IKE structure to IKE payload.
  // Only the SA payload use the interal structure to store the attribute.
  // Other payload use structure which same with the RFC defined, so there is
  // no need to tranform them to IKE payload.
  //
  switch (IkePayload->PayloadType) {
  case IKEV2_PAYLOAD_TYPE_SA:
    //
    // Transform IKE_SA_DATA to IK_SA payload
    //
    SaData    = (IKEV2_SA_DATA *) IkePayload->PayloadBuf;
    SaPayload = Ikev2EncodeSa ((IKEV2_SESSION_COMMON *) SessionCommon, SaData);

    if (SaPayload == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    if (!IkePayload->IsPayloadBufExt) {
      FreePool (IkePayload->PayloadBuf);
    }
    IkePayload->PayloadBuf      = (UINT8 *) SaPayload;
    IkePayload->IsPayloadBufExt = FALSE;
    break;

  case IKEV2_PAYLOAD_TYPE_NOTIFY:
    NotifyPayload               = (IKEV2_NOTIFY *) IkePayload->PayloadBuf;
    NotifyPayload->MessageType  = HTONS (NotifyPayload->MessageType);
    break;

  case IKEV2_PAYLOAD_TYPE_DELETE:
    DeletePayload           = (IKEV2_DELETE *) IkePayload->PayloadBuf;
    DeletePayload->NumSpis  = HTONS (DeletePayload->NumSpis);
    break;

  case IKEV2_PAYLOAD_TYPE_KE:
    KeyPayload              = (IKEV2_KEY_EXCHANGE *) IkePayload->PayloadBuf;
    KeyPayload->DhGroup     = HTONS (KeyPayload->DhGroup);
    break;

  case IKEV2_PAYLOAD_TYPE_TS_INIT:
  case IKEV2_PAYLOAD_TYPE_TS_RSP:
    TsPayload = (IKEV2_TS *) IkePayload->PayloadBuf;
    TsBuffer  = IkePayload->PayloadBuf + sizeof (IKEV2_TS);

    for (Index = 0; Index < TsPayload->TSNumbers; Index++) {
      TrafficSelector = (TRAFFIC_SELECTOR *) TsBuffer;
      TsBuffer        = TsBuffer + TrafficSelector->SelecorLen;
      //
      // Host order to network order
      //
      TrafficSelector->SelecorLen = HTONS (TrafficSelector->SelecorLen);
      TrafficSelector->StartPort  = HTONS (TrafficSelector->StartPort);
      TrafficSelector->EndPort    = HTONS (TrafficSelector->EndPort);

    }

    break;

  case IKEV2_PAYLOAD_TYPE_CP:
    CfgAttribute = (IKEV2_CFG_ATTRIBUTES *)(((IKEV2_CFG *) IkePayload->PayloadBuf) + 1);
    CfgAttribute->AttritType  = HTONS (CfgAttribute->AttritType);
    CfgAttribute->ValueLength = HTONS (CfgAttribute->ValueLength);

  case IKEV2_PAYLOAD_TYPE_ID_INIT:
  case IKEV2_PAYLOAD_TYPE_ID_RSP:
  case IKEV2_PAYLOAD_TYPE_AUTH:
  default:
    break;
  }

  PayloadHdr  = (IKEV2_COMMON_PAYLOAD_HEADER *) IkePayload->PayloadBuf;
  IkePayload->PayloadSize = PayloadHdr->PayloadLength;
  PayloadHdr->PayloadLength = HTONS (PayloadHdr->PayloadLength);
  IKEV2_DUMP_PAYLOAD (IkePayload);
  return EFI_SUCCESS;
}

/**
  The general interface for decoding Payload.

  This function converts the received Payload into internal structure.

  @param[in]      SessionCommon     Pointer to IKE Session Common used for decoding.
  @param[in, out] IkePayload        Pointer to IKE payload to be decoded as input, and
                                    store the decoded result as output.

  @retval EFI_INVALID_PARAMETER  Meet error when decoding the SA payload.
  @retval EFI_SUCCESS            Decoded successfully.

**/
EFI_STATUS
Ikev2DecodePayload (
  IN     UINT8       *SessionCommon,
  IN OUT IKE_PAYLOAD *IkePayload
  )
{
  IKEV2_COMMON_PAYLOAD_HEADER *PayloadHdr;
  UINT16                      PayloadSize;
  UINT8                       PayloadType;
  IKEV2_SA_DATA               *SaData;
  EFI_STATUS                  Status;
  IKEV2_NOTIFY                *NotifyPayload;
  IKEV2_DELETE                *DeletePayload;
  UINT16                      TsTotalSize;
  TRAFFIC_SELECTOR            *TsSelector;
  IKEV2_TS                    *TsPayload;
  IKEV2_KEY_EXCHANGE          *KeyPayload;
  IKEV2_CFG_ATTRIBUTES        *CfgAttribute;
  UINT8                       Index;

  //
  // Transform the IKE payload to Internal IKE structure.
  // Only the SA payload and Hash Payload use the interal
  // structure to store the attribute. Other payloads use
  // structure which is same with the definitions in RFC,
  // so there is no need to tranform them to internal IKE
  // structure.
  //
  Status      = EFI_SUCCESS;
  PayloadSize = (UINT16) IkePayload->PayloadSize;
  PayloadType = IkePayload->PayloadType;
  PayloadHdr  = (IKEV2_COMMON_PAYLOAD_HEADER *) IkePayload->PayloadBuf;
  //
  // The PayloadSize is the size of whole payload.
  // Replace HTONS operation to assignment statements, since the result is same.
  //
  PayloadHdr->PayloadLength = PayloadSize;

  IKEV2_DUMP_PAYLOAD (IkePayload);
  switch (PayloadType) {
  case IKEV2_PAYLOAD_TYPE_SA:
    if (PayloadSize < sizeof (IKEV2_SA)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    SaData = Ikev2DecodeSa ((IKEV2_SESSION_COMMON *) SessionCommon, (IKEV2_SA *) PayloadHdr);
    if (SaData == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    if (!IkePayload->IsPayloadBufExt) {
      FreePool (IkePayload->PayloadBuf);
    }

    IkePayload->PayloadBuf      = (UINT8 *) SaData;
    IkePayload->IsPayloadBufExt = FALSE;
    break;

  case IKEV2_PAYLOAD_TYPE_ID_INIT:
  case IKEV2_PAYLOAD_TYPE_ID_RSP :
    if (PayloadSize < sizeof (IKEV2_ID)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    break;

  case IKEV2_PAYLOAD_TYPE_NOTIFY:
    if (PayloadSize < sizeof (IKEV2_NOTIFY)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    NotifyPayload               = (IKEV2_NOTIFY *) PayloadHdr;
    NotifyPayload->MessageType  = NTOHS (NotifyPayload->MessageType);
    break;

  case IKEV2_PAYLOAD_TYPE_DELETE:
    if (PayloadSize < sizeof (IKEV2_DELETE)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    DeletePayload           = (IKEV2_DELETE *) PayloadHdr;
    DeletePayload->NumSpis  = NTOHS (DeletePayload->NumSpis);
    break;

  case IKEV2_PAYLOAD_TYPE_AUTH:
    if (PayloadSize < sizeof (IKEV2_AUTH)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    break;

  case IKEV2_PAYLOAD_TYPE_KE:
    KeyPayload              = (IKEV2_KEY_EXCHANGE *) IkePayload->PayloadBuf;
    KeyPayload->DhGroup     = HTONS (KeyPayload->DhGroup);
    break;

  case IKEV2_PAYLOAD_TYPE_TS_INIT:
  case IKEV2_PAYLOAD_TYPE_TS_RSP :
    TsTotalSize = 0;
    if (PayloadSize < sizeof (IKEV2_TS)) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    //
    // Parse each traffic selector and transfer network-order to host-order
    //
    TsPayload   = (IKEV2_TS *) IkePayload->PayloadBuf;
    TsSelector  = (TRAFFIC_SELECTOR *) (IkePayload->PayloadBuf + sizeof (IKEV2_TS));

    for (Index = 0; Index < TsPayload->TSNumbers; Index++) {
      TsSelector->SelecorLen  = NTOHS (TsSelector->SelecorLen);
      TsSelector->StartPort   = NTOHS (TsSelector->StartPort);
      TsSelector->EndPort     = NTOHS (TsSelector->EndPort);

      TsTotalSize             = (UINT16) (TsTotalSize + TsSelector->SelecorLen);
      TsSelector              = (TRAFFIC_SELECTOR *) ((UINT8 *) TsSelector + TsSelector->SelecorLen);
    }
    //
    // Check if the total size of Traffic Selectors is correct.
    //
    if (TsTotalSize != PayloadSize - sizeof(IKEV2_TS)) {
      Status = EFI_INVALID_PARAMETER;
    }

  case IKEV2_PAYLOAD_TYPE_CP:
    CfgAttribute = (IKEV2_CFG_ATTRIBUTES *)(((IKEV2_CFG *) IkePayload->PayloadBuf) + 1);
    CfgAttribute->AttritType  = NTOHS (CfgAttribute->AttritType);
    CfgAttribute->ValueLength = NTOHS (CfgAttribute->ValueLength);

  default:
    break;
  }

 Exit:
  return Status;
}

/**
  Decode the IKE packet.

  This function first decrypts the IKE packet if needed , then separates the whole
  IKE packet from the IkePacket->PayloadBuf into IkePacket payload list.

  @param[in]      SessionCommon          Pointer to IKEV1_SESSION_COMMON containing
                                         some parameter used by IKE packet decoding.
  @param[in, out] IkePacket              The IKE Packet to be decoded on input, and
                                         the decoded result on return.
  @param[in]      IkeType                The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                         IKE_CHILD_TYPE are supported.

  @retval         EFI_SUCCESS            The IKE packet is decoded successfully.
  @retval         Otherwise              The IKE packet decoding is failed.

**/
EFI_STATUS
Ikev2DecodePacket (
  IN     IKEV2_SESSION_COMMON  *SessionCommon,
  IN OUT IKE_PACKET            *IkePacket,
  IN     UINTN                 IkeType
  )
{
  EFI_STATUS                  Status;
  IKEV2_COMMON_PAYLOAD_HEADER *PayloadHdr;
  UINT8                       PayloadType;
  UINTN                       RemainBytes;
  UINT16                      PayloadSize;
  IKE_PAYLOAD                 *IkePayload;
  IKE_HEADER                  *IkeHeader;
  IKEV2_SA_SESSION            *IkeSaSession;

  IkeHeader = NULL;

  //
  // Check if the IkePacket need decrypt.
  //
  if (SessionCommon->State >= IkeStateAuth) {
    Status = Ikev2DecryptPacket (SessionCommon, IkePacket, IkeType);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = EFI_SUCCESS;

  //
  // If the IkePacket doesn't contain any payload return invalid parameter.
  //
  if (IkePacket->Header->NextPayload == IKEV2_PAYLOAD_TYPE_NONE) {
    if ((SessionCommon->State >= IkeStateAuth) &&
        (IkePacket->Header->ExchangeType == IKEV2_EXCHANGE_TYPE_INFO)
        ) {
      //
      // If it is Liveness check, there will be no payload load in the encrypt payload.
      //
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_INVALID_PARAMETER;
    }
  }

  //
  // If the PayloadTotalSize < Header length, return invalid parameter.
  //
  RemainBytes = IkePacket->PayloadTotalSize;
  if (RemainBytes < sizeof (IKEV2_COMMON_PAYLOAD_HEADER)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // If the packet is first or second message, store whole message in
  // IkeSa->InitiPacket or IkeSa->RespPacket for following Auth Payload
  // calculate.
  //
  if (IkePacket->Header->ExchangeType == IKEV2_EXCHANGE_TYPE_INIT) {
    IkeHeader = AllocateZeroPool (sizeof (IKE_HEADER));
    ASSERT (IkeHeader != NULL);
    CopyMem (IkeHeader, IkePacket->Header, sizeof (IKE_HEADER));

    //
    // Before store the whole packet, roll back the host order to network order,
    // since the header order was changed in the IkePacketFromNetbuf.
    //
    IkeHdrNetToHost (IkeHeader);
    IkeSaSession = IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);
    if (SessionCommon->IsInitiator) {
      IkeSaSession->RespPacket     = AllocateZeroPool (IkePacket->Header->Length);
      if (IkeSaSession->RespPacket == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
      IkeSaSession->RespPacketSize = IkePacket->Header->Length;
      CopyMem (IkeSaSession->RespPacket, IkeHeader, sizeof (IKE_HEADER));
      CopyMem (
        IkeSaSession->RespPacket + sizeof (IKE_HEADER),
        IkePacket->PayloadsBuf,
        IkePacket->Header->Length - sizeof (IKE_HEADER)
        );
    } else {
      IkeSaSession->InitPacket     = AllocateZeroPool (IkePacket->Header->Length);
      if (IkeSaSession->InitPacket == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
      IkeSaSession->InitPacketSize = IkePacket->Header->Length;
      CopyMem (IkeSaSession->InitPacket, IkeHeader, sizeof (IKE_HEADER));
      CopyMem (
        IkeSaSession->InitPacket + sizeof (IKE_HEADER),
        IkePacket->PayloadsBuf,
        IkePacket->Header->Length - sizeof (IKE_HEADER)
        );
    }
  }

  //
  // Point to the first Payload
  //
  PayloadHdr  = (IKEV2_COMMON_PAYLOAD_HEADER *) IkePacket->PayloadsBuf;
  PayloadType = IkePacket->Header->NextPayload;

  //
  // Parse each payload
  //
  while (RemainBytes >= sizeof (IKEV2_COMMON_PAYLOAD_HEADER)) {
    PayloadSize = NTOHS (PayloadHdr->PayloadLength);

    //
    //Check the size of the payload is correct.
    //
    if (RemainBytes < PayloadSize) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    //
    // At certain states, it should save some datas before decoding.
    //
    if (SessionCommon->BeforeDecodePayload != NULL) {
      SessionCommon->BeforeDecodePayload (
                       (UINT8 *) SessionCommon,
                       (UINT8 *) PayloadHdr,
                       PayloadSize,
                       PayloadType
                       );
    }

    //
    // Initial IkePayload
    //
    IkePayload = IkePayloadAlloc ();
    ASSERT (IkePayload != NULL);

    IkePayload->PayloadType     = PayloadType;
    IkePayload->PayloadBuf      = (UINT8 *) PayloadHdr;
    IkePayload->PayloadSize     = PayloadSize;
    IkePayload->IsPayloadBufExt = TRUE;

    Status = Ikev2DecodePayload ((UINT8 *) SessionCommon, IkePayload);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    IPSEC_DUMP_BUF ("After Decoding Payload", IkePayload->PayloadBuf, IkePayload->PayloadSize);
    //
    // Add each payload into packet
    // Notice, the IkePacket->Hdr->Lenght still recode the whole IkePacket length
    // which is before the decoding.
    //
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, IkePayload);

    RemainBytes -= PayloadSize;
    PayloadType  = PayloadHdr->NextPayload;
    if (PayloadType == IKEV2_PAYLOAD_TYPE_NONE) {
      break;
    }

    PayloadHdr = (IKEV2_COMMON_PAYLOAD_HEADER *) ((UINT8 *) PayloadHdr + PayloadSize);
  }

  if (PayloadType != IKEV2_PAYLOAD_TYPE_NONE) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

Exit:
  if (EFI_ERROR (Status)) {
    ClearAllPayloads (IkePacket);
  }

  if (IkeHeader != NULL) {
    FreePool (IkeHeader);
  }
  return Status;
}

/**
  Encode the IKE packet.

  This function puts all Payloads into one payload then encrypt it if needed.

  @param[in]      SessionCommon      Pointer to IKEV2_SESSION_COMMON containing
                                     some parameter used during IKE packet encoding.
  @param[in, out] IkePacket          Pointer to IKE_PACKET to be encoded as input,
                                     and the encoded result as output.
  @param[in]      IkeType            The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                     IKE_CHILD_TYPE are supportted.

  @retval         EFI_SUCCESS        Encode IKE packet successfully.
  @retval         Otherwise          Encode IKE packet failed.

**/
EFI_STATUS
Ikev2EncodePacket (
  IN     IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET           *IkePacket,
  IN     UINTN                IkeType
  )
{
  IKE_PAYLOAD       *IkePayload;
  UINTN             PayloadTotalSize;
  LIST_ENTRY        *Entry;
  EFI_STATUS        Status;
  IKEV2_SA_SESSION  *IkeSaSession;

  PayloadTotalSize = 0;
  //
  // Encode each payload
  //
  for (Entry = IkePacket->PayloadList.ForwardLink; Entry != &(IkePacket->PayloadList);) {
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
    Entry       = Entry->ForwardLink;
    Status      = Ikev2EncodePayload ((UINT8 *) SessionCommon, IkePayload);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (SessionCommon->AfterEncodePayload != NULL) {
      //
      // For certain states, save some payload for further calculation
      //
      SessionCommon->AfterEncodePayload (
                      (UINT8 *) SessionCommon,
                      IkePayload->PayloadBuf,
                      IkePayload->PayloadSize,
                      IkePayload->PayloadType
                      );
    }

    PayloadTotalSize += IkePayload->PayloadSize;
  }
  IkePacket->PayloadTotalSize = PayloadTotalSize;

  Status = EFI_SUCCESS;
  if (SessionCommon->State >= IkeStateAuth) {
    //
    // Encrypt all payload and transfer IKE packet header from Host order to Network order.
    //
    Status = Ikev2EncryptPacket (SessionCommon, IkePacket);
  } else {
    //
    // Fill in the lenght into IkePacket header and transfer Host order to Network order.
    //
    IkePacket->Header->Length = (UINT32) (sizeof (IKE_HEADER) + IkePacket->PayloadTotalSize);
    IkeHdrHostToNet (IkePacket->Header);
  }

  //
  // If the packet is first message, store whole message in IkeSa->InitiPacket
  // for following Auth Payload calculation.
  //
  if (IkePacket->Header->ExchangeType == IKEV2_EXCHANGE_TYPE_INIT) {
    IkeSaSession =  IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);
    if (SessionCommon->IsInitiator) {
      IkeSaSession->InitPacketSize = IkePacket->PayloadTotalSize + sizeof (IKE_HEADER);
      IkeSaSession->InitPacket     = AllocateZeroPool (IkeSaSession->InitPacketSize);
      ASSERT (IkeSaSession->InitPacket != NULL);
      CopyMem (IkeSaSession->InitPacket, IkePacket->Header, sizeof (IKE_HEADER));
      PayloadTotalSize = 0;
      for (Entry = IkePacket->PayloadList.ForwardLink; Entry != &(IkePacket->PayloadList);) {
        IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
        Entry       = Entry->ForwardLink;
        CopyMem (
          IkeSaSession->InitPacket + sizeof (IKE_HEADER) + PayloadTotalSize,
          IkePayload->PayloadBuf,
          IkePayload->PayloadSize
          );
        PayloadTotalSize = PayloadTotalSize + IkePayload->PayloadSize;
      }
    } else {
      IkeSaSession->RespPacketSize = IkePacket->PayloadTotalSize + sizeof(IKE_HEADER);
      IkeSaSession->RespPacket     = AllocateZeroPool (IkeSaSession->RespPacketSize);
      ASSERT (IkeSaSession->RespPacket != NULL);
      CopyMem (IkeSaSession->RespPacket, IkePacket->Header, sizeof (IKE_HEADER));
      PayloadTotalSize = 0;
      for (Entry = IkePacket->PayloadList.ForwardLink; Entry != &(IkePacket->PayloadList);) {
        IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
        Entry       = Entry->ForwardLink;

        CopyMem (
          IkeSaSession->RespPacket + sizeof (IKE_HEADER) + PayloadTotalSize,
          IkePayload->PayloadBuf,
          IkePayload->PayloadSize
          );
        PayloadTotalSize = PayloadTotalSize + IkePayload->PayloadSize;
      }
    }
  }

  return Status;
}

/**
  Decrypt IKE packet.

  This function decrypts the Encrypted IKE packet and put the result into IkePacket->PayloadBuf.

  @param[in]      SessionCommon       Pointer to IKEV2_SESSION_COMMON containing
                                      some parameter used during decrypting.
  @param[in, out] IkePacket           Pointer to IKE_PACKET to be decrypted as input,
                                      and the decrypted result as output.
  @param[in, out] IkeType             The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                      IKE_CHILD_TYPE are supportted.

  @retval EFI_INVALID_PARAMETER      If the IKE packet length is zero or the
                                     IKE packet length is not aligned with Algorithm Block Size
  @retval EFI_SUCCESS                Decrypt IKE packet successfully.

**/
EFI_STATUS
Ikev2DecryptPacket (
  IN     IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET           *IkePacket,
  IN OUT UINTN                IkeType
  )
{
  UINT8                  CryptBlockSize;      // Encrypt Block Size
  UINTN                  DecryptedSize;       // Encrypted IKE Payload Size
  UINT8                  *DecryptedBuf;       // Encrypted IKE Payload buffer
  UINTN                  IntegritySize;
  UINT8                  *IntegrityBuffer;
  UINTN                  IvSize;              // Iv Size
  UINT8                  CheckSumSize;        // Integrity Check Sum Size depends on intergrity Auth
  UINT8                  *CheckSumData;       // Check Sum data
  IKEV2_SA_SESSION       *IkeSaSession;
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  EFI_STATUS             Status;
  UINT8                  PadLen;
  HASH_DATA_FRAGMENT     Fragments[1];

  IvSize         = 0;
  IkeSaSession   = NULL;
  CryptBlockSize = 0;
  CheckSumSize   = 0;

  //
  // Check if the first payload is the Encrypted payload
  //
  if (IkePacket->Header->NextPayload != IKEV2_PAYLOAD_TYPE_ENCRYPT) {
    return EFI_ACCESS_DENIED;
  }
  CheckSumData    = NULL;
  DecryptedBuf    = NULL;
  IntegrityBuffer = NULL;

  //
  // Get the Block Size
  //
  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {

    CryptBlockSize = (UINT8) IpSecGetEncryptBlockSize ((UINT8) SessionCommon->SaParams->EncAlgId);

    CheckSumSize   = (UINT8) IpSecGetIcvLength ((UINT8) SessionCommon->SaParams->IntegAlgId);
    IkeSaSession   = IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);

  } else if (SessionCommon->IkeSessionType == IkeSessionTypeChildSa) {

    ChildSaSession = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
    IkeSaSession   = ChildSaSession->IkeSaSession;
    CryptBlockSize = (UINT8) IpSecGetEncryptBlockSize ((UINT8) IkeSaSession->SessionCommon.SaParams->EncAlgId);
    CheckSumSize   = (UINT8) IpSecGetIcvLength ((UINT8) IkeSaSession->SessionCommon.SaParams->IntegAlgId);
  } else {
    //
    // The type of SA Session would either be IkeSa or ChildSa.
    //
    return EFI_INVALID_PARAMETER;
  }

  CheckSumData = AllocateZeroPool (CheckSumSize);
  ASSERT (CheckSumData != NULL);

  //
  // Fill in the Integrity buffer
  //
  IntegritySize   = IkePacket->PayloadTotalSize + sizeof (IKE_HEADER);
  IntegrityBuffer = AllocateZeroPool (IntegritySize);
  ASSERT (IntegrityBuffer != NULL);
  CopyMem (IntegrityBuffer, IkePacket->Header, sizeof(IKE_HEADER));
  CopyMem (IntegrityBuffer + sizeof (IKE_HEADER), IkePacket->PayloadsBuf, IkePacket->PayloadTotalSize);

  //
  // Change Host order to Network order, since the header order was changed
  // in the IkePacketFromNetbuf.
  //
  IkeHdrHostToNet ((IKE_HEADER *)IntegrityBuffer);

  //
  // Calculate the Integrity CheckSum Data
  //
  Fragments[0].Data     = IntegrityBuffer;
  Fragments[0].DataSize = IntegritySize - CheckSumSize;

  if (SessionCommon->IsInitiator) {
    Status = IpSecCryptoIoHmac (
               (UINT8)IkeSaSession->SessionCommon.SaParams->IntegAlgId,
               IkeSaSession->IkeKeys->SkArKey,
               IkeSaSession->IkeKeys->SkArKeySize,
               (HASH_DATA_FRAGMENT *) Fragments,
               1,
               CheckSumData,
               CheckSumSize
               );
  } else {
    Status = IpSecCryptoIoHmac (
               (UINT8)IkeSaSession->SessionCommon.SaParams->IntegAlgId,
               IkeSaSession->IkeKeys->SkAiKey,
               IkeSaSession->IkeKeys->SkAiKeySize,
               (HASH_DATA_FRAGMENT *) Fragments,
               1,
               CheckSumData,
               CheckSumSize
               );
  }

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Compare the Integrity CheckSum Data with the one in IkePacket
  //
  if (CompareMem (
        IkePacket->PayloadsBuf + IkePacket->PayloadTotalSize - CheckSumSize,
        CheckSumData,
        CheckSumSize
        ) != 0) {
    DEBUG ((DEBUG_ERROR, "Error auth verify payload\n"));
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  IvSize = CryptBlockSize;

  //
  // Decrypt the payload with the key.
  //
  DecryptedSize = IkePacket->PayloadTotalSize - sizeof (IKEV2_COMMON_PAYLOAD_HEADER) - IvSize - CheckSumSize;
  DecryptedBuf  = AllocateZeroPool (DecryptedSize);
  ASSERT (DecryptedBuf != NULL);

  CopyMem (
    DecryptedBuf,
    IkePacket->PayloadsBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER) + IvSize,
    DecryptedSize
    );

  if (SessionCommon->IsInitiator) {
   Status = IpSecCryptoIoDecrypt (
              (UINT8) SessionCommon->SaParams->EncAlgId,
              IkeSaSession->IkeKeys->SkErKey,
              IkeSaSession->IkeKeys->SkErKeySize << 3,
              IkePacket->PayloadsBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER),
              DecryptedBuf,
              DecryptedSize,
              DecryptedBuf
              );
  } else {
    Status = IpSecCryptoIoDecrypt (
               (UINT8) SessionCommon->SaParams->EncAlgId,
               IkeSaSession->IkeKeys->SkEiKey,
               IkeSaSession->IkeKeys->SkEiKeySize << 3,
               IkePacket->PayloadsBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER),
               DecryptedBuf,
               DecryptedSize,
               DecryptedBuf
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error decrypt buffer with %r\n", Status));
    goto ON_EXIT;
  }

  //
  // Get the Padding length
  //
  //
  PadLen = (UINT8) (*(DecryptedBuf + DecryptedSize - sizeof (IKEV2_PAD_LEN)));

  //
  // Save the next payload of encrypted payload into IkePacket->Hdr->NextPayload
  //
  IkePacket->Header->NextPayload = ((IKEV2_ENCRYPTED *) IkePacket->PayloadsBuf)->Header.NextPayload;

  //
  // Free old IkePacket->PayloadBuf and point it to decrypted paylaod buffer.
  //
  FreePool (IkePacket->PayloadsBuf);
  IkePacket->PayloadsBuf      = DecryptedBuf;
  IkePacket->PayloadTotalSize = DecryptedSize - PadLen;

  IPSEC_DUMP_BUF ("Decrypted Buffer", DecryptedBuf, DecryptedSize);


ON_EXIT:
  if (CheckSumData != NULL) {
    FreePool (CheckSumData);
  }

  if (EFI_ERROR (Status) && DecryptedBuf != NULL) {
    FreePool (DecryptedBuf);
  }

  if (IntegrityBuffer != NULL) {
    FreePool (IntegrityBuffer);
  }

  return Status;
}

/**
  Encrypt IKE packet.

  This function encrypt IKE packet before sending it. The Encrypted IKE packet
  is put in to IKEV2 Encrypted Payload.

  @param[in]        SessionCommon     Pointer to IKEV2_SESSION_COMMON related to the IKE packet.
  @param[in, out]   IkePacket         Pointer to IKE packet to be encrypted.

  @retval      EFI_SUCCESS       Operation is successful.
  @retval      Others            Operation is failed.

**/
EFI_STATUS
Ikev2EncryptPacket (
  IN IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET       *IkePacket
  )
{
  UINT8                  CryptBlockSize;      // Encrypt Block Size
  UINT8                  CryptBlockSizeMask;  // Block Mask
  UINTN                  EncryptedSize;       // Encrypted IKE Payload Size
  UINT8                  *EncryptedBuf;       // Encrypted IKE Payload buffer
  UINT8                  *EncryptPayloadBuf;  // Contain whole Encrypted Payload
  UINTN                  EncryptPayloadSize;  // Total size of the Encrypted payload
  UINT8                  *IntegrityBuf;       // Buffer to be intergity
  UINT8                  *IvBuffer;           // Initialization Vector
  UINT8                  IvSize;              // Iv Size
  UINT8                  CheckSumSize;        // Integrity Check Sum Size depends on intergrity Auth
  UINT8                  *CheckSumData;       // Check Sum data
  UINTN                  Index;
  IKE_PAYLOAD            *EncryptPayload;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  EFI_STATUS             Status;
  LIST_ENTRY             *Entry;
  IKE_PAYLOAD            *IkePayload;
  HASH_DATA_FRAGMENT     Fragments[1];

  Status = EFI_SUCCESS;

  //
  // Initial all buffers to NULL.
  //
  EncryptedBuf      = NULL;
  EncryptPayloadBuf = NULL;
  IvBuffer          = NULL;
  CheckSumData      = NULL;
  IkeSaSession      = NULL;
  CryptBlockSize    = 0;
  CheckSumSize      = 0;
  IntegrityBuf      = NULL;
  //
  // Get the Block Size
  //
  if (SessionCommon->IkeSessionType == IkeSessionTypeIkeSa) {

    CryptBlockSize = (UINT8) IpSecGetEncryptBlockSize ((UINT8) SessionCommon->SaParams->EncAlgId);
    CheckSumSize   = (UINT8) IpSecGetIcvLength ((UINT8) SessionCommon->SaParams->IntegAlgId);
    IkeSaSession   = IKEV2_SA_SESSION_FROM_COMMON (SessionCommon);

  } else if (SessionCommon->IkeSessionType == IkeSessionTypeChildSa) {

    ChildSaSession = IKEV2_CHILD_SA_SESSION_FROM_COMMON (SessionCommon);
    IkeSaSession   = ChildSaSession->IkeSaSession;
    CryptBlockSize = (UINT8) IpSecGetEncryptBlockSize ((UINT8) IkeSaSession->SessionCommon.SaParams->EncAlgId);
    CheckSumSize   = (UINT8) IpSecGetIcvLength ((UINT8) IkeSaSession->SessionCommon.SaParams->IntegAlgId);
  }

  //
  // Calcualte the EncryptPayloadSize and the PAD length
  //
  CryptBlockSizeMask  = (UINT8) (CryptBlockSize - 1);
  EncryptedSize       = (IkePacket->PayloadTotalSize + sizeof (IKEV2_PAD_LEN) + CryptBlockSizeMask) & ~CryptBlockSizeMask;
  EncryptedBuf        = (UINT8 *) AllocateZeroPool (EncryptedSize);
  ASSERT (EncryptedBuf != NULL);

  //
  // Copy all payload into EncryptedIkePayload
  //
  Index = 0;
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload = IKE_PAYLOAD_BY_PACKET (Entry);

    CopyMem (EncryptedBuf + Index, IkePayload->PayloadBuf, IkePayload->PayloadSize);
    Index += IkePayload->PayloadSize;

  };

  //
  // Fill in the Pading Length
  //
  *(EncryptedBuf + EncryptedSize - 1) = (UINT8)(EncryptedSize - IkePacket->PayloadTotalSize - 1);

  //
  // The IV size is equal with block size
  //
  IvSize    = CryptBlockSize;
  IvBuffer  = (UINT8 *) AllocateZeroPool (IvSize);
  if (IvBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Generate IV
  //
  IkeGenerateIv (IvBuffer, IvSize);

  //
  // Encrypt payload buf
  //
  if (SessionCommon->IsInitiator) {
    Status = IpSecCryptoIoEncrypt (
               (UINT8) IkeSaSession->SessionCommon.SaParams->EncAlgId,
               IkeSaSession->IkeKeys->SkEiKey,
               IkeSaSession->IkeKeys->SkEiKeySize << 3,
               IvBuffer,
               EncryptedBuf,
               EncryptedSize,
               EncryptedBuf
               );
  } else {
    Status = IpSecCryptoIoEncrypt (
               (UINT8) IkeSaSession->SessionCommon.SaParams->EncAlgId,
               IkeSaSession->IkeKeys->SkErKey,
               IkeSaSession->IkeKeys->SkErKeySize << 3,
               IvBuffer,
               EncryptedBuf,
               EncryptedSize,
               EncryptedBuf
               );
  }
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Allocate the buffer for the whole IKE payload (Encrypted Payload).
  //
  EncryptPayloadSize = sizeof(IKEV2_ENCRYPTED) + IvSize + EncryptedSize + CheckSumSize;
  EncryptPayloadBuf  = AllocateZeroPool (EncryptPayloadSize);
  ASSERT (EncryptPayloadBuf != NULL);

  //
  // Fill in Header of  Encrypted Payload
  //
  ((IKEV2_ENCRYPTED *) EncryptPayloadBuf)->Header.NextPayload   = IkePacket->Header->NextPayload;
  ((IKEV2_ENCRYPTED *) EncryptPayloadBuf)->Header.PayloadLength = HTONS ((UINT16)EncryptPayloadSize);

  //
  // Fill in Iv
  //
  CopyMem (EncryptPayloadBuf + sizeof (IKEV2_ENCRYPTED), IvBuffer, IvSize);

  //
  // Fill in encrypted data
  //
  CopyMem (EncryptPayloadBuf + sizeof (IKEV2_ENCRYPTED) + IvSize, EncryptedBuf, EncryptedSize);

  //
  // Fill in the IKE Packet header
  //
  IkePacket->PayloadTotalSize    = EncryptPayloadSize;
  IkePacket->Header->Length      = (UINT32) (sizeof (IKE_HEADER) + IkePacket->PayloadTotalSize);
  IkePacket->Header->NextPayload = IKEV2_PAYLOAD_TYPE_ENCRYPT;

  IntegrityBuf                   = AllocateZeroPool (IkePacket->Header->Length);
  if (IntegrityBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  IkeHdrHostToNet (IkePacket->Header);

  CopyMem (IntegrityBuf, IkePacket->Header, sizeof (IKE_HEADER));
  CopyMem (IntegrityBuf + sizeof (IKE_HEADER), EncryptPayloadBuf, EncryptPayloadSize);

  //
  // Calcualte Integrity CheckSum
  //
  Fragments[0].Data     = IntegrityBuf;
  Fragments[0].DataSize = EncryptPayloadSize + sizeof (IKE_HEADER) - CheckSumSize;

  CheckSumData = AllocateZeroPool (CheckSumSize);
  if (CheckSumData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  if (SessionCommon->IsInitiator) {

    IpSecCryptoIoHmac (
      (UINT8)IkeSaSession->SessionCommon.SaParams->IntegAlgId,
      IkeSaSession->IkeKeys->SkAiKey,
      IkeSaSession->IkeKeys->SkAiKeySize,
      (HASH_DATA_FRAGMENT *) Fragments,
      1,
      CheckSumData,
      CheckSumSize
      );
  } else {

    IpSecCryptoIoHmac (
      (UINT8)IkeSaSession->SessionCommon.SaParams->IntegAlgId,
      IkeSaSession->IkeKeys->SkArKey,
      IkeSaSession->IkeKeys->SkArKeySize,
      (HASH_DATA_FRAGMENT *) Fragments,
      1,
      CheckSumData,
      CheckSumSize
      );
  }

  //
  // Copy CheckSum into Encrypted Payload
  //
  CopyMem (EncryptPayloadBuf + EncryptPayloadSize - CheckSumSize, CheckSumData, CheckSumSize);

  IPSEC_DUMP_BUF ("Encrypted payload buffer", EncryptPayloadBuf, EncryptPayloadSize);
  IPSEC_DUMP_BUF ("Integrith CheckSum Data", CheckSumData, CheckSumSize);

  //
  // Clean all payload under IkePacket->PayloadList.
  //
  ClearAllPayloads (IkePacket);

  //
  // Create Encrypted Payload and add into IkePacket->PayloadList
  //
  EncryptPayload = IkePayloadAlloc ();
  ASSERT (EncryptPayload != NULL);

  //
  // Fill the encrypted payload into the IKE_PAYLOAD structure.
  //
  EncryptPayload->PayloadBuf  = EncryptPayloadBuf;
  EncryptPayload->PayloadSize = EncryptPayloadSize;
  EncryptPayload->PayloadType = IKEV2_PAYLOAD_TYPE_ENCRYPT;

  IKE_PACKET_APPEND_PAYLOAD (IkePacket, EncryptPayload);

ON_EXIT:
  if (EncryptedBuf != NULL) {
    FreePool (EncryptedBuf);
  }

  if (EFI_ERROR (Status) && EncryptPayloadBuf != NULL) {
    FreePool (EncryptPayloadBuf);
  }

  if (IvBuffer != NULL) {
    FreePool (IvBuffer);
  }

  if (CheckSumData != NULL) {
    FreePool (CheckSumData);
  }

  if (IntegrityBuf != NULL) {
    FreePool (IntegrityBuf);
  }

  return Status;
}

/**
  Save some useful payloads after accepting the Packet.

  @param[in] SessionCommon   Pointer to IKEV2_SESSION_COMMON related to the operation.
  @param[in] IkePacket       Pointer to received IkePacet.
  @param[in] IkeType         The type used to indicate it is in IkeSa or ChildSa or Info
                             exchange.

**/
VOID
Ikev2OnPacketAccepted (
  IN IKEV2_SESSION_COMMON *SessionCommon,
  IN IKE_PACKET           *IkePacket,
  IN UINT8                IkeType
  )
{
  return;
}

/**

  The notification function. It will be called when the related UDP_TX_TOKEN's event
  is signaled.

  This function frees the Net Buffer pointed to the input Packet.

  @param[in]  Packet           Pointer to Net buffer containing the sending IKE packet.
  @param[in]  EndPoint         Pointer to UDP_END_POINT containing the remote and local
                               address information.
  @param[in]  IoStatus         The Status of the related UDP_TX_TOKEN.
  @param[in]  Context          Pointer to data passed from the caller.

**/
VOID
EFIAPI
Ikev2OnPacketSent (
  IN NET_BUF                   *Packet,
  IN UDP_END_POINT             *EndPoint,
  IN EFI_STATUS                IoStatus,
  IN VOID                      *Context
  )
{
 IKE_PACKET             *IkePacket;
 IKEV2_SA_SESSION       *IkeSaSession;
 IKEV2_CHILD_SA_SESSION *ChildSaSession;
 UINT8                  Value;
 IPSEC_PRIVATE_DATA     *Private;
 EFI_STATUS             Status;

 IkePacket  = (IKE_PACKET *) Context;
 Private    = NULL;

 if (EFI_ERROR (IoStatus)) {
    DEBUG ((DEBUG_ERROR, "Error send the last packet in IkeSessionTypeIkeSa with %r\n", IoStatus));
  }

  NetbufFree (Packet);

  if (IkePacket->IsDeleteInfo) {
    //
    // For each RemotePeerIP, there are only one IKESA.
    //
    IkeSaSession = Ikev2SaSessionLookup (
                     &IkePacket->Private->Ikev2EstablishedList,
                     &IkePacket->RemotePeerIp
                     );
    if (IkeSaSession == NULL) {
      IkePacketFree (IkePacket);
      return;
    }

    Private = IkePacket->Private;
    if (IkePacket->Spi != 0 ) {
      //
      // At that time, the established Child SA still in eht ChildSaEstablishSessionList.
      // And meanwhile, if the Child SA is in the the ChildSa in Delete list,
      // remove it from delete list and delete it direclty.
      //
      ChildSaSession = Ikev2ChildSaSessionLookupBySpi (
                         &IkeSaSession->ChildSaEstablishSessionList,
                         IkePacket->Spi
                         );
      if (ChildSaSession != NULL) {
        Ikev2ChildSaSessionRemove (
          &IkeSaSession->DeleteSaList,
          ChildSaSession->LocalPeerSpi,
          IKEV2_DELET_CHILDSA_LIST
          );

        //
        // Delete the Child SA.
        //
        Ikev2ChildSaSilentDelete (
          IkeSaSession,
          IkePacket->Spi
          );
      }

    } else {
      //
      // Delete the IKE SA
      //
      DEBUG (
        (DEBUG_INFO,
        "\n------ deleted Packet (cookie_i, cookie_r):(0x%lx, 0x%lx)------\n",
        IkeSaSession->InitiatorCookie,
        IkeSaSession->ResponderCookie)
        );

      RemoveEntryList (&IkeSaSession->BySessionTable);
      Ikev2SaSessionFree (IkeSaSession);
    }
  }
  IkePacketFree (IkePacket);

  //
  // when all IKE SAs were disabled by calling "IPsecConfig -disable", the IPsec status
  // should be changed.
  //
  if (Private != NULL && Private->IsIPsecDisabling) {
    //
    // After all IKE SAs were deleted, set the IPSEC_STATUS_DISABLED value in
    // IPsec status variable.
    //
    if (IsListEmpty (&Private->Ikev1EstablishedList) && IsListEmpty (&Private->Ikev2EstablishedList)) {
      Value = IPSEC_STATUS_DISABLED;
      Status = gRT->SetVariable (
                 IPSECCONFIG_STATUS_NAME,
                 &gEfiIpSecConfigProtocolGuid,
                 EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                 sizeof (Value),
                 &Value
                 );
      if (!EFI_ERROR (Status)) {
        //
        // Set the DisabledFlag in Private data.
        //
        Private->IpSec.DisabledFlag = TRUE;
        Private->IsIPsecDisabling   = FALSE;
      }
    }
  }
}

/**
  Send out IKEV2 packet.

  @param[in]  IkeUdpService     Pointer to IKE_UDP_SERVICE used to send the IKE packet.
  @param[in]  SessionCommon     Pointer to IKEV1_SESSION_COMMON related to the IKE packet.
  @param[in]  IkePacket         Pointer to IKE_PACKET to be sent out.
  @param[in]  IkeType           The type of IKE to point what's kind of the IKE
                                packet is to be sent out. IKE_SA_TYPE, IKE_INFO_TYPE
                                and IKE_CHILD_TYPE are supportted.

  @retval     EFI_SUCCESS       The operation complete successfully.
  @retval     Otherwise         The operation is failed.

**/
EFI_STATUS
Ikev2SendIkePacket (
  IN IKE_UDP_SERVICE     *IkeUdpService,
  IN UINT8               *SessionCommon,
  IN IKE_PACKET          *IkePacket,
  IN UINTN               IkeType
  )
{
  EFI_STATUS            Status;
  NET_BUF               *IkePacketNetbuf;
  UDP_END_POINT         EndPoint;
  IKEV2_SESSION_COMMON  *Common;

  Common = (IKEV2_SESSION_COMMON *) SessionCommon;

  //
  // Set the resend interval
  //
  if (Common->TimeoutInterval == 0) {
    Common->TimeoutInterval = IKE_DEFAULT_TIMEOUT_INTERVAL;
  }

  //
  // Retransfer the packet if it is initial packet.
  //
  if (IkePacket->Header->Flags == IKE_HEADER_FLAGS_INIT) {
    //
    // Set timer for next retry, this will cancel previous timer
    //
    Status = gBS->SetTimer (
                    Common->TimeoutEvent,
                    TimerRelative,
                    MultU64x32 (Common->TimeoutInterval, 10000) // ms->100ns
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  IKE_PACKET_REF (IkePacket);
  //
  // If the last sent packet is same with this round packet, the packet is resent packet.
  //
  if (IkePacket != Common->LastSentPacket && Common->LastSentPacket != NULL) {
    IkePacketFree (Common->LastSentPacket);
  }

  Common->LastSentPacket = IkePacket;

  //
  // Transform IkePacke to NetBuf
  //
  IkePacketNetbuf = IkeNetbufFromPacket ((UINT8 *) SessionCommon, IkePacket, IkeType);
  ASSERT (IkePacketNetbuf != NULL);

  ZeroMem (&EndPoint, sizeof (UDP_END_POINT));
  EndPoint.RemotePort = IKE_DEFAULT_PORT;
  CopyMem (&IkePacket->RemotePeerIp, &Common->RemotePeerIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&EndPoint.RemoteAddr, &Common->RemotePeerIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&EndPoint.LocalAddr, &Common->LocalPeerIp, sizeof (EFI_IP_ADDRESS));

  IPSEC_DUMP_PACKET (IkePacket, EfiIPsecOutBound, IkeUdpService->IpVersion);

  if (IkeUdpService->IpVersion == IP_VERSION_4) {
    EndPoint.RemoteAddr.Addr[0] = HTONL (EndPoint.RemoteAddr.Addr[0]);
    EndPoint.LocalAddr.Addr[0]  = HTONL (EndPoint.LocalAddr.Addr[0]);
  }

  //
  // Call UDPIO to send out the IKE packet.
  //
  Status = UdpIoSendDatagram (
             IkeUdpService->Output,
             IkePacketNetbuf,
             &EndPoint,
             NULL,
             Ikev2OnPacketSent,
             (VOID*)IkePacket
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error send packet with %r\n", Status));
  }

  return Status;
}

