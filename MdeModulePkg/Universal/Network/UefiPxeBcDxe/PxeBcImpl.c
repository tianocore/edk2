/** @file
  Interface routines for PxeBc.

Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "PxeBcImpl.h"

UINT32  mPxeDhcpTimeout[4] = { 4, 8, 16, 32 };

/**
  Get and record the arp cache.

  @param  This                    Pointer to EFI_PXE_BC_PROTOCOL

  @retval EFI_SUCCESS             Arp cache updated successfully
  @retval others                  If error occurs when getting arp cache

**/
EFI_STATUS
UpdateArpCache (
  IN EFI_PXE_BASE_CODE_PROTOCOL     * This
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;
  UINT32                  EntryLength;
  UINT32                  EntryCount;
  EFI_ARP_FIND_DATA       *Entries;
  UINT32                  Index;

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  Status = Private->Arp->Find (
                     Private->Arp,
                     TRUE,
                     NULL,
                     &EntryLength,
                     &EntryCount,
                     &Entries,
                     TRUE
                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Mode->ArpCacheEntries = MIN (
                           EntryCount,
                           EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES
                           );
  for (Index = 0; Index < Mode->ArpCacheEntries; Index ++) {
    CopyMem (
      &Mode->ArpCache[Index].IpAddr,
      Entries + 1,
      Entries->SwAddressLength
      );
    CopyMem (
      &Mode->ArpCache[Index].MacAddr,
      (UINT8 *) (Entries + 1) + Entries->SwAddressLength,
      Entries->HwAddressLength
      );
    //
    // Slip to the next FindData.
    //
    Entries = (EFI_ARP_FIND_DATA *) ((UINT8 *) Entries + EntryLength);
  }

  return EFI_SUCCESS;
}

/**
  Timeout routine to update arp cache.

  @param  Event              Pointer to EFI_PXE_BC_PROTOCOL
  @param  Context            Context of the timer event

**/
VOID
EFIAPI
ArpCacheUpdateTimeout (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  UpdateArpCache ((EFI_PXE_BASE_CODE_PROTOCOL *) Context);
}

/**
  Do arp resolution from arp cache in PxeBcMode.

  @param  PxeBcMode      The PXE BC mode to look into.
  @param  Ip4Addr        The Ip4 address for resolution.
  @param  MacAddress     The resoluted MAC address if the resolution is successful.
                         The value is undefined if resolution fails.

  @retval TRUE           The resolution is successful.
  @retval FALSE          Otherwise.

**/
BOOLEAN
FindInArpCache (
  IN  EFI_PXE_BASE_CODE_MODE    *PxeBcMode,
  IN  EFI_IPv4_ADDRESS          *Ip4Addr,
  OUT EFI_MAC_ADDRESS           *MacAddress
  )
{
  UINT32                  Index;

  for (Index = 0; Index < PxeBcMode->ArpCacheEntries; Index ++) {
    if (EFI_IP4_EQUAL (&PxeBcMode->ArpCache[Index].IpAddr.v4, Ip4Addr)) {
      CopyMem (
        MacAddress,
        &PxeBcMode->ArpCache[Index].MacAddr,
        sizeof (EFI_MAC_ADDRESS)
        );
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Notify function for the ICMP receive token, used to process
  the received ICMP packets.

  @param  Context               The PXEBC private data.

**/
VOID
EFIAPI
IcmpErrorListenHandlerDpc (
  IN VOID      *Context
  )
{
  EFI_STATUS              Status;
  EFI_IP4_RECEIVE_DATA    *RxData;
  EFI_IP4_PROTOCOL        *Ip4;
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  UINTN                   Index;
  UINT32                  CopiedLen;
  UINT8                   *CopiedPointer;

  Private = (PXEBC_PRIVATE_DATA *) Context;
  Mode    = &Private->Mode;
  Status  = Private->IcmpErrorRcvToken.Status;
  RxData  = Private->IcmpErrorRcvToken.Packet.RxData;
  Ip4     = Private->Ip4;

  if (Status == EFI_ABORTED) {
    //
    // The reception is actively aborted by the consumer, directly return.
    //
    return;
  }

  if (RxData == NULL) {
    goto Resume;
  }

  if (Status != EFI_ICMP_ERROR) {
    //
    // The return status should be recognized as EFI_ICMP_ERROR.
    //
    goto CleanUp;
  }

  if (EFI_IP4 (RxData->Header->SourceAddress) != 0 &&
      (NTOHL (Mode->SubnetMask.Addr[0]) != 0) &&
      IP4_NET_EQUAL (NTOHL(Mode->StationIp.Addr[0]), EFI_NTOHL (RxData->Header->SourceAddress), NTOHL (Mode->SubnetMask.Addr[0])) &&
      !NetIp4IsUnicast (EFI_NTOHL (RxData->Header->SourceAddress), NTOHL (Mode->SubnetMask.Addr[0]))) {
    //
    // The source address is not zero and it's not a unicast IP address, discard it.
    //
    goto CleanUp;
  }

  if (!EFI_IP4_EQUAL (&RxData->Header->DestinationAddress, &Mode->StationIp.v4)) {
    //
    // The dest address is not equal to Station Ip address, discard it.
    //
    goto CleanUp;
  }

  //
  // Constructor ICMP error packet
  //
  CopiedLen = 0;
  CopiedPointer = (UINT8 *) &Mode->IcmpError;

  for (Index = 0; Index < RxData->FragmentCount; Index ++) {
    CopiedLen += RxData->FragmentTable[Index].FragmentLength;
    if (CopiedLen <= sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)) {
      CopyMem (
        CopiedPointer,
        RxData->FragmentTable[Index].FragmentBuffer,
        RxData->FragmentTable[Index].FragmentLength
        );
    } else {
      CopyMem (
        CopiedPointer,
        RxData->FragmentTable[Index].FragmentBuffer,
        CopiedLen - sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)
        );
    }
    CopiedPointer += CopiedLen;
  }

CleanUp:
  gBS->SignalEvent (RxData->RecycleSignal);

Resume:
  Ip4->Receive (Ip4, &(Private->IcmpErrorRcvToken));
}

/**
  Request IcmpErrorListenHandlerDpc as a DPC at TPL_CALLBACK

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IcmpErrorListenHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoListenHandlerDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, IcmpErrorListenHandlerDpc, Context);
}

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
  PXE Base Code Protocol will be started, and all of the fields of the EFI_PXE_BASE_CODE_MODE
  structure will be initialized as follows:
    StartedSet to TRUE.
    Ipv6SupportedUnchanged.
    Ipv6AvailableUnchanged.
    UsingIpv6Set to UseIpv6.
    BisSupportedUnchanged.
    BisDetectedUnchanged.
    AutoArpSet to TRUE.
    SendGUIDSet to FALSE.
    TTLSet to DEFAULT_TTL.
    ToSSet to DEFAULT_ToS.
    DhcpCompletedSet to FALSE.
    ProxyOfferReceivedSet to FALSE.
    StationIpSet to an address of all zeros.
    SubnetMaskSet to a subnet mask of all zeros.
    DhcpDiscoverZero-filled.
    DhcpAckZero-filled.
    ProxyOfferZero-filled.
    PxeDiscoverValidSet to FALSE.
    PxeDiscoverZero-filled.
    PxeReplyValidSet to FALSE.
    PxeReplyZero-filled.
    PxeBisReplyValidSet to FALSE.
    PxeBisReplyZero-filled.
    IpFilterSet the Filters field to 0 and the IpCnt field to 0.
    ArpCacheEntriesSet to 0.
    ArpCacheZero-filled.
    RouteTableEntriesSet to 0.
    RouteTableZero-filled.
    IcmpErrorReceivedSet to FALSE.
    IcmpErrorZero-filled.
    TftpErroReceivedSet to FALSE.
    TftpErrorZero-filled.
    MakeCallbacksSet to TRUE if the PXE Base Code Callback Protocol is available.
    Set to FALSE if the PXE Base Code Callback Protocol is not available.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  UseIpv6               Specifies the type of IP addresses that are to be used during the session
                                that is being started. Set to TRUE for IPv6 addresses, and FALSE for
                                IPv4 addresses.

  @retval EFI_SUCCESS           The PXE Base Code Protocol was started.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this oper
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
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN BOOLEAN                          UseIpv6
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (Mode->Started) {
    return EFI_ALREADY_STARTED;
  }

  if (UseIpv6) {
    //
    // IPv6 is not supported now.
    //
    return EFI_UNSUPPORTED;
  }

  AsciiPrint ("\n>>Start PXE over IPv4");

  //
  // Configure the udp4 instance to let it receive data
  //
  Status = Private->Udp4Read->Configure (
                               Private->Udp4Read,
                               &Private->Udp4CfgData
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  //
  // Configure block size for TFTP as a default value to handle all link layers.
  // 
  Private->BlockSize   = MIN (Private->Ip4MaxPacketSize, PXEBC_DEFAULT_PACKET_SIZE) -
                           PXEBC_DEFAULT_UDP_OVERHEAD_SIZE - PXEBC_DEFAULT_TFTP_OVERHEAD_SIZE;
  //
  // If PcdTftpBlockSize is set to non-zero, override the default value.
  //
  if (PcdGet64 (PcdTftpBlockSize) != 0) {
    Private->BlockSize   = (UINTN) PcdGet64 (PcdTftpBlockSize);
  }
  
  Private->AddressIsOk = FALSE;

  ZeroMem (Mode, sizeof (EFI_PXE_BASE_CODE_MODE));

  Mode->Started = TRUE;
  Mode->TTL     = DEFAULT_TTL;
  Mode->ToS     = DEFAULT_ToS;
  Mode->AutoArp = TRUE;

  //
  // Create the event for Arp Cache checking.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ArpCacheUpdateTimeout,
                  This,
                  &Private->GetArpCacheEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Start the timeout timer event.
  //
  Status = gBS->SetTimer (
                  Private->GetArpCacheEvent,
                  TimerPeriodic,
                  TICKS_PER_SECOND
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Create ICMP error receiving event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IcmpErrorListenHandler,
                  Private,
                  &(Private->IcmpErrorRcvToken.Event)
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  //DHCP4 service allows only one of its children to be configured in  
  //the active state, If the DHCP4 D.O.R.A started by IP4 auto  
  //configuration and has not been completed, the Dhcp4 state machine 
  //will not be in the right state for the PXE to start a new round D.O.R.A. 
  //so we need to switch it's policy to static.
  //
  Status = PxeBcSetIp4Policy (Private);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
    
  Status = Private->Ip4->Configure (Private->Ip4, &Private->Ip4ConfigData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // start to listen incoming packet
  //
  Status = Private->Ip4->Receive (Private->Ip4, &Private->IcmpErrorRcvToken);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

ON_EXIT:
  Private->Ip4->Configure (Private->Ip4, NULL);

  if (Private->IcmpErrorRcvToken.Event != NULL) {
    gBS->CloseEvent (Private->IcmpErrorRcvToken.Event);
  }

  if (Private->GetArpCacheEvent != NULL) {
    gBS->SetTimer (Private->GetArpCacheEvent, TimerCancel, 0);
    gBS->CloseEvent (Private->GetArpCacheEvent);
  }

  Mode->Started = FALSE;
  Mode->TTL     = 0;
  Mode->ToS     = 0;
  Mode->AutoArp = FALSE;

  return Status;
}


/**
  Disables the use of the PXE Base Code Protocol functions.

  This function stops all activity on the network device. All the resources allocated
  in Start() are released, the Started field of the EFI_PXE_BASE_CODE_MODE structure is
  set to FALSE and EFI_SUCCESS is returned. If the Started field of the EFI_PXE_BASE_CODE_MODE
  structure is already FALSE, then EFI_NOT_STARTED will be returned.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.

  @retval EFI_SUCCESS           The PXE Base Code Protocol was stopped.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is already in the stopped state.
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.

**/
EFI_STATUS
EFIAPI
EfiPxeBcStop (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This
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

  Private->Ip4->Cancel (Private->Ip4, NULL);
  //
  // Dispatch the DPCs queued by the NotifyFunction of the canceled rx token's
  // events.
  //
  DispatchDpc ();

  Private->Ip4->Configure (Private->Ip4, NULL);

  //
  // Close the ICMP error receiving event.
  //
  gBS->CloseEvent (Private->IcmpErrorRcvToken.Event);

  //
  // Cancel the TimeoutEvent timer.
  //
  gBS->SetTimer (Private->GetArpCacheEvent, TimerCancel, 0);

  //
  // Close the TimeoutEvent event.
  //
  gBS->CloseEvent (Private->GetArpCacheEvent);

  Mode->Started = FALSE;

  Private->CurrentUdpSrcPort = 0;
  Private->Udp4Write->Configure (Private->Udp4Write, NULL);
  Private->Udp4Read->Groups (Private->Udp4Read, FALSE, NULL);
  Private->Udp4Read->Configure (Private->Udp4Read, NULL);

  Private->Dhcp4->Stop (Private->Dhcp4);
  Private->Dhcp4->Configure (Private->Dhcp4, NULL);

  Private->FileSize = 0;

  return EFI_SUCCESS;
}


/**
  Attempts to complete a DHCPv4 D.O.R.A. (discover / offer / request / acknowledge) or DHCPv6
  S.A.R.R (solicit / advertise / request / reply) sequence.

  This function attempts to complete the DHCP sequence. If this sequence is completed,
  then EFI_SUCCESS is returned, and the DhcpCompleted, ProxyOfferReceived, StationIp,
  SubnetMask, DhcpDiscover, DhcpAck, and ProxyOffer fields of the EFI_PXE_BASE_CODE_MODE
  structure are filled in.
  If SortOffers is TRUE, then the cached DHCP offer packets will be sorted before
  they are tried. If SortOffers is FALSE, then the cached DHCP offer packets will
  be tried in the order in which they are received. Please see the Preboot Execution
  Environment (PXE) Specification for additional details on the implementation of DHCP.
  This function can take at least 31 seconds to timeout and return control to the
  caller. If the DHCP sequence does not complete, then EFI_TIMEOUT will be returned.
  If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then the DHCP sequence will be stopped and EFI_ABORTED will be returned.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  SortOffers            TRUE if the offers received should be sorted. Set to FALSE to try the
                                offers in the order that they are received.

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
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN BOOLEAN                          SortOffers
  )
{
  PXEBC_PRIVATE_DATA           *Private;
  EFI_PXE_BASE_CODE_MODE       *Mode;
  EFI_DHCP4_PROTOCOL           *Dhcp4;
  EFI_DHCP4_CONFIG_DATA        Dhcp4CfgData;
  EFI_DHCP4_MODE_DATA          Dhcp4Mode;
  EFI_DHCP4_PACKET_OPTION      *OptList[PXEBC_DHCP4_MAX_OPTION_NUM];
  UINT32                       OptCount;
  EFI_STATUS                   Status;
  EFI_ARP_CONFIG_DATA          ArpConfigData;
  EFI_PXE_BASE_CODE_IP_FILTER  IpFilter;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  Private             = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode                = Private->PxeBc.Mode;
  Dhcp4               = Private->Dhcp4;
  Private->Function   = EFI_PXE_BASE_CODE_FUNCTION_DHCP;
  Private->SortOffers = SortOffers;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  Mode->IcmpErrorReceived = FALSE;

  //
  // Stop Udp4Read instance
  //
  Private->Udp4Read->Configure (Private->Udp4Read, NULL);

  //
  // Initialize the DHCP options and build the option list
  //
  OptCount = PxeBcBuildDhcpOptions (Private, OptList, TRUE);

  //
  // Set the DHCP4 config data.
  // The four discovery timeouts are 4, 8, 16, 32 seconds respectively.
  //
  ZeroMem (&Dhcp4CfgData, sizeof (EFI_DHCP4_CONFIG_DATA));
  Dhcp4CfgData.OptionCount      = OptCount;
  Dhcp4CfgData.OptionList       = OptList;
  Dhcp4CfgData.Dhcp4Callback    = PxeBcDhcpCallBack;
  Dhcp4CfgData.CallbackContext  = Private;
  Dhcp4CfgData.DiscoverTryCount = 4;
  Dhcp4CfgData.DiscoverTimeout  = mPxeDhcpTimeout;

  Status          = Dhcp4->Configure (Dhcp4, &Dhcp4CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Zero those arrays to record the varies numbers of DHCP OFFERS.
  //
  Private->GotProxyOffer = FALSE;
  Private->NumOffers     = 0;
  Private->BootpIndex    = 0;
  ZeroMem (Private->ServerCount, sizeof (Private->ServerCount));
  ZeroMem (Private->ProxyIndex, sizeof (Private->ProxyIndex));

  Status = Dhcp4->Start (Dhcp4, NULL);
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    if (Status == EFI_ICMP_ERROR) {
      Mode->IcmpErrorReceived = TRUE;
    }
    goto ON_EXIT;
  }

  Status = Dhcp4->GetModeData (Dhcp4, &Dhcp4Mode);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ASSERT (Dhcp4Mode.State == Dhcp4Bound);

  CopyMem (&Private->StationIp, &Dhcp4Mode.ClientAddress, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Private->SubnetMask, &Dhcp4Mode.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Private->GatewayIp, &Dhcp4Mode.RouterAddress, sizeof (EFI_IPv4_ADDRESS));

  CopyMem (&Mode->StationIp, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Mode->SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));

  //
  // Check the selected offer to see whether BINL is required, if no or BINL is
  // finished, set the various Mode members.
  //
  Status = PxeBcCheckSelectedOffer (Private);

  AsciiPrint ("\n  Station IP address is ");

  PxeBcShowIp4Addr (&Private->StationIp.v4);
  AsciiPrint ("\n");

ON_EXIT:
  if (EFI_ERROR (Status)) {
    Dhcp4->Stop (Dhcp4);
    Dhcp4->Configure (Dhcp4, NULL);
  } else {
    //
    // Remove the previously configured option list and callback function
    //
    ZeroMem (&Dhcp4CfgData, sizeof (EFI_DHCP4_CONFIG_DATA));
    Dhcp4->Configure (Dhcp4, &Dhcp4CfgData);

    Private->AddressIsOk = TRUE;

    if (!Mode->UsingIpv6) {
      //
      // If in IPv4 mode, configure the corresponding ARP with this new
      // station IP address.
      //
      ZeroMem (&ArpConfigData, sizeof (EFI_ARP_CONFIG_DATA));

      ArpConfigData.SwAddressType   = 0x0800;
      ArpConfigData.SwAddressLength = (UINT8) sizeof (EFI_IPv4_ADDRESS);
      ArpConfigData.StationAddress  = &Private->StationIp.v4;

      Private->Arp->Configure (Private->Arp, NULL);
      Private->Arp->Configure (Private->Arp, &ArpConfigData);

      //
      // Updated the route table. Fill the first entry.
      //
      Mode->RouteTableEntries                = 1;
      Mode->RouteTable[0].IpAddr.Addr[0]     = Private->StationIp.Addr[0] & Private->SubnetMask.Addr[0];
      Mode->RouteTable[0].SubnetMask.Addr[0] = Private->SubnetMask.Addr[0];
      Mode->RouteTable[0].GwAddr.Addr[0]     = 0;

      //
      // Create the default route entry if there is a default router.
      //
      if (Private->GatewayIp.Addr[0] != 0) {
        Mode->RouteTableEntries                = 2;
        Mode->RouteTable[1].IpAddr.Addr[0]     = 0;
        Mode->RouteTable[1].SubnetMask.Addr[0] = 0;
        Mode->RouteTable[1].GwAddr.Addr[0]     = Private->GatewayIp.Addr[0];
      }

      //
      // Flush new station IP address into Udp4CfgData and Ip4ConfigData
      //
      CopyMem (&Private->Udp4CfgData.StationAddress, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Private->Udp4CfgData.SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Private->Ip4ConfigData.StationAddress, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Private->Ip4ConfigData.SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  
      //
      // Reconfigure the Ip4 instance to capture background ICMP packets with new station Ip address.
      //
      Private->Ip4->Cancel (Private->Ip4, &Private->IcmpErrorRcvToken);
      Private->Ip4->Configure (Private->Ip4, NULL);
  
      Status = Private->Ip4->Configure (Private->Ip4, &Private->Ip4ConfigData);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
  
      Status = Private->Ip4->Receive (Private->Ip4, &Private->IcmpErrorRcvToken);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      } 
    }
  }

  Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);

  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP 
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem(&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  Type                  The type of bootstrap to perform.
  @param  Layer                 Pointer to the boot server layer number to discover, which must be
                                PXE_BOOT_LAYER_INITIAL when a new server type is being
                                discovered.
  @param  UseBis                TRUE if Boot Integrity Services are to be used. FALSE otherwise.
  @param  Info                  Pointer to a data structure that contains additional information on the
                                type of discovery operation that is to be performed.

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
  PXEBC_PRIVATE_DATA              *Private;
  EFI_PXE_BASE_CODE_MODE          *Mode;
  EFI_PXE_BASE_CODE_DISCOVER_INFO DefaultInfo;
  EFI_PXE_BASE_CODE_DISCOVER_INFO *CreatedInfo;
  EFI_PXE_BASE_CODE_SRVLIST       *SrvList;
  EFI_PXE_BASE_CODE_SRVLIST       DefaultSrvList;
  PXEBC_CACHED_DHCP4_PACKET       *Packet;
  PXEBC_VENDOR_OPTION             *VendorOpt;
  UINT16                          Index;
  EFI_STATUS                      Status;
  PXEBC_BOOT_SVR_ENTRY            *BootSvrEntry;
  EFI_PXE_BASE_CODE_IP_FILTER     IpFilter;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private           = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode              = Private->PxeBc.Mode;
  BootSvrEntry      = NULL;
  SrvList           = NULL;
  CreatedInfo       = NULL;
  Status            = EFI_DEVICE_ERROR;
  Private->Function = EFI_PXE_BASE_CODE_FUNCTION_DISCOVER;

  if (!Private->AddressIsOk) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  //
  // Stop Udp4Read instance
  //
  Private->Udp4Read->Configure (Private->Udp4Read, NULL);

  Mode->IcmpErrorReceived = FALSE;

  //
  // If layer isn't EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL,
  //   use the previous setting;
  // If info isn't offered,
  //   use the cached DhcpAck and ProxyOffer packets.
  //
  ZeroMem (&DefaultInfo, sizeof (EFI_PXE_BASE_CODE_DISCOVER_INFO));
  if (*Layer != EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL) {

    if (!Mode->PxeDiscoverValid || !Mode->PxeReplyReceived || (!Mode->PxeBisReplyReceived && UseBis)) {

      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;  
    }

    DefaultInfo.IpCnt                 = 1;
    DefaultInfo.UseUCast              = TRUE;

    DefaultSrvList.Type               = Type;
    DefaultSrvList.AcceptAnyResponse  = FALSE;
    DefaultSrvList.IpAddr.Addr[0]     = Private->ServerIp.Addr[0];

    SrvList = &DefaultSrvList;
    Info = &DefaultInfo;
  } else if (Info == NULL) {
    //
    // Create info by the cached packet before
    //
    Packet    = (Mode->ProxyOfferReceived) ? &Private->ProxyOffer : &Private->Dhcp4Ack;
    VendorOpt = &Packet->PxeVendorOption;

    if (!Mode->DhcpAckReceived || !IS_VALID_DISCOVER_VENDOR_OPTION (VendorOpt->BitMap)) {
      //
      // Address is not acquired or no discovery options.
      //
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;  
    }

    DefaultInfo.UseMCast    = (BOOLEAN)!IS_DISABLE_MCAST_DISCOVER (VendorOpt->DiscoverCtrl);
    DefaultInfo.UseBCast    = (BOOLEAN)!IS_DISABLE_BCAST_DISCOVER (VendorOpt->DiscoverCtrl);
    DefaultInfo.MustUseList = (BOOLEAN) IS_ENABLE_USE_SERVER_LIST (VendorOpt->DiscoverCtrl);
    DefaultInfo.UseUCast    = DefaultInfo.MustUseList;

    if (DefaultInfo.UseMCast) {
      //
      // Get the multicast discover ip address from vendor option.
      //
      CopyMem (
        &DefaultInfo.ServerMCastIp.Addr,
        &VendorOpt->DiscoverMcastIp,
        sizeof (EFI_IPv4_ADDRESS)
        );
    }

    DefaultInfo.IpCnt = 0;
    Info    = &DefaultInfo;
    SrvList = Info->SrvList;

    if (DefaultInfo.MustUseList) {
      BootSvrEntry  = VendorOpt->BootSvr;
      Status        = EFI_INVALID_PARAMETER;

      while (((UINT8) (BootSvrEntry - VendorOpt->BootSvr)) < VendorOpt->BootSvrLen) {

        if (BootSvrEntry->Type == HTONS (Type)) {
          Status = EFI_SUCCESS;
          break;
        }

        BootSvrEntry = GET_NEXT_BOOT_SVR_ENTRY (BootSvrEntry);
      }

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      DefaultInfo.IpCnt = BootSvrEntry->IpCnt;

      if (DefaultInfo.IpCnt >= 1) {
        CreatedInfo = AllocatePool (sizeof (DefaultInfo) + (DefaultInfo.IpCnt - 1) * sizeof (*SrvList));
        if (CreatedInfo == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto ON_EXIT;
          
        }     
      
        CopyMem (CreatedInfo, &DefaultInfo, sizeof (DefaultInfo));
        Info    = CreatedInfo;
        SrvList = Info->SrvList;
      }

      for (Index = 0; Index < DefaultInfo.IpCnt; Index++) {
        CopyMem (&SrvList[Index].IpAddr, &BootSvrEntry->IpAddr[Index], sizeof (EFI_IPv4_ADDRESS));
        SrvList[Index].AcceptAnyResponse   = FALSE;
        SrvList[Index].Type                = BootSvrEntry->Type;
      }
    }

  } else {

    SrvList = Info->SrvList;

    if (!SrvList[0].AcceptAnyResponse) {

      for (Index = 1; Index < Info->IpCnt; Index++) {
        if (SrvList[Index].AcceptAnyResponse) {
          break;
        }
      }

      if (Index != Info->IpCnt) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;        
      }
    }
  }

  if ((!Info->UseUCast && !Info->UseBCast && !Info->UseMCast) || (Info->MustUseList && Info->IpCnt == 0)) {

    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  //
  // Execute discover by UniCast/BroadCast/MultiCast
  //
  if (Info->UseUCast) {

    for (Index = 0; Index < Info->IpCnt; Index++) {

      if (BootSvrEntry == NULL) {
        Private->ServerIp.Addr[0] = SrvList[Index].IpAddr.Addr[0];
      } else {
        CopyMem (
          &Private->ServerIp,
          &BootSvrEntry->IpAddr[Index],
          sizeof (EFI_IPv4_ADDRESS)
          );
      }

      Status = PxeBcDiscvBootService (
                Private,
                Type,
                Layer,
                UseBis,
                &SrvList[Index].IpAddr,
                0,
                NULL,
                TRUE,
                &Private->PxeReply.Packet.Ack
                );
      if (!EFI_ERROR (Status)) {
        break;
      }                
    }

  } else if (Info->UseMCast) {

    Status = PxeBcDiscvBootService (
              Private,
              Type,
              Layer,
              UseBis,
              &Info->ServerMCastIp,
              0,
              NULL,
              TRUE,
              &Private->PxeReply.Packet.Ack
              );

  } else if (Info->UseBCast) {

    Status = PxeBcDiscvBootService (
              Private,
              Type,
              Layer,
              UseBis,
              NULL,
              Info->IpCnt,
              SrvList,
              TRUE,
              &Private->PxeReply.Packet.Ack
              );
  }

  if (EFI_ERROR (Status) || !Mode->PxeReplyReceived || (!Mode->PxeBisReplyReceived && UseBis)) {
    if (Status == EFI_ICMP_ERROR) {
      Mode->IcmpErrorReceived = TRUE;
    } else {
      Status = EFI_DEVICE_ERROR;
    }
    goto ON_EXIT;
  } else {
    PxeBcParseCachedDhcpPacket (&Private->PxeReply);
  }

  if (Mode->PxeBisReplyReceived) {
    CopyMem (
      &Private->ServerIp,
      &Mode->PxeReply.Dhcpv4.BootpSiAddr,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }

  if (CreatedInfo != NULL) {
    FreePool (CreatedInfo);
  }

ON_EXIT:

  Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);
  
  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP 
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem(&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
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
  then EFI_BUFFER_TOO_SMALL will be returned and BufferSize will be set to zero
  or the size of the requested file (the size of the requested file is only returned
  if the TFTP server supports TFTP options). If BufferSize is large enough for the
  read operation, then BufferSize will be set to the size of the downloaded file,
  and EFI_SUCCESS will be returned. Applications using the PxeBc.Mtftp() services
  should use the get-file-size operations to determine the size of the downloaded
  file prior to using the read-file operations-especially when downloading large
  (greater than 64 MB) files-instead of making two calls to the read-file operation.
  Following this recommendation will save time if the file is larger than expected
  and the TFTP server does not support TFTP option extensions. Without TFTP option
  extension support, the client has to download the entire file, counting and discarding
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
  The format of the data returned from a TFTP read directory operation is a null-terminated
  filename followed by a null-terminated information string, of the form
  "size year-month-day hour:minute:second" (i.e. %d %d-%d-%d %d:%d:%f - note that
  the seconds field can be a decimal number), where the date and time are UTC. For
  an MTFTP read directory command, there is additionally a null-terminated multicast
  IP address preceding the filename of the form %d.%d.%d.%d for IP v4. The final
  entry is itself null-terminated, so that the final information string is terminated
  with two null octets.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  Operation             The type of operation to perform.
  @param  BufferPtr             A pointer to the data buffer.
  @param  Overwrite             Only used on write file operations. TRUE if a file on a remote server can
                                be overwritten.
  @param  BufferSize            For get-file-size operations, *BufferSize returns the size of the
                                requested file.
  @param  BlockSize             The requested block size to be used during a TFTP transfer.
  @param  ServerIp              The TFTP / MTFTP server IP address.
  @param  Filename              A Null-terminated ASCII string that specifies a directory name or a file
                                name.
  @param  Info                  Pointer to the MTFTP information.
  @param  DontUseBuffer         Set to FALSE for normal TFTP and MTFTP read file operation.

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
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE    Operation,
  IN OUT VOID                         *BufferPtr,
  IN BOOLEAN                          Overwrite,
  IN OUT UINT64                       *BufferSize,
  IN UINTN                            *BlockSize    OPTIONAL,
  IN EFI_IP_ADDRESS                   *ServerIp,
  IN UINT8                            *Filename,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO     *Info         OPTIONAL,
  IN BOOLEAN                          DontUseBuffer
  )
{
  PXEBC_PRIVATE_DATA           *Private;
  EFI_MTFTP4_CONFIG_DATA       Mtftp4Config;
  EFI_STATUS                   Status;
  EFI_PXE_BASE_CODE_MODE       *Mode;
  EFI_MAC_ADDRESS              TempMacAddr;
  EFI_PXE_BASE_CODE_IP_FILTER  IpFilter;

  if ((This == NULL)                                                          ||
      (Filename == NULL)                                                      ||
      (BufferSize == NULL)                                                    ||
      ((ServerIp == NULL) || 
       (IP4_IS_UNSPECIFIED (NTOHL (ServerIp->Addr[0])) || 
        IP4_IS_LOCAL_BROADCAST (NTOHL (ServerIp->Addr[0]))))                  ||
      ((BufferPtr == NULL) && DontUseBuffer)                                  ||
      ((BlockSize != NULL) && (*BlockSize < 512))) {

    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_DEVICE_ERROR;
  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = &Private->Mode;

  if (!Mode->AutoArp) {
    //
    // If AutoArp is set false, check arp cache
    //
    UpdateArpCache (This);
    if (!FindInArpCache (Mode, &ServerIp->v4, &TempMacAddr)) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Stop Udp4Read instance
  //
  Private->Udp4Read->Configure (Private->Udp4Read, NULL);

  Mode->TftpErrorReceived = FALSE;
  Mode->IcmpErrorReceived = FALSE;

  Mtftp4Config.UseDefaultSetting = FALSE;
  Mtftp4Config.TimeoutValue      = PXEBC_MTFTP_TIMEOUT;
  Mtftp4Config.TryCount          = PXEBC_MTFTP_RETRIES;

  CopyMem (
    &Mtftp4Config.StationIp,
    &Private->StationIp,
    sizeof (EFI_IPv4_ADDRESS)
    );
  CopyMem (
    &Mtftp4Config.SubnetMask,
    &Private->SubnetMask,
    sizeof (EFI_IPv4_ADDRESS)
    );
  CopyMem (
    &Mtftp4Config.GatewayIp,
    &Private->GatewayIp,
    sizeof (EFI_IPv4_ADDRESS)
    );
  CopyMem (
    &Mtftp4Config.ServerIp,
    ServerIp,
    sizeof (EFI_IPv4_ADDRESS)
    );

  switch (Operation) {

  case EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE:

    Status = PxeBcTftpGetFileSize (
              Private,
              &Mtftp4Config,
              Filename,
              BlockSize,
              BufferSize
              );

    break;

  case EFI_PXE_BASE_CODE_TFTP_READ_FILE:

    Status = PxeBcTftpReadFile (
              Private,
              &Mtftp4Config,
              Filename,
              BlockSize,
              BufferPtr,
              BufferSize,
              DontUseBuffer
              );

    break;

  case EFI_PXE_BASE_CODE_TFTP_WRITE_FILE:

    Status = PxeBcTftpWriteFile (
              Private,
              &Mtftp4Config,
              Filename,
              Overwrite,
              BlockSize,
              BufferPtr,
              BufferSize
              );

    break;

  case EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY:

    Status = PxeBcTftpReadDirectory (
              Private,
              &Mtftp4Config,
              Filename,
              BlockSize,
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

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

ON_EXIT:
  Private->Udp4Read->Configure (Private->Udp4Read, &Private->Udp4CfgData);
  //
  // Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP 
  // receive filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  //
  ZeroMem(&IpFilter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));
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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  OpFlags               The UDP operation flags.
  @param  DestIp                The destination IP address.
  @param  DestPort              The destination UDP port number.
  @param  GatewayIp             The gateway IP address.
  @param  SrcIp                 The source IP address.
  @param  SrcPort               The source UDP port number.
  @param  HeaderSize            An optional field which may be set to the length of a header at
                                HeaderPtr to be prefixed to the data at BufferPtr.
  @param  HeaderPtr             If HeaderSize is not NULL, a pointer to a header to be prefixed to the
                                data at BufferPtr.
  @param  BufferSize            A pointer to the size of the data at BufferPtr.
  @param  BufferPtr             A pointer to the data to be written.

  @retval EFI_SUCCESS           The UDP Write operation was completed.
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
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           OpFlags,
  IN EFI_IP_ADDRESS                   *DestIp,
  IN EFI_PXE_BASE_CODE_UDP_PORT       *DestPort,
  IN EFI_IP_ADDRESS                   *GatewayIp  OPTIONAL,
  IN EFI_IP_ADDRESS                   *SrcIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *SrcPort    OPTIONAL,
  IN UINTN                            *HeaderSize OPTIONAL,
  IN VOID                             *HeaderPtr  OPTIONAL,
  IN UINTN                            *BufferSize,
  IN VOID                             *BufferPtr
  )
{
  PXEBC_PRIVATE_DATA        *Private;
  EFI_UDP4_PROTOCOL         *Udp4;
  EFI_UDP4_COMPLETION_TOKEN Token;
  EFI_UDP4_TRANSMIT_DATA    *Udp4TxData;
  UINT32                    FragCount;
  UINT32                    DataLength;
  EFI_UDP4_SESSION_DATA     Udp4Session;
  EFI_STATUS                Status;
  BOOLEAN                   IsDone;
  EFI_PXE_BASE_CODE_MODE    *Mode;
  EFI_MAC_ADDRESS           TempMacAddr;

  IsDone = FALSE;

  if ((This == NULL) || (DestIp == NULL) || (DestPort == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((GatewayIp != NULL) && (IP4_IS_UNSPECIFIED (NTOHL (GatewayIp->Addr[0])) || IP4_IS_LOCAL_BROADCAST (NTOHL (GatewayIp->Addr[0])))) {
    //
    // Gateway is provided but it's not a unicast IP address.
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((HeaderSize != NULL) && ((*HeaderSize == 0) || (HeaderPtr == NULL))) {
    //
    // The HeaderSize ptr isn't NULL and: 1. the value is zero; or 2. the HeaderPtr
    // is NULL.
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferSize == NULL) || ((*BufferSize != 0) && (BufferPtr == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Udp4    = Private->Udp4Write;
  Mode    = &Private->Mode;
  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (!Private->AddressIsOk && (SrcIp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->AutoArp) {
    //
    // If AutoArp is set false, check arp cache
    //
    UpdateArpCache (This);
    if (!FindInArpCache (Mode, &DestIp->v4, &TempMacAddr)) {
      return EFI_DEVICE_ERROR;
    }
  }

  Mode->IcmpErrorReceived = FALSE;

  if ((Private->CurrentUdpSrcPort == 0) ||
      ((SrcPort != NULL) && (*SrcPort != Private->CurrentUdpSrcPort))) {
    //
    // Port is changed, (re)configure the Udp4Write instance
    //
    if (SrcPort != NULL) {
      Private->CurrentUdpSrcPort = *SrcPort;
    }
  }

  Status = PxeBcConfigureUdpWriteInstance (
             Udp4,
             &Private->StationIp.v4,
             &Private->SubnetMask.v4,
             &Private->GatewayIp.v4,
             &Private->CurrentUdpSrcPort,
             Private->Mode.TTL,
             Private->Mode.ToS
             );
  if (EFI_ERROR (Status)) {
    Private->CurrentUdpSrcPort = 0;
    return EFI_INVALID_PARAMETER;
  }
  
  ZeroMem (&Token, sizeof (EFI_UDP4_COMPLETION_TOKEN));
  ZeroMem (&Udp4Session, sizeof (EFI_UDP4_SESSION_DATA));

  CopyMem (&Udp4Session.DestinationAddress, DestIp, sizeof (EFI_IPv4_ADDRESS));
  Udp4Session.DestinationPort = *DestPort;
  if (SrcIp != NULL) {
    CopyMem (&Udp4Session.SourceAddress, SrcIp, sizeof (EFI_IPv4_ADDRESS));
  }
  if (SrcPort != NULL) {
    Udp4Session.SourcePort = *SrcPort;
  }

  FragCount = (HeaderSize != NULL) ? 2 : 1;
  Udp4TxData = (EFI_UDP4_TRANSMIT_DATA *) AllocateZeroPool (sizeof (EFI_UDP4_TRANSMIT_DATA) + (FragCount - 1) * sizeof (EFI_UDP4_FRAGMENT_DATA));
  if (Udp4TxData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Udp4TxData->FragmentCount = FragCount;
  Udp4TxData->FragmentTable[FragCount - 1].FragmentLength = (UINT32) *BufferSize;
  Udp4TxData->FragmentTable[FragCount - 1].FragmentBuffer = BufferPtr;
  DataLength = (UINT32) *BufferSize;

  if (FragCount == 2) {

    Udp4TxData->FragmentTable[0].FragmentLength = (UINT32) *HeaderSize;
    Udp4TxData->FragmentTable[0].FragmentBuffer = HeaderPtr;
    DataLength += (UINT32) *HeaderSize;
  }

  if (GatewayIp != NULL) {
    Udp4TxData->GatewayAddress  = (EFI_IPv4_ADDRESS *) GatewayIp;
  }
  Udp4TxData->UdpSessionData  = &Udp4Session;
  Udp4TxData->DataLength      = DataLength;
  Token.Packet.TxData         = Udp4TxData;

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
    if (Status == EFI_ICMP_ERROR) {
      Mode->IcmpErrorReceived = TRUE;
    }
    goto ON_EXIT;
  }

  while (!IsDone) {

    Udp4->Poll (Udp4);
  }

  Status = Token.Status;

ON_EXIT:

  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }

  FreePool (Udp4TxData);

  //
  // Reset the instance.
  //
  Udp4->Configure (Udp4, NULL);
  return Status;
}

/**
  Decide whether the incoming UDP packet is acceptable per IP filter settings
  in provided PxeBcMode.

  @param  PxeBcMode          Pointer to EFI_PXE_BASE_CODE_MODE.
  @param  Session            Received UDP session.

  @retval TRUE               The UDP package matches IP filters.
  @retval FALSE              The UDP package doesn't matches IP filters.

**/
BOOLEAN
CheckIpByFilter (
  IN EFI_PXE_BASE_CODE_MODE    *PxeBcMode,
  IN EFI_UDP4_SESSION_DATA     *Session
  )
{
  UINTN                   Index;
  EFI_IPv4_ADDRESS        Ip4Address;
  EFI_IPv4_ADDRESS        DestIp4Address;

  if ((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) != 0) {
    return TRUE;
  }

  CopyMem (&DestIp4Address, &Session->DestinationAddress, sizeof (DestIp4Address));
  if (((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) != 0) &&
      IP4_IS_MULTICAST (EFI_NTOHL (DestIp4Address))
      ) {
    return TRUE;
  }

  if (((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST) != 0) &&
      IP4_IS_LOCAL_BROADCAST (EFI_NTOHL (DestIp4Address))
      ) {
    return TRUE;
  }

  CopyMem (&Ip4Address, &PxeBcMode->StationIp.v4, sizeof (Ip4Address));
  if (((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0) &&
      EFI_IP4_EQUAL (&Ip4Address, &DestIp4Address)
      ) {
    return TRUE;
  }

  ASSERT (PxeBcMode->IpFilter.IpCnt < EFI_PXE_BASE_CODE_MAX_IPCNT);

  for (Index = 0; Index < PxeBcMode->IpFilter.IpCnt; Index++) {
    CopyMem (
      &Ip4Address,
      &PxeBcMode->IpFilter.IpList[Index].v4,
      sizeof (Ip4Address)
      );
    if (EFI_IP4_EQUAL (&Ip4Address, &DestIp4Address)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Reads a UDP packet from the network interface.

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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  OpFlags               The UDP operation flags.
  @param  DestIp                The destination IP address.
  @param  DestPort              The destination UDP port number.
  @param  SrcIp                 The source IP address.
  @param  SrcPort               The source UDP port number.
  @param  HeaderSize            An optional field which may be set to the length of a header at
                                HeaderPtr to be prefixed to the data at BufferPtr.
  @param  HeaderPtr             If HeaderSize is not NULL, a pointer to a header to be prefixed to the
                                data at BufferPtr.
  @param  BufferSize            A pointer to the size of the data at BufferPtr.
  @param  BufferPtr             A pointer to the data to be read.

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
  IN EFI_PXE_BASE_CODE_PROTOCOL                *This,
  IN UINT16                                    OpFlags,
  IN OUT EFI_IP_ADDRESS                        *DestIp     OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT            *DestPort   OPTIONAL,
  IN OUT EFI_IP_ADDRESS                        *SrcIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT            *SrcPort    OPTIONAL,
  IN UINTN                                     *HeaderSize OPTIONAL,
  IN VOID                                      *HeaderPtr  OPTIONAL,
  IN OUT UINTN                                 *BufferSize,
  IN VOID                                      *BufferPtr
  )
{
  PXEBC_PRIVATE_DATA        *Private;
  EFI_PXE_BASE_CODE_MODE    *Mode;
  EFI_UDP4_PROTOCOL         *Udp4;
  EFI_UDP4_COMPLETION_TOKEN Token;
  EFI_UDP4_RECEIVE_DATA     *RxData;
  EFI_UDP4_SESSION_DATA     *Session;
  EFI_STATUS                Status;
  BOOLEAN                   IsDone;
  BOOLEAN                   Matched;
  UINTN                     CopiedLen;
  UINTN                     HeaderLen;
  UINTN                     HeaderCopiedLen;
  UINTN                     BufferCopiedLen;
  UINT32                    FragmentLength;
  UINTN                     FragmentIndex;
  UINT8                     *FragmentBuffer;

  if (This == NULL || DestIp == NULL || DestPort == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT) == 0 && (DestPort == NULL)) ||
      ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT) == 0 && (SrcIp == NULL)) ||
      ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT) == 0 && (SrcPort == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (((HeaderSize != NULL) && (*HeaderSize == 0)) || ((HeaderSize != NULL) && (HeaderPtr == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferSize == NULL) || (BufferPtr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;
  Udp4    = Private->Udp4Read;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  Mode->IcmpErrorReceived = FALSE;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PxeBcCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

TRY_AGAIN:

  IsDone = FALSE;
  Status = Udp4->Receive (Udp4, &Token);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_ICMP_ERROR) {
      Mode->IcmpErrorReceived = TRUE;
    }
    goto ON_EXIT;
  }

  Udp4->Poll (Udp4);

  if (!IsDone) {
    Status = EFI_TIMEOUT;
  } else {

    //
    // check whether this packet matches the filters
    //
    if (EFI_ERROR (Token.Status)){
      goto ON_EXIT;
    }

    RxData  = Token.Packet.RxData;
    Session = &RxData->UdpSession;

    Matched = TRUE;

    if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER) != 0) {
      Matched = FALSE;
      //
      // Check UDP package by IP filter settings
      //
      if (CheckIpByFilter (Mode, Session)) {
        Matched = TRUE;
      }
    }

    if (Matched) {
      Matched = FALSE;

      //
      // Match the destination ip of the received udp dgram
      //
      if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP) != 0) {
        Matched = TRUE;

        if (DestIp != NULL) {
          CopyMem (DestIp, &Session->DestinationAddress, sizeof (EFI_IPv4_ADDRESS));
        }
      } else {
        if (DestIp != NULL) {
          if (EFI_IP4_EQUAL (DestIp, &Session->DestinationAddress)) {
            Matched = TRUE;
          }
        } else {
          if (EFI_IP4_EQUAL (&Private->StationIp, &Session->DestinationAddress)) {
            Matched = TRUE;
          }
        }
      }
    }

    if (Matched) {
      //
      // Match the destination port of the received udp dgram
      //
      if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT) != 0) {

        if (DestPort != NULL) {
          *DestPort = Session->DestinationPort;
        }
      } else {

        if (*DestPort != Session->DestinationPort) {
          Matched = FALSE;
        }
      }
    }

    if (Matched) {
      //
      // Match the source ip of the received udp dgram
      //
      if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP) != 0) {

        if (SrcIp != NULL) {
          CopyMem (SrcIp, &Session->SourceAddress, sizeof (EFI_IPv4_ADDRESS));
        }
      } else {

        if (!EFI_IP4_EQUAL (SrcIp, &Session->SourceAddress)) {
          Matched = FALSE;
        }
      }
    }

    if (Matched) {
      //
      // Match the source port of the received udp dgram
      //
      if ((OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT) != 0) {

        if (SrcPort != NULL) {
          *SrcPort = Session->SourcePort;
        }
      } else {

        if (*SrcPort != Session->SourcePort) {
          Matched = FALSE;
        }
      }
    }

    if (Matched) {
      ASSERT (RxData != NULL);

      HeaderLen = 0;
      if (HeaderSize != NULL) {
        HeaderLen = MIN (*HeaderSize, RxData->DataLength);
      }

      if (RxData->DataLength - HeaderLen > *BufferSize) {
        Status = EFI_BUFFER_TOO_SMALL;
      } else {
        *HeaderSize = HeaderLen;
        *BufferSize = RxData->DataLength - HeaderLen;

        HeaderCopiedLen = 0;
        BufferCopiedLen = 0;
        for (FragmentIndex = 0; FragmentIndex < RxData->FragmentCount; FragmentIndex++) {
          FragmentLength = RxData->FragmentTable[FragmentIndex].FragmentLength;
          FragmentBuffer = RxData->FragmentTable[FragmentIndex].FragmentBuffer;
          if (HeaderCopiedLen + FragmentLength < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopyMem ((UINT8 *) HeaderPtr + HeaderCopiedLen, FragmentBuffer, FragmentLength);
            HeaderCopiedLen += FragmentLength;
          } else if (HeaderCopiedLen < HeaderLen) {
            //
            // Copy the header part of received data.
            //
            CopiedLen = HeaderLen - HeaderCopiedLen;
            CopyMem ((UINT8 *) HeaderPtr + HeaderCopiedLen, FragmentBuffer, CopiedLen);
            HeaderCopiedLen += CopiedLen;

            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *) BufferPtr + BufferCopiedLen, FragmentBuffer + CopiedLen, FragmentLength - CopiedLen);
            BufferCopiedLen += (FragmentLength - CopiedLen);
          } else {
            //
            // Copy the other part of received data.
            //
            CopyMem ((UINT8 *) BufferPtr + BufferCopiedLen, FragmentBuffer, FragmentLength);
            BufferCopiedLen += FragmentLength;
          }
        }
      }
    } else {

      Status = EFI_TIMEOUT;
    }

    //
    // Recycle the RxData
    //
    gBS->SignalEvent (RxData->RecycleSignal);

    if (!Matched) {
      goto TRY_AGAIN;
    }
  }

