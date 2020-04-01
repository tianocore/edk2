/** @file
  Implementation of Neighbor Discovery support routines.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

EFI_MAC_ADDRESS mZeroMacAddress;

/**
  Update the ReachableTime in IP6 service binding instance data, in milliseconds.

  @param[in, out] IpSb     Points to the IP6_SERVICE.

**/
VOID
Ip6UpdateReachableTime (
  IN OUT IP6_SERVICE  *IpSb
  )
{
  UINT32              Random;

  Random = (NetRandomInitSeed () / 4294967295UL) * IP6_RANDOM_FACTOR_SCALE;
  Random = Random + IP6_MIN_RANDOM_FACTOR_SCALED;
  IpSb->ReachableTime = (IpSb->BaseReachableTime * Random) / IP6_RANDOM_FACTOR_SCALE;
}

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
  IN IP6_PROTOCOL            *IpInstance,
  OUT UINT32                 *NeighborCount,
  OUT EFI_IP6_NEIGHBOR_CACHE **NeighborCache
  )
{
  IP6_NEIGHBOR_ENTRY        *Neighbor;
  LIST_ENTRY                *Entry;
  IP6_SERVICE               *IpSb;
  UINT32                    Count;
  EFI_IP6_NEIGHBOR_CACHE    *EfiNeighborCache;
  EFI_IP6_NEIGHBOR_CACHE    *NeighborCacheTmp;

  NET_CHECK_SIGNATURE (IpInstance, IP6_PROTOCOL_SIGNATURE);
  ASSERT (NeighborCount != NULL && NeighborCache != NULL);

  IpSb  = IpInstance->Service;
  Count = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->NeighborTable) {
    Count++;
  }

  if (Count == 0) {
    return EFI_SUCCESS;
  }

  NeighborCacheTmp = AllocatePool (Count * sizeof (EFI_IP6_NEIGHBOR_CACHE));
  if (NeighborCacheTmp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *NeighborCount = Count;
  Count          = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->NeighborTable) {
    Neighbor = NET_LIST_USER_STRUCT (Entry, IP6_NEIGHBOR_ENTRY, Link);

    EfiNeighborCache = NeighborCacheTmp + Count;

   EfiNeighborCache->State = Neighbor->State;
    IP6_COPY_ADDRESS (&EfiNeighborCache->Neighbor, &Neighbor->Neighbor);
    IP6_COPY_LINK_ADDRESS (&EfiNeighborCache->LinkAddress, &Neighbor->LinkAddress);

    Count++;
  }

  ASSERT (*NeighborCount == Count);
  *NeighborCache = NeighborCacheTmp;

  return EFI_SUCCESS;
}

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
  )
{
  LIST_ENTRY                *Entry;
  IP6_SERVICE               *IpSb;
  UINT32                    Count;
  IP6_PREFIX_LIST_ENTRY     *PrefixList;
  EFI_IP6_ADDRESS_INFO      *EfiPrefix;
  EFI_IP6_ADDRESS_INFO      *PrefixTableTmp;

  NET_CHECK_SIGNATURE (IpInstance, IP6_PROTOCOL_SIGNATURE);
  ASSERT (PrefixCount != NULL && PrefixTable != NULL);

  IpSb  = IpInstance->Service;
  Count = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->OnlinkPrefix) {
    Count++;
  }

  if (Count == 0) {
    return EFI_SUCCESS;
  }

  PrefixTableTmp = AllocatePool (Count * sizeof (EFI_IP6_ADDRESS_INFO));
  if (PrefixTableTmp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *PrefixCount = Count;
  Count        = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->OnlinkPrefix) {
    PrefixList = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);
    EfiPrefix  = PrefixTableTmp + Count;
    IP6_COPY_ADDRESS (&EfiPrefix->Address, &PrefixList->Prefix);
    EfiPrefix->PrefixLength = PrefixList->PrefixLength;

    Count++;
  }

  ASSERT (*PrefixCount == Count);
  *PrefixTable = PrefixTableTmp;

  return EFI_SUCCESS;
}

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
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                OnLinkOrAuto,
  IN UINT32                 ValidLifetime,
  IN UINT32                 PreferredLifetime,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *Prefix
  )
{
  IP6_PREFIX_LIST_ENTRY     *PrefixEntry;
  IP6_ROUTE_ENTRY           *RtEntry;
  LIST_ENTRY                *ListHead;
  LIST_ENTRY                *Entry;
  IP6_PREFIX_LIST_ENTRY     *TmpPrefixEntry;

  if (Prefix == NULL || PreferredLifetime > ValidLifetime || PrefixLength > IP6_PREFIX_MAX) {
    return NULL;
  }

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  PrefixEntry = Ip6FindPrefixListEntry (
                  IpSb,
                  OnLinkOrAuto,
                  PrefixLength,
                  Prefix
                  );
  if (PrefixEntry != NULL) {
    PrefixEntry->RefCnt ++;
    return PrefixEntry;
  }

  PrefixEntry = AllocatePool (sizeof (IP6_PREFIX_LIST_ENTRY));
  if (PrefixEntry == NULL) {
    return NULL;
  }

  PrefixEntry->RefCnt            = 1;
  PrefixEntry->ValidLifetime     = ValidLifetime;
  PrefixEntry->PreferredLifetime = PreferredLifetime;
  PrefixEntry->PrefixLength      = PrefixLength;
  IP6_COPY_ADDRESS (&PrefixEntry->Prefix, Prefix);

  ListHead = OnLinkOrAuto ? &IpSb->OnlinkPrefix : &IpSb->AutonomousPrefix;

  //
  // Create a direct route entry for on-link prefix and insert to route area.
  //
  if (OnLinkOrAuto) {
    RtEntry = Ip6CreateRouteEntry (Prefix, PrefixLength, NULL);
    if (RtEntry == NULL) {
      FreePool (PrefixEntry);
      return NULL;
    }

    RtEntry->Flag = IP6_DIRECT_ROUTE;
    InsertHeadList (&IpSb->RouteTable->RouteArea[PrefixLength], &RtEntry->Link);
    IpSb->RouteTable->TotalNum++;
  }

  //
  // Insert the prefix entry in the order that a prefix with longer prefix length
  // is put ahead in the list.
  //
  NET_LIST_FOR_EACH (Entry, ListHead) {
    TmpPrefixEntry = NET_LIST_USER_STRUCT(Entry, IP6_PREFIX_LIST_ENTRY, Link);

    if (TmpPrefixEntry->PrefixLength < PrefixEntry->PrefixLength) {
      break;
    }
  }

  NetListInsertBefore (Entry, &PrefixEntry->Link);

  return PrefixEntry;
}

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
  )
{
  LIST_ENTRY      *Entry;
  IP6_INTERFACE   *IpIf;
  EFI_STATUS      Status;

  if ((!ImmediateDelete) && (PrefixEntry->RefCnt > 0) && ((--PrefixEntry->RefCnt) > 0)) {
    return ;
  }

  if (OnLinkOrAuto) {
      //
      // Remove the direct route for onlink prefix from route table.
      //
      do {
        Status = Ip6DelRoute (
                   IpSb->RouteTable,
                   &PrefixEntry->Prefix,
                   PrefixEntry->PrefixLength,
                   NULL
                   );
      } while (Status != EFI_NOT_FOUND);
  } else {
    //
    // Remove the corresponding addresses generated from this autonomous prefix.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
      IpIf = NET_LIST_USER_STRUCT_S (Entry, IP6_INTERFACE, Link, IP6_INTERFACE_SIGNATURE);

      Ip6RemoveAddr (IpSb, &IpIf->AddressList, &IpIf->AddressCount, &PrefixEntry->Prefix, PrefixEntry->PrefixLength);
    }
  }

  RemoveEntryList (&PrefixEntry->Link);
  FreePool (PrefixEntry);
}

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
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                OnLinkOrAuto,
  IN UINT8                  PrefixLength,
  IN EFI_IPv6_ADDRESS       *Prefix
  )
{
  IP6_PREFIX_LIST_ENTRY     *PrefixList;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *ListHead;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Prefix != NULL);

  if (OnLinkOrAuto) {
    ListHead = &IpSb->OnlinkPrefix;
  } else {
    ListHead = &IpSb->AutonomousPrefix;
  }

  NET_LIST_FOR_EACH (Entry, ListHead) {
    PrefixList = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);
    if (PrefixLength != 255) {
      //
      // Perform exactly prefix match.
      //
      if (PrefixList->PrefixLength == PrefixLength &&
        NetIp6IsNetEqual (&PrefixList->Prefix, Prefix, PrefixLength)) {
        return PrefixList;
      }
    } else {
      //
      // Perform the longest prefix match. The list is already sorted with
      // the longest length prefix put at the head of the list.
      //
      if (NetIp6IsNetEqual (&PrefixList->Prefix, Prefix, PrefixList->PrefixLength)) {
        return PrefixList;
      }
    }
  }

  return NULL;
}

/**
  Release the resource in the prefix list table, and destroy the list entry and
  corresponding addresses or route entries.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  ListHead          The list entry head of the prefix list table.

**/
VOID
Ip6CleanPrefixListTable (
  IN IP6_SERVICE            *IpSb,
  IN LIST_ENTRY             *ListHead
  )
{
  IP6_PREFIX_LIST_ENTRY     *PrefixList;
  BOOLEAN                   OnLink;

  OnLink = (BOOLEAN) (ListHead == &IpSb->OnlinkPrefix);

  while (!IsListEmpty (ListHead)) {
    PrefixList = NET_LIST_HEAD (ListHead, IP6_PREFIX_LIST_ENTRY, Link);
    Ip6DestroyPrefixListEntry (IpSb, PrefixList, OnLink, TRUE);
  }
}

