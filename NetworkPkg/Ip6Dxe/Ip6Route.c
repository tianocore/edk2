/** @file
  The functions and routines to handle the route caches and route table.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip6Impl.h"

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
  )
{
  UINT32 Prefix1;
  UINT32 Prefix2;

  Prefix1 = *((UINT32 *) ((UINTN *) (Ip1)));
  Prefix2 = *((UINT32 *) ((UINTN *) (Ip2)));

  return ((UINT32) (Prefix1 ^ Prefix2) % IP6_ROUTE_CACHE_HASH_SIZE);
}

/**
  Allocate a route entry then initialize it with the Destination/PrefixLength
  and Gateway.

  @param[in]  Destination     The IPv6 destination address. This is an optional
                              parameter that may be NULL.
  @param[in]  PrefixLength    The destination network's prefix length.
  @param[in]  GatewayAddress  The next hop address. This is an optional parameter
                              that may be NULL.

  @return NULL if failed to allocate memeory; otherwise, the newly created route entry.

**/
IP6_ROUTE_ENTRY *
Ip6CreateRouteEntry (
  IN EFI_IPv6_ADDRESS       *Destination    OPTIONAL,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *GatewayAddress OPTIONAL
  )
{
  IP6_ROUTE_ENTRY           *RtEntry;

  RtEntry = AllocateZeroPool (sizeof (IP6_ROUTE_ENTRY));

  if (RtEntry == NULL) {
    return NULL;
  }

  RtEntry->RefCnt       = 1;
  RtEntry->Flag         = 0;
  RtEntry->PrefixLength = PrefixLength;

  if (Destination != NULL) {
    IP6_COPY_ADDRESS (&RtEntry->Destination, Destination);
  }

  if (GatewayAddress != NULL) {
    IP6_COPY_ADDRESS (&RtEntry->NextHop, GatewayAddress);
  }

  return RtEntry;
}

/**
  Free the route table entry. It is reference counted.

  @param[in, out]  RtEntry  The route entry to free.

**/
VOID
Ip6FreeRouteEntry (
  IN OUT IP6_ROUTE_ENTRY    *RtEntry
  )
{
  ASSERT ((RtEntry != NULL) && (RtEntry->RefCnt > 0));

  if (--RtEntry->RefCnt == 0) {
    FreePool (RtEntry);
  }
}

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

  @return NULL if no route matches the Dst. Otherwise, the point to the
  @return most specific route to the Dst.

**/
IP6_ROUTE_ENTRY *
Ip6FindRouteEntry (
  IN IP6_ROUTE_TABLE        *RtTable,
  IN EFI_IPv6_ADDRESS       *Destination OPTIONAL,
  IN EFI_IPv6_ADDRESS       *NextHop     OPTIONAL
  )
{
  LIST_ENTRY                *Entry;
  IP6_ROUTE_ENTRY           *RtEntry;
  INTN                      Index;

  ASSERT (Destination != NULL || NextHop != NULL);

  RtEntry = NULL;

  for (Index = IP6_PREFIX_NUM - 1; Index >= 0; Index--) {
    NET_LIST_FOR_EACH (Entry, &RtTable->RouteArea[Index]) {
      RtEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_ENTRY, Link);

      if (Destination != NULL) {
        if (NetIp6IsNetEqual (Destination, &RtEntry->Destination, RtEntry->PrefixLength)) {
          NET_GET_REF (RtEntry);
          return RtEntry;
        }
      } else if (NextHop != NULL) {
        if (NetIp6IsNetEqual (NextHop, &RtEntry->NextHop, RtEntry->PrefixLength)) {
          NET_GET_REF (RtEntry);
          return RtEntry;
        }
      }

    }
  }

  return NULL;
}

