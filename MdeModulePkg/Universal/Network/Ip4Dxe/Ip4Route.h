/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4Route.h

Abstract:

  EFI IP4 route table and route cache table defintions.


**/

#ifndef __EFI_IP4_ROUTE_H__
#define __EFI_IP4_ROUTE_H__

#include "Ip4Common.h"

typedef enum {
  IP4_DIRECT_ROUTE      = 0x00000001,

  IP4_ROUTE_CACHE_HASH  = 31,
  IP4_ROUTE_CACHE_MAX   = 64  // Max NO. of cache entry per hash bucket
} IP4_ROUTE_ENUM_TYPES;

#define IP4_ROUTE_CACHE_HASH(Dst, Src)  (((Dst) ^ (Src)) % IP4_ROUTE_CACHE_HASH)

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
  LIST_ENTRY                CacheBucket[IP4_ROUTE_CACHE_HASH];
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

IP4_ROUTE_TABLE*
Ip4CreateRouteTable (
  VOID
  );

VOID
Ip4FreeRouteTable (
  IN IP4_ROUTE_TABLE        *RouteTable
  );

EFI_STATUS
Ip4AddRoute (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Netmask,
  IN IP4_ADDR               Gateway
  );

EFI_STATUS
Ip4DelRoute (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Netmask,
  IN IP4_ADDR               Gateway
  );

IP4_ROUTE_CACHE_ENTRY *
Ip4FindRouteCache (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Src
  );

VOID
Ip4FreeRouteCacheEntry (
  IN IP4_ROUTE_CACHE_ENTRY  *RtCacheEntry
  );

IP4_ROUTE_CACHE_ENTRY *
Ip4Route (
  IN IP4_ROUTE_TABLE        *RtTable,
  IN IP4_ADDR               Dest,
  IN IP4_ADDR               Src
  );

EFI_STATUS
Ip4BuildEfiRouteTable (
  IN IP4_PROTOCOL           *IpInstance
  );
#endif
