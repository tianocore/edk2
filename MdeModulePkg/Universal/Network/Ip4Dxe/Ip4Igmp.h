/** @file

Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP4_IGMP_H__
#define __EFI_IP4_IGMP_H__

//
// IGMP message type
//
#define IGMP_MEMBERSHIP_QUERY      0x11
#define IGMP_V1_MEMBERSHIP_REPORT  0x12
#define IGMP_V2_MEMBERSHIP_REPORT  0x16
#define IGMP_LEAVE_GROUP           0x17

#define IGMP_V1ROUTER_PRESENT      400
#define IGMP_UNSOLICIATED_REPORT   10

#pragma pack(1)
typedef struct {
  UINT8                   Type;
  UINT8                   MaxRespTime;
  UINT16                  Checksum;
  IP4_ADDR                Group;
} IGMP_HEAD;
#pragma pack()

///
/// The status of multicast group. It isn't necessary to maintain
/// explicit state of host state diagram. A group with non-zero
/// DelayTime is in "delaying member" state. otherwise, it is in
/// "idle member" state.
///
typedef struct {
  LIST_ENTRY              Link;
  INTN                    RefCnt;
  IP4_ADDR                Address;
  INTN                    DelayTime;
  BOOLEAN                 ReportByUs;
  EFI_MAC_ADDRESS         Mac;
} IGMP_GROUP;

///
/// The IGMP status. Each IP4 service instance has a IGMP_SERVICE_DATA
/// attached. The Igmpv1QuerySeen remember whether the server on this
/// connected network is v1 or v2.
///
typedef struct {
  INTN                    Igmpv1QuerySeen;
  LIST_ENTRY              Groups;
} IGMP_SERVICE_DATA;

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );
#endif
