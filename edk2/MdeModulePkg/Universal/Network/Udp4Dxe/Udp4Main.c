/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Udp4Main.c

Abstract:


**/

#include "Udp4Impl.h"

#include <Protocol/Ip4.h>

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
  This function copies the current operational settings of this EFI UDPv4 Protocol
  instance into user-supplied buffers. This function is used optionally to retrieve
  the operational mode data of underlying networks or drivers.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Udp4ConfigData         Pointer to the buffer to receive the current
                                 configuration data.
  @param  Ip4ModeData            Pointer to the EFI IPv4 Protocol mode data
                                 structure.
  @param  MnpConfigData          Pointer to the managed network configuration data
                                 structure.
  @param  SnpModeData            Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS            The mode data was read.
  @retval EFI_NOT_STARTED        When Udp4ConfigData is queried, no configuration
                                 data is  available because this instance has not
                                 been started.
  @retval EFI_INVALID_PARAMETER  This is NULL.

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

  Ip = Instance->IpInfo->Ip;

  //
  // Get the underlying Ip4ModeData, MnpConfigData and SnpModeData.
  //
  Status = Ip->GetModeData (Ip, Ip4ModeData, MnpConfigData, SnpModeData);

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  This function is used to do the following:
  Initialize and start this instance of the EFI UDPv4 Protocol.
  Change the filtering rules and operational parameters.
  Reset this instance of the EFI UDPv4 Protocol.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  UdpConfigData          Pointer to the buffer to receive the current mode
                                 data.

  @retval EFI_SUCCESS            The configuration settings were set, changed, or
                                 reset successfully.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more following conditions are TRUE: This is
                                 NULL. UdpConfigData.StationAddress is not a valid
                                 unicast IPv4 address. UdpConfigData.SubnetMask is
                                 not a valid IPv4 address mask.
                                 UdpConfigData.RemoteAddress is not a valid unicast
                                 IPv4  address if it is not zero.
  @retval EFI_ALREADY_STARTED    The EFI UDPv4 Protocol instance is already
                                 started/configured and must be stopped/reset
                                 before it can be reconfigured. Only TypeOfService,
                                 TimeToLive, DoNotFragment, ReceiveTimeout, and
                                 TransmitTimeout can be reconfigured without
                                 stopping the current instance of the EFI UDPv4
                                 Protocol.
  @retval EFI_ACCESS_DENIED      UdpConfigData.AllowDuplicatePort is FALSE and
                                 UdpConfigData.StationPort is already used by other
                                 instance.
  @retval EFI_OUT_OF_RESOURCES   The EFI UDPv4 Protocol driver cannot allocate
                                 memory for this EFI UDPv4 Protocol instance.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred and
                                 this instance was not opened.

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
      !((StationAddress == 0) || Ip4IsUnicast (StationAddress, SubnetMask)) ||
      !((RemoteAddress  == 0) || Ip4IsUnicast (RemoteAddress, 0)))) {
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
      Instance->ConfigData.StationAddress = Ip4ConfigData.StationAddress;
      Instance->ConfigData.SubnetMask     = Ip4ConfigData.SubnetMask;

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

  Udp4SetVariableData (Instance->Udp4Service);

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  This function is used to enable and disable the multicast group filtering.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  JoinFlag               Set to TRUE to join a multicast group. Set to
                                 FALSE to leave one or all multicast groups.
  @param  MulticastAddress       Pointer to multicast group address to join or
                                 leave.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The EFI UDPv4 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate resources to join the group.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. JoinFlag is TRUE and
                                 MulticastAddress is NULL. JoinFlag is TRUE and
                                 *MulticastAddress is not a valid  multicast
                                 address.
  @retval EFI_ALREADY_STARTED    The group address is already in the group table
                                 (when JoinFlag is TRUE).
  @retval EFI_NOT_FOUND          The group address is not in the group table (when
                                 JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

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

  Ip = Instance->IpInfo->Ip;

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
  // the mutlicast datagrams destinated to multicast IPs the other instances configured.
  //
  if (JoinFlag) {

    NetMapInsertTail (&Instance->McastIps, (VOID *) (UINTN) McastIp, NULL);
  } else {

    NetMapIterate (&Instance->McastIps, Udp4LeaveGroup, MulticastAddress);
  }

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  This function adds a route to or deletes a route from the routing table.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  DeleteRoute            Set to TRUE to delete this route from the routing
                                 table. Set to FALSE to add this route to the
                                 routing table.
  @param  SubnetAddress          The destination network address that needs to be
                                 routed.
  @param  SubnetMask             The subnet mask of SubnetAddress.
  @param  GatewayAddress         The gateway IP address for this route.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The EFI UDPv4 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. SubnetAddress is NULL. SubnetMask is
                                 NULL. GatewayAddress is NULL. SubnetAddress is not
                                 a valid subnet address. SubnetMask is not a valid
                                 subnet mask. GatewayAddress is not a valid unicast
                                 IP address.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table.
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table.

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

  Ip = Instance->IpInfo->Ip;

  //
  // Invoke the Ip instance the Udp4 instance consumes to do the actual operation.
  //
  return Ip->Routes (Ip, DeleteRoute, SubnetAddress, SubnetMask, GatewayAddress);
}


/**
  This function places a sending request to this instance of the EFI UDPv4 Protocol,
  alongside the transmit data that was filled by the user.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token                  Pointer to the completion token that will be
                                 placed into the transmit queue.

  @retval EFI_SUCCESS            The data has been queued for transmission.
  @retval EFI_NOT_STARTED        This EFI UDPv4 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE: This is
                                 NULL. Token is NULL. Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentLength fields is zero. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentBuffer fields is NULL.
                                 Token.Packet.TxData. GatewayAddress is not a
                                 unicast IPv4 address if it is not NULL. One or
                                 more IPv4 addresses in Token.Packet.TxData.
                                 UdpSessionData are not valid unicast IPv4
                                 addresses if the UdpSessionData is not NULL.
  @retval EFI_ACCESS_DENIED      The transmit completion token with the same
                                 Token.Event is already in the transmit queue.
  @retval EFI_NOT_READY          The completion token could not be queued because
                                 the transmit queue is full.
  @retval EFI_OUT_OF_RESOURCES   Could not queue the transmit data.
  @retval EFI_NOT_FOUND          There is no route to the destination network or
                                 address.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size. Or the length of the IP header + UDP
                                 header + data length is greater than MTU if
                                 DoNotFragment is TRUE.

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
  EFI_UDP4_HEADER         *Udp4Header;
  EFI_UDP4_CONFIG_DATA    *ConfigData;
  IP4_ADDR                Source;
  IP4_ADDR                Destination;
  EFI_UDP4_TRANSMIT_DATA  *TxData;
  EFI_UDP4_SESSION_DATA   *UdpSessionData;
  UDP4_SERVICE_DATA       *Udp4Service;
  IP_IO_OVERRIDE          Override;
  UINT16                  HeadSum;

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
    EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp4TokenExist, Token))) {
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
  Udp4Service = Instance->Udp4Service;
  *((UINTN *) &Packet->ProtoData[0]) = (UINTN) (Udp4Service->IpIo);

  Udp4Header = (EFI_UDP4_HEADER *) NetbufAllocSpace (Packet, UDP4_HEADER_SIZE, TRUE);
  ConfigData = &Instance->ConfigData;

  //
  // Fill the udp header.
  //
  Udp4Header->SrcPort      = HTONS (ConfigData->StationPort);
  Udp4Header->DstPort      = HTONS (ConfigData->RemotePort);
  Udp4Header->Length       = HTONS (Packet->TotalSize);
  Udp4Header->Checksum     = 0;

  UdpSessionData = TxData->UdpSessionData;
  Override.SourceAddress = ConfigData->StationAddress;

  if (UdpSessionData != NULL) {
    //
    // Set the SourceAddress, SrcPort and Destination according to the specified
    // UdpSessionData.
    //
    if (!EFI_IP4_EQUAL (&UdpSessionData->SourceAddress, &mZeroIp4Addr)) {
      CopyMem (&Override.SourceAddress, &UdpSessionData->SourceAddress, sizeof (EFI_IPv4_ADDRESS));
    }

    if (UdpSessionData->SourcePort != 0) {
      Udp4Header->SrcPort = HTONS (UdpSessionData->SourcePort);
    }

    if (UdpSessionData->DestinationPort != 0) {
      Udp4Header->DstPort = HTONS (UdpSessionData->DestinationPort);
    }

    CopyMem (&Source, &Override.SourceAddress, sizeof (IP4_ADDR));
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
    CopyMem (&Override.GatewayAddress, TxData->GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
  } else {
    ZeroMem (&Override.GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
  }

  Override.Protocol                 = EFI_IP_PROTO_UDP;
  Override.TypeOfService            = ConfigData->TypeOfService;
  Override.TimeToLive               = ConfigData->TimeToLive;
  Override.DoNotFragment            = ConfigData->DoNotFragment;

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
  Status = IpIoSend (
             Udp4Service->IpIo,
             Packet,
             Instance->IpInfo,
             Instance,
             Token,
             Destination,
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
  This function places a completion token into the receive packet queue. This function
  is always asynchronous.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token                  Pointer to a token that is associated with the
                                 receive data descriptor.

  @retval EFI_SUCCESS            The receive completion token is cached.
  @retval EFI_NOT_STARTED        This EFI UDPv4 Protocol instance has not been
                                 started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. Token is NULL. Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued
                                 due to a lack of system resources (usually
                                 memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI UDPv4 Protocol instance has been reset to
                                 startup defaults.
  @retval EFI_ACCESS_DENIED      A receive completion token with the same
                                 Token.Event is already in the receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because
                                 the receive  queue is full.

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

  if (EFI_ERROR (NetMapIterate (&Instance->RxTokens, Udp4TokenExist, Token))||
    EFI_ERROR (NetMapIterate (&Instance->TxTokens, Udp4TokenExist, Token))) {
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
  // Try to delivered the received datagrams.
  //
  Udp4InstanceDeliverDgram (Instance);

  //
  // Dispatch the DPC queued by the NotifyFunction of Token->Event.
  //
  NetLibDispatchDpc ();

ON_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  This function is used to abort a pending transmit or receive request.

  @param  This                   Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  Token                  Pointer to a token that has been issued by
                                 EFI_UDP4_PROTOCOL.Transmit() or
                                 EFI_UDP4_PROTOCOL.Receive().

  @retval EFI_SUCCESS            The asynchronous I/O request is aborted and
                                 Token.Event is  signaled. When Token is NULL, all
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
  // Cancle the tokens specified by Token for this instance.
  //
  Status = Udp4InstanceCancelToken (Instance, Token);

  //
  // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
  //
  NetLibDispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  This function can be used by network drivers and applications to increase the rate that
  data packets are moved between the communications device and the transmit/receive queues.
  Argumens:
  This - Pointer to the EFI_UDP4_PROTOCOL instance.

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_TIMEOUT            Data was dropped out of the transmit and/or
                                 receive queue.

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
  Ip       = Instance->IpInfo->Ip;

  //
  // Invode the Ip instance consumed by the udp instance to do the poll operation.
  //
  return Ip->Poll (Ip);
}
