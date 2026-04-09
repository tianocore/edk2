/** @file
  Support functions implementation for UefiPxeBc Driver.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PxeBcImpl.h"

/**
  Flush the previous configuration using the new station Ip address.

  @param[in]   Private        The pointer to the PxeBc private data.
  @param[in]   StationIp      The pointer to the station Ip address.
  @param[in]   SubnetMask     The pointer to the subnet mask address for v4.

  @retval EFI_SUCCESS         Successfully flushed the previous configuration.
  @retval Others              Failed to flush using the new station Ip.

**/
EFI_STATUS
PxeBcFlushStationIp (
  PXEBC_PRIVATE_DATA  *Private,
  EFI_IP_ADDRESS      *StationIp      OPTIONAL,
  EFI_IP_ADDRESS      *SubnetMask     OPTIONAL
  )
{
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;
  EFI_ARP_CONFIG_DATA     ArpConfigData;

  Mode   = Private->PxeBc.Mode;
  Status = EFI_SUCCESS;
  ZeroMem (&ArpConfigData, sizeof (EFI_ARP_CONFIG_DATA));

  if (Mode->UsingIpv6 && (StationIp != NULL)) {
    //
    // Overwrite Udp6CfgData/Ip6CfgData StationAddress.
    //
    CopyMem (&Private->Udp6CfgData.StationAddress, StationIp, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Private->Ip6CfgData.StationAddress, StationIp, sizeof (EFI_IPv6_ADDRESS));

    //
    // Reconfigure the Ip6 instance to capture background ICMP6 packets with new station Ip address.
    //
    Private->Ip6->Cancel (Private->Ip6, &Private->Icmp6Token);
    Private->Ip6->Configure (Private->Ip6, NULL);

    Status = Private->Ip6->Configure (Private->Ip6, &Private->Ip6CfgData);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Status = Private->Ip6->Receive (Private->Ip6, &Private->Icmp6Token);
  } else {
    if (StationIp != NULL) {
      //
      // Reconfigure the ARP instance with station Ip address.
      //
      ArpConfigData.SwAddressType   = 0x0800;
      ArpConfigData.SwAddressLength = (UINT8)sizeof (EFI_IPv4_ADDRESS);
      ArpConfigData.StationAddress  = StationIp;

      Private->Arp->Configure (Private->Arp, NULL);
      Private->Arp->Configure (Private->Arp, &ArpConfigData);

      //
      // Overwrite Udp4CfgData/Ip4CfgData StationAddress.
      //
      CopyMem (&Private->Udp4CfgData.StationAddress, StationIp, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Private->Ip4CfgData.StationAddress, StationIp, sizeof (EFI_IPv4_ADDRESS));
    }

    if (SubnetMask != NULL) {
      //
      // Overwrite Udp4CfgData/Ip4CfgData SubnetMask.
      //
      CopyMem (&Private->Udp4CfgData.SubnetMask, SubnetMask, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Private->Ip4CfgData.SubnetMask, SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    }

    if ((StationIp != NULL) && (SubnetMask != NULL)) {
      //
      // Updated the route table.
      //
      Mode->RouteTableEntries                = 1;
      Mode->RouteTable[0].IpAddr.Addr[0]     = StationIp->Addr[0] & SubnetMask->Addr[0];
      Mode->RouteTable[0].SubnetMask.Addr[0] = SubnetMask->Addr[0];
      Mode->RouteTable[0].GwAddr.Addr[0]     = 0;
    }

    if ((StationIp != NULL) || (SubnetMask != NULL)) {
      //
      // Reconfigure the Ip4 instance to capture background ICMP packets with new station Ip address.
      //
      Private->Ip4->Cancel (Private->Ip4, &Private->IcmpToken);
      Private->Ip4->Configure (Private->Ip4, NULL);

      Status = Private->Ip4->Configure (Private->Ip4, &Private->Ip4CfgData);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      Status = Private->Ip4->Receive (Private->Ip4, &Private->IcmpToken);
    }
  }

ON_EXIT:
  return Status;
}

/**
  Notify the callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
PxeBcCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  *((BOOLEAN *)Context) = TRUE;
}

/**
  Do arp resolution from arp cache in PxeBcMode.

  @param  Mode           The pointer to EFI_PXE_BASE_CODE_MODE.
  @param  Ip4Addr        The Ip4 address for resolution.
  @param  MacAddress     The resolved MAC address if the resolution is successful.
                         The value is undefined if the resolution fails.

  @retval TRUE           Found an matched entry.
  @retval FALSE          Did not find a matched entry.

**/
BOOLEAN
PxeBcCheckArpCache (
  IN  EFI_PXE_BASE_CODE_MODE  *Mode,
  IN  EFI_IPv4_ADDRESS        *Ip4Addr,
  OUT EFI_MAC_ADDRESS         *MacAddress
  )
{
  UINT32  Index;

  ASSERT (!Mode->UsingIpv6);

  //
  // Check whether the current Arp cache in mode data contains this information or not.
  //
  for (Index = 0; Index < Mode->ArpCacheEntries; Index++) {
    if (EFI_IP4_EQUAL (&Mode->ArpCache[Index].IpAddr.v4, Ip4Addr)) {
      CopyMem (
        MacAddress,
        &Mode->ArpCache[Index].MacAddr,
        sizeof (EFI_MAC_ADDRESS)
        );
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Update the arp cache periodically.

  @param  Event              The pointer to EFI_PXE_BC_PROTOCOL.
  @param  Context            Context of the timer event.

**/
VOID
EFIAPI
PxeBcArpCacheUpdate (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_ARP_FIND_DATA       *ArpEntry;
  UINT32                  EntryLength;
  UINT32                  EntryCount;
  UINT32                  Index;
  EFI_STATUS              Status;

  Private = (PXEBC_PRIVATE_DATA *)Context;
  Mode    = Private->PxeBc.Mode;

  ASSERT (!Mode->UsingIpv6);

  //
  // Get the current Arp cache from Arp driver.
  //
  Status = Private->Arp->Find (
                           Private->Arp,
                           TRUE,
                           NULL,
                           &EntryLength,
                           &EntryCount,
                           &ArpEntry,
                           TRUE
                           );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Update the Arp cache in mode data.
  //
  Mode->ArpCacheEntries = MIN (EntryCount, EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES);

  for (Index = 0; Index < Mode->ArpCacheEntries; Index++) {
    CopyMem (
      &Mode->ArpCache[Index].IpAddr,
      ArpEntry + 1,
      ArpEntry->SwAddressLength
      );
    CopyMem (
      &Mode->ArpCache[Index].MacAddr,
      (UINT8 *)(ArpEntry + 1) + ArpEntry->SwAddressLength,
      ArpEntry->HwAddressLength
      );
    ArpEntry = (EFI_ARP_FIND_DATA *)((UINT8 *)ArpEntry + EntryLength);
  }
}

/**
  Notify function to handle the received ICMP message in DPC.

  @param  Context               The PXEBC private data.

**/
VOID
EFIAPI
PxeBcIcmpErrorDpcHandle (
  IN VOID  *Context
  )
{
  EFI_STATUS              Status;
  EFI_IP4_RECEIVE_DATA    *RxData;
  EFI_IP4_PROTOCOL        *Ip4;
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  UINT8                   Type;
  UINTN                   Index;
  UINT32                  CopiedLen;
  UINT8                   *IcmpError;

  Private = (PXEBC_PRIVATE_DATA *)Context;
  Mode    = &Private->Mode;
  Status  = Private->IcmpToken.Status;
  RxData  = Private->IcmpToken.Packet.RxData;
  Ip4     = Private->Ip4;

  ASSERT (!Mode->UsingIpv6);

  if (Status == EFI_ABORTED) {
    //
    // It's triggered by user cancellation.
    //
    return;
  }

  if (RxData == NULL) {
    goto ON_EXIT;
  }

  if (Status != EFI_ICMP_ERROR) {
    //
    // The return status should be recognized as EFI_ICMP_ERROR.
    //
    goto ON_RECYCLE;
  }

  if ((EFI_IP4 (RxData->Header->SourceAddress) != 0) &&
      (NTOHL (Mode->SubnetMask.Addr[0]) != 0) &&
      IP4_NET_EQUAL (NTOHL (Mode->StationIp.Addr[0]), EFI_NTOHL (RxData->Header->SourceAddress), NTOHL (Mode->SubnetMask.Addr[0])) &&
      !NetIp4IsUnicast (EFI_NTOHL (RxData->Header->SourceAddress), NTOHL (Mode->SubnetMask.Addr[0])))
  {
    //
    // The source address of the received packet should be a valid unicast address.
    //
    goto ON_RECYCLE;
  }

  if (!EFI_IP4_EQUAL (&RxData->Header->DestinationAddress, &Mode->StationIp.v4)) {
    //
    // The destination address of the received packet should be equal to the host address.
    //
    goto ON_RECYCLE;
  }

  //
  // The protocol has been configured to only receive ICMP packet.
  //
  ASSERT (RxData->Header->Protocol == EFI_IP_PROTO_ICMP);

  Type = *((UINT8 *)RxData->FragmentTable[0].FragmentBuffer);

  if ((Type != ICMP_DEST_UNREACHABLE) &&
      (Type != ICMP_SOURCE_QUENCH) &&
      (Type != ICMP_REDIRECT) &&
      (Type != ICMP_TIME_EXCEEDED) &&
      (Type != ICMP_PARAMETER_PROBLEM))
  {
    //
    // The type of the receveid ICMP message should be ICMP_ERROR_MESSAGE.
    //
    goto ON_RECYCLE;
  }

  //
  // Copy the right ICMP error message into mode data.
  //
  CopiedLen = 0;
  IcmpError = (UINT8 *)&Mode->IcmpError;

  for (Index = 0; Index < RxData->FragmentCount; Index++) {
    CopiedLen += RxData->FragmentTable[Index].FragmentLength;
    if (CopiedLen <= sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)) {
      CopyMem (
        IcmpError,
        RxData->FragmentTable[Index].FragmentBuffer,
        RxData->FragmentTable[Index].FragmentLength
        );
    } else {
      CopyMem (
        IcmpError,
        RxData->FragmentTable[Index].FragmentBuffer,
        CopiedLen - sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)
        );
    }

    IcmpError += CopiedLen;
  }

ON_RECYCLE:
  gBS->SignalEvent (RxData->RecycleSignal);

ON_EXIT:
  Private->IcmpToken.Status = EFI_NOT_READY;
  Ip4->Receive (Ip4, &Private->IcmpToken);
}

/**
  Callback function to update the latest ICMP6 error message.

  @param  Event                 The event signalled.
  @param  Context               The context passed in using the event notifier.

**/
VOID
EFIAPI
PxeBcIcmpErrorUpdate (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  QueueDpc (TPL_CALLBACK, PxeBcIcmpErrorDpcHandle, Context);
}

/**
  Notify function to handle the received ICMP6 message in DPC.

  @param  Context               The PXEBC private data.

**/
VOID
EFIAPI
PxeBcIcmp6ErrorDpcHandle (
  IN VOID  *Context
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_IP6_RECEIVE_DATA    *RxData;
  EFI_IP6_PROTOCOL        *Ip6;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT8                   Type;
  UINT32                  CopiedLen;
  UINT8                   *Icmp6Error;

  Private = (PXEBC_PRIVATE_DATA *)Context;
  Mode    = &Private->Mode;
  Status  = Private->Icmp6Token.Status;
  RxData  = Private->Icmp6Token.Packet.RxData;
  Ip6     = Private->Ip6;

  ASSERT (Mode->UsingIpv6);

  if (Status == EFI_ABORTED) {
    //
    // It's triggered by user cancellation.
    //
    return;
  }

  if (RxData == NULL) {
    goto ON_EXIT;
  }

  if (Status != EFI_ICMP_ERROR) {
    //
    // The return status should be recognized as EFI_ICMP_ERROR.
    //
    goto ON_RECYCLE;
  }

  if (!NetIp6IsValidUnicast (&RxData->Header->SourceAddress)) {
    //
    // The source address of the received packet should be a valid unicast address.
    //
    goto ON_RECYCLE;
  }

  if (!NetIp6IsUnspecifiedAddr (&Mode->StationIp.v6) &&
      !EFI_IP6_EQUAL (&RxData->Header->DestinationAddress, &Mode->StationIp.v6))
  {
    //
    // The destination address of the received packet should be equal to the host address.
    //
    goto ON_RECYCLE;
  }

  //
  // The protocol has been configured to only receive ICMP packet.
  //
  ASSERT (RxData->Header->NextHeader == IP6_ICMP);

  Type = *((UINT8 *)RxData->FragmentTable[0].FragmentBuffer);

  if ((Type != ICMP_V6_DEST_UNREACHABLE) &&
      (Type != ICMP_V6_PACKET_TOO_BIG) &&
      (Type != ICMP_V6_TIME_EXCEEDED) &&
      (Type != ICMP_V6_PARAMETER_PROBLEM))
  {
    //
    // The type of the receveid packet should be an ICMP6 error message.
    //
    goto ON_RECYCLE;
  }

  //
  // Copy the right ICMP6 error message into mode data.
  //
  CopiedLen  = 0;
  Icmp6Error = (UINT8 *)&Mode->IcmpError;

  for (Index = 0; Index < RxData->FragmentCount; Index++) {
    CopiedLen += RxData->FragmentTable[Index].FragmentLength;
    if (CopiedLen <= sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)) {
      CopyMem (
        Icmp6Error,
        RxData->FragmentTable[Index].FragmentBuffer,
        RxData->FragmentTable[Index].FragmentLength
        );
    } else {
      CopyMem (
        Icmp6Error,
        RxData->FragmentTable[Index].FragmentBuffer,
        CopiedLen - sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)
        );
    }

    Icmp6Error += CopiedLen;
  }

ON_RECYCLE:
  gBS->SignalEvent (RxData->RecycleSignal);

ON_EXIT:
  Private->Icmp6Token.Status = EFI_NOT_READY;
  Ip6->Receive (Ip6, &Private->Icmp6Token);
}

