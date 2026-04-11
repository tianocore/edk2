/** @file

Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"

/**
  Return the cast type (Unicast/Broadcast) specific to an
  interface. All the addresses are host byte ordered.

  @param[in]  IpAddr                The IP address to classify in host byte order
  @param[in]  IpIf                  The interface that IpAddr received from

  @return The cast type of this IP address specific to the interface.
  @retval IP4_LOCAL_HOST        The IpAddr equals to the interface's address
  @retval IP4_SUBNET_BROADCAST  The IpAddr is a directed subnet broadcast to  the
                                interface
  @retval IP4_NET_BROADCAST     The IpAddr is a network broadcast to the interface
  @retval 0                     Otherwise.

**/
INTN
Ip4GetNetCast (
  IN  IP4_ADDR       IpAddr,
  IN  IP4_INTERFACE  *IpIf
  )
{
  if (IpAddr == IpIf->Ip) {
    return IP4_LOCAL_HOST;
  } else if (IpAddr == IpIf->SubnetBrdcast) {
    return IP4_SUBNET_BROADCAST;
  } else if (IpAddr == IpIf->NetBrdcast) {
    return IP4_NET_BROADCAST;
  }

  return 0;
}

/**
  Find the cast type of the packet related to the local host.
  This isn't the same as link layer cast type. For example, DHCP
  server may send local broadcast to the local unicast MAC.

  @param[in]  IpSb                  The IP4 service binding instance that received the
                                    packet
  @param[in]  Dst                   The destination address in the packet (host byte
                                    order)
  @param[in]  Src                   The source address in the packet (host byte order)

  @return The cast type for the Dst, it will return on the first non-promiscuous
          cast type to a configured interface. If the packet doesn't match any of
          the interface, multicast address and local broadcast address are checked.

**/
INTN
Ip4GetHostCast (
  IN  IP4_SERVICE  *IpSb,
  IN  IP4_ADDR     Dst,
  IN  IP4_ADDR     Src
  )
{
  LIST_ENTRY     *Entry;
  IP4_INTERFACE  *IpIf;
  INTN           Type;
  INTN           Class;

  Type = 0;

  if (IpSb->MnpConfigData.EnablePromiscuousReceive) {
    Type = IP4_PROMISCUOUS;
  }

  //
  // Go through the interface list of the IP service, most likely.
  //
  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    //
    // Skip the unconfigured interface and invalid source address:
    // source address can't be broadcast.
    //
    if (!IpIf->Configured || IP4_IS_BROADCAST (Ip4GetNetCast (Src, IpIf))) {
      continue;
    }

    if ((Class = Ip4GetNetCast (Dst, IpIf)) > Type) {
      return Class;
    }
  }

  //
  // If it is local broadcast address. The source address must
  // be a unicast address on one of the direct connected network.
  // If it is a multicast address, accept it only if we are in
  // the group.
  //
  if (Dst == IP4_ALLONE_ADDRESS) {
    IpIf = Ip4FindNet (IpSb, Src);

    if ((IpIf != NULL) && !IP4_IS_BROADCAST (Ip4GetNetCast (Src, IpIf))) {
      return IP4_LOCAL_BROADCAST;
    }
  } else if (IP4_IS_MULTICAST (Dst) && (Ip4FindGroup (&IpSb->IgmpCtrl, Dst) != NULL)) {
    return IP4_MULTICAST;
  }

  return Type;
}