/**
  Callback function when address resolution is finished. It will cancel
  all the queued frames if the address resolution failed, or transmit them
  if the request succeeded.

  @param[in] Context The context of the callback, a pointer to IP6_NEIGHBOR_ENTRY.

**/
VOID
Ip6OnArpResolved (
  IN VOID                   *Context
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_NEIGHBOR_ENTRY        *ArpQue;
  IP6_SERVICE               *IpSb;
  IP6_LINK_TX_TOKEN         *Token;
  EFI_STATUS                Status;
  BOOLEAN                   Sent;

  ArpQue = (IP6_NEIGHBOR_ENTRY *) Context;
  if ((ArpQue == NULL) || (ArpQue->Interface == NULL)) {
    return ;
  }

  IpSb   = ArpQue->Interface->Service;
  if ((IpSb == NULL) || (IpSb->Signature != IP6_SERVICE_SIGNATURE)) {
    return ;
  }

  //
  // ARP resolve failed for some reason. Release all the frame
  // and ARP queue itself. Ip6FreeArpQue will call the frame's
  // owner back.
  //
  if (NET_MAC_EQUAL (&ArpQue->LinkAddress, &mZeroMacAddress, IpSb->SnpMode.HwAddressSize)) {
    Ip6FreeNeighborEntry (IpSb, ArpQue, FALSE, TRUE, EFI_NO_MAPPING, NULL, NULL);
    return ;
  }

  //
  // ARP resolve succeeded, Transmit all the frame.
  //
  Sent = FALSE;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &ArpQue->Frames) {
    RemoveEntryList (Entry);

    Token = NET_LIST_USER_STRUCT (Entry, IP6_LINK_TX_TOKEN, Link);
    IP6_COPY_LINK_ADDRESS (&Token->DstMac, &ArpQue->LinkAddress);

    //
    // Insert the tx token before transmitting it via MNP as the FrameSentDpc
    // may be called before Mnp->Transmit returns which will remove this tx
    // token from the SentFrames list. Remove it from the list if the returned
    // Status of Mnp->Transmit is not EFI_SUCCESS as in this case the
    // FrameSentDpc won't be queued.
    //
    InsertTailList (&ArpQue->Interface->SentFrames, &Token->Link);

    Status = IpSb->Mnp->Transmit (IpSb->Mnp, &Token->MnpToken);
    if (EFI_ERROR (Status)) {
      RemoveEntryList (&Token->Link);
      Token->CallBack (Token->Packet, Status, 0, Token->Context);

      Ip6FreeLinkTxToken (Token);
      continue;
    } else {
      Sent = TRUE;
    }
  }

  //
  // Free the ArpQue only but not the whole neighbor entry.
  //
  Ip6FreeNeighborEntry (IpSb, ArpQue, FALSE, FALSE, EFI_SUCCESS, NULL, NULL);

  if (Sent && (ArpQue->State == EfiNeighborStale)) {
    ArpQue->State = EfiNeighborDelay;
    ArpQue->Ticks = (UINT32) IP6_GET_TICKS (IP6_DELAY_FIRST_PROBE_TIME);
  }
}

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
  IN IP6_SERVICE            *IpSb,
  IN IP6_ARP_CALLBACK       CallBack,
  IN EFI_IPv6_ADDRESS       *Ip6Address,
  IN EFI_MAC_ADDRESS        *LinkAddress OPTIONAL
  )
{
  IP6_NEIGHBOR_ENTRY        *Entry;
  IP6_DEFAULT_ROUTER        *DefaultRouter;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Ip6Address!= NULL);

  Entry = AllocateZeroPool (sizeof (IP6_NEIGHBOR_ENTRY));
  if (Entry == NULL) {
    return NULL;
  }

  Entry->RefCnt    = 1;
  Entry->IsRouter  = FALSE;
  Entry->ArpFree   = FALSE;
  Entry->Dynamic   = FALSE;
  Entry->State     = EfiNeighborInComplete;
  Entry->Transmit  = IP6_MAX_MULTICAST_SOLICIT + 1;
  Entry->CallBack  = CallBack;
  Entry->Interface = NULL;

  InitializeListHead (&Entry->Frames);

  IP6_COPY_ADDRESS (&Entry->Neighbor, Ip6Address);

  if (LinkAddress != NULL) {
    IP6_COPY_LINK_ADDRESS (&Entry->LinkAddress, LinkAddress);
  } else {
    IP6_COPY_LINK_ADDRESS (&Entry->LinkAddress, &mZeroMacAddress);
  }

  InsertHeadList (&IpSb->NeighborTable, &Entry->Link);

  //
  // If corresponding default router entry exists, establish the relationship.
  //
  DefaultRouter = Ip6FindDefaultRouter (IpSb, Ip6Address);
  if (DefaultRouter != NULL) {
    DefaultRouter->NeighborCache = Entry;
  }

  return Entry;
}

/**
  Search a IP6 neighbor cache entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  Ip6Address        Points to the IPv6 address of the neighbor.

  @return NULL if it failed to find the matching neighbor cache entry.
          Otherwise, point to the found neighbor cache entry.

**/
IP6_NEIGHBOR_ENTRY *
Ip6FindNeighborEntry (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Ip6Address
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_NEIGHBOR_ENTRY        *Neighbor;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Ip6Address != NULL);

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->NeighborTable) {
    Neighbor = NET_LIST_USER_STRUCT (Entry, IP6_NEIGHBOR_ENTRY, Link);
    if (EFI_IP6_EQUAL (Ip6Address, &Neighbor->Neighbor)) {
      RemoveEntryList (Entry);
      InsertHeadList (&IpSb->NeighborTable, Entry);

      return Neighbor;
    }
  }

  return NULL;
}

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
  IN IP6_SERVICE            *IpSb,
  IN IP6_NEIGHBOR_ENTRY     *NeighborCache,
  IN BOOLEAN                SendIcmpError,
  IN BOOLEAN                FullFree,
  IN EFI_STATUS             IoStatus,
  IN IP6_FRAME_TO_CANCEL    FrameToCancel OPTIONAL,
  IN VOID                   *Context      OPTIONAL
  )
{
  IP6_LINK_TX_TOKEN         *TxToken;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_DEFAULT_ROUTER        *DefaultRouter;

  //
  // If FrameToCancel fails, the token will not be released.
  // To avoid the memory leak, stop this usage model.
  //
  if (FullFree && FrameToCancel != NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &NeighborCache->Frames) {
    TxToken = NET_LIST_USER_STRUCT (Entry, IP6_LINK_TX_TOKEN, Link);

    if (SendIcmpError && !IP6_IS_MULTICAST (&TxToken->Packet->Ip.Ip6->DestinationAddress)) {
      Ip6SendIcmpError (
        IpSb,
        TxToken->Packet,
        NULL,
        &TxToken->Packet->Ip.Ip6->SourceAddress,
        ICMP_V6_DEST_UNREACHABLE,
        ICMP_V6_ADDR_UNREACHABLE,
        NULL
        );
    }

    if ((FrameToCancel == NULL) || FrameToCancel (TxToken, Context)) {
      RemoveEntryList (Entry);
      TxToken->CallBack (TxToken->Packet, IoStatus, 0, TxToken->Context);
      Ip6FreeLinkTxToken (TxToken);
    }
  }

  if (NeighborCache->ArpFree && IsListEmpty (&NeighborCache->Frames)) {
    RemoveEntryList (&NeighborCache->ArpList);
    NeighborCache->ArpFree = FALSE;
  }

  if (FullFree) {
    if (NeighborCache->IsRouter) {
      DefaultRouter = Ip6FindDefaultRouter (IpSb, &NeighborCache->Neighbor);
      if (DefaultRouter != NULL) {
        Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
      }
    }

    RemoveEntryList (&NeighborCache->Link);
    FreePool (NeighborCache);
  }

  return EFI_SUCCESS;
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Ip6Address,
  IN UINT16                 RouterLifetime
  )
{
  IP6_DEFAULT_ROUTER        *Entry;
  IP6_ROUTE_ENTRY           *RtEntry;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Ip6Address != NULL);

  Entry = AllocatePool (sizeof (IP6_DEFAULT_ROUTER));
  if (Entry == NULL) {
    return NULL;
  }

  Entry->RefCnt        = 1;
  Entry->Lifetime      = RouterLifetime;
  Entry->NeighborCache = Ip6FindNeighborEntry (IpSb, Ip6Address);
  IP6_COPY_ADDRESS (&Entry->Router, Ip6Address);

  //
  // Add a default route into route table with both Destination and PrefixLength set to zero.
  //
  RtEntry = Ip6CreateRouteEntry (NULL, 0, Ip6Address);
  if (RtEntry == NULL) {
    FreePool (Entry);
    return NULL;
  }

  InsertHeadList (&IpSb->RouteTable->RouteArea[0], &RtEntry->Link);
  IpSb->RouteTable->TotalNum++;

  InsertTailList (&IpSb->DefaultRouterList, &Entry->Link);

  return Entry;
}

/**
  Destroy an IP6 default router entry.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.
  @param[in]  DefaultRouter     The to be destroyed IP6_DEFAULT_ROUTER.

**/
VOID
Ip6DestroyDefaultRouter (
  IN IP6_SERVICE            *IpSb,
  IN IP6_DEFAULT_ROUTER     *DefaultRouter
  )
{
  EFI_STATUS                Status;

  RemoveEntryList (&DefaultRouter->Link);

  //
  // Update the Destination Cache - all entries using the time-out router as next-hop
  // should perform next-hop determination again.
  //
  do {
    Status = Ip6DelRoute (IpSb->RouteTable, NULL, 0, &DefaultRouter->Router);
  } while (Status != EFI_NOT_FOUND);

  FreePool (DefaultRouter);
}

/**
  Clean an IP6 default router list.

  @param[in]  IpSb              The pointer to the IP6_SERVICE instance.

**/
VOID
Ip6CleanDefaultRouterList (
  IN IP6_SERVICE            *IpSb
  )
{
  IP6_DEFAULT_ROUTER        *DefaultRouter;

  while (!IsListEmpty (&IpSb->DefaultRouterList)) {
    DefaultRouter = NET_LIST_HEAD (&IpSb->DefaultRouterList, IP6_DEFAULT_ROUTER, Link);
    Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
  }
}