/**
  Callback function to update the latest ICMP6 error message.

  @param  Event                 The event signalled.
  @param  Context               The context passed in using the event notifier.

**/
VOID
EFIAPI
PxeBcIcmp6ErrorUpdate (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  QueueDpc (TPL_CALLBACK, PxeBcIcmp6ErrorDpcHandle, Context);
}

/**
  This function is to configure a UDPv4 instance for UdpWrite.

  @param[in]       Udp4                 The pointer to EFI_UDP4_PROTOCOL.
  @param[in]       StationIp            The pointer to the station address.
  @param[in]       SubnetMask           The pointer to the subnet mask.
  @param[in]       Gateway              The pointer to the gateway address.
  @param[in, out]  SrcPort              The pointer to the source port.
  @param[in]       DoNotFragment        If TRUE, fragment is not enabled.
                                        Otherwise, fragment is enabled.
  @param[in]       Ttl                  The time to live field of the IP header.
  @param[in]       ToS                  The type of service field of the IP header.

  @retval          EFI_SUCCESS          Successfully configured this instance.
  @retval          Others               Failed to configure this instance.

**/
EFI_STATUS
PxeBcConfigUdp4Write (
  IN     EFI_UDP4_PROTOCOL  *Udp4,
  IN     EFI_IPv4_ADDRESS   *StationIp,
  IN     EFI_IPv4_ADDRESS   *SubnetMask,
  IN     EFI_IPv4_ADDRESS   *Gateway,
  IN OUT UINT16             *SrcPort,
  IN     BOOLEAN            DoNotFragment,
  IN     UINT8              Ttl,
  IN     UINT8              ToS
  )
{
  EFI_UDP4_CONFIG_DATA  Udp4CfgData;
  EFI_STATUS            Status;

  ZeroMem (&Udp4CfgData, sizeof (Udp4CfgData));

  Udp4CfgData.TransmitTimeout    = PXEBC_DEFAULT_LIFETIME;
  Udp4CfgData.ReceiveTimeout     = PXEBC_DEFAULT_LIFETIME;
  Udp4CfgData.TypeOfService      = ToS;
  Udp4CfgData.TimeToLive         = Ttl;
  Udp4CfgData.AllowDuplicatePort = TRUE;
  Udp4CfgData.DoNotFragment      = DoNotFragment;

  CopyMem (&Udp4CfgData.StationAddress, StationIp, sizeof (*StationIp));
  CopyMem (&Udp4CfgData.SubnetMask, SubnetMask, sizeof (*SubnetMask));

  Udp4CfgData.StationPort = *SrcPort;

  //
  // Reset the UDPv4 instance.
  //
  Udp4->Configure (Udp4, NULL);

  Status = Udp4->Configure (Udp4, &Udp4CfgData);
  if (!EFI_ERROR (Status) && !EFI_IP4_EQUAL (Gateway, &mZeroIp4Addr)) {
    //
    // The basic configuration is OK, need to add the default route entry
    //
    Status = Udp4->Routes (Udp4, FALSE, &mZeroIp4Addr, &mZeroIp4Addr, Gateway);
    if (EFI_ERROR (Status)) {
      Udp4->Configure (Udp4, NULL);
    }
  }

  if (!EFI_ERROR (Status) && (*SrcPort == 0)) {
    Udp4->GetModeData (Udp4, &Udp4CfgData, NULL, NULL, NULL);
    *SrcPort = Udp4CfgData.StationPort;
  }

  return Status;
}