/**
  Find an interface whose configured IP address is Ip.

  @param[in]  IpSb                  The IP4 service binding instance
  @param[in]  Ip                    The Ip address (host byte order) to find

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindInterface (
  IN IP4_SERVICE  *IpSb,
  IN IP4_ADDR     Ip
  )
{
  LIST_ENTRY     *Entry;
  IP4_INTERFACE  *IpIf;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured && (IpIf->Ip == Ip)) {
      return IpIf;
    }
  }

  return NULL;
}

/**
  Find an interface that Ip is on that connected network.

  @param[in]  IpSb                  The IP4 service binding instance
  @param[in]  Ip                    The Ip address (host byte order) to find

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindNet (
  IN IP4_SERVICE  *IpSb,
  IN IP4_ADDR     Ip
  )
{
  LIST_ENTRY     *Entry;
  IP4_INTERFACE  *IpIf;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured && IP4_NET_EQUAL (Ip, IpIf->Ip, IpIf->SubnetMask)) {
      return IpIf;
    }
  }

  return NULL;
}

/**
  Find an interface of the service with the same Ip/Netmask pair.

  @param[in]  IpSb                  Ip4 service binding instance
  @param[in]  Ip                    The Ip address to find (host byte order)
  @param[in]  Netmask               The network to find (host byte order)

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindStationAddress (
  IN IP4_SERVICE  *IpSb,
  IN IP4_ADDR     Ip,
  IN IP4_ADDR     Netmask
  )
{
  LIST_ENTRY     *Entry;
  IP4_INTERFACE  *IpIf;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured && (IpIf->Ip == Ip) && (IpIf->SubnetMask == Netmask)) {
      return IpIf;
    }
  }

  return NULL;
}

/**
  Get the MAC address for a multicast IP address. Call
  Mnp's McastIpToMac to find the MAC address in stead of
  hard code the NIC to be Ethernet.

  @param[in]  Mnp                   The Mnp instance to get the MAC address.
  @param[in]  Multicast             The multicast IP address to translate.
  @param[out] Mac                   The buffer to hold the translated address.

  @retval EFI_SUCCESS if the multicast IP is successfully translated to a
                      multicast MAC address.
  @retval other       Otherwise some error.

**/
EFI_STATUS
Ip4GetMulticastMac (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp,
  IN  IP4_ADDR                      Multicast,
  OUT EFI_MAC_ADDRESS               *Mac
  )
{
  EFI_IP_ADDRESS  EfiIp;

  EFI_IP4 (EfiIp.v4) = HTONL (Multicast);
  return Mnp->McastIpToMac (Mnp, FALSE, &EfiIp, Mac);
}

/**
  Convert the multibyte field in IP header's byter order.
  In spite of its name, it can also be used to convert from
  host to network byte order.

  @param[in]  Head                  The IP head to convert

  @return Point to the converted IP head

**/
IP4_HEAD *
Ip4NtohHead (
  IN IP4_HEAD  *Head
  )
{
  Head->TotalLen = NTOHS (Head->TotalLen);
  Head->Id       = NTOHS (Head->Id);
  Head->Fragment = NTOHS (Head->Fragment);
  Head->Src      = NTOHL (Head->Src);
  Head->Dst      = NTOHL (Head->Dst);

  return Head;
}

/**
  Validate that Ip/Netmask pair is OK to be used as station
  address. Only continuous netmasks are supported. and check
  that StationAddress is a unicast address on the network.

  @param[in]  Ip                 The IP address to validate.
  @param[in]  Netmask            The netmask of the IP.

  @retval TRUE                   The Ip/Netmask pair is valid.
  @retval FALSE                  The Ip/Netmask pair is invalid.

**/
BOOLEAN
Ip4StationAddressValid (
  IN IP4_ADDR  Ip,
  IN IP4_ADDR  Netmask
  )
{
  //
  // Only support the station address with 0.0.0.0/0 to enable DHCP client.
  //
  if (Netmask == IP4_ALLZERO_ADDRESS) {
    return (BOOLEAN)(Ip == IP4_ALLZERO_ADDRESS);
  }

  //
  // Only support the continuous net masks
  //
  if (NetGetMaskLength (Netmask) == (IP4_MASK_MAX + 1)) {
    return FALSE;
  }

  //
  // Station address can't be class D or class E address
  //
  if (NetGetIpClass (Ip) > IP4_ADDR_CLASSC) {
    return FALSE;
  }

  return NetIp4IsUnicast (Ip, Netmask);
}
