/** @file
  The ICMPv6 handle routines to process the ICMPv6 control messages.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip6Impl.h"

EFI_IP6_ICMP_TYPE mIp6SupportedIcmp[23] = {

  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_NO_ROUTE_TO_DEST
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_COMM_PROHIBITED
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_BEYOND_SCOPE
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_ADDR_UNREACHABLE
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_PORT_UNREACHABLE
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_SOURCE_ADDR_FAILED
  },
  {
    ICMP_V6_DEST_UNREACHABLE,
    ICMP_V6_ROUTE_REJECTED
  },

  {
    ICMP_V6_PACKET_TOO_BIG,
    ICMP_V6_DEFAULT_CODE
  },

  {
    ICMP_V6_TIME_EXCEEDED,
    ICMP_V6_TIMEOUT_HOP_LIMIT
  },
  {
    ICMP_V6_TIME_EXCEEDED,
    ICMP_V6_TIMEOUT_REASSEMBLE
  },

  {
    ICMP_V6_PARAMETER_PROBLEM,
    ICMP_V6_ERRONEOUS_HEADER
  },
  {
    ICMP_V6_PARAMETER_PROBLEM,
    ICMP_V6_UNRECOGNIZE_NEXT_HDR
  },
  {
    ICMP_V6_PARAMETER_PROBLEM,
    ICMP_V6_UNRECOGNIZE_OPTION
  },

  {
    ICMP_V6_ECHO_REQUEST,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_ECHO_REPLY,
    ICMP_V6_DEFAULT_CODE
  },

  {
    ICMP_V6_LISTENER_QUERY,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_LISTENER_REPORT,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_LISTENER_REPORT_2,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_LISTENER_DONE,
    ICMP_V6_DEFAULT_CODE
  },

  {
    ICMP_V6_ROUTER_SOLICIT,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_ROUTER_ADVERTISE,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_NEIGHBOR_SOLICIT,
    ICMP_V6_DEFAULT_CODE
  },
  {
    ICMP_V6_NEIGHBOR_ADVERTISE,
    ICMP_V6_DEFAULT_CODE
  },
};

/**
  Reply an ICMPv6 echo request.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the ICMPv6 informational message.
  @param[in]  Packet             The content of the ICMPv6 message with the IP head
                                 removed.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            Successfully answered the ICMPv6 Echo request.
  @retval Others                 Failed to answer the ICMPv6 Echo request.

**/
EFI_STATUS
Ip6IcmpReplyEcho (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD *Icmp;
  NET_BUF                   *Data;
  EFI_STATUS                Status;
  EFI_IP6_HEADER            ReplyHead;

  Status = EFI_OUT_OF_RESOURCES;
  //
  // make a copy the packet, it is really a bad idea to
  // send the MNP's buffer back to MNP.
  //
  Data = NetbufDuplicate (Packet, NULL, IP6_MAX_HEADLEN);
  if (Data == NULL) {
    goto Exit;
  }

  //
  // Change the ICMP type to echo reply, exchange the source
  // and destination, then send it. The source is updated to
  // use specific destination. See RFC1122. SRR/RR option
  // update is omitted.
  //
  Icmp = (IP6_ICMP_INFORMATION_HEAD *) NetbufGetByte (Data, 0, NULL);
  if (Icmp == NULL) {
    NetbufFree (Data);
    goto Exit;
  }

  Icmp->Head.Type     = ICMP_V6_ECHO_REPLY;
  Icmp->Head.Checksum = 0;

  //
  // Generate the IPv6 basic header
  // If the Echo Reply is a response to a Echo Request sent to one of the node's unicast address,
  // the Source address of the Echo Reply must be the same address.
  //
  ZeroMem (&ReplyHead, sizeof (EFI_IP6_HEADER));

  ReplyHead.PayloadLength  = HTONS ((UINT16) (Packet->TotalSize));
  ReplyHead.NextHeader     = IP6_ICMP;
  ReplyHead.HopLimit       = IpSb->CurHopLimit;
  IP6_COPY_ADDRESS (&ReplyHead.DestinationAddress, &Head->SourceAddress);

  if (Ip6IsOneOfSetAddress (IpSb, &Head->DestinationAddress, NULL, NULL)) {
    IP6_COPY_ADDRESS (&ReplyHead.SourceAddress, &Head->DestinationAddress);
  }

  //
  // If source is unspecified, Ip6Output will select a source for us
  //
  Status = Ip6Output (
             IpSb,
             NULL,
             NULL,
             Data,
             &ReplyHead,
             NULL,
             0,
             Ip6SysPacketSent,
             NULL
             );

Exit:
  NetbufFree (Packet);
  return Status;
}

