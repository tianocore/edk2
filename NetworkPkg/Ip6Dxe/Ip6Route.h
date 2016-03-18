/** @file
  EFI IP6 route table and route cache table defintions.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_ROUTE_H__
#define __EFI_IP6_ROUTE_H__

#define IP6_DIRECT_ROUTE          0x00000001
#define IP6_PACKET_TOO_BIG        0x00000010

#define IP6_ROUTE_CACHE_HASH_SIZE 31
///
/// Max NO. of cache entry per hash bucket
///
#define IP6_ROUTE_CACHE_MAX       32

#define IP6_ROUTE_CACHE_HASH(Ip1, Ip2) Ip6RouteCacheHash ((Ip1), (Ip2))

typedef struct {
  LIST_ENTRY                Link;
  INTN                      RefCnt;
  UINT32                    Flag;
  UINT8                     PrefixLength;
  EFI_IPv6_ADDRESS          Destination;
  EFI_IPv6_ADDRESS          NextHop;
} IP6_ROUTE_ENTRY;

typedef struct {
  LIST_ENTRY                Link;
  INTN                      RefCnt;
  UINTN                     Tag;
  EFI_IPv6_ADDRESS          Destination;
  EFI_IPv6_ADDRESS          Source;
  EFI_IPv6_ADDRESS          NextHop;
} IP6_ROUTE_CACHE_ENTRY;

typedef struct {
  LIST_ENTRY                CacheBucket[IP6_ROUTE_CACHE_HASH_SIZE];
  UINT8                     CacheNum[IP6_ROUTE_CACHE_HASH_SIZE];
} IP6_ROUTE_CACHE;

//
// Each IP6 instance has its own route table. Each ServiceBinding
// instance has a default route table and default address.
//
// All the route table entries with the same prefix length are linked
// together in one route area. For example, RouteArea[0] contains
// the default routes. A route table also contains a route cache.
//

typedef struct _IP6_ROUTE_TABLE {
  INTN                      RefCnt;
  UINT32                    TotalNum;
  LIST_ENTRY                RouteArea[IP6_PREFIX_NUM];
  IP6_ROUTE_CACHE           Cache;
} IP6_ROUTE_TABLE;

/**
  This is the worker function for IP6_ROUTE_CACHE_HASH(). It calculates the value
  as the index of the route cache bucket according to the prefix of two IPv6 addresses.

  @param[in]  Ip1     The IPv6 address.
  @param[in]  Ip2     The IPv6 address.

  @return The hash value of the prefix of two IPv6 addresses.

**/
UINT32
Ip6RouteCacheHash (
  IN EFI_IPv6_ADDRESS       *Ip1,
  IN EFI_IPv6_ADDRESS       *Ip2
  );

/**
  Allocate and initialize an IP6 route cache entry.

  @param[in]  Dst           The destination address.
  @param[in]  Src           The source address.
  @param[in]  GateWay       The next hop address.
  @param[in]  Tag           The tag from the caller. This marks all the cache entries
                            spawned from one route table entry.

  @return NULL if it failed to allocate memory for the cache. Otherwise, point
          to the created route cache entry.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6CreateRouteCacheEntry (
  IN EFI_IPv6_ADDRESS       *Dst,
  IN EFI_IPv6_ADDRESS       *Src,
  IN EFI_IPv6_ADDRESS       *GateWay,
  IN UINTN                  Tag
  );

/**
  Free the route cache entry. It is reference counted.

  @param[in, out]  RtCacheEntry  The route cache entry to free.

**/
VOID
Ip6FreeRouteCacheEntry (
  IN OUT IP6_ROUTE_CACHE_ENTRY  *RtCacheEntry
  );

/**
  Find a route cache with the destination and source address. This is
  used by the ICMPv6 redirect messasge process.

  @param[in]  RtTable       The route table to search the cache for.
  @param[in]  Dest          The destination address.
  @param[in]  Src           The source address.

  @return NULL if no route entry to the (Dest, Src). Otherwise, point
          to the correct route cache entry.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6FindRouteCache (
  IN IP6_ROUTE_TABLE        *RtTable,
  IN EFI_IPv6_ADDRESS       *Dest,
  IN EFI_IPv6_ADDRESS       *Src
  );

/**
  Build a array of EFI_IP6_ROUTE_TABLE to be returned to the caller. The number
  of EFI_IP6_ROUTE_TABLE is also returned.

  @param[in]  RouteTable        The pointer of IP6_ROUTE_TABLE internal used.
  @param[out] EfiRouteCount     The number of returned route entries.
  @param[out] EfiRouteTable     The pointer to the array of EFI_IP6_ROUTE_TABLE.
                                If NULL, only the route entry count is returned.

  @retval EFI_SUCCESS           The EFI_IP6_ROUTE_TABLE successfully built.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the route table.

**/
EFI_STATUS
Ip6BuildEfiRouteTable (
  IN IP6_ROUTE_TABLE        *RouteTable,
  OUT UINT32                *EfiRouteCount,
  OUT EFI_IP6_ROUTE_TABLE   **EfiRouteTable OPTIONAL
  );

