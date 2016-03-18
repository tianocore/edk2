/** @file
  Common definition and functions for IP6 driver.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_COMMON_H__
#define __EFI_IP6_COMMON_H__

#define IP6_LINK_EQUAL(Mac1, Mac2) (CompareMem ((Mac1), (Mac2), sizeof (EFI_MAC_ADDRESS)) == 0)

//
// Convert the Microsecond to second. IP transmit/receive time is
// in the unit of microsecond. IP ticks once per second.
//
#define IP6_US_TO_SEC(Us)              (((Us) + 999999) / 1000000)

#define IP6_ETHER_PROTO                0x86DD

#define IP6_MAC_LEN                    6
#define IP6_IF_ID_LEN                  8

#define IP6_INTERFACE_LOCAL_SCOPE      1
#define IP6_LINK_LOCAL_SCOPE           2
#define IP6_SITE_LOCAL_SCOPE           5

#define IP6_INFINIT_LIFETIME           0xFFFFFFFF

#define IP6_HOP_LIMIT                  255
//
// Make it to 64 since all 54 bits are zero.
//
#define IP6_LINK_LOCAL_PREFIX_LENGTH   64

#define IP6_TIMER_INTERVAL_IN_MS       100
#define IP6_ONE_SECOND_IN_MS           1000

//
// The packet is received as link level broadcast/multicast/promiscuous.
//
#define IP6_LINK_BROADCAST             0x00000001
#define IP6_LINK_MULTICAST             0x00000002
#define IP6_LINK_PROMISC               0x00000004

#define IP6_U_BIT                      0x02

typedef enum {
  Ip6Promiscuous                     = 1,
  Ip6Unicast,
  Ip6Multicast,
  Ip6AnyCast
} IP6_ADDRESS_TYPE;

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  EFI_IPv6_ADDRESS              *Address;
} IP6_DESTROY_CHILD_BY_ADDR_CALLBACK_CONTEXT;

typedef struct _IP6_INTERFACE    IP6_INTERFACE;
typedef struct _IP6_PROTOCOL     IP6_PROTOCOL;
typedef struct _IP6_SERVICE      IP6_SERVICE;
typedef struct _IP6_ADDRESS_INFO IP6_ADDRESS_INFO;

/**
  Build a array of EFI_IP6_ADDRESS_INFO to be returned to the caller. The number
  of EFI_IP6_ADDRESS_INFO is also returned. If AddressList is NULL,
  only the address count is returned.

  @param[in]  IpSb              The IP6 service binding instance.
  @param[out] AddressCount      The number of returned addresses.
  @param[out] AddressList       The pointer to the array of EFI_IP6_ADDRESS_INFO.
                                This is an optional parameter.


  @retval EFI_SUCCESS           The address array is successfully build
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the address info.
  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.

**/
EFI_STATUS
Ip6BuildEfiAddressList (
  IN IP6_SERVICE            *IpSb,
  OUT UINT32                *AddressCount,
  OUT EFI_IP6_ADDRESS_INFO  **AddressList OPTIONAL
  );

/**
  Generate the multicast addresses identify the group of all IPv6 nodes or IPv6
  routers defined in RFC4291.

  All Nodes Addresses: FF01::1, FF02::1.
  All Router Addresses: FF01::2, FF02::2, FF05::2.

  @param[in]  Router            If TRUE, generate all routers addresses,
                                else generate all node addresses.
  @param[in]  Scope             interface-local(1), link-local(2), or site-local(5)
  @param[out] Ip6Addr           The generated multicast address.

  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_SUCCESS           The address is generated.

**/
EFI_STATUS
Ip6SetToAllNodeMulticast (
  IN  BOOLEAN          Router,
  IN  UINT8            Scope,
  OUT EFI_IPv6_ADDRESS *Ip6Addr
  );

/**
  This function converts MAC address to 64 bits interface ID according to RFC4291
  and returns the interface ID. Currently only 48-bit MAC address is supported by
  this function.

  @param[in, out]  IpSb      The IP6 service binding instance.

  @retval          NULL      The operation fails.
  @return                    Pointer to the generated interface ID.

**/
UINT8 *
Ip6CreateInterfaceID (
  IN OUT IP6_SERVICE         *IpSb
  );

/**
  This function creates link-local address from interface identifier. The
  interface identifier is normally created from MAC address. It might be manually
  configured by administrator if the link-local address created from MAC address
  is a duplicate address.

  @param[in, out]  IpSb      The IP6 service binding instance.

  @retval          NULL      If the operation fails.
  @return                    The generated Link Local address, in network order.

**/
EFI_IPv6_ADDRESS *
Ip6CreateLinkLocalAddr (
  IN OUT IP6_SERVICE         *IpSb
  );

/**
  Compute the solicited-node multicast address for an unicast or anycast address,
  by taking the low-order 24 bits of this address, and appending those bits to
  the prefix FF02:0:0:0:0:1:FF00::/104.

  @param  Ip6Addr               The unicast or anycast address, in network order.
  @param  MulticastAddr         The generated solicited-node multicast address,
                                in network order.

**/
VOID
Ip6CreateSNMulticastAddr (
  IN EFI_IPv6_ADDRESS  *Ip6Addr,
  OUT EFI_IPv6_ADDRESS *MulticastAddr
  );