/**
  This function is to configure a UDPv6 instance for UdpWrite.

  @param[in]       Udp6                 The pointer to EFI_UDP6_PROTOCOL.
  @param[in]       StationIp            The pointer to the station address.
  @param[in, out]  SrcPort              The pointer to the source port.

  @retval          EFI_SUCCESS          Successfully configured this instance.
  @retval          Others               Failed to configure this instance.

**/
EFI_STATUS
PxeBcConfigUdp6Write (
  IN     EFI_UDP6_PROTOCOL  *Udp6,
  IN     EFI_IPv6_ADDRESS   *StationIp,
  IN OUT UINT16             *SrcPort
  )
{
  EFI_UDP6_CONFIG_DATA  CfgData;
  EFI_STATUS            Status;

  ZeroMem (&CfgData, sizeof (EFI_UDP6_CONFIG_DATA));

  CfgData.ReceiveTimeout     = PXEBC_DEFAULT_LIFETIME;
  CfgData.TransmitTimeout    = PXEBC_DEFAULT_LIFETIME;
  CfgData.HopLimit           = PXEBC_DEFAULT_HOPLIMIT;
  CfgData.AllowDuplicatePort = TRUE;
  CfgData.StationPort        = *SrcPort;

  CopyMem (&CfgData.StationAddress, StationIp, sizeof (EFI_IPv6_ADDRESS));

  //
  // Reset the UDPv6 instance.
  //
  Udp6->Configure (Udp6, NULL);

  Status = Udp6->Configure (Udp6, &CfgData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!EFI_ERROR (Status) && (*SrcPort == 0)) {
    Udp6->GetModeData (Udp6, &CfgData, NULL, NULL, NULL);
    *SrcPort = CfgData.StationPort;
  }

  return Status;
}

