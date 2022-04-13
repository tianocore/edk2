/** @file

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Udp4Impl.h"

EFI_UDP4_PROTOCOL  mUdp4Protocol = {
  Udp4GetModeData,
  Udp4Configure,
  Udp4Groups,
  Udp4Routes,
  Udp4Transmit,
  Udp4Receive,
  Udp4Cancel,
  Udp4Poll
};

/**
  Reads the current operational settings.

  The GetModeData() function copies the current operational settings of this EFI
  UDPv4 Protocol instance into user-supplied buffers. This function is used
  optionally to retrieve the operational mode data of underlying networks or
  drivers.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[out] Udp4ConfigData    Pointer to the buffer to receive the current configuration data.
  @param[out] Ip4ModeData       Pointer to the EFI IPv4 Protocol mode data structure.
  @param[out] MnpConfigData     Pointer to the managed network configuration data structure.
  @param[out] SnpModeData       Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The mode data was read.
  @retval EFI_NOT_STARTED       When Udp4ConfigData is queried, no configuration data is
                                available because this instance has not been started.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
Udp4GetModeData (
  IN  EFI_UDP4_PROTOCOL                *This,
  OUT EFI_UDP4_CONFIG_DATA             *Udp4ConfigData OPTIONAL,
  OUT EFI_IP4_MODE_DATA                *Ip4ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  )
{
  UDP4_INSTANCE_DATA  *Instance;
  EFI_IP4_PROTOCOL    *Ip;
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured && (Udp4ConfigData != NULL)) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Udp4ConfigData != NULL) {
    //
    // Set the Udp4ConfigData.
    //
    CopyMem (Udp4ConfigData, &Instance->ConfigData, sizeof (*Udp4ConfigData));
  }

  Ip = Instance->IpInfo->Ip.Ip4;

  //
  // Get the underlying Ip4ModeData, MnpConfigData and SnpModeData.
  //
  Status = Ip->GetModeData (Ip, Ip4ModeData, MnpConfigData, SnpModeData);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Initializes, changes, or resets the operational parameters for this instance of the EFI UDPv4
  Protocol.

  The Configure() function is used to do the following:
  * Initialize and start this instance of the EFI UDPv4 Protocol.
  * Change the filtering rules and operational parameters.
  * Reset this instance of the EFI UDPv4 Protocol.
  Until these parameters are initialized, no network traffic can be sent or
  received by this instance. This instance can be also reset by calling Configure()
  with UdpConfigData set to NULL. Once reset, the receiving queue and transmitting
  queue are flushed and no traffic is allowed through this instance.
  With different parameters in UdpConfigData, Configure() can be used to bind
  this instance to specified port.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  UdpConfigData     Pointer to the buffer to receive the current configuration data.

  @retval EFI_SUCCESS           The configuration settings were set, changed, or reset successfully.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more following conditions are TRUE:
  @retval EFI_ALREADY_STARTED   The EFI UDPv4 Protocol instance is already started/configured
                                and must be stopped/reset before it can be reconfigured.
  @retval EFI_ACCESS_DENIED     UdpConfigData. AllowDuplicatePort is FALSE
                                and UdpConfigData.StationPort is already used by
                                other instance.
  @retval EFI_OUT_OF_RESOURCES  The EFI UDPv4 Protocol driver cannot allocate memory for this
                                EFI UDPv4 Protocol instance.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred and this instance
                                 was not opened.

**/
EFI_STATUS
EFIAPI
Udp4Configure (
  IN EFI_UDP4_PROTOCOL     *This,
  IN EFI_UDP4_CONFIG_DATA  *UdpConfigData OPTIONAL
  )
{
  EFI_STATUS           Status;
  UDP4_INSTANCE_DATA   *Instance;
  UDP4_SERVICE_DATA    *Udp4Service;
  EFI_TPL              OldTpl;
  IP4_ADDR             StationAddress;
  IP4_ADDR             SubnetMask;
  IP4_ADDR             RemoteAddress;
  EFI_IP4_CONFIG_DATA  Ip4ConfigData;
  IP4_ADDR             LocalAddr;
  IP4_ADDR             RemoteAddr;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured && (UdpConfigData == NULL)) {
    return EFI_SUCCESS;
  }

  Udp4Service = Instance->Udp4Service;
  Status      = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (UdpConfigData != NULL) {
    CopyMem (&StationAddress, &UdpConfigData->StationAddress, sizeof (IP4_ADDR));
    CopyMem (&SubnetMask, &UdpConfigData->SubnetMask, sizeof (IP4_ADDR));
    CopyMem (&RemoteAddress, &UdpConfigData->RemoteAddress, sizeof (IP4_ADDR));

    StationAddress = NTOHL (StationAddress);
    SubnetMask     = NTOHL (SubnetMask);
    RemoteAddress  = NTOHL (RemoteAddress);

    if (!UdpConfigData->UseDefaultAddress &&
        (!IP4_IS_VALID_NETMASK (SubnetMask) ||
         !((StationAddress == 0) || ((SubnetMask != 0) && NetIp4IsUnicast (StationAddress, SubnetMask))) ||
         IP4_IS_LOCAL_BROADCAST (RemoteAddress)))
    {
      //
      // Don't use default address, and subnet mask is invalid or StationAddress is not
      // a valid unicast IPv4 address or RemoteAddress is not a valid unicast IPv4 address
      // if it is not 0.
      //
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    if (Instance->Configured) {
      //
      // The instance is already configured, try to do the re-configuration.
      //
      if (!Udp4IsReconfigurable (&Instance->ConfigData, UdpConfigData)) {
        //
        // If the new configuration data wants to change some unreconfigurable
        // settings, return EFI_ALREADY_STARTED.
        //
        Status = EFI_ALREADY_STARTED;
        goto ON_EXIT;
      }

      //
      // Save the reconfigurable parameters.
      //
      Instance->ConfigData.TypeOfService   = UdpConfigData->TypeOfService;
      Instance->ConfigData.TimeToLive      = UdpConfigData->TimeToLive;
      Instance->ConfigData.DoNotFragment   = UdpConfigData->DoNotFragment;
      Instance->ConfigData.ReceiveTimeout  = UdpConfigData->ReceiveTimeout;
      Instance->ConfigData.TransmitTimeout = UdpConfigData->TransmitTimeout;
    } else {
      //
      // Construct the Ip configuration data from the UdpConfigData.
      //
      Udp4BuildIp4ConfigData (UdpConfigData, &Ip4ConfigData);

      //
      // Configure the Ip instance wrapped in the IpInfo.
      //
      Status = IpIoConfigIp (Instance->IpInfo, &Ip4ConfigData);
      if (EFI_ERROR (Status)) {
        if (Status == EFI_NO_MAPPING) {
          Instance->IsNoMapping = TRUE;
        }

        goto ON_EXIT;
      }

      Instance->IsNoMapping = FALSE;

      //
      // Save the configuration data.
      //
      CopyMem (&Instance->ConfigData, UdpConfigData, sizeof (Instance->ConfigData));
      IP4_COPY_ADDRESS (&Instance->ConfigData.StationAddress, &Ip4ConfigData.StationAddress);
      IP4_COPY_ADDRESS (&Instance->ConfigData.SubnetMask, &Ip4ConfigData.SubnetMask);

      //
      // Try to allocate the required port resource.
      //
      Status = Udp4Bind (&Udp4Service->ChildrenList, &Instance->ConfigData);
      if (EFI_ERROR (Status)) {
        //
        // Reset the ip instance if bind fails.
        //
        IpIoConfigIp (Instance->IpInfo, NULL);
        goto ON_EXIT;
      }

      //
      // Pre calculate the checksum for the pseudo head, ignore the UDP length first.
      //
      CopyMem (&LocalAddr, &Instance->ConfigData.StationAddress, sizeof (IP4_ADDR));
      CopyMem (&RemoteAddr, &Instance->ConfigData.RemoteAddress, sizeof (IP4_ADDR));
      Instance->HeadSum = NetPseudoHeadChecksum (
                            LocalAddr,
                            RemoteAddr,
                            EFI_IP_PROTO_UDP,
                            0
                            );

      Instance->Configured = TRUE;
    }
  } else {
    //
    // UdpConfigData is NULL, reset the instance.
    //
    Instance->Configured  = FALSE;
    Instance->IsNoMapping = FALSE;

    //
    // Reset the Ip instance wrapped in the IpInfo.
    //
    IpIoConfigIp (Instance->IpInfo, NULL);

    //
    // Cancel all the user tokens.
    //
    Instance->Udp4Proto.Cancel (&Instance->Udp4Proto, NULL);

    //
    // Remove the buffered RxData for this instance.
    //
    Udp4FlushRcvdDgram (Instance);

    ASSERT (IsListEmpty (&Instance->DeliveredDgramQue));
  }

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Joins and leaves multicast groups.

  The Groups() function is used to enable and disable the multicast group
  filtering. If the JoinFlag is FALSE and the MulticastAddress is NULL, then all
  currently joined groups are left.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  JoinFlag          Set to TRUE to join a multicast group. Set to FALSE to leave one
                                or all multicast groups.
  @param[in]  MulticastAddress  Pointer to multicast group address to join or leave.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate resources to join the group.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and MulticastAddress is NULL.
                                - JoinFlag is TRUE and *MulticastAddress is not
                                  a valid multicast address.
  @retval EFI_ALREADY_STARTED   The group address is already in the group table (when
                                JoinFlag is TRUE).
  @retval EFI_NOT_FOUND         The group address is not in the group table (when JoinFlag is
                                FALSE).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Udp4Groups (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv4_ADDRESS   *MulticastAddress OPTIONAL
  )
{
  EFI_STATUS          Status;
  UDP4_INSTANCE_DATA  *Instance;
  EFI_IP4_PROTOCOL    *Ip;
  EFI_TPL             OldTpl;
  IP4_ADDR            McastIp;

  if ((This == NULL) || (JoinFlag && (MulticastAddress == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  McastIp = 0;
  if (JoinFlag) {
    CopyMem (&McastIp, MulticastAddress, sizeof (IP4_ADDR));

    if (!IP4_IS_MULTICAST (NTOHL (McastIp))) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (Instance->IsNoMapping) {
    return EFI_NO_MAPPING;
  }

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  Ip = Instance->IpInfo->Ip.Ip4;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Invoke the Ip instance the Udp4 instance consumes to do the group operation.
  //
  Status = Ip->Groups (Ip, JoinFlag, MulticastAddress);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Keep a local copy of the configured multicast IPs because IpIo receives
  // datagrams from the 0 station address IP instance and then UDP delivers to
  // the matched instance. This copy of multicast IPs is used to avoid receive
  // the multicast datagrams destined to multicast IPs the other instances configured.
  //
  if (JoinFlag) {
    NetMapInsertTail (&Instance->McastIps, (VOID *)(UINTN)McastIp, NULL);
  } else {
    NetMapIterate (&Instance->McastIps, Udp4LeaveGroup, MulticastAddress);
  }

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to or deletes a route from the routing table.
  Routes are determined by comparing the SubnetAddress with the destination IP
  address and arithmetically AND-ing it with the SubnetMask. The gateway address
  must be on the same subnet as the configured station address.
  The default route is added with SubnetAddress and SubnetMask both set to 0.0.0.0.
  The default route matches all destination IP addresses that do not match any
  other routes.
  A zero GatewayAddress is a nonroute. Packets are sent to the destination IP
  address if it can be found in the Address Resolution Protocol (ARP) cache or
  on the local subnet. One automatic nonroute entry will be inserted into the
  routing table for outgoing packets that are addressed to a local subnet
  (gateway address of 0.0.0.0).
  Each instance of the EFI UDPv4 Protocol has its own independent routing table.
  Instances of the EFI UDPv4 Protocol that use the default IP address will also
  have copies of the routing table provided by the EFI_IP4_CONFIG_PROTOCOL. These
  copies will be updated automatically whenever the IP driver reconfigures its
  instances; as a result, the previous modification to these copies will be lost.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  DeleteRoute       Set to TRUE to delete this route from the routing table.
                                Set to FALSE to add this route to the routing table.
  @param[in]  SubnetAddress     The destination network address that needs to be routed.
  @param[in]  SubnetMask        The subnet mask of SubnetAddress.
  @param[in]  GatewayAddress    The gateway IP address for this route.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_NOT_STARTED       The EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                - RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND         This route is not in the routing table.
  @retval EFI_ACCESS_DENIED     The route is already defined in the routing table.

**/
EFI_STATUS
EFIAPI
Udp4Routes (
  IN EFI_UDP4_PROTOCOL  *This,
  IN BOOLEAN            DeleteRoute,
  IN EFI_IPv4_ADDRESS   *SubnetAddress,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *GatewayAddress
  )
{
  UDP4_INSTANCE_DATA  *Instance;
  EFI_IP4_PROTOCOL    *Ip;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (Instance->IsNoMapping) {
    return EFI_NO_MAPPING;
  }

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  Ip = Instance->IpInfo->Ip.Ip4;

  //
  // Invoke the Ip instance the Udp4 instance consumes to do the actual operation.
  //
  return Ip->Routes (Ip, DeleteRoute, SubnetAddress, SubnetMask, GatewayAddress);
}

/**
  Queues outgoing data packets into the transmit queue.

  The Transmit() function places a sending request to this instance of the EFI
  UDPv4 Protocol, alongside the transmit data that was filled by the user. Whenever
  the packet in the token is sent out or some errors occur, the Token.Event will
  be signaled and Token.Status is updated. Providing a proper notification function
  and context for the event will enable the user to receive the notification and
  transmitting status.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token             Pointer to the completion token that will be placed into the
                                transmit queue.

  @retval EFI_SUCCESS           The data has been queued for transmission.
  @retval EFI_NOT_STARTED       This EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_ACCESS_DENIED     The transmit completion token with the same
                                Token.Event was already in the transmit queue.
  @retval EFI_NOT_READY         The completion token could not be queued because the
                                transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval EFI_NOT_FOUND         There is no route to the destination network or address.
  @retval EFI_BAD_BUFFER_SIZE   The data length is greater than the maximum UDP packet
                                size. Or the length of the IP header + UDP header + data
                                length is greater than MTU if DoNotFragment is TRUE.

**/
EFI_STATUS
EFIAPI
Udp4Transmit (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS              Status;
  UDP4_INSTANCE_DATA      *Instance;
  EFI_TPL                 OldTpl;
  NET_BUF                 *Packet;
  EFI_UDP_HEADER          *Udp4Header;
  EFI_UDP4_CONFIG_DATA    *ConfigData;
  IP4_ADDR                Source;
  IP4_ADDR                Destination;
  EFI_UDP4_TRANSMIT_DATA  *TxData;
  EFI_UDP4_SESSION_DATA   *UdpSessionData;
  UDP4_SERVICE_DATA       *Udp4Service;
  IP_IO_OVERRIDE          Override;
  UINT16                  HeadSum;
  EFI_IP_ADDRESS          IpDestAddr;

  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (Instance->IsNoMapping) {
    return EFI_NO_MAPPING;
  }

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Validate the Token, if the token is invalid return the error code.
  //
  Status = Udp4ValidateTxToken (Instance, Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (EFI_ERROR (NetMapIterate (&Instance->TxTokens, Udp4TokenExist, Token)) ||
      EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp4TokenExist, Token)))
  {
    //
    // Try to find a duplicate token in the two token maps, if found, return
    // EFI_ACCESS_DENIED.
    //
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  TxData = Token->Packet.TxData;

  //
  // Create a net buffer to hold the user buffer and the udp header.
  //
  Packet = NetbufFromExt (
             (NET_FRAGMENT *)TxData->FragmentTable,
             TxData->FragmentCount,
             UDP4_HEADER_SIZE,
             0,
             Udp4NetVectorExtFree,
             NULL
             );
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Store the IpIo in ProtoData.
  //
  Udp4Service                       = Instance->Udp4Service;
  *((UINTN *)&Packet->ProtoData[0]) = (UINTN)(Udp4Service->IpIo);

  Udp4Header = (EFI_UDP_HEADER *)NetbufAllocSpace (Packet, UDP4_HEADER_SIZE, TRUE);
  ASSERT (Udp4Header != NULL);

  ConfigData = &Instance->ConfigData;

  //
  // Fill the udp header.
  //
  Udp4Header->SrcPort  = HTONS (ConfigData->StationPort);
  Udp4Header->DstPort  = HTONS (ConfigData->RemotePort);
  Udp4Header->Length   = HTONS ((UINT16)Packet->TotalSize);
  Udp4Header->Checksum = 0;

  UdpSessionData = TxData->UdpSessionData;
  IP4_COPY_ADDRESS (&Override.Ip4OverrideData.SourceAddress, &ConfigData->StationAddress);

  if (UdpSessionData != NULL) {
    //
    // Set the SourceAddress, SrcPort and Destination according to the specified
    // UdpSessionData.
    //
    if (!EFI_IP4_EQUAL (&UdpSessionData->SourceAddress, &mZeroIp4Addr)) {
      IP4_COPY_ADDRESS (&Override.Ip4OverrideData.SourceAddress, &UdpSessionData->SourceAddress);
    }

    if (UdpSessionData->SourcePort != 0) {
      Udp4Header->SrcPort = HTONS (UdpSessionData->SourcePort);
    }

    if (UdpSessionData->DestinationPort != 0) {
      Udp4Header->DstPort = HTONS (UdpSessionData->DestinationPort);
    }

    CopyMem (&Source, &Override.Ip4OverrideData.SourceAddress, sizeof (IP4_ADDR));
    CopyMem (&Destination, &UdpSessionData->DestinationAddress, sizeof (IP4_ADDR));

    //
    // calculate the pseudo head checksum using the overridden parameters.
    //
    HeadSum = NetPseudoHeadChecksum (
                Source,
                Destination,
                EFI_IP_PROTO_UDP,
                0
                );
  } else {
    //
    // UdpSessionData is NULL, use the address and port information previously configured.
    //
    CopyMem (&Destination, &ConfigData->RemoteAddress, sizeof (IP4_ADDR));

    HeadSum = Instance->HeadSum;
  }

  //
  // calculate the checksum.
  //
  Udp4Header->Checksum = Udp4Checksum (Packet, HeadSum);
  if (Udp4Header->Checksum == 0) {
    //
    // If the calculated checksum is 0, fill the Checksum field with all ones.
    //
    Udp4Header->Checksum = 0xffff;
  }

  //
  // Fill the IpIo Override data.
  //
  if (TxData->GatewayAddress != NULL) {
    IP4_COPY_ADDRESS (&Override.Ip4OverrideData.GatewayAddress, TxData->GatewayAddress);
  } else {
    ZeroMem (&Override.Ip4OverrideData.GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
  }

  Override.Ip4OverrideData.Protocol      = EFI_IP_PROTO_UDP;
  Override.Ip4OverrideData.TypeOfService = ConfigData->TypeOfService;
  Override.Ip4OverrideData.TimeToLive    = ConfigData->TimeToLive;
  Override.Ip4OverrideData.DoNotFragment = ConfigData->DoNotFragment;

  //
  // Save the token into the TxToken map.
  //
  Status = NetMapInsertTail (&Instance->TxTokens, Token, Packet);
  if (EFI_ERROR (Status)) {
    goto FREE_PACKET;
  }

  //
  // Send out this datagram through IpIo.
  //
  IpDestAddr.Addr[0] = Destination;
  Status             = IpIoSend (
                         Udp4Service->IpIo,
                         Packet,
                         Instance->IpInfo,
                         Instance,
                         Token,
                         &IpDestAddr,
                         &Override
                         );
  if (EFI_ERROR (Status)) {
    //
    // Remove this token from the TxTokens.
    //
    Udp4RemoveToken (&Instance->TxTokens, Token);
  }

FREE_PACKET:

  NetbufFree (Packet);

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Places an asynchronous receive request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.
  The caller must fill in the Token.Event field in the completion token, and this
  field cannot be NULL. When the receive operation completes, the EFI UDPv4 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled. Providing a proper notification function and context for the event
  will enable the user to receive the notification and receiving status. That
  notification function is guaranteed to not be re-entered.

  @param[in]  This              Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token             Pointer to a token that is associated with
                                the receive data descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This EFI UDPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP, RARP, etc.)
                                is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_OUT_OF_RESOURCES  The receive completion token could not be queued due to a lack of system
                                resources (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_ACCESS_DENIED     A receive completion token with the same Token.Event was already in
                                the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.

**/
EFI_STATUS
EFIAPI
Udp4Receive (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS          Status;
  UDP4_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if ((This == NULL) || (Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (Instance->IsNoMapping) {
    return EFI_NO_MAPPING;
  }

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp4TokenExist, Token)) ||
      EFI_ERROR (NetMapIterate (&Instance->TxTokens, Udp4TokenExist, Token)))
  {
    //
    // Return EFI_ACCESS_DENIED if the specified token is already in the TxTokens or
    // RxTokens map.
    //
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  Token->Packet.RxData = NULL;

  //
  // Save the token into the RxTokens map.
  //
  Status = NetMapInsertTail (&Instance->RxTokens, Token, NULL);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_READY;
    goto ON_EXIT;
  }

  //
  // If there is an icmp error, report it.
  //
  Udp4ReportIcmpError (Instance);

  //
  // Try to deliver the received datagrams.
  //
  Udp4InstanceDeliverDgram (Instance);

  //
  // Dispatch the DPC queued by the NotifyFunction of Token->Event.
  //
  DispatchDpc ();

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Aborts an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that
  the asynchronous operation has completed, this function will not signal the
  token and EFI_NOT_FOUND is returned.

  @param[in]  This  Pointer to the EFI_UDP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that has been issued by
                    EFI_UDP4_PROTOCOL.Transmit() or
                    EFI_UDP4_PROTOCOL.Receive().If NULL, all pending
                    tokens are aborted.

  @retval  EFI_SUCCESS           The asynchronous I/O request was aborted and Token.Event
                                 was signaled. When Token is NULL, all pending requests are
                                 aborted and their events are signaled.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                 not found in the transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
Udp4Cancel (
  IN EFI_UDP4_PROTOCOL          *This,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  )
{
  EFI_STATUS          Status;
  UDP4_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);

  if (Instance->IsNoMapping) {
    return EFI_NO_MAPPING;
  }

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Cancel the tokens specified by Token for this instance.
  //
  Status = Udp4InstanceCancelToken (Instance, Token);

  //
  // Dispatch the DPC queued by the NotifyFunction of the cancelled token's events.
  //
  DispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase
  the rate that data packets are moved between the communications device and the
  transmit and receive queues.
  In some systems, the periodic timer event in the managed network driver may not
  poll the underlying communications device fast enough to transmit and/or receive
  all data packets without missing incoming packets or dropping outgoing packets.
  Drivers and applications that are experiencing packet loss should try calling
  the Poll() function more often.

  @param[in]  This  Pointer to the EFI_UDP4_PROTOCOL instance.

  @retval EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.

**/
EFI_STATUS
EFIAPI
Udp4Poll (
  IN EFI_UDP4_PROTOCOL  *This
  )
{
  UDP4_INSTANCE_DATA  *Instance;
  EFI_IP4_PROTOCOL    *Ip;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (This);
  Ip       = Instance->IpInfo->Ip.Ip4;

  //
  // Invode the Ip instance consumed by the udp instance to do the poll operation.
  //
  return Ip->Poll (Ip);
}
