/** @file
  The internal functions and routines to transmit the IP6 packet.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

UINT32 mIp6Id;

/**
  Output all the available source addresses to a list entry head SourceList. The
  number of source addresses are also returned.

  @param[in]       IpSb             Points to an IP6 service binding instance.
  @param[out]      SourceList       The list entry head of all source addresses.
                                    It is the caller's responsibility to free the
                                    resources.
  @param[out]      SourceCount      The number of source addresses.

  @retval EFI_SUCCESS           The source addresses were copied to a list entry head
                                SourceList.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.

**/
EFI_STATUS
Ip6CandidateSource (
  IN IP6_SERVICE            *IpSb,
  OUT LIST_ENTRY            *SourceList,
  OUT UINT32                *SourceCount
  )
{
  IP6_INTERFACE             *IpIf;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Entry2;
  IP6_ADDRESS_INFO          *AddrInfo;
  IP6_ADDRESS_INFO          *Copy;

  *SourceCount = 0;

  if (IpSb->LinkLocalOk) {
    Copy = AllocatePool (sizeof (IP6_ADDRESS_INFO));
    if (Copy == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Copy->Signature         = IP6_ADDR_INFO_SIGNATURE;
    IP6_COPY_ADDRESS (&Copy->Address, &IpSb->LinkLocalAddr);
    Copy->IsAnycast         = FALSE;
    Copy->PrefixLength      = IP6_LINK_LOCAL_PREFIX_LENGTH;
    Copy->ValidLifetime     = (UINT32) IP6_INFINIT_LIFETIME;
    Copy->PreferredLifetime = (UINT32) IP6_INFINIT_LIFETIME;

    InsertTailList (SourceList, &Copy->Link);
    (*SourceCount)++;
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);

    NET_LIST_FOR_EACH (Entry2, &IpIf->AddressList) {
      AddrInfo = NET_LIST_USER_STRUCT_S (Entry2, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

      if (AddrInfo->IsAnycast) {
        //
        // Never use an anycast address.
        //
        continue;
      }

      Copy = AllocateCopyPool (sizeof (IP6_ADDRESS_INFO), AddrInfo);
      if (Copy == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      InsertTailList (SourceList, &Copy->Link);
      (*SourceCount)++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Calculate how many bits are the same between two IPv6 addresses.

  @param[in]       AddressA         Points to an IPv6 address.
  @param[in]       AddressB         Points to another IPv6 address.

  @return The common bits of the AddressA and AddressB.

**/
UINT8
Ip6CommonPrefixLen (
  IN EFI_IPv6_ADDRESS       *AddressA,
  IN EFI_IPv6_ADDRESS       *AddressB
  )
{
  UINT8                     Count;
  UINT8                     Index;
  UINT8                     ByteA;
  UINT8                     ByteB;
  UINT8                     NumBits;

  Count = 0;
  Index = 0;

  while (Index < 16) {
    ByteA = AddressA->Addr[Index];
    ByteB = AddressB->Addr[Index];

    if (ByteA == ByteB) {
      Count += 8;
      Index++;
      continue;
    }

    //
    // Check how many bits are common between the two bytes.
    //
    NumBits = 8;
    ByteA   = (UINT8) (ByteA ^ ByteB);

    while (ByteA != 0) {
      NumBits--;
      ByteA = (UINT8) (ByteA >> 1);
    }

    return (UINT8) (Count + NumBits);
  }

  return Count;
}

/**
  Output all the available source addresses to a list entry head SourceList. The
  number of source addresses are also returned.

  @param[in]       IpSb             Points to a IP6 service binding instance.
  @param[in]       Destination      The IPv6 destination address.
  @param[out]      Source           The selected IPv6 source address according to
                                    the Destination.

  @retval EFI_SUCCESS           The source addresses were copied to a list entry
                                head SourceList.
  @retval EFI_NO_MAPPING        The IPv6 stack is not auto configured.

**/
EFI_STATUS
Ip6SelectSourceAddress (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Destination,
  OUT EFI_IPv6_ADDRESS      *Source
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                SourceList;
  UINT32                    SourceCount;
  UINT8                     ScopeD;
  LIST_ENTRY                *Entry;
  IP6_ADDRESS_INFO          *AddrInfo;
  IP6_PREFIX_LIST_ENTRY     *Prefix;
  UINT8                     LastCommonLength;
  UINT8                     CurrentCommonLength;
  EFI_IPv6_ADDRESS          *TmpAddress;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  Status = EFI_SUCCESS;
  InitializeListHead (&SourceList);

  if (!IpSb->LinkLocalOk) {
    return EFI_NO_MAPPING;
  }

  //
  // Rule 1: Prefer same address.
  //
  if (Ip6IsOneOfSetAddress (IpSb, Destination, NULL, NULL)) {
    IP6_COPY_ADDRESS (Source, Destination);
    goto Exit;
  }

  //
  // Rule 2: Prefer appropriate scope.
  //
  if (IP6_IS_MULTICAST (Destination)) {
    ScopeD = (UINT8) (Destination->Addr[1] >> 4);
  } else if (NetIp6IsLinkLocalAddr (Destination)) {
    ScopeD = 0x2;
  } else {
    ScopeD = 0xE;
  }

  if (ScopeD <= 0x2) {
    //
    // Return the link-local address if it exists
    // One IP6_SERVICE only has one link-local address.
    //
    IP6_COPY_ADDRESS (Source, &IpSb->LinkLocalAddr);
    goto Exit;
  }

  //
  // All candidate source addresses are global unicast address.
  //
  Ip6CandidateSource (IpSb, &SourceList, &SourceCount);

  if (SourceCount == 0) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  IP6_COPY_ADDRESS (Source, &IpSb->LinkLocalAddr);

  if (SourceCount == 1) {
    goto Exit;
  }

  //
  // Rule 3: Avoid deprecated addresses.
  // TODO: check the "deprecated" state of the stateful configured address
  //
  NET_LIST_FOR_EACH (Entry, &IpSb->AutonomousPrefix) {
    Prefix = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);
    if (Prefix->PreferredLifetime == 0) {
      Ip6RemoveAddr (NULL, &SourceList, &SourceCount, &Prefix->Prefix, Prefix->PrefixLength);

      if (SourceCount == 1) {
        goto Exit;
      }
    }
  }

  //
  // TODO: Rule 4: Prefer home addresses.
  // TODO: Rule 5: Prefer outgoing interface.
  // TODO: Rule 6: Prefer matching label.
  // TODO: Rule 7: Prefer public addresses.
  //

  //
  // Rule 8: Use longest matching prefix.
  //
  LastCommonLength = Ip6CommonPrefixLen (Source, Destination);
  TmpAddress       = NULL;

  for (Entry = SourceList.ForwardLink; Entry != &SourceList; Entry = Entry->ForwardLink) {
    AddrInfo = NET_LIST_USER_STRUCT_S (Entry, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

    CurrentCommonLength = Ip6CommonPrefixLen (&AddrInfo->Address, Destination);
    if (CurrentCommonLength > LastCommonLength) {
      LastCommonLength = CurrentCommonLength;
      TmpAddress       = &AddrInfo->Address;
    }
  }

  if (TmpAddress != NULL) {
    IP6_COPY_ADDRESS (Source, TmpAddress);
  }

Exit:

  Ip6RemoveAddr (NULL, &SourceList, &SourceCount, NULL, 0);

  return Status;
}

/**
  Select an interface to send the packet generated in the IP6 driver
  itself: that is, not by the requests of the IP6 child's consumer. Such
  packets include the ICMPv6 echo replies and other ICMPv6 error packets.

  @param[in]  IpSb                 The IP4 service that wants to send the packets.
  @param[in]  Destination          The destination of the packet.
  @param[in, out]  Source          The source of the packet.

  @return NULL if no proper interface is found, otherwise, the interface that
          can be used to send the system packet from.

**/
IP6_INTERFACE *
Ip6SelectInterface (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Destination,
  IN OUT EFI_IPv6_ADDRESS   *Source
  )
{
  EFI_STATUS                Status;
  EFI_IPv6_ADDRESS          SelectedSource;
  IP6_INTERFACE             *IpIf;
  BOOLEAN                   Exist;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Destination != NULL && Source != NULL);

  if (NetIp6IsUnspecifiedAddr (Destination)) {
    return NULL;
  }

  if (!NetIp6IsUnspecifiedAddr (Source)) {
    Exist = Ip6IsOneOfSetAddress (IpSb, Source, &IpIf, NULL);
    ASSERT (Exist);

    return IpIf;
  }

  //
  // If source is unspecified, select a source according to the destination.
  //
  Status = Ip6SelectSourceAddress (IpSb, Destination, &SelectedSource);
  if (EFI_ERROR (Status)) {
    return IpSb->DefaultInterface;
  }

  Ip6IsOneOfSetAddress (IpSb, &SelectedSource, &IpIf, NULL);
  IP6_COPY_ADDRESS (Source, &SelectedSource);

  return IpIf;
}

/**
  The default callback function for the system generated packet.
  It will free the packet.

  @param[in]  Packet        The packet that transmitted.
  @param[in]  IoStatus      The result of the transmission, succeeded or failed.
  @param[in]  LinkFlag      Not used when transmitted. Check IP6_FRAME_CALLBACK
                            for reference.
  @param[in]  Context       The context provided by us.

**/
VOID
Ip6SysPacketSent (
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    LinkFlag,
  VOID                      *Context
  )
{
  NetbufFree (Packet);
  Packet = NULL;
}

/**
  Prefix an IP6 basic head and unfragmentable extension headers and a fragment header
  to the Packet. Used for IP6 fragmentation.

  @param[in]  IpSb             The IP6 service instance to transmit the packet.
  @param[in]  Packet           The packet to prefix the IP6 header to.
  @param[in]  Head             The caller supplied header.
  @param[in]  FragmentOffset   The fragment offset of the data following the header.
  @param[in]  ExtHdrs          The length of the original extension header.
  @param[in]  ExtHdrsLen       The length of the extension headers.
  @param[in]  LastHeader       The pointer of next header of last extension header.
  @param[in]  HeadLen          The length of the unfragmented part of the IP6 header.

  @retval EFI_BAD_BUFFER_SIZE  There is no enough room in the head space of
                               Packet.
  @retval EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
Ip6PrependHead (
  IN IP6_SERVICE            *IpSb,
  IN NET_BUF                *Packet,
  IN EFI_IP6_HEADER         *Head,
  IN UINT16                 FragmentOffset,
  IN UINT8                  *ExtHdrs,
  IN UINT32                 ExtHdrsLen,
  IN UINT8                  LastHeader,
  IN UINT32                 HeadLen
  )
{
  UINT32                    Len;
  UINT32                    UnFragExtHdrsLen;
  EFI_IP6_HEADER            *PacketHead;
  UINT8                     *UpdatedExtHdrs;
  EFI_STATUS                Status;
  UINT8                     NextHeader;

  UpdatedExtHdrs = NULL;

  //
  // HeadLen is the length of the fixed part of the sequences of fragments, i.e.
  // the unfragment part.
  //
  PacketHead = (EFI_IP6_HEADER *) NetbufAllocSpace (Packet, HeadLen, NET_BUF_HEAD);
  if (PacketHead == NULL) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Set the head up, convert the host byte order to network byte order
  //
  CopyMem (PacketHead, Head, sizeof (EFI_IP6_HEADER));
  PacketHead->PayloadLength = HTONS ((UINT16) (Packet->TotalSize - sizeof (EFI_IP6_HEADER)));
  Packet->Ip.Ip6            = PacketHead;

  Len              = HeadLen - sizeof (EFI_IP6_HEADER);
  UnFragExtHdrsLen = Len - sizeof (IP6_FRAGMENT_HEADER);

  if (UnFragExtHdrsLen == 0) {
    PacketHead->NextHeader = IP6_FRAGMENT;
  }

  //
  // Append the extension headers: firstly copy the unfragmentable headers, then append
  // fragmentation header.
  //
  if ((FragmentOffset & IP6_FRAGMENT_OFFSET_MASK) == 0) {
    NextHeader = Head->NextHeader;
  } else {
    NextHeader = PacketHead->NextHeader;
  }

  Status = Ip6FillFragmentHeader (
             IpSb,
             NextHeader,
             LastHeader,
             ExtHdrs,
             ExtHdrsLen,
             FragmentOffset,
             &UpdatedExtHdrs
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (
    (UINT8 *) (PacketHead + 1),
    UpdatedExtHdrs,
    UnFragExtHdrsLen + sizeof (IP6_FRAGMENT_HEADER)
    );

  FreePool (UpdatedExtHdrs);
  return EFI_SUCCESS;
}

/**
  Transmit an IP6 packet. The packet comes either from the IP6
  child's consumer (IpInstance != NULL) or the IP6 driver itself
  (IpInstance == NULL). It will route the packet, fragment it,
  then transmit all the fragments through an interface.

  @param[in]  IpSb             The IP6 service instance to transmit the packet.
  @param[in]  Interface        The IP6 interface to transmit the packet. Ignored
                               if NULL.
  @param[in]  IpInstance       The IP6 child that issues the transmission.  It is
                               NULL if the packet is from the system.
  @param[in]  Packet           The user data to send, excluding the IP header.
  @param[in]  Head             The caller supplied header. The caller should set
                               the  following header fields: NextHeader, HopLimit,
                               Src, Dest, FlowLabel, PayloadLength. This function
                               will fill in the Ver, TrafficClass.
  @param[in]  ExtHdrs          The extension headers to append to the IPv6 basic
                               header.
  @param[in]  ExtHdrsLen       The length of the extension headers.
  @param[in]  Callback         The callback function to issue when transmission
                               completed.
  @param[in]  Context          The opaque context for the callback.

  @retval EFI_INVALID_PARAMETER Any input parameter or the packet is invalid.
  @retval EFI_NO_MAPPING        There is no interface to the destination.
  @retval EFI_NOT_FOUND         There is no route to the destination.
  @retval EFI_SUCCESS           The packet successfully transmitted.
  @retval EFI_OUT_OF_RESOURCES  Failed to finish the operation due to lack of
                                resources.
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Ip6Output (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface   OPTIONAL,
  IN IP6_PROTOCOL           *IpInstance  OPTIONAL,
  IN NET_BUF                *Packet,
  IN EFI_IP6_HEADER         *Head,
  IN UINT8                  *ExtHdrs,
  IN UINT32                 ExtHdrsLen,
  IN IP6_FRAME_CALLBACK     Callback,
  IN VOID                   *Context
  )
{
  IP6_INTERFACE             *IpIf;
  EFI_IPv6_ADDRESS          NextHop;
  IP6_NEIGHBOR_ENTRY        *NeighborCache;
  IP6_ROUTE_CACHE_ENTRY     *RouteCache;
  EFI_STATUS                Status;
  UINT32                    Mtu;
  UINT32                    HeadLen;
  UINT16                    FragmentOffset;
  UINT8                     *LastHeader;
  UINT32                    UnFragmentLen;
  UINT32                    UnFragmentHdrsLen;
  UINT32                    FragmentHdrsLen;
  UINT16                    *Checksum;
  UINT16                    PacketChecksum;
  UINT16                    PseudoChecksum;
  UINT32                    Index;
  UINT32                    PacketLen;
  UINT32                    RealExtLen;
  UINT32                    Offset;
  NET_BUF                   *TmpPacket;
  NET_BUF                   *Fragment;
  UINT32                    Num;
  UINT8                     *Buf;
  EFI_IP6_HEADER            *PacketHead;
  IP6_ICMP_HEAD             *IcmpHead;
  IP6_TXTOKEN_WRAP          *Wrap;
  IP6_ROUTE_ENTRY           *RouteEntry;
  UINT8                     *UpdatedExtHdrs;
  UINT8                     NextHeader;
  UINT8                     LastHeaderBackup;
  BOOLEAN                   FragmentHeadInserted;
  UINT8                     *ExtHdrsBackup;
  UINT8                     NextHeaderBackup;
  EFI_IPv6_ADDRESS          Source;
  EFI_IPv6_ADDRESS          Destination;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  //
  // RFC2460: Each extension header is an integer multiple of 8 octets long,
  // in order to retain 8-octet alignment for subsequent headers.
  //
  if ((ExtHdrsLen & 0x7) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  LastHeader = NULL;

  Ip6IsExtsValid (
    NULL,
    NULL,
    &Head->NextHeader,
    ExtHdrs,
    ExtHdrsLen,
    FALSE,
    NULL,
    &LastHeader,
    NULL,
    NULL,
    NULL
    );

  //
  // Select an interface/source for system packet, application
  // should select them itself.
  //
  IpIf = Interface;
  if (IpIf == NULL) {
    //
    // IpInstance->Interface is NULL when IpInstance is configured with both stationaddress
    // and destinationaddress is unspecified.
    //
    if (IpInstance == NULL || IpInstance->Interface == NULL) {
      IpIf = Ip6SelectInterface (IpSb, &Head->DestinationAddress, &Head->SourceAddress);
      if (IpInstance != NULL) {
        IpInstance->Interface = IpIf;
      }
    } else {
      IpIf = IpInstance->Interface;
    }
  }

  if (IpIf == NULL) {
    return EFI_NO_MAPPING;
  }

  //
  // Update the common field in Head here.
  //
  Head->Version       = 6;
  Head->TrafficClassL = 0;
  Head->TrafficClassH = 0;

  Checksum            = NULL;
  NextHeader          = *LastHeader;

  switch (NextHeader) {
  case EFI_IP_PROTO_UDP:
    Packet->Udp = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, NULL);
    ASSERT (Packet->Udp != NULL);
    if (Packet->Udp->Checksum == 0) {
      Checksum = &Packet->Udp->Checksum;
    }
    break;

  case EFI_IP_PROTO_TCP:
    Packet->Tcp = (TCP_HEAD *) NetbufGetByte (Packet, 0, NULL);
    ASSERT (Packet->Tcp != NULL);
    if (Packet->Tcp->Checksum == 0) {
      Checksum = &Packet->Tcp->Checksum;
    }
    break;

  case IP6_ICMP:
    //
    // Don't send ICMP packet to an IPv6 anycast address.
    //
    if (Ip6IsAnycast (IpSb, &Head->DestinationAddress)) {
      return EFI_INVALID_PARAMETER;
    }

    IcmpHead = (IP6_ICMP_HEAD *) NetbufGetByte (Packet, 0, NULL);
    ASSERT (IcmpHead != NULL);
    if (IcmpHead->Checksum == 0) {
      Checksum = &IcmpHead->Checksum;
    }
    break;

  default:
    break;
  }

  if (Checksum != NULL) {
    //
    // Calculate the checksum for upper layer protocol if it is not calculated due to lack of
    // IPv6 source address.
    //
    PacketChecksum = NetbufChecksum (Packet);
    PseudoChecksum = NetIp6PseudoHeadChecksum (
                      &Head->SourceAddress,
                      &Head->DestinationAddress,
                      NextHeader,
                      Packet->TotalSize
                      );
    *Checksum = (UINT16) ~NetAddChecksum (PacketChecksum, PseudoChecksum);
  }

  Status = Ip6IpSecProcessPacket (
             IpSb,
             &Head,
             LastHeader, // no need get the lasthead value for output
             &Packet,
             &ExtHdrs,
             &ExtHdrsLen,
             EfiIPsecOutBound,
             Context
             );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  LastHeader = NULL;
  //
  // Check incoming parameters.
  //
  if (!Ip6IsExtsValid (
         IpSb,
         Packet,
         &Head->NextHeader,
         ExtHdrs,
         ExtHdrsLen,
         FALSE,
         NULL,
         &LastHeader,
         &RealExtLen,
         &UnFragmentHdrsLen,
         NULL
         )) {
    return EFI_INVALID_PARAMETER;
  }

  if ((RealExtLen & 0x7) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  LastHeaderBackup = *LastHeader;

  //
  // Perform next hop determination:
  // For multicast packets, the next-hop is always the destination address and
  // is considered to be on-link.
  //
  if (IP6_IS_MULTICAST (&Head->DestinationAddress)) {
    IP6_COPY_ADDRESS (&NextHop, &Head->DestinationAddress);
  } else {
    //
    // For unicast packets, use a combination of the Destination Cache, the Prefix List
    // and the Default Router List to determine the IP address of the appropriate next hop.
    //

    NeighborCache = Ip6FindNeighborEntry (IpSb, &Head->DestinationAddress);
    if (NeighborCache != NULL) {
      //
      // Hit Neighbor Cache.
      //
      IP6_COPY_ADDRESS (&NextHop, &Head->DestinationAddress);
    } else {
      //
      // Not in Neighbor Cache, check Router cache
      //
      RouteCache = Ip6Route (IpSb, &Head->DestinationAddress, &Head->SourceAddress);
      if (RouteCache == NULL) {
        return EFI_NOT_FOUND;
      }

      IP6_COPY_ADDRESS (&NextHop, &RouteCache->NextHop);
      Ip6FreeRouteCacheEntry (RouteCache);
    }
  }

  //
  // Examines the Neighbor Cache for link-layer information about that neighbor.
  // DO NOT create neighbor cache if neighbor is itself - when reporting ICMP error.
  //
  if (!IP6_IS_MULTICAST (&NextHop) && !EFI_IP6_EQUAL (&Head->DestinationAddress, &Head->SourceAddress)) {
    NeighborCache = Ip6FindNeighborEntry (IpSb, &NextHop);
    if (NeighborCache == NULL) {
      NeighborCache = Ip6CreateNeighborEntry (IpSb, Ip6OnArpResolved, &NextHop, NULL);

      if (NeighborCache == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Send out multicast neighbor solicitation for address resolution immediately.
      //
      Ip6CreateSNMulticastAddr (&NeighborCache->Neighbor, &Destination);
      Status = Ip6SelectSourceAddress (IpSb, &NeighborCache->Neighbor, &Source);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = Ip6SendNeighborSolicit (
                 IpSb,
                 &Source,
                 &Destination,
                 &NeighborCache->Neighbor,
                 &IpSb->SnpMode.CurrentAddress
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      --NeighborCache->Transmit;
      NeighborCache->Ticks = IP6_GET_TICKS (IpSb->RetransTimer) + 1;
    }

    NeighborCache->Interface = IpIf;
  }

  UpdatedExtHdrs       = NULL;
  ExtHdrsBackup        = NULL;
  NextHeaderBackup     = 0;
  FragmentHeadInserted = FALSE;

  //
  // Check whether we received Packet Too Big message for the packet sent to the
  // Destination. If yes include a Fragment Header in the subsequent packets.
  //
  RouteEntry = Ip6FindRouteEntry (
                 IpSb->RouteTable,
                 &Head->DestinationAddress,
                 NULL
                 );
  if (RouteEntry != NULL) {
    if ((RouteEntry->Flag & IP6_PACKET_TOO_BIG) == IP6_PACKET_TOO_BIG) {

      //
      // FragmentHead is inserted after Hop-by-Hop Options header, Destination
      // Options header (first occur), Routing header, and before Fragment header,
      // Authentication header, Encapsulating Security Payload header, and
      // Destination Options header (last occur), and upper-layer header.
      //
      Status = Ip6FillFragmentHeader (
                 IpSb,
                 Head->NextHeader,
                 LastHeaderBackup,
                 ExtHdrs,
                 ExtHdrsLen,
                 0,
                 &UpdatedExtHdrs
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((ExtHdrs == NULL) && (ExtHdrsLen == 0)) {
        NextHeaderBackup = Head->NextHeader;
        Head->NextHeader = IP6_FRAGMENT;
      }

      ExtHdrsBackup    = ExtHdrs;
      ExtHdrs          = UpdatedExtHdrs;
      ExtHdrsLen       = ExtHdrsLen + sizeof (IP6_FRAGMENT_HEADER);
      RealExtLen       = RealExtLen + sizeof (IP6_FRAGMENT_HEADER);

      mIp6Id++;

      FragmentHeadInserted = TRUE;
    }

    Ip6FreeRouteEntry (RouteEntry);
  }

  //
  // OK, selected the source and route, fragment the packet then send
  // them. Tag each fragment other than the first one as spawn from it.
  // Each extension header is an integer multiple of 8 octets long, in
  // order to retain 8-octet alignment for subsequent headers.
  //
  Mtu     = IpSb->MaxPacketSize + sizeof (EFI_IP6_HEADER);
  HeadLen = sizeof (EFI_IP6_HEADER) + RealExtLen;

  if (Packet->TotalSize + HeadLen > Mtu) {
    //
    // Remove the inserted Fragment Header since we need fragment the packet.
    //
    if (FragmentHeadInserted) {
      ExtHdrs    = ExtHdrsBackup;
      ExtHdrsLen = ExtHdrsLen - sizeof (IP6_FRAGMENT_HEADER);

      if ((ExtHdrs == NULL) && (ExtHdrsLen == 0)) {
        Head->NextHeader = NextHeaderBackup;
      }
    }

    FragmentHdrsLen = ExtHdrsLen - UnFragmentHdrsLen;

    //
    // The packet is beyond the maximum which can be described through the
    // fragment offset field in Fragment header.
    //
    if ((((Packet->TotalSize + FragmentHdrsLen) >> 3) & (~0x1fff)) != 0) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Error;
    }

    if (FragmentHdrsLen != 0) {
      //
      // Append the fragmentable extension hdrs before the upper layer payload
      // to form a new NET_BUF. This NET_BUF contains all the buffer which will
      // be fragmented below.
      //
      TmpPacket = NetbufGetFragment (Packet, 0, Packet->TotalSize, FragmentHdrsLen);
      ASSERT (TmpPacket != NULL);

      //
      // Allocate the space to contain the fragmentable hdrs and copy the data.
      //
      Buf = NetbufAllocSpace (TmpPacket, FragmentHdrsLen, TRUE);
      ASSERT (Buf != NULL);
      CopyMem (Buf, ExtHdrs + UnFragmentHdrsLen, FragmentHdrsLen);

      //
      // Free the old Packet.
      //
      NetbufFree (Packet);
      Packet = TmpPacket;
    }

    //
    // The unfragment part which appears in every fragmented IPv6 packet includes
    // the IPv6 header, the unfragmentable extension hdrs and the fragment header.
    //
    UnFragmentLen = sizeof (EFI_IP6_HEADER) + UnFragmentHdrsLen + sizeof (IP6_FRAGMENT_HEADER);

    //
    // Mtu now is the length of the fragment part in a full-length fragment.
    //
    Mtu = (Mtu - UnFragmentLen) & (~0x07);
    Num = (Packet->TotalSize + Mtu - 1) / Mtu;

    for (Index = 0, Offset = 0, PacketLen = Mtu; Index < Num; Index++) {
      //
      // Get fragment from the Packet, append UnFragmentLen spare buffer
      // before the fragmented data, the corresponding data is filled in later.
      //
      Fragment = NetbufGetFragment (Packet, Offset, PacketLen, UnFragmentLen);
      if (Fragment == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }

      FragmentOffset = (UINT16) ((UINT16) Offset | 0x1);
      if (Index == Num - 1){
        //
        // The last fragment, clear the M flag.
        //
        FragmentOffset &= (~0x1);
      }

      Status = Ip6PrependHead (
                 IpSb,
                 Fragment,
                 Head,
                 FragmentOffset,
                 ExtHdrs,
                 ExtHdrsLen,
                 LastHeaderBackup,
                 UnFragmentLen
                 );
      ASSERT (Status == EFI_SUCCESS);

      Status = Ip6SendFrame (
                 IpIf,
                 IpInstance,
                 Fragment,
                 &NextHop,
                 Ip6SysPacketSent,
                 Packet
                 );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      //
      // The last fragment of upper layer packet, update the IP6 token status.
      //
      if ((Index == Num -1) && (Context != NULL)) {
        Wrap                = (IP6_TXTOKEN_WRAP *) Context;
        Wrap->Token->Status = Status;
      }

      Offset    += PacketLen;
      PacketLen = Packet->TotalSize - Offset;
      if (PacketLen > Mtu) {
        PacketLen = Mtu;
      }
    }

    NetbufFree (Packet);
    mIp6Id++;

    if (UpdatedExtHdrs != NULL) {
      FreePool (UpdatedExtHdrs);
    }

    return EFI_SUCCESS;
  }

  //
  // Need not fragment the packet, send it in one frame.
  //
  PacketHead = (EFI_IP6_HEADER *) NetbufAllocSpace (Packet, HeadLen, NET_BUF_HEAD);
  if (PacketHead == NULL) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Error;
  }

  CopyMem (PacketHead, Head, sizeof (EFI_IP6_HEADER));
  Packet->Ip.Ip6 = PacketHead;

  if (ExtHdrs != NULL) {
    Buf = (UINT8 *) (PacketHead + 1);
    CopyMem (Buf, ExtHdrs, ExtHdrsLen);
  }

  if (UpdatedExtHdrs != NULL) {
    //
    // A Fragment Header is inserted to the packet, update the payload length.
    //
    PacketHead->PayloadLength = (UINT16) (NTOHS (PacketHead->PayloadLength) +
                                sizeof (IP6_FRAGMENT_HEADER));
    PacketHead->PayloadLength = HTONS (PacketHead->PayloadLength);
    FreePool (UpdatedExtHdrs);
  }

  return Ip6SendFrame (
           IpIf,
           IpInstance,
           Packet,
           &NextHop,
           Callback,
           Context
           );

Error:
  if (UpdatedExtHdrs != NULL) {
    FreePool (UpdatedExtHdrs);
  }
  Ip6CancelPacket (IpIf, Packet, Status);
  return Status;
}

/**
  The filter function to find a packet and all its fragments.
  The packet's fragments have their Context set to the packet.

  @param[in]  Frame            The frames hold by the low level interface.
  @param[in]  Context          Context to the function, which is the packet.

  @retval TRUE                 This is the packet to cancel or its fragments.
  @retval FALSE                This is an unrelated packet.

**/
BOOLEAN
Ip6CancelPacketFragments (
  IN IP6_LINK_TX_TOKEN   *Frame,
  IN VOID                *Context
  )
{
  if ((Frame->Packet == (NET_BUF *) Context) || (Frame->Context == Context)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Remove all the frames on the interface that pass the FrameToCancel,
  either queued on ARP queues or that have already been delivered to
  MNP and not yet recycled.

  @param[in]  Interface     Interface to remove the frames from.
  @param[in]  IoStatus      The transmit status returned to the frames' callback.
  @param[in]  FrameToCancel Function to select the frame to cancel; NULL to select all.
  @param[in]  Context       Opaque parameters passed to FrameToCancel. Ignored if
                            FrameToCancel is NULL.

**/
VOID
Ip6CancelFrames (
  IN IP6_INTERFACE          *Interface,
  IN EFI_STATUS             IoStatus,
  IN IP6_FRAME_TO_CANCEL    FrameToCancel   OPTIONAL,
  IN VOID                   *Context        OPTIONAL
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_LINK_TX_TOKEN         *Token;
  IP6_SERVICE               *IpSb;
  IP6_NEIGHBOR_ENTRY        *ArpQue;
  EFI_STATUS                Status;

  IpSb = Interface->Service;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  //
  // Cancel all the pending frames on ARP requests
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Interface->ArpQues) {
    ArpQue = NET_LIST_USER_STRUCT (Entry, IP6_NEIGHBOR_ENTRY, ArpList);

    Status = Ip6FreeNeighborEntry (
               IpSb,
               ArpQue,
               FALSE,
               FALSE,
               IoStatus,
               FrameToCancel,
               Context
               );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Cancel all the frames that have been delivered to MNP
  // but not yet recycled.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Interface->SentFrames) {
    Token = NET_LIST_USER_STRUCT (Entry, IP6_LINK_TX_TOKEN, Link);

    if ((FrameToCancel == NULL) || FrameToCancel (Token, Context)) {
      IpSb->Mnp->Cancel (IpSb->Mnp, &Token->MnpToken);
    }
  }
}

/**
  Cancel the Packet and all its fragments.

  @param[in]  IpIf                 The interface from which the Packet is sent.
  @param[in]  Packet               The Packet to cancel.
  @param[in]  IoStatus             The status returns to the sender.

**/
VOID
Ip6CancelPacket (
  IN IP6_INTERFACE    *IpIf,
  IN NET_BUF          *Packet,
  IN EFI_STATUS       IoStatus
  )
{
  Ip6CancelFrames (IpIf, IoStatus, Ip6CancelPacketFragments, Packet);
}