/**
  Allocate and initialize a IP6 route cache entry.

  @param[in]  Dst           The destination address.
  @param[in]  Src           The source address.
  @param[in]  GateWay       The next hop address.
  @param[in]  Tag           The tag from the caller. This marks all the cache entries
                            spawned from one route table entry.

  @return NULL if failed to allocate memory for the cache. Otherwise, point
          to the created route cache entry.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6CreateRouteCacheEntry (
  IN EFI_IPv6_ADDRESS       *Dst,
  IN EFI_IPv6_ADDRESS       *Src,
  IN EFI_IPv6_ADDRESS       *GateWay,
  IN UINTN                  Tag
  )
{
  IP6_ROUTE_CACHE_ENTRY     *RtCacheEntry;

  RtCacheEntry = AllocatePool (sizeof (IP6_ROUTE_CACHE_ENTRY));

  if (RtCacheEntry == NULL) {
    return NULL;
  }

  RtCacheEntry->RefCnt = 1;
  RtCacheEntry->Tag    = Tag;

  IP6_COPY_ADDRESS (&RtCacheEntry->Destination, Dst);
  IP6_COPY_ADDRESS (&RtCacheEntry->Source, Src);
  IP6_COPY_ADDRESS (&RtCacheEntry->NextHop, GateWay);

  return RtCacheEntry;
}

/**
  Free the route cache entry. It is reference counted.

  @param[in, out]  RtCacheEntry  The route cache entry to free.

**/
VOID
Ip6FreeRouteCacheEntry (
  IN OUT IP6_ROUTE_CACHE_ENTRY  *RtCacheEntry
  )
{
  ASSERT (RtCacheEntry->RefCnt > 0);

  if (--RtCacheEntry->RefCnt == 0) {
    FreePool (RtCacheEntry);
  }
}

/**
  Find a route cache with the destination and source address. This is
  used by the ICMPv6 redirect messasge process.

  @param[in]  RtTable       The route table to search the cache for.
  @param[in]  Dest          The destination address.
  @param[in]  Src           The source address.

  @return NULL if no route entry to the (Dest, Src). Otherwise, the pointer
          to the correct route cache entry.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6FindRouteCache (
  IN IP6_ROUTE_TABLE        *RtTable,
  IN EFI_IPv6_ADDRESS       *Dest,
  IN EFI_IPv6_ADDRESS       *Src
  )
{
  LIST_ENTRY                *Entry;
  IP6_ROUTE_CACHE_ENTRY     *RtCacheEntry;
  UINT32                    Index;

  Index = IP6_ROUTE_CACHE_HASH (Dest, Src);

  NET_LIST_FOR_EACH (Entry, &RtTable->Cache.CacheBucket[Index]) {
    RtCacheEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_CACHE_ENTRY, Link);

    if (EFI_IP6_EQUAL (Dest, &RtCacheEntry->Destination)&& EFI_IP6_EQUAL (Src, &RtCacheEntry->Source)) {
      NET_GET_REF (RtCacheEntry);
      return RtCacheEntry;
    }
  }

  return NULL;
}

/**
  Build an array of EFI_IP6_ROUTE_TABLE to be returned to the caller. The number
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
  )
{
  LIST_ENTRY                *Entry;
  IP6_ROUTE_ENTRY           *RtEntry;
  EFI_IP6_ROUTE_TABLE       *EfiTable;
  UINT32                    Count;
  INT32                     Index;

  ASSERT (EfiRouteCount != NULL);

  Count          = RouteTable->TotalNum;
  *EfiRouteCount = Count;

  if ((EfiRouteTable == NULL) || (Count == 0)) {
    return EFI_SUCCESS;
  }

  if (*EfiRouteTable == NULL) {
    *EfiRouteTable = AllocatePool (sizeof (EFI_IP6_ROUTE_TABLE) * Count);
    if (*EfiRouteTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  EfiTable = *EfiRouteTable;

  //
  // Copy the route entry to EFI route table.
  //
  Count = 0;

  for (Index = IP6_PREFIX_NUM - 1; Index >= 0; Index--) {

    NET_LIST_FOR_EACH (Entry, &(RouteTable->RouteArea[Index])) {
      RtEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_ENTRY, Link);

      Ip6CopyAddressByPrefix (
        &EfiTable[Count].Destination,
        &RtEntry->Destination,
        RtEntry->PrefixLength
        );

      IP6_COPY_ADDRESS (&EfiTable[Count].Gateway, &RtEntry->NextHop);
      EfiTable[Count].PrefixLength = RtEntry->PrefixLength;

      Count++;
    }
  }

  ASSERT (Count == RouteTable->TotalNum);

  return EFI_SUCCESS;
}

/**
  Create an empty route table. This includes its internal route cache.

  @return NULL if failed to allocate memory for the route table. Otherwise,
          the point to newly created route table.

**/
IP6_ROUTE_TABLE *
Ip6CreateRouteTable (
  VOID
  )
{
  IP6_ROUTE_TABLE           *RtTable;
  UINT32                    Index;

  RtTable = AllocatePool (sizeof (IP6_ROUTE_TABLE));
  if (RtTable == NULL) {
    return NULL;
  }

  RtTable->RefCnt   = 1;
  RtTable->TotalNum = 0;

  for (Index = 0; Index < IP6_PREFIX_NUM; Index++) {
    InitializeListHead (&RtTable->RouteArea[Index]);
  }

  for (Index = 0; Index < IP6_ROUTE_CACHE_HASH_SIZE; Index++) {
    InitializeListHead (&RtTable->Cache.CacheBucket[Index]);
    RtTable->Cache.CacheNum[Index] = 0;
  }

  return RtTable;
}

