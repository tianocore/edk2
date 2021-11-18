/** @file
  Definition of Neighbor Discovery support routines.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP6_ND_H__
#define __EFI_IP6_ND_H__

#define IP6_GET_TICKS(Ms)  (((Ms) + IP6_TIMER_INTERVAL_IN_MS - 1) / IP6_TIMER_INTERVAL_IN_MS)

enum {
  IP6_INF_ROUTER_LIFETIME = 0xFFFF,

  IP6_MAX_RTR_SOLICITATION_DELAY = 1000, ///< 1000 milliseconds
  IP6_MAX_RTR_SOLICITATIONS      = 3,
  IP6_RTR_SOLICITATION_INTERVAL  = 4000,

  IP6_MIN_RANDOM_FACTOR_SCALED = 1,
  IP6_MAX_RANDOM_FACTOR_SCALED = 3,
  IP6_RANDOM_FACTOR_SCALE      = 2,

  IP6_MAX_MULTICAST_SOLICIT  = 3,
  IP6_MAX_UNICAST_SOLICIT    = 3,
  IP6_MAX_ANYCAST_DELAY_TIME = 1,
  IP6_MAX_NEIGHBOR_ADV       = 3,
  IP6_REACHABLE_TIME         = 30000,
  IP6_RETRANS_TIMER          = 1000,
  IP6_DELAY_FIRST_PROBE_TIME = 5000,

  IP6_MIN_LINK_MTU = 1280,
  IP6_MAX_LINK_MTU = 1500,

  IP6_IS_ROUTER_FLAG = 0x80,
  IP6_SOLICITED_FLAG = 0x40,
  IP6_OVERRIDE_FLAG  = 0x20,

  IP6_M_ADDR_CONFIG_FLAG = 0x80,
  IP6_O_CONFIG_FLAG      = 0x40,

  IP6_ON_LINK_FLAG     = 0x80,
  IP6_AUTO_CONFIG_FLAG = 0x40,

  IP6_ND_LENGTH           = 24,
  IP6_RA_LENGTH           = 16,
  IP6_REDITECT_LENGTH     = 40,
  IP6_DAD_ENTRY_SIGNATURE = SIGNATURE_32 ('I', 'P', 'D', 'E')
};

typedef
VOID
(*IP6_ARP_CALLBACK) (
  VOID  *Context
  );

typedef struct _IP6_OPTION_HEADER {
  UINT8    Type;
  UINT8    Length;
} IP6_OPTION_HEADER;

STATIC_ASSERT (sizeof (IP6_OPTION_HEADER) == 2, "IP6_OPTION_HEADER is expected to be exactly 2 bytes long.");

typedef struct _IP6_ETHE_ADDR_OPTION {
  UINT8    Type;
  UINT8    Length;
  UINT8    EtherAddr[6];
} IP6_ETHER_ADDR_OPTION;

STATIC_ASSERT (sizeof (IP6_ETHER_ADDR_OPTION) == 8, "IP6_ETHER_ADDR_OPTION is expected to be exactly 8 bytes long.");

typedef struct _IP6_MTU_OPTION {
  UINT8     Type;
  UINT8     Length;
  UINT16    Reserved;
  UINT32    Mtu;
} IP6_MTU_OPTION;

STATIC_ASSERT (sizeof (IP6_MTU_OPTION) == 8, "IP6_MTU_OPTION is expected to be exactly 8 bytes long.");

typedef struct _IP6_PREFIX_INFO_OPTION {
  UINT8               Type;
  UINT8               Length;
  UINT8               PrefixLength;
  UINT8               Reserved1;
  UINT32              ValidLifetime;
  UINT32              PreferredLifetime;
  UINT32              Reserved2;
  EFI_IPv6_ADDRESS    Prefix;
} IP6_PREFIX_INFO_OPTION;

STATIC_ASSERT (sizeof (IP6_PREFIX_INFO_OPTION) == 32, "IP6_PREFIX_INFO_OPTION is expected to be exactly 32 bytes long.");

typedef
VOID
(*IP6_DAD_CALLBACK) (
  IN BOOLEAN           IsDadPassed,
  IN EFI_IPv6_ADDRESS  *TargetAddress,
  IN VOID              *Context
  );

typedef struct _IP6_DAD_ENTRY {
  UINT32              Signature;
  LIST_ENTRY          Link;
  UINT32              MaxTransmit;
  UINT32              Transmit;
  UINT32              Receive;
  UINT32              RetransTick;
  IP6_ADDRESS_INFO    *AddressInfo;
  EFI_IPv6_ADDRESS    Destination;
  IP6_DAD_CALLBACK    Callback;
  VOID                *Context;
} IP6_DAD_ENTRY;

typedef struct _IP6_DELAY_JOIN_LIST {
  LIST_ENTRY          Link;
  UINT32              DelayTime;       ///< in tick per 50 milliseconds
  IP6_INTERFACE       *Interface;
  IP6_ADDRESS_INFO    *AddressInfo;
  IP6_DAD_CALLBACK    DadCallback;
  VOID                *Context;
} IP6_DELAY_JOIN_LIST;

typedef struct _IP6_NEIGHBOR_ENTRY {
  LIST_ENTRY                Link;
  LIST_ENTRY                ArpList;
  INTN                      RefCnt;
  BOOLEAN                   IsRouter;
  BOOLEAN                   ArpFree;
  BOOLEAN                   Dynamic;
  EFI_IPv6_ADDRESS          Neighbor;
  EFI_MAC_ADDRESS           LinkAddress;
  EFI_IP6_NEIGHBOR_STATE    State;
  UINT32                    Transmit;
  UINT32                    Ticks;

  LIST_ENTRY                Frames;
  IP6_INTERFACE             *Interface;
  IP6_ARP_CALLBACK          CallBack;
} IP6_NEIGHBOR_ENTRY;

typedef struct _IP6_DEFAULT_ROUTER {
  LIST_ENTRY            Link;
  INTN                  RefCnt;
  UINT16                Lifetime;
  EFI_IPv6_ADDRESS      Router;
  IP6_NEIGHBOR_ENTRY    *NeighborCache;
} IP6_DEFAULT_ROUTER;

typedef struct _IP6_PREFIX_LIST_ENTRY {
  LIST_ENTRY          Link;
  INTN                RefCnt;
  UINT32              ValidLifetime;
  UINT32              PreferredLifetime;
  UINT8               PrefixLength;
  EFI_IPv6_ADDRESS    Prefix;
} IP6_PREFIX_LIST_ENTRY;

/**
  Build a array of EFI_IP6_NEIGHBOR_CACHE to be returned to the caller. The number
  of EFI_IP6_NEIGHBOR_CACHE is also returned.

  @param[in]  IpInstance        The pointer to IP6_PROTOCOL instance.
  @param[out] NeighborCount     The number of returned neighbor cache entries.
  @param[out] NeighborCache     The pointer to the array of EFI_IP6_NEIGHBOR_CACHE.

  @retval EFI_SUCCESS           The EFI_IP6_NEIGHBOR_CACHE successfully built.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the route table.

**/
EFI_STATUS
Ip6BuildEfiNeighborCache (
  IN IP6_PROTOCOL             *IpInstance,
  OUT UINT32                  *NeighborCount,
  OUT EFI_IP6_NEIGHBOR_CACHE  **NeighborCache
  );