/**
  Check whether the incoming Ipv6 address is a solicited-node multicast address.

  @param[in]  Ip6               Ip6 address, in network order.

  @retval TRUE                  Yes, solicited-node multicast address
  @retval FALSE                 No

**/
BOOLEAN
Ip6IsSNMulticastAddr (
  IN EFI_IPv6_ADDRESS *Ip6
  );

/**
  Check whether the incoming IPv6 address is one of the maintained address in
  the IP6 service binding instance.

  @param[in]  IpSb              Points to a IP6 service binding instance
  @param[in]  Address           The IP6 address to be checked.
  @param[out] Interface         If not NULL, output the IP6 interface which
                                maintains the Address.
  @param[out] AddressInfo       If not NULL, output the IP6 address information
                                of the Address.

  @retval TRUE                  Yes, it is one of the maintained addresses.
  @retval FALSE                 No, it is not one of the maintained addresses.

**/
BOOLEAN
Ip6IsOneOfSetAddress (
  IN  IP6_SERVICE           *IpSb,
  IN  EFI_IPv6_ADDRESS      *Address,
  OUT IP6_INTERFACE         **Interface   OPTIONAL,
  OUT IP6_ADDRESS_INFO      **AddressInfo OPTIONAL
  );

/**
  Check whether the incoming MAC address is valid.

  @param[in]  IpSb              Points to a IP6 service binding instance.
  @param[in]  LinkAddress       The MAC address.

  @retval TRUE                  Yes, it is valid.
  @retval FALSE                 No, it is not valid.

**/
BOOLEAN
Ip6IsValidLinkAddress (
  IN  IP6_SERVICE      *IpSb,
  IN  EFI_MAC_ADDRESS  *LinkAddress
  );


/**
  Copy the PrefixLength bits from Src to Dest.

  @param[out] Dest              A pointer to the buffer to copy to.
  @param[in]  Src               A pointer to the buffer to copy from.
  @param[in]  PrefixLength      The number of bits to copy.

**/
VOID
Ip6CopyAddressByPrefix (
  OUT EFI_IPv6_ADDRESS *Dest,
  IN  EFI_IPv6_ADDRESS *Src,
  IN  UINT8            PrefixLength
  );

/**
  Insert a node IP6_ADDRESS_INFO to an IP6 interface.

  @param[in, out]  IpIf             Points to an IP6 interface.
  @param[in]       AddrInfo         Points to an IP6_ADDRESS_INFO.

**/
VOID
Ip6AddAddr (
  IN OUT IP6_INTERFACE *IpIf,
  IN IP6_ADDRESS_INFO  *AddrInfo
  );

/**
  Remove the IPv6 address from the address list node points to IP6_ADDRESS_INFO.

  This function removes the matching IPv6 addresses from the address list and
  adjusts the address count of the address list. If IpSb is not NULL, this function
  calls Ip6LeaveGroup to see whether it should call Mnp->Groups() to remove the
  its solicited-node multicast MAC address from the filter list and sends out
  a Multicast Listener Done. If Prefix is NULL, all address in the address list
  will be removed. If Prefix is not NULL, the address that matching the Prefix
  with PrefixLength in the address list will be removed.

  @param[in]       IpSb             NULL or points to IP6 service binding instance.
  @param[in, out]  AddressList      address list array
  @param[in, out]  AddressCount     the count of addresses in address list array
  @param[in]       Prefix           NULL or an IPv6 address prefix
  @param[in]       PrefixLength     the length of Prefix

  @retval    EFI_SUCCESS            The operation completed successfully.
  @retval    EFI_NOT_FOUND          The address matching the Prefix with PrefixLength
                                    cannot be found in address list.
  @retval    EFI_INVALID_PARAMETER  Any input parameter is invalid.

**/
EFI_STATUS
Ip6RemoveAddr (
  IN IP6_SERVICE       *IpSb          OPTIONAL,
  IN OUT LIST_ENTRY    *AddressList,
  IN OUT UINT32        *AddressCount,
  IN EFI_IPv6_ADDRESS  *Prefix        OPTIONAL,
  IN UINT8             PrefixLength
  );

/**
  Get the MAC address for a multicast IP address. Call
  Mnp's McastIpToMac to find the MAC address instead of
  hard-coding the NIC to be Ethernet.

  @param[in]  Mnp                   The Mnp instance to get the MAC address.
  @param[in]  Multicast             The multicast IP address to translate.
  @param[out] Mac                   The buffer to hold the translated address.

  @retval EFI_SUCCESS               The multicast IP is successfully
                                    translated to a multicast MAC address.
  @retval Other                     The address is not converted because an error occurred.

**/
EFI_STATUS
Ip6GetMulticastMac (
  IN  EFI_MANAGED_NETWORK_PROTOCOL *Mnp,
  IN  EFI_IPv6_ADDRESS             *Multicast,
  OUT EFI_MAC_ADDRESS              *Mac
  );

/**
  Convert the multibyte field in IP header's byter order.
  In spite of its name, it can also be used to convert from
  host to network byte order.

  @param[in, out]  Head                  The IP head to convert.

  @return Point to the converted IP head.

**/
EFI_IP6_HEADER *
Ip6NtohHead (
  IN OUT EFI_IP6_HEADER *Head
  );

#endif