/**
  Free the route table and its associated route cache. Route
  table is reference counted.

  @param[in, out]  RtTable      The route table to free.

**/
VOID
Ip6CleanRouteTable (
  IN OUT IP6_ROUTE_TABLE        *RtTable
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_ROUTE_ENTRY           *RtEntry;
  IP6_ROUTE_CACHE_ENTRY     *RtCacheEntry;
  UINT32                    Index;

  ASSERT (RtTable->RefCnt > 0);

  if (--RtTable->RefCnt > 0) {
    return ;
  }

  //
  // Free all the route table entry and its route cache.
  //
  for (Index = 0; Index < IP6_PREFIX_NUM; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &RtTable->RouteArea[Index]) {
      RtEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_ENTRY, Link);
      RemoveEntryList (Entry);
      Ip6FreeRouteEntry (RtEntry);
    }
  }

  for (Index = 0; Index < IP6_ROUTE_CACHE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &RtTable->Cache.CacheBucket[Index]) {
      RtCacheEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_CACHE_ENTRY, Link);
      RemoveEntryList (Entry);
      Ip6FreeRouteCacheEntry (RtCacheEntry);
    }
  }

  FreePool (RtTable);
}

/**
  Remove all the cache entries bearing the Tag. When a route cache
  entry is created, it is tagged with the address of route entry
  from which it is spawned. When a route entry is deleted, the cache
  entries spawned from it are also deleted.

  @param[in]  RtCache       Route cache to remove the entries from.
  @param[in]  Tag           The Tag of the entries to remove.

**/
VOID
Ip6PurgeRouteCache (
  IN IP6_ROUTE_CACHE        *RtCache,
  IN UINTN                  Tag
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_ROUTE_CACHE_ENTRY     *RtCacheEntry;
  UINT32                    Index;

  for (Index = 0; Index < IP6_ROUTE_CACHE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &RtCache->CacheBucket[Index]) {

      RtCacheEntry = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_CACHE_ENTRY, Link);

      if (RtCacheEntry->Tag == Tag) {
        RemoveEntryList (Entry);
        Ip6FreeRouteCacheEntry (RtCacheEntry);
      }
    }
  }
}

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
  )
{
  LIST_ENTRY                *ListHead;
  LIST_ENTRY                *Entry;
  IP6_ROUTE_ENTRY           *Route;

  ListHead = &RtTable->RouteArea[PrefixLength];

  //
  // First check whether the route exists
  //
  NET_LIST_FOR_EACH (Entry, ListHead) {
    Route = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_ENTRY, Link);

    if (NetIp6IsNetEqual (Destination, &Route->Destination, PrefixLength) &&
        EFI_IP6_EQUAL (GatewayAddress, &Route->NextHop)) {
      return EFI_ACCESS_DENIED;
    }
  }

  //
  // Create a route entry and insert it to the route area.
  //
  Route = Ip6CreateRouteEntry (Destination, PrefixLength, GatewayAddress);

  if (Route == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (NetIp6IsUnspecifiedAddr (GatewayAddress)) {
    Route->Flag = IP6_DIRECT_ROUTE;
  }

  InsertHeadList (ListHead, &Route->Link);
  RtTable->TotalNum++;

  return EFI_SUCCESS;
}