/**
  Build a array of EFI_IP6_ADDRESS_INFO to be returned to the caller. The number
  of prefix entries is also returned.

  @param[in]  IpInstance        The pointer to IP6_PROTOCOL instance.
  @param[out] PrefixCount       The number of returned prefix entries.
  @param[out] PrefixTable       The pointer to the array of PrefixTable.

  @retval EFI_SUCCESS           The prefix table successfully built.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the prefix table.

**/
EFI_STATUS
Ip6BuildPrefixTable (
  IN IP6_PROTOCOL           *IpInstance,
  OUT UINT32                *PrefixCount,
  OUT EFI_IP6_ADDRESS_INFO  **PrefixTable
  );

/**
  Allocate and initialize an IP6 default router entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  Ip6Address        The IPv6 address of the default router.
  @param[in]  RouterLifetime    The lifetime associated with the default
                                router, in units of seconds.

  @return NULL if it failed to allocate memory for the default router node.
          Otherwise, point to the created default router node.

**/
IP6_DEFAULT_ROUTER *
Ip6CreateDefaultRouter (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *Ip6Address,
  IN UINT16            RouterLifetime
  );

/**
  Destroy an IP6 default router entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  DefaultRouter     The to be destroyed IP6_DEFAULT_ROUTER.

**/
VOID
Ip6DestroyDefaultRouter (
  IN IP6_SERVICE         *IpSb,
  IN IP6_DEFAULT_ROUTER  *DefaultRouter
  );

