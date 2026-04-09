/** @file
  Transmit the IP4 packet.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"

UINT16  mIp4Id;

/**
  Prepend an IP4 head to the Packet. It will copy the options and
  build the IP4 header fields. Used for IP4 fragmentation.

  @param  Packet           The packet to prepend IP4 header to
  @param  Head             The caller supplied header. The caller should set
                           the following header fields: Tos, TotalLen, Id,
                           Fragment, Ttl, Protocol, Src and Dst. All the fields
                           are in host byte order. This function will fill in
                           the Ver, HeadLen, and checksum.
  @param  Option           The original IP4 option to copy from
  @param  OptLen           The length of the IP4 option

  @retval EFI_BAD_BUFFER_SIZE  There is no enough room in the head space of
                               Packet.
  @retval EFI_SUCCESS          The IP4 header is successfully added to the packet.

**/
EFI_STATUS
Ip4PrependHead (
  IN OUT NET_BUF   *Packet,
  IN     IP4_HEAD  *Head,
  IN     UINT8     *Option,
  IN     UINT32    OptLen
  )
{
  UINT32    HeadLen;
  UINT32    Len;
  IP4_HEAD  *PacketHead;
  BOOLEAN   FirstFragment;

  //
  // Prepend the options: first get the option length, then copy it over.
  //
  HeadLen       = 0;
  FirstFragment = IP4_FIRST_FRAGMENT (Head->Fragment);

  Ip4CopyOption (Option, OptLen, FirstFragment, NULL, &Len);

  HeadLen = IP4_MIN_HEADLEN + Len;
  ASSERT (((Len % 4) == 0) && (HeadLen <= IP4_MAX_HEADLEN));

  PacketHead = (IP4_HEAD *)NetbufAllocSpace (Packet, HeadLen, NET_BUF_HEAD);

  if (PacketHead == NULL) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Ip4CopyOption (Option, OptLen, FirstFragment, (UINT8 *)(PacketHead + 1), &Len);

  //
  // Set the head up, convert the host byte order to network byte order
  //
  PacketHead->Ver      = 4;
  PacketHead->HeadLen  = (UINT8)(HeadLen >> 2);
  PacketHead->Tos      = Head->Tos;
  PacketHead->TotalLen = HTONS ((UINT16)Packet->TotalSize);
  PacketHead->Id       = HTONS (Head->Id);
  PacketHead->Fragment = HTONS (Head->Fragment);
  PacketHead->Checksum = 0;
  PacketHead->Ttl      = Head->Ttl;
  PacketHead->Protocol = Head->Protocol;
  PacketHead->Src      = HTONL (Head->Src);
  PacketHead->Dst      = HTONL (Head->Dst);
  PacketHead->Checksum = (UINT16)(~NetblockChecksum ((UINT8 *)PacketHead, HeadLen));

  Packet->Ip.Ip4 = PacketHead;
  return EFI_SUCCESS;
}

/**
  Select an interface to send the packet generated in the IP4 driver
  itself, that is, not by the requests of IP4 child's consumer. Such
  packets include the ICMP echo replies, and other ICMP error packets.

  @param[in]  IpSb                 The IP4 service that wants to send the packets.
  @param[in]  Dst                  The destination of the packet
  @param[in]  Src                  The source of the packet

  @return NULL if no proper interface is found, otherwise the interface that
          can be used to send the system packet from.

**/
IP4_INTERFACE *
Ip4SelectInterface (
  IN IP4_SERVICE  *IpSb,
  IN IP4_ADDR     Dst,
  IN IP4_ADDR     Src
  )
{
  IP4_INTERFACE  *IpIf;
  IP4_INTERFACE  *Selected;
  LIST_ENTRY     *Entry;

  //
  // Select the interface the Dst is on if one of the connected
  // network. Some IP instance may be configured with 0.0.0.0/0,
  // don't select that interface now.
  //
  IpIf = Ip4FindNet (IpSb, Dst);

  if ((IpIf != NULL) && (IpIf->Ip != IP4_ALLZERO_ADDRESS)) {
    return IpIf;
  }

  //
  // If source is one of the interface address, select it.
  //
  IpIf = Ip4FindInterface (IpSb, Src);

  if ((IpIf != NULL) && (IpIf->Ip != IP4_ALLZERO_ADDRESS)) {
    return IpIf;
  }

  //
  // Select a configured interface as the fall back. Always prefer
  // an interface with non-zero address.
  //
  Selected = NULL;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured && ((Selected == NULL) || (Selected->Ip == 0))) {
      Selected = IpIf;
    }
  }

  return Selected;
}