/**
  Remove a route entry and all the route caches spawn from it.
  It is the help function for EfiIp6Routes.

  @param[in, out] RtTable           The route table to remove the route from.
  @param[in]      Destination       The destination network.
  @param[in]      PrefixLength      The PrefixLength of the Destination.
  @param[in]      GatewayAddress    The next hop address.

  @retval EFI_SUCCESS           The route entry was successfully removed.
  @retval EFI_NOT_FOUND         There is no route entry in the table with that
                                property.

**/
EFI_STATUS
Ip6DelRoute (
  IN OUT IP6_ROUTE_TABLE    *RtTable,
  IN EFI_IPv6_ADDRESS       *Destination,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *GatewayAddress
  )
{
  LIST_ENTRY                *ListHead;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_ROUTE_ENTRY           *Route;
  UINT32                    TotalNum;

  ListHead = &RtTable->RouteArea[PrefixLength];
  TotalNum = RtTable->TotalNum;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, ListHead) {
    Route = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_ENTRY, Link);

    if (Destination != NULL && !NetIp6IsNetEqual (Destination, &Route->Destination, PrefixLength)) {
      continue;
    }
    if (GatewayAddress != NULL && !EFI_IP6_EQUAL (GatewayAddress, &Route->NextHop)) {
      continue;
    }

    Ip6PurgeRouteCache (&RtTable->Cache, (UINTN) Route);
    RemoveEntryList (Entry);
    Ip6FreeRouteEntry (Route);

    ASSERT (RtTable->TotalNum > 0);
    RtTable->TotalNum--;
  }

  return TotalNum == RtTable->TotalNum ? EFI_NOT_FOUND : EFI_SUCCESS;
}

/**
  Search the route table to route the packet. Return/create a route
  cache if there is a route to the destination.

  @param[in]  IpSb          The IP6 service data.
  @param[in]  Dest          The destination address to search for.
  @param[in]  Src           The source address to search for.

  @return NULL if it failed to route the packet. Otherwise, a route cache
          entry that can be used to route packets.

**/
IP6_ROUTE_CACHE_ENTRY *
Ip6Route (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Dest,
  IN EFI_IPv6_ADDRESS       *Src
  )
{
  IP6_ROUTE_TABLE           *RtTable;
  LIST_ENTRY                *ListHead;
  IP6_ROUTE_CACHE_ENTRY     *RtCacheEntry;
  IP6_ROUTE_ENTRY           *RtEntry;
  EFI_IPv6_ADDRESS          NextHop;
  UINT32                    Index;

  RtTable = IpSb->RouteTable;

  ASSERT (RtTable != NULL);

  //
  // Search the destination cache in IP6_ROUTE_TABLE.
  //
  Index    = IP6_ROUTE_CACHE_HASH (Dest, Src);
  ListHead = &RtTable->Cache.CacheBucket[Index];

  RtCacheEntry = Ip6FindRouteCache (RtTable, Dest, Src);

  //
  // If found, promote the cache entry to the head of the hash bucket.
  //
  if (RtCacheEntry != NULL) {
    RemoveEntryList (&RtCacheEntry->Link);
    InsertHeadList (ListHead, &RtCacheEntry->Link);
    return RtCacheEntry;
  }

  //
  // Search the route table for the most specific route
  //
  RtEntry = Ip6FindRouteEntry (RtTable, Dest, NULL);
  if (RtEntry == NULL) {
    return NULL;
  }

  //
  // Found a route to the Dest, if it is a direct route, the packet
  // will be send directly to the destination, such as for connected
  // network. Otherwise, it is an indirect route, the packet will be
  // send the next hop router.
  //
  if ((RtEntry->Flag & IP6_DIRECT_ROUTE) == IP6_DIRECT_ROUTE) {
    IP6_COPY_ADDRESS (&NextHop, Dest);
  } else {
    IP6_COPY_ADDRESS (&NextHop, &RtEntry->NextHop);
  }

  Ip6FreeRouteEntry (RtEntry);

  //
  // Create a route cache entry, and tag it as spawned from this route entry
  //
  RtCacheEntry = Ip6CreateRouteCacheEntry (Dest, Src, &NextHop, (UINTN) RtEntry);

  if (RtCacheEntry == NULL) {
    return NULL;
  }

  InsertHeadList (ListHead, &RtCacheEntry->Link);
  NET_GET_REF (RtCacheEntry);
  RtTable->Cache.CacheNum[Index]++;

  return RtCacheEntry;
}

