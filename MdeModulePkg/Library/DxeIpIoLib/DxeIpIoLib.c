/** @file
  IpIo Library.

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Protocol/Udp4.h>

#include <Library/IpIoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DpcLib.h>


GLOBAL_REMOVE_IF_UNREFERENCED LIST_ENTRY  mActiveIpIoList = {
  &mActiveIpIoList,
  &mActiveIpIoList
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_IP4_CONFIG_DATA  mIp4IoDefaultIpConfigData = {
  EFI_IP_PROTO_UDP,
  FALSE,
  TRUE,
  FALSE,
  FALSE,
  FALSE,
  {{0, 0, 0, 0}},
  {{0, 0, 0, 0}},
  0,
  255,
  FALSE,
  FALSE,
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_IP6_CONFIG_DATA  mIp6IoDefaultIpConfigData = {
  EFI_IP_PROTO_UDP,
  FALSE,
  TRUE,
  FALSE,
  {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  0,
  255,
  0,
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED ICMP_ERROR_INFO  mIcmpErrMap[10] = {
  {FALSE, TRUE }, // ICMP_ERR_UNREACH_NET
  {FALSE, TRUE }, // ICMP_ERR_UNREACH_HOST
  {TRUE,  TRUE }, // ICMP_ERR_UNREACH_PROTOCOL
  {TRUE,  TRUE }, // ICMP_ERR_UNREACH_PORT
  {TRUE,  TRUE }, // ICMP_ERR_MSGSIZE
  {FALSE, TRUE }, // ICMP_ERR_UNREACH_SRCFAIL
  {FALSE, TRUE }, // ICMP_ERR_TIMXCEED_INTRANS
  {FALSE, TRUE }, // ICMP_ERR_TIMEXCEED_REASS
  {FALSE, FALSE}, // ICMP_ERR_QUENCH
  {FALSE, TRUE }  // ICMP_ERR_PARAMPROB
};

GLOBAL_REMOVE_IF_UNREFERENCED ICMP_ERROR_INFO  mIcmp6ErrMap[10] = {
  {FALSE, TRUE}, // ICMP6_ERR_UNREACH_NET
  {FALSE, TRUE}, // ICMP6_ERR_UNREACH_HOST
  {TRUE,  TRUE}, // ICMP6_ERR_UNREACH_PROTOCOL
  {TRUE,  TRUE}, // ICMP6_ERR_UNREACH_PORT
  {TRUE,  TRUE}, // ICMP6_ERR_PACKAGE_TOOBIG
  {FALSE, TRUE}, // ICMP6_ERR_TIMXCEED_HOPLIMIT
  {FALSE, TRUE}, // ICMP6_ERR_TIMXCEED_REASS
  {FALSE, TRUE}, // ICMP6_ERR_PARAMPROB_HEADER
  {FALSE, TRUE}, // ICMP6_ERR_PARAMPROB_NEXHEADER
  {FALSE, TRUE}  // ICMP6_ERR_PARAMPROB_IPV6OPTION
};


/**
  Notify function for IP transmit token.

  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoTransmitHandlerDpc (
  IN VOID      *Context
  );


/**
  Notify function for IP transmit token.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoTransmitHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  );


/**
  This function create an IP child ,open the IP protocol, and return the opened
  IP protocol as Interface.

  @param[in]    ControllerHandle   The controller handle.
  @param[in]    ImageHandle        The image handle.
  @param[in]    ChildHandle        Pointer to the buffer to save the IP child handle.
  @param[in]    IpVersion          The version of the IP protocol to use, either
                                   IPv4 or IPv6.
  @param[out]   Interface          Pointer used to get the IP protocol interface.

  @retval       EFI_SUCCESS        The IP child is created and the IP protocol
                                   interface is retrieved.
  @retval       EFI_UNSUPPORTED    Upsupported IpVersion.
  @retval       Others             The required operation failed.

**/
EFI_STATUS
IpIoCreateIpChildOpenProtocol (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ImageHandle,
  IN  EFI_HANDLE  *ChildHandle,
  IN  UINT8       IpVersion,
  OUT VOID        **Interface
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *ServiceBindingGuid;
  EFI_GUID    *IpProtocolGuid;

  if (IpVersion == IP_VERSION_4) {
    ServiceBindingGuid = &gEfiIp4ServiceBindingProtocolGuid;
    IpProtocolGuid     = &gEfiIp4ProtocolGuid;
  } else if (IpVersion == IP_VERSION_6){
    ServiceBindingGuid = &gEfiIp6ServiceBindingProtocolGuid;
    IpProtocolGuid     = &gEfiIp6ProtocolGuid;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Create an IP child.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             ImageHandle,
             ServiceBindingGuid,
             ChildHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the IP protocol installed on the *ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  *ChildHandle,
                  IpProtocolGuid,
                  Interface,
                  ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // On failure, destroy the IP child.
    //
    NetLibDestroyServiceChild (
      ControllerHandle,
      ImageHandle,
      ServiceBindingGuid,
      *ChildHandle
      );
  }

  return Status;
}


