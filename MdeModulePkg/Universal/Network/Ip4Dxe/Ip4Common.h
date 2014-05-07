/** @file
  Common definition for IP4.
  
Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP4_COMMON_H__
#define __EFI_IP4_COMMON_H__

typedef struct _IP4_INTERFACE  IP4_INTERFACE;
typedef struct _IP4_PROTOCOL   IP4_PROTOCOL;
typedef struct _IP4_SERVICE    IP4_SERVICE;

#define IP4_ETHER_PROTO       0x0800

//
// The packet is received as link level broadcast/multicast/promiscuous.
//
#define IP4_LINK_BROADCAST    0x00000001
#define IP4_LINK_MULTICAST    0x00000002
#define IP4_LINK_PROMISC      0x00000004

//
// IP4 address cast type classfication. Keep it true that any
// type bigger than or equal to LOCAL_BROADCAST is broadcast.
//
#define IP4_PROMISCUOUS       1
#define IP4_LOCAL_HOST        2
#define IP4_MULTICAST         3
#define IP4_LOCAL_BROADCAST   4  // Destination is 255.255.255.255
#define IP4_SUBNET_BROADCAST  5
#define IP4_NET_BROADCAST     6

//
// IP4 header flags
//
#define IP4_HEAD_DF_MASK      0x4000
#define IP4_HEAD_MF_MASK      0x2000
#define IP4_HEAD_OFFSET_MASK  0x1fff

#define IP4_ALLZERO_ADDRESS   0x00000000u
#define IP4_ALLONE_ADDRESS    0xFFFFFFFFu
#define IP4_ALLSYSTEM_ADDRESS 0xE0000001u
#define IP4_ALLROUTER_ADDRESS 0xE0000002u

///
/// Compose the fragment field to be used in the IP4 header.
///
#define IP4_HEAD_FRAGMENT_FIELD(Df, Mf, Offset) \
    ((UINT16)(((Df) ? 0x4000 : 0) | ((Mf) ? 0x2000 : 0) | (((Offset) >> 3) & 0x1fff)))

#define IP4_LAST_FRAGMENT(FragmentField)  \
          (((FragmentField) & IP4_HEAD_MF_MASK) == 0)

#define IP4_FIRST_FRAGMENT(FragmentField) \
          ((BOOLEAN)(((FragmentField) & IP4_HEAD_OFFSET_MASK) == 0))

#define IP4_DO_NOT_FRAGMENT(FragmentField) \
          ((BOOLEAN)(((FragmentField) & IP4_HEAD_DF_MASK) == IP4_HEAD_DF_MASK))

#define IP4_IS_BROADCAST(CastType) ((CastType) >= IP4_LOCAL_BROADCAST)

///
/// Conver the Microsecond to second. IP transmit/receive time is
/// in the unit of microsecond. IP ticks once per second.
///
#define IP4_US_TO_SEC(Us) (((Us) + 999999) / 1000000)

/**
  Return the cast type (Unicast/Boradcast) specific to an
  interface. All the addresses are host byte ordered.

  @param[in]  IpAddr                The IP address to classify in host byte order
  @param[in]  IpIf                  The interface that IpAddr received from

  @return The cast type of this IP address specific to the interface.
  @retval IP4_LOCAL_HOST        The IpAddr equals to the interface's address
  @retval IP4_SUBNET_BROADCAST  The IpAddr is a directed subnet boradcast to  the
                                interface
  @retval IP4_NET_BROADCAST     The IpAddr is a network broadcast to the interface
  @retval 0                     Otherwise.

**/
INTN
Ip4GetNetCast (
  IN  IP4_ADDR          IpAddr,
  IN  IP4_INTERFACE     *IpIf
  );

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
  IN  IP4_SERVICE       *IpSb,
  IN  IP4_ADDR          Dst,
  IN  IP4_ADDR          Src
  );

/**
  Find an interface whose configured IP address is Ip.

  @param[in]  IpSb                  The IP4 service binding instance
  @param[in]  Ip                    The Ip address (host byte order) to find

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindInterface (
  IN IP4_SERVICE        *IpSb,
  IN IP4_ADDR           Ip
  );

/**
  Find an interface that Ip is on that connected network.

  @param[in]  IpSb                  The IP4 service binding instance
  @param[in]  Ip                    The Ip address (host byte order) to find

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindNet (
  IN IP4_SERVICE        *IpSb,
  IN IP4_ADDR           Ip
  );

/**
  Find an interface of the service with the same Ip/Netmask pair.

  @param[in]  IpSb                  Ip4 service binding instance
  @param[in]  Ip                    The Ip adress to find (host byte order)
  @param[in]  Netmask               The network to find (host byte order)

  @return The IP4_INTERFACE point if found, otherwise NULL

**/
IP4_INTERFACE *
Ip4FindStationAddress (
  IN IP4_SERVICE        *IpSb,
  IN IP4_ADDR           Ip,
  IN IP4_ADDR           Netmask
  );

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
  IN  EFI_MANAGED_NETWORK_PROTOCOL *Mnp,
  IN  IP4_ADDR                     Multicast,
  OUT EFI_MAC_ADDRESS              *Mac
  );

/**
  Convert the multibyte field in IP header's byter order.
  In spite of its name, it can also be used to convert from
  host to network byte order.

  @param[in]  Head                  The IP head to convert

  @return Point to the converted IP head

**/
IP4_HEAD *
Ip4NtohHead (
  IN IP4_HEAD           *Head
  );

#endif
