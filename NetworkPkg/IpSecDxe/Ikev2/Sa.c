/** @file
  The operations for IKEv2 SA.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Utility.h"
#include "IpSecDebug.h"
#include "IkeService.h"
#include "Ikev2.h"

/**
  Generates the DH Key.

  This generates the DH local public key and store it in the IKEv2 SA Session's GxBuffer.
  
  @param[in]  IkeSaSession   Pointer to related IKE SA Session.

  @retval EFI_SUCCESS        The operation succeeded.
  @retval Others             The operation failed.

**/
EFI_STATUS
Ikev2GenerateSaDhPublicKey (
  IN IKEV2_SA_SESSION         *IkeSaSession
  );

/**
  Generates the IKEv2 SA key for the furthure IKEv2 exchange.

  @param[in]  IkeSaSession       Pointer to IKEv2 SA Session.
  @param[in]  KePayload          Pointer to Key payload used to generate the Key.

  @retval EFI_UNSUPPORTED    If the Algorithm Id is not supported.
  @retval EFI_SUCCESS        The operation succeeded.

**/
EFI_STATUS
Ikev2GenerateSaKeys (
  IN IKEV2_SA_SESSION       *IkeSaSession,
  IN IKE_PAYLOAD            *KePayload
  );

/**
  Generates the Keys for the furthure IPsec Protocol.

  @param[in]  ChildSaSession     Pointer to IKE Child SA Session.
  @param[in]  KePayload          Pointer to Key payload used to generate the Key.

  @retval EFI_UNSUPPORTED    If one or more Algorithm Id is unsupported.
  @retval EFI_SUCCESS        The operation succeeded.

**/
EFI_STATUS
Ikev2GenerateChildSaKeys (
  IN IKEV2_CHILD_SA_SESSION     *ChildSaSession,
  IN IKE_PAYLOAD                *KePayload
  );

/**
  Gernerates IKEv2 packet for IKE_SA_INIT exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION related to the exchange.
  @param[in] Context    Context Data passed by caller.

  @retval EFI_SUCCESS   The IKEv2 packet generation succeeded.
  @retval Others        The IKEv2 packet generation failed.

**/
IKE_PACKET *
Ikev2InitPskGenerator (
  IN UINT8           *SaSession,
  IN VOID            *Context
  )
{
  IKE_PACKET         *IkePacket;
  IKEV2_SA_SESSION   *IkeSaSession;
  IKE_PAYLOAD        *SaPayload;
  IKE_PAYLOAD        *KePayload;
  IKE_PAYLOAD        *NoncePayload;
  IKE_PAYLOAD        *NotifyPayload;
  EFI_STATUS         Status;

  SaPayload      = NULL;
  KePayload      = NULL;
  NoncePayload   = NULL;
  NotifyPayload  = NULL;

  IkeSaSession = (IKEV2_SA_SESSION *) SaSession;

  //
  // 1. Allocate IKE packet
  //
  IkePacket = IkePacketAlloc ();
  ASSERT (IkePacket != NULL);

  //
  // 1.a Fill the IkePacket->Hdr
  //
  IkePacket->Header->ExchangeType    = IKEV2_EXCHANGE_TYPE_INIT;
  IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
  IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;
  IkePacket->Header->Version         = (UINT8) (2 << 4);
  IkePacket->Header->MessageId       = 0;

  if (IkeSaSession->SessionCommon.IsInitiator) {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_INIT;
  } else {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
  }

  //
  // If the NCookie is not NULL, this IKE_SA_INIT packet is resent by the NCookie
  // and the NCookie payload should be the first payload in this packet.
  //
  if (IkeSaSession->NCookie != NULL) {
    IkePacket->Header->NextPayload = IKEV2_PAYLOAD_TYPE_NOTIFY;
    NotifyPayload = Ikev2GenerateNotifyPayload (
                      IPSEC_PROTO_ISAKMP,
                      IKEV2_PAYLOAD_TYPE_SA,
                      0,
                      IKEV2_NOTIFICATION_COOKIE,
                      NULL,
                      IkeSaSession->NCookie,
                      IkeSaSession->NCookieSize
                      );
  } else {
    IkePacket->Header->NextPayload = IKEV2_PAYLOAD_TYPE_SA;
  }

  //
  // 2. Generate SA Payload according to the SaData & SaParams
  //
  SaPayload = Ikev2GenerateSaPayload (
                IkeSaSession->SaData,
                IKEV2_PAYLOAD_TYPE_KE,
                IkeSessionTypeIkeSa
                );

  //
  // 3. Generate DH public key.
  //    The DhPrivate Key has been generated in Ikev2InitPskParser, if the
  //    IkeSaSession is responder. If resending IKE_SA_INIT with Cookie Notify
  //    No need to recompute the Public key.
  //
  if ((IkeSaSession->SessionCommon.IsInitiator) && (IkeSaSession->NCookie == NULL)) {    
    Status = Ikev2GenerateSaDhPublicKey (IkeSaSession);
    if (EFI_ERROR (Status)) {
      goto CheckError;
    }
  }

  //
  // 4. Generate KE Payload according to SaParams->DhGroup
  //
  KePayload = Ikev2GenerateKePayload (
                IkeSaSession, 
                IKEV2_PAYLOAD_TYPE_NONCE
                );

  //
  // 5. Generate Nonce Payload
  //    If resending IKE_SA_INIT with Cookie Notify paylaod, no need to regenerate
  //    the Nonce Payload.
  //
  if ((IkeSaSession->SessionCommon.IsInitiator) && (IkeSaSession->NCookie == NULL)) {
    IkeSaSession->NiBlkSize = IKE_NONCE_SIZE;
    IkeSaSession->NiBlock   = IkeGenerateNonce (IKE_NONCE_SIZE);
    ASSERT (IkeSaSession->NiBlock != NULL);
  }

  if (IkeSaSession->SessionCommon.IsInitiator) {
    NoncePayload = Ikev2GenerateNoncePayload (
                     IkeSaSession->NiBlock,
                     IkeSaSession->NiBlkSize,
                     IKEV2_PAYLOAD_TYPE_NONE
                     );
  } else {
    //
    // The Nonce Payload has been created in Ikev2PskParser if the IkeSaSession is
    // responder.
    //
    NoncePayload = Ikev2GenerateNoncePayload (
                     IkeSaSession->NrBlock,
                     IkeSaSession->NrBlkSize,
                     IKEV2_PAYLOAD_TYPE_NONE
                     );
  }

  if (NotifyPayload != NULL) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, NotifyPayload);
  }
  if (SaPayload != NULL) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, SaPayload);
  }
  if (KePayload != NULL) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, KePayload);
  }
  if (NoncePayload != NULL) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, NoncePayload);
  }

  return IkePacket;

CheckError:
  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }
  if (SaPayload != NULL) {
    IkePayloadFree (SaPayload);
  }
  return NULL;    
}

