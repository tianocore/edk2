/** @file
  EFI DHCP protocol implementation.
  
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Dhcp4Impl.h"

UINT32  mDhcp4DefaultTimeout[4] = { 4, 8, 16, 32 };


/**
  Send an initial DISCOVER or REQUEST message according to the
  DHCP service's current state.

  @param[in]  DhcpSb                The DHCP service instance

  @retval EFI_SUCCESS           The request has been sent
  @retval other                 Some error occurs when sending the request.

**/
EFI_STATUS
DhcpInitRequest (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  EFI_STATUS                Status;

  ASSERT ((DhcpSb->DhcpState == Dhcp4Init) || (DhcpSb->DhcpState == Dhcp4InitReboot));

  //
  // Clear initial time to make sure that elapsed-time is set to 0 for first Discover or REQUEST message.
  //
  DhcpSb->ActiveChild->ElaspedTime= 0;
  
  if (DhcpSb->DhcpState == Dhcp4Init) {
    DhcpSetState (DhcpSb, Dhcp4Selecting, FALSE);
    Status = DhcpSendMessage (DhcpSb, NULL, NULL, DHCP_MSG_DISCOVER, NULL);

    if (EFI_ERROR (Status)) {
      DhcpSb->DhcpState = Dhcp4Init;
      return Status;
    }
  } else {
    DhcpSetState (DhcpSb, Dhcp4Rebooting, FALSE);
    Status = DhcpSendMessage (DhcpSb, NULL, NULL, DHCP_MSG_REQUEST, NULL);

    if (EFI_ERROR (Status)) {
      DhcpSb->DhcpState = Dhcp4InitReboot;
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
  Call user provided callback function, and return the value the
  function returns. If the user doesn't provide a callback, a
  proper return value is selected to let the caller continue the
  normal process.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Event                 The event as defined in the spec
  @param[in]  Packet                The current packet trigger the event
  @param[out] NewPacket             The user's return new packet

  @retval EFI_NOT_READY         Direct the caller to continue collecting the offer.
  @retval EFI_SUCCESS           The user function returns success.
  @retval EFI_ABORTED           The user function ask it to abort.

**/
EFI_STATUS
DhcpCallUser (
  IN  DHCP_SERVICE          *DhcpSb,
  IN  EFI_DHCP4_EVENT       Event,
  IN  EFI_DHCP4_PACKET      *Packet,      OPTIONAL
  OUT EFI_DHCP4_PACKET      **NewPacket   OPTIONAL
  )
{
  EFI_DHCP4_CONFIG_DATA     *Config;
  EFI_STATUS                Status;

  if (NewPacket != NULL) {
    *NewPacket = NULL;
  }

  //
  // If user doesn't provide the call back function, return the value
  // that directs the client to continue the normal process.
  // In Dhcp4Selecting EFI_SUCCESS tells the client to stop collecting
  // the offers and select a offer, EFI_NOT_READY tells the client to
  // collect more offers.
  //
  Config = &DhcpSb->ActiveConfig;

  if (Config->Dhcp4Callback == NULL) {
    if (Event == Dhcp4RcvdOffer) {
      return EFI_NOT_READY;
    }

    return EFI_SUCCESS;
  }

  Status = Config->Dhcp4Callback (
                     &DhcpSb->ActiveChild->Dhcp4Protocol,
                     Config->CallbackContext,
                     (EFI_DHCP4_STATE) DhcpSb->DhcpState,
                     Event,
                     Packet,
                     NewPacket
                     );

  //
  // User's callback should only return EFI_SUCCESS, EFI_NOT_READY,
  // and EFI_ABORTED. If it returns values other than those, assume
  // it to be EFI_ABORTED.
  //
  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_READY)) {
    return Status;
  }

  return EFI_ABORTED;
}


/**
  Notify the user about the operation result.

  @param  DhcpSb                DHCP service instance
  @param  Which                 Which notify function to signal

**/
VOID
DhcpNotifyUser (
  IN DHCP_SERVICE           *DhcpSb,
  IN INTN                   Which
  )
{
  DHCP_PROTOCOL             *Child;

  if ((Child = DhcpSb->ActiveChild) == NULL) {
    return ;
  }

  if ((Child->CompletionEvent != NULL) &&
      ((Which == DHCP_NOTIFY_COMPLETION) || (Which == DHCP_NOTIFY_ALL))
      ) {

    gBS->SignalEvent (Child->CompletionEvent);
    Child->CompletionEvent = NULL;
  }

  if ((Child->RenewRebindEvent != NULL) &&
      ((Which == DHCP_NOTIFY_RENEWREBIND) || (Which == DHCP_NOTIFY_ALL))
      ) {

    gBS->SignalEvent (Child->RenewRebindEvent);
    Child->RenewRebindEvent = NULL;
  }
}



/**
  Set the DHCP state. If CallUser is true, it will try to notify
  the user before change the state by DhcpNotifyUser. It returns
  EFI_ABORTED if the user return EFI_ABORTED, otherwise, it returns
  EFI_SUCCESS. If CallUser is FALSE, it isn't necessary to test
  the return value of this function.

  @param  DhcpSb                The DHCP service instance
  @param  State                 The new DHCP state to change to
  @param  CallUser              Whether we need to call user

  @retval EFI_SUCCESS           The state is changed
  @retval EFI_ABORTED           The user asks to abort the DHCP process.

**/
EFI_STATUS
DhcpSetState (
  IN OUT DHCP_SERVICE           *DhcpSb,
  IN     INTN                   State,
  IN     BOOLEAN                CallUser
  )
{
  EFI_STATUS                Status;

  if (CallUser) {
    Status = EFI_SUCCESS;

    if (State == Dhcp4Renewing) {
      Status = DhcpCallUser (DhcpSb, Dhcp4EnterRenewing, NULL, NULL);

    } else if (State == Dhcp4Rebinding) {
      Status = DhcpCallUser (DhcpSb, Dhcp4EnterRebinding, NULL, NULL);

    } else if (State == Dhcp4Bound) {
      Status = DhcpCallUser (DhcpSb, Dhcp4BoundCompleted, NULL, NULL);

    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Update the retransmission timer during the state transition.
  // This will clear the retry count. This is also why the rule
  // first transit the state, then send packets.
  //
  if (State == Dhcp4Selecting) {
    DhcpSb->MaxRetries = DhcpSb->ActiveConfig.DiscoverTryCount;
  } else {
    DhcpSb->MaxRetries = DhcpSb->ActiveConfig.RequestTryCount;
  }

  if (DhcpSb->MaxRetries == 0) {
    DhcpSb->MaxRetries = 4;
  }

  DhcpSb->CurRetry      = 0;
  DhcpSb->PacketToLive  = 0;
  DhcpSb->LastTimeout   = 0;
  DhcpSb->DhcpState     = State;
  return EFI_SUCCESS;
}


/**
  Set the retransmit timer for the packet. It will select from either
  the discover timeouts/request timeouts or the default timeout values.

  @param  DhcpSb                The DHCP service instance.

**/
VOID
DhcpSetTransmitTimer (
  IN OUT DHCP_SERVICE           *DhcpSb
  )
{
  UINT32                    *Times;

  ASSERT (DhcpSb->MaxRetries > DhcpSb->CurRetry);

  if (DhcpSb->DhcpState == Dhcp4Selecting) {
    Times = DhcpSb->ActiveConfig.DiscoverTimeout;
  } else {
    Times = DhcpSb->ActiveConfig.RequestTimeout;
  }

  if (Times == NULL) {
    Times = mDhcp4DefaultTimeout;
  }

  DhcpSb->PacketToLive = Times[DhcpSb->CurRetry];
  DhcpSb->LastTimeout  = DhcpSb->PacketToLive;

  return;
}

/**
  Compute the lease. If the server grants a permanent lease, just
  process it as a normal timeout value since the lease will last
  more than 100 years.

  @param  DhcpSb                The DHCP service instance
  @param  Para                  The DHCP parameter extracted from the server's
                                response.
**/
VOID
DhcpComputeLease (
  IN OUT DHCP_SERVICE           *DhcpSb,
  IN     DHCP_PARAMETER         *Para
  )
{
  ASSERT (Para != NULL);

  DhcpSb->Lease = Para->Lease;
  DhcpSb->T2    = Para->T2;
  DhcpSb->T1    = Para->T1;

  if (DhcpSb->Lease == 0) {
    DhcpSb->Lease = DHCP_DEFAULT_LEASE;
  }

  if ((DhcpSb->T2 == 0) || (DhcpSb->T2 >= Para->Lease)) {
    DhcpSb->T2 = Para->Lease - (Para->Lease >> 3);
  }

  if ((DhcpSb->T1 == 0) || (DhcpSb->T1 >= Para->T2)) {
    DhcpSb->T1 = DhcpSb->Lease >> 1;
  }
}


/**
  Configure a UDP IO port to use the acquired lease address.
  DHCP driver needs this port to unicast packet to the server
  such as DHCP release.

  @param[in]  UdpIo                 The UDP IO to configure
  @param[in]  Context               Dhcp service instance.

  @retval EFI_SUCCESS           The UDP IO port is successfully configured.
  @retval Others                It failed to configure the port.

**/
EFI_STATUS
EFIAPI
DhcpConfigLeaseIoPort (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  )
{
  EFI_UDP4_CONFIG_DATA      UdpConfigData;
  EFI_IPv4_ADDRESS          Subnet;
  EFI_IPv4_ADDRESS          Gateway;
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;
  IP4_ADDR                  Ip;

  DhcpSb = (DHCP_SERVICE *) Context;

  UdpConfigData.AcceptBroadcast     = FALSE;
  UdpConfigData.AcceptPromiscuous   = FALSE;
  UdpConfigData.AcceptAnyPort       = FALSE;
  UdpConfigData.AllowDuplicatePort  = TRUE;
  UdpConfigData.TypeOfService       = 0;
  UdpConfigData.TimeToLive          = 64;
  UdpConfigData.DoNotFragment       = FALSE;
  UdpConfigData.ReceiveTimeout      = 1;
  UdpConfigData.TransmitTimeout     = 0;

  UdpConfigData.UseDefaultAddress   = FALSE;
  UdpConfigData.StationPort         = DHCP_CLIENT_PORT;
  UdpConfigData.RemotePort          = DHCP_SERVER_PORT;

  Ip = HTONL (DhcpSb->ClientAddr);
  CopyMem (&UdpConfigData.StationAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Ip = HTONL (DhcpSb->Netmask);
  CopyMem (&UdpConfigData.SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

  ZeroMem (&UdpConfigData.RemoteAddress, sizeof (EFI_IPv4_ADDRESS));

  Status = UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, &UdpConfigData);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add a default route if received from the server.
  //
  if ((DhcpSb->Para != NULL) && (DhcpSb->Para->Router != 0)) {
    ZeroMem (&Subnet, sizeof (EFI_IPv4_ADDRESS));

    Ip = HTONL (DhcpSb->Para->Router);
    CopyMem (&Gateway, &Ip, sizeof (EFI_IPv4_ADDRESS));

    UdpIo->Protocol.Udp4->Routes (UdpIo->Protocol.Udp4, FALSE, &Subnet, &Subnet, &Gateway);
  }

  return EFI_SUCCESS;
}


/**
  Update the lease states when a new lease is acquired. It will not only
  save the acquired the address and lease time, it will also create a UDP
  child to provide address resolution for the address.

  @param  DhcpSb                The DHCP service instance

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval EFI_SUCCESS           The lease is recorded.

**/
EFI_STATUS
DhcpLeaseAcquired (
  IN OUT DHCP_SERVICE           *DhcpSb
  )
{
  INTN                      Class;

  DhcpSb->ClientAddr = EFI_NTOHL (DhcpSb->Selected->Dhcp4.Header.YourAddr);

  if (DhcpSb->Para != NULL) {
    DhcpSb->Netmask     = DhcpSb->Para->NetMask;
    DhcpSb->ServerAddr  = DhcpSb->Para->ServerId;
  }

  if (DhcpSb->Netmask == 0) {
    Class           = NetGetIpClass (DhcpSb->ClientAddr);
    DhcpSb->Netmask = gIp4AllMasks[Class << 3];
  }

  if (DhcpSb->LeaseIoPort != NULL) {
    UdpIoFreeIo (DhcpSb->LeaseIoPort);
  }

  //
  // Create a UDP/IP child to provide ARP service for the Leased IP,
  // and transmit unicast packet with it as source address. Don't
  // start receive on this port, the queued packet will be timeout.
  //
  DhcpSb->LeaseIoPort = UdpIoCreateIo (
                          DhcpSb->Controller,
                          DhcpSb->Image,
                          DhcpConfigLeaseIoPort,
                          UDP_IO_UDP4_VERSION,
                          DhcpSb
                          );

  if (DhcpSb->LeaseIoPort == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!DHCP_IS_BOOTP (DhcpSb->Para)) {
    DhcpComputeLease (DhcpSb, DhcpSb->Para);
  }

  return DhcpSetState (DhcpSb, Dhcp4Bound, TRUE);
}


/**
  Clean up the DHCP related states, IoStatus isn't reset.

  @param  DhcpSb                The DHCP instance service.

**/
VOID
DhcpCleanLease (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  DhcpSb->DhcpState   = Dhcp4Init;
  DhcpSb->Xid         = DhcpSb->Xid + 1;
  DhcpSb->ClientAddr  = 0;
  DhcpSb->Netmask     = 0;
  DhcpSb->ServerAddr  = 0;

  if (DhcpSb->LastOffer != NULL) {
    FreePool (DhcpSb->LastOffer);
    DhcpSb->LastOffer = NULL;
  }

  if (DhcpSb->Selected != NULL) {
    FreePool (DhcpSb->Selected);
    DhcpSb->Selected = NULL;
  }

  if (DhcpSb->Para != NULL) {
    FreePool (DhcpSb->Para);
    DhcpSb->Para = NULL;
  }

  DhcpSb->Lease         = 0;
  DhcpSb->T1            = 0;
  DhcpSb->T2            = 0;
  DhcpSb->ExtraRefresh  = FALSE;

  if (DhcpSb->LeaseIoPort != NULL) {
    UdpIoFreeIo (DhcpSb->LeaseIoPort);
    DhcpSb->LeaseIoPort = NULL;
  }

  if (DhcpSb->LastPacket != NULL) {
    FreePool (DhcpSb->LastPacket);
    DhcpSb->LastPacket = NULL;
  }

  DhcpSb->PacketToLive  = 0;
  DhcpSb->LastTimeout   = 0;
  DhcpSb->CurRetry      = 0;
  DhcpSb->MaxRetries    = 0;
  DhcpSb->LeaseLife     = 0;

  //
  // Clean active config data.
  //
  DhcpCleanConfigure (&DhcpSb->ActiveConfig);
}


/**
  Select a offer among all the offers collected. If the offer selected is
  of BOOTP, the lease is recorded and user notified. If the offer is of
  DHCP, it will request the offer from the server.

  @param[in]  DhcpSb                The DHCP service instance.

  @retval EFI_SUCCESS           One of the offer is selected.

**/
EFI_STATUS
DhcpChooseOffer (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  EFI_DHCP4_PACKET          *Selected;
  EFI_DHCP4_PACKET          *NewPacket;
  EFI_DHCP4_PACKET          *TempPacket;
  EFI_STATUS                Status;

  ASSERT (DhcpSb->LastOffer != NULL);

  //
  // User will cache previous offers if he wants to select
  // from multiple offers. If user provides an invalid packet,
  // use the last offer, otherwise use the provided packet.
  //
  NewPacket = NULL;
  Status    = DhcpCallUser (DhcpSb, Dhcp4SelectOffer, DhcpSb->LastOffer, &NewPacket);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Selected = DhcpSb->LastOffer;

  if ((NewPacket != NULL) && !EFI_ERROR (DhcpValidateOptions (NewPacket, NULL))) {
    TempPacket = (EFI_DHCP4_PACKET *) AllocatePool (NewPacket->Size);
    if (TempPacket != NULL) {
      CopyMem (TempPacket, NewPacket, NewPacket->Size);
      FreePool (Selected);
      Selected = TempPacket;
    }
  }

  DhcpSb->Selected  = Selected;
  DhcpSb->LastOffer = NULL;
  DhcpSb->Para      = NULL;
  DhcpValidateOptions (Selected, &DhcpSb->Para);

  //
  // A bootp offer has been selected, save the lease status,
  // enter bound state then notify the user.
  //
  if (DHCP_IS_BOOTP (DhcpSb->Para)) {
    Status = DhcpLeaseAcquired (DhcpSb);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    DhcpSb->IoStatus = EFI_SUCCESS;
    DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_ALL);
    return EFI_SUCCESS;
  }

  //
  // Send a DHCP requests
  //
  Status = DhcpSetState (DhcpSb, Dhcp4Requesting, TRUE);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DhcpSendMessage (DhcpSb, Selected, DhcpSb->Para, DHCP_MSG_REQUEST, NULL);
}


/**
  Terminate the current address acquire. All the allocated resources
  are released. Be careful when calling this function. A rule related
  to this is: only call DhcpEndSession at the highest level, such as
  DhcpInput, DhcpOnTimerTick...At the other level, just return error.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Status                The result of the DHCP process.

**/
VOID
DhcpEndSession (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_STATUS             Status
  )
{
  if (DHCP_CONNECTED (DhcpSb->DhcpState)) {
    DhcpCallUser (DhcpSb, Dhcp4AddressLost, NULL, NULL);
  } else {
    DhcpCallUser (DhcpSb, Dhcp4Fail, NULL, NULL);
  }

  DhcpCleanLease (DhcpSb);

  DhcpSb->IoStatus = Status;
  DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_ALL);
}


/**
  Handle packets in DHCP select state.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Packet                The DHCP packet received
  @param[in]  Para                  The DHCP parameter extracted from the packet. That
                                    is, all the option value that we care.

  @retval EFI_SUCCESS           The packet is successfully processed.
  @retval Others                Some error occured.

**/
EFI_STATUS
DhcpHandleSelect (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Packet,
  IN DHCP_PARAMETER         *Para
  )
{
  EFI_STATUS                Status;

  Status = EFI_SUCCESS;

  //
  // First validate the message:
  // 1. the offer is a unicast
  // 2. if it is a DHCP message, it must contains a server ID.
  // Don't return a error for these two case otherwise the session is ended.
  //
  if (!DHCP_IS_BOOTP (Para) &&
      ((Para->DhcpType != DHCP_MSG_OFFER) || (Para->ServerId == 0))
      ) {
    goto ON_EXIT;
  }

  //
  // Call the user's callback. The action according to the return is as:
  // 1. EFI_SUCESS: stop waiting for more offers, select the offer now
  // 2. EFI_NOT_READY: wait for more offers
  // 3. EFI_ABORTED: abort the address acquiring.
  //
  Status = DhcpCallUser (DhcpSb, Dhcp4RcvdOffer, Packet, NULL);

  if (Status == EFI_SUCCESS) {
    if (DhcpSb->LastOffer != NULL) {
      FreePool (DhcpSb->LastOffer);
    }

    DhcpSb->LastOffer = Packet;

    return DhcpChooseOffer (DhcpSb);

  } else if (Status == EFI_NOT_READY) {
    if (DhcpSb->LastOffer != NULL) {
      FreePool (DhcpSb->LastOffer);
    }

    DhcpSb->LastOffer = Packet;

  } else if (Status == EFI_ABORTED) {
    //
    // DhcpInput will end the session upon error return. Remember
    // only to call DhcpEndSession at the top level call.
    //
    goto ON_EXIT;
  }

  return EFI_SUCCESS;

ON_EXIT:
  FreePool (Packet);
  return Status;
}


/**
  Handle packets in DHCP request state.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Packet                The DHCP packet received
  @param[in]  Para                  The DHCP parameter extracted from the packet. That
                                    is, all the option value that we care.

  @retval EFI_SUCCESS           The packet is successfully processed.
  @retval Others                Some error occured.

**/
EFI_STATUS
DhcpHandleRequest (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Packet,
  IN DHCP_PARAMETER         *Para
  )
{
  EFI_DHCP4_HEADER          *Head;
  EFI_DHCP4_HEADER          *Selected;
  EFI_STATUS                Status;
  UINT8                     *Message;

  ASSERT (!DHCP_IS_BOOTP (DhcpSb->Para));

  Head      = &Packet->Dhcp4.Header;
  Selected  = &DhcpSb->Selected->Dhcp4.Header;

  //
  // Ignore the BOOTP message and DHCP messages other than DHCP ACK/NACK.
  //
  if (DHCP_IS_BOOTP (Para) ||
      (Para->ServerId != DhcpSb->Para->ServerId) ||
      ((Para->DhcpType != DHCP_MSG_ACK) && (Para->DhcpType != DHCP_MSG_NAK))
      ) {

    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Received a NAK, end the session no matter what the user returns
  //
  Status = EFI_DEVICE_ERROR;

  if (Para->DhcpType == DHCP_MSG_NAK) {
    DhcpCallUser (DhcpSb, Dhcp4RcvdNak, Packet, NULL);
    goto ON_EXIT;
  }

  //
  // Check whether the ACK matches the selected offer
  //
  Message = NULL;

  if (!EFI_IP4_EQUAL (&Head->YourAddr, &Selected->YourAddr)) {
    Message = (UINT8 *) "Lease confirmed isn't the same as that in the offer";
    goto REJECT;
  }

  Status = DhcpCallUser (DhcpSb, Dhcp4RcvdAck, Packet, NULL);

  if (EFI_ERROR (Status)) {
    Message = (UINT8 *) "Lease is denied upon received ACK";
    goto REJECT;
  }

  //
  // Record the lease, transit to BOUND state, then notify the user
  //
  Status = DhcpLeaseAcquired (DhcpSb);

  if (EFI_ERROR (Status)) {
    Message = (UINT8 *) "Lease is denied upon entering bound";
    goto REJECT;
  }

  DhcpSb->IoStatus = EFI_SUCCESS;
  DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_COMPLETION);

  FreePool (Packet);
  return EFI_SUCCESS;

REJECT:
  DhcpSendMessage (DhcpSb, DhcpSb->Selected, DhcpSb->Para, DHCP_MSG_DECLINE, Message);

ON_EXIT:
  FreePool (Packet);
  return Status;
}


/**
  Handle packets in DHCP renew/rebound state.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Packet                The DHCP packet received
  @param[in]  Para                  The DHCP parameter extracted from the packet. That
                                    is, all the option value that we care.

  @retval EFI_SUCCESS           The packet is successfully processed.
  @retval Others                Some error occured.

**/
EFI_STATUS
DhcpHandleRenewRebind (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Packet,
  IN DHCP_PARAMETER         *Para
  )
{
  EFI_DHCP4_HEADER          *Head;
  EFI_DHCP4_HEADER          *Selected;
  EFI_STATUS                Status;

  ASSERT (!DHCP_IS_BOOTP (DhcpSb->Para));

  Head      = &Packet->Dhcp4.Header;
  Selected  = &DhcpSb->Selected->Dhcp4.Header;

  //
  // Ignore the BOOTP message and DHCP messages other than DHCP ACK/NACK
  //
  if (DHCP_IS_BOOTP (Para) ||
      (Para->ServerId != DhcpSb->Para->ServerId) ||
      ((Para->DhcpType != DHCP_MSG_ACK) && (Para->DhcpType != DHCP_MSG_NAK))
      ) {

    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Received a NAK, ignore the user's return then terminate the process
  //
  Status = EFI_DEVICE_ERROR;

  if (Para->DhcpType == DHCP_MSG_NAK) {
    DhcpCallUser (DhcpSb, Dhcp4RcvdNak, Packet, NULL);
    goto ON_EXIT;
  }

  //
  // The lease is different from the selected. Don't send a DECLINE
  // since it isn't existed in the client's FSM.
  //
  if (!EFI_IP4_EQUAL (&Head->YourAddr, &Selected->YourAddr)) {
    goto ON_EXIT;
  }

  Status = DhcpCallUser (DhcpSb, Dhcp4RcvdAck, Packet, NULL);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Record the lease, start timer for T1 and T2,
  //
  DhcpComputeLease (DhcpSb, Para);
  DhcpSb->LeaseLife = 0;
  DhcpSetState (DhcpSb, Dhcp4Bound, TRUE);

  if (DhcpSb->ExtraRefresh != 0) {
    DhcpSb->ExtraRefresh  = FALSE;

    DhcpSb->IoStatus      = EFI_SUCCESS;
    DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_RENEWREBIND);
  }

ON_EXIT:
  FreePool (Packet);
  return Status;
}


/**
  Handle packets in DHCP reboot state.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Packet                The DHCP packet received
  @param[in]  Para                  The DHCP parameter extracted from the packet. That
                                    is, all the option value that we care.

  @retval EFI_SUCCESS           The packet is successfully processed.
  @retval Others                Some error occured.

**/
EFI_STATUS
DhcpHandleReboot (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Packet,
  IN DHCP_PARAMETER         *Para
  )
{
  EFI_DHCP4_HEADER          *Head;
  EFI_STATUS                Status;

  Head = &Packet->Dhcp4.Header;

  //
  // Ignore the BOOTP message and DHCP messages other than DHCP ACK/NACK
  //
  if (DHCP_IS_BOOTP (Para) ||
      ((Para->DhcpType != DHCP_MSG_ACK) && (Para->DhcpType != DHCP_MSG_NAK))
      ) {

    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // If a NAK is received, transit to INIT and try again.
  //
  if (Para->DhcpType == DHCP_MSG_NAK) {
    DhcpCallUser (DhcpSb, Dhcp4RcvdNak, Packet, NULL);

    DhcpSb->ClientAddr  = 0;
    DhcpSb->DhcpState   = Dhcp4Init;

    Status              = DhcpInitRequest (DhcpSb);
    goto ON_EXIT;
  }

  //
  // Check whether the ACK matches the selected offer
  //
  if (EFI_NTOHL (Head->YourAddr) != DhcpSb->ClientAddr) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = DhcpCallUser (DhcpSb, Dhcp4RcvdAck, Packet, NULL);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // OK, get the parameter from server, record the lease
  //
  DhcpSb->Para = AllocateCopyPool (sizeof (DHCP_PARAMETER), Para);
  if (DhcpSb->Para == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  DhcpSb->Selected  = Packet;
  Status            = DhcpLeaseAcquired (DhcpSb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DhcpSb->IoStatus = EFI_SUCCESS;
  DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_COMPLETION);
  return EFI_SUCCESS;

ON_EXIT:
  FreePool (Packet);
  return Status;
}


/**
  Handle the received DHCP packets. This function drives the DHCP
  state machine.

  @param  UdpPacket             The UDP packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DhcpInput (
  NET_BUF                   *UdpPacket,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  )
{
  DHCP_SERVICE              *DhcpSb;
  EFI_DHCP4_HEADER          *Head;
  EFI_DHCP4_PACKET          *Packet;
  DHCP_PARAMETER            *Para;
  EFI_STATUS                Status;
  UINT32                    Len;

  Packet  = NULL;
  DhcpSb  = (DHCP_SERVICE *) Context;

  //
  // Don't restart receive if error occurs or DHCP is destoried.
  //
  if (EFI_ERROR (IoStatus)) {
    return ;
  } else if (DhcpSb->ServiceState == DHCP_DESTORY) {
    NetbufFree (UdpPacket);
    return ;
  }

  ASSERT (UdpPacket != NULL);

  if (DhcpSb->DhcpState == Dhcp4Stopped) {
    goto RESTART;
  }

  //
  // Validate the packet received
  //
  if (UdpPacket->TotalSize < sizeof (EFI_DHCP4_HEADER)) {
    goto RESTART;
  }

  //
  // Copy the DHCP message to a continuous memory block
  //
  Len     = sizeof (EFI_DHCP4_PACKET) + UdpPacket->TotalSize - sizeof (EFI_DHCP4_HEADER);
  Packet  = (EFI_DHCP4_PACKET *) AllocatePool (Len);

  if (Packet == NULL) {
    goto RESTART;
  }

  Packet->Size    = Len;
  Head            = &Packet->Dhcp4.Header;
  Packet->Length  = NetbufCopy (UdpPacket, 0, UdpPacket->TotalSize, (UINT8 *) Head);

  if (Packet->Length != UdpPacket->TotalSize) {
    goto RESTART;
  }

  //
  // Is this packet the answer to our packet?
  //
  if ((Head->OpCode != BOOTP_REPLY) ||
      (NTOHL (Head->Xid) != DhcpSb->Xid) ||
      (CompareMem (DhcpSb->ClientAddressSendOut, Head->ClientHwAddr, Head->HwAddrLen) != 0)) {
    goto RESTART;
  }

  //
  // Validate the options and retrieve the interested options
  //
  Para = NULL;
  if ((Packet->Length > sizeof (EFI_DHCP4_HEADER) + sizeof (UINT32)) &&
      (Packet->Dhcp4.Magik == DHCP_OPTION_MAGIC) &&
      EFI_ERROR (DhcpValidateOptions (Packet, &Para))) {

    goto RESTART;
  }

  //
  // Call the handler for each state. The handler should return
  // EFI_SUCCESS if the process can go on no matter whether the
  // packet is ignored or not. If the return is EFI_ERROR, the
  // session will be terminated. Packet's ownership is handled
  // over to the handlers. If operation succeeds, the handler
  // must notify the user. It isn't necessary to do if EFI_ERROR
  // is returned because the DhcpEndSession will notify the user.
  //
  Status = EFI_SUCCESS;

  switch (DhcpSb->DhcpState) {
  case Dhcp4Selecting:
    Status = DhcpHandleSelect (DhcpSb, Packet, Para);
    break;

  case Dhcp4Requesting:
    Status = DhcpHandleRequest (DhcpSb, Packet, Para);
    break;

  case Dhcp4InitReboot:
  case Dhcp4Init:
  case Dhcp4Bound:
    //
    // Ignore the packet in INITREBOOT, INIT and BOUND states
    //
    FreePool (Packet);
    Status = EFI_SUCCESS;
    break;

  case Dhcp4Renewing:
  case Dhcp4Rebinding:
    Status = DhcpHandleRenewRebind (DhcpSb, Packet, Para);
    break;

  case Dhcp4Rebooting:
    Status = DhcpHandleReboot (DhcpSb, Packet, Para);
    break;
  }

  if (Para != NULL) {
    FreePool (Para);
  }

  Packet = NULL;

  if (EFI_ERROR (Status)) {
    NetbufFree (UdpPacket);
    UdpIoRecvDatagram (DhcpSb->UdpIo, DhcpInput, DhcpSb, 0);
    DhcpEndSession (DhcpSb, Status);
    return ;
  }

RESTART:
  NetbufFree (UdpPacket);

  if (Packet != NULL) {
    FreePool (Packet);
  }

  Status = UdpIoRecvDatagram (DhcpSb->UdpIo, DhcpInput, DhcpSb, 0);

  if (EFI_ERROR (Status)) {
    DhcpEndSession (DhcpSb, EFI_DEVICE_ERROR);
  }
}


/**
  Release the packet.

  @param[in]  Arg                   The packet to release

**/
VOID
EFIAPI
DhcpReleasePacket (
  IN VOID                   *Arg
  )
{
  FreePool (Arg);
}


/**
  Release the net buffer when packet is sent.

  @param  UdpPacket             The UDP packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DhcpOnPacketSent (
  NET_BUF                   *Packet,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  )
{
  NetbufFree (Packet);
}



/**
  Build and transmit a DHCP message according to the current states.
  This function implement the Table 5. of RFC 2131. Always transits
  the state (as defined in Figure 5. of the same RFC) before sending
  a DHCP message. The table is adjusted accordingly.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Seed                  The seed packet which the new packet is based on
  @param[in]  Para                  The DHCP parameter of the Seed packet
  @param[in]  Type                  The message type to send
  @param[in]  Msg                   The human readable message to include in the packet
                                    sent.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources for the packet
  @retval EFI_ACCESS_DENIED     Failed to transmit the packet through UDP
  @retval EFI_SUCCESS           The message is sent
  @retval other                 Other error occurs

**/
EFI_STATUS
DhcpSendMessage (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Seed,
  IN DHCP_PARAMETER         *Para,
  IN UINT8                  Type,
  IN UINT8                  *Msg
  )
{
  EFI_DHCP4_CONFIG_DATA     *Config;
  EFI_DHCP4_PACKET          *Packet;
  EFI_DHCP4_PACKET          *NewPacket;
  EFI_DHCP4_HEADER          *Head;
  EFI_DHCP4_HEADER          *SeedHead;
  UDP_IO                    *UdpIo;
  UDP_END_POINT             EndPoint;
  NET_BUF                   *Wrap;
  NET_FRAGMENT              Frag;
  EFI_STATUS                Status;
  IP4_ADDR                  IpAddr;
  UINT8                     *Buf;
  UINT16                    MaxMsg;
  UINT32                    Len;
  UINT32                    Index;

  //
  // Allocate a big enough memory block to hold the DHCP packet
  //
  Len = sizeof (EFI_DHCP4_PACKET) + 128 + DhcpSb->UserOptionLen;

  if (Msg != NULL) {
    Len += (UINT32)AsciiStrLen ((CHAR8 *) Msg);
  }

  Packet = AllocatePool (Len);

  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Packet->Size    = Len;
  Packet->Length  = sizeof (EFI_DHCP4_HEADER) + sizeof (UINT32);

  //
  // Fill in the DHCP header fields
  //
  Config    = &DhcpSb->ActiveConfig;
  SeedHead  = NULL;

  if (Seed != NULL) {
    SeedHead = &Seed->Dhcp4.Header;
  }

  Head = &Packet->Dhcp4.Header;
  ZeroMem (Head, sizeof (EFI_DHCP4_HEADER));

  Head->OpCode       = BOOTP_REQUEST;
  Head->HwType       = DhcpSb->HwType;
  Head->HwAddrLen    = DhcpSb->HwLen;
  Head->Xid          = HTONL (DhcpSb->Xid);
  Head->Reserved     = HTONS (0x8000);  //Server, broadcast the message please.

  EFI_IP4 (Head->ClientAddr) = HTONL (DhcpSb->ClientAddr);
  CopyMem (Head->ClientHwAddr, DhcpSb->Mac.Addr, DhcpSb->HwLen);

  if ((Type == DHCP_MSG_DECLINE) || (Type == DHCP_MSG_RELEASE)) {
    Head->Seconds = 0;
  } else if ((Type == DHCP_MSG_REQUEST) && (DhcpSb->DhcpState == Dhcp4Requesting)) {
    //
    // Use the same value as the original DHCPDISCOVER message.
    //
    Head->Seconds = DhcpSb->LastPacket->Dhcp4.Header.Seconds;
  } else {
    SetElapsedTime(&Head->Seconds, DhcpSb->ActiveChild);
  }

  //
  // Append the DHCP message type
  //
  Packet->Dhcp4.Magik = DHCP_OPTION_MAGIC;
  Buf                 = Packet->Dhcp4.Option;
  Buf                 = DhcpAppendOption (Buf, DHCP_TAG_TYPE, 1, &Type);

  //
  // Append the serverid option if necessary:
  //   1. DHCP decline message
  //   2. DHCP release message
  //   3. DHCP request to confirm one lease.
  //
  if ((Type == DHCP_MSG_DECLINE) || (Type == DHCP_MSG_RELEASE) ||
      ((Type == DHCP_MSG_REQUEST) && (DhcpSb->DhcpState == Dhcp4Requesting))
      ) {

    ASSERT ((Para != NULL) && (Para->ServerId != 0));

    IpAddr  = HTONL (Para->ServerId);
    Buf     = DhcpAppendOption (Buf, DHCP_TAG_SERVER_ID, 4, (UINT8 *) &IpAddr);
  }

  //
  // Append the requested IP option if necessary:
  //   1. DHCP request to use the previously allocated address
  //   2. DHCP request to confirm one lease
  //   3. DHCP decline to decline one lease
  //
  IpAddr = 0;

  if (Type == DHCP_MSG_REQUEST) {
    if (DhcpSb->DhcpState == Dhcp4Rebooting) {
      IpAddr = EFI_IP4 (Config->ClientAddress);

    } else if (DhcpSb->DhcpState == Dhcp4Requesting) {
      ASSERT (SeedHead != NULL);
      IpAddr = EFI_IP4 (SeedHead->YourAddr);
    }

  } else if (Type == DHCP_MSG_DECLINE) {
    ASSERT (SeedHead != NULL);
    IpAddr = EFI_IP4 (SeedHead->YourAddr);
  }

  if (IpAddr != 0) {
    Buf = DhcpAppendOption (Buf, DHCP_TAG_REQUEST_IP, 4, (UINT8 *) &IpAddr);
  }

  //
  // Append the Max Message Length option if it isn't a DECLINE
  // or RELEASE to direct the server use large messages instead of
  // override the BOOTFILE and SERVER fields in the message head.
  //
  if ((Type != DHCP_MSG_DECLINE) && (Type != DHCP_MSG_RELEASE)) {
    MaxMsg  = HTONS (0xFF00);
    Buf     = DhcpAppendOption (Buf, DHCP_TAG_MAXMSG, 2, (UINT8 *) &MaxMsg);
  }

  //
  // Append the user's message if it isn't NULL
  //
  if (Msg != NULL) {
    Len     = MIN ((UINT32) AsciiStrLen ((CHAR8 *) Msg), 255);
    Buf     = DhcpAppendOption (Buf, DHCP_TAG_MESSAGE, (UINT16) Len, Msg);
  }

  //
  // Append the user configured options
  //
  if (DhcpSb->UserOptionLen != 0) {
    for (Index = 0; Index < Config->OptionCount; Index++) {
      //
      // We can't use any option other than the client ID from user
      // if it is a DHCP decline or DHCP release .
      //
      if (((Type == DHCP_MSG_DECLINE) || (Type == DHCP_MSG_RELEASE)) &&
          (Config->OptionList[Index]->OpCode != DHCP_TAG_CLIENT_ID)) {
        continue;
      }

      Buf = DhcpAppendOption (
              Buf,
              Config->OptionList[Index]->OpCode,
              Config->OptionList[Index]->Length,
              Config->OptionList[Index]->Data
              );
    }
  }

  *(Buf++) = DHCP_TAG_EOP;
  Packet->Length += (UINT32) (Buf - Packet->Dhcp4.Option);

  //
  // OK, the message is built, call the user to override it.
  //
  Status    = EFI_SUCCESS;
  NewPacket = NULL;

  if (Type == DHCP_MSG_DISCOVER) {
    Status = DhcpCallUser (DhcpSb, Dhcp4SendDiscover, Packet, &NewPacket);

  } else if (Type == DHCP_MSG_REQUEST) {
    Status = DhcpCallUser (DhcpSb, Dhcp4SendRequest, Packet, &NewPacket);

  } else if (Type == DHCP_MSG_DECLINE) {
    Status = DhcpCallUser (DhcpSb, Dhcp4SendDecline, Packet, &NewPacket);
  }

  if (EFI_ERROR (Status)) {
    FreePool (Packet);
    return Status;
  }

  if (NewPacket != NULL) {
    FreePool (Packet);
    Packet = NewPacket;
  }

  //
  // Save the Client Address will be sent out
  //
  CopyMem (
    &DhcpSb->ClientAddressSendOut[0],
    &Packet->Dhcp4.Header.ClientHwAddr[0],
    Packet->Dhcp4.Header.HwAddrLen
    );


  //
  // Wrap it into a netbuf then send it.
  //
  Frag.Bulk = (UINT8 *) &Packet->Dhcp4.Header;
  Frag.Len  = Packet->Length;
  Wrap      = NetbufFromExt (&Frag, 1, 0, 0, DhcpReleasePacket, Packet);

  if (Wrap == NULL) {
    FreePool (Packet);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save it as the last sent packet for retransmission
  //
  if (DhcpSb->LastPacket != NULL) {
    FreePool (DhcpSb->LastPacket);
  }

  DhcpSb->LastPacket = Packet;
  DhcpSetTransmitTimer (DhcpSb);

  //
  // Broadcast the message, unless we know the server address.
  // Use the lease UdpIo port to send the unicast packet.
  //
  EndPoint.RemoteAddr.Addr[0] = 0xffffffff;
  EndPoint.LocalAddr.Addr[0]  = 0;
  EndPoint.RemotePort         = DHCP_SERVER_PORT;
  EndPoint.LocalPort          = DHCP_CLIENT_PORT;
  UdpIo                       = DhcpSb->UdpIo;

  if ((DhcpSb->DhcpState == Dhcp4Renewing) || (Type == DHCP_MSG_RELEASE)) {
    EndPoint.RemoteAddr.Addr[0] = DhcpSb->ServerAddr;
    EndPoint.LocalAddr.Addr[0]  = DhcpSb->ClientAddr;
    UdpIo                       = DhcpSb->LeaseIoPort;
  }

  ASSERT (UdpIo != NULL);
  NET_GET_REF (Wrap);
  
  Status = UdpIoSendDatagram (
             UdpIo, 
             Wrap, 
             &EndPoint, 
             NULL, 
             DhcpOnPacketSent, 
             DhcpSb
             );

  if (EFI_ERROR (Status)) {
    NET_PUT_REF (Wrap);
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}


/**
  Retransmit a saved packet. Only DISCOVER and REQUEST messages
  will be retransmitted.

  @param[in]  DhcpSb                The DHCP service instance

  @retval EFI_ACCESS_DENIED     Failed to transmit packet through UDP port
  @retval EFI_SUCCESS           The packet is retransmitted.

**/
EFI_STATUS
DhcpRetransmit (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  UDP_IO                    *UdpIo;
  UDP_END_POINT             EndPoint;
  NET_BUF                   *Wrap;
  NET_FRAGMENT              Frag;
  EFI_STATUS                Status;

  ASSERT (DhcpSb->LastPacket != NULL);

  //
  // For REQUEST message in Dhcp4Requesting state, do not change the secs fields.
  //
  if (DhcpSb->DhcpState != Dhcp4Requesting) {
    SetElapsedTime(&DhcpSb->LastPacket->Dhcp4.Header.Seconds, DhcpSb->ActiveChild);
  }

  //
  // Wrap it into a netbuf then send it.
  //
  Frag.Bulk = (UINT8 *) &DhcpSb->LastPacket->Dhcp4.Header;
  Frag.Len  = DhcpSb->LastPacket->Length;
  Wrap      = NetbufFromExt (&Frag, 1, 0, 0, DhcpReleasePacket, DhcpSb->LastPacket);

  if (Wrap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Broadcast the message, unless we know the server address.
  //
  EndPoint.RemotePort         = DHCP_SERVER_PORT;
  EndPoint.LocalPort          = DHCP_CLIENT_PORT;
  EndPoint.RemoteAddr.Addr[0] = 0xffffffff;
  EndPoint.LocalAddr.Addr[0]  = 0;
  UdpIo                       = DhcpSb->UdpIo;

  if (DhcpSb->DhcpState == Dhcp4Renewing) {
    EndPoint.RemoteAddr.Addr[0] = DhcpSb->ServerAddr;
    EndPoint.LocalAddr.Addr[0]  = DhcpSb->ClientAddr;
    UdpIo                       = DhcpSb->LeaseIoPort;
  }

  ASSERT (UdpIo != NULL);

  NET_GET_REF (Wrap);
  Status = UdpIoSendDatagram (
             UdpIo,
             Wrap,
             &EndPoint,
             NULL,
             DhcpOnPacketSent,
             DhcpSb
             );

  if (EFI_ERROR (Status)) {
    NET_PUT_REF (Wrap);
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}


/**
  Each DHCP service has three timer. Two of them are count down timer.
  One for the packet retransmission. The other is to collect the offers.
  The third timer increaments the lease life which is compared to T1, T2,
  and lease to determine the time to renew and rebind the lease.
  DhcpOnTimerTick will be called once every second.

  @param[in]  Event                 The timer event
  @param[in]  Context               The context, which is the DHCP service instance.

**/
VOID
EFIAPI
DhcpOnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  DHCP_SERVICE              *DhcpSb;
  DHCP_PROTOCOL             *Instance;
  EFI_STATUS                Status;

  DhcpSb   = (DHCP_SERVICE *) Context;
  Instance = DhcpSb->ActiveChild;

  //
  // 0xffff is the maximum supported value for elapsed time according to RFC.
  //
  if (Instance != NULL && Instance->ElaspedTime < 0xffff) {
    Instance->ElaspedTime++;
  }
  
  //
  // Check the retransmit timer
  //
  if ((DhcpSb->PacketToLive > 0) && (--DhcpSb->PacketToLive == 0)) {

    //
    // Select offer at each timeout if any offer received.
    //
    if (DhcpSb->DhcpState == Dhcp4Selecting && DhcpSb->LastOffer != NULL) {

      Status = DhcpChooseOffer (DhcpSb);

      if (EFI_ERROR(Status)) {
        if (DhcpSb->LastOffer != NULL) {
          FreePool (DhcpSb->LastOffer);
          DhcpSb->LastOffer = NULL;
        }
      } else {
        goto ON_EXIT;
      }
    }
    
    if (++DhcpSb->CurRetry < DhcpSb->MaxRetries) {
      //
      // Still has another try
      //
      DhcpRetransmit (DhcpSb);
      DhcpSetTransmitTimer (DhcpSb);

    } else if (DHCP_CONNECTED (DhcpSb->DhcpState)) {

      //
      // Retransmission failed, if the DHCP request is initiated by
      // user, adjust the current state according to the lease life.
      // Otherwise do nothing to wait the lease to timeout
      //
      if (DhcpSb->ExtraRefresh != 0) {
        Status = EFI_SUCCESS;

        if (DhcpSb->LeaseLife < DhcpSb->T1) {
          Status = DhcpSetState (DhcpSb, Dhcp4Bound, FALSE);

        } else if (DhcpSb->LeaseLife < DhcpSb->T2) {
          Status = DhcpSetState (DhcpSb, Dhcp4Renewing, FALSE);

        } else if (DhcpSb->LeaseLife < DhcpSb->Lease) {
          Status = DhcpSetState (DhcpSb, Dhcp4Rebinding, FALSE);

        } else {
          goto END_SESSION;

        }

        DhcpSb->IoStatus = EFI_TIMEOUT;
        DhcpNotifyUser (DhcpSb, DHCP_NOTIFY_RENEWREBIND);
      }
    } else {
      goto END_SESSION;
    }
  }
  
  //
  // If an address has been acquired, check whether need to
  // refresh or whether it has expired.
  //
  if (DHCP_CONNECTED (DhcpSb->DhcpState)) {
    DhcpSb->LeaseLife++;

    //
    // Don't timeout the lease, only count the life if user is
    // requesting extra renew/rebind. Adjust the state after that.
    //
    if (DhcpSb->ExtraRefresh != 0) {
      return ;
    }

    if (DhcpSb->LeaseLife == DhcpSb->Lease) {
      //
      // Lease expires, end the session
      //
      goto END_SESSION;

    } else if (DhcpSb->LeaseLife == DhcpSb->T2) {
      //
      // T2 expires, transit to rebinding then send a REQUEST to any server
      //
      if (EFI_ERROR (DhcpSetState (DhcpSb, Dhcp4Rebinding, TRUE))) {
        goto END_SESSION;
      }

      if (Instance != NULL) {
        Instance->ElaspedTime= 0;
      }      
      
      Status = DhcpSendMessage (
                 DhcpSb,
                 DhcpSb->Selected,
                 DhcpSb->Para,
                 DHCP_MSG_REQUEST,
                 NULL
                 );

      if (EFI_ERROR (Status)) {
        goto END_SESSION;
      }

    } else if (DhcpSb->LeaseLife == DhcpSb->T1) {
      //
      // T1 expires, transit to renewing, then send a REQUEST to the server
      //
      if (EFI_ERROR (DhcpSetState (DhcpSb, Dhcp4Renewing, TRUE))) {
        goto END_SESSION;
      }

      if (Instance != NULL) {
        Instance->ElaspedTime= 0;
      }    

      Status = DhcpSendMessage (
                 DhcpSb,
                 DhcpSb->Selected,
                 DhcpSb->Para,
                 DHCP_MSG_REQUEST,
                 NULL
                 );

      if (EFI_ERROR (Status)) {
        goto END_SESSION;
      }
    }
  }

ON_EXIT:
  if ((Instance != NULL) && (Instance->Token != NULL)) {
    Instance->Timeout--;
    if (Instance->Timeout == 0) {
      PxeDhcpDone (Instance);
    }
  }

  return ;

END_SESSION:
  DhcpEndSession (DhcpSb, EFI_TIMEOUT);

  return ;
}
