/** @file
  Contains all EFI_UDP6_PROTOCOL interfaces.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Udp6Impl.h"

EFI_UDP6_PROTOCOL  mUdp6Protocol = {
  Udp6GetModeData,
  Udp6Configure,
  Udp6Groups,
  Udp6Transmit,
  Udp6Receive,
  Udp6Cancel,
  Udp6Poll
};

/**
  This function copies the current operational settings of this EFI UDPv6 Protocol
  instance into user-supplied buffers. This function is used optionally to retrieve
  the operational mode data of underlying networks or drivers.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[out] Udp6ConfigData     The buffer in which the current UDP configuration
                                 data is returned. This parameter is optional and
                                 may be NULL.
  @param[out] Ip6ModeData        The buffer in which the current EFI IPv6 Protocol
                                 mode data is returned. This parameter is optional
                                 and may be NULL.
  @param[out] MnpConfigData      The buffer in which the current managed network
                                 configuration data is returned. This parameter is
                                 optional and may be NULL.
  @param[out] SnpModeData        The buffer in which the simple network mode data
                                 is returned. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The mode data was read.
  @retval EFI_NOT_STARTED        When Udp6ConfigData is queried, no configuration
                                 data is  available because this instance has not
                                 been started.
  @retval EFI_INVALID_PARAMETER  This is NULL.

**/
EFI_STATUS
EFIAPI
Udp6GetModeData (
  IN  EFI_UDP6_PROTOCOL                *This,
  OUT EFI_UDP6_CONFIG_DATA             *Udp6ConfigData OPTIONAL,
  OUT EFI_IP6_MODE_DATA                *Ip6ModeData    OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData  OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData    OPTIONAL
  )
{
  UDP6_INSTANCE_DATA  *Instance;
  EFI_IP6_PROTOCOL    *Ip;
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured && (Udp6ConfigData != NULL)) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Udp6ConfigData != NULL) {
    //
    // Set the Udp6ConfigData.
    //
    CopyMem (Udp6ConfigData, &Instance->ConfigData, sizeof (EFI_UDP6_CONFIG_DATA));
  }

  Ip = Instance->IpInfo->Ip.Ip6;

  //
  // Get the underlying Ip6ModeData, MnpConfigData and SnpModeData.
  //
  Status = Ip->GetModeData (Ip, Ip6ModeData, MnpConfigData, SnpModeData);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This function is used to do the following:
  Initialize and start this instance of the EFI UDPv6 Protocol.
  Change the filtering rules and operational parameters.
  Reset this instance of the EFI UDPv6 Protocol.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  UdpConfigData      Pointer to the buffer to set the configuration
                                 data. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The configuration settings were set, changed, or
                                 reset successfully.
  @retval EFI_NO_MAPPING         When the UdpConfigData.UseAnyStationAddress is set
                                 to true and there is no address available for the IP6
                                 driver to bind a source address to this instance.
  @retval EFI_INVALID_PARAMETER  One or more following conditions are TRUE:
                                 This is NULL.
                                 UdpConfigData.StationAddress is not a valid
                                 unicast IPv6 address.
                                 UdpConfigData.RemoteAddress is not a valid unicast
                                 IPv6  address if it is not zero.
  @retval EFI_ALREADY_STARTED    The EFI UDPv6 Protocol instance is already
                                 started/configured and must be stopped/reset
                                 before it can be reconfigured. Only TrafficClass,
                                 HopLimit, ReceiveTimeout, and  TransmitTimeout can
                                 be reconfigured without stopping the  current
                                 instance of the EFI UDPv6 Protocol.
  @retval EFI_ACCESS_DENIED      UdpConfigData.AllowDuplicatePort is FALSE and
                                 UdpConfigData.StationPort is already used by another
                                 instance.
  @retval EFI_OUT_OF_RESOURCES   The EFI UDPv6 Protocol driver cannot allocate
                                 memory for this EFI UDPv6 Protocol instance.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred, and
                                 this instance was not opened.