/**
  Parses the IKEv2 packet for IKE_SA_INIT exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION related to the exchange.
  @param[in] IkePacket  The received IKE packet to be parsed.

  @retval EFI_SUCCESS            The IKEv2 packet is acceptable and the relative data is
                                 saved for furthure communication.
  @retval EFI_INVALID_PARAMETER  The IKEv2 packet is malformed or the SA proposal is unacceptable.

**/
EFI_STATUS
Ikev2InitPskParser (
  IN UINT8            *SaSession,
  IN IKE_PACKET       *IkePacket
  ) 
{
  IKEV2_SA_SESSION     *IkeSaSession;
  IKE_PAYLOAD          *SaPayload;
  IKE_PAYLOAD          *KeyPayload;
  IKE_PAYLOAD          *IkePayload;
  IKE_PAYLOAD          *NoncePayload;
  IKE_PAYLOAD          *NotifyPayload;
  UINT8                *NonceBuffer;
  UINTN                NonceSize;
  LIST_ENTRY           *Entry;
  EFI_STATUS           Status;

  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;
  KeyPayload     = NULL;
  SaPayload      = NULL;
  NoncePayload   = NULL;
  IkePayload     = NULL;
  NotifyPayload  = NULL;

  //
  // Iterate payloads to find the SaPayload and KeyPayload.
  //
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_SA) {
      SaPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_KE) {
      KeyPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_NONCE) {
      NoncePayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_NOTIFY) {
      NotifyPayload = IkePayload;
    }
  }

  //
  // According to RFC 4306 - 2.6. If the responder responds with the COOKIE Notify
  // payload with the cookie data, initiator MUST retry the IKE_SA_INIT with a
  // Notify payload of type COOKIE containing the responder suppplied cookie data
  // as first payload and all other payloads unchanged.
  //
  if (IkeSaSession->SessionCommon.IsInitiator) {
    if (NotifyPayload != NULL) {
      Status = Ikev2ParserNotifyCookiePayload (NotifyPayload, IkeSaSession);
      return Status;
    }
  }

  if ((KeyPayload == NULL) || (SaPayload == NULL) || (NoncePayload == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Store NoncePayload for SKEYID computing.
  //
  NonceSize   = NoncePayload->PayloadSize - sizeof (IKEV2_COMMON_PAYLOAD_HEADER);
  NonceBuffer = (UINT8 *) AllocatePool (NonceSize);
  ASSERT (NonceBuffer != NULL);
  CopyMem (
    NonceBuffer,
    NoncePayload->PayloadBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER),
    NonceSize
    );

  //
  // Check if IkePacket Header matches the state
  //
  if (IkeSaSession->SessionCommon.IsInitiator) {
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_RESPOND
    //
    if (IkePacket->Header->Flags != IKE_HEADER_FLAGS_RESPOND) {
      Status = EFI_INVALID_PARAMETER;
      goto CheckError;
    }

    //
    // 2. Parse the SA Payload and Key Payload to find out the cryptographic
    //    suite and fill in the Sa paramse into CommonSession->SaParams
    //
    if (!Ikev2SaParseSaPayload (IkeSaSession, SaPayload, IkePacket->Header->Flags)) {
      Status = EFI_INVALID_PARAMETER;
      goto CheckError;
    }

    //
    // 3. If Initiator, the NoncePayload is Nr_b.
    //
    IKEV2_DUMP_STATE (IkeSaSession->SessionCommon.State, IkeStateAuth);
    IkeSaSession->NrBlock             = NonceBuffer;
    IkeSaSession->NrBlkSize           = NonceSize;
    IkeSaSession->SessionCommon.State = IkeStateAuth;
    IkeSaSession->ResponderCookie     = IkePacket->Header->ResponderCookie;

    //
    // 4. Change the state of IkeSaSession
    //
    IkeSaSession->SessionCommon.State = IkeStateAuth;
  } else {
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_INIT
    //
    if (IkePacket->Header->Flags != IKE_HEADER_FLAGS_INIT) {
      Status = EFI_INVALID_PARAMETER;
      goto CheckError;
    }

    //
    // 2. Parse the SA payload and find out the perfered one
    //    and fill in the SA parameters into CommonSession->SaParams and SaData into
    //    IkeSaSession for the responder SA payload generation.
    //
    if (!Ikev2SaParseSaPayload (IkeSaSession, SaPayload, IkePacket->Header->Flags)) {
      Status = EFI_INVALID_PARAMETER;
      goto CheckError;
    }

    //
    // 3. Generat Dh Y parivate Key
    //
    Status = Ikev2GenerateSaDhPublicKey (IkeSaSession);
    if (EFI_ERROR (Status)) {
      goto CheckError;
    }

    //
    // 4. If Responder, the NoncePayload is Ni_b and go to generate Nr_b.
    //
    IkeSaSession->NiBlock   = NonceBuffer;
    IkeSaSession->NiBlkSize = NonceSize;

    //
    // 5. Generate Nr_b
    //
    IkeSaSession->NrBlock   = IkeGenerateNonce (IKE_NONCE_SIZE);
    ASSERT_EFI_ERROR (IkeSaSession->NrBlock != NULL);
    IkeSaSession->NrBlkSize = IKE_NONCE_SIZE;

    //
    // 6. Save the Cookies
    //
    IkeSaSession->InitiatorCookie = IkePacket->Header->InitiatorCookie;
    IkeSaSession->ResponderCookie = IkeGenerateCookie ();
  }

  if (IkeSaSession->SessionCommon.PreferDhGroup != ((IKEV2_KEY_EXCHANGE *)KeyPayload->PayloadBuf)->DhGroup) {
    Status = EFI_INVALID_PARAMETER;
    goto CheckError;
  }
  //
  // Call Ikev2GenerateSaKeys to create SKEYID, SKEYID_d, SKEYID_a, SKEYID_e.
  //
  Status = Ikev2GenerateSaKeys (IkeSaSession, KeyPayload);
  if (EFI_ERROR(Status)) {
    goto CheckError;
  }
  return EFI_SUCCESS;

CheckError:
  if (NonceBuffer != NULL) {
    FreePool (NonceBuffer);
  }
  
  return Status;
}

/**
  Generates the IKEv2 packet for IKE_AUTH exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION.
  @param[in] Context    Context data passed by caller.

  @retval   Pointer to IKE Packet to be sent out.

**/
IKE_PACKET *
Ikev2AuthPskGenerator (
  IN UINT8         *SaSession,
  IN VOID          *Context
  )
{
  IKE_PACKET             *IkePacket;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKE_PAYLOAD            *IdPayload;
  IKE_PAYLOAD            *AuthPayload;
  IKE_PAYLOAD            *SaPayload;
  IKE_PAYLOAD            *TsiPayload;
  IKE_PAYLOAD            *TsrPayload;
  IKE_PAYLOAD            *NotifyPayload;
  IKE_PAYLOAD            *CpPayload;
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  

  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;
  ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (GetFirstNode (&IkeSaSession->ChildSaSessionList));

  CpPayload      = NULL;
  NotifyPayload  = NULL;
  
  //
  // 1. Allocate IKE Packet
  //
  IkePacket= IkePacketAlloc ();
  ASSERT (IkePacket != NULL);

  //
  // 1.a Fill the IkePacket Header.
  //
  IkePacket->Header->ExchangeType    = IKEV2_EXCHANGE_TYPE_AUTH;
  IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
  IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;
  IkePacket->Header->Version         = (UINT8)(2 << 4);
  if (ChildSaSession->SessionCommon.IsInitiator) {
    IkePacket->Header->NextPayload   = IKEV2_PAYLOAD_TYPE_ID_INIT;
  } else {
    IkePacket->Header->NextPayload   = IKEV2_PAYLOAD_TYPE_ID_RSP;
  }

  //
  // According to RFC4306_2.2, For the IKE_SA_INIT message the MessageID should 
  // be always number 0 and 1;
  //
  IkePacket->Header->MessageId = 1;

  if (IkeSaSession->SessionCommon.IsInitiator) {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_INIT;
  } else {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
  }

  //
  // 2. Generate ID Payload according to IP version and address.
  //
  IdPayload = Ikev2GenerateIdPayload (
                &IkeSaSession->SessionCommon,
                IKEV2_PAYLOAD_TYPE_AUTH
                );

  //
  // 3. Generate Auth Payload
  //    If it is tunnel mode, should create the configuration payload after the
  //    Auth payload.
  //
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {

    AuthPayload = Ikev2PskGenerateAuthPayload (
                    ChildSaSession->IkeSaSession,
                    IdPayload,
                    IKEV2_PAYLOAD_TYPE_SA,
                    FALSE
                    );
  } else {
    AuthPayload = Ikev2PskGenerateAuthPayload (
                    ChildSaSession->IkeSaSession,
                    IdPayload,
                    IKEV2_PAYLOAD_TYPE_CP,
                    FALSE
                    );
    if (IkeSaSession->SessionCommon.UdpService->IpVersion == IP_VERSION_4) {
      CpPayload = Ikev2GenerateCpPayload (
                    ChildSaSession->IkeSaSession,
                    IKEV2_PAYLOAD_TYPE_SA,
                    IKEV2_CFG_ATTR_INTERNAL_IP4_ADDRESS
                    );
    } else {
      CpPayload = Ikev2GenerateCpPayload (
                    ChildSaSession->IkeSaSession,
                    IKEV2_PAYLOAD_TYPE_SA,
                    IKEV2_CFG_ATTR_INTERNAL_IP6_ADDRESS
                    );
    }
  }

  //
  // 4. Generate SA Payload according to the SA Data in ChildSaSession
  //
  SaPayload = Ikev2GenerateSaPayload (
                ChildSaSession->SaData,
                IKEV2_PAYLOAD_TYPE_TS_INIT,
                IkeSessionTypeChildSa
                );

  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {
    //
    // Generate Tsi and Tsr.
    //
    TsiPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_TS_RSP,
                   FALSE
                   );

    TsrPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_NOTIFY,
                   FALSE
                   );

    //
    // Generate Notify Payload. If transport mode, there should have Notify
    // payload with TRANSPORT_MODE notification.
    //
    NotifyPayload = Ikev2GenerateNotifyPayload (
                      0,
                      IKEV2_PAYLOAD_TYPE_NONE,
                      0,
                      IKEV2_NOTIFICATION_USE_TRANSPORT_MODE,
                      NULL,
                      NULL,
                      0
                      );
  } else {
    //
    // Generate Tsr for Tunnel mode.
    //
    TsiPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_TS_RSP,
                   TRUE
                   );
    TsrPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_NONE,
                   FALSE
                   );
  }

  IKE_PACKET_APPEND_PAYLOAD (IkePacket, IdPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, AuthPayload);
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, CpPayload);
  }
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, SaPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, TsiPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, TsrPayload);
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, NotifyPayload);
  }

  return IkePacket;
}

