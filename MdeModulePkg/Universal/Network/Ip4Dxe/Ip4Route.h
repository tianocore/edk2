/** @file
  EFI IP4 route table and route cache table defintions.
  
Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP4_ROUTE_H__
#define __EFI_IP4_ROUTE_H__

#include "Ip4Common.h"

#define IP4_DIRECT_ROUTE       0x00000001

#define IP4_ROUTE_CACHE_HASH_VALUE 31
#define IP4_ROUTE_CACHE_MAX        64  // Max NO. of cache entry per hash bucket

#define IP4_ROUTE_CACHE_HASH(Dst, Src)  (((Dst) ^ (Src)) % IP4_ROUTE_CACHE_HASH_VALUE)

///
/// The route entry in the route table. Dest/Netmask is the destion
/// network. The nexthop is the gateway to send the packet to in
/// order to reach the Dest/Netmask. If the Flag has IP4_DIRECT_ROUTE
/// on, the gateway is the destination of the IP packet itself. Route
/// enties of the connected network have the flag on.
///
typedef struct {
  LIST_ENTRY                Link;
  INTN                      RefCnt;
  IP4_ADDR                  Dest;
  IP4_ADDR                  Netmask;
  IP4_ADDR                  NextHop;
  UINT32                    Flag;
} IP4_ROUTE_ENTRY;

///
/// The route cache entry. The route cache entry is optional.
/// But it is necessary to support the ICMP redirect message.
/// Check Ip4ProcessIcmpRedirect for information.
///
/// The cache entry field Tag is used to tag all the route
/// cache entry spawned from a route table entry. This makes
/// it simple to delete all the route cache entries from a
/// to-be-deleted route entry.
///
typedef struct {
  LIST_ENTRY                Link;
  INTN                      RefCnt;
  IP4_ADDR                  Dest;
  IP4_ADDR                  Src;
  IP4_ADDR                  NextHop;
  UINTN                     Tag;
} IP4_ROUTE_CACHE_ENTRY;

///
/// The route cache table is organized as a hash table. Each
/// IP4 route table has a embedded route cache. For now the
/// route cache and route table are binded togehter. But keep
/// the route cache a seperated structure in case we want to
/// detach them later.
///
typedef struct {
  LIST_ENTRY                CacheBucket[IP4_ROUTE_CACHE_HASH_VALUE];
} IP4_ROUTE_CACHE;

///
/// Each IP4 instance has its own route table. Each ServiceBinding
/// instance has a default route table and default address.
///
/// All the route table entries with the same mask are linked
/// together in one route area. For example, RouteArea[0] contains
/// the default routes. A route table also contains a route cache.
///
typedef struct _IP4_ROUTE_TABLE IP4_ROUTE_TABLE;

struct _IP4_ROUTE_TABLE {
  INTN                      RefCnt;
  UINT32                    TotalNum;
  LIST_ENTRY                RouteArea[IP4_MASK_NUM];
  IP4_ROUTE_TABLE           *Next;
  IP4_ROUTE_CACHE           Cache;
};

/**
  Create an empty route table, includes its internal route cache

  @return NULL if failed to allocate memory for the route table, otherwise
          the point to newly created route table.

**/
IP4_ROUTE_TABLE *
Ip4CreateRouteTable (
  VOID
  );

/**
  Free the route table and its associated route cache. Route
  table is reference counted.

  @param[in]  RtTable               The route table to free.

**/
VOID
Ip4FreeRouteTable (
  IN IP4_ROUTE_TABLE        *RtTable
  );

/**
  Add a route entry to the route table. All the IP4_ADDRs are in
  host byte order.

  @param[in, out]  RtTable      Route table to add route to
  @param[in]       Dest         The destination of the network
  @param[in]       Netmask      The netmask of the destination
  @param[in]       Gateway      The next hop address

  @retval EFI_ACCESS_DENIED     The same route already exists
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the entry
  @retval EFI_SUCCESS           The route is added successfully.

**/
EFI_STATUS
Ip4AddRoute (
  IN OUT IP4_ROUTE_TABLE        *RtTable,
  IN     IP4_ADDR               Dest,
  IN     IP4_ADDR               Netmask,
  IN     IP4_ADDR               Gateway
  );

/**
  Remove a route entry and all the route caches spawn from it.

  @param  RtTable           The route table to remove the route from
  @param  Dest              The destination network
  @param  Netmask           The netmask of the Dest
  @param  Gateway           The next hop address

  @retval EFI_SUCCESS           The route entry is successfully removed
  @retval EFI_NOT_FOUND         There is no route entry in the table with that
                                properity.

**/
EFI_STATUS
Ip4DelRoute (
  IN OUT IP4_ROUTE_TABLE      *RtTable,
  IN     IP4_ADDR             Dest,
  IN     IP4_ADDR             Netmask,
  IN     IP4_ADDR             Gateway
  );

/**
  Find a route cache with the dst and src. This is used by ICMP
  redirect messasge process. All kinds of redirect is treated as
  host redirect according to RFC1122. So, only route cache entries
  are modified according to the ICMP redirect message.

  @param[in]  RtTable               The route table to search the cache for
  @param[in]  Dest                  The destination address
  @param[in]  Src                   The source address

  @return NULL if no route entry to the (Dest, Src). Otherwise the point
          to the correct route cache entry.

**/
IP4_ROUTE_CACHE_ENTRY *
Ip4FindRouteCache (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Src
  );

/**
  Free the route cache entry. It is reference counted.

  @param  RtCacheEntry          The route cache entry to free.

**/
VOID
Ip4FreeRouteCacheEntry (
  IN IP4_ROUTE_CACHE_ENTRY  *RtCacheEntry
  );

/**
  Search the route table to route the packet. Return/create a route
  cache if there is a route to the destination.

  @param[in]  RtTable               The route table to search from
  @param[in]  Dest                  The destination address to search for
  @param[in]  Src                   The source address to search for

  @return NULL if failed to route packet, otherwise a route cache
          entry that can be used to route packet.

**/
IP4_ROUTE_CACHE_ENTRY *
Ip4Route (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Src
  );

/**
  Build a EFI_IP4_ROUTE_TABLE to be returned to the caller of
  GetModeData. The EFI_IP4_ROUTE_TABLE is clumsy to use in the
  internal operation of the IP4 driver.

  @param[in]  IpInstance        The IP4 child that requests the route table.

  @retval EFI_SUCCESS           The route table is successfully build
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the rotue table.

**/
EFI_STATUS
Ip4BuildEfiRouteTable (
  IN IP4_PROTOCOL           *IpInstance
  );
#endif