/**
  Clean an IP6 default router list.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.

**/
VOID
Ip6CleanDefaultRouterList (
  IN IP6_SERVICE  *IpSb
  );

/**
  Search a default router node from an IP6 default router list.

  @param[in]  IpSb          The pointer to the IP6_SERVICE instance.
  @param[in]  Ip6Address    The IPv6 address of the to be searched default router node.

  @return NULL if it failed to find the matching default router node.
          Otherwise, point to the found default router node.

**/
IP6_DEFAULT_ROUTER *
Ip6FindDefaultRouter (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *Ip6Address
  );

/**
  The function to be called after DAD (Duplicate Address Detection) is performed.

  @param[in]  IsDadPassed   If TRUE, the DAD operation succeed. Otherwise, the DAD operation failed.
  @param[in]  IpIf          Points to the IP6_INTERFACE.
  @param[in]  DadEntry      The DAD entry which already performed DAD.

**/
VOID
Ip6OnDADFinished (
  IN BOOLEAN        IsDadPassed,
  IN IP6_INTERFACE  *IpIf,
  IN IP6_DAD_ENTRY  *DadEntry
  );

/**
  Create a DAD (Duplicate Address Detection) entry and queue it to be performed.

  @param[in]  IpIf          Points to the IP6_INTERFACE.
  @param[in]  AddressInfo   The address information which needs DAD performed.
  @param[in]  Callback      The callback routine that will be called after DAD
                            is performed. This is an optional parameter that
                            may be NULL.
  @param[in]  Context       The opaque parameter for a DAD callback routine.
                            This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS           The DAD entry was created and queued.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory to complete the
                                operation.


**/
EFI_STATUS
Ip6InitDADProcess (
  IN IP6_INTERFACE     *IpIf,
  IN IP6_ADDRESS_INFO  *AddressInfo,
  IN IP6_DAD_CALLBACK  Callback  OPTIONAL,
  IN VOID              *Context  OPTIONAL
  );

/**
  Search IP6_DAD_ENTRY from the Duplicate Address Detection List.

  @param[in]  IpSb          The pointer to the IP6_SERVICE instance.
  @param[in]  Target        The address information which needs DAD performed .
  @param[out] Interface     If not NULL, output the IP6 interface that configures
                            the tentative address.

  @return NULL if failed to find the matching DAD entry.
          Otherwise, point to the found DAD entry.

**/
IP6_DAD_ENTRY *
Ip6FindDADEntry (
  IN  IP6_SERVICE       *IpSb,
  IN  EFI_IPv6_ADDRESS  *Target,
  OUT IP6_INTERFACE     **Interface OPTIONAL
  );

/**
  Allocate and initialize a IP6 prefix list entry.

  @param[in]  IpSb              The pointer to IP6_SERVICE instance.
  @param[in]  OnLinkOrAuto      If TRUE, the entry is created for the on link prefix list.
                                Otherwise, it is created for the autoconfiguration prefix list.
  @param[in]  ValidLifetime     The length of time in seconds that the prefix
                                is valid for the purpose of on-link determination.
  @param[in]  PreferredLifetime The length of time in seconds that addresses
                                generated from the prefix via stateless address
                                autoconfiguration remain preferred.
  @param[in]  PrefixLength      The prefix length of the Prefix.
  @param[in]  Prefix            The prefix address.

  @return NULL if it failed to allocate memory for the prefix node. Otherwise, point
          to the created or existing prefix list entry.

**/
IP6_PREFIX_LIST_ENTRY *
Ip6CreatePrefixListEntry (
  IN IP6_SERVICE       *IpSb,
  IN BOOLEAN           OnLinkOrAuto,
  IN UINT32            ValidLifetime,
  IN UINT32            PreferredLifetime,
  IN UINT8             PrefixLength,
  IN EFI_IPv6_ADDRESS  *Prefix
  );