/**
  Process Packet Too Big message sent by a router in response to a packet that
  it cannot forward because the packet is larger than the MTU of outgoing link.
  Since this driver already uses IPv6 minimum link MTU as the maximum packet size,
  if Packet Too Big message is still received, do not reduce the packet size, but
  rather include a Fragment header in the subsequent packets.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the ICMPv6 error packet.
  @param[in]  Packet             The content of the ICMPv6 error with the IP head
                                 removed.

  @retval EFI_SUCCESS            The ICMPv6 error processed successfully.
  @retval EFI_OUT_OF_RESOURCES   Failed to finish the operation due to lack of
                                 resource.
  @retval EFI_NOT_FOUND          The packet too big message is not sent to us.

**/
EFI_STATUS
Ip6ProcessPacketTooBig (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_ERROR_HEAD       Icmp;
  UINT32                    Mtu;
  IP6_ROUTE_ENTRY           *RouteEntry;
  EFI_IPv6_ADDRESS          *DestAddress;

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
  Mtu         = NTOHL (Icmp.Fourth);
  DestAddress = &Icmp.IpHead.DestinationAddress;

  if (Mtu < IP6_MIN_LINK_MTU) {
    //
    // Normally the multicast address is considered to be on-link and not recorded
    // in route table. Here it is added into the table since the MTU information
    // need be recorded.
    //
    if (IP6_IS_MULTICAST (DestAddress)) {
      RouteEntry = Ip6CreateRouteEntry (DestAddress, 128, NULL);
      if (RouteEntry == NULL) {
        NetbufFree (Packet);
        return EFI_OUT_OF_RESOURCES;
      }

      RouteEntry->Flag = IP6_DIRECT_ROUTE | IP6_PACKET_TOO_BIG;
      InsertHeadList (&IpSb->RouteTable->RouteArea[128], &RouteEntry->Link);
      IpSb->RouteTable->TotalNum++;
    } else {
      RouteEntry = Ip6FindRouteEntry (IpSb->RouteTable, DestAddress, NULL);
      if (RouteEntry == NULL) {
        NetbufFree (Packet);
        return EFI_NOT_FOUND;
      }

      RouteEntry->Flag = RouteEntry->Flag | IP6_PACKET_TOO_BIG;

      Ip6FreeRouteEntry (RouteEntry);
    }
  }

  NetbufFree (Packet);
  return EFI_SUCCESS;
}