/**
  Search a default router node from an IP6 default router list.

  @param[in]  IpSb          The pointer to the IP6_SERVICE instance.
  @param[in]  Ip6Address    The IPv6 address of the to be searched default router node.

  @return NULL if it failed to find the matching default router node.
          Otherwise, point to the found default router node.

**/
IP6_DEFAULT_ROUTER *
Ip6FindDefaultRouter (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Ip6Address
  )
{
  LIST_ENTRY                *Entry;
  IP6_DEFAULT_ROUTER        *DefaultRouter;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (Ip6Address != NULL);

  NET_LIST_FOR_EACH (Entry, &IpSb->DefaultRouterList) {
    DefaultRouter = NET_LIST_USER_STRUCT (Entry, IP6_DEFAULT_ROUTER, Link);
    if (EFI_IP6_EQUAL (Ip6Address, &DefaultRouter->Router)) {
      return DefaultRouter;
    }
  }

  return NULL;
}

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
  )
{
  IP6_SERVICE               *IpSb;
  IP6_ADDRESS_INFO          *AddrInfo;
  EFI_DHCP6_PROTOCOL        *Dhcp6;
  UINT16                    OptBuf[4];
  EFI_DHCP6_PACKET_OPTION   *Oro;
  EFI_DHCP6_RETRANSMISSION  InfoReqReXmit;
  EFI_IPv6_ADDRESS          AllNodes;

  IpSb     = IpIf->Service;
  AddrInfo = DadEntry->AddressInfo;

  if (IsDadPassed) {
    //
    // DAD succeed.
    //
    if (NetIp6IsLinkLocalAddr (&AddrInfo->Address)) {
      ASSERT (!IpSb->LinkLocalOk);

      IP6_COPY_ADDRESS (&IpSb->LinkLocalAddr, &AddrInfo->Address);
      IpSb->LinkLocalOk = TRUE;
      IpIf->Configured  = TRUE;

      //
      // Check whether DHCP6 need to be started.
      //
      Dhcp6 = IpSb->Ip6ConfigInstance.Dhcp6;

      if (IpSb->Dhcp6NeedStart) {
        Dhcp6->Start (Dhcp6);
        IpSb->Dhcp6NeedStart = FALSE;
      }

      if (IpSb->Dhcp6NeedInfoRequest) {
        //
        // Set the exta options to send. Here we only want the option request option
        // with DNS SERVERS.
        //
        Oro         = (EFI_DHCP6_PACKET_OPTION *) OptBuf;
        Oro->OpCode = HTONS (DHCP6_OPT_ORO);
        Oro->OpLen  = HTONS (2);
        *((UINT16 *) &Oro->Data[0]) = HTONS (DHCP6_OPT_DNS_SERVERS);

        InfoReqReXmit.Irt = 4;
        InfoReqReXmit.Mrc = 64;
        InfoReqReXmit.Mrt = 60;
        InfoReqReXmit.Mrd = 0;

        Dhcp6->InfoRequest (
                 Dhcp6,
                 TRUE,
                 Oro,
                 0,
                 NULL,
                 &InfoReqReXmit,
                 IpSb->Ip6ConfigInstance.Dhcp6Event,
                 Ip6ConfigOnDhcp6Reply,
                 &IpSb->Ip6ConfigInstance
                 );
      }

      //
      // Add an on-link prefix for link-local address.
      //
      Ip6CreatePrefixListEntry (
        IpSb,
        TRUE,
        (UINT32) IP6_INFINIT_LIFETIME,
        (UINT32) IP6_INFINIT_LIFETIME,
        IP6_LINK_LOCAL_PREFIX_LENGTH,
        &IpSb->LinkLocalAddr
        );

    } else {
      //
      // Global scope unicast address.
      //
      Ip6AddAddr (IpIf, AddrInfo);

      //
      // Add an on-link prefix for this address.
      //
      Ip6CreatePrefixListEntry (
        IpSb,
        TRUE,
        AddrInfo->ValidLifetime,
        AddrInfo->PreferredLifetime,
        AddrInfo->PrefixLength,
        &AddrInfo->Address
        );

      IpIf->Configured = TRUE;
    }
  } else {
    //
    // Leave the group we joined before.
    //
    Ip6LeaveGroup (IpSb, &DadEntry->Destination);
  }

  if (DadEntry->Callback != NULL) {
    DadEntry->Callback (IsDadPassed, &AddrInfo->Address, DadEntry->Context);
  }

  if (!IsDadPassed && NetIp6IsLinkLocalAddr (&AddrInfo->Address)) {
    FreePool (AddrInfo);
    RemoveEntryList (&DadEntry->Link);
    FreePool (DadEntry);
    //
    // Leave link-scope all-nodes multicast address (FF02::1)
    //
    Ip6SetToAllNodeMulticast (FALSE, IP6_LINK_LOCAL_SCOPE, &AllNodes);
    Ip6LeaveGroup (IpSb, &AllNodes);
    //
    // Disable IP operation since link-local address is a duplicate address.
    //
    IpSb->LinkLocalDadFail = TRUE;
    IpSb->Mnp->Configure (IpSb->Mnp, NULL);
    gBS->SetTimer (IpSb->Timer, TimerCancel, 0);
    gBS->SetTimer (IpSb->FasterTimer, TimerCancel, 0);
    return ;
  }

  if (!IsDadPassed || NetIp6IsLinkLocalAddr (&AddrInfo->Address)) {
    //
    // Free the AddressInfo we hold if DAD fails or it is a link-local address.
    //
    FreePool (AddrInfo);
  }

  RemoveEntryList (&DadEntry->Link);
  FreePool (DadEntry);
}

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
  IN IP6_INTERFACE          *IpIf,
  IN IP6_ADDRESS_INFO       *AddressInfo,
  IN IP6_DAD_CALLBACK       Callback  OPTIONAL,
  IN VOID                   *Context  OPTIONAL
  )
{
  IP6_DAD_ENTRY                             *Entry;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS  *DadXmits;
  IP6_SERVICE                               *IpSb;
  EFI_STATUS                                Status;
  UINT32                                    MaxDelayTick;

  NET_CHECK_SIGNATURE (IpIf, IP6_INTERFACE_SIGNATURE);
  ASSERT (AddressInfo != NULL);

  //
  // Do nothing if we have already started DAD on the address.
  //
  if (Ip6FindDADEntry (IpIf->Service, &AddressInfo->Address, NULL) != NULL) {
    return EFI_SUCCESS;
  }

  Status   = EFI_SUCCESS;
  IpSb     = IpIf->Service;
  DadXmits = &IpSb->Ip6ConfigInstance.DadXmits;

  //
  // Allocate the resources and insert info
  //
  Entry = AllocatePool (sizeof (IP6_DAD_ENTRY));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Map the incoming unicast address to solicited-node multicast address
  //
  Ip6CreateSNMulticastAddr (&AddressInfo->Address, &Entry->Destination);

  //
  // Join in the solicited-node multicast address.
  //
  Status = Ip6JoinGroup (IpSb, IpIf, &Entry->Destination);
  if (EFI_ERROR (Status)) {
    FreePool (Entry);
    return Status;
  }

  Entry->Signature    = IP6_DAD_ENTRY_SIGNATURE;
  Entry->MaxTransmit  = DadXmits->DupAddrDetectTransmits;
  Entry->Transmit     = 0;
  Entry->Receive      = 0;
  MaxDelayTick        = IP6_MAX_RTR_SOLICITATION_DELAY / IP6_TIMER_INTERVAL_IN_MS;
  Entry->RetransTick  = (MaxDelayTick * ((NET_RANDOM (NetRandomInitSeed ()) % 5) + 1)) / 5;
  Entry->AddressInfo  = AddressInfo;
  Entry->Callback     = Callback;
  Entry->Context      = Context;
  InsertTailList (&IpIf->DupAddrDetectList, &Entry->Link);

  if (Entry->MaxTransmit == 0) {
    //
    // DAD is disabled on this interface, immediately mark this DAD successful.
    //
    Ip6OnDADFinished (TRUE, IpIf, Entry);
  }

  return EFI_SUCCESS;
}

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
  IN  IP6_SERVICE      *IpSb,
  IN  EFI_IPv6_ADDRESS *Target,
  OUT IP6_INTERFACE    **Interface OPTIONAL
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Entry2;
  IP6_INTERFACE             *IpIf;
  IP6_DAD_ENTRY             *DupAddrDetect;
  IP6_ADDRESS_INFO          *AddrInfo;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);

    NET_LIST_FOR_EACH (Entry2, &IpIf->DupAddrDetectList) {
      DupAddrDetect = NET_LIST_USER_STRUCT_S (Entry2, IP6_DAD_ENTRY, Link, IP6_DAD_ENTRY_SIGNATURE);
      AddrInfo      = DupAddrDetect->AddressInfo;
      if (EFI_IP6_EQUAL (&AddrInfo->Address, Target)) {
        if (Interface != NULL) {
          *Interface = IpIf;
        }
        return DupAddrDetect;
      }
    }
  }

  return NULL;
}

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

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            The router solicit message was successfully sent.