/**
  Destroy a IP6 prefix list entry.

  @param[in]  IpSb              The pointer to IP6_SERVICE instance.
  @param[in]  PrefixEntry       The to be destroyed prefix list entry.
  @param[in]  OnLinkOrAuto      If TRUE, the entry is removed from on link prefix list.
                                Otherwise remove from autoconfiguration prefix list.
  @param[in]  ImmediateDelete   If TRUE, remove the entry directly.
                                Otherwise, check the reference count to see whether
                                it should be removed.

**/
VOID
Ip6DestroyPrefixListEntry (
  IN IP6_SERVICE            *IpSb,
  IN IP6_PREFIX_LIST_ENTRY  *PrefixEntry,
  IN BOOLEAN                OnLinkOrAuto,
  IN BOOLEAN                ImmediateDelete
  );

/**
  Search the list array to find an IP6 prefix list entry.

  @param[in]  IpSb              The pointer to IP6_SERVICE instance.
  @param[in]  OnLinkOrAuto      If TRUE, the search the link prefix list,
                                Otherwise search the autoconfiguration prefix list.
  @param[in]  PrefixLength      The prefix length of the Prefix
  @param[in]  Prefix            The prefix address.

  @return NULL if cannot find the IP6 prefix list entry. Otherwise, return the
          pointer to the IP6 prefix list entry.

**/
IP6_PREFIX_LIST_ENTRY *
Ip6FindPrefixListEntry (
  IN IP6_SERVICE       *IpSb,
  IN BOOLEAN           OnLinkOrAuto,
  IN UINT8             PrefixLength,
  IN EFI_IPv6_ADDRESS  *Prefix
  );

/**
  Release the resource in prefix list table, and destroy the list entry and
  corresponding addresses or route entries.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  ListHead          The list entry head of the prefix list table.

**/
VOID
Ip6CleanPrefixListTable (
  IN IP6_SERVICE  *IpSb,
  IN LIST_ENTRY   *ListHead
  );

/**
  Allocate and initialize an IP6 neighbor cache entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  CallBack          The callback function to be called when
                                address resolution is finished.
  @param[in]  Ip6Address        Points to the IPv6 address of the neighbor.
  @param[in]  LinkAddress       Points to the MAC address of the neighbor.
                                Ignored if NULL.

  @return NULL if failed to allocate memory for the neighbor cache entry.
          Otherwise, point to the created neighbor cache entry.

**/
IP6_NEIGHBOR_ENTRY *
Ip6CreateNeighborEntry (
  IN IP6_SERVICE       *IpSb,
  IN IP6_ARP_CALLBACK  CallBack,
  IN EFI_IPv6_ADDRESS  *Ip6Address,
  IN EFI_MAC_ADDRESS   *LinkAddress OPTIONAL
  );

/**
  Search a IP6 neighbor cache entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  Ip6Address        Points to the IPv6 address of the neighbor.

  @return NULL if it failed to find the matching neighbor cache entry.
          Otherwise, point to the found neighbor cache entry.

**/
IP6_NEIGHBOR_ENTRY *
Ip6FindNeighborEntry (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *Ip6Address
  );

/**
  Free a IP6 neighbor cache entry and remove all the frames on the address
  resolution queue that pass the FrameToCancel. That is, either FrameToCancel
  is NULL, or it returns true for the frame.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  NeighborCache     The to be free neighbor cache entry.
  @param[in]  SendIcmpError     If TRUE, send out ICMP error.
  @param[in]  FullFree          If TRUE, remove the neighbor cache entry.
                                Otherwise remove the pending frames.
  @param[in]  IoStatus          The status returned to the cancelled frames'
                                callback function.
  @param[in]  FrameToCancel     Function to select which frame to cancel.
                                This is an optional parameter that may be NULL.
  @param[in]  Context           Opaque parameter to the FrameToCancel.
                                Ignored if FrameToCancel is NULL.

  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
  @retval EFI_SUCCESS           The operation finished successfully.

**/
EFI_STATUS
Ip6FreeNeighborEntry (
  IN IP6_SERVICE          *IpSb,
  IN IP6_NEIGHBOR_ENTRY   *NeighborCache,
  IN BOOLEAN              SendIcmpError,
  IN BOOLEAN              FullFree,
  IN EFI_STATUS           IoStatus,
  IN IP6_FRAME_TO_CANCEL  FrameToCancel OPTIONAL,
  IN VOID                 *Context      OPTIONAL
  );