/**
  Process the ICMPv6 error packet, and deliver the packet to upper layer.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the ICMPv6 error packet.
  @param[in]  Packet             The content of the ICMPv6 error with the IP head
                                 removed.

  @retval EFI_SUCCESS            The ICMPv6 error processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessIcmpError (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_ERROR_HEAD       Icmp;

  //
  // Check the validity of the packet
  //
  if (Packet->TotalSize < sizeof (Icmp)) {
    goto DROP;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
  if (Icmp.Head.Type == ICMP_V6_PACKET_TOO_BIG) {
    return Ip6ProcessPacketTooBig (IpSb, Head, Packet);
  }

  //
  // Notify the upper-layer process that an ICMPv6 eror message is received.
  //
  IP6_GET_CLIP_INFO (Packet)->Status = EFI_ICMP_ERROR;
  return Ip6Demultiplex (IpSb, Head, Packet);

DROP:
  NetbufFree (Packet);
  Packet = NULL;
  return EFI_INVALID_PARAMETER;
}

/**
  Process the ICMPv6 informational messages. If it is an ICMPv6 echo
  request, answer it. If it is a MLD message, trigger MLD routines to
  process it. If it is a ND message, trigger ND routines to process it.
  Otherwise, deliver it to upper layer.

  @param[in]  IpSb               The IP service that receivd the packet.
  @param[in]  Head               The IP head of the ICMPv6 informational packet.
  @param[in]  Packet             The content of the ICMPv6 informational packet
                                 with IP head removed.

  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval EFI_SUCCESS            The ICMPv6 informational message processed.
  @retval Others                 Failed to process ICMPv6 informational message.

**/
EFI_STATUS
Ip6ProcessIcmpInformation (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD Icmp;
  EFI_STATUS                Status;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  NET_CHECK_SIGNATURE (Packet, NET_BUF_SIGNATURE);
  ASSERT (Head != NULL);

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
  Status = EFI_INVALID_PARAMETER;

  switch (Icmp.Head.Type) {
  case ICMP_V6_ECHO_REQUEST:
    //
    // If ICMPv6 echo, reply it
    //
    if (Icmp.Head.Code == 0) {
      Status = Ip6IcmpReplyEcho (IpSb, Head, Packet);
    }
    break;
  case ICMP_V6_LISTENER_QUERY:
    Status = Ip6ProcessMldQuery (IpSb, Head, Packet);
    break;
  case ICMP_V6_LISTENER_REPORT:
  case ICMP_V6_LISTENER_REPORT_2:
    Status = Ip6ProcessMldReport (IpSb, Head, Packet);
    break;
  case ICMP_V6_NEIGHBOR_SOLICIT:
    Status = Ip6ProcessNeighborSolicit (IpSb, Head, Packet);
    break;
  case ICMP_V6_NEIGHBOR_ADVERTISE:
    Status = Ip6ProcessNeighborAdvertise (IpSb, Head, Packet);
    break;
  case ICMP_V6_ROUTER_ADVERTISE:
    Status = Ip6ProcessRouterAdvertise (IpSb, Head, Packet);
    break;
  case ICMP_V6_REDIRECT:
    Status = Ip6ProcessRedirect (IpSb, Head, Packet);
    break;
  case ICMP_V6_ECHO_REPLY:
    Status = Ip6Demultiplex (IpSb, Head, Packet);
    break;
  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

/**
  Handle the ICMPv6 packet. First validate the message format,
  then, according to the message types, process it as an informational packet or
  an error packet.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the ICMPv6 packet.
  @param[in]  Packet             The content of the ICMPv6 packet with IP head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is malformated.
  @retval EFI_SUCCESS            The ICMPv6 message successfully processed.
  @retval Others                 Failed to handle the ICMPv6 packet.

**/
EFI_STATUS
Ip6IcmpHandle (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_HEAD             Icmp;
  UINT16                    PseudoCheckSum;
  UINT16                    CheckSum;

  //
  // Check the validity of the incoming packet.
  //
  if (Packet->TotalSize < sizeof (Icmp)) {
    goto DROP;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);

  //
  // Make sure checksum is valid.
  //
  PseudoCheckSum = NetIp6PseudoHeadChecksum (
                     &Head->SourceAddress,
                     &Head->DestinationAddress,
                     IP6_ICMP,
                     Packet->TotalSize
                     );
  CheckSum = (UINT16) ~NetAddChecksum (PseudoCheckSum, NetbufChecksum (Packet));
  if (CheckSum != 0) {
    goto DROP;
  }

  //
  // According to the packet type, call corresponding process
  //
  if (Icmp.Type <= ICMP_V6_ERROR_MAX) {
    return Ip6ProcessIcmpError (IpSb, Head, Packet);
  } else {
    return Ip6ProcessIcmpInformation (IpSb, Head, Packet);
  }

DROP:
  NetbufFree (Packet);
  return EFI_INVALID_PARAMETER;
}

/**
  Retrieve the Prefix address according to the PrefixLength by clear the useless
  bits.

  @param[in]       PrefixLength  The prefix length of the prefix.
  @param[in, out]  Prefix        On input, points to the original prefix address
                                 with dirty bits; on output, points to the updated
                                 address with useless bit clear.

**/
VOID
Ip6GetPrefix (
  IN     UINT8              PrefixLength,
  IN OUT EFI_IPv6_ADDRESS   *Prefix
  )
{
  UINT8                     Byte;
  UINT8                     Bit;
  UINT8                     Mask;
  UINT8                     Value;

  ASSERT ((Prefix != NULL) && (PrefixLength < IP6_PREFIX_NUM));

  if (PrefixLength == 0) {
    ZeroMem (Prefix, sizeof (EFI_IPv6_ADDRESS));
    return ;
  }

  if (PrefixLength == IP6_PREFIX_NUM - 1) {
    return ;
  }

  Byte  = (UINT8) (PrefixLength / 8);
  Bit   = (UINT8) (PrefixLength % 8);
  Value = Prefix->Addr[Byte];

  if ((Byte > 0) && (Byte < 16)) {
    ZeroMem (Prefix->Addr + Byte, 16 - Byte);
  }

  if (Bit > 0) {
    Mask = (UINT8) (0xFF << (8 - Bit));
    Prefix->Addr[Byte] = (UINT8) (Value & Mask);
  }

}

/**
  Check whether the DestinationAddress is an anycast address.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  DestinationAddress Points to the Destination Address of the packet.

  @retval TRUE                   The DestinationAddress is anycast address.
  @retval FALSE                  The DestinationAddress is not anycast address.

**/
BOOLEAN
Ip6IsAnycast (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *DestinationAddress
  )
{
  IP6_PREFIX_LIST_ENTRY     *PrefixEntry;
  EFI_IPv6_ADDRESS          Prefix;
  BOOLEAN                   Flag;

  ZeroMem (&Prefix, sizeof (EFI_IPv6_ADDRESS));

  Flag = FALSE;

  //
  // If the address is known as on-link or autonomous prefix, record it as
  // anycast address.
  //
  do {
    PrefixEntry = Ip6FindPrefixListEntry (IpSb, Flag, 255, DestinationAddress);
    if (PrefixEntry != NULL) {
      IP6_COPY_ADDRESS (&Prefix, &PrefixEntry->Prefix);
      Ip6GetPrefix (PrefixEntry->PrefixLength, &Prefix);
      if (EFI_IP6_EQUAL (&Prefix, DestinationAddress)) {
        return TRUE;
      }
    }

    Flag = (BOOLEAN) !Flag;
  } while (Flag);

  return FALSE;
}

/**
  Generate ICMPv6 error message and send it out to DestinationAddress. Currently
  Destination Unreachable message, Time Exceeded message and Parameter Problem
  message are supported.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Packet             The packet which invoking ICMPv6 error.
  @param[in]  SourceAddress      If not NULL, points to the SourceAddress.
                                 Otherwise, the IP layer will select a source address
                                 according to the DestinationAddress.
  @param[in]  DestinationAddress Points to the Destination Address of the ICMPv6
                                 error message.
  @param[in]  Type               The type of the ICMPv6 message.
  @param[in]  Code               The additional level of the ICMPv6 message.
  @param[in]  Pointer            If not NULL, identifies the octet offset within
                                 the invoking packet where the error was detected.

  @retval EFI_INVALID_PARAMETER  The packet is malformated.
  @retval EFI_OUT_OF_RESOURCES   There is no sufficient resource to complete the
                                 operation.
  @retval EFI_SUCCESS            The ICMPv6 message was successfully sent out.
  @retval Others                 Failed to generate the ICMPv6 packet.

**/
EFI_STATUS
Ip6SendIcmpError (
  IN IP6_SERVICE            *IpSb,
  IN NET_BUF                *Packet,
  IN EFI_IPv6_ADDRESS       *SourceAddress       OPTIONAL,
  IN EFI_IPv6_ADDRESS       *DestinationAddress,
  IN UINT8                  Type,
  IN UINT8                  Code,
  IN UINT32                 *Pointer             OPTIONAL
  )
{
  UINT32                    PacketLen;
  NET_BUF                   *ErrorMsg;
  UINT16                    PayloadLen;
  EFI_IP6_HEADER            Head;
  IP6_ICMP_INFORMATION_HEAD *IcmpHead;
  UINT8                     *ErrorBody;

  if (DestinationAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // An ICMPv6 error message must not be originated as a result of receiving
  // a packet whose source address does not uniquely identify a single node --
  // e.g., the IPv6 Unspecified Address, an IPv6 multicast address, or an address
  // known by the ICMP message originator to be an IPv6 anycast address.
  //
  if (NetIp6IsUnspecifiedAddr (DestinationAddress) ||
      IP6_IS_MULTICAST (DestinationAddress)        ||
      Ip6IsAnycast (IpSb, DestinationAddress)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Type) {
  case ICMP_V6_DEST_UNREACHABLE:
  case ICMP_V6_TIME_EXCEEDED:
    break;

  case ICMP_V6_PARAMETER_PROBLEM:
    if (Pointer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  PacketLen = sizeof (IP6_ICMP_ERROR_HEAD) + Packet->TotalSize;

  if (PacketLen > IpSb->MaxPacketSize) {
    PacketLen = IpSb->MaxPacketSize;
  }

  ErrorMsg = NetbufAlloc (PacketLen);
  if (ErrorMsg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PayloadLen = (UINT16) (PacketLen - sizeof (EFI_IP6_HEADER));

  //
  // Create the basic IPv6 header.
  //
  ZeroMem (&Head, sizeof (EFI_IP6_HEADER));

  Head.PayloadLength  = HTONS (PayloadLen);
  Head.NextHeader     = IP6_ICMP;
  Head.HopLimit       = IpSb->CurHopLimit;

  if (SourceAddress != NULL) {
    IP6_COPY_ADDRESS (&Head.SourceAddress, SourceAddress);
  } else {
    ZeroMem (&Head.SourceAddress, sizeof (EFI_IPv6_ADDRESS));
  }

  IP6_COPY_ADDRESS (&Head.DestinationAddress, DestinationAddress);

  NetbufReserve (ErrorMsg, sizeof (EFI_IP6_HEADER));

  //
  // Fill in the ICMP error message head
  //
  IcmpHead = (IP6_ICMP_INFORMATION_HEAD *) NetbufAllocSpace (ErrorMsg, sizeof (IP6_ICMP_INFORMATION_HEAD), FALSE);
  if (IcmpHead == NULL) {
    NetbufFree (ErrorMsg);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (IcmpHead, sizeof (IP6_ICMP_INFORMATION_HEAD));
  IcmpHead->Head.Type = Type;
  IcmpHead->Head.Code = Code;

  if (Pointer != NULL) {
    IcmpHead->Fourth = HTONL (*Pointer);
  }

  //
  // Fill in the ICMP error message body
  //
  PayloadLen -= sizeof (IP6_ICMP_INFORMATION_HEAD);
  ErrorBody =  NetbufAllocSpace (ErrorMsg, PayloadLen, FALSE);
  if (ErrorBody != NULL) {
    ZeroMem (ErrorBody, PayloadLen);
    NetbufCopy (Packet, 0, PayloadLen, ErrorBody);
  }

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, NULL, NULL, ErrorMsg, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