/**
  This function close the previously openned IP protocol and destroy the IP child.

  @param[in]  ControllerHandle    The controller handle.
  @param[in]  ImageHandle         The image handle.
  @param[in]  ChildHandle         The child handle of the IP child.
  @param[in]  IpVersion           The version of the IP protocol to use, either
                                  IPv4 or IPv6.

  @retval     EFI_SUCCESS         The IP protocol is closed and the relevant IP child
                                  is destroyed.
  @retval     EFI_UNSUPPORTED     Upsupported IpVersion.
  @retval     Others              The required operation failed.

**/
EFI_STATUS
IpIoCloseProtocolDestroyIpChild (
  IN EFI_HANDLE  ControllerHandle,
  IN EFI_HANDLE  ImageHandle,
  IN EFI_HANDLE  ChildHandle,
  IN UINT8       IpVersion
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *ServiceBindingGuid;
  EFI_GUID    *IpProtocolGuid;

  if (IpVersion == IP_VERSION_4) {
    ServiceBindingGuid = &gEfiIp4ServiceBindingProtocolGuid;
    IpProtocolGuid     = &gEfiIp4ProtocolGuid;
  } else if (IpVersion == IP_VERSION_6) {
    ServiceBindingGuid = &gEfiIp6ServiceBindingProtocolGuid;
    IpProtocolGuid     = &gEfiIp6ProtocolGuid;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Close the previously openned IP protocol.
  //
  Status = gBS->CloseProtocol (
                  ChildHandle,
                  IpProtocolGuid,
                  ImageHandle,
                  ControllerHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Destroy the IP child.
  //
  return NetLibDestroyServiceChild (
           ControllerHandle,
           ImageHandle,
           ServiceBindingGuid,
           ChildHandle
           );
}

/**
  This function handles ICMPv4 packets. It is the worker function of
  IpIoIcmpHandler.

  @param[in]       IpIo            Pointer to the IP_IO instance.
  @param[in, out]  Pkt             Pointer to the ICMPv4 packet.
  @param[in]       Session         Pointer to the net session of this ICMPv4 packet.

  @retval          EFI_SUCCESS     The ICMPv4 packet is handled successfully.
  @retval          EFI_ABORTED     This type of ICMPv4 packet is not supported.

**/
EFI_STATUS
IpIoIcmpv4Handler (
  IN     IP_IO                *IpIo,
  IN OUT NET_BUF              *Pkt,
  IN     EFI_NET_SESSION_DATA *Session
  )
{
  IP4_ICMP_ERROR_HEAD  *IcmpHdr;
  EFI_IP4_HEADER       *IpHdr;
  UINT8                IcmpErr;
  UINT8                *PayLoadHdr;
  UINT8                Type;
  UINT8                Code;
  UINT32               TrimBytes;

  ASSERT (IpIo != NULL);
  ASSERT (Pkt != NULL);
  ASSERT (Session != NULL);
  ASSERT (IpIo->IpVersion == IP_VERSION_4);

  //
  // Check the ICMP packet length.
  //
  if (Pkt->TotalSize < sizeof (IP4_ICMP_ERROR_HEAD)) {
    return EFI_ABORTED;
  }

  IcmpHdr = NET_PROTO_HDR (Pkt, IP4_ICMP_ERROR_HEAD);
  IpHdr   = (EFI_IP4_HEADER *) (&IcmpHdr->IpHead);

  if (Pkt->TotalSize < ICMP_ERRLEN (IpHdr)) {

    return EFI_ABORTED;
  }

  Type = IcmpHdr->Head.Type;
  Code = IcmpHdr->Head.Code;

  //
  // Analyze the ICMP Error in this ICMP pkt
  //
  switch (Type) {
  case ICMP_TYPE_UNREACH:
    switch (Code) {
    case ICMP_CODE_UNREACH_NET:
    case ICMP_CODE_UNREACH_HOST:
    case ICMP_CODE_UNREACH_PROTOCOL:
    case ICMP_CODE_UNREACH_PORT:
    case ICMP_CODE_UNREACH_SRCFAIL:
      IcmpErr = (UINT8) (ICMP_ERR_UNREACH_NET + Code);

      break;

    case ICMP_CODE_UNREACH_NEEDFRAG:
      IcmpErr = ICMP_ERR_MSGSIZE;

      break;

    case ICMP_CODE_UNREACH_NET_UNKNOWN:
    case ICMP_CODE_UNREACH_NET_PROHIB:
    case ICMP_CODE_UNREACH_TOSNET:
      IcmpErr = ICMP_ERR_UNREACH_NET;

      break;

    case ICMP_CODE_UNREACH_HOST_UNKNOWN:
    case ICMP_CODE_UNREACH_ISOLATED:
    case ICMP_CODE_UNREACH_HOST_PROHIB:
    case ICMP_CODE_UNREACH_TOSHOST:
      IcmpErr = ICMP_ERR_UNREACH_HOST;

      break;

    default:
      return EFI_ABORTED;
    }

    break;

  case ICMP_TYPE_TIMXCEED:
    if (Code > 1) {
      return EFI_ABORTED;
    }

    IcmpErr = (UINT8) (Code + ICMP_ERR_TIMXCEED_INTRANS);

    break;

  case ICMP_TYPE_PARAMPROB:
    if (Code > 1) {
      return EFI_ABORTED;
    }

    IcmpErr = ICMP_ERR_PARAMPROB;

    break;

  case ICMP_TYPE_SOURCEQUENCH:
    if (Code != 0) {
      return EFI_ABORTED;
    }

    IcmpErr = ICMP_ERR_QUENCH;

    break;

  default:
    return EFI_ABORTED;
  }

  //
  // Notify user the ICMP pkt only containing payload except
  // IP and ICMP header
  //
  PayLoadHdr = (UINT8 *) ((UINT8 *) IpHdr + EFI_IP4_HEADER_LEN (IpHdr));
  TrimBytes  = (UINT32) (PayLoadHdr - (UINT8 *) IcmpHdr);

  NetbufTrim (Pkt, TrimBytes, TRUE);

  //
  // If the input packet has invalid format, and TrimBytes is larger than
  // the packet size, the NetbufTrim might trim the packet to zero.
  //
  if (Pkt->TotalSize != 0) {
    IpIo->PktRcvdNotify (EFI_ICMP_ERROR, IcmpErr, Session, Pkt, IpIo->RcvdContext);
  }

  return EFI_SUCCESS;
}

/**
  This function handles ICMPv6 packets. It is the worker function of
  IpIoIcmpHandler.

  @param[in]       IpIo            Pointer to the IP_IO instance.
  @param[in, out]  Pkt             Pointer to the ICMPv6 packet.
  @param[in]       Session         Pointer to the net session of this ICMPv6 packet.

  @retval          EFI_SUCCESS     The ICMPv6 packet is handled successfully.
  @retval          EFI_ABORTED     This type of ICMPv6 packet is not supported.

**/
EFI_STATUS
IpIoIcmpv6Handler (
  IN     IP_IO                *IpIo,
  IN OUT NET_BUF              *Pkt,
  IN     EFI_NET_SESSION_DATA *Session
  )
{
  IP6_ICMP_ERROR_HEAD  *IcmpHdr;
  EFI_IP6_HEADER       *IpHdr;
  UINT8                IcmpErr;
  UINT8                *PayLoadHdr;
  UINT8                Type;
  UINT8                Code;
  UINT8                NextHeader;
  UINT32               TrimBytes;
  BOOLEAN              Flag;

  ASSERT (IpIo != NULL);
  ASSERT (Pkt != NULL);
  ASSERT (Session != NULL);
  ASSERT (IpIo->IpVersion == IP_VERSION_6);

  //
  // Check the ICMPv6 packet length.
  //
  if (Pkt->TotalSize < sizeof (IP6_ICMP_ERROR_HEAD)) {

    return EFI_ABORTED;
  }

  IcmpHdr = NET_PROTO_HDR (Pkt, IP6_ICMP_ERROR_HEAD);
  Type    = IcmpHdr->Head.Type;
  Code    = IcmpHdr->Head.Code;

  //
  // Analyze the ICMPv6 Error in this ICMPv6 packet
  //
  switch (Type) {
  case ICMP_V6_DEST_UNREACHABLE:
    switch (Code) {
    case ICMP_V6_NO_ROUTE_TO_DEST:
    case ICMP_V6_BEYOND_SCOPE:
    case ICMP_V6_ROUTE_REJECTED:
      IcmpErr = ICMP6_ERR_UNREACH_NET;

      break;

    case ICMP_V6_COMM_PROHIBITED:
    case ICMP_V6_ADDR_UNREACHABLE:
    case ICMP_V6_SOURCE_ADDR_FAILED:
      IcmpErr = ICMP6_ERR_UNREACH_HOST;

      break;

    case ICMP_V6_PORT_UNREACHABLE:
      IcmpErr = ICMP6_ERR_UNREACH_PORT;

      break;

     default:
      return EFI_ABORTED;
    }

    break;

  case ICMP_V6_PACKET_TOO_BIG:
    if (Code >= 1) {
      return EFI_ABORTED;
    }

    IcmpErr = ICMP6_ERR_PACKAGE_TOOBIG;

    break;

  case ICMP_V6_TIME_EXCEEDED:
    if (Code > 1) {
      return EFI_ABORTED;
    }

    IcmpErr = (UINT8) (ICMP6_ERR_TIMXCEED_HOPLIMIT + Code);

    break;

  case ICMP_V6_PARAMETER_PROBLEM:
    if (Code > 3) {
      return EFI_ABORTED;
    }

    IcmpErr = (UINT8) (ICMP6_ERR_PARAMPROB_HEADER + Code);

    break;

   default:

     return EFI_ABORTED;
   }

  //
  // Notify user the ICMPv6 packet only containing payload except
  // IPv6 basic header, extension header and ICMP header
  //

  IpHdr      = (EFI_IP6_HEADER *) (&IcmpHdr->IpHead);
  NextHeader = IpHdr->NextHeader;
  PayLoadHdr = (UINT8 *) ((UINT8 *) IcmpHdr + sizeof (IP6_ICMP_ERROR_HEAD));
  Flag       = TRUE;

  do {
    switch (NextHeader) {
    case EFI_IP_PROTO_UDP:
    case EFI_IP_PROTO_TCP:
    case EFI_IP_PROTO_ICMP:
    case IP6_NO_NEXT_HEADER:
      Flag = FALSE;

      break;

    case IP6_HOP_BY_HOP:
    case IP6_DESTINATION:
      //
      // The Hdr Ext Len is 8-bit unsigned integer in 8-octet units, not including
      // the first 8 octets.
      //
      NextHeader = *(PayLoadHdr);
      PayLoadHdr = (UINT8 *) (PayLoadHdr + (*(PayLoadHdr + 1) + 1) * 8);

      break;

    case IP6_FRAGMENT:
      //
      // The Fragment Header Length is 8 octets.
      //
      NextHeader = *(PayLoadHdr);
      PayLoadHdr = (UINT8 *) (PayLoadHdr + 8);

      break;

    default:

      return EFI_ABORTED;
    }
  } while (Flag);

  TrimBytes = (UINT32) (PayLoadHdr - (UINT8 *) IcmpHdr);

  NetbufTrim (Pkt, TrimBytes, TRUE);

  //
  // If the input packet has invalid format, and TrimBytes is larger than
  // the packet size, the NetbufTrim might trim the packet to zero.
  //
  if (Pkt->TotalSize != 0) {
    IpIo->PktRcvdNotify (EFI_ICMP_ERROR, IcmpErr, Session, Pkt, IpIo->RcvdContext);
  }

  return EFI_SUCCESS;
}

/**
  This function handles ICMP packets.

  @param[in]       IpIo            Pointer to the IP_IO instance.
  @param[in, out]  Pkt             Pointer to the ICMP packet.
  @param[in]       Session         Pointer to the net session of this ICMP packet.

  @retval          EFI_SUCCESS     The ICMP packet is handled successfully.
  @retval          EFI_ABORTED     This type of ICMP packet is not supported.
  @retval          EFI_UNSUPPORTED The IP protocol version in IP_IO is not supported.

**/
EFI_STATUS
IpIoIcmpHandler (
  IN     IP_IO                *IpIo,
  IN OUT NET_BUF              *Pkt,
  IN     EFI_NET_SESSION_DATA *Session
  )
{

  if (IpIo->IpVersion == IP_VERSION_4) {

    return IpIoIcmpv4Handler (IpIo, Pkt, Session);

  } else if (IpIo->IpVersion == IP_VERSION_6) {

    return IpIoIcmpv6Handler (IpIo, Pkt, Session);

  } else {

    return EFI_UNSUPPORTED;
  }
}


/**
  Free function for receive token of IP_IO. It is used to
  signal the recycle event to notify IP to recycle the
  data buffer.

  @param[in]  Event                 The event to be signaled.

**/
VOID
EFIAPI
IpIoExtFree (
  IN VOID  *Event
  )
{
  gBS->SignalEvent ((EFI_EVENT) Event);
}


/**
  Create a send entry to wrap a packet before sending
  out it through IP.

  @param[in, out]  IpIo                 Pointer to the IP_IO instance.
  @param[in, out]  Pkt                  Pointer to the packet.
  @param[in]       Sender               Pointer to the IP sender.
  @param[in]       Context              Pointer to the context.
  @param[in]       NotifyData           Pointer to the notify data.
  @param[in]       Dest                 Pointer to the destination IP address.
  @param[in]       Override             Pointer to the overriden IP_IO data.

  @return Pointer to the data structure created to wrap the packet. If any error occurs,
          then return NULL.

**/
IP_IO_SEND_ENTRY *
IpIoCreateSndEntry (
  IN OUT IP_IO             *IpIo,
  IN OUT NET_BUF           *Pkt,
  IN     IP_IO_IP_PROTOCOL Sender,
  IN     VOID              *Context    OPTIONAL,
  IN     VOID              *NotifyData OPTIONAL,
  IN     EFI_IP_ADDRESS    *Dest       OPTIONAL,
  IN     IP_IO_OVERRIDE    *Override
  )
{
  IP_IO_SEND_ENTRY          *SndEntry;
  EFI_EVENT                 Event;
  EFI_STATUS                Status;
  NET_FRAGMENT              *ExtFragment;
  UINT32                    FragmentCount;
  IP_IO_OVERRIDE            *OverrideData;
  IP_IO_IP_TX_DATA          *TxData;
  EFI_IP4_TRANSMIT_DATA     *Ip4TxData;
  EFI_IP6_TRANSMIT_DATA     *Ip6TxData;

  if ((IpIo->IpVersion != IP_VERSION_4) && (IpIo->IpVersion != IP_VERSION_6)) {
    return NULL;
  }

  Event        = NULL;
  TxData       = NULL;
  OverrideData = NULL;

  //
  // Allocate resource for SndEntry
  //
  SndEntry = AllocatePool (sizeof (IP_IO_SEND_ENTRY));
  if (NULL == SndEntry) {
    return NULL;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoTransmitHandler,
                  SndEntry,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  FragmentCount = Pkt->BlockOpNum;

  //
  // Allocate resource for TxData
  //
  TxData = (IP_IO_IP_TX_DATA *) AllocatePool (
    sizeof (IP_IO_IP_TX_DATA) + sizeof (NET_FRAGMENT) * (FragmentCount - 1)
    );

  if (NULL == TxData) {
    goto ON_ERROR;
  }

  //
  // Build a fragment table to contain the fragments in the packet.
  //
  if (IpIo->IpVersion == IP_VERSION_4) {
    ExtFragment = (NET_FRAGMENT *) TxData->Ip4TxData.FragmentTable;
  } else {
    ExtFragment = (NET_FRAGMENT *) TxData->Ip6TxData.FragmentTable;
  }

  NetbufBuildExt (Pkt, ExtFragment, &FragmentCount);


  //
  // Allocate resource for OverrideData if needed
  //
  if (NULL != Override) {

    OverrideData = AllocateCopyPool (sizeof (IP_IO_OVERRIDE), Override);
    if (NULL == OverrideData) {
      goto ON_ERROR;
    }
  }

  //
  // Set other fields of TxData except the fragment table
  //
  if (IpIo->IpVersion == IP_VERSION_4) {

    Ip4TxData = &TxData->Ip4TxData;

    IP4_COPY_ADDRESS (&Ip4TxData->DestinationAddress, Dest);

    Ip4TxData->OverrideData    = &OverrideData->Ip4OverrideData;
    Ip4TxData->OptionsLength   = 0;
    Ip4TxData->OptionsBuffer   = NULL;
    Ip4TxData->TotalDataLength = Pkt->TotalSize;
    Ip4TxData->FragmentCount   = FragmentCount;

    //
    // Set the fields of SndToken
    //
    SndEntry->SndToken.Ip4Token.Event         = Event;
    SndEntry->SndToken.Ip4Token.Packet.TxData = Ip4TxData;
  } else {

    Ip6TxData = &TxData->Ip6TxData;

    if (Dest != NULL) {
      CopyMem (&Ip6TxData->DestinationAddress, Dest, sizeof (EFI_IPv6_ADDRESS));
    } else {
      ZeroMem (&Ip6TxData->DestinationAddress, sizeof (EFI_IPv6_ADDRESS));
    }

    Ip6TxData->OverrideData  = &OverrideData->Ip6OverrideData;
    Ip6TxData->DataLength    = Pkt->TotalSize;
    Ip6TxData->FragmentCount = FragmentCount;
    Ip6TxData->ExtHdrsLength = 0;
    Ip6TxData->ExtHdrs       = NULL;

    //
    // Set the fields of SndToken
    //
    SndEntry->SndToken.Ip6Token.Event         = Event;
    SndEntry->SndToken.Ip6Token.Packet.TxData = Ip6TxData;
  }

  //
  // Set the fields of SndEntry
  //
  SndEntry->IpIo        = IpIo;
  SndEntry->Ip          = Sender;
  SndEntry->Context     = Context;
  SndEntry->NotifyData  = NotifyData;

  SndEntry->Pkt         = Pkt;
  NET_GET_REF (Pkt);

  InsertTailList (&IpIo->PendingSndList, &SndEntry->Entry);

  return SndEntry;

ON_ERROR:

  if (OverrideData != NULL) {
    FreePool (OverrideData);
  }

  if (TxData != NULL) {
    FreePool (TxData);
  }

  if (SndEntry != NULL) {
    FreePool (SndEntry);
  }

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  return NULL;
}


/**
  Destroy the SndEntry.

  This function pairs with IpIoCreateSndEntry().

  @param[in]  SndEntry              Pointer to the send entry to be destroyed.

**/
VOID
IpIoDestroySndEntry (
  IN IP_IO_SEND_ENTRY  *SndEntry
  )
{
  EFI_EVENT         Event;
  IP_IO_IP_TX_DATA  *TxData;
  IP_IO_OVERRIDE    *Override;

  if (SndEntry->IpIo->IpVersion == IP_VERSION_4) {
    Event              = SndEntry->SndToken.Ip4Token.Event;
    TxData             = (IP_IO_IP_TX_DATA *) SndEntry->SndToken.Ip4Token.Packet.TxData;
    Override           = (IP_IO_OVERRIDE *) TxData->Ip4TxData.OverrideData;
  } else if (SndEntry->IpIo->IpVersion == IP_VERSION_6) {
    Event              = SndEntry->SndToken.Ip6Token.Event;
    TxData             = (IP_IO_IP_TX_DATA *) SndEntry->SndToken.Ip6Token.Packet.TxData;
    Override           = (IP_IO_OVERRIDE *) TxData->Ip6TxData.OverrideData;
  } else {
    return ;
  }

  gBS->CloseEvent (Event);

  FreePool (TxData);

  if (NULL != Override) {
    FreePool (Override);
  }

  NetbufFree (SndEntry->Pkt);

  RemoveEntryList (&SndEntry->Entry);

  FreePool (SndEntry);
}


/**
  Notify function for IP transmit token.

  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoTransmitHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO             *IpIo;
  IP_IO_SEND_ENTRY  *SndEntry;
  EFI_STATUS        Status;

  SndEntry  = (IP_IO_SEND_ENTRY *) Context;

  IpIo      = SndEntry->IpIo;

  if (IpIo->IpVersion == IP_VERSION_4) {
    Status = SndEntry->SndToken.Ip4Token.Status;
  } else if (IpIo->IpVersion == IP_VERSION_6){
    Status = SndEntry->SndToken.Ip6Token.Status;
  } else {
    return ;
  }

  if ((IpIo->PktSentNotify != NULL) && (SndEntry->NotifyData != NULL)) {
    IpIo->PktSentNotify (
            Status,
            SndEntry->Context,
            SndEntry->Ip,
            SndEntry->NotifyData
            );
  }

  IpIoDestroySndEntry (SndEntry);
}


/**
  Notify function for IP transmit token.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoTransmitHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoTransmitHandlerDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, IpIoTransmitHandlerDpc, Context);
}


/**
  The dummy handler for the dummy IP receive token.

  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoDummyHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO_IP_INFO             *IpInfo;
  EFI_STATUS                 Status;
  EFI_EVENT                  RecycleEvent;

  IpInfo      = (IP_IO_IP_INFO *) Context;

  if ((IpInfo->IpVersion != IP_VERSION_4) && (IpInfo->IpVersion != IP_VERSION_6)) {
    return ;
  }

  RecycleEvent = NULL;

  if (IpInfo->IpVersion == IP_VERSION_4) {
    Status = IpInfo->DummyRcvToken.Ip4Token.Status;

    if (IpInfo->DummyRcvToken.Ip4Token.Packet.RxData != NULL) {
      RecycleEvent = IpInfo->DummyRcvToken.Ip4Token.Packet.RxData->RecycleSignal;
    }
  } else {
    Status = IpInfo->DummyRcvToken.Ip6Token.Status;

    if (IpInfo->DummyRcvToken.Ip6Token.Packet.RxData != NULL) {
      RecycleEvent = IpInfo->DummyRcvToken.Ip6Token.Packet.RxData->RecycleSignal;
    }
  }



  if (EFI_ABORTED == Status) {
    //
    // The reception is actively aborted by the consumer, directly return.
    //
    return;
  } else if (EFI_SUCCESS == Status) {
    //
    // Recycle the RxData.
    //
    ASSERT (RecycleEvent != NULL);

    gBS->SignalEvent (RecycleEvent);
  }

  //
  // Continue the receive.
  //
  if (IpInfo->IpVersion == IP_VERSION_4) {
    IpInfo->Ip.Ip4->Receive (
                      IpInfo->Ip.Ip4,
                      &IpInfo->DummyRcvToken.Ip4Token
                      );
  } else {
    IpInfo->Ip.Ip6->Receive (
                      IpInfo->Ip.Ip6,
                      &IpInfo->DummyRcvToken.Ip6Token
                      );
  }
}


/**
  This function add IpIoDummyHandlerDpc to the end of the DPC queue.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoDummyHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoDummyHandlerDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, IpIoDummyHandlerDpc, Context);
}


/**
  Notify function for the IP receive token, used to process
  the received IP packets.

  @param[in]  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoListenHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO                 *IpIo;
  EFI_STATUS            Status;
  IP_IO_IP_RX_DATA      *RxData;
  EFI_NET_SESSION_DATA  Session;
  NET_BUF               *Pkt;

  IpIo = (IP_IO *) Context;

  if (IpIo->IpVersion == IP_VERSION_4) {
    Status = IpIo->RcvToken.Ip4Token.Status;
    RxData = (IP_IO_IP_RX_DATA *) IpIo->RcvToken.Ip4Token.Packet.RxData;
  } else if (IpIo->IpVersion == IP_VERSION_6) {
    Status = IpIo->RcvToken.Ip6Token.Status;
    RxData = (IP_IO_IP_RX_DATA *) IpIo->RcvToken.Ip6Token.Packet.RxData;
  } else {
    return;
  }

  if (EFI_ABORTED == Status) {
    //
    // The reception is actively aborted by the consumer, directly return.
    //
    return;
  }

  if ((EFI_SUCCESS != Status) && (EFI_ICMP_ERROR != Status)) {
    //
    // Only process the normal packets and the icmp error packets.
    //
    if (RxData != NULL) {
      goto CleanUp;
    } else {
      goto Resume;
    }
  }

  //
  // if RxData is NULL with Status == EFI_SUCCESS or EFI_ICMP_ERROR, this should be a code issue in the low layer (IP).
  //
  ASSERT (RxData != NULL);
  if (RxData == NULL) {
    goto Resume;
  }

  if (NULL == IpIo->PktRcvdNotify) {
    goto CleanUp;
  }

  if (IpIo->IpVersion == IP_VERSION_4) {
    ASSERT (RxData->Ip4RxData.Header != NULL);
    if (IP4_IS_LOCAL_BROADCAST (EFI_IP4 (RxData->Ip4RxData.Header->SourceAddress))) {
      //
      // The source address is a broadcast address, discard it.
      //
      goto CleanUp;
    }
    if ((EFI_IP4 (RxData->Ip4RxData.Header->SourceAddress) != 0) &&
        (IpIo->SubnetMask != 0) &&
        IP4_NET_EQUAL (IpIo->StationIp, EFI_NTOHL (((EFI_IP4_RECEIVE_DATA *) RxData)->Header->SourceAddress), IpIo->SubnetMask) &&
        !NetIp4IsUnicast (EFI_NTOHL (((EFI_IP4_RECEIVE_DATA *) RxData)->Header->SourceAddress), IpIo->SubnetMask)) {
      //
      // The source address doesn't match StationIp and it's not a unicast IP address, discard it.
      //
      goto CleanUp;
    }

    if (RxData->Ip4RxData.DataLength == 0) {
      //
      // Discard zero length data payload packet.
      //
      goto CleanUp;
    }

    //
    // The fragment should always be valid for non-zero length packet.
    //
    ASSERT (RxData->Ip4RxData.FragmentCount != 0);

    //
    // Create a netbuffer representing IPv4 packet
    //
    Pkt = NetbufFromExt (
            (NET_FRAGMENT *) RxData->Ip4RxData.FragmentTable,
            RxData->Ip4RxData.FragmentCount,
            0,
            0,
            IpIoExtFree,
            RxData->Ip4RxData.RecycleSignal
            );
    if (NULL == Pkt) {
      goto CleanUp;
    }

    //
    // Create a net session
    //
    Session.Source.Addr[0] = EFI_IP4 (RxData->Ip4RxData.Header->SourceAddress);
    Session.Dest.Addr[0]   = EFI_IP4 (RxData->Ip4RxData.Header->DestinationAddress);
    Session.IpHdr.Ip4Hdr   = RxData->Ip4RxData.Header;
    Session.IpHdrLen       = RxData->Ip4RxData.HeaderLength;
    Session.IpVersion      = IP_VERSION_4;
  } else {
    ASSERT (RxData->Ip6RxData.Header != NULL);
    if (!NetIp6IsValidUnicast(&RxData->Ip6RxData.Header->SourceAddress)) {
      goto CleanUp;
    }

    if (RxData->Ip6RxData.DataLength == 0) {
      //
      // Discard zero length data payload packet.
      //
      goto CleanUp;
    }

    //
    // The fragment should always be valid for non-zero length packet.
    //
    ASSERT (RxData->Ip6RxData.FragmentCount != 0);

    //
    // Create a netbuffer representing IPv6 packet
    //
    Pkt = NetbufFromExt (
            (NET_FRAGMENT *) RxData->Ip6RxData.FragmentTable,
            RxData->Ip6RxData.FragmentCount,
            0,
            0,
            IpIoExtFree,
            RxData->Ip6RxData.RecycleSignal
            );
    if (NULL == Pkt) {
      goto CleanUp;
    }

    //
    // Create a net session
    //
    CopyMem (
      &Session.Source,
      &RxData->Ip6RxData.Header->SourceAddress,
      sizeof(EFI_IPv6_ADDRESS)
      );
    CopyMem (
      &Session.Dest,
      &RxData->Ip6RxData.Header->DestinationAddress,
      sizeof(EFI_IPv6_ADDRESS)
      );
    Session.IpHdr.Ip6Hdr = RxData->Ip6RxData.Header;
    Session.IpHdrLen     = RxData->Ip6RxData.HeaderLength;
    Session.IpVersion    = IP_VERSION_6;
  }

  if (EFI_SUCCESS == Status) {

    IpIo->PktRcvdNotify (EFI_SUCCESS, 0, &Session, Pkt, IpIo->RcvdContext);
  } else {
    //
    // Status is EFI_ICMP_ERROR
    //
    Status = IpIoIcmpHandler (IpIo, Pkt, &Session);
    if (EFI_ERROR (Status)) {
      NetbufFree (Pkt);
    }
  }

  goto Resume;

CleanUp:

  if (IpIo->IpVersion == IP_VERSION_4){
    gBS->SignalEvent (RxData->Ip4RxData.RecycleSignal);
  } else {
    gBS->SignalEvent (RxData->Ip6RxData.RecycleSignal);
  }

Resume:

  if (IpIo->IpVersion == IP_VERSION_4){
    IpIo->Ip.Ip4->Receive (IpIo->Ip.Ip4, &(IpIo->RcvToken.Ip4Token));
  } else {
    IpIo->Ip.Ip6->Receive (IpIo->Ip.Ip6, &(IpIo->RcvToken.Ip6Token));
  }
}

/**
  This function add IpIoListenHandlerDpc to the end of the DPC queue.

  @param[in]  Event                The event signaled.
  @param[in]  Context              The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoListenHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoListenHandlerDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, IpIoListenHandlerDpc, Context);
}


/**
  Create a new IP_IO instance.

  If IpVersion is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  This function uses IP4/IP6 service binding protocol in Controller to create
  an IP4/IP6 child (aka IP4/IP6 instance).

  @param[in]  Image             The image handle of the driver or application that
                                consumes IP_IO.
  @param[in]  Controller        The controller handle that has IP4 or IP6 service
                                binding protocol installed.
  @param[in]  IpVersion         The version of the IP protocol to use, either
                                IPv4 or IPv6.

  @return Pointer to a newly created IP_IO instance, or NULL if failed.

**/
IP_IO *
EFIAPI
IpIoCreate (
  IN EFI_HANDLE Image,
  IN EFI_HANDLE Controller,
  IN UINT8      IpVersion
  )
{
  EFI_STATUS  Status;
  IP_IO       *IpIo;
  EFI_EVENT   Event;

  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  IpIo = AllocateZeroPool (sizeof (IP_IO));
  if (NULL == IpIo) {
    return NULL;
  }

  InitializeListHead (&(IpIo->PendingSndList));
  InitializeListHead (&(IpIo->IpList));
  IpIo->Controller  = Controller;
  IpIo->Image       = Image;
  IpIo->IpVersion   = IpVersion;
  Event             = NULL;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoListenHandler,
                  IpIo,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpIo;
  }

  if (IpVersion == IP_VERSION_4) {
    IpIo->RcvToken.Ip4Token.Event = Event;
  } else {
    IpIo->RcvToken.Ip6Token.Event = Event;
  }

  //
  // Create an IP child and open IP protocol
  //
  Status = IpIoCreateIpChildOpenProtocol (
             Controller,
             Image,
             &IpIo->ChildHandle,
             IpVersion,
             (VOID **) & (IpIo->Ip)
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpIo;
  }

  return IpIo;

ReleaseIpIo:

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  gBS->FreePool (IpIo);

  return NULL;
}


/**
  Open an IP_IO instance for use.

  If Ip version is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  This function is called after IpIoCreate(). It is used for configuring the IP
  instance and register the callbacks and their context data for sending and
  receiving IP packets.

  @param[in, out]  IpIo               Pointer to an IP_IO instance that needs
                                      to open.
  @param[in]       OpenData           The configuration data and callbacks for
                                      the IP_IO instance.

  @retval          EFI_SUCCESS            The IP_IO instance opened with OpenData
                                          successfully.
  @retval          EFI_ACCESS_DENIED      The IP_IO instance is configured, avoid to
                                          reopen it.
  @retval          EFI_UNSUPPORTED        IPv4 RawData mode is no supported.
  @retval          EFI_INVALID_PARAMETER  Invalid input parameter.
  @retval          Others                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoOpen (
  IN OUT IP_IO           *IpIo,
  IN     IP_IO_OPEN_DATA *OpenData
  )
{
  EFI_STATUS        Status;
  UINT8             IpVersion;

  if (IpIo == NULL || OpenData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IpIo->IsConfigured) {
    return EFI_ACCESS_DENIED;
  }

  IpVersion = IpIo->IpVersion;

  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  //
  // configure ip
  //
  if (IpVersion == IP_VERSION_4){
    //
    // RawData mode is no supported.
    //
    ASSERT (!OpenData->IpConfigData.Ip4CfgData.RawData);
    if (OpenData->IpConfigData.Ip4CfgData.RawData) {
      return EFI_UNSUPPORTED;
    }

    if (!OpenData->IpConfigData.Ip4CfgData.UseDefaultAddress) {
      IpIo->StationIp = EFI_NTOHL (OpenData->IpConfigData.Ip4CfgData.StationAddress);
      IpIo->SubnetMask = EFI_NTOHL (OpenData->IpConfigData.Ip4CfgData.SubnetMask);
    }

    Status = IpIo->Ip.Ip4->Configure (
                             IpIo->Ip.Ip4,
                             &OpenData->IpConfigData.Ip4CfgData
                             );
  } else {

    Status = IpIo->Ip.Ip6->Configure (
                             IpIo->Ip.Ip6,
                             &OpenData->IpConfigData.Ip6CfgData
                             );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // @bug To delete the default route entry in this Ip, if it is:
  // @bug (0.0.0.0, 0.0.0.0, 0.0.0.0). Delete this statement if Ip modified
  // @bug its code
  //
  if (IpVersion == IP_VERSION_4){
    Status = IpIo->Ip.Ip4->Routes (
                             IpIo->Ip.Ip4,
                             TRUE,
                             &mZeroIp4Addr,
                             &mZeroIp4Addr,
                             &mZeroIp4Addr
                             );

    if (EFI_ERROR (Status) && (EFI_NOT_FOUND != Status)) {
      return Status;
    }
  }

  IpIo->PktRcvdNotify = OpenData->PktRcvdNotify;
  IpIo->PktSentNotify = OpenData->PktSentNotify;

  IpIo->RcvdContext   = OpenData->RcvdContext;
  IpIo->SndContext    = OpenData->SndContext;

  if (IpVersion == IP_VERSION_4){
    IpIo->Protocol = OpenData->IpConfigData.Ip4CfgData.DefaultProtocol;

    //
    // start to listen incoming packet
    //
    Status = IpIo->Ip.Ip4->Receive (
                             IpIo->Ip.Ip4,
                             &(IpIo->RcvToken.Ip4Token)
                             );
    if (EFI_ERROR (Status)) {
      IpIo->Ip.Ip4->Configure (IpIo->Ip.Ip4, NULL);
      return Status;
    }

  } else {

    IpIo->Protocol = OpenData->IpConfigData.Ip6CfgData.DefaultProtocol;
    Status = IpIo->Ip.Ip6->Receive (
                             IpIo->Ip.Ip6,
                             &(IpIo->RcvToken.Ip6Token)
                             );
    if (EFI_ERROR (Status)) {
      IpIo->Ip.Ip6->Configure (IpIo->Ip.Ip6, NULL);
      return Status;
    }
  }

  IpIo->IsConfigured = TRUE;
  InsertTailList (&mActiveIpIoList, &IpIo->Entry);

  return Status;
}


/**
  Stop an IP_IO instance.

  If Ip version is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  This function is paired with IpIoOpen(). The IP_IO will be unconfigured and all
  the pending send/receive tokens will be canceled.

  @param[in, out]  IpIo            Pointer to the IP_IO instance that needs to stop.

  @retval          EFI_SUCCESS            The IP_IO instance stopped successfully.
  @retval          EFI_INVALID_PARAMETER  Invalid input parameter.
  @retval          Others                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoStop (
  IN OUT IP_IO *IpIo
  )
{
  EFI_STATUS        Status;
  IP_IO_IP_INFO     *IpInfo;
  UINT8             IpVersion;

  if (IpIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IpIo->IsConfigured) {
    return EFI_SUCCESS;
  }

  IpVersion = IpIo->IpVersion;

  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  //
  // Remove the IpIo from the active IpIo list.
  //
  RemoveEntryList (&IpIo->Entry);

  //
  // Configure NULL Ip
  //
  if (IpVersion == IP_VERSION_4) {
    Status = IpIo->Ip.Ip4->Configure (IpIo->Ip.Ip4, NULL);
  } else {
    Status = IpIo->Ip.Ip6->Configure (IpIo->Ip.Ip6, NULL);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IpIo->IsConfigured = FALSE;

  //
  // Detroy the Ip List used by IpIo
  //

  while (!IsListEmpty (&(IpIo->IpList))) {
    IpInfo = NET_LIST_HEAD (&(IpIo->IpList), IP_IO_IP_INFO, Entry);

    IpIoRemoveIp (IpIo, IpInfo);
  }

  //
  // All pending send tokens should be flushed by resetting the IP instances.
  //
  ASSERT (IsListEmpty (&IpIo->PendingSndList));

  //
  // Close the receive event.
  //
  if (IpVersion == IP_VERSION_4){
    gBS->CloseEvent (IpIo->RcvToken.Ip4Token.Event);
  } else {
    gBS->CloseEvent (IpIo->RcvToken.Ip6Token.Event);
  }

  return EFI_SUCCESS;
}


/**
  Destroy an IP_IO instance.

  This function is paired with IpIoCreate(). The IP_IO will be closed first.
  Resource will be freed afterwards. See IpIoCloseProtocolDestroyIpChild().

  @param[in, out]  IpIo         Pointer to the IP_IO instance that needs to be
                                destroyed.

  @retval          EFI_SUCCESS  The IP_IO instance destroyed successfully.
  @retval          Others       Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoDestroy (
  IN OUT IP_IO *IpIo
  )
{
  EFI_STATUS    Status;

  //
  // Stop the IpIo.
  //
  Status = IpIoStop (IpIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the IP protocol and destroy the child.
  //
  Status = IpIoCloseProtocolDestroyIpChild (
             IpIo->Controller,
             IpIo->Image,
             IpIo->ChildHandle,
             IpIo->IpVersion
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->FreePool (IpIo);

  return EFI_SUCCESS;
}


/**
  Send out an IP packet.

  This function is called after IpIoOpen(). The data to be sent is wrapped in
  Pkt. The IP instance wrapped in IpIo is used for sending by default but can be
  overriden by Sender. Other sending configs, like source address and gateway
  address etc., are specified in OverrideData.

  @param[in, out]  IpIo                  Pointer to an IP_IO instance used for sending IP
                                         packet.
  @param[in, out]  Pkt                   Pointer to the IP packet to be sent.
  @param[in]       Sender                The IP protocol instance used for sending.
  @param[in]       Context               Optional context data.
  @param[in]       NotifyData            Optional notify data.
  @param[in]       Dest                  The destination IP address to send this packet to.
                                         This parameter is optional when using IPv6.
  @param[in]       OverrideData          The data to override some configuration of the IP
                                         instance used for sending.

  @retval          EFI_SUCCESS           The operation is completed successfully.
  @retval          EFI_INVALID_PARAMETER The input parameter is not correct.
  @retval          EFI_NOT_STARTED       The IpIo is not configured.
  @retval          EFI_OUT_OF_RESOURCES  Failed due to resource limit.
  @retval          Others                Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoSend (
  IN OUT IP_IO          *IpIo,
  IN OUT NET_BUF        *Pkt,
  IN     IP_IO_IP_INFO  *Sender        OPTIONAL,
  IN     VOID           *Context       OPTIONAL,
  IN     VOID           *NotifyData    OPTIONAL,
  IN     EFI_IP_ADDRESS *Dest          OPTIONAL,
  IN     IP_IO_OVERRIDE *OverrideData  OPTIONAL
  )
{
  EFI_STATUS        Status;
  IP_IO_IP_PROTOCOL Ip;
  IP_IO_SEND_ENTRY  *SndEntry;

  if ((IpIo == NULL) || (Pkt == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((IpIo->IpVersion == IP_VERSION_4) && (Dest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IpIo->IsConfigured) {
    return EFI_NOT_STARTED;
  }

  Ip = (NULL == Sender) ? IpIo->Ip : Sender->Ip;

  //
  // create a new SndEntry
  //
  SndEntry = IpIoCreateSndEntry (IpIo, Pkt, Ip, Context, NotifyData, Dest, OverrideData);
  if (NULL == SndEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Send this Packet
  //
  if (IpIo->IpVersion == IP_VERSION_4){
    Status = Ip.Ip4->Transmit (
                       Ip.Ip4,
                       &SndEntry->SndToken.Ip4Token
                       );
  } else {
    Status = Ip.Ip6->Transmit (
                       Ip.Ip6,
                       &SndEntry->SndToken.Ip6Token
                       );
  }

  if (EFI_ERROR (Status)) {
    IpIoDestroySndEntry (SndEntry);
  }

  return Status;
}


/**
  Cancel the IP transmit token which wraps this Packet.

  If IpIo is NULL, then ASSERT().
  If Packet is NULL, then ASSERT().

  @param[in]  IpIo                  Pointer to the IP_IO instance.
  @param[in]  Packet                Pointer to the packet of NET_BUF to cancel.

**/
VOID
EFIAPI
IpIoCancelTxToken (
  IN IP_IO  *IpIo,
  IN VOID   *Packet
  )
{
  LIST_ENTRY        *Node;
  IP_IO_SEND_ENTRY  *SndEntry;
  IP_IO_IP_PROTOCOL Ip;

  ASSERT ((IpIo != NULL) && (Packet != NULL));

  NET_LIST_FOR_EACH (Node, &IpIo->PendingSndList) {

    SndEntry = NET_LIST_USER_STRUCT (Node, IP_IO_SEND_ENTRY, Entry);

    if (SndEntry->Pkt == Packet) {

      Ip = SndEntry->Ip;

      if (IpIo->IpVersion == IP_VERSION_4) {
        Ip.Ip4->Cancel (
                  Ip.Ip4,
                  &SndEntry->SndToken.Ip4Token
                  );
      } else {
        Ip.Ip6->Cancel (
                  Ip.Ip6,
                  &SndEntry->SndToken.Ip6Token
                  );
      }

      break;
    }
  }

}


/**
  Add a new IP instance for sending data.

  If IpIo is NULL, then ASSERT().
  If Ip version is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  The function is used to add the IP_IO to the IP_IO sending list. The caller
  can later use IpIoFindSender() to get the IP_IO and call IpIoSend() to send
  data.

  @param[in, out]  IpIo               Pointer to a IP_IO instance to add a new IP
                                      instance for sending purpose.

  @return Pointer to the created IP_IO_IP_INFO structure, NULL if failed.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoAddIp (
  IN OUT IP_IO  *IpIo
  )
{
  EFI_STATUS     Status;
  IP_IO_IP_INFO  *IpInfo;
  EFI_EVENT      Event;

  ASSERT (IpIo != NULL);
  ASSERT ((IpIo->IpVersion == IP_VERSION_4) || (IpIo->IpVersion == IP_VERSION_6));

  IpInfo = AllocatePool (sizeof (IP_IO_IP_INFO));
  if (IpInfo == NULL) {
    return NULL;
  }

  //
  // Init this IpInfo, set the Addr and SubnetMask to 0 before we configure the IP
  // instance.
  //
  InitializeListHead (&IpInfo->Entry);
  IpInfo->ChildHandle = NULL;
  ZeroMem (&IpInfo->Addr, sizeof (IpInfo->Addr));
  ZeroMem (&IpInfo->PreMask, sizeof (IpInfo->PreMask));

  IpInfo->RefCnt    = 1;
  IpInfo->IpVersion = IpIo->IpVersion;

  //
  // Create the IP instance and open the IP protocol.
  //
  Status = IpIoCreateIpChildOpenProtocol (
             IpIo->Controller,
             IpIo->Image,
             &IpInfo->ChildHandle,
             IpInfo->IpVersion,
             (VOID **) &IpInfo->Ip
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpInfo;
  }

  //
  // Create the event for the DummyRcvToken.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoDummyHandler,
                  IpInfo,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpChild;
  }

  if (IpInfo->IpVersion == IP_VERSION_4) {
    IpInfo->DummyRcvToken.Ip4Token.Event = Event;
  } else {
    IpInfo->DummyRcvToken.Ip6Token.Event = Event;
  }

  //
  // Link this IpInfo into the IpIo.
  //
  InsertTailList (&IpIo->IpList, &IpInfo->Entry);

  return IpInfo;

ReleaseIpChild:

  IpIoCloseProtocolDestroyIpChild (
    IpIo->Controller,
    IpIo->Image,
    IpInfo->ChildHandle,
    IpInfo->IpVersion
    );

ReleaseIpInfo:

  gBS->FreePool (IpInfo);

  return NULL;
}


/**
  Configure the IP instance of this IpInfo and start the receiving if IpConfigData
  is not NULL.

  If IpInfo is NULL, then ASSERT().
  If Ip version is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  @param[in, out]  IpInfo          Pointer to the IP_IO_IP_INFO instance.
  @param[in, out]  IpConfigData    The IP configure data used to configure the IP
                                   instance, if NULL the IP instance is reset. If
                                   UseDefaultAddress is set to TRUE, and the configure
                                   operation succeeds, the default address information
                                   is written back in this IpConfigData.

  @retval          EFI_SUCCESS     The IP instance of this IpInfo is configured successfully
                                   or no need to reconfigure it.
  @retval          Others          Configuration fails.

**/
EFI_STATUS
EFIAPI
IpIoConfigIp (
  IN OUT IP_IO_IP_INFO        *IpInfo,
  IN OUT VOID                 *IpConfigData OPTIONAL
  )
{
  EFI_STATUS         Status;
  IP_IO_IP_PROTOCOL  Ip;
  UINT8              IpVersion;
  EFI_IP4_MODE_DATA  Ip4ModeData;
  EFI_IP6_MODE_DATA  Ip6ModeData;

  ASSERT (IpInfo != NULL);

  if (IpInfo->RefCnt > 1) {
    //
    // This IP instance is shared, don't reconfigure it until it has only one
    // consumer. Currently, only the tcp children cloned from their passive parent
    // will share the same IP. So this cases only happens while IpConfigData is NULL,
    // let the last consumer clean the IP instance.
    //
    return EFI_SUCCESS;
  }

  IpVersion = IpInfo->IpVersion;
  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  Ip = IpInfo->Ip;

  if (IpInfo->IpVersion == IP_VERSION_4) {
    Status = Ip.Ip4->Configure (Ip.Ip4, IpConfigData);
  } else {
    Status = Ip.Ip6->Configure (Ip.Ip6, IpConfigData);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IpConfigData != NULL) {
    if (IpInfo->IpVersion == IP_VERSION_4) {

      if (((EFI_IP4_CONFIG_DATA *) IpConfigData)->UseDefaultAddress) {
        Status = Ip.Ip4->GetModeData (
                           Ip.Ip4,
                           &Ip4ModeData,
                           NULL,
                           NULL
                           );
        if (EFI_ERROR (Status)) {
          Ip.Ip4->Configure (Ip.Ip4, NULL);
          return Status;
        }

        IP4_COPY_ADDRESS (&((EFI_IP4_CONFIG_DATA*) IpConfigData)->StationAddress, &Ip4ModeData.ConfigData.StationAddress);
        IP4_COPY_ADDRESS (&((EFI_IP4_CONFIG_DATA*) IpConfigData)->SubnetMask, &Ip4ModeData.ConfigData.SubnetMask);
      }

      CopyMem (
        &IpInfo->Addr.Addr,
        &((EFI_IP4_CONFIG_DATA *) IpConfigData)->StationAddress,
        sizeof (IP4_ADDR)
        );
      CopyMem (
        &IpInfo->PreMask.SubnetMask,
        &((EFI_IP4_CONFIG_DATA *) IpConfigData)->SubnetMask,
        sizeof (IP4_ADDR)
        );

      Status = Ip.Ip4->Receive (
                         Ip.Ip4,
                         &IpInfo->DummyRcvToken.Ip4Token
                         );
      if (EFI_ERROR (Status)) {
        Ip.Ip4->Configure (Ip.Ip4, NULL);
      }
    } else {
      Status = Ip.Ip6->GetModeData (
                         Ip.Ip6,
                         &Ip6ModeData,
                         NULL,
                         NULL
                         );
      if (EFI_ERROR (Status)) {
        Ip.Ip6->Configure (Ip.Ip6, NULL);
        return Status;
      }

      if (Ip6ModeData.IsConfigured) {
        CopyMem (
          &((EFI_IP6_CONFIG_DATA *) IpConfigData)->StationAddress,
          &Ip6ModeData.ConfigData.StationAddress,
          sizeof (EFI_IPv6_ADDRESS)
          );

        if (Ip6ModeData.AddressList != NULL) {
          FreePool (Ip6ModeData.AddressList);
        }

        if (Ip6ModeData.GroupTable != NULL) {
          FreePool (Ip6ModeData.GroupTable);
        }

        if (Ip6ModeData.RouteTable != NULL) {
          FreePool (Ip6ModeData.RouteTable);
        }

        if (Ip6ModeData.NeighborCache != NULL) {
          FreePool (Ip6ModeData.NeighborCache);
        }

        if (Ip6ModeData.PrefixTable != NULL) {
          FreePool (Ip6ModeData.PrefixTable);
        }

        if (Ip6ModeData.IcmpTypeList != NULL) {
          FreePool (Ip6ModeData.IcmpTypeList);
        }

      } else {
        Status = EFI_NO_MAPPING;
        return Status;
      }

      CopyMem (
        &IpInfo->Addr,
        &Ip6ModeData.ConfigData.StationAddress,
        sizeof (EFI_IPv6_ADDRESS)
        );

      Status = Ip.Ip6->Receive (
                         Ip.Ip6,
                         &IpInfo->DummyRcvToken.Ip6Token
                         );
      if (EFI_ERROR (Status)) {
        Ip.Ip6->Configure (Ip.Ip6, NULL);
      }
    }
  } else {
    //
    // The IP instance is reset, set the stored Addr and SubnetMask to zero.
    //
    ZeroMem (&IpInfo->Addr, sizeof (IpInfo->Addr));
    ZeroMem (&IpInfo->PreMask, sizeof (IpInfo->PreMask));
  }

  return Status;
}


/**
  Destroy an IP instance maintained in IpIo->IpList for
  sending purpose.

  If Ip version is not IP_VERSION_4 or IP_VERSION_6, then ASSERT().

  This function pairs with IpIoAddIp(). The IpInfo is previously created by
  IpIoAddIp(). The IP_IO_IP_INFO::RefCnt is decremented and the IP instance
  will be dstroyed if the RefCnt is zero.

  @param[in]  IpIo                  Pointer to the IP_IO instance.
  @param[in]  IpInfo                Pointer to the IpInfo to be removed.

**/
VOID
EFIAPI
IpIoRemoveIp (
  IN IP_IO            *IpIo,
  IN IP_IO_IP_INFO    *IpInfo
  )
{

  UINT8               IpVersion;

  if (IpIo == NULL || IpInfo == NULL) {
    return;
  }

  ASSERT (IpInfo->RefCnt > 0);

  NET_PUT_REF (IpInfo);

  if (IpInfo->RefCnt > 0) {

    return;
  }

  IpVersion = IpIo->IpVersion;

  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  RemoveEntryList (&IpInfo->Entry);

  if (IpVersion == IP_VERSION_4){
    IpInfo->Ip.Ip4->Configure (
                      IpInfo->Ip.Ip4,
                      NULL
                      );
    IpIoCloseProtocolDestroyIpChild (
      IpIo->Controller,
      IpIo->Image,
      IpInfo->ChildHandle,
      IP_VERSION_4
      );

    gBS->CloseEvent (IpInfo->DummyRcvToken.Ip4Token.Event);

  } else {

    IpInfo->Ip.Ip6->Configure (
                      IpInfo->Ip.Ip6,
                      NULL
                      );

    IpIoCloseProtocolDestroyIpChild (
      IpIo->Controller,
      IpIo->Image,
      IpInfo->ChildHandle,
      IP_VERSION_6
      );

    gBS->CloseEvent (IpInfo->DummyRcvToken.Ip6Token.Event);
  }

  FreePool (IpInfo);
}


/**
  Find the first IP protocol maintained in IpIo whose local
  address is the same as Src.

  This function is called when the caller needs the IpIo to send data to the
  specified Src. The IpIo was added previously by IpIoAddIp().

  @param[in, out]  IpIo              Pointer to the pointer of the IP_IO instance.
  @param[in]       IpVersion         The version of the IP protocol to use, either
                                     IPv4 or IPv6.
  @param[in]       Src               The local IP address.

  @return Pointer to the IP protocol can be used for sending purpose and its local
          address is the same with Src. NULL if failed.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoFindSender (
  IN OUT IP_IO           **IpIo,
  IN     UINT8           IpVersion,
  IN     EFI_IP_ADDRESS  *Src
  )
{
  LIST_ENTRY      *IpIoEntry;
  IP_IO           *IpIoPtr;
  LIST_ENTRY      *IpInfoEntry;
  IP_IO_IP_INFO   *IpInfo;

  if (IpIo == NULL || Src == NULL) {
    return NULL;
  }

  if ((IpVersion != IP_VERSION_4) && (IpVersion != IP_VERSION_6)) {
    return NULL;
  }

  NET_LIST_FOR_EACH (IpIoEntry, &mActiveIpIoList) {
    IpIoPtr = NET_LIST_USER_STRUCT (IpIoEntry, IP_IO, Entry);

    if (((*IpIo != NULL) && (*IpIo != IpIoPtr)) || (IpIoPtr->IpVersion != IpVersion)) {
      continue;
    }

    NET_LIST_FOR_EACH (IpInfoEntry, &IpIoPtr->IpList) {
      IpInfo = NET_LIST_USER_STRUCT (IpInfoEntry, IP_IO_IP_INFO, Entry);
      if (IpInfo->IpVersion == IP_VERSION_4){

        if (EFI_IP4_EQUAL (&IpInfo->Addr.v4, &Src->v4)) {
          *IpIo = IpIoPtr;
          return IpInfo;
        }

      } else {

        if (EFI_IP6_EQUAL (&IpInfo->Addr.v6, &Src->v6)) {
          *IpIo = IpIoPtr;
          return IpInfo;
        }
      }
    }
  }

  //
  // No match.
  //
  return NULL;
}


/**
  Get the ICMP error map information.

  The ErrorStatus will be returned. The IsHard and Notify are optional. If they
  are not NULL, this routine will fill them.

  @param[in]   IcmpError             IcmpError Type.
  @param[in]   IpVersion             The version of the IP protocol to use,
                                     either IPv4 or IPv6.
  @param[out]  IsHard                If TRUE, indicates that it is a hard error.
  @param[out]  Notify                If TRUE, SockError needs to be notified.

  @retval EFI_UNSUPPORTED            Unrecognizable ICMP error code.
  @return ICMP Error Status, such as EFI_NETWORK_UNREACHABLE.

**/
EFI_STATUS
EFIAPI
IpIoGetIcmpErrStatus (
  IN  UINT8       IcmpError,
  IN  UINT8       IpVersion,
  OUT BOOLEAN     *IsHard  OPTIONAL,
  OUT BOOLEAN     *Notify  OPTIONAL
  )
{
  if (IpVersion == IP_VERSION_4 ) {
    ASSERT (IcmpError <= ICMP_ERR_PARAMPROB);

    if (IsHard != NULL) {
      *IsHard = mIcmpErrMap[IcmpError].IsHard;
    }

    if (Notify != NULL) {
      *Notify = mIcmpErrMap[IcmpError].Notify;
    }

    switch (IcmpError) {
    case ICMP_ERR_UNREACH_NET:
      return  EFI_NETWORK_UNREACHABLE;

    case ICMP_ERR_TIMXCEED_INTRANS:
    case ICMP_ERR_TIMXCEED_REASS:
    case ICMP_ERR_UNREACH_HOST:
      return  EFI_HOST_UNREACHABLE;

    case ICMP_ERR_UNREACH_PROTOCOL:
      return  EFI_PROTOCOL_UNREACHABLE;

    case ICMP_ERR_UNREACH_PORT:
      return  EFI_PORT_UNREACHABLE;

    case ICMP_ERR_MSGSIZE:
    case ICMP_ERR_UNREACH_SRCFAIL:
    case ICMP_ERR_QUENCH:
    case ICMP_ERR_PARAMPROB:
      return  EFI_ICMP_ERROR;

    default:
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

  } else if (IpVersion == IP_VERSION_6) {

    ASSERT (IcmpError <= ICMP6_ERR_PARAMPROB_IPV6OPTION);

    if (IsHard != NULL) {
      *IsHard = mIcmp6ErrMap[IcmpError].IsHard;
    }

    if (Notify != NULL) {
      *Notify = mIcmp6ErrMap[IcmpError].Notify;
    }

    switch (IcmpError) {
    case ICMP6_ERR_UNREACH_NET:
      return EFI_NETWORK_UNREACHABLE;

    case ICMP6_ERR_UNREACH_HOST:
    case ICMP6_ERR_TIMXCEED_HOPLIMIT:
    case ICMP6_ERR_TIMXCEED_REASS:
      return EFI_HOST_UNREACHABLE;

    case ICMP6_ERR_UNREACH_PROTOCOL:
      return EFI_PROTOCOL_UNREACHABLE;

    case ICMP6_ERR_UNREACH_PORT:
      return EFI_PORT_UNREACHABLE;

    case ICMP6_ERR_PACKAGE_TOOBIG:
    case ICMP6_ERR_PARAMPROB_HEADER:
    case ICMP6_ERR_PARAMPROB_NEXHEADER:
    case ICMP6_ERR_PARAMPROB_IPV6OPTION:
      return EFI_ICMP_ERROR;

    default:
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

  } else {
    //
    // Should never be here
    //
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }
}


/**
  Refresh the remote peer's Neighbor Cache entries.

  This function is called when the caller needs the IpIo to refresh the existing
  IPv6 neighbor cache entries since the neighbor is considered reachable by the
  node has recently received a confirmation that packets sent recently to the
  neighbor were received by its IP layer.

  @param[in]   IpIo                  Pointer to an IP_IO instance
  @param[in]   Neighbor              The IP address of the neighbor
  @param[in]   Timeout               Time in 100-ns units that this entry will
                                     remain in the neighbor cache. A value of
                                     zero means that the entry is permanent.
                                     A value of non-zero means that the entry is
                                     dynamic and will be deleted after Timeout.

  @retval      EFI_SUCCESS           The operation is completed successfully.
  @retval      EFI_NOT_STARTED       The IpIo is not configured.
  @retval      EFI_INVALID_PARAMETER Neighbor Address is invalid.
  @retval      EFI_NOT_FOUND         The neighbor cache entry is not in the
                                     neighbor table.
  @retval      EFI_UNSUPPORTED       IP version is IPv4, which doesn't support neighbor cache refresh.
  @retval      EFI_OUT_OF_RESOURCES  Failed due to resource limit.

**/
EFI_STATUS
EFIAPI
IpIoRefreshNeighbor (
  IN IP_IO           *IpIo,
  IN EFI_IP_ADDRESS  *Neighbor,
  IN UINT32          Timeout
  )
{
  EFI_IP6_PROTOCOL  *Ip;

  if (!IpIo->IsConfigured) {
    return EFI_NOT_STARTED;
  }

  if (IpIo->IpVersion != IP_VERSION_6) {
    return EFI_UNSUPPORTED;
  }

  Ip = IpIo->Ip.Ip6;

  return Ip->Neighbors (Ip, FALSE, &Neighbor->v6, NULL, Timeout, TRUE);
}

