/** @file
  Implement IP6 pesudo interface.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

/**
  Request Ip6OnFrameSentDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event              The transmit token's event.
  @param[in]  Context            The Context which is pointed to the token.

**/
VOID
EFIAPI
Ip6OnFrameSent (
  IN EFI_EVENT               Event,
  IN VOID                    *Context
  );

/**
  Fileter function to cancel all the frame related to an IP instance.

  @param[in]  Frame             The transmit request to test whether to cancel.
  @param[in]  Context           The context which is the Ip instance that issued
                                the transmit.

  @retval TRUE                  The frame belongs to this instance and is to be
                                removed.
  @retval FALSE                 The frame doesn't belong to this instance.

**/
BOOLEAN
Ip6CancelInstanceFrame (
  IN IP6_LINK_TX_TOKEN *Frame,
  IN VOID              *Context
  )
{
  if (Frame->IpInstance == (IP6_PROTOCOL *) Context) {
    return TRUE;
  }

  return FALSE;
}

/**
  Set the interface's address. This will trigger the DAD process for the
  address to set. To set an already set address, the lifetimes wil be
  updated to the new value passed in.

  @param[in]  Interface             The interface to set the address.
  @param[in]  Ip6Addr               The interface's to be assigned IPv6 address.
  @param[in]  IsAnycast             If TRUE, the unicast IPv6 address is anycast.
                                    Otherwise, it is not anycast.
  @param[in]  PrefixLength          The prefix length of the Ip6Addr.
  @param[in]  ValidLifetime         The valid lifetime for this address.
  @param[in]  PreferredLifetime     The preferred lifetime for this address.
  @param[in]  DadCallback           The caller's callback to trigger when DAD finishes.
                                    This is an optional parameter that may be NULL.
  @param[in]  Context               The context that will be passed to DadCallback.
                                    This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS               The interface is scheduled to be configured with
                                    the specified address.
  @retval EFI_OUT_OF_RESOURCES      Failed to set the interface's address due to
                                    lack of resources.

**/
EFI_STATUS
Ip6SetAddress (
  IN IP6_INTERFACE          *Interface,
  IN EFI_IPv6_ADDRESS       *Ip6Addr,
  IN BOOLEAN                IsAnycast,
  IN UINT8                  PrefixLength,
  IN UINT32                 ValidLifetime,
  IN UINT32                 PreferredLifetime,
  IN IP6_DAD_CALLBACK       DadCallback  OPTIONAL,
  IN VOID                   *Context     OPTIONAL
  )
{
  IP6_SERVICE            *IpSb;
  IP6_ADDRESS_INFO       *AddressInfo;
  LIST_ENTRY             *Entry;
  IP6_PREFIX_LIST_ENTRY  *PrefixEntry;
  UINT64                 Delay;
  IP6_DELAY_JOIN_LIST    *DelayNode;

  NET_CHECK_SIGNATURE (Interface, IP6_INTERFACE_SIGNATURE);

  IpSb = Interface->Service;

  if (Ip6IsOneOfSetAddress (IpSb, Ip6Addr, NULL, &AddressInfo)) {
    ASSERT (AddressInfo != NULL);
    //
    // Update the lifetime.
    //
    AddressInfo->ValidLifetime     = ValidLifetime;
    AddressInfo->PreferredLifetime = PreferredLifetime;

    if (DadCallback != NULL) {
      DadCallback (TRUE, Ip6Addr, Context);
    }

    return EFI_SUCCESS;
  }

  AddressInfo = (IP6_ADDRESS_INFO *) AllocatePool (sizeof (IP6_ADDRESS_INFO));
  if (AddressInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AddressInfo->Signature         = IP6_ADDR_INFO_SIGNATURE;
  IP6_COPY_ADDRESS (&AddressInfo->Address, Ip6Addr);
  AddressInfo->IsAnycast         = IsAnycast;
  AddressInfo->PrefixLength      = PrefixLength;
  AddressInfo->ValidLifetime     = ValidLifetime;
  AddressInfo->PreferredLifetime = PreferredLifetime;

  if (AddressInfo->PrefixLength == 0) {
    //
    // Find an appropriate prefix from on-link prefixes and update the prefixlength.
    // Longest prefix match is used here.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->OnlinkPrefix) {
      PrefixEntry = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);

      if (NetIp6IsNetEqual (&PrefixEntry->Prefix, &AddressInfo->Address, PrefixEntry->PrefixLength)) {
        AddressInfo->PrefixLength = PrefixEntry->PrefixLength;
        break;
      }
    }
  }

  if (AddressInfo->PrefixLength == 0) {
    //
    // If the prefix length is still zero, try the autonomous prefixes.
    // Longest prefix match is used here.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->AutonomousPrefix) {
      PrefixEntry = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);

      if (NetIp6IsNetEqual (&PrefixEntry->Prefix, &AddressInfo->Address, PrefixEntry->PrefixLength)) {
        AddressInfo->PrefixLength = PrefixEntry->PrefixLength;
        break;
      }
    }
  }

  if (AddressInfo->PrefixLength == 0) {
    //
    // BUGBUG: Stil fail, use 64 as the default prefix length.
    //
    AddressInfo->PrefixLength = IP6_LINK_LOCAL_PREFIX_LENGTH;
  }


  //
  // Node should delay joining the solicited-node mulitcast address by a random delay
  // between 0 and MAX_RTR_SOLICITATION_DELAY (1 second).
  // Thus queue the address to be processed in Duplicate Address Detection module
  // after the delay time (in milliseconds).
  //
  Delay = (UINT64) NET_RANDOM (NetRandomInitSeed ());
  Delay = MultU64x32 (Delay, IP6_ONE_SECOND_IN_MS);
  Delay = RShiftU64 (Delay, 32);

  DelayNode = (IP6_DELAY_JOIN_LIST *) AllocatePool (sizeof (IP6_DELAY_JOIN_LIST));
  if (DelayNode == NULL) {
    FreePool (AddressInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  DelayNode->DelayTime   = (UINT32) (DivU64x32 (Delay, IP6_TIMER_INTERVAL_IN_MS));
  DelayNode->Interface   = Interface;
  DelayNode->AddressInfo = AddressInfo;
  DelayNode->DadCallback = DadCallback;
  DelayNode->Context     = Context;

  InsertTailList (&Interface->DelayJoinList, &DelayNode->Link);
  return EFI_SUCCESS;
}

/**
  Create an IP6_INTERFACE.

  @param[in]  IpSb                  The IP6 service binding instance.
  @param[in]  LinkLocal             If TRUE, the instance is created for link-local address.
                                    Otherwise, it is not for a link-local address.

  @return Point to the created IP6_INTERFACE, otherwise NULL.

**/
IP6_INTERFACE *
Ip6CreateInterface (
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                LinkLocal
  )
{
  EFI_STATUS                Status;
  IP6_INTERFACE             *Interface;
  EFI_IPv6_ADDRESS          *Ip6Addr;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  Interface = AllocatePool (sizeof (IP6_INTERFACE));
  if (Interface == NULL) {
    return NULL;
  }

  Interface->Signature        = IP6_INTERFACE_SIGNATURE;
  Interface->RefCnt           = 1;

  InitializeListHead (&Interface->AddressList);
  Interface->AddressCount     = 0;
  Interface->Configured       = FALSE;

  Interface->Service          = IpSb;
  Interface->Controller       = IpSb->Controller;
  Interface->Image            = IpSb->Image;

  InitializeListHead (&Interface->ArpQues);
  InitializeListHead (&Interface->SentFrames);

  Interface->DupAddrDetect    = IpSb->Ip6ConfigInstance.DadXmits.DupAddrDetectTransmits;
  InitializeListHead (&Interface->DupAddrDetectList);

  InitializeListHead (&Interface->DelayJoinList);

  InitializeListHead (&Interface->IpInstances);
  Interface->PromiscRecv      = FALSE;

  if (!LinkLocal) {
    return Interface;
  }

  //
  // Get the link local addr
  //
  Ip6Addr = Ip6CreateLinkLocalAddr (IpSb);
  if (Ip6Addr == NULL) {
    goto ON_ERROR;
  }

  //
  // Perform DAD - Duplicate Address Detection.
  //
  Status = Ip6SetAddress (
             Interface,
             Ip6Addr,
             FALSE,
             IP6_LINK_LOCAL_PREFIX_LENGTH,
             (UINT32) IP6_INFINIT_LIFETIME,
             (UINT32) IP6_INFINIT_LIFETIME,
             NULL,
             NULL
             );

  FreePool (Ip6Addr);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return Interface;

ON_ERROR:

  FreePool (Interface);
  return NULL;
}

/**
  Free the interface used by IpInstance. All the IP instance with
  the same Ip/prefix pair share the same interface. It is reference
  counted. All the frames that haven't been sent will be cancelled.
  Because the IpInstance is optional, the caller must remove
  IpInstance from the interface's instance list.

  @param[in]  Interface         The interface used by the IpInstance.
  @param[in]  IpInstance        The IP instance that free the interface. NULL if
                                the IP driver is releasing the default interface.

**/
VOID
Ip6CleanInterface (
  IN  IP6_INTERFACE         *Interface,
  IN  IP6_PROTOCOL          *IpInstance           OPTIONAL
  )
{
  IP6_DAD_ENTRY             *Duplicate;
  IP6_DELAY_JOIN_LIST       *Delay;

  NET_CHECK_SIGNATURE (Interface, IP6_INTERFACE_SIGNATURE);
  ASSERT (Interface->RefCnt > 0);

  //
  // Remove all the pending transmit token related to this IP instance.
  //
  Ip6CancelFrames (Interface, EFI_ABORTED, Ip6CancelInstanceFrame, IpInstance);

  if (--Interface->RefCnt > 0) {
    return;
  }

  //
  // Destroy the interface if this is the last IP instance.
  // Remove all the system transmitted packets
  // from this interface, cancel the receive request if exists.
  //
  Ip6CancelFrames (Interface, EFI_ABORTED, Ip6CancelInstanceFrame, NULL);

  ASSERT (IsListEmpty (&Interface->IpInstances));
  ASSERT (IsListEmpty (&Interface->ArpQues));
  ASSERT (IsListEmpty (&Interface->SentFrames));

  while (!IsListEmpty (&Interface->DupAddrDetectList)) {
    Duplicate = NET_LIST_HEAD (&Interface->DupAddrDetectList, IP6_DAD_ENTRY, Link);
    NetListRemoveHead (&Interface->DupAddrDetectList);
    FreePool (Duplicate);
  }

  while (!IsListEmpty (&Interface->DelayJoinList)) {
    Delay = NET_LIST_HEAD (&Interface->DelayJoinList, IP6_DELAY_JOIN_LIST, Link);
    NetListRemoveHead (&Interface->DelayJoinList);
    FreePool (Delay);
  }

  Ip6RemoveAddr (Interface->Service, &Interface->AddressList, &Interface->AddressCount, NULL, 0);

  RemoveEntryList (&Interface->Link);
  FreePool (Interface);
}

/**
  Create and wrap a transmit request into a newly allocated IP6_LINK_TX_TOKEN.

  @param[in]  Interface         The interface to send out from.
  @param[in]  IpInstance        The IpInstance that transmit the packet.  NULL if
                                the packet is sent by the IP6 driver itself.
  @param[in]  Packet            The packet to transmit
  @param[in]  CallBack          Call back function to execute if transmission
                                finished.
  @param[in]  Context           Opaque parameter to the callback.

  @return The wrapped token if succeed or NULL.

**/
IP6_LINK_TX_TOKEN *
Ip6CreateLinkTxToken (
  IN IP6_INTERFACE          *Interface,
  IN IP6_PROTOCOL           *IpInstance    OPTIONAL,
  IN NET_BUF                *Packet,
  IN IP6_FRAME_CALLBACK     CallBack,
  IN VOID                   *Context
  )
{
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *MnpToken;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     *MnpTxData;
  IP6_LINK_TX_TOKEN                     *Token;
  EFI_STATUS                            Status;
  UINT32                                Count;

  Token = AllocatePool (sizeof (IP6_LINK_TX_TOKEN) + (Packet->BlockOpNum - 1) * sizeof (EFI_MANAGED_NETWORK_FRAGMENT_DATA));

  if (Token == NULL) {
    return NULL;
  }

  Token->Signature = IP6_LINK_TX_SIGNATURE;
  InitializeListHead (&Token->Link);

  Token->IpInstance = IpInstance;
  Token->CallBack   = CallBack;
  Token->Packet     = Packet;
  Token->Context    = Context;
  ZeroMem (&Token->DstMac, sizeof (EFI_MAC_ADDRESS));
  IP6_COPY_LINK_ADDRESS (&Token->SrcMac, &Interface->Service->SnpMode.CurrentAddress);

  MnpToken          = &(Token->MnpToken);
  MnpToken->Status  = EFI_NOT_READY;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ip6OnFrameSent,
                  Token,
                  &MnpToken->Event
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Token);
    return NULL;
  }

  MnpTxData                     = &Token->MnpTxData;
  MnpToken->Packet.TxData       = MnpTxData;

  MnpTxData->DestinationAddress = &Token->DstMac;
  MnpTxData->SourceAddress      = &Token->SrcMac;
  MnpTxData->ProtocolType       = IP6_ETHER_PROTO;
  MnpTxData->DataLength         = Packet->TotalSize;
  MnpTxData->HeaderLength       = 0;

  Count                         = Packet->BlockOpNum;

  NetbufBuildExt (Packet, (NET_FRAGMENT *) MnpTxData->FragmentTable, &Count);
  MnpTxData->FragmentCount      = (UINT16)Count;

  return Token;
}

