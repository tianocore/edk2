/** @file
  The general interfaces of the IKEv2.

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
#include "IkeService.h"
#include "IpSecConfigImpl.h"

/**
  General interface to intialize a IKEv2 negotiation.

  @param[in]  UdpService      Point to Udp Servcie used for the IKE packet sending.
  @param[in]  SpdEntry        Point to SPD entry related to this IKE negotiation.
  @param[in]  PadEntry        Point to PAD entry related to this IKE negotiation.
  @param[in]  RemoteIp        Point to IP Address which the remote peer to negnotiate.

  @retval EFI_SUCCESS           The operation is successful.
  @retval EFI_OUT_OF_RESOURCES  The required system resource can't be allocated.
  @retval EFI_INVALID_PARAMETER If UdpService or RemoteIp is NULL.
  @return Others                The operation is failed.

**/
EFI_STATUS
Ikev2NegotiateSa (
  IN IKE_UDP_SERVICE         *UdpService,
  IN IPSEC_SPD_ENTRY         *SpdEntry,
  IN IPSEC_PAD_ENTRY         *PadEntry,
  IN EFI_IP_ADDRESS          *RemoteIp
  )
{
  IPSEC_PRIVATE_DATA        *Private;
  IKEV2_SA_SESSION          *IkeSaSession;
  IKEV2_SESSION_COMMON      *SessionCommon;
  IKEV2_PACKET_HANDLER      Handler;
  IKE_PACKET                *IkePacket;
  EFI_STATUS                Status;

  if (UdpService == NULL || RemoteIp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IkePacket = NULL;
  Private   = (UdpService->IpVersion == IP_VERSION_4) ?
               IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
               IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  //
  // Lookup the remote ip address in the processing IKE SA session list.
  //
  IkeSaSession = Ikev2SaSessionLookup (&Private->Ikev2SessionList, RemoteIp);
  if (IkeSaSession != NULL) {
    //
    // Drop the packet if already in process.
    //
    return EFI_SUCCESS;
  }

  //
  // Create a new IkeSaSession and initiate the common parameters.
  //
  IkeSaSession = Ikev2SaSessionAlloc (Private, UdpService);
  if (IkeSaSession == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the specific parameters and state(IKE_STATE_INIT).
  //
  IkeSaSession->Spd            = SpdEntry;
  IkeSaSession->Pad            = PadEntry;
  SessionCommon                = &IkeSaSession->SessionCommon;
  SessionCommon->IsInitiator   = TRUE;
  SessionCommon->State         = IkeStateInit;
  //
  // TODO: Get the prefer DH Group from the IPsec Configuration, after the IPsecconfig application update
  // to support it.
  //
  SessionCommon->PreferDhGroup = IKEV2_TRANSFORM_ID_DH_1024MODP;

  CopyMem (
    &SessionCommon->RemotePeerIp,
    RemoteIp,
    sizeof (EFI_IP_ADDRESS)
    );

  CopyMem (
    &SessionCommon->LocalPeerIp,
    &UdpService->DefaultAddress,
    sizeof (EFI_IP_ADDRESS)
    );

  IKEV2_DUMP_STATE (SessionCommon->State, IkeStateInit);

  //
  // Initiate the SAD data of the IkeSaSession.
  //
  IkeSaSession->SaData = Ikev2InitializeSaData (SessionCommon);
  if (IkeSaSession->SaData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Generate an IKE request packet and send it out.
  //
  Handler   = mIkev2Initial[IkeSaSession->Pad->Data->AuthMethod][SessionCommon->State];
  IkePacket = Handler.Generator ((UINT8 *) IkeSaSession, NULL);
  if (IkePacket == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Status = Ikev2SendIkePacket (UdpService, (UINT8 *) SessionCommon, IkePacket, 0);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Insert the current IkeSaSession into the processing IKE SA list.
  //
  Ikev2SaSessionInsert (&Private->Ikev2SessionList, IkeSaSession, RemoteIp);

  return EFI_SUCCESS;

ON_ERROR:

  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }
  Ikev2SaSessionFree (IkeSaSession);
  return Status;
}

/**
  It is general interface to negotiate the Child SA.

  There are three situations which will invoke this function. First, create a CHILD
  SA if the input Context is NULL. Second, rekeying the existing IKE SA if the Context
  is a IKEv2_SA_SESSION. Third, rekeying the existing CHILD SA if the context is a
  IKEv2_CHILD_SA_SESSION.

  @param[in] IkeSaSession  Pointer to IKEv2_SA_SESSION related to this operation.
  @param[in] SpdEntry      Pointer to IPSEC_SPD_ENTRY related to this operation.
  @param[in] Context       The data pass from the caller.

  @retval EFI_SUCCESS          The operation is successful.
  @retval EFI_OUT_OF_RESOURCES The required system resource can't be allocated.
  @retval EFI_UNSUPPORTED      The condition is not support yet.
  @return Others               The operation is failed.

**/
EFI_STATUS
Ikev2NegotiateChildSa (
  IN UINT8           *IkeSaSession,
  IN IPSEC_SPD_ENTRY *SpdEntry,
  IN UINT8           *Context
  )
{
  EFI_STATUS                Status;
  IKEV2_SA_SESSION          *SaSession;
  IKEV2_CHILD_SA_SESSION    *ChildSaSession;
  IKEV2_SESSION_COMMON      *ChildSaCommon;
  IKE_PACKET                *IkePacket;
  IKE_UDP_SERVICE           *UdpService;

  SaSession  = (IKEV2_SA_SESSION*) IkeSaSession;
  UdpService = SaSession->SessionCommon.UdpService;
  IkePacket  = NULL;

  //
  // 1. Create another child SA session if context is null.
  // 2. Rekeying the IKE SA session if the context is IKE SA session.
  // 3. Rekeying the child SA session if the context is child SA session.
  //
  if (Context == NULL) {
    //
    // Create a new ChildSaSession and initiate the common parameters.
    //
    ChildSaSession = Ikev2ChildSaSessionAlloc (UdpService, SaSession);

    if (ChildSaSession == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Set the specific parameters and state as IKE_STATE_CREATE_CHILD.
    //
    ChildSaSession->Spd         = SpdEntry;
    ChildSaCommon               = &ChildSaSession->SessionCommon;
    ChildSaCommon->IsInitiator  = TRUE;
    ChildSaCommon->State        = IkeStateCreateChild;

    IKEV2_DUMP_STATE (ChildSaCommon->State, IkeStateCreateChild);

    if (SpdEntry->Selector->NextLayerProtocol != EFI_IPSEC_ANY_PROTOCOL) {
      ChildSaSession->ProtoId = SpdEntry->Selector->NextLayerProtocol;
    }

    if (SpdEntry->Selector->LocalPort != EFI_IPSEC_ANY_PORT) {
      ChildSaSession->LocalPort = SpdEntry->Selector->LocalPort;
    }

    if (SpdEntry->Selector->RemotePort != EFI_IPSEC_ANY_PORT) {
      ChildSaSession->RemotePort = SpdEntry->Selector->RemotePort;
    }
    //
    // Initiate the SAD data parameters of the ChildSaSession.
    //
    ChildSaSession->SaData = Ikev2InitializeSaData (ChildSaCommon);
    if (ChildSaSession->SaData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    //
    // Generate an IKE request packet and send it out.
    //
    IkePacket = mIkev2CreateChild.Generator ((UINT8 *) ChildSaSession, NULL);

    if (IkePacket == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    Status = Ikev2SendIkePacket (UdpService, (UINT8 *) ChildSaCommon, IkePacket, 0);

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Insert the ChildSaSession into processing child SA list.
    //
    Ikev2ChildSaSessionInsert (&SaSession->ChildSaSessionList, ChildSaSession);
  } else {
    //
    // TODO: Rekeying IkeSaSession or ChildSaSession, NOT support yet.
    //
    // Rekey IkeSa, set IkeSaSession->State and pass over IkeSaSession
    // Rekey ChildSa, set ChildSaSession->State and pass over ChildSaSession
    //
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (ChildSaSession->SaData != NULL) {
    FreePool (ChildSaSession->SaData);
  }

  if (ChildSaSession->SessionCommon.TimeoutEvent != NULL) {
    gBS->CloseEvent (ChildSaSession->SessionCommon.TimeoutEvent);
  }

  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }

  Ikev2ChildSaSessionFree (ChildSaSession);
  return Status;
}

/**
  It is general interface to start the Information Exchange.

  There are three situations which will invoke this function. First, deliver a Delete Information
  to delete the IKE SA if the input Context is NULL and the state of related IkeSaSeesion's is on
  deleting.Second, deliver a Notify Information without the contents if the input Context is NULL.
  Third, deliver a Notify Information if the input Context is not NULL.

  @param[in] IkeSaSession  Pointer to IKEv2_SA_SESSION related to this operation.
  @param[in] Context       Data passed by caller.

  @retval EFI_SUCCESS          The operation is successful.
  @retval EFI_OUT_OF_RESOURCES The required system resource can't be allocated.
  @retval EFI_UNSUPPORTED      The condition is not support yet.
  @return Otherwise            The operation is failed.

**/
EFI_STATUS
Ikev2NegotiateInfo (
  IN UINT8           *IkeSaSession,
  IN UINT8           *Context
  )
{

  EFI_STATUS                Status;
  IKEV2_SA_SESSION          *Ikev2SaSession;
  IKEV2_CHILD_SA_SESSION    *ChildSaSession;
  IKEV2_SESSION_COMMON      *SaCommon;
  IKE_PACKET                *IkePacket;
  IKE_UDP_SERVICE           *UdpService;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *NextEntry;

  Ikev2SaSession = (IKEV2_SA_SESSION *) IkeSaSession;
  UdpService     = Ikev2SaSession->SessionCommon.UdpService;
  SaCommon       = &Ikev2SaSession->SessionCommon;
  IkePacket      = NULL;
  Status         = EFI_SUCCESS;

  //
  // Delete the IKE SA.
  //
  if (Ikev2SaSession->SessionCommon.State == IkeStateSaDeleting && Context == NULL) {

    //
    // Generate Information Packet which contains the Delete Payload.
    //
    IkePacket = mIkev2Info.Generator ((UINT8 *) Ikev2SaSession, NULL);
    if (IkePacket == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Send out the Packet
    //
    if (UdpService != NULL && UdpService->Output != NULL) {
      Status = Ikev2SendIkePacket (UdpService, (UINT8 *) SaCommon, IkePacket, 0);

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }
  } else if (!IsListEmpty (&Ikev2SaSession->DeleteSaList)) {
    //
    // Iterate all Deleting Child SAs.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Ikev2SaSession->DeleteSaList) {
      ChildSaSession                      = IKEV2_CHILD_SA_SESSION_BY_DEL_SA (Entry);
      ChildSaSession->SessionCommon.State = IkeStateSaDeleting;

      //
      // Generate Information Packet which contains the Child SA Delete Payload.
      //
      IkePacket = mIkev2Info.Generator ((UINT8 *) ChildSaSession, NULL);
      if (IkePacket == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_ERROR;
      }

      //
      // Send out the Packet
      //
      if (UdpService != NULL && UdpService->Output != NULL) {
        Status = Ikev2SendIkePacket (UdpService, (UINT8 *) &ChildSaSession->SessionCommon, IkePacket, 0);

        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }
      }
    }
  }  else if (Context == NULL) {
    //
    // TODO: Deliver null notification message.
    //
  }  else if (Context != NULL) {
    //
    // TODO: Send out the Information Exchange which contains the Notify Payload.
    //
  }
ON_ERROR:
  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }
  return Status;

}

/**
  The general interface when received a IKEv2 packet for the IKE SA establishing.

  This function first find the related IKE SA Session according to the IKE packet's
  remote IP. Then call the corresponding function to handle this IKE packet according
  to the related IKE SA Session's State.

  @param[in] UdpService    Pointer of related UDP Service.
  @param[in] IkePacket     Data passed by caller.

**/
VOID
Ikev2HandleSa (
  IN IKE_UDP_SERVICE     *UdpService,
  IN IKE_PACKET          *IkePacket
  )
{
  EFI_STATUS              Status;
  IKEV2_SA_SESSION        *IkeSaSession;
  IKEV2_CHILD_SA_SESSION  *ChildSaSession;
  IKEV2_SESSION_COMMON    *IkeSaCommon;
  IKEV2_SESSION_COMMON    *ChildSaCommon;
  IKEV2_PACKET_HANDLER    Handler;
  IKE_PACKET              *Reply;
  IPSEC_PAD_ENTRY         *PadEntry;
  IPSEC_PRIVATE_DATA      *Private;
  BOOLEAN                 IsNewSession;

  Private = (UdpService->IpVersion == IP_VERSION_4) ?
             IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
             IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  ChildSaSession = NULL;
  ChildSaCommon  = NULL;

  //
  // Lookup the remote ip address in the processing IKE SA session list.
  //
  IkeSaSession = Ikev2SaSessionLookup (&Private->Ikev2SessionList, &IkePacket->RemotePeerIp);
  IsNewSession = FALSE;

  if (IkeSaSession == NULL) {
    //
    // Lookup the remote ip address in the pad.
    //
    PadEntry = IpSecLookupPadEntry (UdpService->IpVersion, &IkePacket->RemotePeerIp);
    if (PadEntry == NULL) {
      //
      // Drop the packet if no pad entry matched, this is the request from RFC 4301.
      //
      return ;
    }

    //
    // Create a new IkeSaSession and initiate the common parameters.
    //
    IkeSaSession             = Ikev2SaSessionAlloc (Private, UdpService);
    if (IkeSaSession == NULL) {
      return;
    }
    IkeSaSession->Pad        = PadEntry;
    IkeSaCommon              = &IkeSaSession->SessionCommon;
    IkeSaCommon->IsInitiator = FALSE;
    IkeSaCommon->State       = IkeStateInit;

    IKEV2_DUMP_STATE (IkeSaCommon->State, IkeStateInit);

    CopyMem (
      &IkeSaCommon->RemotePeerIp,
      &IkePacket->RemotePeerIp,
      sizeof (EFI_IP_ADDRESS)
      );

    CopyMem (
      &IkeSaCommon->LocalPeerIp,
      &UdpService->DefaultAddress,
      sizeof (EFI_IP_ADDRESS)
      );

    IsNewSession = TRUE;
  }

  //
  // Validate the IKE packet header.
  //
  if (!Ikev2ValidateHeader (IkeSaSession, IkePacket->Header)) {
    //
    // Drop the packet if invalid IKE header.
    //
    goto ON_ERROR;
  }

  //
  // Decode all the payloads in the IKE packet.
  //
  IkeSaCommon = &IkeSaSession->SessionCommon;
  Status      = Ikev2DecodePacket (IkeSaCommon, IkePacket, IkeSessionTypeIkeSa);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Try to reate the first ChildSa Session of that IkeSaSession.
  // If the IkeSaSession is responder, here will create the first ChildSaSession.
  //
  if (IkeSaCommon->State == IkeStateAuth && IsListEmpty(&IkeSaSession->ChildSaSessionList)) {
    //
    // Generate a piggyback child SA in IKE_STATE_AUTH state.
    //
    ASSERT (IsListEmpty (&IkeSaSession->ChildSaSessionList) &&
            IsListEmpty (&IkeSaSession->ChildSaEstablishSessionList));

    ChildSaSession = Ikev2ChildSaSessionCreate (IkeSaSession, UdpService);
    ChildSaCommon  = &ChildSaSession->SessionCommon;
  }

  //
  // Parse the IKE request packet according to the auth method and current state.
  //
  Handler = mIkev2Initial[IkeSaSession->Pad->Data->AuthMethod][IkeSaCommon->State];
  Status  = Handler.Parser ((UINT8 *)IkeSaSession, IkePacket);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Try to reate the first ChildSa Session of that IkeSaSession.
  // If the IkeSaSession is initiator, here will create the first ChildSaSession.
  //
  if (IkeSaCommon->State == IkeStateAuth && IsListEmpty(&IkeSaSession->ChildSaSessionList)) {
    //
    // Generate a piggyback child SA in IKE_STATE_AUTH state.
    //
    ASSERT (IsListEmpty (&IkeSaSession->ChildSaSessionList) &&
            IsListEmpty (&IkeSaSession->ChildSaEstablishSessionList));

    ChildSaSession = Ikev2ChildSaSessionCreate (IkeSaSession, UdpService);
    ChildSaCommon  = &ChildSaSession->SessionCommon;

    //
    // Initialize the SA data for Child SA.
    //
    ChildSaSession->SaData = Ikev2InitializeSaData (ChildSaCommon);
  }

  //
  // Generate the IKE response packet and send it out if not established.
  //
  if (IkeSaCommon->State != IkeStateIkeSaEstablished) {
    Handler = mIkev2Initial[IkeSaSession->Pad->Data->AuthMethod][IkeSaCommon->State];
    Reply   = Handler.Generator ((UINT8 *) IkeSaSession, NULL);
    if (Reply == NULL) {
      goto ON_ERROR;
    }

    Status = Ikev2SendIkePacket (UdpService, (UINT8 *) IkeSaCommon, Reply, 0);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
    if (!IkeSaCommon->IsInitiator) {
      IkeSaCommon->State ++;
      IKEV2_DUMP_STATE (IkeSaCommon->State - 1, IkeSaCommon->State);
    }
  }

  //
  // Insert the new IkeSaSession into the Private processing IkeSaSession List.
  //
  if (IsNewSession) {
    Ikev2SaSessionInsert (&Private->Ikev2SessionList, IkeSaSession, &IkePacket->RemotePeerIp);
  }

  //
  // Register the IkeSaSession and remove it from processing list.
  //
  if (IkeSaCommon->State == IkeStateIkeSaEstablished) {

    //
    // Remove the Established IKE SA Session from the IKE SA Session Negotiating list
    // and insert it into IKE SA Session Established list.
    //
    Ikev2SaSessionRemove (&Private->Ikev2SessionList, &IkePacket->RemotePeerIp);
    Ikev2SaSessionReg (IkeSaSession, Private);

    //
    // Remove the Established Child SA Session from the IkeSaSession->ChildSaSessionList
    // ,insert it into IkeSaSession->ChildSaEstablishSessionList and save this Child SA
    // into SAD.
    //
    ChildSaSession = IKEV2_CHILD_SA_SESSION_BY_IKE_SA (IkeSaSession->ChildSaSessionList.BackLink);
    Ikev2ChildSaSessionRemove (
      &IkeSaSession->ChildSaSessionList,
      ChildSaSession->LocalPeerSpi,
      IKEV2_ESTABLISHING_CHILDSA_LIST
      );
    Ikev2ChildSaSessionReg (ChildSaSession, Private);
  }

  return ;

ON_ERROR:
  if (ChildSaSession != NULL) {
    //
    // Remove the ChildSa from the list (Established list or Negotiating list).
    //
    RemoveEntryList (&ChildSaSession->ByIkeSa);
    Ikev2ChildSaSessionFree (ChildSaSession);
  }

  if (IsNewSession && IkeSaSession != NULL) {
    //
    // Remove the IkeSa from the list (Established list or Negotiating list).
    //
    if ((&IkeSaSession->BySessionTable)->ForwardLink != NULL &&
        !IsListEmpty (&IkeSaSession->BySessionTable
       )){
      RemoveEntryList (&IkeSaSession->BySessionTable);
    }
    Ikev2SaSessionFree (IkeSaSession);
  }

  return ;
}

/**

  The general interface when received a IKEv2 packet for the IKE Child SA establishing
  or IKE SA/CHILD SA rekeying.

  This function first find the related IKE SA Session according to the IKE packet's
  remote IP. Then call the corresponding function to handle this IKE packet according
  to the related IKE Child Session's State.

  @param[in] UdpService    Pointer of related UDP Service.
  @param[in] IkePacket     Data passed by caller.

**/
VOID
Ikev2HandleChildSa (
  IN IKE_UDP_SERVICE  *UdpService,
  IN IKE_PACKET       *IkePacket
  )
{
  EFI_STATUS                       Status;
  IKEV2_SA_SESSION                 *IkeSaSession;
  IKEV2_CREATE_CHILD_REQUEST_TYPE  RequestType;
  IKE_PACKET                       *Reply;
  IPSEC_PRIVATE_DATA               *Private;

  Private = (UdpService->IpVersion == IP_VERSION_4) ?
             IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
             IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  Reply   = NULL;

  //
  // Lookup the remote ip address in the processing IKE SA session list.
  //
  IkeSaSession = Ikev2SaSessionLookup (&Private->Ikev2EstablishedList, &IkePacket->RemotePeerIp);

  if (IkeSaSession == NULL) {
    //
    // Drop the packet if no IKE SA associated.
    //
    return ;
  }

  //
  // Validate the IKE packet header.
  //
  if (!Ikev2ValidateHeader (IkeSaSession, IkePacket->Header)) {
    //
    // Drop the packet if invalid IKE header.
    //
    return;
  }

  //
  // Decode all the payloads in the IKE packet.
  //
  Status = Ikev2DecodePacket (&IkeSaSession->SessionCommon, IkePacket, IkeSessionTypeIkeSa);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Get the request type: CreateChildSa/RekeyChildSa/RekeyIkeSa.
  //
  RequestType = Ikev2ChildExchangeRequestType (IkePacket);

  switch (RequestType) {
  case IkeRequestTypeCreateChildSa:
  case IkeRequestTypeRekeyChildSa:
  case IkeRequestTypeRekeyIkeSa:
    //
    // Parse the IKE request packet. Not support CREATE_CHILD_SA exchange yet, so
    // only EFI_UNSUPPORTED will be returned and that will trigger a reply with a
    // Notify payload of type NO_ADDITIONAL_SAS.
    //
    Status = mIkev2CreateChild.Parser ((UINT8 *) IkeSaSession, IkePacket);
    if (EFI_ERROR (Status)) {
      goto ON_REPLY;
    }

  default:
    //
    // No support.
    //
    return ;
  }

ON_REPLY:
  //
  // Generate the reply packet if needed and send it out.
  //
  if (IkePacket->Header->Flags != IKE_HEADER_FLAGS_RESPOND) {
    Reply = mIkev2CreateChild.Generator ((UINT8 *) IkeSaSession, &IkePacket->Header->MessageId);
    if (Reply != NULL) {
      Status = Ikev2SendIkePacket (UdpService, (UINT8 *) &(IkeSaSession->SessionCommon), Reply, 0);
      if (EFI_ERROR (Status)) {
        //
        //  Delete Reply payload.
        //
        if (Reply != NULL) {
          IkePacketFree (Reply);
        }
      }
    }
  }
  return ;
}

/**

  It is general interface to handle IKEv2 information Exchange.

  @param[in] UdpService  Point to IKE UPD Service related to this information exchange.
  @param[in] IkePacket   The IKE packet to be parsed.

**/
VOID
Ikev2HandleInfo (
  IN IKE_UDP_SERVICE  *UdpService,
  IN IKE_PACKET       *IkePacket
  )
{
  EFI_STATUS              Status;
  IKEV2_SESSION_COMMON    *SessionCommon;
  IKEV2_SA_SESSION        *IkeSaSession;
  IPSEC_PRIVATE_DATA      *Private;

  Private = (UdpService->IpVersion == IP_VERSION_4) ?
             IPSEC_PRIVATE_DATA_FROM_UDP4LIST(UdpService->ListHead) :
             IPSEC_PRIVATE_DATA_FROM_UDP6LIST(UdpService->ListHead);

  //
  // Lookup the remote ip address in the processing IKE SA session list.
  //
  IkeSaSession = Ikev2SaSessionLookup (&Private->Ikev2EstablishedList, &IkePacket->RemotePeerIp);

  if (IkeSaSession == NULL) {
    //
    // Drop the packet if no IKE SA associated.
    //
    return ;
  }
  //
  // Validate the IKE packet header.
  //
  if (!Ikev2ValidateHeader (IkeSaSession, IkePacket->Header)) {

    //
    // Drop the packet if invalid IKE header.
    //
    return;
  }

  SessionCommon = &IkeSaSession->SessionCommon;

  //
  // Decode all the payloads in the IKE packet.
  //
  Status = Ikev2DecodePacket (SessionCommon, IkePacket, IkeSessionTypeIkeSa);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = mIkev2Info.Parser ((UINT8 *)IkeSaSession, IkePacket);

  if (EFI_ERROR (Status)) {
    //
    // Drop the packet if fail to parse.
    //
    return;
  }
}

IKE_EXCHANGE_INTERFACE  mIkev1Exchange = {
  1,
  NULL, //Ikev1NegotiateSa
  NULL, //Ikev1NegotiateChildSa
  NULL,
  NULL, //Ikev1HandleSa,
  NULL, //Ikev1HandleChildSa
  NULL, //Ikev1HandleInfo
};

IKE_EXCHANGE_INTERFACE  mIkev2Exchange = {
  2,
  Ikev2NegotiateSa,
  Ikev2NegotiateChildSa,
  Ikev2NegotiateInfo,
  Ikev2HandleSa,
  Ikev2HandleChildSa,
  Ikev2HandleInfo
};