**/
EFI_STATUS
EFIAPI
Udp6Configure (
  IN EFI_UDP6_PROTOCOL     *This,
  IN EFI_UDP6_CONFIG_DATA  *UdpConfigData OPTIONAL
  )
{
  EFI_STATUS           Status;
  UDP6_INSTANCE_DATA   *Instance;
  UDP6_SERVICE_DATA    *Udp6Service;
  EFI_TPL              OldTpl;
  EFI_IPv6_ADDRESS     StationAddress;
  EFI_IPv6_ADDRESS     RemoteAddress;
  EFI_IP6_CONFIG_DATA  Ip6ConfigData;
  EFI_IPv6_ADDRESS     LocalAddr;
  EFI_IPv6_ADDRESS     RemoteAddr;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured && (UdpConfigData == NULL)) {
    return EFI_SUCCESS;
  }

  Udp6Service = Instance->Udp6Service;
  Status      = EFI_SUCCESS;
  ASSERT (Udp6Service != NULL);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (UdpConfigData != NULL) {
    IP6_COPY_ADDRESS (&StationAddress, &UdpConfigData->StationAddress);
    IP6_COPY_ADDRESS (&RemoteAddress, &UdpConfigData->RemoteAddress);

    if ((!NetIp6IsUnspecifiedAddr (&StationAddress) && !NetIp6IsValidUnicast (&StationAddress)) ||
        (!NetIp6IsUnspecifiedAddr (&RemoteAddress) && !NetIp6IsValidUnicast (&RemoteAddress))
        )
    {
      //
      // If not use default address, and StationAddress is not a valid unicast
      // if it is not IPv6 address or RemoteAddress is not a valid unicast IPv6
      // address if it is not 0.
      //
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    if (Instance->Configured) {
      //
      // The instance is already configured, try to do the re-configuration.
      //
      if (!Udp6IsReconfigurable (&Instance->ConfigData, UdpConfigData)) {
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
      Instance->ConfigData.TrafficClass    = UdpConfigData->TrafficClass;
      Instance->ConfigData.HopLimit        = UdpConfigData->HopLimit;
      Instance->ConfigData.ReceiveTimeout  = UdpConfigData->ReceiveTimeout;
      Instance->ConfigData.TransmitTimeout = UdpConfigData->TransmitTimeout;
    } else {
      //
      // Construct the Ip configuration data from the UdpConfigData.
      //
      Udp6BuildIp6ConfigData (UdpConfigData, &Ip6ConfigData);

      //
      // Configure the Ip instance wrapped in the IpInfo.
      //
      Status = IpIoConfigIp (Instance->IpInfo, &Ip6ConfigData);
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
      CopyMem (
        &Instance->ConfigData,
        UdpConfigData,
        sizeof (EFI_UDP6_CONFIG_DATA)
        );
      IP6_COPY_ADDRESS (&Instance->ConfigData.StationAddress, &Ip6ConfigData.StationAddress);
      //
      // Try to allocate the required port resource.
      //
      Status = Udp6Bind (&Udp6Service->ChildrenList, &Instance->ConfigData);
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
      IP6_COPY_ADDRESS (&LocalAddr, &Instance->ConfigData.StationAddress);
      IP6_COPY_ADDRESS (&RemoteAddr, &Instance->ConfigData.RemoteAddress);

      Instance->HeadSum = NetIp6PseudoHeadChecksum (
                            &LocalAddr,
                            &RemoteAddr,
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
    Instance->Udp6Proto.Cancel (&Instance->Udp6Proto, NULL);

    //
    // Remove the buffered RxData for this instance.
    //
    Udp6FlushRcvdDgram (Instance);

    ASSERT (IsListEmpty (&Instance->DeliveredDgramQue));
  }

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This function is used to enable and disable the multicast group filtering.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  JoinFlag           Set to TRUE to join a multicast group. Set to
                                 FALSE to leave one or all multicast groups.
  @param[in]  MulticastAddress   Pointer to multicast group address to join or
                                 leave. This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate resources to join the group.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL.
                                 JoinFlag is TRUE and MulticastAddress is NULL.
                                 JoinFlag is TRUE and *MulticastAddress is not a
                                 valid  multicast address.
  @retval EFI_ALREADY_STARTED    The group address is already in the group table
                                 (when JoinFlag is TRUE).
  @retval EFI_NOT_FOUND          The group address is not in the group table (when
                                 JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
Udp6Groups (
  IN EFI_UDP6_PROTOCOL  *This,
  IN BOOLEAN            JoinFlag,
  IN EFI_IPv6_ADDRESS   *MulticastAddress OPTIONAL
  )
{
  EFI_STATUS          Status;
  UDP6_INSTANCE_DATA  *Instance;
  EFI_IP6_PROTOCOL    *Ip;
  EFI_TPL             OldTpl;
  EFI_IPv6_ADDRESS    *McastIp;

  if ((This == NULL) || (JoinFlag && (MulticastAddress == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  McastIp = NULL;

  if (JoinFlag) {
    if (!IP6_IS_MULTICAST (MulticastAddress)) {
      return EFI_INVALID_PARAMETER;
    }

    McastIp = AllocateCopyPool (sizeof (EFI_IPv6_ADDRESS), MulticastAddress);
    if (McastIp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);
  if (!Instance->Configured) {
    if (McastIp != NULL) {
      FreePool (McastIp);
    }

    return EFI_NOT_STARTED;
  }

  Ip = Instance->IpInfo->Ip.Ip6;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Invoke the Ip instance the Udp6 instance consumes to do the group operation.
  //
  Status = Ip->Groups (Ip, JoinFlag, MulticastAddress);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Keep a local copy of the configured multicast IPs because IpIo receives
  // datagrams from the 0 station address IP instance and then UDP delivers to
  // the matched instance. This copy of multicast IPs is used to avoid receive
  // the multicast datagrams destinated to multicast IPs the other instances configured.
  //
  if (JoinFlag) {
    Status = NetMapInsertTail (&Instance->McastIps, (VOID *)McastIp, NULL);
  } else {
    Status = NetMapIterate (&Instance->McastIps, Udp6LeaveGroup, MulticastAddress);
    if ((MulticastAddress != NULL) && (Status == EFI_ABORTED)) {
      Status = EFI_SUCCESS;
    }
  }

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  if (EFI_ERROR (Status)) {
    if (McastIp != NULL) {
      FreePool (McastIp);
    }
  }

  return Status;
}

/**
  This function places a sending request to this instance of the EFI UDPv6 Protocol,
  alongside the transmit data that was filled by the user.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the completion token that will be
                                 placed into the transmit queue.

  @retval EFI_SUCCESS            The data was queued for transmission.
  @retval EFI_NOT_STARTED        This EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         The under-layer IPv6 driver was responsible for
                                 choosing a source address for this instance, but
                                 no  source address was available for use.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 This is NULL.
                                 Token is NULL. Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentLength
                                 fields is zero.
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentBuffer
                                 fields is NULL. One or more of the
                                 Token.Packet.TxData.UdpSessionData.DestinationAddress
                                 are not valid unicast IPv6
                                 addresses if the  UdpSessionData is not NULL.
                                 Token.Packet.TxData.UdpSessionData.
                                 DestinationAddress is NULL
                                 Token.Packet.TxData.UdpSessionData.
                                 DestinationPort
                                 is zero.
                                 Token.Packet.TxData.UdpSessionData is NULL and this
                                 instance's UdpConfigData.RemoteAddress  is unspecified.
  @retval EFI_ACCESS_DENIED      The transmit completion token with the same
                                 Token.Event is already in the transmit queue.
  @retval EFI_NOT_READY          The completion token could not be queued because
                                 the transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES   Could not queue the transmit data.
  @retval EFI_NOT_FOUND          There is no route to the destination network or
                                 address.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size. Or, the length of the IP header + UDP
                                 header + data length is greater than MTU if
                                 DoNotFragment is TRUE.

**/
EFI_STATUS
EFIAPI
Udp6Transmit (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS              Status;
  UDP6_INSTANCE_DATA      *Instance;
  EFI_TPL                 OldTpl;
  NET_BUF                 *Packet;
  EFI_UDP_HEADER          *Udp6Header;
  EFI_UDP6_CONFIG_DATA    *ConfigData;
  EFI_IPv6_ADDRESS        Source;
  EFI_IPv6_ADDRESS        Destination;
  EFI_UDP6_TRANSMIT_DATA  *TxData;
  EFI_UDP6_SESSION_DATA   *UdpSessionData;
  UDP6_SERVICE_DATA       *Udp6Service;
  IP_IO_OVERRIDE          Override;
  UINT16                  HeadSum;
  EFI_IP_ADDRESS          IpDestAddr;

  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Validate the Token, if the token is invalid return the error code.
  //
  Status = Udp6ValidateTxToken (Instance, Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (EFI_ERROR (NetMapIterate (&Instance->TxTokens, Udp6TokenExist, Token)) ||
      EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp6TokenExist, Token))
      )
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
             UDP6_HEADER_SIZE,
             0,
             Udp6NetVectorExtFree,
             NULL
             );
  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Store the IpIo in ProtoData.
  //
  Udp6Service                       = Instance->Udp6Service;
  *((UINTN *)&Packet->ProtoData[0]) = (UINTN)(Udp6Service->IpIo);

  Udp6Header = (EFI_UDP_HEADER *)NetbufAllocSpace (Packet, UDP6_HEADER_SIZE, TRUE);
  ASSERT (Udp6Header != NULL);
  if (Udp6Header == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  ConfigData = &Instance->ConfigData;

  //
  // Fill the udp header.
  //
  Udp6Header->SrcPort  = HTONS (ConfigData->StationPort);
  Udp6Header->DstPort  = HTONS (ConfigData->RemotePort);
  Udp6Header->Length   = HTONS ((UINT16)Packet->TotalSize);
  Udp6Header->Checksum = 0;
  //
  // Set the UDP Header in NET_BUF, this UDP header is for IP6 can fast get the
  // Udp header for pseudoHeadCheckSum.
  //
  Packet->Udp    = Udp6Header;
  UdpSessionData = TxData->UdpSessionData;

  if (UdpSessionData != NULL) {
    //
    // Set the Destination according to the specified
    // UdpSessionData.
    //

    if (UdpSessionData->DestinationPort != 0) {
      Udp6Header->DstPort = HTONS (UdpSessionData->DestinationPort);
    }

    IP6_COPY_ADDRESS (&Source, &ConfigData->StationAddress);
    if (!NetIp6IsUnspecifiedAddr (&UdpSessionData->DestinationAddress)) {
      IP6_COPY_ADDRESS (&Destination, &UdpSessionData->DestinationAddress);
    } else {
      IP6_COPY_ADDRESS (&Destination, &ConfigData->RemoteAddress);
    }

    //
    // Calculate the pseudo head checksum using the overridden parameters.
    //
    if (!NetIp6IsUnspecifiedAddr (&ConfigData->StationAddress)) {
      HeadSum = NetIp6PseudoHeadChecksum (
                  &Source,
                  &Destination,
                  EFI_IP_PROTO_UDP,
                  0
                  );

      //
      // calculate the checksum.
      //
      Udp6Header->Checksum = Udp6Checksum (Packet, HeadSum);
      if (Udp6Header->Checksum == 0) {
        //
        // If the calculated checksum is 0, fill the Checksum field with all ones.
        //
        Udp6Header->Checksum = 0xffff;
      }
    } else {
      //
      // Set the checksum is zero if the ConfigData->StationAddress is unspecified
      // and the Ipv6 will fill the correct value of this checksum.
      //
      Udp6Header->Checksum = 0;
    }
  } else {
    //
    // UdpSessionData is NULL, use the address and port information previously configured.
    //
    IP6_COPY_ADDRESS (&Destination, &ConfigData->RemoteAddress);

    HeadSum = Instance->HeadSum;
    //
    // calculate the checksum.
    //
    Udp6Header->Checksum = Udp6Checksum (Packet, HeadSum);
    if (Udp6Header->Checksum == 0) {
      //
      // If the calculated checksum is 0, fill the Checksum field with all ones.
      //
      Udp6Header->Checksum = 0xffff;
    }
  }

  //
  // Fill the IpIo Override data.
  //
  Override.Ip6OverrideData.Protocol  = EFI_IP_PROTO_UDP;
  Override.Ip6OverrideData.HopLimit  = ConfigData->HopLimit;
  Override.Ip6OverrideData.FlowLabel = 0;

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
  if (UdpSessionData != NULL) {
    IP6_COPY_ADDRESS (&(IpDestAddr.v6), &Destination);
  } else {
    ZeroMem (&IpDestAddr.v6, sizeof (EFI_IPv6_ADDRESS));
  }

  Status = IpIoSend (
             Udp6Service->IpIo,
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
    Udp6RemoveToken (&Instance->TxTokens, Token);
  }

FREE_PACKET:

  NetbufFree (Packet);

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This function places a completion token into the receive packet queue. This function
  is always asynchronous.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that is associated with the
                                 receive data descriptor.

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This EFI UDPv6 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. Token is NULL. Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued
                                 due to a lack of system resources (usually
                                 memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI UDPv6 Protocol instance has been reset to
                                 startup defaults.
  @retval EFI_ACCESS_DENIED      A receive completion token with the same
                                 Token.Event is already in the receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because
                                 the receive  queue is full.

**/
EFI_STATUS
EFIAPI
Udp6Receive (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS          Status;
  UDP6_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if ((This == NULL) || (Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp6TokenExist, Token)) ||
      EFI_ERROR (NetMapIterate (&Instance->TxTokens, Udp6TokenExist, Token))
      )
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
  Udp6ReportIcmpError (Instance);

  //
  // Try to delivered the received datagrams.
  //
  Udp6InstanceDeliverDgram (Instance);

  //
  // Dispatch the DPC queued by the NotifyFunction of Token->Event.
  //
  DispatchDpc ();

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This function is used to abort a pending transmit or receive request.

  @param[in]  This               Pointer to the EFI_UDP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that has been issued by
                                 EFI_UDP6_PROTOCOL.Transmit() or
                                 EFI_UDP6_PROTOCOL.Receive(). This parameter is
                                 optional and may be NULL.

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted, and
                                 Token.Event was  signaled. When Token is NULL, all
                                 pending requests are aborted and their events are
                                 signaled.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_NO_MAPPING         When using the default address, configuration
                                 (DHCP, BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O
                                 request is not found in the transmit or receive
                                 queue. It is either completed or not issued by
                                 Transmit() or Receive().

**/
EFI_STATUS
EFIAPI
Udp6Cancel (
  IN EFI_UDP6_PROTOCOL          *This,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token OPTIONAL
  )
{
  EFI_STATUS          Status;
  UDP6_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Cancel the tokens specified by Token for this instance.
  //
  Status = Udp6InstanceCancelToken (Instance, Token);

  //
  // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
  //
  DispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This function can be used by network drivers and applications to increase the rate that
  data packets are moved between the communications device and the transmit/receive queues.

  @param[in] This                Pointer to the EFI_UDP6_PROTOCOL instance.

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_TIMEOUT            Data was dropped out of the transmit and/or
                                 receive queue.

**/
EFI_STATUS
EFIAPI
Udp6Poll (
  IN EFI_UDP6_PROTOCOL  *This
  )
{
  UDP6_INSTANCE_DATA  *Instance;
  EFI_IP6_PROTOCOL    *Ip;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (This);
  Ip       = Instance->IpInfo->Ip.Ip6;

  //
  // Invode the Ip instance consumed by the udp instance to do the poll operation.
  //
  return Ip->Poll (Ip);
}