/**
  Free the link layer transmit token. It will close the event,
  then free the memory used.

  @param[in]  Token                 Token to free.

**/
VOID
Ip6FreeLinkTxToken (
  IN IP6_LINK_TX_TOKEN      *Token
  )
{
  NET_CHECK_SIGNATURE (Token, IP6_LINK_TX_SIGNATURE);

  gBS->CloseEvent (Token->MnpToken.Event);
  FreePool (Token);
}

/**
  Callback function when the received packet is freed.
  Check Ip6OnFrameReceived for information.

  @param[in]  Context       Points to EFI_MANAGED_NETWORK_RECEIVE_DATA.

**/
VOID
EFIAPI
Ip6RecycleFrame (
  IN VOID                   *Context
  )
{
  EFI_MANAGED_NETWORK_RECEIVE_DATA  *RxData;

  RxData = (EFI_MANAGED_NETWORK_RECEIVE_DATA *) Context;

  gBS->SignalEvent (RxData->RecycleEvent);
}

/**
  Received a frame from MNP. Wrap it in net buffer then deliver
  it to IP's input function. The ownship of the packet also
  is transferred to IP. When Ip is finished with this packet, it
  will call NetbufFree to release the packet, NetbufFree will
  again call the Ip6RecycleFrame to signal MNP's event and free
  the token used.

  @param[in]  Context         Context for the callback.

**/
VOID
EFIAPI
Ip6OnFrameReceivedDpc (
  IN VOID                     *Context
  )
{
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *MnpToken;
  EFI_MANAGED_NETWORK_RECEIVE_DATA      *MnpRxData;
  IP6_LINK_RX_TOKEN                     *Token;
  NET_FRAGMENT                          Netfrag;
  NET_BUF                               *Packet;
  UINT32                                Flag;
  IP6_SERVICE                           *IpSb;

  Token = (IP6_LINK_RX_TOKEN *) Context;
  NET_CHECK_SIGNATURE (Token, IP6_LINK_RX_SIGNATURE);

  //
  // First clear the interface's receive request in case the
  // caller wants to call Ip6ReceiveFrame in the callback.
  //
  IpSb = (IP6_SERVICE *) Token->Context;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);


  MnpToken  = &Token->MnpToken;
  MnpRxData = MnpToken->Packet.RxData;

  if (EFI_ERROR (MnpToken->Status) || (MnpRxData == NULL)) {
    Token->CallBack (NULL, MnpToken->Status, 0, Token->Context);
    return ;
  }

  //
  // Wrap the frame in a net buffer then deliever it to IP input.
  // IP will reassemble the packet, and deliver it to upper layer
  //
  Netfrag.Len  = MnpRxData->DataLength;
  Netfrag.Bulk = MnpRxData->PacketData;

  Packet = NetbufFromExt (&Netfrag, 1, IP6_MAX_HEADLEN, 0, Ip6RecycleFrame, Token->MnpToken.Packet.RxData);

  if (Packet == NULL) {
    gBS->SignalEvent (MnpRxData->RecycleEvent);

    Token->CallBack (NULL, EFI_OUT_OF_RESOURCES, 0, Token->Context);

    return ;
  }

  Flag  = (MnpRxData->BroadcastFlag ? IP6_LINK_BROADCAST : 0);
  Flag |= (MnpRxData->MulticastFlag ? IP6_LINK_MULTICAST : 0);
  Flag |= (MnpRxData->PromiscuousFlag ? IP6_LINK_PROMISC : 0);

  Token->CallBack (Packet, EFI_SUCCESS, Flag, Token->Context);
}

