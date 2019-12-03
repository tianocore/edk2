/** @file
  Multicast Listener Discovery support routines.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

/**
  Create a IP6_MLD_GROUP list entry node and record to IP6 service binding data.

  @param[in, out]  IpSb          Points to IP6 service binding instance.
  @param[in]       MulticastAddr The IPv6 multicast address to be recorded.
  @param[in]       DelayTimer    The maximum allowed delay before sending a responding
                                 report, in units of milliseconds.
  @return The created IP6_ML_GROUP list entry or NULL.

**/
IP6_MLD_GROUP *
Ip6CreateMldEntry (
  IN OUT IP6_SERVICE        *IpSb,
  IN EFI_IPv6_ADDRESS       *MulticastAddr,
  IN UINT32                 DelayTimer
  )
{
  IP6_MLD_GROUP             *Entry;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (MulticastAddr != NULL && IP6_IS_MULTICAST (MulticastAddr));

  Entry = AllocatePool (sizeof (IP6_MLD_GROUP));
  if (Entry != NULL) {
    Entry->RefCnt     = 1;
    Entry->DelayTimer = DelayTimer;
    Entry->SendByUs   = FALSE;
    IP6_COPY_ADDRESS (&Entry->Address, MulticastAddr);
    InsertTailList (&IpSb->MldCtrl.Groups, &Entry->Link);
  }

  return Entry;
}

/**
  Search a IP6_MLD_GROUP list entry node from a list array.

  @param[in]       IpSb          Points to IP6 service binding instance.
  @param[in]       MulticastAddr The IPv6 multicast address to be searched.

  @return The found IP6_ML_GROUP list entry or NULL.

**/
IP6_MLD_GROUP *
Ip6FindMldEntry (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *MulticastAddr
  )
{
  LIST_ENTRY                *Entry;
  IP6_MLD_GROUP             *Group;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (MulticastAddr != NULL && IP6_IS_MULTICAST (MulticastAddr));

  NET_LIST_FOR_EACH (Entry, &IpSb->MldCtrl.Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IP6_MLD_GROUP, Link);
    if (EFI_IP6_EQUAL (MulticastAddr, &Group->Address)) {
      return Group;
    }
  }

  return NULL;
}

/**
  Count the number of IP6 multicast groups that are mapped to the
  same MAC address. Several IP6 multicast address may be mapped to
  the same MAC address.

  @param[in]  MldCtrl              The MLD control block to search in.
  @param[in]  Mac                  The MAC address to search.

  @return The number of the IP6 multicast group that mapped to the same
          multicast group Mac.

**/
INTN
Ip6FindMac (
  IN IP6_MLD_SERVICE_DATA   *MldCtrl,
  IN EFI_MAC_ADDRESS        *Mac
  )
{
  LIST_ENTRY                *Entry;
  IP6_MLD_GROUP             *Group;
  INTN                      Count;

  Count = 0;

  NET_LIST_FOR_EACH (Entry, &MldCtrl->Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IP6_MLD_GROUP, Link);

    if (NET_MAC_EQUAL (&Group->Mac, Mac, sizeof (EFI_MAC_ADDRESS))) {
      Count++;
    }
  }

  return Count;
}