/**
  Parses IKE_AUTH packet.

  @param[in]  SaSession   Pointer to the IKE_SA_SESSION related to this packet.
  @param[in]  IkePacket   Pointer to the IKE_AUTH packet to be parsered.

  @retval     EFI_INVALID_PARAMETER   The IKE packet is malformed or the SA 
                                      proposal is unacceptable.
  @retval     EFI_SUCCESS             The IKE packet is acceptable and the
                                      relative data is saved for furthure communication.

**/
EFI_STATUS 
Ikev2AuthPskParser (
  IN UINT8             *SaSession,
  IN IKE_PACKET        *IkePacket
  )
{
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKE_PAYLOAD            *IkePayload;
  IKE_PAYLOAD            *SaPayload;
  IKE_PAYLOAD            *IdiPayload;
  IKE_PAYLOAD            *IdrPayload;
  IKE_PAYLOAD            *AuthPayload;
  IKE_PAYLOAD            *TsiPayload;
  IKE_PAYLOAD            *TsrPayload;
  IKE_PAYLOAD            *VerifiedAuthPayload;
  LIST_ENTRY             *Entry;
  EFI_STATUS             Status;

  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;
  ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (GetFirstNode (&IkeSaSession->ChildSaSessionList));

  SaPayload   = NULL;
  IdiPayload  = NULL;
  IdrPayload  = NULL;
  AuthPayload = NULL;
  TsiPayload  = NULL;
  TsrPayload  = NULL;

  //
  // Iterate payloads to find the SaPayload/ID/AUTH/TS Payload.
  //
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);

    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_ID_INIT) {
      IdiPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_ID_RSP) {
      IdrPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_SA) {
      SaPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_AUTH) {
      AuthPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_TS_INIT) {
      TsiPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_TS_RSP) {
      TsrPayload = IkePayload;
    }
  }

  if ((SaPayload == NULL) || (AuthPayload == NULL) || (TsiPayload == NULL) || (TsrPayload == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((IdiPayload == NULL) && (IdrPayload == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check IkePacket Header is match the state
  //
  if (IkeSaSession->SessionCommon.IsInitiator) {
    
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_RESPOND
    //
    if ((IkePacket->Header->Flags != IKE_HEADER_FLAGS_RESPOND) ||
        (IkePacket->Header->ExchangeType != IKEV2_EXCHANGE_TYPE_AUTH)
        ) {
      return EFI_INVALID_PARAMETER;
    }

  } else {
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_INIT
    //
    if ((IkePacket->Header->Flags != IKE_HEADER_FLAGS_INIT) ||
        (IkePacket->Header->ExchangeType != IKEV2_EXCHANGE_TYPE_AUTH)
        ) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // 2. Parse the SA payload and Key Payload and find out the perferable one
    //    and fill in the Sa paramse into CommonSession->SaParams and SaData into
    //    IkeSaSession for the responder SA payload generation.
    //
  }

  //
  // Verify the Auth Payload.
  //
  VerifiedAuthPayload = Ikev2PskGenerateAuthPayload (
                          IkeSaSession,
                          IkeSaSession->SessionCommon.IsInitiator ? IdrPayload : IdiPayload,
                          IKEV2_PAYLOAD_TYPE_SA,
                          TRUE
                          );
  if ((VerifiedAuthPayload != NULL) &&
      (0 != CompareMem (
              VerifiedAuthPayload->PayloadBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER),
              AuthPayload->PayloadBuf + sizeof (IKEV2_COMMON_PAYLOAD_HEADER),
              VerifiedAuthPayload->PayloadSize - sizeof (IKEV2_COMMON_PAYLOAD_HEADER)
              ))) {
    return EFI_INVALID_PARAMETER;
  };

  //
  // 3. Parse the SA Payload to find out the cryptographic suite
  //    and fill in the Sa paramse into CommonSession->SaParams. If no acceptable
  //    porposal found, return EFI_INVALID_PARAMETER.
  //
  if (!Ikev2ChildSaParseSaPayload (ChildSaSession, SaPayload, IkePacket->Header->Flags)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 4. Parse TSi, TSr payloads.
  //
  if ((((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId !=
       ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId) &&
      (((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId != 0) &&
      (((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId != 0)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IkeSaSession->SessionCommon.IsInitiator) {
    //
    //TODO:check the Port range. Only support any port and one certain port here.
    //
    ChildSaSession->ProtoId    = ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId;
    ChildSaSession->LocalPort  = ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort;
    ChildSaSession->RemotePort = ((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort;
    //
    // Association a SPD with this SA.
    //
    Status = Ikev2ChildSaAssociateSpdEntry (ChildSaSession);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Associate the IkeSaSession's SPD to the first ChildSaSession's SPD.
    //
    if (ChildSaSession->IkeSaSession->Spd == NULL) {
      ChildSaSession->IkeSaSession->Spd = ChildSaSession->Spd;
      Ikev2ChildSaSessionSpdSelectorCreate (ChildSaSession);
    }
  } else {
    //
    //TODO:check the Port range.
    //
    if ((((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != 0) &&
        (((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != ChildSaSession->RemotePort)
        ) {
      return EFI_INVALID_PARAMETER;
    } 
    if ((((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != 0) &&
        (((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != ChildSaSession->LocalPort)
        ) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // For the tunnel mode, it should add the vitual IP address into the SA's SPD Selector.
    //
    if (ChildSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
      if (!ChildSaSession->IkeSaSession->SessionCommon.IsInitiator) {
        //
        // If it is tunnel mode, the UEFI part must be the initiator.
        //
        return EFI_INVALID_PARAMETER;
      }
      //
      // Get the Virtual IP address from the Tsi traffic selector. 
      // TODO: check the CFG reply payload
      //
      CopyMem (
        &ChildSaSession->SpdSelector->LocalAddress[0].Address,
        TsiPayload->PayloadBuf + sizeof (IKEV2_TS) + sizeof (TRAFFIC_SELECTOR),
        (ChildSaSession->SessionCommon.UdpService->IpVersion == IP_VERSION_4) ?
        sizeof (EFI_IPv4_ADDRESS) : sizeof (EFI_IPv6_ADDRESS)
        );
      }    
  }

  //
  // 5. Generate keymats for IPsec protocol.
  //
  Ikev2GenerateChildSaKeys (ChildSaSession, NULL);
  if (IkeSaSession->SessionCommon.IsInitiator) {
    //
    // 6. Change the state of IkeSaSession
    //
    IKEV2_DUMP_STATE (IkeSaSession->SessionCommon.State, IkeStateIkeSaEstablished);
    IkeSaSession->SessionCommon.State = IkeStateIkeSaEstablished;
  }
  
  return EFI_SUCCESS;
}

/**
  Gernerates IKEv2 packet for IKE_SA_INIT exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION related to the exchange.
  @param[in] Context    Context Data passed by caller.

  @retval EFI_SUCCESS   The IKE packet generation succeeded.
  @retval Others        The IKE packet generation failed.

**/
IKE_PACKET*
Ikev2InitCertGenerator (
  IN UINT8           *SaSession,
  IN VOID            *Context
  ) 
{
  IKE_PACKET         *IkePacket;
  IKE_PAYLOAD        *CertReqPayload;
  LIST_ENTRY         *Node;
  IKE_PAYLOAD        *NoncePayload;

  if (!FeaturePcdGet (PcdIpsecCertificateEnabled)) {
    return NULL;
  }

  //
  // The first two messages exchange is same between PSK and Cert.
  //
  IkePacket = Ikev2InitPskGenerator (SaSession, Context);

  if ((IkePacket != NULL) && (!((IKEV2_SA_SESSION *)SaSession)->SessionCommon.IsInitiator)) {
    //
    // Add the Certification Request Payload
    //
    CertReqPayload = Ikev2GenerateCertificatePayload (
                       (IKEV2_SA_SESSION *)SaSession,
                       IKEV2_PAYLOAD_TYPE_NONE,
                       (UINT8*)PcdGetPtr(PcdIpsecUefiCaFile),
                       PcdGet32(PcdIpsecUefiCaFileSize),
                       IKEV2_CERT_ENCODEING_HASH_AND_URL_OF_X509_CERT,
                       TRUE
                       );
    //
    // Change Nonce Payload Next payload type.
    //
    IKE_PACKET_END_PAYLOAD (IkePacket, Node);
    NoncePayload = IKE_PAYLOAD_BY_PACKET (Node);
    ((IKEV2_NONCE *)NoncePayload->PayloadBuf)->Header.NextPayload = IKEV2_PAYLOAD_TYPE_CERTREQ;

    //
    // Add Certification Request Payload
    //
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, CertReqPayload);
  }

  return IkePacket;
}

/**
  Parses the IKEv2 packet for IKE_SA_INIT exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION related to the exchange.
  @param[in] IkePacket  The received IKEv2 packet to be parsed.

  @retval EFI_SUCCESS            The IKEv2 packet is acceptable and the relative data is
                                 saved for furthure communication.
  @retval EFI_INVALID_PARAMETER  The IKE packet is malformed or the SA proposal is unacceptable.                        
  @retval EFI_UNSUPPORTED        The certificate authentication is not supported.

**/
EFI_STATUS
Ikev2InitCertParser (
  IN UINT8            *SaSession,
  IN IKE_PACKET       *IkePacket
  )
{
  if (!FeaturePcdGet (PcdIpsecCertificateEnabled)) {
    return EFI_UNSUPPORTED;
  } 
  
  //
  // The first two messages exchange is same between PSK and Cert.
  // Todo: Parse Certificate Request from responder Initial Exchange. 
  //
  return Ikev2InitPskParser (SaSession, IkePacket);
}

/**
  Generates the IKEv2 packet for IKE_AUTH exchange.

  @param[in] SaSession  Pointer to IKEV2_SA_SESSION.
  @param[in] Context    Context data passed by caller.

  @retval Pointer to IKEv2 Packet to be sent out.

**/
IKE_PACKET *
Ikev2AuthCertGenerator (
  IN UINT8         *SaSession,
  IN VOID          *Context
  )
{
  IKE_PACKET             *IkePacket;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKE_PAYLOAD            *IdPayload;
  IKE_PAYLOAD            *AuthPayload;
  IKE_PAYLOAD            *SaPayload;
  IKE_PAYLOAD            *TsiPayload;
  IKE_PAYLOAD            *TsrPayload;
  IKE_PAYLOAD            *NotifyPayload;
  IKE_PAYLOAD            *CpPayload;
  IKE_PAYLOAD            *CertPayload;
  IKE_PAYLOAD            *CertReqPayload;
  IKEV2_CHILD_SA_SESSION *ChildSaSession;

  if (!FeaturePcdGet (PcdIpsecCertificateEnabled)) {
    return NULL;
  }

  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;
  ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (GetFirstNode (&IkeSaSession->ChildSaSessionList));

  CpPayload      = NULL;
  NotifyPayload  = NULL;
  CertPayload    = NULL;
  CertReqPayload = NULL;

  //
  // 1. Allocate IKE Packet
  //
  IkePacket= IkePacketAlloc ();
  ASSERT (IkePacket != NULL);

  //
  // 1.a Fill the IkePacket Header.
  //
  IkePacket->Header->ExchangeType    = IKEV2_EXCHANGE_TYPE_AUTH;
  IkePacket->Header->InitiatorCookie = IkeSaSession->InitiatorCookie;
  IkePacket->Header->ResponderCookie = IkeSaSession->ResponderCookie;
  IkePacket->Header->Version         = (UINT8)(2 << 4);
  if (ChildSaSession->SessionCommon.IsInitiator) {
    IkePacket->Header->NextPayload   = IKEV2_PAYLOAD_TYPE_ID_INIT;
  } else {
    IkePacket->Header->NextPayload   = IKEV2_PAYLOAD_TYPE_ID_RSP;
  }

  //
  // According to RFC4306_2.2, For the IKE_SA_INIT message the MessageID should
  // be always number 0 and 1;
  //
  IkePacket->Header->MessageId = 1;

  if (IkeSaSession->SessionCommon.IsInitiator) {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_INIT;
  } else {
    IkePacket->Header->Flags = IKE_HEADER_FLAGS_RESPOND;
  }

  //
  // 2. Generate ID Payload according to IP version and address.
  //
  IdPayload = Ikev2GenerateCertIdPayload (
                &IkeSaSession->SessionCommon,
                IKEV2_PAYLOAD_TYPE_CERT,
                (UINT8 *)PcdGetPtr (PcdIpsecUefiCertificate),
                PcdGet32 (PcdIpsecUefiCertificateSize)
                );

  //
  // 3. Generate Certificate Payload
  //
  CertPayload = Ikev2GenerateCertificatePayload (
                  IkeSaSession,
                  (UINT8)(IkeSaSession->SessionCommon.IsInitiator ? IKEV2_PAYLOAD_TYPE_CERTREQ : IKEV2_PAYLOAD_TYPE_AUTH),
                  (UINT8 *)PcdGetPtr (PcdIpsecUefiCertificate),
                  PcdGet32 (PcdIpsecUefiCertificateSize),
                  IKEV2_CERT_ENCODEING_X509_CERT_SIGN,
                  FALSE
                  );
  if (IkeSaSession->SessionCommon.IsInitiator) {
    CertReqPayload = Ikev2GenerateCertificatePayload (
                       IkeSaSession,
                       IKEV2_PAYLOAD_TYPE_AUTH,
                       (UINT8 *)PcdGetPtr (PcdIpsecUefiCertificate),
                       PcdGet32 (PcdIpsecUefiCertificateSize),
                       IKEV2_CERT_ENCODEING_HASH_AND_URL_OF_X509_CERT,
                       TRUE
                       );
  }

  //
  // 4. Generate Auth Payload
  //    If it is tunnel mode, should create the configuration payload after the
  //    Auth payload.
  //
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {
    AuthPayload = Ikev2CertGenerateAuthPayload (
                    ChildSaSession->IkeSaSession,
                    IdPayload,
                    IKEV2_PAYLOAD_TYPE_SA,
                    FALSE,
                    (UINT8 *)PcdGetPtr (PcdIpsecUefiCertificateKey),
                    PcdGet32 (PcdIpsecUefiCertificateKeySize),
                    ChildSaSession->IkeSaSession->Pad->Data->AuthData,
                    ChildSaSession->IkeSaSession->Pad->Data->AuthDataSize
                    );
  } else {
    AuthPayload = Ikev2CertGenerateAuthPayload (
                    ChildSaSession->IkeSaSession,
                    IdPayload,
                    IKEV2_PAYLOAD_TYPE_CP,
                    FALSE,
                    (UINT8 *)PcdGetPtr (PcdIpsecUefiCertificateKey),
                    PcdGet32 (PcdIpsecUefiCertificateKeySize),
                    ChildSaSession->IkeSaSession->Pad->Data->AuthData,
                    ChildSaSession->IkeSaSession->Pad->Data->AuthDataSize
                    );
    if (IkeSaSession->SessionCommon.UdpService->IpVersion == IP_VERSION_4) {
      CpPayload = Ikev2GenerateCpPayload (
                    ChildSaSession->IkeSaSession,
                    IKEV2_PAYLOAD_TYPE_SA,
                    IKEV2_CFG_ATTR_INTERNAL_IP4_ADDRESS
                    );
    } else {
      CpPayload = Ikev2GenerateCpPayload (
                    ChildSaSession->IkeSaSession,
                    IKEV2_PAYLOAD_TYPE_SA,
                    IKEV2_CFG_ATTR_INTERNAL_IP6_ADDRESS
                    );
    }
  }

  //
  // 5. Generate SA Payload according to the Sa Data in ChildSaSession
  //
  SaPayload = Ikev2GenerateSaPayload (
                ChildSaSession->SaData,
                IKEV2_PAYLOAD_TYPE_TS_INIT,
                IkeSessionTypeChildSa
                );

  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {
    //
    // Generate Tsi and Tsr.
    //
    TsiPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_TS_RSP,
                   FALSE
                   );

    TsrPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_NOTIFY,
                   FALSE
                   );

    //
    // Generate Notify Payload. If transport mode, there should have Notify 
    // payload with TRANSPORT_MODE notification.
    //
    NotifyPayload = Ikev2GenerateNotifyPayload (
                      0,
                      IKEV2_PAYLOAD_TYPE_NONE,
                      0,
                      IKEV2_NOTIFICATION_USE_TRANSPORT_MODE,
                      NULL,
                      NULL,
                      0
                      );
  } else {
    //
    // Generate Tsr for Tunnel mode.
    //
    TsiPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_TS_RSP,
                   TRUE
                   );
    TsrPayload = Ikev2GenerateTsPayload (
                   ChildSaSession,
                   IKEV2_PAYLOAD_TYPE_NONE,
                   FALSE
                   );
  }

  IKE_PACKET_APPEND_PAYLOAD (IkePacket, IdPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, CertPayload);
  if (IkeSaSession->SessionCommon.IsInitiator) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, CertReqPayload);
  }
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, AuthPayload);
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, CpPayload);
  }
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, SaPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, TsiPayload);
  IKE_PACKET_APPEND_PAYLOAD (IkePacket, TsrPayload);
  if (IkeSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTransport) {
    IKE_PACKET_APPEND_PAYLOAD (IkePacket, NotifyPayload);
  }

  return IkePacket;
}

/**
  Parses IKE_AUTH packet.

  @param[in]  SaSession   Pointer to the IKE_SA_SESSION related to this packet.
  @param[in]  IkePacket   Pointer to the IKE_AUTH packet to be parsered.

  @retval     EFI_INVALID_PARAMETER   The IKEv2 packet is malformed or the SA
                                      proposal is unacceptable.
  @retval     EFI_SUCCESS             The IKE packet is acceptable and the
                                      relative data is saved for furthure communication.
  @retval     EFI_UNSUPPORTED         The certificate authentication is not supported.

**/
EFI_STATUS
Ikev2AuthCertParser (
  IN UINT8             *SaSession,
  IN IKE_PACKET        *IkePacket
  )
{
  IKEV2_CHILD_SA_SESSION *ChildSaSession;
  IKEV2_SA_SESSION       *IkeSaSession;
  IKE_PAYLOAD            *IkePayload;
  IKE_PAYLOAD            *SaPayload;
  IKE_PAYLOAD            *IdiPayload;
  IKE_PAYLOAD            *IdrPayload;
  IKE_PAYLOAD            *AuthPayload;
  IKE_PAYLOAD            *TsiPayload;
  IKE_PAYLOAD            *TsrPayload;
  IKE_PAYLOAD            *CertPayload;
  IKE_PAYLOAD            *VerifiedAuthPayload;
  LIST_ENTRY             *Entry;
  EFI_STATUS             Status;

  if (!FeaturePcdGet (PcdIpsecCertificateEnabled)) {
    return EFI_UNSUPPORTED;
  }

  IkeSaSession   = (IKEV2_SA_SESSION *) SaSession;
  ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (GetFirstNode (&IkeSaSession->ChildSaSessionList));

  SaPayload           = NULL;
  IdiPayload          = NULL;
  IdrPayload          = NULL;
  AuthPayload         = NULL;
  TsiPayload          = NULL;
  TsrPayload          = NULL;
  CertPayload         = NULL;
  VerifiedAuthPayload = NULL;
  Status              = EFI_INVALID_PARAMETER;

  //
  // Iterate payloads to find the SaPayload/ID/AUTH/TS Payload.
  //
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);

    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_ID_INIT) {
      IdiPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_ID_RSP) {
      IdrPayload = IkePayload;
    }

    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_SA) {
      SaPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_AUTH) {
      AuthPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_TS_INIT) {
      TsiPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_TS_RSP) {
      TsrPayload = IkePayload;
    }
    if (IkePayload->PayloadType == IKEV2_PAYLOAD_TYPE_CERT) {
      CertPayload = IkePayload;
    }
  }

  if ((SaPayload == NULL) || (AuthPayload == NULL) || (TsiPayload == NULL) || 
      (TsrPayload == NULL) || (CertPayload == NULL)) {
    goto Exit;
  }
  if ((IdiPayload == NULL) && (IdrPayload == NULL)) {
    goto Exit;
  }

  //
  // Check IkePacket Header is match the state
  //
  if (IkeSaSession->SessionCommon.IsInitiator) {
    
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_RESPOND
    //
    if ((IkePacket->Header->Flags != IKE_HEADER_FLAGS_RESPOND) ||
        (IkePacket->Header->ExchangeType != IKEV2_EXCHANGE_TYPE_AUTH)) {
      goto Exit;
    }
  } else {
    //
    // 1. Check the IkePacket->Hdr == IKE_HEADER_FLAGS_INIT
    //
    if ((IkePacket->Header->Flags != IKE_HEADER_FLAGS_INIT) ||
        (IkePacket->Header->ExchangeType != IKEV2_EXCHANGE_TYPE_AUTH)) {
      goto Exit;
    }
  }

  //
  // Verify the Auth Payload.
  //
  VerifiedAuthPayload = Ikev2CertGenerateAuthPayload (
                          IkeSaSession,
                          IkeSaSession->SessionCommon.IsInitiator ? IdrPayload:IdiPayload,
                          IKEV2_PAYLOAD_TYPE_SA,
                          TRUE,
                          NULL,
                          0,
                          NULL,
                          0
                          );

  if ((VerifiedAuthPayload != NULL) &&
      (!IpSecCryptoIoVerifySignDataByCertificate (
          CertPayload->PayloadBuf + sizeof (IKEV2_CERT),
          CertPayload->PayloadSize - sizeof (IKEV2_CERT),
          (UINT8 *)PcdGetPtr (PcdIpsecUefiCaFile),
          PcdGet32 (PcdIpsecUefiCaFileSize),
          VerifiedAuthPayload->PayloadBuf + sizeof (IKEV2_AUTH),
          VerifiedAuthPayload->PayloadSize - sizeof (IKEV2_AUTH),
          AuthPayload->PayloadBuf + sizeof (IKEV2_AUTH),
          AuthPayload->PayloadSize - sizeof (IKEV2_AUTH)
          ))) {
    goto Exit;
  }

  //
  // 3. Parse the SA Payload to find out the cryptographic suite
  //    and fill in the SA paramse into CommonSession->SaParams. If no acceptable
  //    porposal found, return EFI_INVALID_PARAMETER.
  //
  if (!Ikev2ChildSaParseSaPayload (ChildSaSession, SaPayload, IkePacket->Header->Flags)) {
    goto Exit;
  }

  //
  // 4. Parse TSi, TSr payloads.
  //
  if ((((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId !=
      ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId) &&
      (((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId != 0) &&
      (((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId != 0)
      ) {
    goto Exit;
  }

  if (!IkeSaSession->SessionCommon.IsInitiator) {
    //
    //Todo:check the Port range. Only support any port and one certain port here.
    //
    ChildSaSession->ProtoId    = ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->IpProtocolId;
    ChildSaSession->LocalPort  = ((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort;
    ChildSaSession->RemotePort = ((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort;
    //
    // Association a SPD with this SA.
    //
    if (EFI_ERROR (Ikev2ChildSaAssociateSpdEntry (ChildSaSession))) {
      goto Exit;
    }
    //
    // Associate the IkeSaSession's SPD to the first ChildSaSession's SPD.
    //
    if (ChildSaSession->IkeSaSession->Spd == NULL) {
      ChildSaSession->IkeSaSession->Spd = ChildSaSession->Spd;
      Ikev2ChildSaSessionSpdSelectorCreate (ChildSaSession);
    }
  } else {
    //
    // Todo:check the Port range.
    //
    if ((((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != 0) &&
        (((TRAFFIC_SELECTOR *)(TsrPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != ChildSaSession->RemotePort)
        ) {
      goto Exit;
    } 
    if ((((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != 0) &&
        (((TRAFFIC_SELECTOR *)(TsiPayload->PayloadBuf + sizeof (IKEV2_TS)))->StartPort != ChildSaSession->LocalPort)
        ) {
      goto Exit;
    }
    //
    // For the tunnel mode, it should add the vitual IP address into the SA's SPD Selector.
    //
    if (ChildSaSession->Spd->Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
      if (!ChildSaSession->IkeSaSession->SessionCommon.IsInitiator) {
        //
        // If it is tunnel mode, the UEFI part must be the initiator.
        //
        goto Exit;
      }
      //
      // Get the Virtual IP address from the Tsi traffic selector. 
      // TODO: check the CFG reply payload
      //
      CopyMem (
        &ChildSaSession->SpdSelector->LocalAddress[0].Address,
        TsiPayload->PayloadBuf + sizeof (IKEV2_TS) + sizeof (TRAFFIC_SELECTOR),
        (ChildSaSession->SessionCommon.UdpService->IpVersion == IP_VERSION_4) ?
        sizeof (EFI_IPv4_ADDRESS) : sizeof (EFI_IPv6_ADDRESS)
        );
    }
  }
  
  //
  // 5. Generat keymats for IPsec protocol.
  //
  Ikev2GenerateChildSaKeys (ChildSaSession, NULL);
  if (IkeSaSession->SessionCommon.IsInitiator) {
    //
    // 6. Change the state of IkeSaSession
    //
    IKEV2_DUMP_STATE (IkeSaSession->SessionCommon.State, IkeStateIkeSaEstablished);
    IkeSaSession->SessionCommon.State = IkeStateIkeSaEstablished;
  }

  Status = EFI_SUCCESS;

Exit:
  if (VerifiedAuthPayload != NULL) {
    IkePayloadFree (VerifiedAuthPayload);
  }
  return Status;
}

/**
  Generates the DH Public Key.

  This generates the DH local public key and store it in the IKE SA Session's GxBuffer.

  @param[in]  IkeSaSession   Pointer to related IKE SA Session.

  @retval EFI_SUCCESS        The operation succeeded.
  @retval Others             The operation failed.

**/
EFI_STATUS
Ikev2GenerateSaDhPublicKey (
  IN IKEV2_SA_SESSION         *IkeSaSession
  )
{
  EFI_STATUS         Status;
  IKEV2_SESSION_KEYS *IkeKeys;

  IkeSaSession->IkeKeys = AllocateZeroPool (sizeof (IKEV2_SESSION_KEYS));
  ASSERT (IkeSaSession->IkeKeys != NULL);
  IkeKeys = IkeSaSession->IkeKeys;
  IkeKeys->DhBuffer = AllocateZeroPool (sizeof (IKEV2_DH_BUFFER));
  ASSERT (IkeKeys->DhBuffer != NULL);

  //
  // Init DH with the certain DH Group Description.
  //
  IkeKeys->DhBuffer->GxSize   = OakleyModpGroup[(UINT8)IkeSaSession->SessionCommon.PreferDhGroup].Size >> 3;
  IkeKeys->DhBuffer->GxBuffer = AllocateZeroPool (IkeKeys->DhBuffer->GxSize);
  ASSERT (IkeKeys->DhBuffer->GxBuffer != NULL);

  //
  // Get X PublicKey
  //
  Status = IpSecCryptoIoDhGetPublicKey (
             &IkeKeys->DhBuffer->DhContext,
             OakleyModpGroup[(UINT8)IkeSaSession->SessionCommon.PreferDhGroup].GroupGenerator,
             OakleyModpGroup[(UINT8)IkeSaSession->SessionCommon.PreferDhGroup].Size,
             OakleyModpGroup[(UINT8)IkeSaSession->SessionCommon.PreferDhGroup].Modulus,
             IkeKeys->DhBuffer->GxBuffer,
             &IkeKeys->DhBuffer->GxSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error CPLKeyManGetKeyParam X public key error Status = %r\n", Status));
    return Status;
  }

  IPSEC_DUMP_BUF ("DH Public Key (g^x) Dump", IkeKeys->DhBuffer->GxBuffer, IkeKeys->DhBuffer->GxSize);

  return EFI_SUCCESS;
}

/**
  Computes the DH Shared/Exchange Key.

  Given peer's public key, this function computes the exchanged common key and
  stores it in the IKEv2 SA Session's GxyBuffer.

  @param[in]  DhBuffer       Pointer to buffer of peer's puliic key.
  @param[in]  KePayload      Pointer to received key payload.
  
  @retval EFI_SUCCESS        The operation succeeded.
  @retval Otherwise          The operation failed.

**/
EFI_STATUS
Ikev2GenerateSaDhComputeKey (
  IN IKEV2_DH_BUFFER       *DhBuffer,
  IN IKE_PAYLOAD            *KePayload
  )
{
  EFI_STATUS          Status;
  IKEV2_KEY_EXCHANGE  *Ke;
  UINT8               *PubKey;
  UINTN               PubKeySize;

  Ke                  = (IKEV2_KEY_EXCHANGE *) KePayload->PayloadBuf;
  PubKey              = (UINT8 *) (Ke + 1);
  PubKeySize          = KePayload->PayloadSize - sizeof (IKEV2_KEY_EXCHANGE);
  DhBuffer->GxySize   = DhBuffer->GxSize;
  DhBuffer->GxyBuffer = AllocateZeroPool (DhBuffer->GxySize);
  ASSERT (DhBuffer->GxyBuffer != NULL);

  //
  // Get GxyBuf
  //
  Status = IpSecCryptoIoDhComputeKey (
             DhBuffer->DhContext,
             PubKey,
             PubKeySize,
             DhBuffer->GxyBuffer,
             &DhBuffer->GxySize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error CPLKeyManGetKeyParam Y session key error Status = %r\n", Status));
    return Status;
  }

  //
  // Create GxyBuf.
  //
  DhBuffer->GySize   = PubKeySize;
  DhBuffer->GyBuffer = AllocateZeroPool (DhBuffer->GySize);
  ASSERT (DhBuffer->GyBuffer != NULL);
  CopyMem (DhBuffer->GyBuffer, PubKey, DhBuffer->GySize);

  IPSEC_DUMP_BUF ("DH Public Key (g^y) Dump", DhBuffer->GyBuffer, DhBuffer->GySize);
  IPSEC_DUMP_BUF ("DH Shared Key (g^xy) Dump", DhBuffer->GxyBuffer, DhBuffer->GxySize);

  return EFI_SUCCESS;
}

/**
  Generates the IKE SKEYSEED and seven other secrets. SK_d, SK_ai, SK_ar, SK_ei, SK_er,
  SK_pi, SK_pr are keys for the furthure IKE exchange.

  @param[in]  IkeSaSession       Pointer to IKE SA Session.
  @param[in]  KePayload          Pointer to Key payload used to generate the Key.

  @retval EFI_UNSUPPORTED        If one or more Algorithm Id is not supported.
  @retval EFI_OUT_OF_RESOURCES   If there is no enough resource to be allocated to
                                 meet the requirement.
  @retval EFI_SUCCESS            The operation succeeded.

**/
EFI_STATUS
Ikev2GenerateSaKeys (
  IN IKEV2_SA_SESSION       *IkeSaSession,
  IN IKE_PAYLOAD            *KePayload
  )
{
  EFI_STATUS          Status;
  IKEV2_SA_PARAMS     *SaParams;
  PRF_DATA_FRAGMENT   Fragments[4];
  UINT64              InitiatorCookieNet;
  UINT64              ResponderCookieNet;
  UINT8               *KeyBuffer;
  UINTN               KeyBufferSize;
  UINTN               AuthAlgKeyLen;
  UINTN               EncryptAlgKeyLen;
  UINTN               IntegrityAlgKeyLen;
  UINTN               PrfAlgKeyLen;
  UINT8               *OutputKey;
  UINTN               OutputKeyLength;
  UINT8               *Digest;
  UINTN               DigestSize;

  Digest    = NULL;
  OutputKey = NULL;
  KeyBuffer = NULL;
  Status = EFI_SUCCESS;

  //
  // Generate Gxy
  //
  Ikev2GenerateSaDhComputeKey (IkeSaSession->IkeKeys->DhBuffer, KePayload);

  //
  // Get the key length of Authenticaion, Encryption, PRF, and Integrity.
  //
  SaParams           = IkeSaSession->SessionCommon.SaParams;
  AuthAlgKeyLen      = IpSecGetHmacDigestLength ((UINT8)SaParams->Prf);
  EncryptAlgKeyLen   = IpSecGetEncryptKeyLength ((UINT8)SaParams->EncAlgId);
  IntegrityAlgKeyLen = IpSecGetHmacDigestLength ((UINT8)SaParams->IntegAlgId);
  PrfAlgKeyLen       = IpSecGetHmacDigestLength ((UINT8)SaParams->Prf);

  //
  // If one or more algorithm is not support, return EFI_UNSUPPORTED.
  //
  if (AuthAlgKeyLen == 0 || 
      EncryptAlgKeyLen == 0 ||
      IntegrityAlgKeyLen == 0 ||
      PrfAlgKeyLen == 0
      ) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // Compute SKEYSEED = prf(Ni | Nr, g^ir)
  //
  KeyBufferSize = IkeSaSession->NiBlkSize + IkeSaSession->NrBlkSize;
  KeyBuffer     = AllocateZeroPool (KeyBufferSize);
  ASSERT (KeyBuffer != NULL);

  CopyMem (KeyBuffer, IkeSaSession->NiBlock, IkeSaSession->NiBlkSize);
  CopyMem (KeyBuffer + IkeSaSession->NiBlkSize, IkeSaSession->NrBlock, IkeSaSession->NrBlkSize);

  Fragments[0].Data     = IkeSaSession->IkeKeys->DhBuffer->GxyBuffer;
  Fragments[0].DataSize = IkeSaSession->IkeKeys->DhBuffer->GxySize;

  DigestSize = IpSecGetHmacDigestLength ((UINT8)SaParams->Prf);
  Digest     = AllocateZeroPool (DigestSize);

  if (Digest == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  IpSecCryptoIoHmac (
    (UINT8)SaParams->Prf,
    KeyBuffer,
    KeyBufferSize,
    (HASH_DATA_FRAGMENT *) Fragments,
    1,
    Digest,
    DigestSize
    );

  //
  // {SK_d | SK_ai | SK_ar | SK_ei | SK_er | SK_pi | SK_pr } = prf+
  //               (SKEYSEED, Ni | Nr | SPIi | SPIr )
  //
  Fragments[0].Data     = IkeSaSession->NiBlock;
  Fragments[0].DataSize = IkeSaSession->NiBlkSize;
  Fragments[1].Data     = IkeSaSession->NrBlock;
  Fragments[1].DataSize = IkeSaSession->NrBlkSize;
  InitiatorCookieNet    = HTONLL (IkeSaSession->InitiatorCookie);
  ResponderCookieNet    = HTONLL (IkeSaSession->ResponderCookie);
  Fragments[2].Data     = (UINT8 *)(&InitiatorCookieNet);
  Fragments[2].DataSize = sizeof (IkeSaSession->InitiatorCookie);
  Fragments[3].Data     = (UINT8 *)(&ResponderCookieNet);
  Fragments[3].DataSize = sizeof (IkeSaSession->ResponderCookie);

  IPSEC_DUMP_BUF (">>> NiBlock", IkeSaSession->NiBlock, IkeSaSession->NiBlkSize);
  IPSEC_DUMP_BUF (">>> NrBlock", IkeSaSession->NrBlock, IkeSaSession->NrBlkSize);
  IPSEC_DUMP_BUF (">>> InitiatorCookie", (UINT8 *)&IkeSaSession->InitiatorCookie, sizeof(UINT64));
  IPSEC_DUMP_BUF (">>> ResponderCookie", (UINT8 *)&IkeSaSession->ResponderCookie, sizeof(UINT64));
  
  OutputKeyLength = PrfAlgKeyLen + 
                    2 * EncryptAlgKeyLen +
                    2 * AuthAlgKeyLen +
                    2 * IntegrityAlgKeyLen;
  OutputKey       = AllocateZeroPool (OutputKeyLength);
  if (OutputKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Generate Seven Keymates.
  //
  Status = Ikev2SaGenerateKey (
             (UINT8)SaParams->Prf,
             Digest,
             DigestSize,
             OutputKey,
             OutputKeyLength,
             Fragments,
             4
             );
  if (EFI_ERROR(Status)) {
    goto Exit;
  }

  //
  // Save the seven keys into KeySession.
  // First, SK_d
  //
  IkeSaSession->IkeKeys->SkdKey     = AllocateZeroPool (PrfAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkdKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkdKeySize = PrfAlgKeyLen;
  CopyMem (IkeSaSession->IkeKeys->SkdKey, OutputKey, PrfAlgKeyLen);

  IPSEC_DUMP_BUF (">>> SK_D Key", IkeSaSession->IkeKeys->SkdKey, PrfAlgKeyLen);

  //
  // Second, Sk_ai
  //
  IkeSaSession->IkeKeys->SkAiKey     = AllocateZeroPool (IntegrityAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkAiKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkAiKeySize = IntegrityAlgKeyLen;
  CopyMem (IkeSaSession->IkeKeys->SkAiKey, OutputKey + PrfAlgKeyLen, IntegrityAlgKeyLen);
  
  IPSEC_DUMP_BUF (">>> SK_Ai Key", IkeSaSession->IkeKeys->SkAiKey, IkeSaSession->IkeKeys->SkAiKeySize);

  //
  // Third, Sk_ar
  //
  IkeSaSession->IkeKeys->SkArKey     = AllocateZeroPool (IntegrityAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkArKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkArKeySize = IntegrityAlgKeyLen;
  CopyMem (
    IkeSaSession->IkeKeys->SkArKey,
    OutputKey + PrfAlgKeyLen + IntegrityAlgKeyLen,
    IntegrityAlgKeyLen
    );
  
  IPSEC_DUMP_BUF (">>> SK_Ar Key", IkeSaSession->IkeKeys->SkArKey, IkeSaSession->IkeKeys->SkArKeySize);

  //
  // Fourth, Sk_ei
  //
  IkeSaSession->IkeKeys->SkEiKey     = AllocateZeroPool (EncryptAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkEiKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkEiKeySize = EncryptAlgKeyLen;
  
  CopyMem (
    IkeSaSession->IkeKeys->SkEiKey,
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen,
    EncryptAlgKeyLen
    );
  IPSEC_DUMP_BUF (
    ">>> SK_Ei Key", 
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen,
    EncryptAlgKeyLen
    );

  //
  // Fifth, Sk_er
  //
  IkeSaSession->IkeKeys->SkErKey     = AllocateZeroPool (EncryptAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkErKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkErKeySize = EncryptAlgKeyLen;

  CopyMem (
    IkeSaSession->IkeKeys->SkErKey,
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen + EncryptAlgKeyLen,
    EncryptAlgKeyLen
    );
  IPSEC_DUMP_BUF (
    ">>> SK_Er Key",
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen + EncryptAlgKeyLen,
    EncryptAlgKeyLen
    );

  //
  // Sixth, Sk_pi
  //
  IkeSaSession->IkeKeys->SkPiKey     = AllocateZeroPool (AuthAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkPiKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkPiKeySize = AuthAlgKeyLen;

  CopyMem (
    IkeSaSession->IkeKeys->SkPiKey,
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen +  2 * EncryptAlgKeyLen,
    AuthAlgKeyLen
    );
  IPSEC_DUMP_BUF (
    ">>> SK_Pi Key",
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen +  2 * EncryptAlgKeyLen,
    AuthAlgKeyLen
    );

  //
  // Seventh, Sk_pr
  //
  IkeSaSession->IkeKeys->SkPrKey     = AllocateZeroPool (AuthAlgKeyLen);
  if (IkeSaSession->IkeKeys->SkPrKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  IkeSaSession->IkeKeys->SkPrKeySize = AuthAlgKeyLen;

  CopyMem (
    IkeSaSession->IkeKeys->SkPrKey,
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen + 2 * EncryptAlgKeyLen + AuthAlgKeyLen,
    AuthAlgKeyLen
    ); 
  IPSEC_DUMP_BUF (
    ">>> SK_Pr Key",
    OutputKey + AuthAlgKeyLen + 2 * IntegrityAlgKeyLen + 2 * EncryptAlgKeyLen + AuthAlgKeyLen,
    AuthAlgKeyLen
    );


Exit:
  if (Digest != NULL) {
    FreePool (Digest);
  }
  if (KeyBuffer != NULL) {
    FreePool (KeyBuffer);
  }
  if (OutputKey != NULL) {
    FreePool (OutputKey);
  }

  if (EFI_ERROR(Status)) {
    if (IkeSaSession->IkeKeys->SkdKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkdKey);
    }
    if (IkeSaSession->IkeKeys->SkAiKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkAiKey);
    }
    if (IkeSaSession->IkeKeys->SkArKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkArKey);
    }
    if (IkeSaSession->IkeKeys->SkEiKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkEiKey);
    }
    if (IkeSaSession->IkeKeys->SkErKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkErKey);
    }
    if (IkeSaSession->IkeKeys->SkPiKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkPiKey);
    }
    if (IkeSaSession->IkeKeys->SkPrKey != NULL) {
      FreePool (IkeSaSession->IkeKeys->SkPrKey);
    }
  }

  
  return Status;
}

/**
  Generates the Keys for the furthure IPsec Protocol.

  @param[in]  ChildSaSession     Pointer to IKE Child SA Session.
  @param[in]  KePayload          Pointer to Key payload used to generate the Key.

  @retval EFI_UNSUPPORTED    If one or more Algorithm Id is not supported.
  @retval EFI_SUCCESS        The operation succeeded.

**/
EFI_STATUS
Ikev2GenerateChildSaKeys (
  IN IKEV2_CHILD_SA_SESSION     *ChildSaSession,
  IN IKE_PAYLOAD                *KePayload
  )
{
  EFI_STATUS          Status;
  IKEV2_SA_PARAMS     *SaParams;
  PRF_DATA_FRAGMENT   Fragments[3];
  UINTN               EncryptAlgKeyLen;
  UINTN               IntegrityAlgKeyLen;
  UINT8*              OutputKey;
  UINTN               OutputKeyLength;

  Status = EFI_SUCCESS;
  OutputKey = NULL;
  
  if (KePayload != NULL) {
    //
    // Generate Gxy 
    //
    Ikev2GenerateSaDhComputeKey (ChildSaSession->DhBuffer, KePayload);
    Fragments[0].Data     = ChildSaSession->DhBuffer->GxyBuffer;
    Fragments[0].DataSize = ChildSaSession->DhBuffer->GxySize;
  }

  Fragments[1].Data     = ChildSaSession->NiBlock;
  Fragments[1].DataSize = ChildSaSession->NiBlkSize;
  Fragments[2].Data     = ChildSaSession->NrBlock;
  Fragments[2].DataSize = ChildSaSession->NrBlkSize;

  //
  // Get the key length of Authenticaion, Encryption, PRF, and Integrity.
  //
  SaParams           = ChildSaSession->SessionCommon.SaParams;
  EncryptAlgKeyLen   = IpSecGetEncryptKeyLength ((UINT8)SaParams->EncAlgId);
  IntegrityAlgKeyLen = IpSecGetHmacDigestLength ((UINT8)SaParams->IntegAlgId);
  OutputKeyLength    = 2 * EncryptAlgKeyLen + 2 * IntegrityAlgKeyLen;

  if ((EncryptAlgKeyLen == 0) || (IntegrityAlgKeyLen == 0)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // 
  // If KePayload is not NULL, calculate KEYMAT = prf+(SK_d, g^ir (new) | Ni | Nr ),
  // otherwise, KEYMAT = prf+(SK_d, Ni | Nr )
  //
  OutputKey = AllocateZeroPool (OutputKeyLength);
  if (OutputKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Derive Key from the SkdKey Buffer.
  //
  Status = Ikev2SaGenerateKey (
             (UINT8)ChildSaSession->IkeSaSession->SessionCommon.SaParams->Prf,
             ChildSaSession->IkeSaSession->IkeKeys->SkdKey,
             ChildSaSession->IkeSaSession->IkeKeys->SkdKeySize,
             OutputKey,
             OutputKeyLength,
             KePayload == NULL ? &Fragments[1] : Fragments,
             KePayload == NULL ? 2 : 3
             );

  if (EFI_ERROR (Status)) {
    goto Exit;  
  }
  
  //
  // Copy KEYMATE (SK_ENCRYPT_i | SK_ENCRYPT_r | SK_INTEG_i | SK_INTEG_r) to
  // ChildKeyMates.
  //  
  if (!ChildSaSession->SessionCommon.IsInitiator) {

    // 
    // Initiator Encryption Key
    //
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncAlgoId    = (UINT8)SaParams->EncAlgId;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKeyLength = EncryptAlgKeyLen;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey       = AllocateZeroPool (EncryptAlgKeyLen);
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    CopyMem (
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey,
      OutputKey,
      EncryptAlgKeyLen
      );

    //
    // Initiator Authentication Key
    //
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthAlgoId    = (UINT8)SaParams->IntegAlgId;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKeyLength = IntegrityAlgKeyLen;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey       = AllocateZeroPool (IntegrityAlgKeyLen);
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }    
    
    CopyMem (
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey,
      OutputKey + EncryptAlgKeyLen,
      IntegrityAlgKeyLen
      );

    //
    // Responder Encrypt Key
    //
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncAlgoId    = (UINT8)SaParams->EncAlgId;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKeyLength = EncryptAlgKeyLen;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey       = AllocateZeroPool (EncryptAlgKeyLen);
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }   
    
    CopyMem (
      ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey,
      OutputKey + EncryptAlgKeyLen + IntegrityAlgKeyLen,
      EncryptAlgKeyLen
      );

    //
    // Responder Authentication Key
    //
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthAlgoId    = (UINT8)SaParams->IntegAlgId;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKeyLength = IntegrityAlgKeyLen;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey       = AllocateZeroPool (IntegrityAlgKeyLen);
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }   
    
    CopyMem (
      ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey,
      OutputKey + 2 * EncryptAlgKeyLen + IntegrityAlgKeyLen,
      IntegrityAlgKeyLen
      );
  } else {
    //
    // Initiator Encryption Key
    //
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncAlgoId    = (UINT8)SaParams->EncAlgId;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKeyLength = EncryptAlgKeyLen;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey       = AllocateZeroPool (EncryptAlgKeyLen);
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }   
    
    CopyMem (
      ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey,
      OutputKey,
      EncryptAlgKeyLen
      );

    //
    // Initiator Authentication Key
    //
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthAlgoId    = (UINT8)SaParams->IntegAlgId;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKeyLength = IntegrityAlgKeyLen;
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey       = AllocateZeroPool (IntegrityAlgKeyLen);
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }   
    
    CopyMem (
      ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey,
      OutputKey + EncryptAlgKeyLen,
      IntegrityAlgKeyLen
      );

    //
    // Responder Encryption Key
    //
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncAlgoId    = (UINT8)SaParams->EncAlgId;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKeyLength = EncryptAlgKeyLen;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey       = AllocateZeroPool (EncryptAlgKeyLen);
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }  
    
    CopyMem (
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey,
      OutputKey + EncryptAlgKeyLen + IntegrityAlgKeyLen,
      EncryptAlgKeyLen
      );

    //
    // Responder Authentication Key
    //
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthAlgoId    = (UINT8)SaParams->IntegAlgId;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKeyLength = IntegrityAlgKeyLen;
    ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey       = AllocateZeroPool (IntegrityAlgKeyLen);
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }   
    
    CopyMem (
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey,
      OutputKey + 2 * EncryptAlgKeyLen + IntegrityAlgKeyLen,
      IntegrityAlgKeyLen
      );
  }

  IPSEC_DUMP_BUF (
      " >>> Local Encryption Key",
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey,
      EncryptAlgKeyLen
      );
  IPSEC_DUMP_BUF (
      " >>> Remote Encryption Key",
      ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey,
      EncryptAlgKeyLen
      );
  IPSEC_DUMP_BUF (
      " >>> Local Authentication Key",
      ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey,
      IntegrityAlgKeyLen
      );
  IPSEC_DUMP_BUF (
    " >>> Remote Authentication Key",
    ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey,
    IntegrityAlgKeyLen
    );



Exit:
  if (EFI_ERROR (Status)) {
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey != NULL) {
      FreePool (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.EncKey);
    }
    if (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey != NULL) {
      FreePool (ChildSaSession->ChildKeymats.LocalPeerInfo.EspAlgoInfo.AuthKey);
    }
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey != NULL) {
      FreePool (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.EncKey);
    }
    if (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey != NULL) {
      FreePool (ChildSaSession->ChildKeymats.RemotePeerInfo.EspAlgoInfo.AuthKey);
    }
  }

  if (OutputKey != NULL) {
    FreePool (OutputKey);
  }
  
  return EFI_SUCCESS;
}

GLOBAL_REMOVE_IF_UNREFERENCED IKEV2_PACKET_HANDLER mIkev2Initial[][2] = {
  { //PSK
    { // IKEV2_INIT
      Ikev2InitPskParser,
      Ikev2InitPskGenerator
    },
    { //IKEV2_AUTH
      Ikev2AuthPskParser,
      Ikev2AuthPskGenerator
    }
  },
  { // CERT
    { // IKEV2_INIT
      Ikev2InitCertParser,
      Ikev2InitCertGenerator
    },
    { // IKEV2_AUTH
      Ikev2AuthCertParser,
      Ikev2AuthCertGenerator
    },
  },
};