ON_EXIT:

  Udp4->Cancel (Udp4, &Token);

  gBS->CloseEvent (Token.Event);

  return Status;
}

/**
  Updates the IP receive filters of a network device and enables software filtering.

  The NewFilter field is used to modify the network device's current IP receive
  filter settings and to enable a software filter. This function updates the IpFilter
  field of the EFI_PXE_BASE_CODE_MODE structure with the contents of NewIpFilter.
  The software filter is used when the USE_FILTER in OpFlags is set to UdpRead().
  The current hardware filter remains in effect no matter what the settings of OpFlags
  are, so that the meaning of ANY_DEST_IP set in OpFlags to UdpRead() is from those
  packets whose reception is enabled in hardware-physical NIC address (unicast),
  broadcast address, logical address or addresses (multicast), or all (promiscuous).
  UdpRead() does not modify the IP filter settings.
  Dhcp(), Discover(), and Mtftp() set the IP filter, and return with the IP receive
  filter list emptied and the filter set to EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP.
  If an application or driver wishes to preserve the IP receive filter settings,
  it will have to preserve the IP receive filter settings before these calls, and
  use SetIpFilter() to restore them after the calls. If incompatible filtering is
  requested (for example, PROMISCUOUS with anything else) or if the device does not
  support a requested filter setting and it cannot be accommodated in software
  (for example, PROMISCUOUS not supported), EFI_INVALID_PARAMETER will be returned.
  The IPlist field is used to enable IPs other than the StationIP. They may be
  multicast or unicast. If IPcnt is set as well as EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP,
  then both the StationIP and the IPs from the IPlist will be used.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewFilter             Pointer to the new set of IP receive filters.

  @retval EFI_SUCCESS           The IP receive filter settings were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetIpFilter (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN EFI_PXE_BASE_CODE_IP_FILTER      *NewFilter
  )
{
  EFI_STATUS                Status;
  PXEBC_PRIVATE_DATA        *Private;
  EFI_PXE_BASE_CODE_MODE    *Mode;
  UINTN                     Index;
  EFI_UDP4_CONFIG_DATA      *Udp4Cfg;
  BOOLEAN                   PromiscuousNeed;
  BOOLEAN                   AcceptPromiscuous;
  BOOLEAN                   AcceptBroadcast;
  BOOLEAN                   MultiCastUpdate;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "This == NULL.\n"));
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode = Private->PxeBc.Mode;

  if (NewFilter == NULL) {
    DEBUG ((EFI_D_ERROR, "NewFilter == NULL.\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (NewFilter->IpCnt > EFI_PXE_BASE_CODE_MAX_IPCNT) {
    DEBUG ((EFI_D_ERROR, "NewFilter->IpCnt > %d.\n", EFI_PXE_BASE_CODE_MAX_IPCNT));
    return EFI_INVALID_PARAMETER;
  }

  if (!Mode->Started) {
    DEBUG ((EFI_D_ERROR, "BC was not started.\n"));
    return EFI_NOT_STARTED;
  }

  if (Mode->UsingIpv6) {
    DEBUG ((EFI_D_ERROR, "This driver is PXE for IPv4 Only.\n"));
    return EFI_INVALID_PARAMETER;
  }

  PromiscuousNeed = FALSE;

  for (Index = 0; Index < NewFilter->IpCnt; ++Index) {
    if (IP4_IS_LOCAL_BROADCAST (EFI_IP4 (NewFilter->IpList[Index].v4))) {
      //
      // The IP is a broadcast address.
      //
      DEBUG ((EFI_D_ERROR, "There is broadcast address in NewFilter.\n"));
      return EFI_INVALID_PARAMETER;
    }
    if ((EFI_NTOHL(Mode->StationIp) != 0) &&
        (EFI_NTOHL(Mode->SubnetMask) != 0) &&
        IP4_NET_EQUAL(EFI_NTOHL(Mode->StationIp), EFI_NTOHL(NewFilter->IpList[Index].v4), EFI_NTOHL(Mode->SubnetMask)) &&
        NetIp4IsUnicast (EFI_IP4 (NewFilter->IpList[Index].v4), EFI_NTOHL(Mode->SubnetMask)) &&
        ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) != 0)) {
      //
      // If EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP is set and IP4 address is in IpList,
      // promiscuous mode is needed.
      //
      PromiscuousNeed = TRUE;
    }
  }

  AcceptPromiscuous = FALSE;
  AcceptBroadcast   = FALSE;
  MultiCastUpdate   = FALSE;

  if (PromiscuousNeed ||
      ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) != 0) ||
      ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) != 0)
     ) {
    //
    // Configure the udp4 filter to receive all packages.
    //
    AcceptPromiscuous  = TRUE;
  } else if ((NewFilter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST) != 0) {
    //
    // Configure the udp4 filter to receive all broadcast packages.
    //
    AcceptBroadcast   = TRUE;
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
  
  //
  // Check whether we need reconfigure the UDP instance.
  //
  Udp4Cfg = &Private->Udp4CfgData;
  if ((AcceptPromiscuous != Udp4Cfg->AcceptPromiscuous) ||
  	  (AcceptBroadcast != Udp4Cfg->AcceptBroadcast)     || MultiCastUpdate) {
    //
    // Clear the UDP instance configuration, all joined groups will be left
    // during the operation.
    //
    Private->Udp4Read->Configure (Private->Udp4Read, NULL);

    //
    // Configure the UDP instance with the new configuration.
    //
    Udp4Cfg->AcceptPromiscuous = AcceptPromiscuous;
    Udp4Cfg->AcceptBroadcast   = AcceptBroadcast;
    Status = Private->Udp4Read->Configure (Private->Udp4Read, Udp4Cfg);
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
          // Join the mutilcast group.
          //
          Status = Private->Udp4Read->Groups (Private->Udp4Read, TRUE, &NewFilter->IpList[Index].v4);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }
      }
    }
  }


  //
  // Save the new filter.
  //
  CopyMem (&Mode->IpFilter, NewFilter, sizeof (Mode->IpFilter));

  return EFI_SUCCESS;
}


/**
  Uses the ARP protocol to resolve a MAC address.

  This function uses the ARP protocol to resolve a MAC address. The UsingIpv6 field
  of the EFI_PXE_BASE_CODE_MODE structure is used to determine if IPv4 or IPv6
  addresses are being used. The IP address specified by IpAddr is used to resolve
  a MAC address. If the ARP protocol succeeds in resolving the specified address,
  then the ArpCacheEntries and ArpCache fields of the EFI_PXE_BASE_CODE_MODE structure
  are updated, and EFI_SUCCESS is returned. If MacAddr is not NULL, the resolved
  MAC address is placed there as well.  If the PXE Base Code protocol is in the
  stopped state, then EFI_NOT_STARTED is returned. If the ARP protocol encounters
  a timeout condition while attempting to resolve an address, then EFI_TIMEOUT is
  returned. If the Callback Protocol does not return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  then EFI_ABORTED is returned.

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  IpAddr                Pointer to the IP address that is used to resolve a MAC address.
  @param  MacAddr               If not NULL, a pointer to the MAC address that was resolved with the
                                ARP protocol.

  @retval EFI_SUCCESS           The IP or MAC address was resolved.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_ICMP_ERROR        Something error occur with the ICMP packet message.

**/
EFI_STATUS
EFIAPI
EfiPxeBcArp (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_IP_ADDRESS                   * IpAddr,
  IN EFI_MAC_ADDRESS                  * MacAddr OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;
  EFI_MAC_ADDRESS         TempMacAddr;

  if (This == NULL || IpAddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (!Private->AddressIsOk || Mode->UsingIpv6) {
    //
    // We can't resolve the IP address if we don't have a local address now.
    // Don't have ARP for IPv6.
    //
    return EFI_INVALID_PARAMETER;
  }

  Mode->IcmpErrorReceived = FALSE;

  if (!Mode->AutoArp) {
    //
    // If AutoArp is set false, check arp cache
    //
    UpdateArpCache (This);
    if (!FindInArpCache (Mode, &IpAddr->v4, &TempMacAddr)) {
      return EFI_DEVICE_ERROR;
    }
  } else {
    Status = Private->Arp->Request (Private->Arp, &IpAddr->v4, NULL, &TempMacAddr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_ICMP_ERROR) {
        Mode->IcmpErrorReceived = TRUE;
      }
      return Status;
    }
  }

  if (MacAddr != NULL) {
    CopyMem (MacAddr, &TempMacAddr, sizeof (EFI_MAC_ADDRESS));
  }

  return EFI_SUCCESS;
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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewAutoArp            If not NULL, a pointer to a value that specifies whether to replace the
                                current value of AutoARP.
  @param  NewSendGUID           If not NULL, a pointer to a value that specifies whether to replace the
                                current value of SendGUID.
  @param  NewTTL                If not NULL, a pointer to be used in place of the current value of TTL,
                                the "time to live" field of the IP header.
  @param  NewToS                If not NULL, a pointer to be used in place of the current value of ToS,
                                the "type of service" field of the IP header.
  @param  NewMakeCallback       If not NULL, a pointer to a value that specifies whether to replace the
                                current value of the MakeCallback field of the Mode structure.

  @retval EFI_SUCCESS           The new parameters values were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetParameters (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN BOOLEAN                          *NewAutoArp OPTIONAL,
  IN BOOLEAN                          *NewSendGUID OPTIONAL,
  IN UINT8                            *NewTTL OPTIONAL,
  IN UINT8                            *NewToS OPTIONAL,
  IN BOOLEAN                          *NewMakeCallback  // OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_STATUS              Status;

  Status = EFI_SUCCESS;

  if (This == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (NewSendGUID != NULL && *NewSendGUID) {
    //
    // FixMe, cann't locate SendGuid
    //
  }

  if (NewMakeCallback != NULL && *NewMakeCallback) {

    Status = gBS->HandleProtocol (
                    Private->Controller,
                    &gEfiPxeBaseCodeCallbackProtocolGuid,
                    (VOID **) &Private->PxeBcCallback
                    );
    if (EFI_ERROR (Status) || (Private->PxeBcCallback->Callback == NULL)) {

      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  if (!Mode->Started) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (NewMakeCallback != NULL) {

    if (*NewMakeCallback) {
      //
      // Update the Callback protocol.
      //
      Status = gBS->HandleProtocol (
                      Private->Controller,
                      &gEfiPxeBaseCodeCallbackProtocolGuid,
                      (VOID **) &Private->PxeBcCallback
                      );

      if (EFI_ERROR (Status) || (Private->PxeBcCallback->Callback == NULL)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }
    } else {
      Private->PxeBcCallback = NULL;
    }

    Mode->MakeCallbacks = *NewMakeCallback;
  }

  if (NewAutoArp != NULL) {
    Mode->AutoArp = *NewAutoArp;
  }

  if (NewSendGUID != NULL) {
    Mode->SendGUID = *NewSendGUID;
  }

  if (NewTTL != NULL) {
    Mode->TTL = *NewTTL;
  }

  if (NewToS != NULL) {
    Mode->ToS = *NewToS;
  }

ON_EXIT:
  return Status;
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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewStationIp          Pointer to the new IP address to be used by the network device.
  @param  NewSubnetMask         Pointer to the new subnet mask to be used by the network device.

  @retval EFI_SUCCESS           The new station IP address and/or subnet mask were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetStationIP (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_IP_ADDRESS                   * NewStationIp  OPTIONAL,
  IN EFI_IP_ADDRESS                   * NewSubnetMask OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA      *Private;
  EFI_PXE_BASE_CODE_MODE  *Mode;
  EFI_ARP_CONFIG_DATA     ArpConfigData;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (NewSubnetMask != NULL && !IP4_IS_VALID_NETMASK (NTOHL (NewSubnetMask->Addr[0]))) {
    return EFI_INVALID_PARAMETER;
  }

  if (NewStationIp != NULL) {
    if (IP4_IS_UNSPECIFIED(NTOHL (NewStationIp->Addr[0])) || 
        IP4_IS_LOCAL_BROADCAST(NTOHL (NewStationIp->Addr[0])) ||
        (NewSubnetMask != NULL && NewSubnetMask->Addr[0] != 0 && !NetIp4IsUnicast (NTOHL (NewStationIp->Addr[0]), NTOHL (NewSubnetMask->Addr[0])))) {
      return EFI_INVALID_PARAMETER;
    }
  }
  
  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (This);
  Mode    = Private->PxeBc.Mode;

  if (!Mode->Started) {
    return EFI_NOT_STARTED;
  }

  if (NewStationIp != NULL) {
    CopyMem (&Mode->StationIp, NewStationIp, sizeof (EFI_IP_ADDRESS));
    CopyMem (&Private->StationIp, NewStationIp, sizeof (EFI_IP_ADDRESS));
  }

  if (NewSubnetMask != NULL) {
    CopyMem (&Mode->SubnetMask, NewSubnetMask, sizeof (EFI_IP_ADDRESS));
    CopyMem (&Private->SubnetMask ,NewSubnetMask, sizeof (EFI_IP_ADDRESS));
  }

  Private->AddressIsOk = TRUE;

  if (!Mode->UsingIpv6) {
    //
    // If in IPv4 mode, configure the corresponding ARP with this new
    // station IP address.
    //
    ZeroMem (&ArpConfigData, sizeof (EFI_ARP_CONFIG_DATA));

    ArpConfigData.SwAddressType   = 0x0800;
    ArpConfigData.SwAddressLength = (UINT8) sizeof (EFI_IPv4_ADDRESS);
    ArpConfigData.StationAddress  = &Private->StationIp.v4;

    Private->Arp->Configure (Private->Arp, NULL);
    Private->Arp->Configure (Private->Arp, &ArpConfigData);

    //
    // Update the route table.
    //
    Mode->RouteTableEntries                = 1;
    Mode->RouteTable[0].IpAddr.Addr[0]     = Private->StationIp.Addr[0] & Private->SubnetMask.Addr[0];
    Mode->RouteTable[0].SubnetMask.Addr[0] = Private->SubnetMask.Addr[0];
    Mode->RouteTable[0].GwAddr.Addr[0]     = 0;
  }

  return EFI_SUCCESS;
}

/**
  Updates the contents of the cached DHCP and Discover packets.

  The pointers to the new packets are used to update the contents of the cached
  packets in the EFI_PXE_BASE_CODE_MODE structure.

  @param  This                   Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewDhcpDiscoverValid   Pointer to a value that will replace the current
                                 DhcpDiscoverValid field.
  @param  NewDhcpAckReceived     Pointer to a value that will replace the current
                                 DhcpAckReceived field.
  @param  NewProxyOfferReceived  Pointer to a value that will replace the current
                                 ProxyOfferReceived field.
  @param  NewPxeDiscoverValid    Pointer to a value that will replace the current
                                 ProxyOfferReceived field.
  @param  NewPxeReplyReceived    Pointer to a value that will replace the current
                                 PxeReplyReceived field.
  @param  NewPxeBisReplyReceived Pointer to a value that will replace the current
                                 PxeBisReplyReceived field.
  @param  NewDhcpDiscover        Pointer to the new cached DHCP Discover packet contents.
  @param  NewDhcpAck             Pointer to the new cached DHCP Ack packet contents.
  @param  NewProxyOffer          Pointer to the new cached Proxy Offer packet contents.
  @param  NewPxeDiscover         Pointer to the new cached PXE Discover packet contents.
  @param  NewPxeReply            Pointer to the new cached PXE Reply packet contents.
  @param  NewPxeBisReply         Pointer to the new cached PXE BIS Reply packet contents.

  @retval EFI_SUCCESS            The cached packet contents were updated.
  @retval EFI_NOT_STARTED        The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER  This is NULL or not point to a valid EFI_PXE_BASE_CODE_PROTOCOL structure.

**/
EFI_STATUS
EFIAPI
EfiPxeBcSetPackets (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN BOOLEAN                          * NewDhcpDiscoverValid OPTIONAL,
  IN BOOLEAN                          * NewDhcpAckReceived OPTIONAL,
  IN BOOLEAN                          * NewProxyOfferReceived OPTIONAL,
  IN BOOLEAN                          * NewPxeDiscoverValid OPTIONAL,
  IN BOOLEAN                          * NewPxeReplyReceived OPTIONAL,
  IN BOOLEAN                          * NewPxeBisReplyReceived OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewDhcpDiscover OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewDhcpAck OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewProxyOffer OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeDiscover OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeReply OPTIONAL,
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeBisReply OPTIONAL
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

EFI_PXE_BASE_CODE_PROTOCOL  mPxeBcProtocolTemplate = {
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

  @param  This                  Pointer to the EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL instance.
  @param  Function              The PXE Base Code Protocol function that is waiting for an event.
  @param  Received              TRUE if the callback is being invoked due to a receive event. FALSE if
                                the callback is being invoked due to a transmit event.
  @param  PacketLength          The length, in bytes, of Packet. This field will have a value of zero if
                                this is a wait for receive event.
  @param  PacketPtr             If Received is TRUE, a pointer to the packet that was just received;
                                otherwise a pointer to the packet that is about to be transmitted.

  @retval EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE if Function specifies a continue operation
  @retval EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT    if Function specifies an abort operation

**/
EFI_PXE_BASE_CODE_CALLBACK_STATUS
EFIAPI
EfiPxeLoadFileCallback (
  IN EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  * This,
  IN EFI_PXE_BASE_CODE_FUNCTION           Function,
  IN BOOLEAN                              Received,
  IN UINT32                               PacketLength,
  IN EFI_PXE_BASE_CODE_PACKET             * PacketPtr OPTIONAL
  )
{
  EFI_INPUT_KEY Key;
  EFI_STATUS    Status;

  //
  // Catch Ctrl-C or ESC to abort.
  //
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

  if (!EFI_ERROR (Status)) {

    if (Key.ScanCode == SCAN_ESC || Key.UnicodeChar == (0x1F & 'c')) {

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
    if (PacketLength != 0 && PacketPtr != NULL) {
      if (PacketPtr->Raw[0x1C] != 0x00 || PacketPtr->Raw[0x1D] != 0x01) {
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

  if (PacketLength != 0 && PacketPtr != NULL) {
    //
    // Print '.' when transmit a packet
    //
    AsciiPrint (".");

  }

  return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
}

EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL mPxeBcCallBackTemplate = {
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_REVISION,
  EfiPxeLoadFileCallback
};


/**
  Find the boot file.

  @param  Private      Pointer to PxeBc private data.
  @param  BufferSize   Pointer to buffer size.
  @param  Buffer       Pointer to buffer.

  @retval EFI_SUCCESS          Discover the boot file successfully.
  @retval EFI_TIMEOUT          The TFTP/MTFTP operation timed out.
  @retval EFI_ABORTED          PXE bootstrap server, so local boot need abort.
  @retval EFI_BUFFER_TOO_SMALL The buffer is too small to load the boot file.

**/
EFI_STATUS
DiscoverBootFile (
  IN     PXEBC_PRIVATE_DATA  *Private,
  IN OUT UINT64              *BufferSize,
  IN     VOID                *Buffer
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_PXE_BASE_CODE_MODE      *Mode;
  EFI_STATUS                  Status;
  UINT16                      Type;
  UINT16                      Layer;
  BOOLEAN                     UseBis;
  PXEBC_CACHED_DHCP4_PACKET   *Packet;
  UINT16                      Value;

  PxeBc = &Private->PxeBc;
  Mode  = PxeBc->Mode;
  Type  = EFI_PXE_BASE_CODE_BOOT_TYPE_BOOTSTRAP;
  Layer = EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL;

  //
  // do DHCP.
  //
  Status = PxeBc->Dhcp (PxeBc, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select a boot server
  //
  Status = PxeBcSelectBootPrompt (Private);

  if (Status == EFI_SUCCESS) {
    Status = PxeBcSelectBootMenu (Private, &Type, TRUE);
  } else if (Status == EFI_TIMEOUT) {
    Status = PxeBcSelectBootMenu (Private, &Type, FALSE);
  }

  if (!EFI_ERROR (Status)) {

    if (Type == EFI_PXE_BASE_CODE_BOOT_TYPE_BOOTSTRAP) {
      //
      // Local boot(PXE bootstrap server) need abort
      //
      return EFI_ABORTED;
    }

    UseBis  = (BOOLEAN) (Mode->BisSupported && Mode->BisDetected);
    Status  = PxeBc->Discover (PxeBc, Type, &Layer, UseBis, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *BufferSize = 0;

  //
  // Get bootfile name and (m)tftp server ip addresss
  //
  if (Mode->PxeReplyReceived) {
    Packet = &Private->PxeReply;
  } else if (Mode->ProxyOfferReceived) {
    Packet = &Private->ProxyOffer;
  } else {
    Packet = &Private->Dhcp4Ack;
  }

  //
  // Use siaddr(next server) in DHCPOFFER packet header, if zero, use option 54(server identifier)
  // in DHCPOFFER packet.
  // (It does not comply with PXE Spec, Ver2.1)
  //
  if (EFI_IP4_EQUAL (&Packet->Packet.Offer.Dhcp4.Header.ServerAddr, &mZeroIp4Addr)) {
    CopyMem (
      &Private->ServerIp,
      Packet->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_SERVER_ID]->Data,
      sizeof (EFI_IPv4_ADDRESS)
      );
  } else {
    CopyMem (
      &Private->ServerIp,
      &Packet->Packet.Offer.Dhcp4.Header.ServerAddr,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }
  if (Private->ServerIp.Addr[0] == 0) {
    return EFI_DEVICE_ERROR;
  }

  ASSERT (Packet->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL);

  //
  // bootlfile name
  //
  Private->BootFileName = (CHAR8 *) (Packet->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Data);

  if (Packet->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN] != NULL) {
    //
    // Already have the bootfile length option, compute the file size
    //
    CopyMem (&Value, Packet->Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN]->Data, sizeof (Value));
    Value       = NTOHS (Value);
    *BufferSize = 512 * Value;
    Status      = EFI_BUFFER_TOO_SMALL;
  } else {
    //
    // Get the bootfile size from tftp
    //
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                      Buffer,
                      FALSE,
                      BufferSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      (UINT8 *) Private->BootFileName,
                      NULL,
                      FALSE
                      );
  }

  Private->FileSize = (UINTN) *BufferSize;

  //
  // Display all the information: boot server address, boot file name and boot file size.
  //
  AsciiPrint ("\n  Server IP address is ");
  PxeBcShowIp4Addr (&Private->ServerIp.v4);
  AsciiPrint ("\n  NBP filename is %a", Private->BootFileName);
  AsciiPrint ("\n  NBP filesize is %d Bytes", Private->FileSize);

  return Status;
}

/**
  Causes the driver to load a specified file.

  @param  This                  Protocol instance pointer.
  @param  FilePath              The device specific path of the file to load.
  @param  BootPolicy            If TRUE, indicates that the request originates from the
                                boot manager is attempting to load FilePath as a boot
                                selection. If FALSE, then FilePath must match as exact file
                                to be loaded.
  @param  BufferSize            On input the size of Buffer in bytes. On output with a return
                                code of EFI_SUCCESS, the amount of data transferred to
                                Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                the size of Buffer required to retrieve the requested file.
  @param  Buffer                The memory buffer to transfer the file to. IF Buffer is NULL,
                                then no the size of the requested file is returned in
                                BufferSize.

  @retval EFI_SUCCESS                 The file was loaded.
  @retval EFI_UNSUPPORTED             The device does not support the provided BootPolicy
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
  IN EFI_LOAD_FILE_PROTOCOL           * This,
  IN EFI_DEVICE_PATH_PROTOCOL         * FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA          *Private;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  BOOLEAN                     NewMakeCallback;
  EFI_STATUS                  Status;
  UINT64                      TmpBufSize;
  EFI_STATUS                  MediaStatus;

  if (FilePath == NULL || !IsDevicePathEnd (FilePath)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Private         = PXEBC_PRIVATE_DATA_FROM_LOADFILE (This);
  PxeBc           = &Private->PxeBc;
  NewMakeCallback = FALSE;
  Status          = EFI_DEVICE_ERROR;

  if (This == NULL || BufferSize == NULL) {

    return EFI_INVALID_PARAMETER;
  }

  //
  // Only support BootPolicy
  //
  if (!BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check media status before PXE start
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Private->Controller, PXEBC_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    return EFI_NO_MEDIA;
  }

  Status = PxeBc->Start (PxeBc, FALSE);
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Private->Controller,
                  &gEfiPxeBaseCodeCallbackProtocolGuid,
                  (VOID **) &Private->PxeBcCallback
                  );
  if (Status == EFI_UNSUPPORTED) {

    CopyMem (&Private->LoadFileCallback, &mPxeBcCallBackTemplate, sizeof (Private->LoadFileCallback));

    Status = gBS->InstallProtocolInterface (
                    &Private->Controller,
                    &gEfiPxeBaseCodeCallbackProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->LoadFileCallback
                    );

    NewMakeCallback = (BOOLEAN) (Status == EFI_SUCCESS);

    Status          = PxeBc->SetParameters (PxeBc, NULL, NULL, NULL, NULL, &NewMakeCallback);
    if (EFI_ERROR (Status)) {
      PxeBc->Stop (PxeBc);
      return Status;
    }
  }

  if (Private->FileSize == 0) {
    TmpBufSize  = 0;
    Status      = DiscoverBootFile (Private, &TmpBufSize, Buffer);

    if (sizeof (UINTN) < sizeof (UINT64) && (TmpBufSize > 0xFFFFFFFF)) {
      Status = EFI_DEVICE_ERROR;
    } else if (TmpBufSize > 0 && *BufferSize >= (UINTN) TmpBufSize && Buffer != NULL) {
      AsciiPrint ("\n Downloading NBP file...\n");
      *BufferSize = (UINTN) TmpBufSize;
      Status = PxeBc->Mtftp (
                        PxeBc,
                        EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                        Buffer,
                        FALSE,
                        &TmpBufSize,
                        &Private->BlockSize,
                        &Private->ServerIp,
                        (UINT8 *) Private->BootFileName,
                        NULL,
                        FALSE
                        );
    } else if (TmpBufSize > 0) {
      *BufferSize = (UINTN) TmpBufSize;
      Status      = EFI_BUFFER_TOO_SMALL;
    }
  } else if (Buffer == NULL || Private->FileSize > *BufferSize) {
    *BufferSize = Private->FileSize;
    Status      = EFI_BUFFER_TOO_SMALL;
  } else {
    //
    // Download the file.
    //
    AsciiPrint ("\n Downloading NBP file...\n");
    TmpBufSize = (UINT64) (*BufferSize);
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                      Buffer,
                      FALSE,
                      &TmpBufSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      (UINT8 *) Private->BootFileName,
                      NULL,
                      FALSE
                      );
  }
  //
  // If we added a callback protocol, now is the time to remove it.
  //
  if (NewMakeCallback) {

    NewMakeCallback = FALSE;

    PxeBc->SetParameters (PxeBc, NULL, NULL, NULL, NULL, &NewMakeCallback);

    gBS->UninstallProtocolInterface (
          Private->Controller,
          &gEfiPxeBaseCodeCallbackProtocolGuid,
          &Private->LoadFileCallback
          );
  }

  //
  // Check download status
  //
  if (Status == EFI_SUCCESS) {
    AsciiPrint ("\n  NBP file downloaded successfully.\n");
    //
    // The DHCP4 can have only one configured child instance so we need to stop
    // reset the DHCP4 child before we return. Otherwise the other programs which 
    // also need to use DHCP4 will be impacted.
    // The functionality of PXE Base Code protocol will not be stopped,
    // when downloading is successfully.
    //
    Private->Dhcp4->Stop (Private->Dhcp4);
    Private->Dhcp4->Configure (Private->Dhcp4, NULL);
    return EFI_SUCCESS;

  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    if (Buffer != NULL) {
      AsciiPrint ("PXE-E05: Download buffer is smaller than requested file.\n");
    } else {
      //
      // The functionality of PXE Base Code protocol will not be stopped.
      //
      return Status;
    }

  } else if (Status == EFI_DEVICE_ERROR) {
    AsciiPrint ("PXE-E07: Network device error.\n");

  } else if (Status == EFI_OUT_OF_RESOURCES) {
    AsciiPrint ("PXE-E09: Could not allocate I/O buffers.\n");

  } else if (Status == EFI_NO_MEDIA) {
    AsciiPrint ("PXE-E12: Could not detect network connection.\n");

  } else if (Status == EFI_NO_RESPONSE) {
    AsciiPrint ("PXE-E16: No offer received.\n");

  } else if (Status == EFI_TIMEOUT) {
    AsciiPrint ("PXE-E18: Server response timeout.\n");

  } else if (Status == EFI_ABORTED) {
    AsciiPrint ("PXE-E21: Remote boot cancelled.\n");

  } else if (Status == EFI_ICMP_ERROR) {
    AsciiPrint ("PXE-E22: Client received ICMP error from server.\n");

  } else if (Status == EFI_TFTP_ERROR) {
    AsciiPrint ("PXE-E23: Client received TFTP error from server.\n");

  } else {
    AsciiPrint ("PXE-E99: Unexpected network error.\n");
  }

  PxeBc->Stop (PxeBc);

  return Status;
}

EFI_LOAD_FILE_PROTOCOL  mLoadFileProtocolTemplate = { EfiPxeLoadFile };

