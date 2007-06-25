/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

--*/




#define RAND_MAX  0x10000
#include "Bc.h"

//
// Definitions for internet group management protocol version 2 message
// structure Per RFC 2236, November 1997
//
STATIC UINT8      RouterAlertOption[4]  = { 0x80 | 20, 4, 0, 0 };
STATIC IPV4_ADDR  AllRoutersGroup       = { { 224, 0, 0, 2 } };

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
ClearGroupTimer (
  PXE_BASECODE_DEVICE *Private,
  UINTN               TimerId
  )
{
  if (Private == NULL) {
    return ;
  }

  if (TimerId >= Private->MCastGroupCount) {
    return ;
  }

  if (Private->IgmpGroupEvent[TimerId] == NULL) {
    return ;
  }

  gBS->CloseEvent (Private->IgmpGroupEvent[TimerId]);
  Private->IgmpGroupEvent[TimerId] = NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
SetGroupTimer (
  PXE_BASECODE_DEVICE *Private,
  UINTN               TimerId,
  UINTN               MaxRespTime
  )
/*++
Routine description:
  Set IGMP response timeout value.

Parameters:
  Private := Pointer to PxeBc interface
  TimerId := Timer ID#
  MaxRespTime := Base response timeout value in tenths of seconds

Returns:
--*/
{
  EFI_STATUS  EfiStatus;

  if (Private == NULL) {
    return ;
  }

  if (TimerId >= Private->MCastGroupCount) {
    return ;
  }

  if (Private->IgmpGroupEvent[TimerId] != NULL) {
    gBS->CloseEvent (Private->IgmpGroupEvent[TimerId]);
  }

  EfiStatus = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &Private->IgmpGroupEvent[TimerId]
                    );

  if (EFI_ERROR (EfiStatus)) {
    Private->IgmpGroupEvent[TimerId] = NULL;
    return ;
  }

  EfiStatus = gBS->SetTimer (
                    Private->IgmpGroupEvent[TimerId],
                    TimerRelative,
                    MaxRespTime * 1000000 + Random (Private) % RAND_MAX
                    );

  if (EFI_ERROR (EfiStatus)) {
    gBS->CloseEvent (Private->IgmpGroupEvent[TimerId]);
    Private->IgmpGroupEvent[TimerId] = NULL;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
SendIgmpMessage (
  PXE_BASECODE_DEVICE *Private,
  UINT8               Type,
  INTN                GroupId
  )
/*++
Routine description:
  Send an IGMP message

Parameters:
  Private := Pointer to PxeBc interface
  Type := Message type opcode
  GroupId := Group ID#

Returns:
--*/
{
  Private->IgmpMessage.Type         = Type;
  Private->IgmpMessage.MaxRespTime  = 0;
  Private->IgmpMessage.Checksum     = 0;
  Private->IgmpMessage.GroupAddress = Private->MCastGroup[GroupId];
  Private->IgmpMessage.Checksum = IpChecksum (
                                    (UINT16 *) &Private->IgmpMessage,
                                    sizeof Private->IgmpMessage
                                    );

  Ipv4SendWOp (
    Private,
    0,
    (UINT8 *) &Private->IgmpMessage,
    sizeof Private->IgmpMessage,
    PROT_IGMP,
    RouterAlertOption,
    sizeof RouterAlertOption,
    ((Type == IGMP_TYPE_LEAVE_GROUP) ? AllRoutersGroup.L : Private->IgmpMessage.GroupAddress),
    EFI_PXE_BASE_CODE_FUNCTION_IGMP
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
ReportIgmp (
  PXE_BASECODE_DEVICE *Private,
  INTN                GroupId
  )
/*++
Routine description:
  Send an IGMP report message.

Parameters:
  Private := Pointer to PxeBc interface
  GroupId := Group ID#

Returns:
--*/
{
  //
  // if version 1 querier, send v1 report
  //
  UINT8 Type;

  if (Private->Igmpv1TimeoutEvent != NULL) {
    if (!EFI_ERROR (gBS->CheckEvent (Private->Igmpv1TimeoutEvent))) {
      gBS->CloseEvent (Private->Igmpv1TimeoutEvent);
      Private->Igmpv1TimeoutEvent = NULL;
      Private->UseIgmpv1Reporting = TRUE;
    }
  }

  Type = (UINT8) (Private->UseIgmpv1Reporting ? IGMP_TYPE_V1REPORT : IGMP_TYPE_REPORT);

  SendIgmpMessage (Private, Type, GroupId);
  ClearGroupTimer (Private, GroupId);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
IgmpCheckTimers (
  PXE_BASECODE_DEVICE *Private
  )
/*++
Routine description:
  Check IGMP timers and send reports for all groups that have expired.
Parameters:
  Private := Pointer to PxeBc interface

Returns:
--*/
{
  UINTN GroupId;

  if (Private == NULL) {
    return ;
  }

  for (GroupId = 0; GroupId < Private->MCastGroupCount; ++GroupId) {
    if (Private->IgmpGroupEvent[GroupId] == NULL) {
      continue;
    }

    if (!EFI_ERROR (gBS->CheckEvent (Private->IgmpGroupEvent[GroupId]))) {
      //
      // send a report
      //
      ReportIgmp (Private, GroupId);
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
INTN
FindMulticastGroup (
  PXE_BASECODE_DEVICE *Private,
  UINT32              GroupAddress
  )
/*++
Routine description:
  Fund group ID# (index).

Parameters:
  Private := Pointer to PxeBc interface
  GroupAddress := Group multicast address

Returns:
  0 := Group not found
  other := Group ID#
--*/
{
  UINTN GroupId;

  for (GroupId = 0; GroupId < Private->MCastGroupCount; ++GroupId) {
    if (Private->MCastGroup[GroupId] == GroupAddress) {
      return GroupId + 1;
    }
  }

  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
IgmpJoinGroup (
  PXE_BASECODE_DEVICE  *Private,
  EFI_IP_ADDRESS       *GroupPtr
  )
/*++
Routine description:
  Join multicast group.

Parameters:
  Private := Pointer to PxeBc interface
  GroupPtr := Pointer to group mutlicast IP address.

Returns:
--*/
{
  UINT32  Grp;

  Grp = *(UINT32 *) GroupPtr;

  //
  // see if we already have it or if we can't take anymore
  //
  if (FindMulticastGroup (Private, Grp) || Private->MCastGroupCount == MAX_MCAST_GROUPS) {
    return ;
  }
  //
  // add the group
  //
  Private->MCastGroup[Private->MCastGroupCount] = Grp;

  ReportIgmp (Private, Private->MCastGroupCount);
  //
  // send a report
  // so it will get sent again per RFC 2236
  //
  SetGroupTimer (
    Private,
    Private->MCastGroupCount++,
    UNSOLICITED_REPORT_INTERVAL * 10
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
IgmpLeaveGroup (
  PXE_BASECODE_DEVICE       *Private,
  EFI_IP_ADDRESS            *GroupPtr
  )
/*++
Routine description:
  Leave multicast group.

Parameters:
  Private := Pointer to PxeBc interface
  GroupPtr := Mutlicast group IP address.

Returns:
--*/
{
  UINT32  Grp;
  UINTN   GroupId;

  Grp = *(UINT32 *) GroupPtr;

  //
  // if not in group, ignore
  //
  GroupId = FindMulticastGroup (Private, Grp);

  if (GroupId == 0) {
    return ;
  }
  //
  // if not v1 querrier, send leave group IGMP message
  //
  if (Private->Igmpv1TimeoutEvent != NULL) {
    if (!EFI_ERROR (gBS->CheckEvent (Private->Igmpv1TimeoutEvent))) {
      gBS->CloseEvent (Private->Igmpv1TimeoutEvent);
      Private->Igmpv1TimeoutEvent = NULL;
      Private->UseIgmpv1Reporting = TRUE;
    } else {
      SendIgmpMessage (Private, IGMP_TYPE_LEAVE_GROUP, GroupId - 1);
    }
  }

  while (GroupId < Private->MCastGroupCount) {
    Private->MCastGroup[GroupId - 1]      = Private->MCastGroup[GroupId];
    Private->IgmpGroupEvent[GroupId - 1]  = Private->IgmpGroupEvent[GroupId];
    ++GroupId;
  }

  --Private->MCastGroupCount;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
HandleIgmp (
  PXE_BASECODE_DEVICE *Private,
  IGMPV2_MESSAGE      *IgmpMessagePtr,
  UINTN               IgmpLength
  )
/*++
Routine description:
  Handle received IGMP packet

Parameters:
  Private         := Pointer to PxeBc interface
  IgmpMessagePtr  := Pointer to IGMP packet
  IgmpLength      := packet length in bytes

Returns:
--*/
{
  EFI_STATUS  EfiStatus;
  UINTN       GroupId;
  INTN        MaxRespTime;

  if (Private == NULL) {
    return ;
  }

  if (Private->MCastGroupCount == 0) {
    //
    // if we don't belong to any multicast groups, ignore
    //
    return ;
  }
  //
  // verify checksum
  //
  if (IpChecksum ((UINT16 *) IgmpMessagePtr, IgmpLength)) {
    //
    // bad checksum - ignore packet
    //
    return ;
  }

  switch (IgmpMessagePtr->Type) {
  case IGMP_TYPE_QUERY:
    //
    // if a version 1 querier, note the fact and set max resp time
    //
    MaxRespTime = IgmpMessagePtr->MaxRespTime;

    if (MaxRespTime == 0) {
      Private->UseIgmpv1Reporting = TRUE;

      if (Private->Igmpv1TimeoutEvent != NULL) {
        gBS->CloseEvent (Private->Igmpv1TimeoutEvent);
      }

      EfiStatus = gBS->CreateEvent (
                        EVT_TIMER,
                        TPL_CALLBACK,
                        NULL,
                        NULL,
                        &Private->Igmpv1TimeoutEvent
                        );

      if (EFI_ERROR (EfiStatus)) {
        Private->Igmpv1TimeoutEvent = NULL;
      } else {
        EfiStatus = gBS->SetTimer (
                          Private->Igmpv1TimeoutEvent,
                          TimerRelative,
                          (UINT64) V1ROUTER_PRESENT_TIMEOUT * 10000000
                          );
      }

      MaxRespTime = IGMP_DEFAULT_MAX_RESPONSE_TIME * 10;
    }
    //
    // if a general query (!GroupAddress), set all our group timers
    //
    if (!IgmpMessagePtr->GroupAddress) {
      for (GroupId = 0; GroupId < Private->MCastGroupCount; ++GroupId) {
        SetGroupTimer (Private, GroupId, MaxRespTime);
      }
    } else {
      //
      // specific query - set only specific group
      //
      GroupId = FindMulticastGroup (Private, IgmpMessagePtr->GroupAddress);

      if (GroupId != 0) {
        SetGroupTimer (Private, GroupId - 1, MaxRespTime);
      }
    }

    break;

  //
  // if we have a timer running for this group, clear it
  //
  case IGMP_TYPE_V1REPORT:
  case IGMP_TYPE_REPORT:
    GroupId = FindMulticastGroup (Private, IgmpMessagePtr->GroupAddress);

    if (GroupId != 0) {
      ClearGroupTimer (Private, GroupId - 1);
    }

    break;
  }
}

/* EOF - pxe_bc_igmp.c */