/**
  This function is to configure a UDPv4 instance for UdpWrite.

  @param[in]       Udp4                 The pointer to EFI_UDP4_PROTOCOL.
  @param[in]       Session              The pointer to the UDP4 session data.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       Gateway              The pointer to the gateway address.
  @param[in]       HeaderSize           An optional field which may be set to the length of a header
                                        at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]       HeaderPtr            If HeaderSize is not NULL, a pointer to a header to be
                                        prefixed to the data at BufferPtr.
  @param[in]       BufferSize           A pointer to the size of the data at BufferPtr.
  @param[in]       BufferPtr            A pointer to the data to be written.

  @retval          EFI_SUCCESS          Successfully send out data using Udp4Write.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp4Write (
  IN EFI_UDP4_PROTOCOL      *Udp4,
  IN EFI_UDP4_SESSION_DATA  *Session,
  IN EFI_EVENT              TimeoutEvent,
  IN EFI_IPv4_ADDRESS       *Gateway      OPTIONAL,
  IN UINTN                  *HeaderSize   OPTIONAL,
  IN VOID                   *HeaderPtr    OPTIONAL,
  IN UINTN                  *BufferSize,
  IN VOID                   *BufferPtr
  )
{
  EFI_UDP4_COMPLETION_TOKEN  Token;
  EFI_UDP4_TRANSMIT_DATA     *TxData;
  UINT32                     TxLength;
  UINT32                     FragCount;
  UINT32                     DataLength;
  BOOLEAN                    IsDone;
  EFI_STATUS                 Status;

  //
  // Arrange one fragment buffer for data, and another fragment buffer for header if has.
  //
  FragCount = (HeaderSize != NULL) ? 2 : 1;
  TxLength  = sizeof (EFI_UDP4_TRANSMIT_DATA) + (FragCount - 1) * sizeof (EFI_UDP4_FRAGMENT_DATA);
  TxData    = (EFI_UDP4_TRANSMIT_DATA *)AllocateZeroPool (TxLength);
  if (TxData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxData->FragmentCount                               = FragCount;
  TxData->FragmentTable[FragCount - 1].FragmentLength = (UINT32)*BufferSize;
  TxData->FragmentTable[FragCount - 1].FragmentBuffer = BufferPtr;
  DataLength                                          = (UINT32)*BufferSize;

  if (HeaderSize != NULL) {
    TxData->FragmentTable[0].FragmentLength = (UINT32)*HeaderSize;
    TxData->FragmentTable[0].FragmentBuffer = HeaderPtr;
    DataLength                             += (UINT32)*HeaderSize;
  }

  if (Gateway != NULL) {
    TxData->GatewayAddress = Gateway;
  }

  TxData->UdpSessionData = Session;
  TxData->DataLength     = DataLength;
  Token.Packet.TxData    = TxData;
  Token.Status           = EFI_NOT_READY;
  IsDone                 = FALSE;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PxeBcCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Udp4->Transmit (Udp4, &Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Poll the UDPv6 read instance if no packet received and no timeout triggered.
  //
  while (!IsDone &&
         Token.Status == EFI_NOT_READY &&
         EFI_ERROR (gBS->CheckEvent (TimeoutEvent)))
  {
    Udp4->Poll (Udp4);
  }

  Status = (Token.Status == EFI_NOT_READY) ? EFI_TIMEOUT : Token.Status;

ON_EXIT:
  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }

  FreePool (TxData);

  return Status;
}

/**
  This function is to configure a UDPv4 instance for UdpWrite.

  @param[in]       Udp6                 The pointer to EFI_UDP6_PROTOCOL.
  @param[in]       Session              The pointer to the UDP6 session data.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       HeaderSize           An optional field which may be set to the length of a header
                                        at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]       HeaderPtr            If HeaderSize is not NULL, a pointer to a header to be
                                        prefixed to the data at BufferPtr.
  @param[in]       BufferSize           A pointer to the size of the data at BufferPtr.
  @param[in]       BufferPtr            A pointer to the data to be written.

  @retval          EFI_SUCCESS          Successfully sent out data using Udp6Write.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp6Write (
  IN EFI_UDP6_PROTOCOL      *Udp6,
  IN EFI_UDP6_SESSION_DATA  *Session,
  IN EFI_EVENT              TimeoutEvent,
  IN UINTN                  *HeaderSize   OPTIONAL,
  IN VOID                   *HeaderPtr    OPTIONAL,
  IN UINTN                  *BufferSize,
  IN VOID                   *BufferPtr
  )
{
  EFI_UDP6_COMPLETION_TOKEN  Token;
  EFI_UDP6_TRANSMIT_DATA     *TxData;
  UINT32                     TxLength;
  UINT32                     FragCount;
  UINT32                     DataLength;
  BOOLEAN                    IsDone;
  EFI_STATUS                 Status;

  //
  // Arrange one fragment buffer for data, and another fragment buffer for header if has.
  //
  FragCount = (HeaderSize != NULL) ? 2 : 1;
  TxLength  = sizeof (EFI_UDP6_TRANSMIT_DATA) + (FragCount - 1) * sizeof (EFI_UDP6_FRAGMENT_DATA);
  TxData    = (EFI_UDP6_TRANSMIT_DATA *)AllocateZeroPool (TxLength);
  if (TxData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxData->FragmentCount                               = FragCount;
  TxData->FragmentTable[FragCount - 1].FragmentLength = (UINT32)*BufferSize;
  TxData->FragmentTable[FragCount - 1].FragmentBuffer = BufferPtr;
  DataLength                                          = (UINT32)*BufferSize;

  if (HeaderSize != NULL) {
    TxData->FragmentTable[0].FragmentLength = (UINT32)*HeaderSize;
    TxData->FragmentTable[0].FragmentBuffer = HeaderPtr;
    DataLength                             += (UINT32)*HeaderSize;
  }

  TxData->UdpSessionData = Session;
  TxData->DataLength     = DataLength;
  Token.Packet.TxData    = TxData;
  Token.Status           = EFI_NOT_READY;
  IsDone                 = FALSE;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PxeBcCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Udp6->Transmit (Udp6, &Token);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Poll the UDPv6 read instance if no packet received and no timeout triggered.
  //
  while (!IsDone &&
         Token.Status == EFI_NOT_READY &&
         EFI_ERROR (gBS->CheckEvent (TimeoutEvent)))
  {
    Udp6->Poll (Udp6);
  }

  Status = (Token.Status == EFI_NOT_READY) ? EFI_TIMEOUT : Token.Status;

ON_EXIT:
  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }

  FreePool (TxData);

  return Status;
}

/**
  Check the received packet using the Ip filter.

  @param[in]  Mode                The pointer to the mode data of PxeBc.
  @param[in]  Session             The pointer to the current UDPv4 session.
  @param[in]  OpFlags             Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Passed the Ip filter successfully.
  @retval     FALSE               Failed to pass the Ip filter.

**/
BOOLEAN
PxeBcCheckByIpFilter (
  IN EFI_PXE_BASE_CODE_MODE  *Mode,
  IN VOID                    *Session,
  IN UINT16                  OpFlags
  )
{
  EFI_IP_ADDRESS  DestinationIp;
  UINTN           Index;

  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER) == 0) {
    return TRUE;
  }

  if ((Mode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) != 0) {
    return TRUE;
  }

  //
  // Convert the destination address in session data to host order.
  //
  if (Mode->UsingIpv6) {
    CopyMem (
      &DestinationIp,
      &((EFI_UDP6_SESSION_DATA *)Session)->DestinationAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
    NTOHLLL (&DestinationIp.v6);
  } else {
    ZeroMem (&DestinationIp, sizeof (EFI_IP_ADDRESS));
    CopyMem (
      &DestinationIp,
      &((EFI_UDP4_SESSION_DATA *)Session)->DestinationAddress,
      sizeof (EFI_IPv4_ADDRESS)
      );
    EFI_NTOHL (DestinationIp);
  }

  if (((Mode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) != 0) &&
      (IP4_IS_MULTICAST (DestinationIp.Addr[0]) ||
       IP6_IS_MULTICAST (&DestinationIp)))
  {
    return TRUE;
  }

  if (((Mode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST) != 0) &&
      IP4_IS_LOCAL_BROADCAST (DestinationIp.Addr[0]))
  {
    ASSERT (!Mode->UsingIpv6);
    return TRUE;
  }

  if (((Mode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0) &&
      (EFI_IP4_EQUAL (&Mode->StationIp.v4, &DestinationIp) ||
       EFI_IP6_EQUAL (&Mode->StationIp.v6, &DestinationIp)))
  {
    //
    // Matched if the dest address is equal to the station address.
    //
    return TRUE;
  }

  for (Index = 0; Index < Mode->IpFilter.IpCnt; Index++) {
    ASSERT (Index < EFI_PXE_BASE_CODE_MAX_IPCNT);
    if (EFI_IP4_EQUAL (&Mode->IpFilter.IpList[Index].v4, &DestinationIp) ||
        EFI_IP6_EQUAL (&Mode->IpFilter.IpList[Index].v6, &DestinationIp))
    {
      //
      // Matched if the dest address is equal to any of address in the filter list.
      //
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Filter the received packet using the destination Ip.

  @param[in]       Mode           The pointer to the mode data of PxeBc.
  @param[in]       Session        The pointer to the current UDPv4 session.
  @param[in, out]  DestIp         The pointer to the destination Ip address.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Passed the IPv4 filter successfully.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcCheckByDestIp (
  IN     EFI_PXE_BASE_CODE_MODE  *Mode,
  IN     VOID                    *Session,
  IN OUT EFI_IP_ADDRESS          *DestIp,
  IN     UINT16                  OpFlags
  )
{
  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP) != 0) {
    //
    // Copy the destination address from the received packet if accept any.
    //
    if (DestIp != NULL) {
      if (Mode->UsingIpv6) {
        CopyMem (
          DestIp,
          &((EFI_UDP6_SESSION_DATA *)Session)->DestinationAddress,
          sizeof (EFI_IPv6_ADDRESS)
          );
      } else {
        ZeroMem (DestIp, sizeof (EFI_IP_ADDRESS));
        CopyMem (
          DestIp,
          &((EFI_UDP4_SESSION_DATA *)Session)->DestinationAddress,
          sizeof (EFI_IPv4_ADDRESS)
          );
      }
    }

    return TRUE;
  } else if ((DestIp != NULL) &&
             (EFI_IP4_EQUAL (DestIp, &((EFI_UDP4_SESSION_DATA *)Session)->DestinationAddress) ||
              EFI_IP6_EQUAL (DestIp, &((EFI_UDP6_SESSION_DATA *)Session)->DestinationAddress)))
  {
    //
    // The destination address in the received packet is matched if present.
    //
    return TRUE;
  } else if (EFI_IP4_EQUAL (&Mode->StationIp, &((EFI_UDP4_SESSION_DATA *)Session)->DestinationAddress) ||
             EFI_IP6_EQUAL (&Mode->StationIp, &((EFI_UDP6_SESSION_DATA *)Session)->DestinationAddress))
  {
    //
    // The destination address in the received packet is equal to the host address.
    //
    return TRUE;
  }

  return FALSE;
}

/**
  Check the received packet using the destination port.

  @param[in]       Mode           The pointer to the mode data of PxeBc.
  @param[in]       Session        The pointer to the current UDPv4 session.
  @param[in, out]  DestPort       The pointer to the destination port.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Passed the IPv4 filter successfully.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcCheckByDestPort (
  IN     EFI_PXE_BASE_CODE_MODE  *Mode,
  IN     VOID                    *Session,
  IN OUT UINT16                  *DestPort,
  IN     UINT16                  OpFlags
  )
{
  UINT16  Port;

  if (Mode->UsingIpv6) {
    Port = ((EFI_UDP6_SESSION_DATA *)Session)->DestinationPort;
  } else {
    Port = ((EFI_UDP4_SESSION_DATA *)Session)->DestinationPort;
  }

  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT) != 0) {
    //
    // Return the destination port in the received packet if accept any.
    //
    if (DestPort != NULL) {
      *DestPort = Port;
    }

    return TRUE;
  } else if ((DestPort != NULL) && (*DestPort == Port)) {
    //
    // The destination port in the received packet is matched if present.
    //
    return TRUE;
  }

  return FALSE;
}

/**
  Filter the received packet using the source Ip.

  @param[in]       Mode           The pointer to the mode data of PxeBc.
  @param[in]       Session        The pointer to the current UDPv4 session.
  @param[in, out]  SrcIp          The pointer to the source Ip address.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Passed the IPv4 filter successfully.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcFilterBySrcIp (
  IN     EFI_PXE_BASE_CODE_MODE  *Mode,
  IN     VOID                    *Session,
  IN OUT EFI_IP_ADDRESS          *SrcIp,
  IN     UINT16                  OpFlags
  )
{
  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP) != 0) {
    //
    // Copy the source address from the received packet if accept any.
    //
    if (SrcIp != NULL) {
      if (Mode->UsingIpv6) {
        CopyMem (
          SrcIp,
          &((EFI_UDP6_SESSION_DATA *)Session)->SourceAddress,
          sizeof (EFI_IPv6_ADDRESS)
          );
      } else {
        ZeroMem (SrcIp, sizeof (EFI_IP_ADDRESS));
        CopyMem (
          SrcIp,
          &((EFI_UDP4_SESSION_DATA *)Session)->SourceAddress,
          sizeof (EFI_IPv4_ADDRESS)
          );
      }
    }

    return TRUE;
  } else if ((SrcIp != NULL) &&
             (EFI_IP4_EQUAL (SrcIp, &((EFI_UDP4_SESSION_DATA *)Session)->SourceAddress) ||
              EFI_IP6_EQUAL (SrcIp, &((EFI_UDP6_SESSION_DATA *)Session)->SourceAddress)))
  {
    //
    // The source address in the received packet is matched if present.
    //
    return TRUE;
  }

  return FALSE;
}

/**
  Filter the received packet using the source port.

  @param[in]       Mode           The pointer to the mode data of PxeBc.
  @param[in]       Session        The pointer to the current UDPv4 session.
  @param[in, out]  SrcPort        The pointer to the source port.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Passed the IPv4 filter successfully.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcFilterBySrcPort (
  IN     EFI_PXE_BASE_CODE_MODE  *Mode,
  IN     VOID                    *Session,
  IN OUT UINT16                  *SrcPort,
  IN     UINT16                  OpFlags
  )
{
  UINT16  Port;

  if (Mode->UsingIpv6) {
    Port = ((EFI_UDP6_SESSION_DATA *)Session)->SourcePort;
  } else {
    Port = ((EFI_UDP4_SESSION_DATA *)Session)->SourcePort;
  }

  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT) != 0) {
    //
    // Return the source port in the received packet if accept any.
    //
    if (SrcPort != NULL) {
      *SrcPort = Port;
    }

    return TRUE;
  } else if ((SrcPort != NULL) && (*SrcPort == Port)) {
    //
    // The source port in the received packet is matched if present.
    //
    return TRUE;
  }

  return FALSE;
}

/**
  This function is to receive packet using Udp4Read.

  @param[in]       Udp4                 The pointer to EFI_UDP4_PROTOCOL.
  @param[in]       Token                The pointer to EFI_UDP4_COMPLETION_TOKEN.
  @param[in]       Mode                 The pointer to EFI_PXE_BASE_CODE_MODE.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       OpFlags              The UDP operation flags.
  @param[in]       IsDone               The pointer to the IsDone flag.
  @param[out]      IsMatched            The pointer to the IsMatched flag.
  @param[in, out]  DestIp               The pointer to the destination address.
  @param[in, out]  DestPort             The pointer to the destination port.
  @param[in, out]  SrcIp                The pointer to the source address.
  @param[in, out]  SrcPort              The pointer to the source port.

  @retval          EFI_SUCCESS          Successfully read the data using Udp4.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp4Read (
  IN     EFI_UDP4_PROTOCOL           *Udp4,
  IN     EFI_UDP4_COMPLETION_TOKEN   *Token,
  IN     EFI_PXE_BASE_CODE_MODE      *Mode,
  IN     EFI_EVENT                   TimeoutEvent,
  IN     UINT16                      OpFlags,
  IN     BOOLEAN                     *IsDone,
  OUT BOOLEAN                        *IsMatched,
  IN OUT EFI_IP_ADDRESS              *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS              *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort     OPTIONAL
  )
{
  EFI_UDP4_RECEIVE_DATA  *RxData;
  EFI_UDP4_SESSION_DATA  *Session;
  EFI_STATUS             Status;

  Token->Status = EFI_NOT_READY;
  *IsDone       = FALSE;

  Status = Udp4->Receive (Udp4, Token);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll the UDPv6 read instance if no packet received and no timeout triggered.
  //
  while (!(*IsDone) &&
         Token->Status == EFI_NOT_READY &&
         EFI_ERROR (gBS->CheckEvent (TimeoutEvent)))
  {
    //
    // Poll the token until reply/ICMPv6 error message received or timeout.
    //
    Udp4->Poll (Udp4);
    if ((Token->Status == EFI_ICMP_ERROR) ||
        (Token->Status == EFI_NETWORK_UNREACHABLE) ||
        (Token->Status == EFI_HOST_UNREACHABLE) ||
        (Token->Status == EFI_PROTOCOL_UNREACHABLE) ||
        (Token->Status == EFI_PORT_UNREACHABLE))
    {
      break;
    }
  }

  Status = (Token->Status == EFI_NOT_READY) ? EFI_TIMEOUT : Token->Status;

  if (!EFI_ERROR (Status)) {
    //
    // check whether this packet matches the filters
    //
    RxData  = Token->Packet.RxData;
    Session = &RxData->UdpSession;

    *IsMatched = PxeBcCheckByIpFilter (Mode, Session, OpFlags);

    if (*IsMatched) {
      *IsMatched = PxeBcCheckByDestIp (Mode, Session, DestIp, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcCheckByDestPort (Mode, Session, DestPort, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcFilterBySrcIp (Mode, Session, SrcIp, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcFilterBySrcPort (Mode, Session, SrcPort, OpFlags);
    }

    if (!(*IsMatched)) {
      //
      // Recycle the receiving buffer if not matched.
      //
      gBS->SignalEvent (RxData->RecycleSignal);
    }
  }

  return Status;
}

/**
  This function is to receive packets using Udp6Read.

  @param[in]       Udp6                 The pointer to EFI_UDP6_PROTOCOL.
  @param[in]       Token                The pointer to EFI_UDP6_COMPLETION_TOKEN.
  @param[in]       Mode                 The pointer to EFI_PXE_BASE_CODE_MODE.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       OpFlags              The UDP operation flags.
  @param[in]       IsDone               The pointer to the IsDone flag.
  @param[out]      IsMatched            The pointer to the IsMatched flag.
  @param[in, out]  DestIp               The pointer to the destination address.
  @param[in, out]  DestPort             The pointer to the destination port.
  @param[in, out]  SrcIp                The pointer to the source address.
  @param[in, out]  SrcPort              The pointer to the source port.

  @retval          EFI_SUCCESS          Successfully read data using Udp6.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp6Read (
  IN     EFI_UDP6_PROTOCOL           *Udp6,
  IN     EFI_UDP6_COMPLETION_TOKEN   *Token,
  IN     EFI_PXE_BASE_CODE_MODE      *Mode,
  IN     EFI_EVENT                   TimeoutEvent,
  IN     UINT16                      OpFlags,
  IN     BOOLEAN                     *IsDone,
  OUT BOOLEAN                        *IsMatched,
  IN OUT EFI_IP_ADDRESS              *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS              *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort     OPTIONAL
  )
{
  EFI_UDP6_RECEIVE_DATA  *RxData;
  EFI_UDP6_SESSION_DATA  *Session;
  EFI_STATUS             Status;

  Token->Status = EFI_NOT_READY;
  *IsDone       = FALSE;

  Status = Udp6->Receive (Udp6, Token);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll the UDPv6 read instance if no packet received and no timeout triggered.
  //
  while (!(*IsDone) &&
         Token->Status == EFI_NOT_READY &&
         EFI_ERROR (gBS->CheckEvent (TimeoutEvent)))
  {
    //
    // Poll the token until reply/ICMPv6 error message received or timeout.
    //
    Udp6->Poll (Udp6);
    if ((Token->Status == EFI_ICMP_ERROR) ||
        (Token->Status == EFI_NETWORK_UNREACHABLE) ||
        (Token->Status == EFI_HOST_UNREACHABLE) ||
        (Token->Status == EFI_PROTOCOL_UNREACHABLE) ||
        (Token->Status == EFI_PORT_UNREACHABLE))
    {
      break;
    }
  }

  Status = (Token->Status == EFI_NOT_READY) ? EFI_TIMEOUT : Token->Status;

  if (!EFI_ERROR (Status)) {
    //
    // check whether this packet matches the filters
    //
    RxData  = Token->Packet.RxData;
    Session = &RxData->UdpSession;

    *IsMatched = PxeBcCheckByIpFilter (Mode, Session, OpFlags);

    if (*IsMatched) {
      *IsMatched = PxeBcCheckByDestIp (Mode, Session, DestIp, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcCheckByDestPort (Mode, Session, DestPort, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcFilterBySrcIp (Mode, Session, SrcIp, OpFlags);
    }

    if (*IsMatched) {
      *IsMatched = PxeBcFilterBySrcPort (Mode, Session, SrcPort, OpFlags);
    }

    if (!(*IsMatched)) {
      //
      // Recycle the receiving buffer if not matched.
      //
      gBS->SignalEvent (RxData->RecycleSignal);
    }
  }

  return Status;
}

/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
PxeBcShowIp4Addr (
  IN EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN  Index;

  for (Index = 0; Index < 4; Index++) {
    AsciiPrint ("%d", Ip->Addr[Index]);
    if (Index < 3) {
      AsciiPrint (".");
    }
  }
}

/**
  This function is to display the IPv6 address.

  @param[in]  Ip        The pointer to the IPv6 address.

**/
VOID
PxeBcShowIp6Addr (
  IN EFI_IPv6_ADDRESS  *Ip
  )
{
  UINTN  Index;

  for (Index = 0; Index < 16; Index++) {
    if (Ip->Addr[Index] != 0) {
      AsciiPrint ("%x", Ip->Addr[Index]);
    }

    Index++;
    if (Index > 15) {
      return;
    }

    if (((Ip->Addr[Index] & 0xf0) == 0) && (Ip->Addr[Index - 1] != 0)) {
      AsciiPrint ("0");
    }

    AsciiPrint ("%x", Ip->Addr[Index]);
    if (Index < 15) {
      AsciiPrint (":");
    }
  }
}

/**
  This function is to convert UINTN to ASCII string with the required formatting.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  Length         The length of the required format.

**/
VOID
PxeBcUintnToAscDecWithFormat (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN INTN   Length
  )
{
  UINTN  Remainder;

  for ( ; Length > 0; Length--) {
    Remainder          = Number % 10;
    Number            /= 10;
    Buffer[Length - 1] = (UINT8)('0' + Remainder);
  }
}

/**
  This function is to convert a UINTN to a ASCII string, and return the
  actual length of the buffer.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  BufferSize     The maxsize of the buffer.

  @return     Length         The actual length of the ASCII string.

**/
UINTN
PxeBcUintnToAscDec (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  Index;
  UINTN  Length;
  CHAR8  TempStr[64];

  Index          = 63;
  TempStr[Index] = 0;

  do {
    Index--;
    TempStr[Index] = (CHAR8)('0' + (Number % 10));
    Number         = (UINTN)(Number / 10);
  } while (Number != 0);

  AsciiStrCpyS ((CHAR8 *)Buffer, BufferSize, &TempStr[Index]);

  Length = AsciiStrLen ((CHAR8 *)Buffer);

  return Length;
}

/**
  This function is to convert unicode hex number to a UINT8.

  @param[out]  Digit                   The converted UINT8 for output.
  @param[in]   Char                    The unicode hex number to be converted.

  @retval      EFI_SUCCESS             Successfully converted the unicode hex.
  @retval      EFI_INVALID_PARAMETER   Failed to convert the unicode hex.

**/
EFI_STATUS
PxeBcUniHexToUint8 (
  OUT UINT8   *Digit,
  IN  CHAR16  Char
  )
{
  if ((Char >= L'0') && (Char <= L'9')) {
    *Digit = (UINT8)(Char - L'0');
    return EFI_SUCCESS;
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    *Digit = (UINT8)(Char - L'A' + 0x0A);
    return EFI_SUCCESS;
  }

  if ((Char >= L'a') && (Char <= L'f')) {
    *Digit = (UINT8)(Char - L'a' + 0x0A);
    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Calculate the elapsed time.

  @param[in]      Private      The pointer to PXE private data

**/
VOID
CalcElapsedTime (
  IN     PXEBC_PRIVATE_DATA  *Private
  )
{
  EFI_TIME  Time;
  UINT64    CurrentStamp;
  UINT64    ElapsedTimeValue;

  //
  // Generate a time stamp of the centiseconds from 1900/1/1, assume 30day/month.
  //
  ZeroMem (&Time, sizeof (EFI_TIME));
  gRT->GetTime (&Time, NULL);
  CurrentStamp = MultU64x32 (
                   ((((UINT32)(Time.Year - 1900) * 360 + (Time.Month - 1) * 30 + (Time.Day - 1)) * 24 + Time.Hour) * 60 + Time.Minute) * 60 + Time.Second,
                   100
                   ) +
                 DivU64x32 (
                   Time.Nanosecond,
                   10000000
                   );

  //
  // Sentinel value of 0 means that this is the first DHCP packet that we are
  // sending and that we need to initialize the value.  First DHCP Solicit
  // gets 0 elapsed-time.  Otherwise, calculate based on StartTime.
  //
  if (Private->ElapsedTime == 0) {
    Private->ElapsedTime = CurrentStamp;
  } else {
    ElapsedTimeValue = CurrentStamp - Private->ElapsedTime;

    //
    // If elapsed time cannot fit in two bytes, set it to 0xffff.
    //
    if (ElapsedTimeValue > 0xffff) {
      ElapsedTimeValue = 0xffff;
    }

    //
    // Save the elapsed time
    //
    Private->ElapsedTime = ElapsedTimeValue;
  }
}