/**
  Add Neighbor cache entries. It is a work function for EfiIp6Neighbors().

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  TargetIp6Address   Pointer to Target IPv6 address.
  @param[in]  TargetLinkAddress  Pointer to link-layer address of the target. Ignored if NULL.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain in the neighbor
                                 cache. It will be deleted after Timeout. A value of zero means that
                                 the entry is permanent. A non-zero value means that the entry is
                                 dynamic.
  @param[in]  Override           If TRUE, the cached link-layer address of the matching entry will
                                 be overridden and updated; if FALSE, and if a
                                 corresponding cache entry already existed, EFI_ACCESS_DENIED
                                 will be returned.

  @retval  EFI_SUCCESS           The neighbor cache entry has been added.
  @retval  EFI_OUT_OF_RESOURCES  Could not add the entry to the neighbor cache
                                 due to insufficient resources.
  @retval  EFI_NOT_FOUND         TargetLinkAddress is NULL.
  @retval  EFI_ACCESS_DENIED     The to-be-added entry is already defined in the neighbor cache,
                                 and that entry is tagged as un-overridden (when DeleteFlag
                                 is FALSE).

**/
EFI_STATUS
Ip6AddNeighbor (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *TargetIp6Address,
  IN EFI_MAC_ADDRESS   *TargetLinkAddress OPTIONAL,
  IN UINT32            Timeout,
  IN BOOLEAN           Override
  );

/**
  Delete or update Neighbor cache entries. It is a work function for EfiIp6Neighbors().

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  TargetIp6Address   Pointer to Target IPv6 address.
  @param[in]  TargetLinkAddress  Pointer to link-layer address of the target. Ignored if NULL.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain in the neighbor
                                 cache. It will be deleted after Timeout. A value of zero means that
                                 the entry is permanent. A non-zero value means that the entry is
                                 dynamic.
  @param[in]  Override           If TRUE, the cached link-layer address of the matching entry will
                                 be overridden and updated; if FALSE, and if a
                                 corresponding cache entry already existed, EFI_ACCESS_DENIED
                                 will be returned.

  @retval  EFI_SUCCESS           The neighbor cache entry has been updated or deleted.
  @retval  EFI_NOT_FOUND         This entry is not in the neighbor cache.

**/
EFI_STATUS
Ip6DelNeighbor (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *TargetIp6Address,
  IN EFI_MAC_ADDRESS   *TargetLinkAddress OPTIONAL,
  IN UINT32            Timeout,
  IN BOOLEAN           Override
  );

/**
  Process the Neighbor Solicitation message. The message may be sent for Duplicate
  Address Detection or Address Resolution.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the message.
  @param[in]  Packet             The content of the message with IP head removed.

  @retval EFI_SUCCESS            The packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval EFI_ICMP_ERROR         The packet indicates that DAD is failed.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessNeighborSolicit (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  );

/**
  Process the Neighbor Advertisement message.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the message.
  @param[in]  Packet             The content of the message with IP head removed.

  @retval EFI_SUCCESS            The packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval EFI_ICMP_ERROR         The packet indicates that DAD is failed.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessNeighborAdvertise (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  );

/**
  Process the Router Advertisement message according to RFC4861.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the message.
  @param[in]  Packet             The content of the message with the IP head removed.

  @retval EFI_SUCCESS            The packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the operation.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessRouterAdvertise (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  );

/**
  Process the ICMPv6 redirect message. Find the instance, then update
  its route cache.

  @param[in]  IpSb               The IP6 service binding instance that received
                                 the packet.
  @param[in]  Head               The IP head of the received ICMPv6 packet.
  @param[in]  Packet             The content of the ICMPv6 redirect packet with
                                 the IP head removed.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            Successfully updated the route caches.

**/
EFI_STATUS
Ip6ProcessRedirect (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  );

/**
  Generate router solicit message and send it out to Destination Address or
  All Router Link Local scope multicast address.

  @param[in]  IpSb               The IP service to send the packet.
  @param[in]  Interface          If not NULL, points to the IP6 interface to send
                                 the packet.
  @param[in]  SourceAddress      If not NULL, the source address of the message.
  @param[in]  DestinationAddress If not NULL, the destination address of the message.
  @param[in]  SourceLinkAddress  If not NULL, the MAC address of the source.
                                 A source link-layer address option will be appended
                                 to the message.

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the operation.
  @retval EFI_SUCCESS            The router solicit message was successfully sent.

**/
EFI_STATUS
Ip6SendRouterSolicit (
  IN IP6_SERVICE       *IpSb,
  IN IP6_INTERFACE     *Interface          OPTIONAL,
  IN EFI_IPv6_ADDRESS  *SourceAddress      OPTIONAL,
  IN EFI_IPv6_ADDRESS  *DestinationAddress OPTIONAL,
  IN EFI_MAC_ADDRESS   *SourceLinkAddress  OPTIONAL
  );