**/
EFI_STATUS
Ip6SendRouterSolicit (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface          OPTIONAL,
  IN EFI_IPv6_ADDRESS       *SourceAddress      OPTIONAL,
  IN EFI_IPv6_ADDRESS       *DestinationAddress OPTIONAL,
  IN EFI_MAC_ADDRESS        *SourceLinkAddress  OPTIONAL
  )
{
  NET_BUF                   *Packet;
  EFI_IP6_HEADER            Head;
  IP6_ICMP_INFORMATION_HEAD *IcmpHead;
  IP6_ETHER_ADDR_OPTION     *LinkLayerOption;
  UINT16                    PayloadLen;
  IP6_INTERFACE             *IpIf;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  IpIf = Interface;
  if (IpIf == NULL && IpSb->DefaultInterface != NULL) {
    IpIf = IpSb->DefaultInterface;
  }

  //
  // Generate the packet to be sent
  //

  PayloadLen = (UINT16) sizeof (IP6_ICMP_INFORMATION_HEAD);
  if (SourceLinkAddress != NULL) {
    PayloadLen += sizeof (IP6_ETHER_ADDR_OPTION);
  }

  Packet = NetbufAlloc (sizeof (EFI_IP6_HEADER) + (UINT32) PayloadLen);
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create the basic IPv6 header.
  //
  Head.FlowLabelL     = 0;
  Head.FlowLabelH     = 0;
  Head.PayloadLength  = HTONS (PayloadLen);
  Head.NextHeader     = IP6_ICMP;
  Head.HopLimit       = IP6_HOP_LIMIT;

  if (SourceAddress != NULL) {
    IP6_COPY_ADDRESS (&Head.SourceAddress, SourceAddress);
  } else {
    ZeroMem (&Head.SourceAddress, sizeof (EFI_IPv6_ADDRESS));
  }


  if (DestinationAddress != NULL) {
    IP6_COPY_ADDRESS (&Head.DestinationAddress, DestinationAddress);
  } else {
    Ip6SetToAllNodeMulticast (TRUE, IP6_LINK_LOCAL_SCOPE, &Head.DestinationAddress);
  }

  NetbufReserve (Packet, sizeof (EFI_IP6_HEADER));

  //
  // Fill in the ICMP header, and Source link-layer address if contained.
  //

  IcmpHead = (IP6_ICMP_INFORMATION_HEAD *) NetbufAllocSpace (Packet, sizeof (IP6_ICMP_INFORMATION_HEAD), FALSE);
  ASSERT (IcmpHead != NULL);
  ZeroMem (IcmpHead, sizeof (IP6_ICMP_INFORMATION_HEAD));
  IcmpHead->Head.Type = ICMP_V6_ROUTER_SOLICIT;
  IcmpHead->Head.Code = 0;

  LinkLayerOption = NULL;
  if (SourceLinkAddress != NULL) {
    LinkLayerOption = (IP6_ETHER_ADDR_OPTION *) NetbufAllocSpace (
                                                  Packet,
                                                  sizeof (IP6_ETHER_ADDR_OPTION),
                                                  FALSE
                                                  );
    ASSERT (LinkLayerOption != NULL);
    LinkLayerOption->Type   = Ip6OptionEtherSource;
    LinkLayerOption->Length = (UINT8) sizeof (IP6_ETHER_ADDR_OPTION);
    CopyMem (LinkLayerOption->EtherAddr, SourceLinkAddress, 6);
  }

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, IpIf, NULL, Packet, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

/**
  Generate a Neighbor Advertisement message and send it out to Destination Address.

  @param[in]  IpSb               The IP service to send the packet.
  @param[in]  SourceAddress      The source address of the message.
  @param[in]  DestinationAddress The destination address of the message.
  @param[in]  TargetIp6Address   The target address field in the Neighbor Solicitation
                                 message that prompted this advertisement.
  @param[in]  TargetLinkAddress  The MAC address for the target, i.e. the sender
                                 of the advertisement.
  @param[in]  IsRouter           If TRUE, indicates the sender is a router.
  @param[in]  Override           If TRUE, indicates the advertisement should override
                                 an existing cache entry and update the MAC address.
  @param[in]  Solicited          If TRUE, indicates the advertisement was sent
                                 in response to a Neighbor Solicitation from
                                 the Destination address.

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            The Neighbor Advertise message was successfully sent.

**/
EFI_STATUS
Ip6SendNeighborAdvertise (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *SourceAddress,
  IN EFI_IPv6_ADDRESS       *DestinationAddress,
  IN EFI_IPv6_ADDRESS       *TargetIp6Address,
  IN EFI_MAC_ADDRESS        *TargetLinkAddress,
  IN BOOLEAN                IsRouter,
  IN BOOLEAN                Override,
  IN BOOLEAN                Solicited
  )
{
  NET_BUF                   *Packet;
  EFI_IP6_HEADER            Head;
  IP6_ICMP_INFORMATION_HEAD *IcmpHead;
  IP6_ETHER_ADDR_OPTION     *LinkLayerOption;
  EFI_IPv6_ADDRESS          *Target;
  UINT16                    PayloadLen;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  //
  // The Neighbor Advertisement message must include a Target link-layer address option
  // when responding to multicast solicitation and should include such option when
  // responding to unicast solicitation. It also must include such option as unsolicited
  // advertisement.
  //
  ASSERT (DestinationAddress != NULL && TargetIp6Address != NULL && TargetLinkAddress != NULL);

  PayloadLen = (UINT16) (sizeof (IP6_ICMP_INFORMATION_HEAD) + sizeof (EFI_IPv6_ADDRESS) + sizeof (IP6_ETHER_ADDR_OPTION));

  //
  // Generate the packet to be sent
  //

  Packet = NetbufAlloc (sizeof (EFI_IP6_HEADER) + (UINT32) PayloadLen);
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create the basic IPv6 header.
  //
  Head.FlowLabelL     = 0;
  Head.FlowLabelH     = 0;
  Head.PayloadLength  = HTONS (PayloadLen);
  Head.NextHeader     = IP6_ICMP;
  Head.HopLimit       = IP6_HOP_LIMIT;

  IP6_COPY_ADDRESS (&Head.SourceAddress, SourceAddress);
  IP6_COPY_ADDRESS (&Head.DestinationAddress, DestinationAddress);

  NetbufReserve (Packet, sizeof (EFI_IP6_HEADER));

  //
  // Fill in the ICMP header, Target address, and Target link-layer address.
  // Set the Router flag, Solicited flag and Override flag.
  //

  IcmpHead = (IP6_ICMP_INFORMATION_HEAD *) NetbufAllocSpace (Packet, sizeof (IP6_ICMP_INFORMATION_HEAD), FALSE);
  ASSERT (IcmpHead != NULL);
  ZeroMem (IcmpHead, sizeof (IP6_ICMP_INFORMATION_HEAD));
  IcmpHead->Head.Type = ICMP_V6_NEIGHBOR_ADVERTISE;
  IcmpHead->Head.Code = 0;

  if (IsRouter) {
    IcmpHead->Fourth |= IP6_IS_ROUTER_FLAG;
  }

  if (Solicited) {
    IcmpHead->Fourth |= IP6_SOLICITED_FLAG;
  }

  if (Override) {
    IcmpHead->Fourth |= IP6_OVERRIDE_FLAG;
  }

  Target = (EFI_IPv6_ADDRESS *) NetbufAllocSpace (Packet, sizeof (EFI_IPv6_ADDRESS), FALSE);
  ASSERT (Target != NULL);
  IP6_COPY_ADDRESS (Target, TargetIp6Address);

  LinkLayerOption = (IP6_ETHER_ADDR_OPTION *) NetbufAllocSpace (
                                                Packet,
                                                sizeof (IP6_ETHER_ADDR_OPTION),
                                                FALSE
                                                );
  ASSERT (LinkLayerOption != NULL);
  LinkLayerOption->Type   = Ip6OptionEtherTarget;
  LinkLayerOption->Length = 1;
  CopyMem (LinkLayerOption->EtherAddr, TargetLinkAddress, 6);

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, NULL, NULL, Packet, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *SourceAddress,
  IN EFI_IPv6_ADDRESS       *DestinationAddress,
  IN EFI_IPv6_ADDRESS       *TargetIp6Address,
  IN EFI_MAC_ADDRESS        *SourceLinkAddress OPTIONAL
  )
{
  NET_BUF                   *Packet;
  EFI_IP6_HEADER            Head;
  IP6_ICMP_INFORMATION_HEAD *IcmpHead;
  IP6_ETHER_ADDR_OPTION     *LinkLayerOption;
  EFI_IPv6_ADDRESS          *Target;
  BOOLEAN                   IsDAD;
  UINT16                    PayloadLen;
  IP6_NEIGHBOR_ENTRY        *Neighbor;

  //
  // Check input parameters
  //
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  if (DestinationAddress == NULL || TargetIp6Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IsDAD = FALSE;

  if (SourceAddress == NULL || (SourceAddress != NULL && NetIp6IsUnspecifiedAddr (SourceAddress))) {
    IsDAD = TRUE;
  }

  //
  // The Neighbor Solicitation message should include a source link-layer address option
  // if the solicitation is not sent by performing DAD - Duplicate Address Detection.
  // Otherwise must not include it.
  //
  PayloadLen = (UINT16) (sizeof (IP6_ICMP_INFORMATION_HEAD) + sizeof (EFI_IPv6_ADDRESS));

  if (!IsDAD) {
    if (SourceLinkAddress == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    PayloadLen = (UINT16) (PayloadLen + sizeof (IP6_ETHER_ADDR_OPTION));
  }

  //
  // Generate the packet to be sent
  //

  Packet = NetbufAlloc (sizeof (EFI_IP6_HEADER) + (UINT32) PayloadLen);
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create the basic IPv6 header
  //
  Head.FlowLabelL     = 0;
  Head.FlowLabelH     = 0;
  Head.PayloadLength  = HTONS (PayloadLen);
  Head.NextHeader     = IP6_ICMP;
  Head.HopLimit       = IP6_HOP_LIMIT;

  if (SourceAddress != NULL) {
    IP6_COPY_ADDRESS (&Head.SourceAddress, SourceAddress);
  } else {
    ZeroMem (&Head.SourceAddress, sizeof (EFI_IPv6_ADDRESS));
  }

  IP6_COPY_ADDRESS (&Head.DestinationAddress, DestinationAddress);

  NetbufReserve (Packet, sizeof (EFI_IP6_HEADER));

  //
  // Fill in the ICMP header, Target address, and Source link-layer address.
  //
  IcmpHead = (IP6_ICMP_INFORMATION_HEAD *) NetbufAllocSpace (Packet, sizeof (IP6_ICMP_INFORMATION_HEAD), FALSE);
  ASSERT (IcmpHead != NULL);
  ZeroMem (IcmpHead, sizeof (IP6_ICMP_INFORMATION_HEAD));
  IcmpHead->Head.Type = ICMP_V6_NEIGHBOR_SOLICIT;
  IcmpHead->Head.Code = 0;

  Target = (EFI_IPv6_ADDRESS *) NetbufAllocSpace (Packet, sizeof (EFI_IPv6_ADDRESS), FALSE);
  ASSERT (Target != NULL);
  IP6_COPY_ADDRESS (Target, TargetIp6Address);

  LinkLayerOption = NULL;
  if (!IsDAD) {

    //
    // Fill in the source link-layer address option
    //
    LinkLayerOption = (IP6_ETHER_ADDR_OPTION *) NetbufAllocSpace (
                                                  Packet,
                                                  sizeof (IP6_ETHER_ADDR_OPTION),
                                                  FALSE
                                                  );
    ASSERT (LinkLayerOption != NULL);
    LinkLayerOption->Type   = Ip6OptionEtherSource;
    LinkLayerOption->Length = 1;
    CopyMem (LinkLayerOption->EtherAddr, SourceLinkAddress, 6);
  }

  //
  // Create a Neighbor Cache entry in the INCOMPLETE state when performing
  // address resolution.
  //
  if (!IsDAD && Ip6IsSNMulticastAddr (DestinationAddress)) {
    Neighbor = Ip6FindNeighborEntry (IpSb, TargetIp6Address);
    if (Neighbor == NULL) {
      Neighbor = Ip6CreateNeighborEntry (IpSb, Ip6OnArpResolved, TargetIp6Address, NULL);
      ASSERT (Neighbor != NULL);
    }
  }

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, IpSb->DefaultInterface, NULL, Packet, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD Icmp;
  EFI_IPv6_ADDRESS          Target;
  IP6_ETHER_ADDR_OPTION     LinkLayerOption;
  BOOLEAN                   IsDAD;
  BOOLEAN                   IsUnicast;
  BOOLEAN                   IsMaintained;
  IP6_DAD_ENTRY             *DupAddrDetect;
  IP6_INTERFACE             *IpIf;
  IP6_NEIGHBOR_ENTRY        *Neighbor;
  BOOLEAN                   Solicited;
  BOOLEAN                   UpdateCache;
  EFI_IPv6_ADDRESS          Dest;
  UINT16                    OptionLen;
  UINT8                     *Option;
  BOOLEAN                   Provided;
  EFI_STATUS                Status;
  VOID                      *MacAddress;

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
  NetbufCopy (Packet, sizeof (Icmp), sizeof (Target), Target.Addr);

  //
  // Perform Message Validation:
  // The IP Hop Limit field has a value of 255, i.e., the packet
  // could not possibly have been forwarded by a router.
  // ICMP Code is 0.
  // Target Address is not a multicast address.
  //
  Status = EFI_INVALID_PARAMETER;

  if (Head->HopLimit != IP6_HOP_LIMIT || Icmp.Head.Code != 0 || !NetIp6IsValidUnicast (&Target)) {
    goto Exit;
  }

  //
  // ICMP length is 24 or more octets.
  //
  OptionLen = 0;
  if (Head->PayloadLength < IP6_ND_LENGTH) {
    goto Exit;
  } else {
    OptionLen = (UINT16) (Head->PayloadLength - IP6_ND_LENGTH);
    if (OptionLen != 0) {
      Option    = NetbufGetByte (Packet, IP6_ND_LENGTH, NULL);
      ASSERT (Option != NULL);

      //
      // All included options should have a length that is greater than zero.
      //
      if (!Ip6IsNDOptionValid (Option, OptionLen)) {
        goto Exit;
      }
    }
  }

  IsDAD        = NetIp6IsUnspecifiedAddr (&Head->SourceAddress);
  IsUnicast    = (BOOLEAN) !Ip6IsSNMulticastAddr (&Head->DestinationAddress);
  IsMaintained = Ip6IsOneOfSetAddress (IpSb, &Target, &IpIf, NULL);

  Provided = FALSE;
  if (OptionLen >= sizeof (IP6_ETHER_ADDR_OPTION)) {
    NetbufCopy (
      Packet,
      IP6_ND_LENGTH,
      sizeof (IP6_ETHER_ADDR_OPTION),
      (UINT8 *) &LinkLayerOption
      );
    //
    // The solicitation for neighbor discovery should include a source link-layer
    // address option. If the option is not recognized, silently ignore it.
    //
    if (LinkLayerOption.Type == Ip6OptionEtherSource) {
      if (IsDAD) {
        //
        // If the IP source address is the unspecified address, the source
        // link-layer address option must not be included in the message.
        //
        goto Exit;
      }

      Provided = TRUE;
    }
  }

  //
  // If the IP source address is the unspecified address, the IP
  // destination address is a solicited-node multicast address.
  //
  if (IsDAD && IsUnicast) {
    goto Exit;
  }

  //
  // If the target address is tentative, and the source address is a unicast address,
  // the solicitation's sender is performing address resolution on the target;
  //  the solicitation should be silently ignored.
  //
  if (!IsDAD && !IsMaintained) {
    goto Exit;
  }

  //
  // If received unicast neighbor solicitation but destination is not this node,
  // drop the packet.
  //
  if (IsUnicast && !IsMaintained) {
    goto Exit;
  }

  //
  // In DAD, when target address is a tentative address,
  // process the received neighbor solicitation message but not send out response.
  //
  if (IsDAD && !IsMaintained) {
    DupAddrDetect = Ip6FindDADEntry (IpSb, &Target, &IpIf);
    if (DupAddrDetect != NULL) {
      //
      // Check the MAC address of the incoming packet.
      //
      if (IpSb->RecvRequest.MnpToken.Packet.RxData == NULL) {
        goto Exit;
      }

      MacAddress = IpSb->RecvRequest.MnpToken.Packet.RxData->SourceAddress;
      if (MacAddress != NULL) {
        if (CompareMem (
              MacAddress,
              &IpSb->SnpMode.CurrentAddress,
              IpSb->SnpMode.HwAddressSize
              ) != 0) {
          //
          // The NS is from another node to performing DAD on the same address.
          // Fail DAD for the tentative address.
          //
          Ip6OnDADFinished (FALSE, IpIf, DupAddrDetect);
          Status = EFI_ICMP_ERROR;
        } else {
          //
          // The below layer loopback the NS we sent. Record it and wait for more.
          //
          DupAddrDetect->Receive++;
          Status = EFI_SUCCESS;
        }
      }
    }
    goto Exit;
  }

  //
  // If the solicitation does not contain a link-layer address, DO NOT create or
  // update the neighbor cache entries.
  //
  if (Provided) {
    Neighbor    = Ip6FindNeighborEntry (IpSb, &Head->SourceAddress);
    UpdateCache = FALSE;

    if (Neighbor == NULL) {
      Neighbor = Ip6CreateNeighborEntry (IpSb, Ip6OnArpResolved, &Head->SourceAddress, NULL);
      if (Neighbor == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
      UpdateCache = TRUE;
    } else {
      if (CompareMem (Neighbor->LinkAddress.Addr, LinkLayerOption.EtherAddr, 6) != 0) {
        UpdateCache = TRUE;
      }
    }

    if (UpdateCache) {
      Neighbor->State = EfiNeighborStale;
      Neighbor->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
      CopyMem (Neighbor->LinkAddress.Addr, LinkLayerOption.EtherAddr, 6);
      //
      // Send queued packets if exist.
      //
      Neighbor->CallBack ((VOID *) Neighbor);
    }
  }

  //
  // Sends a Neighbor Advertisement as response.
  // Set the Router flag to zero since the node is a host.
  // If the source address of the solicitation is unspecified, and target address
  // is one of the maintained address, reply a unsolicited multicast advertisement.
  //
  if (IsDAD && IsMaintained) {
    Solicited = FALSE;
    Ip6SetToAllNodeMulticast (FALSE, IP6_LINK_LOCAL_SCOPE, &Dest);
  } else {
    Solicited = TRUE;
    IP6_COPY_ADDRESS (&Dest, &Head->SourceAddress);
  }

  Status = Ip6SendNeighborAdvertise (
             IpSb,
             &Target,
             &Dest,
             &Target,
             &IpSb->SnpMode.CurrentAddress,
             FALSE,
             TRUE,
             Solicited
             );
Exit:
  NetbufFree (Packet);
  return Status;
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD Icmp;
  EFI_IPv6_ADDRESS          Target;
  IP6_ETHER_ADDR_OPTION     LinkLayerOption;
  BOOLEAN                   Provided;
  INTN                      Compare;
  IP6_NEIGHBOR_ENTRY        *Neighbor;
  IP6_DEFAULT_ROUTER        *DefaultRouter;
  BOOLEAN                   Solicited;
  BOOLEAN                   IsRouter;
  BOOLEAN                   Override;
  IP6_DAD_ENTRY             *DupAddrDetect;
  IP6_INTERFACE             *IpIf;
  UINT16                    OptionLen;
  UINT8                     *Option;
  EFI_STATUS                Status;

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
  NetbufCopy (Packet, sizeof (Icmp), sizeof (Target), Target.Addr);

  //
  // Validate the incoming Neighbor Advertisement
  //
  Status = EFI_INVALID_PARAMETER;
  //
  // The IP Hop Limit field has a value of 255, i.e., the packet
  // could not possibly have been forwarded by a router.
  // ICMP Code is 0.
  // Target Address is not a multicast address.
  //
  if (Head->HopLimit != IP6_HOP_LIMIT || Icmp.Head.Code != 0 || !NetIp6IsValidUnicast (&Target)) {
    goto Exit;
  }

  //
  // ICMP length is 24 or more octets.
  //
  Provided  = FALSE;
  OptionLen = 0;
  if (Head->PayloadLength < IP6_ND_LENGTH) {
    goto Exit;
  } else {
    OptionLen = (UINT16) (Head->PayloadLength - IP6_ND_LENGTH);
    if (OptionLen != 0) {
      Option    = NetbufGetByte (Packet, IP6_ND_LENGTH, NULL);
      ASSERT (Option != NULL);

      //
      // All included options should have a length that is greater than zero.
      //
      if (!Ip6IsNDOptionValid (Option, OptionLen)) {
        goto Exit;
      }
    }
  }

  //
  // If the IP destination address is a multicast address, Solicited Flag is ZERO.
  //
  Solicited = FALSE;
  if ((Icmp.Fourth & IP6_SOLICITED_FLAG) == IP6_SOLICITED_FLAG) {
    Solicited = TRUE;
  }
  if (IP6_IS_MULTICAST (&Head->DestinationAddress) && Solicited) {
    goto Exit;
  }

  //
  // DAD - Check whether the Target is one of our tentative address.
  //
  DupAddrDetect = Ip6FindDADEntry (IpSb, &Target, &IpIf);
  if (DupAddrDetect != NULL) {
    //
    // DAD fails, some other node is using this address.
    //
    NetbufFree (Packet);
    Ip6OnDADFinished (FALSE, IpIf, DupAddrDetect);
    return EFI_ICMP_ERROR;
  }

  //
  // Search the Neighbor Cache for the target's entry. If no entry exists,
  // the advertisement should be silently discarded.
  //
  Neighbor = Ip6FindNeighborEntry (IpSb, &Target);
  if (Neighbor == NULL) {
    goto Exit;
  }

  //
  // Get IsRouter Flag and Override Flag
  //
  IsRouter = FALSE;
  Override = FALSE;
  if ((Icmp.Fourth & IP6_IS_ROUTER_FLAG) == IP6_IS_ROUTER_FLAG) {
    IsRouter = TRUE;
  }
  if ((Icmp.Fourth & IP6_OVERRIDE_FLAG) == IP6_OVERRIDE_FLAG) {
    Override = TRUE;
  }

  //
  // Check whether link layer option is included.
  //
  if (OptionLen >= sizeof (IP6_ETHER_ADDR_OPTION)) {
    NetbufCopy (
      Packet,
      IP6_ND_LENGTH,
      sizeof (IP6_ETHER_ADDR_OPTION),
      (UINT8 *) &LinkLayerOption
      );

    if (LinkLayerOption.Type == Ip6OptionEtherTarget) {
      Provided = TRUE;
    }
  }

  Compare = 0;
  if (Provided) {
    Compare = CompareMem (Neighbor->LinkAddress.Addr, LinkLayerOption.EtherAddr, 6);
  }

  if (!Neighbor->IsRouter && IsRouter) {
    DefaultRouter = Ip6FindDefaultRouter (IpSb, &Target);
    if (DefaultRouter != NULL) {
      DefaultRouter->NeighborCache = Neighbor;
    }
  }

  if (Neighbor->State == EfiNeighborInComplete) {
    //
    // If the target's Neighbor Cache entry is in INCOMPLETE state and no
    // Target Link-Layer address option is included while link layer has
    // address, the message should be silently discarded.
    //
    if (!Provided) {
      goto Exit;
    }
    //
    // Update the Neighbor Cache
    //
    CopyMem (Neighbor->LinkAddress.Addr, LinkLayerOption.EtherAddr, 6);
    if (Solicited) {
      Neighbor->State = EfiNeighborReachable;
      Neighbor->Ticks = IP6_GET_TICKS (IpSb->ReachableTime);
    } else {
      Neighbor->State = EfiNeighborStale;
      Neighbor->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
      //
      // Send any packets queued for the neighbor awaiting address resolution.
      //
      Neighbor->CallBack ((VOID *) Neighbor);
    }

    Neighbor->IsRouter = IsRouter;

  } else {
    if (!Override && Compare != 0) {
      //
      // When the Override Flag is clear and supplied link-layer address differs from
      // that in the cache, if the state of the entry is not REACHABLE, ignore the
      // message. Otherwise set it to STALE but do not update the entry in any
      // other way.
      //
      if (Neighbor->State == EfiNeighborReachable) {
        Neighbor->State = EfiNeighborStale;
        Neighbor->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
      }
    } else {
      if (Compare != 0) {
        CopyMem (Neighbor->LinkAddress.Addr, LinkLayerOption.EtherAddr, 6);
      }
      //
      // Update the entry's state
      //
      if (Solicited) {
        Neighbor->State = EfiNeighborReachable;
        Neighbor->Ticks = IP6_GET_TICKS (IpSb->ReachableTime);
      } else {
        if (Compare != 0) {
          Neighbor->State = EfiNeighborStale;
          Neighbor->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
        }
      }

      //
      // When IsRouter is changed from TRUE to FALSE, remove the router from the
      // Default Router List and remove the Destination Cache entries for all destinations
      // using the neighbor as a router.
      //
      if (Neighbor->IsRouter && !IsRouter) {
        DefaultRouter = Ip6FindDefaultRouter (IpSb, &Target);
        if (DefaultRouter != NULL) {
          Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
        }
      }

      Neighbor->IsRouter = IsRouter;
    }
  }

  if (Neighbor->State == EfiNeighborReachable) {
    Neighbor->CallBack ((VOID *) Neighbor);
  }

  Status = EFI_SUCCESS;

Exit:
  NetbufFree (Packet);
  return Status;
}

/**
  Process the Router Advertisement message according to RFC4861.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the message.
  @param[in]  Packet             The content of the message with the IP head removed.

  @retval EFI_SUCCESS            The packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to complete the
                                 operation.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessRouterAdvertise (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD Icmp;
  UINT32                    ReachableTime;
  UINT32                    RetransTimer;
  UINT16                    RouterLifetime;
  UINT32                    Offset;
  UINT8                     Type;
  UINT8                     Length;
  IP6_ETHER_ADDR_OPTION     LinkLayerOption;
  UINT32                    Fourth;
  UINT8                     CurHopLimit;
  BOOLEAN                   Mflag;
  BOOLEAN                   Oflag;
  IP6_DEFAULT_ROUTER        *DefaultRouter;
  IP6_NEIGHBOR_ENTRY        *NeighborCache;
  EFI_MAC_ADDRESS           LinkLayerAddress;
  IP6_MTU_OPTION            MTUOption;
  IP6_PREFIX_INFO_OPTION    PrefixOption;
  IP6_PREFIX_LIST_ENTRY     *PrefixList;
  BOOLEAN                   OnLink;
  BOOLEAN                   Autonomous;
  EFI_IPv6_ADDRESS          StatelessAddress;
  EFI_STATUS                Status;
  UINT16                    OptionLen;
  UINT8                     *Option;
  INTN                      Result;

  Status = EFI_INVALID_PARAMETER;

  if (IpSb->Ip6ConfigInstance.Policy != Ip6ConfigPolicyAutomatic) {
    //
    // Skip the process below as it's not required under the current policy.
    //
    goto Exit;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);

  //
  // Validate the incoming Router Advertisement
  //

  //
  // The IP source address must be a link-local address
  //
  if (!NetIp6IsLinkLocalAddr (&Head->SourceAddress)) {
    goto Exit;
  }
  //
  // The IP Hop Limit field has a value of 255, i.e. the packet
  // could not possibly have been forwarded by a router.
  // ICMP Code is 0.
  // ICMP length (derived from the IP length) is 16 or more octets.
  //
  if (Head->HopLimit != IP6_HOP_LIMIT || Icmp.Head.Code != 0 ||
      Head->PayloadLength < IP6_RA_LENGTH) {
    goto Exit;
  }

  //
  // All included options have a length that is greater than zero.
  //
  OptionLen = (UINT16) (Head->PayloadLength - IP6_RA_LENGTH);
  if (OptionLen != 0) {
    Option    = NetbufGetByte (Packet, IP6_RA_LENGTH, NULL);
    ASSERT (Option != NULL);

    if (!Ip6IsNDOptionValid (Option, OptionLen)) {
      goto Exit;
    }
  }

  //
  // Process Fourth field.
  // In Router Advertisement, Fourth is composed of CurHopLimit (8bit), M flag, O flag,
  // and Router Lifetime (16 bit).
  //

  Fourth = NTOHL (Icmp.Fourth);
  CopyMem (&RouterLifetime, &Fourth, sizeof (UINT16));

  //
  // If the source address already in the default router list, update it.
  // Otherwise create a new entry.
  // A Lifetime of zero indicates that the router is not a default router.
  //
  DefaultRouter = Ip6FindDefaultRouter (IpSb, &Head->SourceAddress);
  if (DefaultRouter == NULL) {
    if (RouterLifetime != 0) {
      DefaultRouter = Ip6CreateDefaultRouter (IpSb, &Head->SourceAddress, RouterLifetime);
      if (DefaultRouter == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
    }
  } else {
    if (RouterLifetime != 0) {
      DefaultRouter->Lifetime = RouterLifetime;
      //
      // Check the corresponding neighbor cache entry here.
      //
      if (DefaultRouter->NeighborCache == NULL) {
        DefaultRouter->NeighborCache = Ip6FindNeighborEntry (IpSb, &Head->SourceAddress);
      }
    } else {
      //
      // If the address is in the host's default router list and the router lifetime is zero,
      // immediately time-out the entry.
      //
      Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
    }
  }

  CurHopLimit = *((UINT8 *) &Fourth + 3);
  if (CurHopLimit != 0) {
    IpSb->CurHopLimit = CurHopLimit;
  }

  Mflag = FALSE;
  Oflag = FALSE;
  if ((*((UINT8 *) &Fourth + 2) & IP6_M_ADDR_CONFIG_FLAG) == IP6_M_ADDR_CONFIG_FLAG) {
    Mflag = TRUE;
  } else {
    if ((*((UINT8 *) &Fourth + 2) & IP6_O_CONFIG_FLAG) == IP6_O_CONFIG_FLAG) {
      Oflag = TRUE;
    }
  }

  if (Mflag || Oflag) {
    //
    // Use Ip6Config to get available addresses or other configuration from DHCP.
    //
    Ip6ConfigStartStatefulAutoConfig (&IpSb->Ip6ConfigInstance, Oflag);
  }

  //
  // Process Reachable Time and Retrans Timer fields.
  //
  NetbufCopy (Packet, sizeof (Icmp), sizeof (UINT32), (UINT8 *) &ReachableTime);
  NetbufCopy (Packet, sizeof (Icmp) + sizeof (UINT32), sizeof (UINT32), (UINT8 *) &RetransTimer);
  ReachableTime = NTOHL (ReachableTime);
  RetransTimer  = NTOHL (RetransTimer);

  if (ReachableTime != 0 && ReachableTime != IpSb->BaseReachableTime) {
    //
    // If new value is not unspecified and differs from the previous one, record it
    // in BaseReachableTime and recompute a ReachableTime.
    //
    IpSb->BaseReachableTime = ReachableTime;
    Ip6UpdateReachableTime (IpSb);
  }

  if (RetransTimer != 0) {
    IpSb->RetransTimer = RetransTimer;
  }

  //
  // IsRouter flag must be set to TRUE if corresponding neighbor cache entry exists.
  //
  NeighborCache = Ip6FindNeighborEntry (IpSb, &Head->SourceAddress);
  if (NeighborCache != NULL) {
    NeighborCache->IsRouter = TRUE;
  }

  //
  // If an valid router advertisement is received, stops router solicitation.
  //
  IpSb->RouterAdvertiseReceived = TRUE;

  //
  // The only defined options that may appear are the Source
  // Link-Layer Address, Prefix information and MTU options.
  // All included options have a length that is greater than zero and
  // fit within the input packet.
  //
  Offset = 16;
  while (Offset < (UINT32) Head->PayloadLength) {
    NetbufCopy (Packet, Offset, sizeof (UINT8), &Type);
    switch (Type) {
    case Ip6OptionEtherSource:
      //
      // Update the neighbor cache
      //
      NetbufCopy (Packet, Offset, sizeof (IP6_ETHER_ADDR_OPTION), (UINT8 *) &LinkLayerOption);

      //
      // Option size validity ensured by Ip6IsNDOptionValid().
      //
      ASSERT (LinkLayerOption.Length != 0);
      ASSERT (Offset + (UINT32) LinkLayerOption.Length * 8 <= (UINT32) Head->PayloadLength);

      ZeroMem (&LinkLayerAddress, sizeof (EFI_MAC_ADDRESS));
      CopyMem (&LinkLayerAddress, LinkLayerOption.EtherAddr, 6);

      if (NeighborCache == NULL) {
        NeighborCache = Ip6CreateNeighborEntry (
                          IpSb,
                          Ip6OnArpResolved,
                          &Head->SourceAddress,
                          &LinkLayerAddress
                          );
        if (NeighborCache == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Exit;
        }
        NeighborCache->IsRouter = TRUE;
        NeighborCache->State    = EfiNeighborStale;
        NeighborCache->Ticks    = (UINT32) IP6_INFINIT_LIFETIME;
      } else {
        Result = CompareMem (&LinkLayerAddress, &NeighborCache->LinkAddress, 6);

        //
        // If the link-local address is the same as that already in the cache,
        // the cache entry's state remains unchanged. Otherwise update the
        // reachability state to STALE.
        //
        if ((NeighborCache->State == EfiNeighborInComplete) || (Result != 0)) {
          CopyMem (&NeighborCache->LinkAddress, &LinkLayerAddress, 6);

          NeighborCache->Ticks = (UINT32) IP6_INFINIT_LIFETIME;

          if (NeighborCache->State == EfiNeighborInComplete) {
            //
            // Send queued packets if exist.
            //
            NeighborCache->State = EfiNeighborStale;
            NeighborCache->CallBack ((VOID *) NeighborCache);
          } else {
            NeighborCache->State = EfiNeighborStale;
          }
        }
      }

      Offset += (UINT32) LinkLayerOption.Length * 8;
      break;
    case Ip6OptionPrefixInfo:
      NetbufCopy (Packet, Offset, sizeof (IP6_PREFIX_INFO_OPTION), (UINT8 *) &PrefixOption);

      //
      // Option size validity ensured by Ip6IsNDOptionValid().
      //
      ASSERT (PrefixOption.Length == 4);
      ASSERT (Offset + (UINT32) PrefixOption.Length * 8 <= (UINT32) Head->PayloadLength);

      PrefixOption.ValidLifetime     = NTOHL (PrefixOption.ValidLifetime);
      PrefixOption.PreferredLifetime = NTOHL (PrefixOption.PreferredLifetime);

      //
      // Get L and A flag, recorded in the lower 2 bits of Reserved1
      //
      OnLink = FALSE;
      if ((PrefixOption.Reserved1 & IP6_ON_LINK_FLAG) == IP6_ON_LINK_FLAG) {
        OnLink = TRUE;
      }
      Autonomous = FALSE;
      if ((PrefixOption.Reserved1 & IP6_AUTO_CONFIG_FLAG) == IP6_AUTO_CONFIG_FLAG) {
        Autonomous = TRUE;
      }

      //
      // If the prefix is the link-local prefix, silently ignore the prefix option.
      //
      if (PrefixOption.PrefixLength == IP6_LINK_LOCAL_PREFIX_LENGTH &&
          NetIp6IsLinkLocalAddr (&PrefixOption.Prefix)
          ) {
        Offset += sizeof (IP6_PREFIX_INFO_OPTION);
        break;
      }
      //
      // Do following if on-link flag is set according to RFC4861.
      //
      if (OnLink) {
        PrefixList = Ip6FindPrefixListEntry (
                       IpSb,
                       TRUE,
                       PrefixOption.PrefixLength,
                       &PrefixOption.Prefix
                       );
        //
        // Create a new entry for the prefix, if the ValidLifetime is zero,
        // silently ignore the prefix option.
        //
        if (PrefixList == NULL && PrefixOption.ValidLifetime != 0) {
          PrefixList = Ip6CreatePrefixListEntry (
                         IpSb,
                         TRUE,
                         PrefixOption.ValidLifetime,
                         PrefixOption.PreferredLifetime,
                         PrefixOption.PrefixLength,
                         &PrefixOption.Prefix
                         );
          if (PrefixList == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Exit;
          }
        } else if (PrefixList != NULL) {
          if (PrefixOption.ValidLifetime != 0) {
            PrefixList->ValidLifetime = PrefixOption.ValidLifetime;
          } else {
            //
            // If the prefix exists and incoming ValidLifetime is zero, immediately
            // remove the prefix.
            Ip6DestroyPrefixListEntry (IpSb, PrefixList, OnLink, TRUE);
          }
        }
      }

      //
      // Do following if Autonomous flag is set according to RFC4862.
      //
      if (Autonomous && PrefixOption.PreferredLifetime <= PrefixOption.ValidLifetime) {
        PrefixList = Ip6FindPrefixListEntry (
                       IpSb,
                       FALSE,
                       PrefixOption.PrefixLength,
                       &PrefixOption.Prefix
                       );
        //
        // Create a new entry for the prefix, and form an address by prefix + interface id
        // If the sum of the prefix length and interface identifier length
        // does not equal 128 bits, the Prefix Information option MUST be ignored.
        //
        if (PrefixList == NULL &&
            PrefixOption.ValidLifetime != 0 &&
            PrefixOption.PrefixLength + IpSb->InterfaceIdLen * 8 == 128
            ) {
          //
          // Form the address in network order.
          //
          CopyMem (&StatelessAddress, &PrefixOption.Prefix, sizeof (UINT64));
          CopyMem (&StatelessAddress.Addr[8], IpSb->InterfaceId, sizeof (UINT64));

          //
          // If the address is not yet in the assigned address list, adds it into.
          //
          if (!Ip6IsOneOfSetAddress (IpSb, &StatelessAddress, NULL, NULL)) {
            //
            // And also not in the DAD process, check its uniqueness firstly.
            //
            if (Ip6FindDADEntry (IpSb, &StatelessAddress, NULL) == NULL) {
              Status = Ip6SetAddress (
                         IpSb->DefaultInterface,
                         &StatelessAddress,
                         FALSE,
                         PrefixOption.PrefixLength,
                         PrefixOption.ValidLifetime,
                         PrefixOption.PreferredLifetime,
                         NULL,
                         NULL
                         );
              if (EFI_ERROR (Status)) {
                goto Exit;
              }
            }
          }

          //
          // Adds the prefix option to stateless prefix option list.
          //
          PrefixList = Ip6CreatePrefixListEntry (
                         IpSb,
                         FALSE,
                         PrefixOption.ValidLifetime,
                         PrefixOption.PreferredLifetime,
                         PrefixOption.PrefixLength,
                         &PrefixOption.Prefix
                         );
          if (PrefixList == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Exit;
          }
        } else if (PrefixList != NULL) {

          //
          // Reset the preferred lifetime of the address if the advertised prefix exists.
          // Perform specific action to valid lifetime together.
          //
          PrefixList->PreferredLifetime = PrefixOption.PreferredLifetime;
          if ((PrefixOption.ValidLifetime > 7200) ||
              (PrefixOption.ValidLifetime > PrefixList->ValidLifetime)) {
            //
            // If the received Valid Lifetime is greater than 2 hours or
            // greater than RemainingLifetime, set the valid lifetime of the
            // corresponding address to the advertised Valid Lifetime.
            //
            PrefixList->ValidLifetime = PrefixOption.ValidLifetime;

          } else if (PrefixList->ValidLifetime <= 7200) {
            //
            // If RemainingLifetime is less than or equals to 2 hours, ignore the
            // Prefix Information option with regards to the valid lifetime.
            // TODO: If this option has been authenticated, set the valid lifetime.
            //
          } else {
            //
            // Otherwise, reset the valid lifetime of the corresponding
            // address to 2 hours.
            //
            PrefixList->ValidLifetime = 7200;
          }
        }
      }

      Offset += sizeof (IP6_PREFIX_INFO_OPTION);
      break;
    case Ip6OptionMtu:
      NetbufCopy (Packet, Offset, sizeof (IP6_MTU_OPTION), (UINT8 *) &MTUOption);

      //
      // Option size validity ensured by Ip6IsNDOptionValid().
      //
      ASSERT (MTUOption.Length == 1);
      ASSERT (Offset + (UINT32) MTUOption.Length * 8 <= (UINT32) Head->PayloadLength);

      //
      // Use IPv6 minimum link MTU 1280 bytes as the maximum packet size in order
      // to omit implementation of Path MTU Discovery. Thus ignore the MTU option
      // in Router Advertisement.
      //

      Offset += sizeof (IP6_MTU_OPTION);
      break;
    default:
      //
      // Silently ignore unrecognized options
      //
      NetbufCopy (Packet, Offset + sizeof (UINT8), sizeof (UINT8), &Length);

      ASSERT (Length != 0);

      Offset += (UINT32) Length * 8;
      break;
    }
  }

  Status = EFI_SUCCESS;

Exit:
  NetbufFree (Packet);
  return Status;
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  )
{
  IP6_ICMP_INFORMATION_HEAD *Icmp;
  EFI_IPv6_ADDRESS          *Target;
  EFI_IPv6_ADDRESS          *IcmpDest;
  UINT8                     *Option;
  UINT16                    OptionLen;
  IP6_ROUTE_ENTRY           *RouteEntry;
  IP6_ROUTE_CACHE_ENTRY     *RouteCache;
  IP6_NEIGHBOR_ENTRY        *NeighborCache;
  INT32                     Length;
  UINT8                     OptLen;
  IP6_ETHER_ADDR_OPTION     *LinkLayerOption;
  EFI_MAC_ADDRESS           Mac;
  UINT32                    Index;
  BOOLEAN                   IsRouter;
  EFI_STATUS                Status;
  INTN                      Result;

  Status = EFI_INVALID_PARAMETER;

  Icmp = (IP6_ICMP_INFORMATION_HEAD *) NetbufGetByte (Packet, 0, NULL);
  if (Icmp == NULL) {
    goto Exit;
  }

  //
  // Validate the incoming Redirect message
  //

  //
  // The IP Hop Limit field has a value of 255, i.e. the packet
  // could not possibly have been forwarded by a router.
  // ICMP Code is 0.
  // ICMP length (derived from the IP length) is 40 or more octets.
  //
  if (Head->HopLimit != IP6_HOP_LIMIT || Icmp->Head.Code != 0 ||
      Head->PayloadLength < IP6_REDITECT_LENGTH) {
    goto Exit;
  }

  //
  // The IP source address must be a link-local address
  //
  if (!NetIp6IsLinkLocalAddr (&Head->SourceAddress)) {
    goto Exit;
  }

  //
  // The dest of this ICMP redirect message is not us.
  //
  if (!Ip6IsOneOfSetAddress (IpSb, &Head->DestinationAddress, NULL, NULL)) {
    goto Exit;
  }

  //
  // All included options have a length that is greater than zero.
  //
  OptionLen = (UINT16) (Head->PayloadLength - IP6_REDITECT_LENGTH);
  if (OptionLen != 0) {
    Option    = NetbufGetByte (Packet, IP6_REDITECT_LENGTH, NULL);
    ASSERT (Option != NULL);

    if (!Ip6IsNDOptionValid (Option, OptionLen)) {
      goto Exit;
    }
  }

  Target   = (EFI_IPv6_ADDRESS *) (Icmp + 1);
  IcmpDest = Target + 1;

  //
  // The ICMP Destination Address field in the redirect message does not contain
  // a multicast address.
  //
  if (IP6_IS_MULTICAST (IcmpDest)) {
    goto Exit;
  }

  //
  // The ICMP Target Address is either a link-local address (when redirected to
  // a router) or the same as the ICMP Destination Address (when redirected to
  // the on-link destination).
  //
  IsRouter = (BOOLEAN) !EFI_IP6_EQUAL (Target, IcmpDest);
  if (!NetIp6IsLinkLocalAddr (Target) && IsRouter) {
    goto Exit;
  }

  //
  // Check the options. The only interested option here is the target-link layer
  // address option.
  //
  Length          = Packet->TotalSize - 40;
  Option          = (UINT8 *) (IcmpDest + 1);
  LinkLayerOption = NULL;
  while (Length > 0) {
    switch (*Option) {
    case Ip6OptionEtherTarget:

      LinkLayerOption = (IP6_ETHER_ADDR_OPTION *) Option;
      OptLen          = LinkLayerOption->Length;
      if (OptLen != 1) {
        //
        // For ethernet, the length must be 1.
        //
        goto Exit;
      }
      break;

    default:

      OptLen = *(Option + 1);
      if (OptLen == 0) {
        //
        // A length of 0 is invalid.
        //
        goto Exit;
      }
      break;
    }

    Length -= 8 * OptLen;
    Option += 8 * OptLen;
  }

  if (Length != 0) {
    goto Exit;
  }

  //
  // The IP source address of the Redirect is the same as the current
  // first-hop router for the specified ICMP Destination Address.
  //
  RouteCache = Ip6FindRouteCache (IpSb->RouteTable, IcmpDest, &Head->DestinationAddress);
  if (RouteCache != NULL) {
    if (!EFI_IP6_EQUAL (&RouteCache->NextHop, &Head->SourceAddress)) {
      //
      // The source of this Redirect message must match the NextHop of the
      // corresponding route cache entry.
      //
      goto Exit;
    }

    //
    // Update the NextHop.
    //
    IP6_COPY_ADDRESS (&RouteCache->NextHop, Target);

    if (!IsRouter) {
      RouteEntry = (IP6_ROUTE_ENTRY *) RouteCache->Tag;
      RouteEntry->Flag = RouteEntry->Flag | IP6_DIRECT_ROUTE;
    }

  } else {
    //
    // Get the Route Entry.
    //
    RouteEntry = Ip6FindRouteEntry (IpSb->RouteTable, IcmpDest, NULL);
    if (RouteEntry == NULL) {
      RouteEntry = Ip6CreateRouteEntry (IcmpDest, 0, NULL);
      if (RouteEntry == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
    }

    if (!IsRouter) {
      RouteEntry->Flag = IP6_DIRECT_ROUTE;
    }

    //
    // Create a route cache for this.
    //
    RouteCache = Ip6CreateRouteCacheEntry (
                   IcmpDest,
                   &Head->DestinationAddress,
                   Target,
                   (UINTN) RouteEntry
                   );
    if (RouteCache == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // Insert the newly created route cache entry.
    //
    Index = IP6_ROUTE_CACHE_HASH (IcmpDest, &Head->DestinationAddress);
    InsertHeadList (&IpSb->RouteTable->Cache.CacheBucket[Index], &RouteCache->Link);
  }

  //
  // Try to locate the neighbor cache for the Target.
  //
  NeighborCache = Ip6FindNeighborEntry (IpSb, Target);

  if (LinkLayerOption != NULL) {
    if (NeighborCache == NULL) {
      //
      // Create a neighbor cache for the Target.
      //
      ZeroMem (&Mac, sizeof (EFI_MAC_ADDRESS));
      CopyMem (&Mac, LinkLayerOption->EtherAddr, 6);
      NeighborCache = Ip6CreateNeighborEntry (IpSb, Ip6OnArpResolved, Target, &Mac);
      if (NeighborCache == NULL) {
        //
        // Just report a success here. The neighbor cache can be created in
        // some other place.
        //
        Status = EFI_SUCCESS;
        goto Exit;
      }

      NeighborCache->State = EfiNeighborStale;
      NeighborCache->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
    } else {
      Result = CompareMem (LinkLayerOption->EtherAddr, &NeighborCache->LinkAddress, 6);

      //
      // If the link-local address is the same as that already in the cache,
      // the cache entry's state remains unchanged. Otherwise update the
      // reachability state to STALE.
      //
      if ((NeighborCache->State == EfiNeighborInComplete) || (Result != 0)) {
        CopyMem (&NeighborCache->LinkAddress, LinkLayerOption->EtherAddr, 6);

        NeighborCache->Ticks = (UINT32) IP6_INFINIT_LIFETIME;

        if (NeighborCache->State == EfiNeighborInComplete) {
          //
          // Send queued packets if exist.
          //
          NeighborCache->State = EfiNeighborStale;
          NeighborCache->CallBack ((VOID *) NeighborCache);
        } else {
          NeighborCache->State = EfiNeighborStale;
        }
      }
    }
  }

  if (NeighborCache != NULL && IsRouter) {
    //
    // The Target is a router, set IsRouter to TRUE.
    //
    NeighborCache->IsRouter = TRUE;
  }

  Status = EFI_SUCCESS;

Exit:
  NetbufFree (Packet);
  return Status;
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *TargetIp6Address,
  IN EFI_MAC_ADDRESS        *TargetLinkAddress OPTIONAL,
  IN UINT32                 Timeout,
  IN BOOLEAN                Override
  )
{
  IP6_NEIGHBOR_ENTRY        *Neighbor;

  Neighbor = Ip6FindNeighborEntry (IpSb, TargetIp6Address);
  if (Neighbor != NULL) {
    if (!Override) {
      return EFI_ACCESS_DENIED;
    } else {
      if (TargetLinkAddress != NULL) {
        IP6_COPY_LINK_ADDRESS (&Neighbor->LinkAddress, TargetLinkAddress);
      }
    }
  } else {
    if (TargetLinkAddress == NULL) {
      return EFI_NOT_FOUND;
    }

    Neighbor = Ip6CreateNeighborEntry (IpSb, Ip6OnArpResolved, TargetIp6Address, TargetLinkAddress);
    if (Neighbor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  Neighbor->State = EfiNeighborReachable;

  if (Timeout != 0) {
    Neighbor->Ticks   = IP6_GET_TICKS (Timeout / TICKS_PER_MS);
    Neighbor->Dynamic = TRUE;
  } else {
    Neighbor->Ticks   = (UINT32) IP6_INFINIT_LIFETIME;
  }

  return EFI_SUCCESS;
}

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
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *TargetIp6Address,
  IN EFI_MAC_ADDRESS        *TargetLinkAddress OPTIONAL,
  IN UINT32                 Timeout,
  IN BOOLEAN                Override
  )
{
  IP6_NEIGHBOR_ENTRY        *Neighbor;

  Neighbor = Ip6FindNeighborEntry (IpSb, TargetIp6Address);
  if (Neighbor == NULL) {
    return EFI_NOT_FOUND;
  }

  RemoveEntryList (&Neighbor->Link);
  FreePool (Neighbor);

  return EFI_SUCCESS;
}

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
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  LIST_ENTRY                *Entry2;
  IP6_INTERFACE             *IpIf;
  IP6_DELAY_JOIN_LIST       *DelayNode;
  EFI_IPv6_ADDRESS          Source;
  IP6_DAD_ENTRY             *DupAddrDetect;
  EFI_STATUS                Status;
  IP6_NEIGHBOR_ENTRY        *NeighborCache;
  EFI_IPv6_ADDRESS          Destination;
  IP6_SERVICE               *IpSb;
  BOOLEAN                   Flag;

  IpSb = (IP6_SERVICE *) Context;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  ZeroMem (&Source, sizeof (EFI_IPv6_ADDRESS));

  //
  // A host SHOULD transmit up to MAX_RTR_SOLICITATIONS (3) Router
  // Solicitation messages, each separated by at least
  // RTR_SOLICITATION_INTERVAL (4) seconds.
  //
  if ((IpSb->Ip6ConfigInstance.Policy == Ip6ConfigPolicyAutomatic) &&
      !IpSb->RouterAdvertiseReceived &&
      IpSb->SolicitTimer > 0
      ) {
    if ((IpSb->Ticks == 0) || (--IpSb->Ticks == 0)) {
      Status = Ip6SendRouterSolicit (IpSb, NULL, NULL, NULL, NULL);
      if (!EFI_ERROR (Status)) {
        IpSb->SolicitTimer--;
        IpSb->Ticks = (UINT32) IP6_GET_TICKS (IP6_RTR_SOLICITATION_INTERVAL);
      }
    }
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);

    //
    // Process the delay list to join the solicited-node multicast address.
    //
    NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DelayJoinList) {
      DelayNode = NET_LIST_USER_STRUCT (Entry2, IP6_DELAY_JOIN_LIST, Link);
      if ((DelayNode->DelayTime == 0) || (--DelayNode->DelayTime == 0)) {
        //
        // The timer expires, init the duplicate address detection.
        //
        Ip6InitDADProcess (
          DelayNode->Interface,
          DelayNode->AddressInfo,
          DelayNode->DadCallback,
          DelayNode->Context
          );

        //
        // Remove the delay node
        //
        RemoveEntryList (&DelayNode->Link);
        FreePool (DelayNode);
      }
    }

    //
    // Process the duplicate address detection list.
    //
    NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DupAddrDetectList) {
      DupAddrDetect = NET_LIST_USER_STRUCT (Entry2, IP6_DAD_ENTRY, Link);

      if ((DupAddrDetect->RetransTick == 0) || (--DupAddrDetect->RetransTick == 0)) {
        //
        // The timer expires, check the remaining transmit counts.
        //
        if (DupAddrDetect->Transmit < DupAddrDetect->MaxTransmit) {
          //
          // Send the Neighbor Solicitation message with
          // Source - unspecified address, destination - solicited-node multicast address
          // Target - the address to be validated
          //
          Status = Ip6SendNeighborSolicit (
                     IpSb,
                     NULL,
                     &DupAddrDetect->Destination,
                     &DupAddrDetect->AddressInfo->Address,
                     NULL
                     );
          if (EFI_ERROR (Status)) {
            return;
          }

          DupAddrDetect->Transmit++;
          DupAddrDetect->RetransTick = IP6_GET_TICKS (IpSb->RetransTimer);
        } else {
          //
          // All required solicitation has been sent out, and the RetransTime after the last
          // Neighbor Solicit is elapsed, finish the DAD process.
          //
          Flag = FALSE;
          if ((DupAddrDetect->Receive == 0) ||
              (DupAddrDetect->Transmit <= DupAddrDetect->Receive)) {
            Flag = TRUE;
          }

          Ip6OnDADFinished (Flag, IpIf, DupAddrDetect);
        }
      }
    }
  }

  //
  // Polling the state of Neighbor cache
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->NeighborTable) {
    NeighborCache = NET_LIST_USER_STRUCT (Entry, IP6_NEIGHBOR_ENTRY, Link);

    switch (NeighborCache->State) {
    case EfiNeighborInComplete:
      if (NeighborCache->Ticks > 0) {
        --NeighborCache->Ticks;
      }

      //
      // Retransmit Neighbor Solicitation messages approximately every
      // RetransTimer milliseconds while awaiting a response.
      //
      if (NeighborCache->Ticks == 0) {
        if (NeighborCache->Transmit > 1) {
          //
          // Send out multicast neighbor solicitation for address resolution.
          // After last neighbor solicitation message has been sent out, wait
          // for RetransTimer and then remove entry if no response is received.
          //
          Ip6CreateSNMulticastAddr (&NeighborCache->Neighbor, &Destination);
          Status = Ip6SelectSourceAddress (IpSb, &NeighborCache->Neighbor, &Source);
          if (EFI_ERROR (Status)) {
            return;
          }

          Status = Ip6SendNeighborSolicit (
                     IpSb,
                     &Source,
                     &Destination,
                     &NeighborCache->Neighbor,
                     &IpSb->SnpMode.CurrentAddress
                     );
          if (EFI_ERROR (Status)) {
            return;
          }
        }

        //
        // Update the retransmit times.
        //
        if (NeighborCache->Transmit > 0) {
          --NeighborCache->Transmit;
          NeighborCache->Ticks = IP6_GET_TICKS (IpSb->RetransTimer);
        }
      }

      if (NeighborCache->Transmit == 0) {
        //
        // Timeout, send ICMP destination unreachable packet and then remove entry
        //
        Status = Ip6FreeNeighborEntry (
                   IpSb,
                   NeighborCache,
                   TRUE,
                   TRUE,
                   EFI_ICMP_ERROR,
                   NULL,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          return;
        }
      }

      break;

    case EfiNeighborReachable:
      //
      // This entry is inserted by EfiIp6Neighbors() as static entry
      // and will not timeout.
      //
      if (!NeighborCache->Dynamic && (NeighborCache->Ticks == IP6_INFINIT_LIFETIME)) {
        break;
      }

      if ((NeighborCache->Ticks == 0) || (--NeighborCache->Ticks == 0)) {
        if (NeighborCache->Dynamic) {
          //
          // This entry is inserted by EfiIp6Neighbors() as dynamic entry
          // and will be deleted after timeout.
          //
          Status = Ip6FreeNeighborEntry (
                     IpSb,
                     NeighborCache,
                     FALSE,
                     TRUE,
                     EFI_TIMEOUT,
                     NULL,
                     NULL
                     );
          if (EFI_ERROR (Status)) {
            return;
          }
        } else {
          NeighborCache->State = EfiNeighborStale;
          NeighborCache->Ticks = (UINT32) IP6_INFINIT_LIFETIME;
        }
      }

      break;

    case EfiNeighborDelay:
      if ((NeighborCache->Ticks == 0) || (--NeighborCache->Ticks == 0)) {

        NeighborCache->State    = EfiNeighborProbe;
        NeighborCache->Ticks    = IP6_GET_TICKS (IpSb->RetransTimer);
        NeighborCache->Transmit = IP6_MAX_UNICAST_SOLICIT + 1;
        //
        // Send out unicast neighbor solicitation for Neighbor Unreachability Detection
        //
        Status = Ip6SelectSourceAddress (IpSb, &NeighborCache->Neighbor, &Source);
        if (EFI_ERROR (Status)) {
          return;
        }

        Status = Ip6SendNeighborSolicit (
                   IpSb,
                   &Source,
                   &NeighborCache->Neighbor,
                   &NeighborCache->Neighbor,
                   &IpSb->SnpMode.CurrentAddress
                   );
        if (EFI_ERROR (Status)) {
          return;
        }

        NeighborCache->Transmit--;
      }

      break;

    case EfiNeighborProbe:
      if (NeighborCache->Ticks > 0) {
        --NeighborCache->Ticks;
      }

      //
      // Retransmit Neighbor Solicitation messages approximately every
      // RetransTimer milliseconds while awaiting a response.
      //
      if (NeighborCache->Ticks == 0) {
        if (NeighborCache->Transmit > 1) {
          //
          // Send out unicast neighbor solicitation for Neighbor Unreachability
          // Detection. After last neighbor solicitation message has been sent out,
          // wait for RetransTimer and then remove entry if no response is received.
          //
          Status = Ip6SelectSourceAddress (IpSb, &NeighborCache->Neighbor, &Source);
          if (EFI_ERROR (Status)) {
            return;
          }

          Status = Ip6SendNeighborSolicit (
                     IpSb,
                     &Source,
                     &NeighborCache->Neighbor,
                     &NeighborCache->Neighbor,
                     &IpSb->SnpMode.CurrentAddress
                     );
          if (EFI_ERROR (Status)) {
            return;
          }
        }

        //
        // Update the retransmit times.
        //
        if (NeighborCache->Transmit > 0) {
          --NeighborCache->Transmit;
          NeighborCache->Ticks = IP6_GET_TICKS (IpSb->RetransTimer);
        }
      }

      if (NeighborCache->Transmit == 0) {
        //
        // Delete the neighbor entry.
        //
        Status = Ip6FreeNeighborEntry (
                   IpSb,
                   NeighborCache,
                   FALSE,
                   TRUE,
                   EFI_TIMEOUT,
                   NULL,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          return;
        }
      }

      break;

    default:
      break;
    }
  }
}

/**
  The heartbeat timer of ND module in 1 second. This time routine handles following
  things: 1) maintain default router list; 2) maintain prefix options;
  3) maintain route caches.

  @param[in]  IpSb              The IP6 service binding instance.

**/
VOID
Ip6NdTimerTicking (
  IN IP6_SERVICE            *IpSb
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP6_DEFAULT_ROUTER        *DefaultRouter;
  IP6_PREFIX_LIST_ENTRY     *PrefixOption;
  UINT8                     Index;
  IP6_ROUTE_CACHE_ENTRY     *RouteCache;

  //
  // Decrease the lifetime of default router, if expires remove it from default router list.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->DefaultRouterList) {
    DefaultRouter = NET_LIST_USER_STRUCT (Entry, IP6_DEFAULT_ROUTER, Link);
    if (DefaultRouter->Lifetime != IP6_INF_ROUTER_LIFETIME) {
      if ((DefaultRouter->Lifetime == 0) || (--DefaultRouter->Lifetime == 0)) {
        Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
      }
    }
  }

  //
  // Decrease Valid lifetime and Preferred lifetime of Prefix options and corresponding addresses.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->AutonomousPrefix) {
    PrefixOption = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);
    if (PrefixOption->ValidLifetime != (UINT32) IP6_INFINIT_LIFETIME) {
      if ((PrefixOption->ValidLifetime > 0) && (--PrefixOption->ValidLifetime > 0)) {
        if ((PrefixOption->PreferredLifetime != (UINT32) IP6_INFINIT_LIFETIME) &&
            (PrefixOption->PreferredLifetime > 0)
            ) {
          --PrefixOption->PreferredLifetime;
        }
      } else {
        Ip6DestroyPrefixListEntry (IpSb, PrefixOption, FALSE, TRUE);
      }
    }
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->OnlinkPrefix) {
    PrefixOption = NET_LIST_USER_STRUCT (Entry, IP6_PREFIX_LIST_ENTRY, Link);
    if (PrefixOption->ValidLifetime != (UINT32) IP6_INFINIT_LIFETIME) {
      if ((PrefixOption->ValidLifetime == 0) || (--PrefixOption->ValidLifetime == 0)) {
        Ip6DestroyPrefixListEntry (IpSb, PrefixOption, TRUE, TRUE);
      }
    }
  }

  //
  // Each bucket of route cache can contain at most IP6_ROUTE_CACHE_MAX entries.
  // Remove the entries at the tail of the bucket. These entries
  // are likely to be used least.
  // Reclaim frequency is set to 1 second.
  //
  for (Index = 0; Index < IP6_ROUTE_CACHE_HASH_SIZE; Index++) {
    while (IpSb->RouteTable->Cache.CacheNum[Index] > IP6_ROUTE_CACHE_MAX) {
      Entry = NetListRemoveTail (&IpSb->RouteTable->Cache.CacheBucket[Index]);
      if (Entry == NULL) {
        break;
      }

      RouteCache = NET_LIST_USER_STRUCT (Entry, IP6_ROUTE_CACHE_ENTRY, Link);
      Ip6FreeRouteCacheEntry (RouteCache);
      ASSERT (IpSb->RouteTable->Cache.CacheNum[Index] > 0);
      IpSb->RouteTable->Cache.CacheNum[Index]--;
    }
  }
}