/**
  The default callback function for system generated packet.
  It will free the packet.

  @param  Ip4Instance          The IP4 child that issued the transmission.  It most
                               like is NULL.
  @param  Packet               The packet that transmitted.
  @param  IoStatus             The result of the transmission, succeeded or failed.
  @param  LinkFlag             Not used when transmission. check IP4_FRAME_CALLBACK
                               for reference.
  @param  Context              The context provided by us

**/
VOID
Ip4SysPacketSent (
  IP4_PROTOCOL  *Ip4Instance,
  NET_BUF       *Packet,
  EFI_STATUS    IoStatus,
  UINT32        LinkFlag,
  VOID          *Context
  )
{
  NetbufFree (Packet);
}

/**
  Transmit an IP4 packet. The packet comes either from the IP4
  child's consumer (IpInstance != NULL) or the IP4 driver itself
  (IpInstance == NULL). It will route the packet, fragment it,
  then transmit all the fragments through some interface.

  @param[in]  IpSb             The IP4 service instance to transmit the packet
  @param[in]  IpInstance       The IP4 child that issues the transmission.  It is
                               NULL if the packet is from the system.
  @param[in]  Packet           The user data to send, excluding the IP header.
  @param[in]  Head             The caller supplied header. The caller should set
                               the following header fields: Tos, TotalLen, Id, tl,
                               Fragment, Protocol, Src and Dst. All the fields are
                               in host byte  order. This function will fill in the
                               Ver, HeadLen,  Fragment, and checksum. The Fragment
                               only need to include the DF flag. Ip4Output will
                               compute the MF and offset for  you.
  @param[in]  Option           The original option to append to the IP headers
  @param[in]  OptLen           The length of the option
  @param[in]  GateWay          The next hop address to transmit packet to.
                               255.255.255.255 means broadcast.
  @param[in]  Callback         The callback function to issue when transmission
                               completed.
  @param[in]  Context          The opaque context for the callback

  @retval EFI_NO_MAPPING       There is no interface to the destination.
  @retval EFI_NOT_FOUND        There is no route to the destination
  @retval EFI_SUCCESS          The packet is successfully transmitted.
  @retval EFI_BAD_BUFFER_SIZE  The length of the IPv4 header + option length +
                               total data length is greater than MTU (or greater
                               than the maximum packet size if Token.Packet.TxData.
                               OverrideData.DoNotFragment is TRUE.)
  @retval Others               Failed to transmit the packet.

**/
EFI_STATUS
Ip4Output (
  IN IP4_SERVICE         *IpSb,
  IN IP4_PROTOCOL        *IpInstance  OPTIONAL,
  IN NET_BUF             *Packet,
  IN IP4_HEAD            *Head,
  IN UINT8               *Option,
  IN UINT32              OptLen,
  IN IP4_ADDR            GateWay,
  IN IP4_FRAME_CALLBACK  Callback,
  IN VOID                *Context
  )
{
  IP4_INTERFACE          *IpIf;
  IP4_ROUTE_CACHE_ENTRY  *CacheEntry;
  IP4_ADDR               Dest;
  EFI_STATUS             Status;
  NET_BUF                *Fragment;
  UINT32                 Index;
  UINT32                 HeadLen;
  UINT32                 PacketLen;
  UINT32                 Offset;
  UINT32                 Mtu;
  UINT32                 Num;
  BOOLEAN                RawData;

  //
  // Select an interface/source for system packet, application
  // should select them itself.
  //
  if (IpInstance == NULL) {
    IpIf = Ip4SelectInterface (IpSb, Head->Dst, Head->Src);
  } else {
    IpIf = IpInstance->Interface;
  }

  if (IpIf == NULL) {
    return EFI_NO_MAPPING;
  }

  if ((Head->Src == IP4_ALLZERO_ADDRESS) && (IpInstance == NULL)) {
    Head->Src = IpIf->Ip;
  }

  //
  // Before IPsec process, prepared the IP head.
  // If Ip4Output is transmitting RawData, don't update IPv4 header.
  //
  HeadLen = sizeof (IP4_HEAD) + ((OptLen + 3) & (~0x03));

  if ((IpInstance != NULL) && IpInstance->ConfigData.RawData) {
    RawData = TRUE;
  } else {
    Head->HeadLen = (UINT8)(HeadLen >> 2);
    Head->Id      = mIp4Id++;
    Head->Ver     = 4;
    RawData       = FALSE;
  }

  //
  // Call IPsec process.
  //
  Status = Ip4IpSecProcessPacket (
             IpSb,
             &Head,
             &Packet,
             &Option,
             &OptLen,
             EfiIPsecOutBound,
             Context
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dest = Head->Dst;
  if (IP4_IS_BROADCAST (Ip4GetNetCast (Dest, IpIf)) || (Dest == IP4_ALLONE_ADDRESS)) {
    //
    // Set the gateway to local broadcast if the Dest is
    // the broadcast address for the connected network or
    // it is local broadcast.
    //
    GateWay = IP4_ALLONE_ADDRESS;
  } else if (IP4_IS_MULTICAST (Dest)) {
    //
    // Set the gateway to the destination if it is an multicast
    // address. The IP4_INTERFACE won't consult ARP to send local
    // broadcast and multicast.
    //
    GateWay = Head->Dst;
  } else if (GateWay == IP4_ALLZERO_ADDRESS) {
    //
    // Route the packet unless overridden, that is, GateWay isn't zero.
    //
    if (IpInstance == NULL) {
      CacheEntry = Ip4Route (IpSb->DefaultRouteTable, Head->Dst, Head->Src, IpIf->SubnetMask, TRUE);
    } else {
      CacheEntry = Ip4Route (IpInstance->RouteTable, Head->Dst, Head->Src, IpIf->SubnetMask, FALSE);
      //
      // If failed to route the packet by using the instance's route table,
      // try to use the default route table.
      //
      if (CacheEntry == NULL) {
        CacheEntry = Ip4Route (IpSb->DefaultRouteTable, Head->Dst, Head->Src, IpIf->SubnetMask, TRUE);
      }
    }

    if (CacheEntry == NULL) {
      return EFI_NOT_FOUND;
    }

    GateWay = CacheEntry->NextHop;
    Ip4FreeRouteCacheEntry (CacheEntry);
  }

  //
  // OK, selected the source and route, fragment the packet then send
  // them. Tag each fragment other than the first one as spawn from it.
  //
  Mtu = IpSb->MaxPacketSize + sizeof (IP4_HEAD);

  if (Packet->TotalSize + HeadLen > Mtu) {
    //
    // Fragmentation is disabled for RawData mode.
    //
    if (RawData) {
      return EFI_BAD_BUFFER_SIZE;
    }

    //
    // Packet is fragmented from the tail to the head, that is, the
    // first frame sent is the last fragment of the packet. The first
    // fragment is NOT sent in this loop. First compute how many
    // fragments there are.
    //
    Mtu = (Mtu - HeadLen) & (~0x07);
    Num = (Packet->TotalSize + Mtu - 1) / Mtu;

    //
    // Initialize the packet length and Offset. Other than the last
    // fragment, the packet length equals to MTU. The offset is always
    // aligned to MTU.
    //
    PacketLen = Packet->TotalSize - (Num - 1) * Mtu;
    Offset    = Mtu * (Num - 1);

    for (Index = 0; Index < Num - 1; Index++, Offset -= Mtu) {
      Fragment = NetbufGetFragment (Packet, Offset, PacketLen, IP4_MAX_HEADLEN);

      if (Fragment == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_ERROR;
      }

      //
      // Update the header's fragment. The caller fills the IP4 header
      // fields that are required by Ip4PrependHead except the fragment.
      //
      Head->Fragment = IP4_HEAD_FRAGMENT_FIELD (FALSE, (Index != 0), Offset);
      Ip4PrependHead (Fragment, Head, Option, OptLen);

      //
      // Transmit the fragments, pass the Packet address as the context.
      // So, we can find all the fragments spawned from the Packet by
      // compare the NetBuf and Context to the Packet.
      //
      Status = Ip4SendFrame (
                 IpIf,
                 IpInstance,
                 Fragment,
                 GateWay,
                 Ip4SysPacketSent,
                 Packet,
                 IpSb
                 );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      PacketLen = Mtu;
    }

    //
    // Trim the already sent data, then adjust the head's fragment field.
    //
    NetbufTrim (Packet, Packet->TotalSize - Mtu, FALSE);
    Head->Fragment = IP4_HEAD_FRAGMENT_FIELD (FALSE, TRUE, 0);
  }

  //
  // Send the first fragment, it is either the original packet, or the
  // first fragment of a fragmented packet. It seems that there is a subtle
  // bug here: what if the caller free the packet in Callback and IpIf (or
  // MNP child used by that interface) still holds the fragments and try
  // to access the data? The caller can free the packet if it recycles the
  // consumer's (such as UDP) data in the Callback. But this can't happen.
  // The detailed sequence is:
  // 1. for the packets generated by IP4 driver itself:
  //    The Callback is Ip4SysPacketSent, which is the same as the
  //    fragments' callback. Ip4SysPacketSent simply calls NetbufFree
  //    to release its reference to the packet. So, no problem for
  //    system packets.
  //
  // 2. for the upper layer's packets (use UDP as an example):
  //    UDP requests the IP layer to transmit some data which is
  //    wrapped in an asynchronous token, the token is wrapped
  //    in IP4_TXTOKEN_WRAP by IP4. IP4 also wrap the user's data
  //    in a net buffer, which is Packet we get here. IP4_TXTOKEN_WRAP
  //    is bound with the Packet. It will only be freed when all
  //    the references to Packet have been released. Upon then, the
  //    Packet's OnFree callback will release the IP4_TXTOKEN_WRAP,
  //    and signal the user's recycle event. So, also no problem for
  //    upper layer's packets.
  //
  Ip4PrependHead (Packet, Head, Option, OptLen);
  Status = Ip4SendFrame (IpIf, IpInstance, Packet, GateWay, Callback, Context, IpSb);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  Ip4CancelPacket (IpIf, Packet, Status);
  return Status;
}

/**
  The filter function to find a packet and all its fragments.
  The packet's fragments have their Context set to the packet.

  @param[in]  Frame            The frames hold by the low level interface
  @param[in]  Context          Context to the function, which is the packet.

  @retval TRUE                 This is the packet to cancel or its fragments.
  @retval FALSE                This is unrelated packet.

**/
BOOLEAN
Ip4CancelPacketFragments (
  IN IP4_LINK_TX_TOKEN  *Frame,
  IN VOID               *Context
  )
{
  if ((Frame->Packet == (NET_BUF *)Context) || (Frame->Context == Context)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Cancel the Packet and all its fragments.

  @param  IpIf                 The interface from which the Packet is sent
  @param  Packet               The Packet to cancel
  @param  IoStatus             The status returns to the sender.

**/
VOID
Ip4CancelPacket (
  IN IP4_INTERFACE  *IpIf,
  IN NET_BUF        *Packet,
  IN EFI_STATUS     IoStatus
  )
{
  Ip4CancelFrames (IpIf, IoStatus, Ip4CancelPacketFragments, Packet);
}
