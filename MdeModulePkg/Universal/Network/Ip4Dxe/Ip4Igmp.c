/** @file
  This file implements the RFC2236: IGMP v2.
  
Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4Impl.h"

//
// Route Alert option in IGMP report to direct routers to
// examine the packet more closely.
//
UINT32  mRouteAlertOption = 0x00000494;


/**
  Init the IGMP control data of the IP4 service instance, configure
  MNP to receive ALL SYSTEM multicast.

  @param[in, out]  IpSb          The IP4 service whose IGMP is to be initialized.

  @retval EFI_SUCCESS            IGMP of the IpSb is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to initialize IGMP.
  @retval Others                 Failed to initialize the IGMP of IpSb.

**/
EFI_STATUS
Ip4InitIgmp (
  IN OUT IP4_SERVICE            *IpSb
  )
{
  IGMP_SERVICE_DATA             *IgmpCtrl;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  IGMP_GROUP                    *Group;
  EFI_STATUS                    Status;

  IgmpCtrl = &IpSb->IgmpCtrl;

  //
  // Configure MNP to receive ALL_SYSTEM multicast
  //
  Group    = AllocatePool (sizeof (IGMP_GROUP));

  if (Group == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Mnp               = IpSb->Mnp;

  Group->Address    = IP4_ALLSYSTEM_ADDRESS;
  Group->RefCnt     = 1;
  Group->DelayTime  = 0;
  Group->ReportByUs = FALSE;

  Status = Ip4GetMulticastMac (Mnp, IP4_ALLSYSTEM_ADDRESS, &Group->Mac);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->Groups (Mnp, TRUE, &Group->Mac);

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  InsertHeadList (&IgmpCtrl->Groups, &Group->Link);
  return EFI_SUCCESS;

ON_ERROR:
  FreePool (Group);
  return Status;
}


/**
  Find the IGMP_GROUP structure which contains the status of multicast
  group Address in this IGMP control block

  @param[in]  IgmpCtrl               The IGMP control block to search from
  @param[in]  Address                The multicast address to search

  @return NULL if the multicast address isn't in the IGMP control block. Otherwise
          the point to the IGMP_GROUP which contains the status of multicast group
          for Address.

**/
IGMP_GROUP *
Ip4FindGroup (
  IN IGMP_SERVICE_DATA      *IgmpCtrl,
  IN IP4_ADDR               Address
  )
{
  LIST_ENTRY                *Entry;
  IGMP_GROUP                *Group;

  NET_LIST_FOR_EACH (Entry, &IgmpCtrl->Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IGMP_GROUP, Link);

    if (Group->Address == Address) {
      return Group;
    }
  }

  return NULL;
}


/**
  Count the number of IP4 multicast groups that are mapped to the
  same MAC address. Several IP4 multicast address may be mapped to
  the same MAC address.

  @param[in]  IgmpCtrl               The IGMP control block to search in
  @param[in]  Mac                    The MAC address to search

  @return The number of the IP4 multicast group that mapped to the same
          multicast group Mac.

**/
INTN
Ip4FindMac (
  IN IGMP_SERVICE_DATA      *IgmpCtrl,
  IN EFI_MAC_ADDRESS        *Mac
  )
{
  LIST_ENTRY                *Entry;
  IGMP_GROUP                *Group;
  INTN                      Count;

  Count = 0;

  NET_LIST_FOR_EACH (Entry, &IgmpCtrl->Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IGMP_GROUP, Link);

    if (NET_MAC_EQUAL (&Group->Mac, Mac, sizeof (EFI_MAC_ADDRESS))) {
      Count++;
    }
  }

  return Count;
}


