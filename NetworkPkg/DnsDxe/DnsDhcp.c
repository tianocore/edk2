/** @file
Functions implementation related with DHCPv4/v6 for DNS driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DnsImpl.h"

/**
  This function initialize the DHCP4 message instance.

  This function will pad each item of dhcp4 message packet.

  @param  Seed             Pointer to the message instance of the DHCP4 packet.
  @param  InterfaceInfo    Pointer to the EFI_IP4_CONFIG2_INTERFACE_INFO instance.

**/
VOID
DnsInitSeedPacket (
  OUT EFI_DHCP4_PACKET               *Seed,
  IN  EFI_IP4_CONFIG2_INTERFACE_INFO *InterfaceInfo
  )
{
  EFI_DHCP4_HEADER           *Header;

  //
  // Get IfType and HwAddressSize from SNP mode data.
  //
  Seed->Size            = sizeof (EFI_DHCP4_PACKET);
  Seed->Length          = sizeof (Seed->Dhcp4);
  Header                = &Seed->Dhcp4.Header;
  ZeroMem (Header, sizeof (EFI_DHCP4_HEADER));
  Header->OpCode        = DHCP4_OPCODE_REQUEST;
  Header->HwType        = InterfaceInfo->IfType;
  Header->HwAddrLen     = (UINT8) InterfaceInfo->HwAddressSize;
  CopyMem (Header->ClientHwAddr, &(InterfaceInfo->HwAddress), Header->HwAddrLen);

  Seed->Dhcp4.Magik     = DHCP4_MAGIC;
  Seed->Dhcp4.Option[0] = DHCP4_TAG_EOP;
}

/**
  The common notify function.

  @param[in]  Event   The event signaled.
  @param[in]  Context The context.

**/
VOID
EFIAPI
DhcpCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if ((Event == NULL) || (Context == NULL)) {
    return ;
  }

  *((BOOLEAN *) Context) = TRUE;
}

/**
  Parse the ACK to get required information

  @param  Dhcp4            The DHCP4 protocol.
  @param  Packet           Packet waiting for parse.
  @param  DnsServerInfor   The required Dns4 server information.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_NO_MAPPING        DHCP failed to acquire address and other information.
  @retval EFI_DEVICE_ERROR      Other errors as indicated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.

**/
EFI_STATUS
ParseDhcp4Ack (
  IN EFI_DHCP4_PROTOCOL         *Dhcp4,
  IN EFI_DHCP4_PACKET           *Packet,
  IN DNS4_SERVER_INFOR          *DnsServerInfor
  )
{
  EFI_STATUS              Status;
  UINT32                  OptionCount;
  EFI_DHCP4_PACKET_OPTION **OptionList;
  UINT32                  ServerCount;
  EFI_IPv4_ADDRESS        *ServerList;
  UINT32                  Index;
  UINT32                  Count;

  ServerCount = 0;
  ServerList = NULL;

  OptionCount = 0;
  OptionList  = NULL;

  Status      = Dhcp4->Parse (Dhcp4, Packet, &OptionCount, OptionList);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_DEVICE_ERROR;
  }

  OptionList = AllocatePool (OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));
  if (OptionList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Dhcp4->Parse (Dhcp4, Packet, &OptionCount, OptionList);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (OptionList);
    return EFI_DEVICE_ERROR;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < OptionCount; Index++) {
    //
    // Get DNS server addresses
    //
    if (OptionList[Index]->OpCode == DHCP4_TAG_DNS_SERVER) {

      if (((OptionList[Index]->Length & 0x3) != 0) || (OptionList[Index]->Length == 0)) {
        Status = EFI_DEVICE_ERROR;
        break;
      }

      ServerCount = OptionList[Index]->Length/4;
      ServerList = AllocatePool (ServerCount * sizeof (EFI_IPv4_ADDRESS));
      if (ServerList == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      for (Count=0; Count < ServerCount; Count++) {
        CopyMem (ServerList + Count, &OptionList[Index]->Data[4 * Count], sizeof (EFI_IPv4_ADDRESS));
      }

      *(DnsServerInfor->ServerCount) = ServerCount;
      DnsServerInfor->ServerList     = ServerList;

      Status = EFI_SUCCESS;
    }
  }

  gBS->FreePool (OptionList);

  return Status;
}