/**
  Generate MLD report message and send it out to MulticastAddr.

  @param[in]  IpSb               The IP service to send the packet.
  @param[in]  Interface          The IP interface to send the packet.
                                 If NULL, a system interface will be selected.
  @param[in]  MulticastAddr      The specific IPv6 multicast address to which
                                 the message sender is listening.

  @retval EFI_OUT_OF_RESOURCES   There are not sufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            The MLD report message was successfully sent out.

**/
EFI_STATUS
Ip6SendMldReport (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface OPTIONAL,
  IN EFI_IPv6_ADDRESS       *MulticastAddr
  )
{
  IP6_MLD_HEAD              *MldHead;
  NET_BUF                   *Packet;
  EFI_IP6_HEADER            Head;
  UINT16                    PayloadLen;
  UINTN                     OptionLen;
  UINT8                     *Options;
  EFI_STATUS                Status;
  UINT16                    HeadChecksum;
  UINT16                    PseudoChecksum;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (MulticastAddr != NULL && IP6_IS_MULTICAST (MulticastAddr));

  //
  // Generate the packet to be sent
  // IPv6 basic header + Hop by Hop option + MLD message
  //

  OptionLen = 0;
  Status = Ip6FillHopByHop (NULL, &OptionLen, IP6_ICMP);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  PayloadLen = (UINT16) (OptionLen + sizeof (IP6_MLD_HEAD));
  Packet = NetbufAlloc (sizeof (EFI_IP6_HEADER) + (UINT32) PayloadLen);
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create the basic IPv6 header.
  // RFC3590: Use link-local address as source address if it is available,
  // otherwise use the unspecified address.
  //
  Head.FlowLabelL     = 0;
  Head.FlowLabelH     = 0;
  Head.PayloadLength  = HTONS (PayloadLen);
  Head.NextHeader     = IP6_HOP_BY_HOP;
  Head.HopLimit       = 1;
  IP6_COPY_ADDRESS (&Head.DestinationAddress, MulticastAddr);

  //
  // If Link-Local address is not ready, we use unspecified address.
  //
  IP6_COPY_ADDRESS (&Head.SourceAddress, &IpSb->LinkLocalAddr);

  NetbufReserve (Packet, sizeof (EFI_IP6_HEADER));

  //
  // Fill a IPv6 Router Alert option in a Hop-by-Hop Options Header
  //
  Options = NetbufAllocSpace (Packet, (UINT32) OptionLen, FALSE);
  ASSERT (Options != NULL);
  Status = Ip6FillHopByHop (Options, &OptionLen, IP6_ICMP);
  if (EFI_ERROR (Status)) {
    NetbufFree (Packet);
    Packet = NULL;
    return Status;
  }

  //
  // Fill in MLD message - Report
  //
  MldHead = (IP6_MLD_HEAD *) NetbufAllocSpace (Packet, sizeof (IP6_MLD_HEAD), FALSE);
  ASSERT (MldHead != NULL);
  ZeroMem (MldHead, sizeof (IP6_MLD_HEAD));
  MldHead->Head.Type = ICMP_V6_LISTENER_REPORT;
  MldHead->Head.Code = 0;
  IP6_COPY_ADDRESS (&MldHead->Group, MulticastAddr);

  HeadChecksum   = NetblockChecksum ((UINT8 *) MldHead, sizeof (IP6_MLD_HEAD));
  PseudoChecksum = NetIp6PseudoHeadChecksum (
                     &Head.SourceAddress,
                     &Head.DestinationAddress,
                     IP6_ICMP,
                     sizeof (IP6_MLD_HEAD)
                     );

  MldHead->Head.Checksum = (UINT16) ~NetAddChecksum (HeadChecksum, PseudoChecksum);

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, Interface, NULL, Packet, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

/**
  Generate MLD Done message and send it out to MulticastAddr.

  @param[in]  IpSb               The IP service to send the packet.
  @param[in]  MulticastAddr      The specific IPv6 multicast address to which
                                 the message sender is ceasing to listen.

  @retval EFI_OUT_OF_RESOURCES   There are not sufficient resources to complete the
                                 operation.
  @retval EFI_SUCCESS            The MLD report message was successfully sent out.

**/
EFI_STATUS
Ip6SendMldDone (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *MulticastAddr
  )
{
  IP6_MLD_HEAD              *MldHead;
  NET_BUF                   *Packet;
  EFI_IP6_HEADER            Head;
  UINT16                    PayloadLen;
  UINTN                     OptionLen;
  UINT8                     *Options;
  EFI_STATUS                Status;
  EFI_IPv6_ADDRESS          Destination;
  UINT16                    HeadChecksum;
  UINT16                    PseudoChecksum;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
  ASSERT (MulticastAddr != NULL && IP6_IS_MULTICAST (MulticastAddr));

  //
  // Generate the packet to be sent
  // IPv6 basic header + Hop by Hop option + MLD message
  //

  OptionLen = 0;
  Status = Ip6FillHopByHop (NULL, &OptionLen, IP6_ICMP);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  PayloadLen = (UINT16) (OptionLen + sizeof (IP6_MLD_HEAD));
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
  Head.NextHeader     = IP6_HOP_BY_HOP;
  Head.HopLimit       = 1;

  //
  // If Link-Local address is not ready, we use unspecified address.
  //
  IP6_COPY_ADDRESS (&Head.SourceAddress, &IpSb->LinkLocalAddr);

  Ip6SetToAllNodeMulticast (TRUE, IP6_LINK_LOCAL_SCOPE, &Destination);
  IP6_COPY_ADDRESS (&Head.DestinationAddress, &Destination);

  NetbufReserve (Packet, sizeof (EFI_IP6_HEADER));

  //
  // Fill a IPv6 Router Alert option in a Hop-by-Hop Options Header
  //
  Options = NetbufAllocSpace (Packet, (UINT32) OptionLen, FALSE);
  ASSERT (Options != NULL);
  Status = Ip6FillHopByHop (Options, &OptionLen, IP6_ICMP);
  if (EFI_ERROR (Status)) {
    NetbufFree (Packet);
    Packet = NULL;
    return Status;
  }

  //
  // Fill in MLD message - Done
  //
  MldHead = (IP6_MLD_HEAD *) NetbufAllocSpace (Packet, sizeof (IP6_MLD_HEAD), FALSE);
  ASSERT (MldHead != NULL);
  ZeroMem (MldHead, sizeof (IP6_MLD_HEAD));
  MldHead->Head.Type = ICMP_V6_LISTENER_DONE;
  MldHead->Head.Code = 0;
  IP6_COPY_ADDRESS (&MldHead->Group, MulticastAddr);

  HeadChecksum   = NetblockChecksum ((UINT8 *) MldHead, sizeof (IP6_MLD_HEAD));
  PseudoChecksum = NetIp6PseudoHeadChecksum (
                     &Head.SourceAddress,
                     &Head.DestinationAddress,
                     IP6_ICMP,
                     sizeof (IP6_MLD_HEAD)
                     );

  MldHead->Head.Checksum = (UINT16) ~NetAddChecksum (HeadChecksum, PseudoChecksum);

  //
  // Transmit the packet
  //
  return Ip6Output (IpSb, NULL, NULL, Packet, &Head, NULL, 0, Ip6SysPacketSent, NULL);
}

/**
  Init the MLD data of the IP6 service instance. Configure
  MNP to receive ALL SYSTEM multicast.

  @param[in]  IpSb              The IP6 service whose MLD is to be initialized.

  @retval EFI_OUT_OF_RESOURCES  There are not sufficient resourcet to complete the
                                operation.
  @retval EFI_SUCCESS           The MLD module successfully initialized.

**/
EFI_STATUS
Ip6InitMld (
  IN IP6_SERVICE            *IpSb
  )
{
  EFI_IPv6_ADDRESS          AllNodes;
  IP6_MLD_GROUP             *Group;
  EFI_STATUS                Status;

  //
  // Join the link-scope all-nodes multicast address (FF02::1).
  // This address is started in Idle Listener state and never transitions to
  // another state, and never sends a Report or Done for that address.
  //

  Ip6SetToAllNodeMulticast (FALSE, IP6_LINK_LOCAL_SCOPE, &AllNodes);

  Group = Ip6CreateMldEntry (IpSb, &AllNodes, (UINT32) IP6_INFINIT_LIFETIME);
  if (Group == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Ip6GetMulticastMac (IpSb->Mnp, &AllNodes, &Group->Mac);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  //
  // Configure MNP to receive all-nodes multicast
  //
  Status = IpSb->Mnp->Groups (IpSb->Mnp, TRUE, &Group->Mac);
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ERROR;
  }

  return EFI_SUCCESS;

ERROR:
  RemoveEntryList (&Group->Link);
  FreePool (Group);
  return Status;
}

/**
  Add a group address to the array of group addresses.
  The caller should make sure that no duplicated address
  existed in the array.

  @param[in, out]  IpInstance       Points to an IP6_PROTOCOL instance.
  @param[in]       Group            The IP6 multicast address to add.

  @retval EFI_OUT_OF_RESOURCES      There are not sufficient resources to complete
                                    the operation.
  @retval EFI_SUCESS                The address is added to the group address array.

**/
EFI_STATUS
Ip6CombineGroups (
  IN OUT IP6_PROTOCOL *IpInstance,
  IN EFI_IPv6_ADDRESS *Group
  )
{
  EFI_IPv6_ADDRESS     *GroupList;

  NET_CHECK_SIGNATURE (IpInstance, IP6_PROTOCOL_SIGNATURE);
  ASSERT (Group != NULL && IP6_IS_MULTICAST (Group));

  IpInstance->GroupCount++;

  GroupList = AllocatePool (IpInstance->GroupCount * sizeof (EFI_IPv6_ADDRESS));
  if (GroupList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IpInstance->GroupCount > 1) {
    ASSERT (IpInstance->GroupList != NULL);

    CopyMem (
      GroupList,
      IpInstance->GroupList,
      (IpInstance->GroupCount - 1) * sizeof (EFI_IPv6_ADDRESS)
      );

    FreePool (IpInstance->GroupList);
  }

  IP6_COPY_ADDRESS (GroupList + (IpInstance->GroupCount - 1), Group);

  IpInstance->GroupList = GroupList;

  return EFI_SUCCESS;
}

/**
  Remove a group address from the array of group addresses.
  Although the function doesn't assume the byte order of Group,
  the network byte order is used by the caller.

  @param[in, out]  IpInstance       Points to an IP6_PROTOCOL instance.
  @param[in]       Group            The IP6 multicast address to remove.

  @retval EFI_NOT_FOUND             Cannot find the to be removed group address.
  @retval EFI_SUCCESS               The group address was successfully removed.

**/
EFI_STATUS
Ip6RemoveGroup (
  IN OUT IP6_PROTOCOL *IpInstance,
  IN EFI_IPv6_ADDRESS *Group
  )
{
  UINT32                    Index;
  UINT32                    Count;

  Count = IpInstance->GroupCount;

  for (Index = 0; Index < Count; Index++) {
    if (EFI_IP6_EQUAL (IpInstance->GroupList + Index, Group)) {
      break;
    }
  }

  if (Index == Count) {
    return EFI_NOT_FOUND;
  }

  while (Index < Count - 1) {
    IP6_COPY_ADDRESS (IpInstance->GroupList + Index, IpInstance->GroupList + Index + 1);
    Index++;
  }

  ASSERT (IpInstance->GroupCount > 0);
  IpInstance->GroupCount--;

  return EFI_SUCCESS;
}

/**
  Join the multicast group on behalf of this IP6 service binding instance.

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  Interface          Points to an IP6_INTERFACE structure.
  @param[in]  Address            The group address to join.

  @retval EFI_SUCCESS            Successfully join the multicast group.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval Others                 Failed to join the multicast group.

**/
EFI_STATUS
Ip6JoinGroup (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface,
  IN EFI_IPv6_ADDRESS       *Address
  )
{
  IP6_MLD_GROUP            *Group;
  EFI_STATUS               Status;

  Group = Ip6FindMldEntry (IpSb, Address);
  if (Group != NULL) {
    Group->RefCnt++;
    return EFI_SUCCESS;
  }

  //
  // Repeat the report once or twcie after short delays [Unsolicited Report Interval] (default:10s)
  // Simulate this operation as a Multicast-Address-Specific Query was received for that addresss.
  //
  Group = Ip6CreateMldEntry (IpSb, Address, IP6_UNSOLICITED_REPORT_INTERVAL);
  if (Group == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Group->SendByUs = TRUE;

  Status = Ip6GetMulticastMac (IpSb->Mnp, Address, &Group->Mac);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IpSb->Mnp->Groups (IpSb->Mnp, TRUE, &Group->Mac);
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    goto ERROR;
  }

  //
  // Send unsolicited report when a node starts listening to a multicast address
  //
  Status = Ip6SendMldReport (IpSb, Interface, Address);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  return EFI_SUCCESS;

ERROR:
  RemoveEntryList (&Group->Link);
  FreePool (Group);
  return Status;
}

/**
  Leave the IP6 multicast group.

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  Address            The group address to leave.

  @retval EFI_NOT_FOUND          The IP6 service instance isn't in the group.
  @retval EFI_SUCCESS            Successfully leave the multicast group..
  @retval Others                 Failed to leave the multicast group.

**/
EFI_STATUS
Ip6LeaveGroup (
 IN IP6_SERVICE            *IpSb,
 IN EFI_IPv6_ADDRESS       *Address
  )
{
  IP6_MLD_GROUP            *Group;
  EFI_STATUS               Status;

  Group = Ip6FindMldEntry (IpSb, Address);
  if (Group == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // If more than one instance is in the group, decrease
  // the RefCnt then return.
  //
  if ((Group->RefCnt > 0) && (--Group->RefCnt > 0)) {
    return EFI_SUCCESS;
  }

  //
  // If multiple IP6 group addresses are mapped to the same
  // multicast MAC address, don't configure the MNP to leave
  // the MAC.
  //
  if (Ip6FindMac (&IpSb->MldCtrl, &Group->Mac) == 1) {
    Status = IpSb->Mnp->Groups (IpSb->Mnp, FALSE, &Group->Mac);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      return Status;
    }
  }

  //
  // Send a leave report if we are the last node to report
  //
  if (Group->SendByUs) {
    Status = Ip6SendMldDone (IpSb, Address);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  RemoveEntryList (&Group->Link);
  FreePool (Group);

  return EFI_SUCCESS;
}

/**
  Worker function for EfiIp6Groups(). The caller
  should make sure that the parameters are valid.

  @param[in]  IpInstance        The IP6 child to change the setting.
  @param[in]  JoinFlag          TRUE to join the group, otherwise leave it.
  @param[in]  GroupAddress      The target group address. If NULL, leave all
                                the group addresses.

  @retval EFI_ALREADY_STARTED   Wants to join the group, but is already a member of it
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate sufficient resources.
  @retval EFI_DEVICE_ERROR      Failed to set the group configuraton.
  @retval EFI_SUCCESS           Successfully updated the group setting.
  @retval EFI_NOT_FOUND         Try to leave the group which it isn't a member.

**/
EFI_STATUS
Ip6Groups (
  IN IP6_PROTOCOL           *IpInstance,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv6_ADDRESS       *GroupAddress       OPTIONAL
  )
{
  EFI_STATUS                Status;
  IP6_SERVICE               *IpSb;
  UINT32                    Index;
  EFI_IPv6_ADDRESS          *Group;

  IpSb = IpInstance->Service;

  if (JoinFlag) {
    ASSERT (GroupAddress != NULL);

    for (Index = 0; Index < IpInstance->GroupCount; Index++) {
      if (EFI_IP6_EQUAL (IpInstance->GroupList + Index, GroupAddress)) {
        return EFI_ALREADY_STARTED;
      }
    }

    Status = Ip6JoinGroup (IpSb, IpInstance->Interface, GroupAddress);
    if (!EFI_ERROR (Status)) {
      return Ip6CombineGroups (IpInstance, GroupAddress);
    }

    return Status;
  }

  //
  // Leave the group. Leave all the groups if GroupAddress is NULL.
  //
  for (Index = IpInstance->GroupCount; Index > 0; Index--) {
    Group = IpInstance->GroupList + (Index - 1);

    if ((GroupAddress == NULL) || EFI_IP6_EQUAL (Group, GroupAddress)) {
      Status = Ip6LeaveGroup (IpInstance->Service, Group);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Ip6RemoveGroup (IpInstance, Group);

      if (IpInstance->GroupCount == 0) {
        ASSERT (Index == 1);
        FreePool (IpInstance->GroupList);
        IpInstance->GroupList = NULL;
      }

      if (GroupAddress != NULL) {
        return EFI_SUCCESS;
      }
    }
  }

  return ((GroupAddress != NULL) ? EFI_NOT_FOUND : EFI_SUCCESS);
}

/**
  Set a random value of the delay timer for the multicast address from the range
  [0, Maximum Response Delay]. If a timer for any address is already
  running, it is reset to the new random value only if the requested
  Maximum Response Delay is less than the remaining value of the
  running timer.  If the Query packet specifies a Maximum Response
  Delay of zero, each timer is effectively set to zero, and the action
  specified below for timer expiration is performed immediately.

  @param[in]      IpSb               The IP6 service binding instance.
  @param[in]      MaxRespDelay       The Maximum Response Delay, in milliseconds.
  @param[in]      MulticastAddr      The multicast address.
  @param[in, out] Group              Points to a IP6_MLD_GROUP list entry node.

  @retval EFI_SUCCESS                The delay timer is successfully updated or
                                     timer expiration is performed immediately.
  @retval Others                     Failed to send out MLD report message.

**/
EFI_STATUS
Ip6UpdateDelayTimer (
  IN IP6_SERVICE            *IpSb,
  IN UINT16                 MaxRespDelay,
  IN EFI_IPv6_ADDRESS       *MulticastAddr,
  IN OUT IP6_MLD_GROUP      *Group
  )
{
  UINT32                    Delay;

  //
  // If the Query packet specifies a Maximum Response Delay of zero, perform timer
  // expiration immediately.
  //
  if (MaxRespDelay == 0) {
    Group->DelayTimer = 0;
    return Ip6SendMldReport (IpSb, NULL, MulticastAddr);
  }

  Delay = (UINT32) (MaxRespDelay / 1000);

  //
  // Sets a delay timer to a random value selected from the range [0, Maximum Response Delay]
  // If a timer is already running, resets it if the request Maximum Response Delay
  // is less than the remaining value of the running timer.
  //
  if (Group->DelayTimer == 0 || Delay < Group->DelayTimer) {
    Group->DelayTimer = Delay / 4294967295UL * NET_RANDOM (NetRandomInitSeed ());
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_IPv6_ADDRESS          AllNodes;
  IP6_MLD_GROUP             *Group;
  IP6_MLD_HEAD              MldPacket;
  LIST_ENTRY                *Entry;
  EFI_STATUS                Status;

  Status = EFI_INVALID_PARAMETER;

  //
  // Check the validity of the packet, generic query or specific query
  //
  if (!NetIp6IsUnspecifiedAddr (&Head->SourceAddress) && !NetIp6IsLinkLocalAddr (&Head->SourceAddress)) {
    goto Exit;
  }

  if (Head->HopLimit != 1 || !IP6_IS_MULTICAST (&Head->DestinationAddress)) {
    goto Exit;
  }

  //
  // The Packet points to MLD report raw data without Hop-By-Hop option.
  //
  NetbufCopy (Packet, 0, sizeof (IP6_MLD_HEAD), (UINT8 *) &MldPacket);
  MldPacket.MaxRespDelay = NTOHS (MldPacket.MaxRespDelay);

  Ip6SetToAllNodeMulticast (FALSE, IP6_LINK_LOCAL_SCOPE, &AllNodes);
  if (!EFI_IP6_EQUAL (&Head->DestinationAddress, &AllNodes)) {
    //
    // Receives a Multicast-Address-Specific Query, check it firstly
    //
    if (!EFI_IP6_EQUAL (&Head->DestinationAddress, &MldPacket.Group)) {
      goto Exit;
    }
    //
    // The node is not listening but it receives the specific query. Just return.
    //
    Group = Ip6FindMldEntry (IpSb, &MldPacket.Group);
    if (Group == NULL) {
      Status = EFI_SUCCESS;
      goto Exit;
    }

    Status = Ip6UpdateDelayTimer (
               IpSb,
               MldPacket.MaxRespDelay,
               &MldPacket.Group,
               Group
               );
    goto Exit;
  }

  //
  // Receives a General Query, sets a delay timer for each multicast address it is listening
  //
  NET_LIST_FOR_EACH (Entry, &IpSb->MldCtrl.Groups) {
    Group  = NET_LIST_USER_STRUCT (Entry, IP6_MLD_GROUP, Link);
    Status = Ip6UpdateDelayTimer (IpSb, MldPacket.MaxRespDelay, &Group->Address, Group);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = EFI_SUCCESS;

Exit:
  NetbufFree (Packet);
  return Status;
}

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
  )
{
  IP6_MLD_HEAD              MldPacket;
  IP6_MLD_GROUP             *Group;
  EFI_STATUS                Status;

  Status = EFI_INVALID_PARAMETER;

  //
  // Validate the incoming message, if invalid, drop it.
  //
  if (!NetIp6IsUnspecifiedAddr (&Head->SourceAddress) && !NetIp6IsLinkLocalAddr (&Head->SourceAddress)) {
    goto Exit;
  }

  if (Head->HopLimit != 1 || !IP6_IS_MULTICAST (&Head->DestinationAddress)) {
    goto Exit;
  }

  //
  // The Packet points to MLD report raw data without Hop-By-Hop option.
  //
  NetbufCopy (Packet, 0, sizeof (IP6_MLD_HEAD), (UINT8 *) &MldPacket);
  if (!EFI_IP6_EQUAL (&Head->DestinationAddress, &MldPacket.Group)) {
    goto Exit;
  }

  Group = Ip6FindMldEntry (IpSb, &MldPacket.Group);
  if (Group == NULL) {
    goto Exit;
  }

  //
  // The report is sent by another node, stop its own timer relates to the multicast address and clear
  //

  if (!Group->SendByUs) {
    Group->DelayTimer = 0;
  }

  Status = EFI_SUCCESS;

Exit:
  NetbufFree (Packet);
  return Status;
}

/**
  The heartbeat timer of MLD module. It sends out a solicited MLD report when
  DelayTimer expires.

  @param[in]  IpSb              The IP6 service binding instance.

**/
VOID
Ip6MldTimerTicking (
  IN IP6_SERVICE            *IpSb
  )
{
  IP6_MLD_GROUP             *Group;
  LIST_ENTRY                *Entry;

  //
  // Send solicited report when timer expires
  //
  NET_LIST_FOR_EACH (Entry, &IpSb->MldCtrl.Groups) {
    Group = NET_LIST_USER_STRUCT (Entry, IP6_MLD_GROUP, Link);
    if ((Group->DelayTimer > 0) && (--Group->DelayTimer == 0)) {
      Ip6SendMldReport (IpSb, NULL, &Group->Address);
    }
  }
}