/**
  Generate the Neighbor Solicitation message and send it to the Destination Address.

  @param[in]  IpSb               The IP service to send the packet
  @param[in]  SourceAddress      The source address of the message.
  @param[in]  DestinationAddress The destination address of the message.
  @param[in]  TargetIp6Address   The IP address of the target of the solicitation.
                                 It must not be a multicast address.
  @param[in]  SourceLinkAddress  The MAC address for the sender. If not NULL,
                                 a source link-layer address option will be appended
                                 to the message.

  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            The Neighbor Advertise message was successfully sent.

**/
EFI_STATUS
Ip6SendNeighborSolicit (
  IN IP6_SERVICE       *IpSb,
  IN EFI_IPv6_ADDRESS  *SourceAddress,
  IN EFI_IPv6_ADDRESS  *DestinationAddress,
  IN EFI_IPv6_ADDRESS  *TargetIp6Address,
  IN EFI_MAC_ADDRESS   *SourceLinkAddress OPTIONAL
  );

/**
  Set the interface's address. This will trigger the DAD process for the
  address to set. To set an already set address, the lifetimes wil be
  updated to the new value passed in.

  @param[in]  Interface             The interface to set the address.
  @param[in]  Ip6Addr               The interface's to be assigned IPv6 address.
  @param[in]  IsAnycast             If TRUE, the unicast IPv6 address is anycast.
                                    Otherwise, it is not anycast.
  @param[in]  PrefixLength          The prefix length of the Ip6Addr.
  @param[in]  ValidLifetime         The valid lifetime for this address.
  @param[in]  PreferredLifetime     The preferred lifetime for this address.
  @param[in]  DadCallback           The caller's callback to trigger when DAD finishes.
                                    This is an optional parameter that may be NULL.
  @param[in]  Context               The context that will be passed to DadCallback.
                                    This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS               The interface is scheduled to be configured with
                                    the specified address.
  @retval EFI_OUT_OF_RESOURCES      Failed to set the interface's address due to
                                    lack of resources.

**/
EFI_STATUS
Ip6SetAddress (
  IN IP6_INTERFACE     *Interface,
  IN EFI_IPv6_ADDRESS  *Ip6Addr,
  IN BOOLEAN           IsAnycast,
  IN UINT8             PrefixLength,
  IN UINT32            ValidLifetime,
  IN UINT32            PreferredLifetime,
  IN IP6_DAD_CALLBACK  DadCallback  OPTIONAL,
  IN VOID              *Context     OPTIONAL
  );

/**
  The heartbeat timer of ND module in IP6_TIMER_INTERVAL_IN_MS milliseconds.
  This time routine handles DAD module and neighbor state transition.
  It is also responsible for sending out router solicitations.

  @param[in]  Event                 The IP6 service instance's heartbeat timer.
  @param[in]  Context               The IP6 service instance.

**/
VOID
EFIAPI
Ip6NdFasterTimerTicking (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  The heartbeat timer of ND module in 1 second. This time routine handles following
  things: 1) maintain default router list; 2) maintain prefix options;
  3) maintain route caches.

  @param[in]  IpSb              The IP6 service binding instance.

**/
VOID
Ip6NdTimerTicking (
  IN IP6_SERVICE  *IpSb
  );

/**
  Callback function when address resolution is finished. It will cancel
  all the queued frames if the address resolution failed, or transmit them
  if the request succeeded.

  @param[in] Context The context of the callback, a pointer to IP6_NEIGHBOR_ENTRY.

**/
VOID
Ip6OnArpResolved (
  IN VOID  *Context
  );

/**
  Update the ReachableTime in IP6 service binding instance data, in milliseconds.

  @param[in, out] IpSb     Points to the IP6_SERVICE.

**/
VOID
Ip6UpdateReachableTime (
  IN OUT IP6_SERVICE  *IpSb
  );

#endif