/**
  EFI_DHCP6_INFO_CALLBACK is provided by the consumer of the EFI DHCPv6 Protocol
  instance to intercept events that occurs in the DHCPv6 Information Request
  exchange process.

  @param  This                  Pointer to the EFI_DHCP6_PROTOCOL instance that
                                is used to configure this  callback function.
  @param  Context               Pointer to the context that is initialized in
                                the EFI_DHCP6_PROTOCOL.InfoRequest().
  @param  Packet                Pointer to Reply packet that has been received.
                                The EFI DHCPv6 Protocol instance is responsible
                                for freeing the buffer.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_DEVICE_ERROR      Other errors as indicated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
ParseDhcp6Ack (
  IN EFI_DHCP6_PROTOCOL          *This,
  IN VOID                        *Context,
  IN EFI_DHCP6_PACKET            *Packet
  )
{
  EFI_STATUS                  Status;
  UINT32                      OptionCount;
  EFI_DHCP6_PACKET_OPTION     **OptionList;
  DNS6_SERVER_INFOR           *DnsServerInfor;
  UINT32                      ServerCount;
  EFI_IPv6_ADDRESS            *ServerList;
  UINT32                      Index;
  UINT32                      Count;

  OptionCount = 0;
  ServerCount = 0;
  ServerList  = NULL;

  Status      = This->Parse (This, Packet, &OptionCount, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_DEVICE_ERROR;
  }

  OptionList = AllocateZeroPool (OptionCount * sizeof (EFI_DHCP6_PACKET_OPTION *));
  if (OptionList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = This->Parse (This, Packet, &OptionCount, OptionList);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (OptionList);
    return EFI_DEVICE_ERROR;
  }

  DnsServerInfor = (DNS6_SERVER_INFOR *) Context;

  for (Index = 0; Index < OptionCount; Index++) {
    OptionList[Index]->OpCode = NTOHS (OptionList[Index]->OpCode);
    OptionList[Index]->OpLen  = NTOHS (OptionList[Index]->OpLen);

    //
    // Get DNS server addresses from this reply packet.
    //
    if (OptionList[Index]->OpCode == DHCP6_TAG_DNS_SERVER) {

      if (((OptionList[Index]->OpLen & 0xf) != 0) || (OptionList[Index]->OpLen == 0)) {
        Status = EFI_DEVICE_ERROR;
        gBS->FreePool (OptionList);
        return Status;
      }

      ServerCount = OptionList[Index]->OpLen/16;
      ServerList = AllocatePool (ServerCount * sizeof (EFI_IPv6_ADDRESS));
      if (ServerList == NULL) {
        gBS->FreePool (OptionList);
        return EFI_OUT_OF_RESOURCES;
      }

      for (Count=0; Count < ServerCount; Count++) {
        CopyMem (ServerList + Count, &OptionList[Index]->Data[16 * Count], sizeof (EFI_IPv6_ADDRESS));
      }

      *(DnsServerInfor->ServerCount) = ServerCount;
      DnsServerInfor->ServerList     = ServerList;
    }
  }

  gBS->FreePool (OptionList);

  return Status;

}

/**
  Parse the DHCP ACK to get Dns4 server information.

  @param  Instance         The DNS instance.
  @param  DnsServerCount   Retrieved Dns4 server Ip count.
  @param  DnsServerList    Retrieved Dns4 server Ip list.

  @retval EFI_SUCCESS           The Dns4 information is got from the DHCP ACK.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
GetDns4ServerFromDhcp4 (
  IN  DNS_INSTANCE               *Instance,
  OUT UINT32                     *DnsServerCount,
  OUT EFI_IPv4_ADDRESS           **DnsServerList
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          Image;
  EFI_HANDLE                          Controller;
  EFI_STATUS                          MediaStatus;
  EFI_HANDLE                          MnpChildHandle;
  EFI_MANAGED_NETWORK_PROTOCOL        *Mnp;
  EFI_MANAGED_NETWORK_CONFIG_DATA     MnpConfigData;
  EFI_HANDLE                          Dhcp4Handle;
  EFI_DHCP4_PROTOCOL                  *Dhcp4;
  EFI_IP4_CONFIG2_PROTOCOL            *Ip4Config2;
  UINTN                               DataSize;
  VOID                                *Data;
  EFI_IP4_CONFIG2_INTERFACE_INFO      *InterfaceInfo;
  EFI_DHCP4_PACKET                    SeedPacket;
  EFI_DHCP4_PACKET_OPTION             *ParaList[2];
  DNS4_SERVER_INFOR                   DnsServerInfor;

  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN    Token;
  BOOLEAN                             IsDone;
  UINTN                               Index;

  Image                      = Instance->Service->ImageHandle;
  Controller                 = Instance->Service->ControllerHandle;

  MnpChildHandle             = NULL;
  Mnp                        = NULL;

  Dhcp4Handle                = NULL;
  Dhcp4                      = NULL;

  Ip4Config2                 = NULL;
  DataSize                   = 0;
  Data                       = NULL;
  InterfaceInfo              = NULL;

  ZeroMem ((UINT8 *) ParaList, sizeof (ParaList));

  ZeroMem (&MnpConfigData, sizeof (EFI_MANAGED_NETWORK_CONFIG_DATA));

  ZeroMem (&DnsServerInfor, sizeof (DNS4_SERVER_INFOR));

  ZeroMem (&Token, sizeof (EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN));

  DnsServerInfor.ServerCount = DnsServerCount;

  IsDone = FALSE;

  //
  // Check media.
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Controller, DNS_CHECK_MEDIA_GET_DHCP_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    return EFI_NO_MEDIA;
  }

  //
  // Create a Mnp child instance, get the protocol and config for it.
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &MnpChildHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  MnpChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  MnpConfigData.ReceivedQueueTimeoutValue = 0;
  MnpConfigData.TransmitQueueTimeoutValue = 0;
  MnpConfigData.ProtocolTypeFilter        = IP4_ETHER_PROTO;
  MnpConfigData.EnableUnicastReceive      = TRUE;
  MnpConfigData.EnableMulticastReceive    = TRUE;
  MnpConfigData.EnableBroadcastReceive    = TRUE;
  MnpConfigData.EnablePromiscuousReceive  = FALSE;
  MnpConfigData.FlushQueuesOnReset        = TRUE;
  MnpConfigData.EnableReceiveTimestamps   = FALSE;
  MnpConfigData.DisableBackgroundPolling  = FALSE;

  Status = Mnp->Configure(Mnp, &MnpConfigData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Create a DHCP4 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiDhcp4ServiceBindingProtocolGuid,
             &Dhcp4Handle
             );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->OpenProtocol (
                  Dhcp4Handle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Dhcp4,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Get Ip4Config2 instance info.
  //
  Status = gBS->HandleProtocol (Controller, &gEfiIp4Config2ProtocolGuid, (VOID **) &Ip4Config2);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Ip4Config2->GetData (Ip4Config2, Ip4Config2DataTypeInterfaceInfo, &DataSize, Data);
  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    goto ON_EXIT;
  }

  Data = AllocateZeroPool (DataSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = Ip4Config2->GetData (Ip4Config2, Ip4Config2DataTypeInterfaceInfo, &DataSize, Data);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  InterfaceInfo = (EFI_IP4_CONFIG2_INTERFACE_INFO *)Data;

  //
  // Build required Token.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DhcpCommonNotify,
                  &IsDone,
                  &Token.CompletionEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  SetMem (&Token.RemoteAddress, sizeof (EFI_IPv4_ADDRESS), 0xff);

  Token.RemotePort = 67;

  Token.ListenPointCount = 1;

  Token.ListenPoints = AllocateZeroPool (Token.ListenPointCount * sizeof (EFI_DHCP4_LISTEN_POINT));
  if (Token.ListenPoints == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  if (Instance->Dns4CfgData.UseDefaultSetting) {
    CopyMem (&(Token.ListenPoints[0].ListenAddress), &(InterfaceInfo->StationAddress), sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&(Token.ListenPoints[0].SubnetMask), &(InterfaceInfo->SubnetMask), sizeof (EFI_IPv4_ADDRESS));
  } else {
    CopyMem (&(Token.ListenPoints[0].ListenAddress), &(Instance->Dns4CfgData.StationIp), sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&(Token.ListenPoints[0].SubnetMask), &(Instance->Dns4CfgData.SubnetMask), sizeof (EFI_IPv4_ADDRESS));
  }

  Token.ListenPoints[0].ListenPort = 68;

  Token.TimeoutValue = DNS_TIME_TO_GETMAP;

  DnsInitSeedPacket (&SeedPacket, InterfaceInfo);

  ParaList[0] = AllocateZeroPool (sizeof (EFI_DHCP4_PACKET_OPTION));
  if (ParaList[0] == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  ParaList[0]->OpCode  = DHCP4_TAG_TYPE;
  ParaList[0]->Length  = 1;
  ParaList[0]->Data[0] = DHCP4_MSG_REQUEST;

  ParaList[1] = AllocateZeroPool (sizeof (EFI_DHCP4_PACKET_OPTION));
  if (ParaList[1] == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  ParaList[1]->OpCode  = DHCP4_TAG_PARA_LIST;
  ParaList[1]->Length  = 1;
  ParaList[1]->Data[0] = DHCP4_TAG_DNS_SERVER;

  Status = Dhcp4->Build (Dhcp4, &SeedPacket, 0, NULL, 2, ParaList, &Token.Packet);

  Token.Packet->Dhcp4.Header.Xid      = HTONL(NET_RANDOM (NetRandomInitSeed ()));

  Token.Packet->Dhcp4.Header.Reserved = HTONS ((UINT16)0x8000);

  if (Instance->Dns4CfgData.UseDefaultSetting) {
    CopyMem (&(Token.Packet->Dhcp4.Header.ClientAddr), &(InterfaceInfo->StationAddress), sizeof (EFI_IPv4_ADDRESS));
  } else {
    CopyMem (&(Token.Packet->Dhcp4.Header.ClientAddr), &(Instance->Dns4CfgData.StationIp), sizeof (EFI_IPv4_ADDRESS));
  }

  CopyMem (Token.Packet->Dhcp4.Header.ClientHwAddr, &(InterfaceInfo->HwAddress), InterfaceInfo->HwAddressSize);

  Token.Packet->Dhcp4.Header.HwAddrLen = (UINT8)(InterfaceInfo->HwAddressSize);

  //
  // TransmitReceive Token
  //
  Status = Dhcp4->TransmitReceive (Dhcp4, &Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Poll the packet
  //
  do {
    Status = Mnp->Poll (Mnp);
  } while (!IsDone);

  //
  // Parse the ACK to get required information if received done.
  //
  if (IsDone && !EFI_ERROR (Token.Status)) {
    for (Index = 0; Index < Token.ResponseCount; Index++) {
      Status = ParseDhcp4Ack (Dhcp4, &Token.ResponseList[Index], &DnsServerInfor);
      if (!EFI_ERROR (Status)) {
        break;
      }
    }

    *DnsServerList = DnsServerInfor.ServerList;
  } else {
    Status = Token.Status;
  }

ON_EXIT:

  if (Data != NULL) {
    FreePool (Data);
  }

  for (Index = 0; Index < 2; Index++) {
    if (ParaList[Index] != NULL) {
      FreePool (ParaList[Index]);
    }
  }

  if (Token.ListenPoints) {
    FreePool (Token.ListenPoints);
  }

  if (Token.Packet) {
    FreePool (Token.Packet);
  }

  if (Token.ResponseList != NULL) {
    FreePool (Token.ResponseList);
  }

  if (Token.CompletionEvent != NULL) {
    gBS->CloseEvent (Token.CompletionEvent);
  }

  if (Dhcp4 != NULL) {
    Dhcp4->Stop (Dhcp4);
    Dhcp4->Configure (Dhcp4, NULL);

    gBS->CloseProtocol (
           Dhcp4Handle,
           &gEfiDhcp4ProtocolGuid,
           Image,
           Controller
           );
  }

  if (Dhcp4Handle != NULL) {
    NetLibDestroyServiceChild (
      Controller,
      Image,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Dhcp4Handle
      );
  }

  if (Mnp != NULL) {
    Mnp->Configure (Mnp, NULL);

    gBS->CloseProtocol (
           MnpChildHandle,
           &gEfiManagedNetworkProtocolGuid,
           Image,
           Controller
           );
  }

  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiManagedNetworkServiceBindingProtocolGuid,
    MnpChildHandle
    );

  return Status;
}

/**
  Parse the DHCP ACK to get Dns6 server information.

  @param  Image            The handle of the driver image.
  @param  Controller       The handle of the controller.
  @param  DnsServerCount   Retrieved Dns6 server Ip count.
  @param  DnsServerList    Retrieved Dns6 server Ip list.

  @retval EFI_SUCCESS           The Dns6 information is got from the DHCP ACK.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
GetDns6ServerFromDhcp6 (
  IN  EFI_HANDLE                 Image,
  IN  EFI_HANDLE                 Controller,
  OUT UINT32                     *DnsServerCount,
  OUT EFI_IPv6_ADDRESS           **DnsServerList
  )
{
  EFI_HANDLE                Dhcp6Handle;
  EFI_DHCP6_PROTOCOL        *Dhcp6;
  EFI_STATUS                Status;
  EFI_STATUS                TimerStatus;
  EFI_DHCP6_PACKET_OPTION   *Oro;
  EFI_DHCP6_RETRANSMISSION  InfoReqReXmit;
  EFI_EVENT                 Timer;
  EFI_STATUS                MediaStatus;
  DNS6_SERVER_INFOR         DnsServerInfor;

  Dhcp6Handle = NULL;
  Dhcp6       = NULL;
  Oro         = NULL;
  Timer       = NULL;

  ZeroMem (&DnsServerInfor, sizeof (DNS6_SERVER_INFOR));

  DnsServerInfor.ServerCount = DnsServerCount;

  //
  // Check media status before doing DHCP.
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Controller, DNS_CHECK_MEDIA_GET_DHCP_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    return EFI_NO_MEDIA;
  }

  //
  // Create a DHCP6 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiDhcp6ServiceBindingProtocolGuid,
             &Dhcp6Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Dhcp6Handle,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **) &Dhcp6,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Oro = AllocateZeroPool (sizeof (EFI_DHCP6_PACKET_OPTION) + 1);
  if (Oro == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Ask the server to reply with DNS options.
  // All members in EFI_DHCP6_PACKET_OPTION are in network order.
  //
  Oro->OpCode  = HTONS (DHCP6_TAG_DNS_REQUEST);
  Oro->OpLen   = HTONS (2);
  Oro->Data[1] = DHCP6_TAG_DNS_SERVER;

  InfoReqReXmit.Irt = 4;
  InfoReqReXmit.Mrc = 1;
  InfoReqReXmit.Mrt = 10;
  InfoReqReXmit.Mrd = 30;

  Status = Dhcp6->InfoRequest (
                    Dhcp6,
                    TRUE,
                    Oro,
                    0,
                    NULL,
                    &InfoReqReXmit,
                    NULL,
                    ParseDhcp6Ack,
                    &DnsServerInfor
                    );
  if (Status == EFI_NO_MAPPING) {
    Status = gBS->CreateEvent (EVT_TIMER, TPL_CALLBACK, NULL, NULL, &Timer);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Status = gBS->SetTimer (
                    Timer,
                    TimerRelative,
                    DNS_TIME_TO_GETMAP * TICKS_PER_SECOND
                    );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    do {
      TimerStatus = gBS->CheckEvent (Timer);
      if (!EFI_ERROR (TimerStatus)) {
        Status = Dhcp6->InfoRequest (
                          Dhcp6,
                          TRUE,
                          Oro,
                          0,
                          NULL,
                          &InfoReqReXmit,
                          NULL,
                          ParseDhcp6Ack,
                          &DnsServerInfor
                          );
      }
    } while (TimerStatus == EFI_NOT_READY);
  }

  *DnsServerList  = DnsServerInfor.ServerList;

ON_EXIT:

  if (Oro != NULL) {
    FreePool (Oro);
  }

  if (Timer != NULL) {
    gBS->CloseEvent (Timer);
  }

  if (Dhcp6 != NULL) {
    gBS->CloseProtocol (
           Dhcp6Handle,
           &gEfiDhcp6ProtocolGuid,
           Image,
           Controller
           );
  }

  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiDhcp6ServiceBindingProtocolGuid,
    Dhcp6Handle
    );

  return Status;

}

