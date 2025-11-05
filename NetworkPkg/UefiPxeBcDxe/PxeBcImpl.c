/** @file
  This implementation of EFI_PXE_BASE_CODE_PROTOCOL and EFI_LOAD_FILE_PROTOCOL.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PxeBcImpl.h"

/**
  Enables the use of the PXE Base Code Protocol functions.

  This function enables the use of the PXE Base Code Protocol functions. If the
  Started field of the EFI_PXE_BASE_CODE_MODE structure is already TRUE, then
  EFI_ALREADY_STARTED will be returned. If UseIpv6 is TRUE, then IPv6 formatted
  addresses will be used in this session. If UseIpv6 is FALSE, then IPv4 formatted
  addresses will be used in this session. If UseIpv6 is TRUE, and the Ipv6Supported
  field of the EFI_PXE_BASE_CODE_MODE structure is FALSE, then EFI_UNSUPPORTED will
  be returned. If there is not enough memory or other resources to start the PXE
  Base Code Protocol, then EFI_OUT_OF_RESOURCES will be returned. Otherwise, the
  PXE Base Code Protocol will be started.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  UseIpv6           Specifies the type of IP addresses that are to be
                                used during the session that is being started.
                                Set to TRUE for IPv6, and FALSE for IPv4.

  @retval EFI_SUCCESS           The PXE Base Code Protocol was started.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_UNSUPPORTED       UseIpv6 is TRUE, but the Ipv6Supported field of the
                                EFI_PXE_BASE_CODE_MODE structure is FALSE.
  @retval EFI_ALREADY_STARTED   The PXE Base Code Protocol is already in the started state.
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory or other resources to start the
                                PXE Base Code Protocol.

**/
EFI_STATUS
EFIAPI
EfiPxeBcStart (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN BOOLEAN                     UseIpv6
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  UINTN                   Index;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (Mode->Started) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Detect whether using IPv6 or not, and set it into mode data.
  //
  if (UseIpv6 && Mode->Ipv6Available && Mode->Ipv6Supported && (Private->Ip6Nic != NULL)) {
    Mode->UsingIpv6 = TRUE;
  } else if (!UseIpv6 && (Private->Ip4Nic != NULL)) {
    Mode->UsingIpv6 = FALSE;
  } else {
    return EFI_UNSUPPORTED;
  }

  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_IP_NETWORK | EFI_IOB_PC_RECONFIG,
    (VOID *)&(Mode->UsingIpv6),
    sizeof (Mode->UsingIpv6)
    );

  if (Mode->UsingIpv6) {
    AsciiPrint ("\n>>Start PXE over IPv6");
    //
    // Configure udp6 instance to receive data.
    //
    Status = Private->Udp6Read->Configure (
                                  Private->Udp6Read,
                                  &Private->Udp6CfgData
                                  );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Configure block size for TFTP as a default value to handle all link layers.
    //
    Private->BlockSize = Private->Ip6MaxPacketSize -
                         PXEBC_DEFAULT_UDP_OVERHEAD_SIZE - PXEBC_DEFAULT_TFTP_OVERHEAD_SIZE;

    //
    // PXE over IPv6 starts here, initialize the fields and list header.
    //
    Private->Ip6Policy                          = PXEBC_IP6_POLICY_MAX;
    Private->ProxyOffer.Dhcp6.Packet.Offer.Size = PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE;
    Private->DhcpAck.Dhcp6.Packet.Ack.Size      = PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE;
    Private->PxeReply.Dhcp6.Packet.Ack.Size     = PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE;

    for (Index = 0; Index < PXEBC_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp6.Packet.Offer.Size = PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE;
    }

    //
    // Create event and set status for token to capture ICMP6 error message.
    //
    Private->Icmp6Token.Status = EFI_NOT_READY;
    Status                     = gBS->CreateEvent (
                                        EVT_NOTIFY_SIGNAL,
                                        TPL_NOTIFY,
                                        PxeBcIcmp6ErrorUpdate,
                                        Private,
                                        &Private->Icmp6Token.Event
                                        );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Set Ip6 policy to Automatic to start the IP6 router discovery.
    //
    Status = PxeBcSetIp6Policy (Private);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  } else {
    AsciiPrint ("\n>>Start PXE over IPv4");
    //
    // Configure udp4 instance to receive data.
    //
    Status = Private->Udp4Read->Configure (
                                  Private->Udp4Read,
                                  &Private->Udp4CfgData
                                  );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Configure block size for TFTP as a default value to handle all link layers.
    //
    Private->BlockSize = Private->Ip4MaxPacketSize -
                         PXEBC_DEFAULT_UDP_OVERHEAD_SIZE - PXEBC_DEFAULT_TFTP_OVERHEAD_SIZE;

    //
    // PXE over IPv4 starts here, initialize the fields.
    //
    Private->ProxyOffer.Dhcp4.Packet.Offer.Size = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
    Private->DhcpAck.Dhcp4.Packet.Ack.Size      = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
    Private->PxeReply.Dhcp4.Packet.Ack.Size     = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;

    for (Index = 0; Index < PXEBC_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp4.Packet.Offer.Size = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
    }

    PxeBcSeedDhcp4Packet (&Private->SeedPacket, Private->Udp4Read);

    //
    // Create the event for Arp cache update.
    //
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    PxeBcArpCacheUpdate,
                    Private,
                    &Private->ArpUpdateEvent
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Start a periodic timer by second to update Arp cache.
    //
    Status = gBS->SetTimer (
                    Private->ArpUpdateEvent,
                    TimerPeriodic,
                    TICKS_PER_SECOND
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Create event and set status for token to capture ICMP error message.
    //
    Private->Icmp6Token.Status = EFI_NOT_READY;
    Status                     = gBS->CreateEvent (
                                        EVT_NOTIFY_SIGNAL,
                                        TPL_NOTIFY,
                                        PxeBcIcmpErrorUpdate,
                                        Private,
                                        &Private->IcmpToken.Event
                                        );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // DHCP4 service allows only one of its children to be configured in
    // the active state, If the DHCP4 D.O.R.A started by IP4 auto
    // configuration and has not been completed, the Dhcp4 state machine
    // will not be in the right state for the PXE to start a new round D.O.R.A.
    // so we need to switch its policy to static.
    //
    Status = PxeBcSetIp4Policy (Private);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // If PcdTftpBlockSize is set to non-zero, override the default value.
  //
  if (PcdGet64 (PcdTftpBlockSize) != 0) {
    Private->BlockSize = (UINTN)PcdGet64 (PcdTftpBlockSize);
  }

  //
  // Create event for UdpRead/UdpWrite timeout since they are both blocking API.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Private->UdpTimeOutEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Private->IsAddressOk = FALSE;
  Mode->Started        = TRUE;

  return EFI_SUCCESS;

ON_ERROR:
  if (Mode->UsingIpv6) {
    if (Private->Icmp6Token.Event != NULL) {
      gBS->CloseEvent (Private->Icmp6Token.Event);
      Private->Icmp6Token.Event = NULL;
    }

    Private->Udp6Read->Configure (Private->Udp6Read, NULL);
    Private->Ip6->Configure (Private->Ip6, NULL);
  } else {
    if (Private->ArpUpdateEvent != NULL) {
      gBS->CloseEvent (Private->ArpUpdateEvent);
      Private->ArpUpdateEvent = NULL;
    }

    if (Private->IcmpToken.Event != NULL) {
      gBS->CloseEvent (Private->IcmpToken.Event);
      Private->IcmpToken.Event = NULL;
    }

    Private->Udp4Read->Configure (Private->Udp4Read, NULL);
    Private->Ip4->Configure (Private->Ip4, NULL);
  }

  return Status;
}

/**
  Disable the use of the PXE Base Code Protocol functions.

  This function stops all activity on the network device. All the resources allocated
  in Start() are released, the Started field of the EFI_PXE_BASE_CODE_MODE structure is
  set to FALSE, and EFI_SUCCESS is returned. If the Started field of the EFI_PXE_BASE_CODE_MODE
  structure is already FALSE, then EFI_NOT_STARTED will be returned.

  @param[in]  This               Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.

  @retval EFI_SUCCESS           The PXE Base Code Protocol was stopped.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is already in the stopped state.
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.
  @retval Others

**/
EFI_STATUS
EFIAPI
EfiPxeBcStop (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  BOOLEAN                 Ipv6Supported;
  BOOLEAN                 Ipv6Available;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private       = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode          = Private->PxeBc.Mode;
  Ipv6Supported = Mode->Ipv6Supported;
  Ipv6Available = Mode->Ipv6Available;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (Mode->UsingIpv6) {
    //
    // Configure all the instances for IPv6 as NULL.
    //
    ZeroMem (&Private->Udp6CfgData.StationAddress, sizeof (EFI_IPv6_ADDRESS));
    ZeroMem (&Private->Ip6CfgData.StationAddress, sizeof (EFI_IPv6_ADDRESS));
    Private->Dhcp6->Stop (Private->Dhcp6);
    Private->Dhcp6->Configure (Private->Dhcp6, NULL);
    Private->Udp6Write->Configure (Private->Udp6Write, NULL);
    Private->Udp6Read->Groups (Private->Udp6Read, FALSE, NULL);
    Private->Udp6Read->Configure (Private->Udp6Read, NULL);
    Private->Ip6->Cancel (Private->Ip6, &Private->Icmp6Token);
    Private->Ip6->Configure (Private->Ip6, NULL);
    PxeBcUnregisterIp6Address (Private);
    if (Private->Icmp6Token.Event != NULL) {
      gBS->CloseEvent (Private->Icmp6Token.Event);
      Private->Icmp6Token.Event = NULL;
    }

    if (Private->Dhcp6Request != NULL) {
      FreePool (Private->Dhcp6Request);
      Private->Dhcp6Request = NULL;
    }

    if (Private->BootFileName != NULL) {
      FreePool (Private->BootFileName);
      Private->BootFileName = NULL;
    }
  } else {
    //
    // Configure all the instances for IPv4 as NULL.
    //
    ZeroMem (&Private->Udp4CfgData.StationAddress, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Private->Udp4CfgData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Private->Ip4CfgData.StationAddress, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Private->Ip4CfgData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    Private->Dhcp4->Stop (Private->Dhcp4);
    Private->Dhcp4->Configure (Private->Dhcp4, NULL);
    Private->Udp4Write->Configure (Private->Udp4Write, NULL);
    Private->Udp4Read->Groups (Private->Udp4Read, FALSE, NULL);
    Private->Udp4Read->Configure (Private->Udp4Read, NULL);
    Private->Ip4->Cancel (Private->Ip4, &Private->IcmpToken);
    Private->Ip4->Configure (Private->Ip4, NULL);
    if (Private->ArpUpdateEvent != NULL) {
      gBS->CloseEvent (Private->ArpUpdateEvent);
      Private->ArpUpdateEvent = NULL;
    }

    if (Private->IcmpToken.Event != NULL) {
      gBS->CloseEvent (Private->IcmpToken.Event);
      Private->IcmpToken.Event = NULL;
    }

    Private->BootFileName = NULL;
  }

  gBS->CloseEvent (Private->UdpTimeOutEvent);
  Private->CurSrcPort   = 0;
  Private->BootFileSize = 0;
  Private->SolicitTimes = 0;
  Private->ElapsedTime  = 0;
  ZeroMem (&Private->StationIp, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->SubnetMask, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->GatewayIp, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->ServerIp, sizeof (EFI_IP_ADDRESS));

  //
  // Reset the mode data.
  //
  ZeroMem (Mode, sizeof (EFI_PXE_BASE_CODE_MODE));
  Mode->Ipv6Available = Ipv6Available;
  Mode->Ipv6Supported = Ipv6Supported;
  Mode->AutoArp       = TRUE;
  Mode->TTL           = DEFAULT_TTL;
  Mode->ToS           = DEFAULT_ToS;

  return EFI_SUCCESS;
}

/**
  Attempts to complete a DHCPv4 D.O.R.A. (discover / offer / request / acknowledge) or DHCPv6
  S.A.R.R (solicit / advertise / request / reply) sequence.

  If SortOffers is TRUE, then the cached DHCP offer packets will be sorted before
  they are tried. If SortOffers is FALSE, then the cached DHCP offer packets will
  be tried in the order in which they are received. Please see the Preboot Execution
  Environment (PXE) Specification and Unified Extensible Firmware Interface (UEFI)
  Specification for additional details on the implementation of DHCP.
  If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then the DHCP sequence will be stopped and EFI_ABORTED will be returned.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  SortOffers        TRUE if the offers received should be sorted. Set to FALSE to
                                try the offers in the order that they are received.

  @retval EFI_SUCCESS           Valid DHCP has completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory to complete the DHCP Protocol.
  @retval EFI_ABORTED           The callback function aborted the DHCP Protocol.
  @retval EFI_TIMEOUT           The DHCP Protocol timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the DHCP session.
  @retval EFI_NO_RESPONSE       Valid PXE offer was not received.

**/
EFI_STATUS
EFIAPI
EfiPxeBcDhcp (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN BOOLEAN                     SortOffers
  )
{
  PXEBC_PRIVATE_DATA           *Private;
  EFI_PXE_BASE_CODE_MODE       *Mode;
  EFI_STATUS                   Status;
  EFI_PXE_BASE_CODE_IP_FILTER  IpFilter;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status                  = EFI_SUCCESS;
  Private                 = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode                    = Private->PxeBc.Mode;
  Mode->IcmpErrorReceived = FALSE;
  Private->Function       = EFI_PXE_BASE_CODE_FUNCTION_DHCP;
  Private->IsOfferSorted  = SortOffers;
  Private->SolicitTimes   = 0;
  Private->ElapsedTime    = 0;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (Mode->UsingIpv6) {
    //
    // Stop Udp6Read instance
    //
    Private->Udp6Read->Configure (Private->Udp6Read, NULL);

    //
    // Start S.A.R.R. process to get a IPv6 address and other boot information.
    //
    Status = PxeBcDhcp6Sarr (Private, Private->Dhcp6);
  } else {
    //
    // Stop Udp4Read instance
    //
    Private->Udp4Read->Configure (Private->Udp4Read, NULL);

    //
    // Start D.O.R.A. process to get a IPv4 address and other boot information.
    //
    Status = PxeBcDhcp4Dora (Private, Private->Dhcp4);
  }

  //
  // Reconfigure the UDP instance with the default configuration.
  //
  if (Mode->UsingIpv6) {
    Private->Udp6Read->Configure (Private->Udp6Read, &Private->Udp6CfgData);
  } else {
    Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);
  }

  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem (&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
  IpFilter.Filters = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  This->SetIpFilter (This, &IpFilter);

  return Status;
}

/**
  Attempts to complete the PXE Boot Server and/or boot image discovery sequence.

  This function attempts to complete the PXE Boot Server and/or boot image discovery
  sequence. If this sequence is completed, then EFI_SUCCESS is returned, and the
  PxeDiscoverValid, PxeDiscover, PxeReplyReceived, and PxeReply fields of the
  EFI_PXE_BASE_CODE_MODE structure are filled in. If UseBis is TRUE, then the
  PxeBisReplyReceived and PxeBisReply fields of the EFI_PXE_BASE_CODE_MODE structure
  will also be filled in. If UseBis is FALSE, then PxeBisReplyValid will be set to FALSE.
  In the structure referenced by parameter Info, the PXE Boot Server list, SrvList[],
  has two uses: It is the Boot Server IP address list used for unicast discovery
  (if the UseUCast field is TRUE), and it is the list used for Boot Server verification
  (if the MustUseList field is TRUE). Also, if the MustUseList field in that structure
  is TRUE and the AcceptAnyResponse field in the SrvList[] array is TRUE, any Boot
  Server reply of that type will be accepted. If the AcceptAnyResponse field is
  FALSE, only responses from Boot Servers with matching IP addresses will be accepted.
  This function can take at least 10 seconds to timeout and return control to the
  caller. If the Discovery sequence does not complete, then EFI_TIMEOUT will be
  returned. Please see the Preboot Execution Environment (PXE) Specification for
  additional details on the implementation of the Discovery sequence.
  If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then the Discovery sequence is stopped and EFI_ABORTED will be returned.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  Type              The type of bootstrap to perform.
  @param[in]  Layer             Pointer to the boot server layer number to discover, which must be
                                PXE_BOOT_LAYER_INITIAL when a new server type is being
                                discovered.
  @param[in]  UseBis            TRUE if Boot Integrity Services are to be used. FALSE otherwise.
  @param[in]  Info              Pointer to a data structure that contains additional information
                                on the type of discovery operation that is to be performed.
                                It is optional.

  @retval EFI_SUCCESS           The Discovery sequence has been completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory to complete Discovery.
  @retval EFI_ABORTED           The callback function aborted the Discovery sequence.
  @retval EFI_TIMEOUT           The Discovery sequence timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the PXE discovery
                                session.

**/
EFI_STATUS
EFIAPI
EfiPxeBcDiscover (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           Type,
  IN UINT16                           *Layer,
  IN BOOLEAN                          UseBis,
  IN EFI_PXE_BASE_CODE_DISCOVER_INFO  *Info   OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA               *Private;
  EFI_PXE_BASE_CODE_MODE           *Mode;
  EFI_PXE_BASE_CODE_DISCOVER_INFO  DefaultInfo;
  EFI_PXE_BASE_CODE_SRVLIST        *SrvList;
  PXEBC_BOOT_SVR_ENTRY             *BootSvrEntry;
  UINT16                           Index;
  EFI_STATUS                       Status;
  EFI_PXE_BASE_CODE_IP_FILTER      IpFilter;
  EFI_PXE_BASE_CODE_DISCOVER_INFO  *NewCreatedInfo;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private                 = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode                    = Private->PxeBc.Mode;
  Mode->IcmpErrorReceived = FALSE;
  BootSvrEntry            = NULL;
  SrvList                 = NULL;
  Status                  = EFI_DEVICE_ERROR;
  Private->Function       = EFI_PXE_BASE_CODE_FUNCTION_DISCOVER;
  NewCreatedInfo          = NULL;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  //
  // Station address should be ready before do discover.
  //
  if (!Private->IsAddressOk) {
    return EFI_INVALID_PARAMETER;
  }

  if (Mode->UsingIpv6) {
    //
    // Stop Udp6Read instance
    //
    Private->Udp6Read->Configure (Private->Udp6Read, NULL);
  } else {
    //
    // Stop Udp4Read instance
    //
    Private->Udp4Read->Configure (Private->Udp4Read, NULL);
  }

  //
  // There are 3 methods to get the information for discover.
  //
  ZeroMem (&DefaultInfo, sizeof (EFI_PXE_BASE_CODE_DISCOVER_INFO));
  if (*Layer != EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL) {
    //
    // 1. Take the previous setting as the discover info.
    //
    if (!Mode->PxeDiscoverValid ||
        !Mode->PxeReplyReceived ||
        (!Mode->PxeBisReplyReceived && UseBis))
    {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Info                         = &DefaultInfo;
    Info->IpCnt                  = 1;
    Info->UseUCast               = TRUE;
    SrvList                      = Info->SrvList;
    SrvList[0].Type              = Type;
    SrvList[0].AcceptAnyResponse = FALSE;

    CopyMem (&SrvList->IpAddr, &Private->ServerIp, sizeof (EFI_IP_ADDRESS));
  } else if (Info == NULL) {
    //
    // 2. Extract the discover information from the cached packets if unspecified.
    //
    NewCreatedInfo = &DefaultInfo;
    Status         = PxeBcExtractDiscoverInfo (Private, Type, &NewCreatedInfo, &BootSvrEntry, &SrvList);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    ASSERT (NewCreatedInfo != NULL);
    Info = NewCreatedInfo;
  } else {
    //
    // 3. Take the pass-in information as the discover info, and validate the server list.
    //
    SrvList = Info->SrvList;

    if (!SrvList[0].AcceptAnyResponse) {
      for (Index = 1; Index < Info->IpCnt; Index++) {
        if (SrvList[Index].AcceptAnyResponse) {
          break;
        }
      }

      if (Index != Info->IpCnt) {
        //
        // It's invalid if the first server doesn't accept any response
        // but any of the other servers does accept any response.
        //
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }
    }
  }

  //
  // Info and BootSvrEntry/SrvList are all ready by now, so execute discover by UniCast/BroadCast/MultiCast.
  //
  if ((!Info->UseUCast && !Info->UseBCast && !Info->UseMCast) ||
      (Info->MustUseList && (Info->IpCnt == 0)))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Private->IsDoDiscover = TRUE;

  if (Info->UseMCast) {
    //
    // Do discover by multicast.
    //
    Status = PxeBcDiscoverBootServer (
               Private,
               Type,
               Layer,
               UseBis,
               &Info->ServerMCastIp,
               Info->IpCnt,
               SrvList
               );
  } else if (Info->UseBCast) {
    //
    // Do discover by broadcast, but only valid for IPv4.
    //
    ASSERT (!Mode->UsingIpv6);
    Status = PxeBcDiscoverBootServer (
               Private,
               Type,
               Layer,
               UseBis,
               NULL,
               Info->IpCnt,
               SrvList
               );
  } else if (Info->UseUCast) {
    //
    // Do discover by unicast.
    //
    for (Index = 0; Index < Info->IpCnt; Index++) {
      if (BootSvrEntry == NULL) {
        CopyMem (&Private->ServerIp, &SrvList[Index].IpAddr, sizeof (EFI_IP_ADDRESS));
      } else {
        ASSERT (!Mode->UsingIpv6);
        ZeroMem (&Private->ServerIp, sizeof (EFI_IP_ADDRESS));
        CopyMem (&Private->ServerIp, &BootSvrEntry->IpAddr[Index], sizeof (EFI_IPv4_ADDRESS));
      }

      Status = PxeBcDiscoverBootServer (
                 Private,
                 Type,
                 Layer,
                 UseBis,
                 &Private->ServerIp,
                 Info->IpCnt,
                 SrvList
                 );
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Parse the cached PXE reply packet, and store it into mode data if valid.
    //
    if (Mode->UsingIpv6) {
      Status = PxeBcParseDhcp6Packet (&Private->PxeReply.Dhcp6);
      if (!EFI_ERROR (Status)) {
        CopyMem (
          &Mode->PxeReply.Dhcpv6,
          &Private->PxeReply.Dhcp6.Packet.Ack.Dhcp6,
          Private->PxeReply.Dhcp6.Packet.Ack.Length
          );
        Mode->PxeReplyReceived = TRUE;
        Mode->PxeDiscoverValid = TRUE;
      }
    } else {
      Status = PxeBcParseDhcp4Packet (&Private->PxeReply.Dhcp4);
      if (!EFI_ERROR (Status)) {
        CopyMem (
          &Mode->PxeReply.Dhcpv4,
          &Private->PxeReply.Dhcp4.Packet.Ack.Dhcp4,
          Private->PxeReply.Dhcp4.Packet.Ack.Length
          );
        Mode->PxeReplyReceived = TRUE;
        Mode->PxeDiscoverValid = TRUE;
      }
    }
  }

ON_EXIT:

  if ((NewCreatedInfo != NULL) && (NewCreatedInfo != &DefaultInfo)) {
    FreePool (NewCreatedInfo);
  }

  if (Mode->UsingIpv6) {
    Private->Udp6Read->Configure (Private->Udp6Read, &Private->Udp6CfgData);
  } else {
    Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);
  }

  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem (&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
  IpFilter.Filters = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  This->SetIpFilter (This, &IpFilter);

  return Status;
}

/**
  Used to perform TFTP and MTFTP services.

  This function is used to perform TFTP and MTFTP services. This includes the
  TFTP operations to get the size of a file, read a directory, read a file, and
  write a file. It also includes the MTFTP operations to get the size of a file,
  read a directory, and read a file. The type of operation is specified by Operation.
  If the callback function that is invoked during the TFTP/MTFTP operation does
  not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE, then EFI_ABORTED will
  be returned.
  For read operations, the return data will be placed in the buffer specified by
  BufferPtr. If BufferSize is too small to contain the entire downloaded file,
  then EFI_BUFFER_TOO_SMALL will be returned and BufferSize will be set to zero,
  or the size of the requested file. (NOTE: the size of the requested file is only returned
  if the TFTP server supports TFTP options). If BufferSize is large enough for the
  read operation, then BufferSize will be set to the size of the downloaded file,
  and EFI_SUCCESS will be returned. Applications using the PxeBc.Mtftp() services
  should use the get-file-size operations to determine the size of the downloaded
  file prior to using the read-file operations-especially when downloading large
  (greater than 64 MB) files-instead of making two calls to the read-file operation.
  Following this recommendation will save time if the file is larger than expected
  and the TFTP server does not support TFTP option extensions. Without TFTP option
  extension support, the client must download the entire file, counting and discarding
  the received packets, to determine the file size.
  For write operations, the data to be sent is in the buffer specified by BufferPtr.
  BufferSize specifies the number of bytes to send. If the write operation completes
  successfully, then EFI_SUCCESS will be returned.
  For TFTP "get file size" operations, the size of the requested file or directory
  is returned in BufferSize, and EFI_SUCCESS will be returned. If the TFTP server
  does not support options, the file will be downloaded into a bit bucket and the
  length of the downloaded file will be returned. For MTFTP "get file size" operations,
  if the MTFTP server does not support the "get file size" option, EFI_UNSUPPORTED
  will be returned.
  This function can take up to 10 seconds to timeout and return control to the caller.
  If the TFTP sequence does not complete, EFI_TIMEOUT will be returned.
  If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then the TFTP sequence is stopped and EFI_ABORTED will be returned.

  @param[in]      This          Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]      Operation     The type of operation to perform.
  @param[in, out] BufferPtr     A pointer to the data buffer.
  @param[in]      Overwrite     Only used on write file operations. TRUE if a file on a remote
                                server can be overwritten.
  @param[in, out] BufferSize    For get-file-size operations, *BufferSize returns the size of the
                                requested file.
  @param[in]      BlockSize     The requested block size to be used during a TFTP transfer.
  @param[in]      ServerIp      The TFTP / MTFTP server IP address.
  @param[in]      Filename      A Null-terminated ASCII string that specifies a directory name
                                or a file name.
  @param[in]      Info          Pointer to the MTFTP information.
  @param[in]      DontUseBuffer Set to FALSE for normal TFTP and MTFTP read file operation.

  @retval EFI_SUCCESS           The TFTP/MTFTP operation was completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is not large enough to complete the read operation.
  @retval EFI_ABORTED           The callback function aborted the TFTP/MTFTP operation.
  @retval EFI_TIMEOUT           The TFTP/MTFTP operation timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the MTFTP session.
  @retval EFI_TFTP_ERROR        A TFTP error packet was received during the MTFTP session.

**/
EFI_STATUS
EFIAPI
EfiPxeBcMtftp (
  IN     EFI_PXE_BASE_CODE_PROTOCOL     *This,
  IN     EFI_PXE_BASE_CODE_TFTP_OPCODE  Operation,
  IN OUT VOID                           *BufferPtr    OPTIONAL,
  IN     BOOLEAN                        Overwrite,
  IN OUT UINT64                         *BufferSize,
  IN     UINTN                          *BlockSize    OPTIONAL,
  IN     EFI_IP_ADDRESS                 *ServerIp,
  IN     UINT8                          *Filename,
  IN     EFI_PXE_BASE_CODE_MTFTP_INFO   *Info         OPTIONAL,
  IN     BOOLEAN                        DontUseBuffer
  )
{
  PXEBC_PRIVATE_DATA           *Private;
  EFI_PXE_BASE_CODE_MODE       *Mode;
  EFI_MTFTP4_CONFIG_DATA       Mtftp4Config;
  EFI_MTFTP6_CONFIG_DATA       Mtftp6Config;
  VOID                         *Config;
  EFI_STATUS                   Status;
  EFI_PXE_BASE_CODE_IP_FILTER  IpFilter;
  UINTN                        WindowSize;

  if ((This == NULL) ||
      (Filename == NULL) ||
      (BufferSize == NULL) ||
      (ServerIp == NULL) ||
      ((BlockSize != NULL) && (*BlockSize < PXE_MTFTP_DEFAULT_BLOCK_SIZE)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Operation == EFI_PXE_BASE_CODE_TFTP_READ_FILE) ||
      (Operation == EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY) ||
      (Operation == EFI_PXE_BASE_CODE_MTFTP_READ_FILE) ||
      (Operation == EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY))
  {
    if ((BufferPtr == NULL) && !DontUseBuffer) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Config  = NULL;
  Status  = EFI_DEVICE_ERROR;
  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  //
  // Get PcdPxeTftpWindowSize.
  //
  WindowSize = (UINTN)PcdGet64 (PcdPxeTftpWindowSize);

  if (Mode->UsingIpv6) {
    if (!NetIp6IsValidUnicast (&ServerIp->v6)) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (IP4_IS_UNSPECIFIED (NTOHL (ServerIp->Addr[0])) || IP4_IS_LOCAL_BROADCAST (NTOHL (ServerIp->Addr[0]))) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (Mode->UsingIpv6) {
    //
    // Set configuration data for Mtftp6 instance.
    //
    ZeroMem (&Mtftp6Config, sizeof (EFI_MTFTP6_CONFIG_DATA));
    Config                    = &Mtftp6Config;
    Mtftp6Config.TimeoutValue = PXEBC_MTFTP_TIMEOUT;
    Mtftp6Config.TryCount     = PXEBC_MTFTP_RETRIES;
    CopyMem (&Mtftp6Config.StationIp, &Private->StationIp.v6, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Mtftp6Config.ServerIp, &ServerIp->v6, sizeof (EFI_IPv6_ADDRESS));
    //
    // Stop Udp6Read instance
    //
    Private->Udp6Read->Configure (Private->Udp6Read, NULL);
  } else {
    //
    // Set configuration data for Mtftp4 instance.
    //
    ZeroMem (&Mtftp4Config, sizeof (EFI_MTFTP4_CONFIG_DATA));
    Config                         = &Mtftp4Config;
    Mtftp4Config.UseDefaultSetting = FALSE;
    Mtftp4Config.TimeoutValue      = PXEBC_MTFTP_TIMEOUT;
    Mtftp4Config.TryCount          = PXEBC_MTFTP_RETRIES;
    CopyMem (&Mtftp4Config.StationIp, &Private->StationIp.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4Config.SubnetMask, &Private->SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4Config.GatewayIp, &Private->GatewayIp.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4Config.ServerIp, &ServerIp->v4, sizeof (EFI_IPv4_ADDRESS));
    //
    // Stop Udp4Read instance
    //
    Private->Udp4Read->Configure (Private->Udp4Read, NULL);
  }

  Mode->TftpErrorReceived = FALSE;
  Mode->IcmpErrorReceived = FALSE;

  switch (Operation) {
    case EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE:
      //
      // Send TFTP request to get file size.
      //
      Status = PxeBcTftpGetFileSize (
                 Private,
                 Config,
                 Filename,
                 BlockSize,
                 (WindowSize > 1) ? &WindowSize : NULL,
                 BufferSize
                 );

      break;

    case EFI_PXE_BASE_CODE_TFTP_READ_FILE:
      //
      // Send TFTP request to read file.
      //
      Status = PxeBcTftpReadFile (
                 Private,
                 Config,
                 Filename,
                 BlockSize,
                 (WindowSize > 1) ? &WindowSize : NULL,
                 BufferPtr,
                 BufferSize,
                 DontUseBuffer
                 );

      break;

    case EFI_PXE_BASE_CODE_TFTP_WRITE_FILE:
      //
      // Send TFTP request to write file.
      //
      Status = PxeBcTftpWriteFile (
                 Private,
                 Config,
                 Filename,
                 Overwrite,
                 BlockSize,
                 BufferPtr,
                 BufferSize
                 );

      break;

    case EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY:
      //
      // Send TFTP request to read directory.
      //
      Status = PxeBcTftpReadDirectory (
                 Private,
                 Config,
                 Filename,
                 BlockSize,
                 (WindowSize > 1) ? &WindowSize : NULL,
                 BufferPtr,
                 BufferSize,
                 DontUseBuffer
                 );

      break;

    case EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE:
    case EFI_PXE_BASE_CODE_MTFTP_READ_FILE:
    case EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY:
      Status = EFI_UNSUPPORTED;

      break;

    default:
      Status = EFI_INVALID_PARAMETER;

      break;
  }

  if (Status == EFI_ICMP_ERROR) {
    Mode->IcmpErrorReceived = TRUE;
  }

  //
  // Reconfigure the UDP instance with the default configuration.
  //
  if (Mode->UsingIpv6) {
    Private->Udp6Read->Configure (Private->Udp6Read, &Private->Udp6CfgData);
  } else {
    Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);
  }

  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem (&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
  IpFilter.Filters = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  This->SetIpFilter (This, &IpFilter);

  return Status;
}

/**
  Writes a UDP packet to the network interface.

  This function writes a UDP packet specified by the (optional HeaderPtr and)
  BufferPtr parameters to the network interface. The UDP header is automatically
  built by this routine. It uses the parameters OpFlags, DestIp, DestPort, GatewayIp,
  SrcIp, and SrcPort to build this header. If the packet is successfully built and
  transmitted through the network interface, then EFI_SUCCESS will be returned.
  If a timeout occurs during the transmission of the packet, then EFI_TIMEOUT will
  be returned. If an ICMP error occurs during the transmission of the packet, then
  the IcmpErrorReceived field is set to TRUE, the IcmpError field is filled in and
  EFI_ICMP_ERROR will be returned. If the Callback Protocol does not return
  EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE, then EFI_ABORTED will be returned.

  @param[in]      This          Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]      OpFlags       The UDP operation flags.
  @param[in]      DestIp        The destination IP address.
  @param[in]      DestPort      The destination UDP port number.
  @param[in]      GatewayIp     The gateway IP address.
  @param[in]      SrcIp         The source IP address.
  @param[in, out] SrcPort       The source UDP port number.
  @param[in]      HeaderSize    An optional field which may be set to the length of a header
                                at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]  HeaderPtr         If HeaderSize is not NULL, a pointer to a header to be
                                prefixed to the data at BufferPtr.
  @param[in]  BufferSize        A pointer to the size of the data at BufferPtr.
  @param[in]  BufferPtr         A pointer to the data to be written.

  @retval EFI_SUCCESS           The UDP Write operation completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_BAD_BUFFER_SIZE   The buffer is too long to be transmitted.
  @retval EFI_ABORTED           The callback function aborted the UDP Write operation.
  @retval EFI_TIMEOUT           The UDP Write operation timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the UDP write session.

**/
EFI_STATUS
EFIAPI
EfiPxeBcUdpWrite (
  IN     EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN     UINT16                      OpFlags,
  IN     EFI_IP_ADDRESS              *DestIp,
  IN     EFI_PXE_BASE_CODE_UDP_PORT  *DestPort,
  IN     EFI_IP_ADDRESS              *GatewayIp  OPTIONAL,
  IN     EFI_IP_ADDRESS              *SrcIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort    OPTIONAL,
  IN     UINTN                       *HeaderSize OPTIONAL,
  IN     VOID                        *HeaderPtr  OPTIONAL,
  IN     UINTN                       *BufferSize,
  IN     VOID                        *BufferPtr
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_UDP4_SESSION_DATA   Udp4Session;
  EFI_UDP6_SESSION_DATA   Udp6Session;
  EFI_STATUS              Status;
  BOOLEAN                 DoNotFragment;

  if ((This == NULL) || (DestIp == NULL) || (DestPort == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT) != 0) {
    DoNotFragment = FALSE;
  } else {
    DoNotFragment = TRUE;
  }

  if (!Mode->UsingIpv6 && (GatewayIp != NULL) && (Mode->SubnetMask.Addr[0] != 0) &&
      !NetIp4IsUnicast (NTOHL (GatewayIp->Addr[0]), EFI_NTOHL (Mode->SubnetMask)))
  {
    //
    // Gateway is provided but it's not a unicast IPv4 address, while it will be ignored for IPv6.
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((HeaderSize != NULL) && ((*HeaderSize == 0) || (HeaderPtr == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferSize == NULL) || ((*BufferSize != 0) && (BufferPtr == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (!Private->IsAddressOk && (SrcIp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Private->CurSrcPort == 0) ||
      ((SrcPort != NULL) && (*SrcPort != Private->CurSrcPort)))
  {
    //
    // Reconfigure UDPv4/UDPv6 for UdpWrite if the source port changed.
    //
    if (SrcPort != NULL) {
      Private->CurSrcPort = *SrcPort;
    }
  }

  if (Mode->UsingIpv6) {
    Status = PxeBcConfigUdp6Write (
               Private->Udp6Write,
               &Private->StationIp.v6,
               &Private->CurSrcPort
               );
  } else {
    //
    // Configure the UDPv4 instance with gateway information from DHCP server as default.
    //
    Status = PxeBcConfigUdp4Write (
               Private->Udp4Write,
               &Private->StationIp.v4,
               &Private->SubnetMask.v4,
               &Private->GatewayIp.v4,
               &Private->CurSrcPort,
               DoNotFragment,
               Private->Mode.TTL,
               Private->Mode.ToS
               );
  }

  if (EFI_ERROR (Status)) {
    Private->CurSrcPort = 0;
    return EFI_INVALID_PARAMETER;
  } else if (SrcPort != NULL) {
    *SrcPort = Private->CurSrcPort;
  }

  //
  // Start a timer as timeout event for this blocking API.
  //
  gBS->SetTimer (Private->UdpTimeOutEvent, TimerRelative, PXEBC_UDP_TIMEOUT);

  if (Mode->UsingIpv6) {
    //
    // Construct UDPv6 session data.
    //
    ZeroMem (&Udp6Session, sizeof (EFI_UDP6_SESSION_DATA));
    CopyMem (&Udp6Session.DestinationAddress, DestIp, sizeof (EFI_IPv6_ADDRESS));
    Udp6Session.DestinationPort = *DestPort;
    if (SrcIp != NULL) {
      CopyMem (&Udp6Session.SourceAddress, SrcIp, sizeof (EFI_IPv6_ADDRESS));
    }

    if (SrcPort != NULL) {
      Udp6Session.SourcePort = *SrcPort;
    }

    Status = PxeBcUdp6Write (
               Private->Udp6Write,
               &Udp6Session,
               Private->UdpTimeOutEvent,
               HeaderSize,
               HeaderPtr,
               BufferSize,
               BufferPtr
               );
  } else {
    //
    // Construct UDPv4 session data.
    //
    ZeroMem (&Udp4Session, sizeof (EFI_UDP4_SESSION_DATA));
    CopyMem (&Udp4Session.DestinationAddress, DestIp, sizeof (EFI_IPv4_ADDRESS));
    Udp4Session.DestinationPort = *DestPort;
    if (SrcIp != NULL) {
      CopyMem (&Udp4Session.SourceAddress, SrcIp, sizeof (EFI_IPv4_ADDRESS));
    }

    if (SrcPort != NULL) {
      Udp4Session.SourcePort = *SrcPort;
    }

    //
    // Override the gateway information if user specified.
    //
    Status = PxeBcUdp4Write (
               Private->Udp4Write,
               &Udp4Session,
               Private->UdpTimeOutEvent,
               (EFI_IPv4_ADDRESS *)GatewayIp,
               HeaderSize,
               HeaderPtr,
               BufferSize,
               BufferPtr
               );
  }

  gBS->SetTimer (Private->UdpTimeOutEvent, TimerCancel, 0);

  //
  // Reset the UdpWrite instance.
  //
  if (Mode->UsingIpv6) {
    Private->Udp6Write->Configure (Private->Udp6Write, NULL);
  } else {
    Private->Udp4Write->Configure (Private->Udp4Write, NULL);
  }

  return Status;
}

/**
  Reads a UDP packet from the network interface.
+
  This function reads a UDP packet from a network interface. The data contents
  are returned in (the optional HeaderPtr and) BufferPtr, and the size of the
  buffer received is returned in BufferSize . If the input BufferSize is smaller
  than the UDP packet received (less optional HeaderSize), it will be set to the
  required size, and EFI_BUFFER_TOO_SMALL will be returned. In this case, the
  contents of BufferPtr are undefined, and the packet is lost. If a UDP packet is
  successfully received, then EFI_SUCCESS will be returned, and the information
  from the UDP header will be returned in DestIp, DestPort, SrcIp, and SrcPort if
  they are not NULL. Depending on the values of OpFlags and the DestIp, DestPort,
  SrcIp, and SrcPort input values, different types of UDP packet receive filtering
  will be performed. The following tables summarize these receive filter operations.

  @param[in]      This          Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]      OpFlags       The UDP operation flags.
  @param[in, out] DestIp        The destination IP address.
  @param[in, out] DestPort      The destination UDP port number.
  @param[in, out] SrcIp         The source IP address.
  @param[in, out] SrcPort       The source UDP port number.
  @param[in]      HeaderSize    An optional field which may be set to the length of a
                                header at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]      HeaderPtr     If HeaderSize is not NULL, a pointer to a header to be
                                prefixed to the data at BufferPtr.
  @param[in, out] BufferSize    A pointer to the size of the data at BufferPtr.
  @param[in]      BufferPtr     A pointer to the data to be read.

  @retval EFI_SUCCESS           The UDP Read operation was completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_BUFFER_TOO_SMALL  The packet is larger than Buffer can hold.
  @retval EFI_ABORTED           The callback function aborted the UDP Read operation.
  @retval EFI_TIMEOUT           The UDP Read operation timed out.

**/
EFI_STATUS
EFIAPI
EfiPxeBcUdpRead (
  IN     EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN     UINT16                      OpFlags,
  IN OUT EFI_IP_ADDRESS              *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS              *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort     OPTIONAL,
  IN     UINTN                       *HeaderSize  OPTIONAL,
  IN     VOID                        *HeaderPtr   OPTIONAL,
  IN OUT UINTN                       *BufferSize,
  IN     VOID                        *BufferPtr
  )
{
  PXEBC_PRIVATE_DATA         *Private;
  EFI_PXE_BASE_CODE_MODE     *Mode;
  EFI_UDP4_COMPLETION_TOKEN  Udp4Token;
  EFI_UDP6_COMPLETION_TOKEN  Udp6Token;
  EFI_UDP4_RECEIVE_DATA      *Udp4Rx;
  EFI_UDP6_RECEIVE_DATA      *Udp6Rx;
  EFI_STATUS                 Status;
  BOOLEAN                    IsDone;
  BOOLEAN                    IsMatched;
  UINTN                      CopiedLen;
  UINTN                      HeaderLen;
  UINTN                      HeaderCopiedLen;
  UINTN                      BufferCopiedLen;
  UINT32                     FragmentLength;
  UINTN                      FragmentIndex;
  UINT8                      *FragmentBuffer;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private   = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode      = Private->PxeBc.Mode;
  IsDone    = FALSE;
  IsMatched = FALSE;
  Udp4Rx    = NULL;
  Udp6Rx    = NULL;

  if ((((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT) == 0) && (DestPort == NULL)) ||
      (((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP) == 0) && (SrcIp == NULL)) ||
      (((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT) == 0) && (SrcPort == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (((HeaderSize != NULL) && (*HeaderSize == 0)) || ((HeaderSize != NULL) && (HeaderPtr == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferSize == NULL) || (BufferPtr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  ZeroMem (&Udp6Token, sizeof (EFI_UDP6_COMPLETION_TOKEN));
  ZeroMem (&Udp4Token, sizeof (EFI_UDP4_COMPLETION_TOKEN));

  if (Mode->UsingIpv6) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    PxeBcCommonNotify,
                    &IsDone,
                    &Udp6Token.Event
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    PxeBcCommonNotify,
                    &IsDone,
                    &Udp4Token.Event
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Start a timer as timeout event for this blocking API.
  //
  gBS->SetTimer (Private->UdpTimeOutEvent, TimerRelative, PXEBC_UDP_TIMEOUT);
  Mode->IcmpErrorReceived = FALSE;

  //
  // Read packet by Udp4Read/Udp6Read until matched or timeout.
  //
  while (!IsMatched && !EFI_ERROR (Status)) {
    if (Mode->UsingIpv6) {
      Status = PxeBcUdp6Read (
                 Private->Udp6Read,
                 &Udp6Token,
                 Mode,
                 Private->UdpTimeOutEvent,
                 OpFlags,
                 &IsDone,
                 &IsMatched,
                 DestIp,
                 DestPort,
                 SrcIp,
                 SrcPort
                 );
    } else {
      Status = PxeBcUdp4Read (
                 Private->Udp4Read,
                 &Udp4Token,
                 Mode,
                 Private->UdpTimeOutEvent,
                 OpFlags,
                 &IsDone,
                 &IsMatched,
                 DestIp,
                 DestPort,
                 SrcIp,
                 SrcPort
                 );
    }
  }

  if ((Status == EFI_ICMP_ERROR) ||
      (Status == EFI_NETWORK_UNREACHABLE) ||
      (Status == EFI_HOST_UNREACHABLE) ||
      (Status == EFI_PROTOCOL_UNREACHABLE) ||
      (Status == EFI_PORT_UNREACHABLE))
  {
    //
    // Get different return status for icmp error from Udp, refers to UEFI spec.
    //
    Mode->IcmpErrorReceived = TRUE;
  }

  gBS->SetTimer (Private->UdpTimeOutEvent, TimerCancel, 0);

  if (IsMatched) {
    //
    // Copy the received packet to user if matched by filter.
    //
    if (Mode->UsingIpv6) {
      Udp6Rx = Udp6Token.Packet.RxData;
      ASSERT (Udp6Rx != NULL);

      HeaderLen = 0;
      if (HeaderSize != NULL) {
        HeaderLen = MIN (*HeaderSize, Udp6Rx->DataLength);
      }

      if (Udp6Rx->DataLength - HeaderLen > *BufferSize) {
        Status = EFI_BUFFER_TOO_SMALL;
      } else {
        if (HeaderSize != NULL) {
          *HeaderSize = HeaderLen;
        }

        *BufferSize = Udp6Rx->DataLength - HeaderLen;

        HeaderCopiedLen = 0;
        BufferCopiedLen = 0;
        for (FragmentIndex = 0; FragmentIndex < Udp6Rx->FragmentCount; FragmentIndex++) {
          FragmentLength = Udp6Rx->FragmentTable[FragmentIndex].FragmentLength;
          FragmentBuffer = Udp6Rx->FragmentTable[FragmentIndex].FragmentBuffer;
          if (HeaderCopiedLen + FragmentLength < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopyMem ((UINT8 *)HeaderPtr + HeaderCopiedLen, FragmentBuffer, FragmentLength);
            HeaderCopiedLen += FragmentLength;
          } else if (HeaderCopiedLen < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopiedLen = HeaderLen - HeaderCopiedLen;
            CopyMem ((UINT8 *)HeaderPtr + HeaderCopiedLen, FragmentBuffer, CopiedLen);
            HeaderCopiedLen += CopiedLen;

            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *)BufferPtr + BufferCopiedLen, FragmentBuffer + CopiedLen, FragmentLength - CopiedLen);
            BufferCopiedLen += (FragmentLength - CopiedLen);
          } else {
            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *)BufferPtr + BufferCopiedLen, FragmentBuffer, FragmentLength);
            BufferCopiedLen += FragmentLength;
          }
        }
      }

      //
      // Recycle the receiving buffer after copy to user.
      //
      gBS->SignalEvent (Udp6Rx->RecycleSignal);
    } else {
      Udp4Rx = Udp4Token.Packet.RxData;
      ASSERT (Udp4Rx != NULL);

      HeaderLen = 0;
      if (HeaderSize != NULL) {
        HeaderLen = MIN (*HeaderSize, Udp4Rx->DataLength);
      }

      if (Udp4Rx->DataLength - HeaderLen > *BufferSize) {
        Status = EFI_BUFFER_TOO_SMALL;
      } else {
        if (HeaderSize != NULL) {
          *HeaderSize = HeaderLen;
        }

        *BufferSize = Udp4Rx->DataLength - HeaderLen;

        HeaderCopiedLen = 0;
        BufferCopiedLen = 0;
        for (FragmentIndex = 0; FragmentIndex < Udp4Rx->FragmentCount; FragmentIndex++) {
          FragmentLength = Udp4Rx->FragmentTable[FragmentIndex].FragmentLength;
          FragmentBuffer = Udp4Rx->FragmentTable[FragmentIndex].FragmentBuffer;
          if (HeaderCopiedLen + FragmentLength < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopyMem ((UINT8 *)HeaderPtr + HeaderCopiedLen, FragmentBuffer, FragmentLength);
            HeaderCopiedLen += FragmentLength;
          } else if (HeaderCopiedLen < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopiedLen = HeaderLen - HeaderCopiedLen;
            CopyMem ((UINT8 *)HeaderPtr + HeaderCopiedLen, FragmentBuffer, CopiedLen);
            HeaderCopiedLen += CopiedLen;

            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *)BufferPtr + BufferCopiedLen, FragmentBuffer + CopiedLen, FragmentLength - CopiedLen);
            BufferCopiedLen += (FragmentLength - CopiedLen);
          } else {
            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *)BufferPtr + BufferCopiedLen, FragmentBuffer, FragmentLength);
            BufferCopiedLen += FragmentLength;
          }
        }
      }

      //
      // Recycle the receiving buffer after copy to user.
      //
      gBS->SignalEvent (Udp4Rx->RecycleSignal);
    }
  }

  if (Mode->UsingIpv6) {
    Private->Udp6Read->Cancel (Private->Udp6Read, &Udp6Token);
    gBS->CloseEvent (Udp6Token.Event);
  } else {
    Private->Udp4Read->Cancel (Private->Udp4Read, &Udp4Token);
    gBS->CloseEvent (Udp4Token.Event);
  }

  return Status;
}

/**
  Updates the IP receive filters of a network device and enables software filtering.

  The NewFilter field is used to modify the network device's current IP receive
  filter settings and to enable a software filter. This function updates the IpFilter
  field of the EFI_PXE_BASE_CODE_MODE structure with the contents of NewIpFilter.
  The software filter is used when the USE_FILTER in OpFlags is set to UdpRead().
  The current hardware filter remains in effect no matter what the settings of OpFlags.
  This is so that the meaning of ANY_DEST_IP set in OpFlags to UdpRead() is from those
  packets whose reception is enabled in hardware-physical NIC address (unicast),
  broadcast address, logical address or addresses (multicast), or all (promiscuous).
  UdpRead() does not modify the IP filter settings.
  Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP receive
  filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  If an application or driver wishes to preserve the IP receive filter settings,
  it will have to preserve the IP receive filter settings before these calls, and
  use SetIpFilter() to restore them after the calls. If incompatible filtering is
  requested (for example, PROMISCUOUS with anything else), or if the device does not
  support a requested filter setting and it cannot be accommodated in software
  (for example, PROMISCUOUS not supported), EFI_INVALID_PARAMETER will be returned.
  The IPlist field is used to enable IPs other than the StationIP. They may be
  multicast or unicast. If IPcnt is set as well as EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP,
  then both the StationIP and the IPs from the IPlist will be used.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  NewFilter         Pointer to the new set of IP receive filters.

  @retval EFI_SUCCESS           The IP receive filter settings were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetIpFilter (
  IN EFI_PXE_BASE_CODE_PROTOCOL   *This,
  IN EFI_PXE_BASE_CODE_IP_FILTER  *NewFilter
  )
{
  EFI_STATUS              Status;
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_UDP4_CONFIG_DATA    *Udp4Cfg;
  EFI_UDP6_CONFIG_DATA    *Udp6Cfg;
  UINTN                   Index;
  BOOLEAN                 NeedPromiscuous;
  BOOLEAN                 AcceptPromiscuous;
  BOOLEAN                 AcceptBroadcast;
  BOOLEAN                 MultiCastUpdate;

  if ((This == NULL) || (NewFilter == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private         = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode            = Private->PxeBc.Mode;
  Status          = EFI_SUCCESS;
  NeedPromiscuous = FALSE;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  for (Index = 0; Index < NewFilter->IpCnt; Index++) {
    ASSERT (Index < EFI_PXE_BASE_CODE_MAX_IPCNT);
    if (!Mode->UsingIpv6 &&
        IP4_IS_LOCAL_BROADCAST (EFI_IP4 (NewFilter->IpList[Index].v4)))
    {
      //
      // IPv4 broadcast address should not be in IP filter.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (Mode->UsingIpv6) {
      if (((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0) &&
          NetIp6IsValidUnicast (&NewFilter->IpList[Index].v6))
      {
        NeedPromiscuous = TRUE;
      }
    } else if ((EFI_NTOHL (Mode->StationIp) != 0) &&
               (EFI_NTOHL (Mode->SubnetMask) != 0) &&
               IP4_NET_EQUAL (EFI_NTOHL (Mode->StationIp), EFI_NTOHL (NewFilter->IpList[Index].v4), EFI_NTOHL (Mode->SubnetMask.v4)) &&
               NetIp4IsUnicast (EFI_IP4 (NewFilter->IpList[Index].v4), EFI_NTOHL (Mode->SubnetMask)) &&
               ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0))
    {
      NeedPromiscuous = TRUE;
    }
  }

  AcceptPromiscuous = FALSE;
  AcceptBroadcast   = FALSE;
  MultiCastUpdate   = FALSE;

  if (NeedPromiscuous ||
      ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) != 0) ||
      ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) != 0))
  {
    //
    // Configure UDPv4/UDPv6 as promiscuous mode to receive all packets.
    //
    AcceptPromiscuous = TRUE;
  } else if ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST) != 0) {
    //
    // Configure UDPv4 to receive all broadcast packets.
    //
    AcceptBroadcast = TRUE;
  }

  //
  // In multicast condition when Promiscuous FALSE and IpCnt no-zero.
  // Here check if there is any update of the multicast ip address. If yes,
  // we need leave the old multicast group (by Config UDP instance to NULL),
  // and join the new multicast group.
  //
  if (!AcceptPromiscuous) {
    if ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0) {
      if (Mode->IpFilter.IpCnt != NewFilter->IpCnt) {
        MultiCastUpdate = TRUE;
      } else if (CompareMem (Mode->IpFilter.IpList, NewFilter->IpList, NewFilter->IpCnt * sizeof (EFI_IP_ADDRESS)) != 0 ) {
        MultiCastUpdate = TRUE;
      }
    }
  }

  if (!Mode->UsingIpv6) {
    //
    // Check whether we need reconfigure the UDP4 instance.
    //
    Udp4Cfg = &Private->Udp4CfgData;
    if ((AcceptPromiscuous != Udp4Cfg->AcceptPromiscuous)   ||
        (AcceptBroadcast != Udp4Cfg->AcceptBroadcast)     || MultiCastUpdate)
    {
      //
      // Clear the UDP4 instance configuration, all joined groups will be left
      // during the operation.
      //
      Private->Udp4Read->Configure (Private->Udp4Read, NULL);

      //
      // Configure the UDP instance with the new configuration.
      //
      Udp4Cfg->AcceptPromiscuous = AcceptPromiscuous;
      Udp4Cfg->AcceptBroadcast   = AcceptBroadcast;
      Status                     = Private->Udp4Read->Configure (Private->Udp4Read, Udp4Cfg);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // In not Promiscuous mode, need to join the new multicast group.
      //
      if (!AcceptPromiscuous) {
        for (Index = 0; Index < NewFilter->IpCnt; ++Index) {
          if (IP4_IS_MULTICAST (EFI_NTOHL (NewFilter->IpList[Index].v4))) {
            //
            // Join the multicast group.
            //
            Status = Private->Udp4Read->Groups (Private->Udp4Read, TRUE, &NewFilter->IpList[Index].v4);
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }
      }
    }
  } else {
    //
    // Check whether we need reconfigure the UDP6 instance.
    //
    Udp6Cfg = &Private->Udp6CfgData;
    if ((AcceptPromiscuous != Udp6Cfg->AcceptPromiscuous) || MultiCastUpdate) {
      //
      // Clear the UDP6 instance configuration, all joined groups will be left
      // during the operation.
      //
      Private->Udp6Read->Configure (Private->Udp6Read, NULL);

      //
      // Configure the UDP instance with the new configuration.
      //
      Udp6Cfg->AcceptPromiscuous = AcceptPromiscuous;
      Status                     = Private->Udp6Read->Configure (Private->Udp6Read, Udp6Cfg);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // In not Promiscuous mode, need to join the new multicast group.
      //
      if (!AcceptPromiscuous) {
        for (Index = 0; Index < NewFilter->IpCnt; ++Index) {
          if (IP6_IS_MULTICAST (&NewFilter->IpList[Index].v6)) {
            //
            // Join the multicast group.
            //
            Status = Private->Udp6Read->Groups (Private->Udp6Read, TRUE, &NewFilter->IpList[Index].v6);
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }
      }
    }
  }

  //
  // Save the new IP filter into mode data.
  //
  CopyMem (&Mode->IpFilter, NewFilter, sizeof (Mode->IpFilter));

  return Status;
}

/**
  Uses the ARP protocol to resolve a MAC address. It is not supported for IPv6.

  This function uses the ARP protocol to resolve a MAC address. The IP address specified
  by IpAddr is used to resolve a MAC address. If the ARP protocol succeeds in resolving
  the specified address, then the ArpCacheEntries and ArpCache fields of the mode data
  are updated, and EFI_SUCCESS is returned. If MacAddr is not NULL, the resolved
  MAC address is placed there as well.  If the PXE Base Code protocol is in the
  stopped state, then EFI_NOT_STARTED is returned. If the ARP protocol encounters
  a timeout condition while attempting to resolve an address, then EFI_TIMEOUT is
  returned. If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then EFI_ABORTED is returned.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  IpAddr            Pointer to the IP address that is used to resolve a MAC address.
  @param[in]  MacAddr           If not NULL, a pointer to the MAC address that was resolved with the
                                ARP protocol.

  @retval EFI_SUCCESS           The IP or MAC address was resolved.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_ICMP_ERROR        An error occur with the ICMP packet message.

**/
EFI_STATUS
EFIAPI
EfiPxeBcArp (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN EFI_IP_ADDRESS              *IpAddr,
  IN EFI_MAC_ADDRESS             *MacAddr OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_EVENT               ResolvedEvent;
  EFI_STATUS              Status;
  EFI_MAC_ADDRESS         TempMac;
  EFI_MAC_ADDRESS         ZeroMac;
  BOOLEAN                 IsResolved;

  if ((This == NULL) || (IpAddr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private       = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode          = Private->PxeBc.Mode;
  ResolvedEvent = NULL;
  Status        = EFI_SUCCESS;
  IsResolved    = FALSE;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (Mode->UsingIpv6) {
    return EFI_UNSUPPORTED;
  }

  //
  // Station address should be ready before do arp.
  //
  if (!Private->IsAddressOk) {
    return EFI_INVALID_PARAMETER;
  }

  Mode->IcmpErrorReceived = FALSE;
  ZeroMem (&TempMac, sizeof (EFI_MAC_ADDRESS));
  ZeroMem (&ZeroMac, sizeof (EFI_MAC_ADDRESS));

  if (!Mode->AutoArp) {
    //
    // If AutoArp is FALSE, only search in the current Arp cache.
    //
    PxeBcArpCacheUpdate (NULL, Private);
    if (!PxeBcCheckArpCache (Mode, &IpAddr->v4, &TempMac)) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    PxeBcCommonNotify,
                    &IsResolved,
                    &ResolvedEvent
                    );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    //
    // If AutoArp is TRUE, try to send Arp request on initiative.
    //
    Status = Private->Arp->Request (Private->Arp, &IpAddr->v4, ResolvedEvent, &TempMac);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
      goto ON_EXIT;
    }

    while (!IsResolved) {
      if (CompareMem (&TempMac, &ZeroMac, sizeof (EFI_MAC_ADDRESS)) != 0) {
        break;
      }
    }

    if (CompareMem (&TempMac, &ZeroMac, sizeof (EFI_MAC_ADDRESS)) != 0) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_TIMEOUT;
    }
  }

  //
  // Copy the Mac address to user if needed.
  //
  if ((MacAddr != NULL) && !EFI_ERROR (Status)) {
    CopyMem (MacAddr, &TempMac, sizeof (EFI_MAC_ADDRESS));
  }

ON_EXIT:
  if (ResolvedEvent != NULL) {
    gBS->CloseEvent (ResolvedEvent);
  }

  return Status;
}

/**
  Updates the parameters that affect the operation of the PXE Base Code Protocol.

  This function sets parameters that affect the operation of the PXE Base Code Protocol.
  The parameter specified by NewAutoArp is used to control the generation of ARP
  protocol packets. If NewAutoArp is TRUE, then ARP Protocol packets will be generated
  as required by the PXE Base Code Protocol. If NewAutoArp is FALSE, then no ARP
  Protocol packets will be generated. In this case, the only mappings that are
  available are those stored in the ArpCache of the EFI_PXE_BASE_CODE_MODE structure.
  If there are not enough mappings in the ArpCache to perform a PXE Base Code Protocol
  service, then the service will fail. This function updates the AutoArp field of
  the EFI_PXE_BASE_CODE_MODE structure to NewAutoArp.
  The SetParameters() call must be invoked after a Callback Protocol is installed
  to enable the use of callbacks.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  NewAutoArp        If not NULL, a pointer to a value that specifies whether to replace the
                                current value of AutoARP.
  @param[in]  NewSendGUID       If not NULL, a pointer to a value that specifies whether to replace the
                                current value of SendGUID.
  @param[in]  NewTTL            If not NULL, a pointer to be used in place of the current value of TTL,
                                the "time to live" field of the IP header.
  @param[in]  NewToS            If not NULL, a pointer to be used in place of the current value of ToS,
                                the "type of service" field of the IP header.
  @param[in]  NewMakeCallback   If not NULL, a pointer to a value that specifies whether to replace the
                                current value of the MakeCallback field of the Mode structure.

  @retval EFI_SUCCESS           The new parameters values were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetParameters (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN BOOLEAN                     *NewAutoArp         OPTIONAL,
  IN BOOLEAN                     *NewSendGUID        OPTIONAL,
  IN UINT8                       *NewTTL             OPTIONAL,
  IN UINT8                       *NewToS             OPTIONAL,
  IN BOOLEAN                     *NewMakeCallback    OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_GUID                SystemGuid;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (NewMakeCallback != NULL) {
    if (*NewMakeCallback) {
      //
      // Update the previous PxeBcCallback protocol.
      //
      Status = gBS->HandleProtocol (
                      Mode->UsingIpv6 ? Private->Ip6Nic->Controller : Private->Ip4Nic->Controller,
                      &gEfiPxeBaseCodeCallbackProtocolGuid,
                      (VOID **)&Private->PxeBcCallback
                      );

      if (EFI_ERROR (Status) || (Private->PxeBcCallback->Callback == NULL)) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      Private->PxeBcCallback = NULL;
    }

    Mode->MakeCallbacks = *NewMakeCallback;
  }

  if (NewSendGUID != NULL) {
    if (*NewSendGUID && EFI_ERROR (NetLibGetSystemGuid (&SystemGuid))) {
      DEBUG ((DEBUG_WARN, "PXE: Failed to read system GUID from the smbios table!\n"));
      return EFI_INVALID_PARAMETER;
    }

    Mode->SendGUID = *NewSendGUID;
  }

  if (NewAutoArp != NULL) {
    Mode->AutoArp = *NewAutoArp;
  }

  if (NewTTL != NULL) {
    Mode->TTL = *NewTTL;
  }

  if (NewToS != NULL) {
    Mode->ToS = *NewToS;
  }

  return EFI_SUCCESS;
}

/**
  Updates the station IP address and/or subnet mask values of a network device.

  This function updates the station IP address and/or subnet mask values of a network
  device. The NewStationIp field is used to modify the network device's current IP address.
  If NewStationIP is NULL, then the current IP address will not be modified. Otherwise,
  this function updates the StationIp field of the EFI_PXE_BASE_CODE_MODE structure
  with NewStationIp. The NewSubnetMask field is used to modify the network device's current subnet
  mask. If NewSubnetMask is NULL, then the current subnet mask will not be modified.
  Otherwise, this function updates the SubnetMask field of the EFI_PXE_BASE_CODE_MODE
  structure with NewSubnetMask.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  NewStationIp      Pointer to the new IP address to be used by the network device.
  @param[in]  NewSubnetMask     Pointer to the new subnet mask to be used by the network device.

  @retval EFI_SUCCESS           The new station IP address and/or subnet mask were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetStationIP (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN EFI_IP_ADDRESS              *NewStationIp    OPTIONAL,
  IN EFI_IP_ADDRESS              *NewSubnetMask   OPTIONAL
  )
{
  EFI_STATUS              Status;
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NewStationIp != NULL) && !NetIp6IsValidUnicast (&NewStationIp->v6)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;
  Status  = EFI_SUCCESS;

  if (!Mode->UsingIpv6 &&
      (NewSubnetMask != NULL) &&
      !IP4_IS_VALID_NETMASK (NTOHL (NewSubnetMask->Addr[0])))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->UsingIpv6 && (NewStationIp != NULL)) {
    if (IP4_IS_UNSPECIFIED (NTOHL (NewStationIp->Addr[0])) ||
        IP4_IS_LOCAL_BROADCAST (NTOHL (NewStationIp->Addr[0])) ||
        ((NewSubnetMask != NULL) && (NewSubnetMask->Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (NewStationIp->Addr[0]), NTOHL (NewSubnetMask->Addr[0]))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (Mode->UsingIpv6 && (NewStationIp != NULL)) {
    //
    // Set the IPv6 address by Ip6Config protocol.
    //
    Status = PxeBcRegisterIp6Address (Private, &NewStationIp->v6);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  if (NewStationIp != NULL) {
    CopyMem (&Mode->StationIp, NewStationIp, sizeof (EFI_IP_ADDRESS));
    CopyMem (&Private->StationIp, NewStationIp, sizeof (EFI_IP_ADDRESS));
  }

  if (!Mode->UsingIpv6 && (NewSubnetMask != NULL)) {
    CopyMem (&Mode->SubnetMask, NewSubnetMask, sizeof (EFI_IP_ADDRESS));
    CopyMem (&Private->SubnetMask, NewSubnetMask, sizeof (EFI_IP_ADDRESS));
  }

  Status = PxeBcFlushStationIp (Private, NewStationIp, NewSubnetMask);
  if (!EFI_ERROR (Status)) {
    Private->IsAddressOk = TRUE;
  }

ON_EXIT:
  return Status;
}

/**
  Updates the contents of the cached DHCP and Discover packets.

  The pointers to the new packets are used to update the contents of the cached
  packets in the EFI_PXE_BASE_CODE_MODE structure.

  @param[in]  This                   Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param[in]  NewDhcpDiscoverValid   Pointer to a value that will replace the current
                                     DhcpDiscoverValid field.
  @param[in]  NewDhcpAckReceived     Pointer to a value that will replace the current
                                     DhcpAckReceived field.
  @param[in]  NewProxyOfferReceived  Pointer to a value that will replace the current
                                     ProxyOfferReceived field.
  @param[in]  NewPxeDiscoverValid    Pointer to a value that will replace the current
                                     ProxyOfferReceived field.
  @param[in]  NewPxeReplyReceived    Pointer to a value that will replace the current
                                     PxeReplyReceived field.
  @param[in]  NewPxeBisReplyReceived Pointer to a value that will replace the current
                                     PxeBisReplyReceived field.
  @param[in]  NewDhcpDiscover        Pointer to the new cached DHCP Discover packet contents.
  @param[in]  NewDhcpAck             Pointer to the new cached DHCP Ack packet contents.
  @param[in]  NewProxyOffer          Pointer to the new cached Proxy Offer packet contents.
  @param[in]  NewPxeDiscover         Pointer to the new cached PXE Discover packet contents.
  @param[in]  NewPxeReply            Pointer to the new cached PXE Reply packet contents.
  @param[in]  NewPxeBisReply         Pointer to the new cached PXE BIS Reply packet contents.

  @retval EFI_SUCCESS            The cached packet contents were updated.
  @retval EFI_NOT_STARTED        The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER  This is NULL or does not point to a valid
                                 EFI_PXE_BASE_CODE_PROTOCOL structure.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetPackets (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN BOOLEAN                     *NewDhcpDiscoverValid      OPTIONAL,
  IN BOOLEAN                     *NewDhcpAckReceived        OPTIONAL,
  IN BOOLEAN                     *NewProxyOfferReceived     OPTIONAL,
  IN BOOLEAN                     *NewPxeDiscoverValid       OPTIONAL,
  IN BOOLEAN                     *NewPxeReplyReceived       OPTIONAL,
  IN BOOLEAN                     *NewPxeBisReplyReceived    OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewDhcpDiscover           OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewDhcpAck                OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewProxyOffer             OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewPxeDiscover            OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewPxeReply               OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET    *NewPxeBisReply            OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (NewDhcpDiscoverValid != NULL) {
    Mode->DhcpDiscoverValid = *NewDhcpDiscoverValid;
  }

  if (NewDhcpAckReceived != NULL) {
    Mode->DhcpAckReceived = *NewDhcpAckReceived;
  }

  if (NewProxyOfferReceived != NULL) {
    Mode->ProxyOfferReceived = *NewProxyOfferReceived;
  }

  if (NewPxeDiscoverValid != NULL) {
    Mode->PxeDiscoverValid = *NewPxeDiscoverValid;
  }

  if (NewPxeReplyReceived != NULL) {
    Mode->PxeReplyReceived = *NewPxeReplyReceived;
  }

  if (NewPxeBisReplyReceived != NULL) {
    Mode->PxeBisReplyReceived = *NewPxeBisReplyReceived;
  }

  if (NewDhcpDiscover != NULL) {
    CopyMem (&Mode->DhcpDiscover, NewDhcpDiscover, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  if (NewDhcpAck != NULL) {
    CopyMem (&Mode->DhcpAck, NewDhcpAck, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  if (NewProxyOffer != NULL) {
    CopyMem (&Mode->ProxyOffer, NewProxyOffer, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  if (NewPxeDiscover != NULL) {
    CopyMem (&Mode->PxeDiscover, NewPxeDiscover, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  if (NewPxeReply != NULL) {
    CopyMem (&Mode->PxeReply, NewPxeReply, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  if (NewPxeBisReply != NULL) {
    CopyMem (&Mode->PxeBisReply, NewPxeBisReply, sizeof (EFI_PXE_BASE_CODE_PACKET));
  }

  return EFI_SUCCESS;
}

EFI_PXE_BASE_CODE_PROTOCOL  gPxeBcProtocolTemplate = {
  EFI_PXE_BASE_CODE_PROTOCOL_REVISION,
  EfiPxeBcStart,
  EfiPxeBcStop,
  EfiPxeBcDhcp,
  EfiPxeBcDiscover,
  EfiPxeBcMtftp,
  EfiPxeBcUdpWrite,
  EfiPxeBcUdpRead,
  EfiPxeBcSetIpFilter,
  EfiPxeBcArp,
  EfiPxeBcSetParameters,
  EfiPxeBcSetStationIP,
  EfiPxeBcSetPackets,
  NULL
};

/**
  Callback function that is invoked when the PXE Base Code Protocol is about to transmit, has
  received, or is waiting to receive a packet.

  This function is invoked when the PXE Base Code Protocol is about to transmit, has received,
  or is waiting to receive a packet. Parameters Function and Received specify the type of event.
  Parameters PacketLen and Packet specify the packet that generated the event. If these fields
  are zero and NULL respectively, then this is a status update callback. If the operation specified
  by Function is to continue, then CALLBACK_STATUS_CONTINUE should be returned. If the operation
  specified by Function should be aborted, then CALLBACK_STATUS_ABORT should be returned. Due to
  the polling nature of UEFI device drivers, a callback function should not execute for more than 5 ms.
  The SetParameters() function must be called after a Callback Protocol is installed to enable the
  use of callbacks.

  @param[in]  This              Pointer to the EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL instance.
  @param[in]  Function          The PXE Base Code Protocol function that is waiting for an event.
  @param[in]  Received          TRUE if the callback is being invoked due to a receive event. FALSE if
                                the callback is being invoked due to a transmit event.
  @param[in]  PacketLength      The length, in bytes, of Packet. This field will have a value of zero if
                                this is a wait for receive event.
  @param[in]  PacketPtr         If Received is TRUE, a pointer to the packet that was just received;
                                otherwise a pointer to the packet that is about to be transmitted.

  @retval EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE If Function specifies a continue operation.
  @retval EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT    If Function specifies an abort operation.

**/
EFI_PXE_BASE_CODE_CALLBACK_STATUS
EFIAPI
EfiPxeLoadFileCallback (
  IN EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  *This,
  IN EFI_PXE_BASE_CODE_FUNCTION           Function,
  IN BOOLEAN                              Received,
  IN UINT32                               PacketLength,
  IN EFI_PXE_BASE_CODE_PACKET             *PacketPtr     OPTIONAL
  )
{
  EFI_INPUT_KEY  Key;
  EFI_STATUS     Status;

  //
  // Catch Ctrl-C or ESC to abort.
  //
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

  if (!EFI_ERROR (Status)) {
    if ((Key.ScanCode == SCAN_ESC) || (Key.UnicodeChar == (0x1F & 'c'))) {
      return EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT;
    }
  }

  //
  // No print if receive packet
  //
  if (Received) {
    return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
  }

  //
  // Print only for three functions
  //
  switch (Function) {
    case EFI_PXE_BASE_CODE_FUNCTION_MTFTP:
      //
      // Print only for open MTFTP packets, not every MTFTP packets
      //
      if ((PacketLength != 0) && (PacketPtr != NULL)) {
        if ((PacketPtr->Raw[0x1C] != 0x00) || (PacketPtr->Raw[0x1D] != 0x01)) {
          return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
        }
      }

      break;

    case EFI_PXE_BASE_CODE_FUNCTION_DHCP:
    case EFI_PXE_BASE_CODE_FUNCTION_DISCOVER:
      break;

    default:
      return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
  }

  if ((PacketLength != 0) && (PacketPtr != NULL)) {
    //
    // Print '.' when transmit a packet
    //
    AsciiPrint (".");
  }

  return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
}

EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  gPxeBcCallBackTemplate = {
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_REVISION,
  EfiPxeLoadFileCallback
};

/**
  Causes the driver to load a specified file.

  @param[in]      This                Protocol instance pointer.
  @param[in]      FilePath            The device specific path of the file to load.
  @param[in]      BootPolicy          If TRUE, indicates that the request originates from the
                                      boot manager is attempting to load FilePath as a boot
                                      selection. If FALSE, then FilePath must match an exact file
                                      to be loaded.
  @param[in, out] BufferSize          On input the size of Buffer in bytes. On output with a return
                                      code of EFI_SUCCESS, the amount of data transferred to
                                      Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of Buffer required to retrieve the requested file.
  @param[in]      Buffer              The memory buffer to transfer the file to. IF Buffer is NULL,
                                      then no the size of the requested file is returned in
                                      BufferSize.

  @retval EFI_SUCCESS                 The file was loaded.
  @retval EFI_UNSUPPORTED             The device does not support the provided BootPolicy.
  @retval EFI_INVALID_PARAMETER       FilePath is not a valid device path, or
                                      BufferSize is NULL.
  @retval EFI_NO_MEDIA                No medium was present to load the file.
  @retval EFI_DEVICE_ERROR            The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE             The remote system did not respond.
  @retval EFI_NOT_FOUND               The file was not found.
  @retval EFI_ABORTED                 The file load process was manually cancelled.

**/
EFI_STATUS
EFIAPI
EfiPxeLoadFile (
  IN     EFI_LOAD_FILE_PROTOCOL    *This,
  IN     EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN     BOOLEAN                   BootPolicy,
  IN OUT UINTN                     *BufferSize,
  IN     VOID                      *Buffer       OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA          *Private;
  PXEBC_VIRTUAL_NIC           *VirtualNic;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  BOOLEAN                     UsingIpv6;
  EFI_STATUS                  Status;
  EFI_STATUS                  MediaStatus;

  if ((This == NULL) || (BufferSize == NULL) || (FilePath == NULL) || !IsDevicePathEnd (FilePath)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only support BootPolicy
  //
  if (!BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  VirtualNic = PXEBC_VIRTUAL_NIC_FROM_LOADFILE (This);
  Private    = VirtualNic->Private;
  PxeBc      = &Private->PxeBc;
  UsingIpv6  = FALSE;
  Status     = EFI_DEVICE_ERROR;

  //
  // Check media status before PXE start
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Private->Controller, PXEBC_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    return EFI_NO_MEDIA;
  }

  //
  // Check whether the virtual nic is using IPv6 or not.
  //
  if (VirtualNic == Private->Ip6Nic) {
    UsingIpv6 = TRUE;
  }

  //
  // Start Pxe Base Code to initialize PXE boot.
  //
  Status = PxeBc->Start (PxeBc, UsingIpv6);
  if ((Status == EFI_ALREADY_STARTED) && (UsingIpv6 != PxeBc->Mode->UsingIpv6)) {
    //
    // PxeBc protocol has already been started but not on the required IP version, restart it.
    //
    Status = PxeBc->Stop (PxeBc);
    if (!EFI_ERROR (Status)) {
      Status = PxeBc->Start (PxeBc, UsingIpv6);
    }
  }

  if ((Status == EFI_SUCCESS) || (Status == EFI_ALREADY_STARTED)) {
    Status = PxeBcLoadBootFile (Private, BufferSize, Buffer);
  }

  if ((Status != EFI_SUCCESS) &&
      (Status != EFI_UNSUPPORTED) &&
      (Status != EFI_BUFFER_TOO_SMALL))
  {
    //
    // There are three cases, which needn't stop pxebc here.
    //   1. success to download file.
    //   2. success to get file size.
    //   3. unsupported.
    //
    PxeBc->Stop (PxeBc);
  } else {
    //
    // The DHCP4 can have only one configured child instance so we need to stop
    // reset the DHCP4 child before we return. Otherwise these programs which
    // also need to use DHCP4 will be impacted.
    //
    if (!PxeBc->Mode->UsingIpv6) {
      Private->Dhcp4->Stop (Private->Dhcp4);
      Private->Dhcp4->Configure (Private->Dhcp4, NULL);
    }
  }

  return Status;
}

EFI_LOAD_FILE_PROTOCOL  gLoadFileProtocolTemplate = { EfiPxeLoadFile };