/**
  Request Ip6OnFrameReceivedDpc as a DPC at TPL_CALLBACK.

  @param  Event                 The receive event delivered to MNP for receive.
  @param  Context               Context for the callback.

**/
VOID
EFIAPI
Ip6OnFrameReceived (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // Request Ip6OnFrameReceivedDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, Ip6OnFrameReceivedDpc, Context);
}

/**
  Request to receive the packet from the interface.

  @param[in]  CallBack          Function to call when receive finished.
  @param[in]  IpSb              Points to IP6 service binding instance.

  @retval EFI_ALREADY_STARTED   There is already a pending receive request.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to receive.
  @retval EFI_SUCCESS           The recieve request has been started.

**/
EFI_STATUS
Ip6ReceiveFrame (
  IN  IP6_FRAME_CALLBACK    CallBack,
  IN  IP6_SERVICE           *IpSb
  )
{
  EFI_STATUS                Status;
  IP6_LINK_RX_TOKEN         *Token;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  Token           = &IpSb->RecvRequest;
  Token->CallBack = CallBack;
  Token->Context  = (VOID *) IpSb;

  Status = IpSb->Mnp->Receive (IpSb->Mnp, &Token->MnpToken);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Callback funtion when frame transmission is finished. It will
  call the frame owner's callback function to tell it the result.

  @param[in]  Context        Context which points to the token.

**/
VOID
EFIAPI
Ip6OnFrameSentDpc (
  IN VOID                    *Context
  )
{
  IP6_LINK_TX_TOKEN         *Token;

  Token = (IP6_LINK_TX_TOKEN *) Context;
  NET_CHECK_SIGNATURE (Token, IP6_LINK_TX_SIGNATURE);

  RemoveEntryList (&Token->Link);

  Token->CallBack (
          Token->Packet,
          Token->MnpToken.Status,
          0,
          Token->Context
          );

  Ip6FreeLinkTxToken (Token);
}

/**
  Request Ip6OnFrameSentDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The transmit token's event.
  @param[in]  Context               Context which points to the token.

**/
VOID
EFIAPI
Ip6OnFrameSent (
  IN EFI_EVENT               Event,
  IN VOID                    *Context
  )
{
  //
  // Request Ip6OnFrameSentDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, Ip6OnFrameSentDpc, Context);
}

/**
  Send a frame from the interface. If the next hop is a multicast address,
  it is transmitted immediately. If the next hop is a unicast,
  and the NextHop's MAC is not known, it will perform address resolution.
  If an error occurred, the CallBack won't be called. So, the caller
  must test the return value, and take action when there is an error.

  @param[in]  Interface         The interface to send the frame from
  @param[in]  IpInstance        The IP child that request the transmission.
                                NULL if it is the IP6 driver itself.
  @param[in]  Packet            The packet to transmit.
  @param[in]  NextHop           The immediate destination to transmit the packet to.
  @param[in]  CallBack          Function to call back when transmit finished.
  @param[in]  Context           Opaque parameter to the callback.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to send the frame.
  @retval EFI_NO_MAPPING        Can't resolve the MAC for the nexthop.
  @retval EFI_SUCCESS           The packet successfully transmitted.

**/
EFI_STATUS
Ip6SendFrame (
  IN  IP6_INTERFACE         *Interface,
  IN  IP6_PROTOCOL          *IpInstance      OPTIONAL,
  IN  NET_BUF               *Packet,
  IN  EFI_IPv6_ADDRESS      *NextHop,
  IN  IP6_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context
  )
{
  IP6_SERVICE               *IpSb;
  IP6_LINK_TX_TOKEN         *Token;
  EFI_STATUS                Status;
  IP6_NEIGHBOR_ENTRY        *NeighborCache;
  LIST_ENTRY                *Entry;
  IP6_NEIGHBOR_ENTRY        *ArpQue;

  IpSb = Interface->Service;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  //
  // Only when link local address is performing DAD, the interface could be used in unconfigured.
  //
  if (IpSb->LinkLocalOk) {
    ASSERT (Interface->Configured);
  }

  Token = Ip6CreateLinkTxToken (Interface, IpInstance, Packet, CallBack, Context);

  if (Token == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IP6_IS_MULTICAST (NextHop)) {
    Status = Ip6GetMulticastMac (IpSb->Mnp, NextHop, &Token->DstMac);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    goto SendNow;
  }

  //
  // If send to itself, directly send out
  //
  if (EFI_IP6_EQUAL (&Packet->Ip.Ip6->DestinationAddress, &Packet->Ip.Ip6->SourceAddress)) {
    IP6_COPY_LINK_ADDRESS (&Token->DstMac, &IpSb->SnpMode.CurrentAddress);
    goto SendNow;
  }

  //
  // If unicast, check the neighbor state.
  //

  NeighborCache = Ip6FindNeighborEntry (IpSb, NextHop);
  ASSERT (NeighborCache != NULL);

  if (NeighborCache->Interface == NULL) {
    NeighborCache->Interface = Interface;
  }

  switch (NeighborCache->State) {
  case EfiNeighborStale:
    NeighborCache->State = EfiNeighborDelay;
    NeighborCache->Ticks = (UINT32) IP6_GET_TICKS (IP6_DELAY_FIRST_PROBE_TIME);
    //
    // Fall through
    //
  case EfiNeighborReachable:
  case EfiNeighborDelay:
  case EfiNeighborProbe:
    IP6_COPY_LINK_ADDRESS (&Token->DstMac, &NeighborCache->LinkAddress);
    goto SendNow;
    break;

  default:
    break;
  }

  //
  // Have to do asynchronous ARP resolution. First check whether there is
  // already a pending request.
  //
  NET_LIST_FOR_EACH (Entry, &Interface->ArpQues) {
    ArpQue = NET_LIST_USER_STRUCT (Entry, IP6_NEIGHBOR_ENTRY, ArpList);
    if (ArpQue == NeighborCache) {
      InsertTailList (&NeighborCache->Frames, &Token->Link);
      NeighborCache->ArpFree = TRUE;
      return EFI_SUCCESS;
    }
  }

  //
  // First frame requires ARP.
  //
  InsertTailList (&NeighborCache->Frames, &Token->Link);
  InsertTailList (&Interface->ArpQues, &NeighborCache->ArpList);

  NeighborCache->ArpFree = TRUE;

  return EFI_SUCCESS;

SendNow:
 //
  // Insert the tx token into the SentFrames list before calling Mnp->Transmit.
  // Remove it if the returned status is not EFI_SUCCESS.
  //
  InsertTailList (&Interface->SentFrames, &Token->Link);
  Status = IpSb->Mnp->Transmit (IpSb->Mnp, &Token->MnpToken);
  if (EFI_ERROR (Status)) {
    RemoveEntryList (&Token->Link);
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  Ip6FreeLinkTxToken (Token);
  return Status;
}

/**
  The heartbeat timer of IP6 service instance. It times out
  all of its IP6 children's received-but-not-delivered and
  transmitted-but-not-recycle packets.

  @param[in]  Event                 The IP6 service instance's heartbeat timer.
  @param[in]  Context               The IP6 service instance.

**/
VOID
EFIAPI
Ip6TimerTicking (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  IP6_SERVICE               *IpSb;

  IpSb = (IP6_SERVICE *) Context;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  Ip6PacketTimerTicking (IpSb);
  Ip6NdTimerTicking (IpSb);
  Ip6MldTimerTicking (IpSb);
}