/**
  Send an IGMP protocol message to the Dst, such as IGMP v1 membership report.

  @param[in]  IpSb               The IP4 service instance that requests the
                                 transmission
  @param[in]  Dst                The destinaton to send to
  @param[in]  Type               The IGMP message type, such as IGMP v1 membership
                                 report
  @param[in]  Group              The group address in the IGMP message head.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to build the message
  @retval EFI_SUCCESS            The IGMP message is successfully send
  @retval Others                 Failed to send the IGMP message.

**/
EFI_STATUS
Ip4SendIgmpMessage (
  IN IP4_SERVICE            *IpSb,
  IN IP4_ADDR               Dst,
  IN UINT8                  Type,
  IN IP4_ADDR               Group
  )
{
  IP4_HEAD                  Head;
  NET_BUF                   *Packet;
  IGMP_HEAD                 *Igmp;

  //
  // Allocate a net buffer to hold the message
  //
  Packet = NetbufAlloc (IP4_MAX_HEADLEN + sizeof (IGMP_HEAD));

  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the IGMP and IP header, then transmit the message
  //
  NetbufReserve (Packet, IP4_MAX_HEADLEN);

  Igmp = (IGMP_HEAD *) NetbufAllocSpace (Packet, sizeof (IGMP_HEAD), FALSE);
  if (Igmp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Igmp->Type        = Type;
  Igmp->MaxRespTime = 0;
  Igmp->Checksum    = 0;
  Igmp->Group       = HTONL (Group);
  Igmp->Checksum    = (UINT16) (~NetblockChecksum ((UINT8 *) Igmp, sizeof (IGMP_HEAD)));

  Head.Tos          = 0;
  Head.Protocol     = IP4_PROTO_IGMP;
  Head.Ttl          = 1;
  Head.Fragment     = 0;
  Head.Dst          = Dst;
  Head.Src          = IP4_ALLZERO_ADDRESS;

  return Ip4Output (
           IpSb,
           NULL,
           Packet,
           &Head,
           (UINT8 *) &mRouteAlertOption,
           sizeof (UINT32),
           IP4_ALLZERO_ADDRESS,
           Ip4SysPacketSent,
           NULL
           );
}


/**
  Send an IGMP membership report. Depends on whether the server is
  v1 or v2, it will send either a V1 or V2 membership report.

  @param[in]  IpSb               The IP4 service instance that requests the
                                 transmission.
  @param[in]  Group              The group address to report

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to build the message
  @retval EFI_SUCCESS            The IGMP report message is successfully send
  @retval Others                 Failed to send the report.

**/
EFI_STATUS
Ip4SendIgmpReport (
  IN IP4_SERVICE            *IpSb,
  IN IP4_ADDR               Group
  )
{
  if (IpSb->IgmpCtrl.Igmpv1QuerySeen != 0) {
    return Ip4SendIgmpMessage (IpSb, Group, IGMP_V1_MEMBERSHIP_REPORT, Group);
  } else {
    return Ip4SendIgmpMessage (IpSb, Group, IGMP_V2_MEMBERSHIP_REPORT, Group);
  }
}


/**
  Join the multicast group on behalf of this IP4 child

  @param[in]  IpInstance         The IP4 child that wants to join the group
  @param[in]  Address            The group to join

  @retval EFI_SUCCESS            Successfully join the multicast group
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources
  @retval Others                 Failed to join the multicast group.

**/
EFI_STATUS
Ip4JoinGroup (
  IN IP4_PROTOCOL           *IpInstance,
  IN IP4_ADDR               Address
  )
{
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  IP4_SERVICE                   *IpSb;
  IGMP_SERVICE_DATA             *IgmpCtrl;
  IGMP_GROUP                    *Group;
  EFI_STATUS                    Status;

  IpSb      = IpInstance->Service;
  IgmpCtrl  = &IpSb->IgmpCtrl;
  Mnp       = IpSb->Mnp;

  //
  // If the IP service already is a member in the group, just
  // increase the refernce count and return.
  //
  Group     = Ip4FindGroup (IgmpCtrl, Address);

  if (Group != NULL) {
    Group->RefCnt++;
    return EFI_SUCCESS;
  }

  //
  // Otherwise, create a new IGMP_GROUP,  Get the multicast's MAC address,
  // send a report, then direct MNP to receive the multicast.
  //
  Group = AllocatePool (sizeof (IGMP_GROUP));

  if (Group == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Group->Address    = Address;
  Group->RefCnt     = 1;
  Group->DelayTime  = IGMP_UNSOLICIATED_REPORT;
  Group->ReportByUs = TRUE;

  Status = Ip4GetMulticastMac (Mnp, Address, &Group->Mac);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Ip4SendIgmpReport (IpSb, Address);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->Groups (Mnp, TRUE, &Group->Mac);

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ON_ERROR;
  }

  InsertHeadList (&IgmpCtrl->Groups, &Group->Link);
  return EFI_SUCCESS;

ON_ERROR:
  FreePool (Group);
  return Status;
}


/**
  Leave the IP4 multicast group on behalf of IpInstance.

  @param[in]  IpInstance         The IP4 child that wants to leave the group
                                 address
  @param[in]  Address            The group address to leave

  @retval EFI_NOT_FOUND          The IP4 service instance isn't in the group
  @retval EFI_SUCCESS            Successfully leave the multicast group.
  @retval Others                 Failed to leave the multicast group.

**/
EFI_STATUS
Ip4LeaveGroup (
  IN IP4_PROTOCOL           *IpInstance,
  IN IP4_ADDR               Address
  )
{
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  IP4_SERVICE                   *IpSb;
  IGMP_SERVICE_DATA             *IgmpCtrl;
  IGMP_GROUP                    *Group;
  EFI_STATUS                    Status;

  IpSb      = IpInstance->Service;
  IgmpCtrl  = &IpSb->IgmpCtrl;
  Mnp       = IpSb->Mnp;

  Group     = Ip4FindGroup (IgmpCtrl, Address);

  if (Group == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // If more than one instance is in the group, decrease
  // the RefCnt then return.
  //
  if (--Group->RefCnt > 0) {
    return EFI_SUCCESS;
  }

  //
  // If multiple IP4 group addresses are mapped to the same
  // multicast MAC address, don't configure the MNP to leave
  // the MAC.
  //
  if (Ip4FindMac (IgmpCtrl, &Group->Mac) == 1) {
    Status = Mnp->Groups (Mnp, FALSE, &Group->Mac);

    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      return Status;
    }
  }

  //
  // Send a leave report if the membership is reported by us
  // and we are talking IGMPv2.
  //
  if (Group->ReportByUs && IgmpCtrl->Igmpv1QuerySeen == 0) {
    Ip4SendIgmpMessage (IpSb, IP4_ALLROUTER_ADDRESS, IGMP_LEAVE_GROUP, Group->Address);
  }

  RemoveEntryList (&Group->Link);
  FreePool (Group);

  return EFI_SUCCESS;
}


/**
  Handle the received IGMP message for the IP4 service instance.

  @param[in]  IpSb               The IP4 service instance that received the message
  @param[in]  Head               The IP4 header of the received message
  @param[in]  Packet             The IGMP message, without IP4 header

  @retval EFI_INVALID_PARAMETER  The IGMP message is malformated.
  @retval EFI_SUCCESS            The IGMP message is successfully processed.

**/
EFI_STATUS
Ip4IgmpHandle (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IGMP_SERVICE_DATA         *IgmpCtrl;
  IGMP_HEAD                 Igmp;
  IGMP_GROUP                *Group;
  IP4_ADDR                  Address;
  LIST_ENTRY                *Entry;

  IgmpCtrl = &IpSb->IgmpCtrl;

  //
  // Must checksum over the whole packet, later IGMP version
  // may employ message longer than 8 bytes. IP's header has
  // already been trimmed off.
  //
  if ((Packet->TotalSize < sizeof (Igmp)) || (NetbufChecksum (Packet) != 0)) {
    NetbufFree (Packet);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Copy the packet in case it is fragmented
  //
  NetbufCopy (Packet, 0, sizeof (IGMP_HEAD), (UINT8 *)&Igmp);

  switch (Igmp.Type) {
  case IGMP_MEMBERSHIP_QUERY:
    //
    // If MaxRespTime is zero, it is most likely that we are
    // talking to a V1 router
    //
    if (Igmp.MaxRespTime == 0) {
      IgmpCtrl->Igmpv1QuerySeen = IGMP_V1ROUTER_PRESENT;
      Igmp.MaxRespTime          = 100;
    }

    //
    // Igmp is ticking once per second but MaxRespTime is in
    // the unit of 100ms.
    //
    Igmp.MaxRespTime /= 10;
    Address = NTOHL (Igmp.Group);

    if (Address == IP4_ALLSYSTEM_ADDRESS) {
      break;
    }

    NET_LIST_FOR_EACH (Entry, &IgmpCtrl->Groups) {
      Group = NET_LIST_USER_STRUCT (Entry, IGMP_GROUP, Link);

      //
      // If address is all zero, all the memberships will be reported.
      // otherwise only one is reported.
      //
      if ((Address == IP4_ALLZERO_ADDRESS) || (Address == Group->Address)) {
        //
        // If the timer is pending, only update it if the time left
        // is longer than the MaxRespTime. TODO: randomize the DelayTime.
        //
        if ((Group->DelayTime == 0) || (Group->DelayTime > Igmp.MaxRespTime)) {
          Group->DelayTime = MAX (1, Igmp.MaxRespTime);
        }
      }
    }

    break;

  case IGMP_V1_MEMBERSHIP_REPORT:
  case IGMP_V2_MEMBERSHIP_REPORT:
    Address = NTOHL (Igmp.Group);
    Group   = Ip4FindGroup (IgmpCtrl, Address);

    if ((Group != NULL) && (Group->DelayTime > 0)) {
      Group->DelayTime  = 0;
      Group->ReportByUs = FALSE;
    }

    break;
  }

  NetbufFree (Packet);
  return EFI_SUCCESS;
}


/**
  The periodical timer function for IGMP. It does the following
  things:
  1. Decrease the Igmpv1QuerySeen to make it possible to refresh
     the IGMP server type.
  2. Decrease the report timer for each IGMP group in "delaying
     member" state.

  @param[in]  IpSb                   The IP4 service instance that is ticking

**/
VOID
Ip4IgmpTicking (
  IN IP4_SERVICE            *IpSb
  )
{
  IGMP_SERVICE_DATA         *IgmpCtrl;
  LIST_ENTRY                *Entry;
  IGMP_GROUP                *Group;

  IgmpCtrl = &IpSb->IgmpCtrl;

  if (IgmpCtrl->Igmpv1QuerySeen > 0) {
    IgmpCtrl->Igmpv1QuerySeen--;
  }

  //
  // Decrease the report timer for each IGMP group in "delaying member"
  //
  NET_LIST_FOR_EACH (Entry, &IgmpCtrl->Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IGMP_GROUP, Link);
    ASSERT (Group->DelayTime >= 0);

    if (Group->DelayTime > 0) {
      Group->DelayTime--;

      if (Group->DelayTime == 0) {
        Ip4SendIgmpReport (IpSb, Group->Address);
        Group->ReportByUs = TRUE;
      }
    }
  }
}


/**
  Add a group address to the array of group addresses.
  The caller should make sure that no duplicated address
  existed in the array. Although the function doesn't
  assume the byte order of the both Source and Addr, the
  network byte order is used by the caller.

  @param[in]  Source                 The array of group addresses to add to
  @param[in]  Count                  The number of group addresses in the Source
  @param[in]  Addr                   The IP4 multicast address to add

  @return NULL if failed to allocate memory for the new groups,
          otherwise the new combined group addresses.

**/
IP4_ADDR *
Ip4CombineGroups (
  IN  IP4_ADDR              *Source,
  IN  UINT32                Count,
  IN  IP4_ADDR              Addr
  )
{
  IP4_ADDR                  *Groups;

  Groups = AllocatePool (sizeof (IP4_ADDR) * (Count + 1));

  if (Groups == NULL) {
    return NULL;
  }

  CopyMem (Groups, Source, Count * sizeof (IP4_ADDR));
  Groups[Count] = Addr;

  return Groups;
}


/**
  Remove a group address from the array of group addresses.
  Although the function doesn't assume the byte order of the
  both Groups and Addr, the network byte order is used by
  the caller.

  @param  Groups            The array of group addresses to remove from
  @param  Count             The number of group addresses in the Groups
  @param  Addr              The IP4 multicast address to remove

  @return The nubmer of group addresses in the Groups after remove.
          It is Count if the Addr isn't in the Groups.

**/
INTN
Ip4RemoveGroupAddr (
  IN OUT IP4_ADDR               *Groups,
  IN     UINT32                 Count,
  IN     IP4_ADDR               Addr
  )
{
  UINT32                    Index;

  for (Index = 0; Index < Count; Index++) {
    if (Groups[Index] == Addr) {
      break;
    }
  }

  while (Index < Count - 1) {
    Groups[Index] = Groups[Index + 1];
    Index++;
  }

  return Index;
}
