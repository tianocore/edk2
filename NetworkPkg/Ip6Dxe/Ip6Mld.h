/** @file
  Multicast Listener Discovery support routines.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_MLD_H__
#define __EFI_IP6_MLD_H__

#define IP6_UNSOLICITED_REPORT_INTERVAL 10

#pragma pack(1)
typedef struct {
  IP6_ICMP_HEAD           Head;
  UINT16                  MaxRespDelay;
  UINT16                  Reserved;
  EFI_IPv6_ADDRESS        Group;
} IP6_MLD_HEAD;
#pragma pack()

//
// The status of multicast group. It isn't necessary to maintain
// explicit state of host state diagram. A group with finity
// DelayTime (less than 0xffffffff) is in "delaying listener" state. otherwise, it is in
// "idle listener" state.
//
typedef struct {
  LIST_ENTRY              Link;
  INTN                    RefCnt;
  EFI_IPv6_ADDRESS        Address;
  UINT32                  DelayTimer;
  BOOLEAN                 SendByUs;
  EFI_MAC_ADDRESS         Mac;
} IP6_MLD_GROUP;

//
// The MLD status. Each IP6 service instance has a MLD_SERVICE_DATA
// attached. The Mldv1QuerySeen remember whether the server on this
// connected network is v1 or v2.
//
typedef struct {
  INTN                    Mldv1QuerySeen;
  LIST_ENTRY              Groups;
} IP6_MLD_SERVICE_DATA;

/**
  Search a IP6_MLD_GROUP list entry node from a list array.

  @param[in]       IpSb          Points to an IP6 service binding instance.
  @param[in]       MulticastAddr The IPv6 multicast address to be searched.

  @return The found IP6_ML_GROUP list entry or NULL.

**/
IP6_MLD_GROUP *
Ip6FindMldEntry (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *MulticastAddr
  );

/**
  Init the MLD data of the IP6 service instance, configure
  MNP to receive ALL SYSTEM multicasts.

  @param[in]  IpSb              The IP6 service whose MLD is to be initialized.

  @retval EFI_OUT_OF_RESOURCES  There are not sufficient resources to complete the
                                operation.
  @retval EFI_SUCCESS           The MLD module successfully initialized.

**/
EFI_STATUS
Ip6InitMld (
  IN IP6_SERVICE            *IpSb
  );

/**
  Join the multicast group on behalf of this IP6 service binding instance.

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  Interface          Points to an IP6_INTERFACE structure.
  @param[in]  Address            The group address to join.

  @retval EFI_SUCCESS            Successfully joined the multicast group.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval Others                 Failed to join the multicast group.

**/
EFI_STATUS
Ip6JoinGroup (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface,
  IN EFI_IPv6_ADDRESS       *Address
  );

/**
  Leave the IP6 multicast group.

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  Address            The group address to leave.

  @retval EFI_NOT_FOUND          The IP6 service instance isn't in the group.
  @retval EFI_SUCCESS            Successfully left the multicast group.
  @retval Others                 Failed to leave the multicast group.

**/
EFI_STATUS
Ip6LeaveGroup (
 IN IP6_SERVICE            *IpSb,
 IN EFI_IPv6_ADDRESS       *Address
  );

/**
  Worker function for EfiIp6Groups(). The caller
  should verify that the parameters are valid.

  @param[in]  IpInstance        The IP6 child to change the setting.
  @param[in]  JoinFlag          TRUE to join the group, otherwise leave it.
  @param[in]  GroupAddress      The target group address. If NULL, leave all
                                the group addresses.

  @retval EFI_ALREADY_STARTED   Wants to join the group, but is already a member of it.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate some resources.
  @retval EFI_DEVICE_ERROR      Failed to set the group configuraton.
  @retval EFI_SUCCESS           Successfully updated the group setting.
  @retval EFI_NOT_FOUND         Tried to leave a group of whom it isn't a member.

**/
EFI_STATUS
Ip6Groups (
  IN IP6_PROTOCOL           *IpInstance,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv6_ADDRESS       *GroupAddress       OPTIONAL
  );

/**
  Process the Multicast Listener Query message.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the MLD query packet.
  @param[in]  Packet             The content of the MLD query packet with IP head
                                 removed.

  @retval EFI_SUCCESS            The MLD query packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.
  @retval Others                 Failed to process the packet.

**/
EFI_STATUS
Ip6ProcessMldQuery (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  );

/**
  Process the Multicast Listener Report message.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the MLD report packet.
  @param[in]  Packet             The content of the MLD report packet with IP head
                                 removed.

  @retval EFI_SUCCESS            The MLD report packet processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid.

**/
EFI_STATUS
Ip6ProcessMldReport (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  );


/**
  The heartbeat timer of the MLD module. It sends out solicited MLD report when
  DelayTimer expires.

  @param[in]  IpSb              The IP6 service binding instance.

**/
VOID
Ip6MldTimerTicking (
  IN IP6_SERVICE            *IpSb
  );

#endif