/**
  Create an empty route table, includes its internal route cache.

  @return NULL if failed to allocate memory for the route table. Otherwise,
          the point to newly created route table.

**/
IP6_ROUTE_TABLE *
Ip6CreateRouteTable (
  VOID
  );

/**
  Free the route table and its associated route cache. Route
  table is reference counted.

  @param[in, out]  RtTable      The route table to free.

**/
VOID
Ip6CleanRouteTable (
  IN OUT IP6_ROUTE_TABLE        *RtTable
  );

/**
  Allocate a route entry then initialize it with the Destination/PrefixLength
  and Gateway.

  @param[in]  Destination     The IPv6 destination address. This is an optional
                              parameter that may be NULL.
  @param[in]  PrefixLength    The destination network's prefix length.
  @param[in]  GatewayAddress  The next hop address. This is optional parameter
                              that may be NULL.

  @return NULL if it failed to allocate memeory. Otherwise, the newly created route entry.

**/
IP6_ROUTE_ENTRY *
Ip6CreateRouteEntry (
  IN EFI_IPv6_ADDRESS       *Destination    OPTIONAL,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *GatewayAddress OPTIONAL
  );

/**
  Search the route table for a most specific match to the Dst. It searches
  from the longest route area (prefix length == 128) to the shortest route area
  (default routes). In each route area, it will first search the instance's
  route table, then the default route table. This is required per the following
  requirements:
  1. IP search the route table for a most specific match.
  2. The local route entries have precedence over the default route entry.

  @param[in]  RtTable       The route table to search from.
  @param[in]  Destination   The destionation address to search. If NULL, search
                            the route table by NextHop.
  @param[in]  NextHop       The next hop address. If NULL, search the route table
                            by Destination.

  @return NULL if no route matches the Dst. Otherwise the point to the
          most specific route to the Dst.

**/
IP6_ROUTE_ENTRY *
Ip6FindRouteEntry (
  IN IP6_ROUTE_TABLE        *RtTable,
  IN EFI_IPv6_ADDRESS       *Destination OPTIONAL,
  IN EFI_IPv6_ADDRESS       *NextHop     OPTIONAL
  );

/**
  Free the route table entry. It is reference counted.

  @param[in, out]  RtEntry  The route entry to free.

**/
VOID
Ip6FreeRouteEntry (
  IN OUT IP6_ROUTE_ENTRY    *RtEntry
  );

/**
  Add a route entry to the route table. It is the help function for EfiIp6Routes.

  @param[in, out]  RtTable        Route table to add route to.
  @param[in]       Destination    The destination of the network.
  @param[in]       PrefixLength   The PrefixLength of the destination.
  @param[in]       GatewayAddress The next hop address.

  @retval EFI_ACCESS_DENIED     The same route already exists.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the entry.
  @retval EFI_SUCCESS           The route was added successfully.

**/
EFI_STATUS
Ip6AddRoute (
  IN OUT IP6_ROUTE_TABLE    *RtTable,
  IN EFI_IPv6_ADDRESS       *Destination,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *GatewayAddress
  );

/**
  Remove a route entry and all the route caches spawn from it.
  It is the help function for EfiIp6Routes.

  @param[in, out] RtTable           The route table to remove the route from.
  @param[in]      Destination       The destination network.
  @param[in]      PrefixLength      The PrefixLength of the Destination.
  @param[in]      GatewayAddress    The next hop address.

  @retval EFI_SUCCESS           Successfully removed the route entry.
  @retval EFI_NOT_FOUND         There is no route entry in the table with that
                                properity.

**/
EFI_STATUS
Ip6DelRoute (
  IN OUT IP6_ROUTE_TABLE    *RtTable,
  IN EFI_IPv6_ADDRESS       *Destination,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *GatewayAddress
  );

/**
  Search the route table to route the packet. Return/create a route
  cache if there is a route to the destination.

  @param[in]  IpSb          The IP6 service data.
  @param[in]  Dest          The destination address to search for.
  @param[in]  Src           The source address to search for.

  @return NULL if failed to route packet. Otherwise, a route cache
          entry that can be used to route packet.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6Route (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Dest,
  IN EFI_IPv6_ADDRESS       *Src
  );

#endif
