/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4Igmp.h

Abstract:


**/

#ifndef __EFI_IP4_IGMP_H__
#define __EFI_IP4_IGMP_H__

#pragma pack(1)
typedef struct {
  UINT8                   Type;
  UINT8                   MaxRespTime;
  UINT16                  Checksum;
  IP4_ADDR                Group;
} IGMP_HEAD;
#pragma pack()

//
// The status of multicast group. It isn't necessary to maintain
// explicit state of host state diagram. A group with non-zero
// DelayTime is in "delaying member" state. otherwise, it is in
// "idle member" state.
//
typedef struct {
  LIST_ENTRY              Link;
  INTN                    RefCnt;
  IP4_ADDR                Address;
  INTN                    DelayTime;
  BOOLEAN                 ReportByUs;
  EFI_MAC_ADDRESS         Mac;
} IGMP_GROUP;

//
// The IGMP status. Each IP4 service instance has a IGMP_SERVICE_DATA
// attached. The Igmpv1QuerySeen remember whether the server on this
// connected network is v1 or v2.
//
typedef struct {
  INTN                    Igmpv1QuerySeen;
  LIST_ENTRY              Groups;
} IGMP_SERVICE_DATA;

enum {
  //
  // IGMP message type
  //
  IGMP_MEMBERSHIP_QUERY     = 0x11,
  IGMP_V1_MEMBERSHIP_REPORT = 0x12,
  IGMP_V2_MEMBERSHIP_REPORT = 0x16,
  IGMP_LEAVE_GROUP          = 0x17,

  IGMP_V1ROUTER_PRESENT     = 400,
  IGMP_UNSOLICIATED_REPORT  = 10
};

EFI_STATUS
Ip4InitIgmp (
  IN IP4_SERVICE          *IpService
  );

EFI_STATUS
Ip4JoinGroup (
  IN IP4_PROTOCOL         *IpInstance,
  IN IP4_ADDR             Address
  );

EFI_STATUS
Ip4LeaveGroup (
  IN IP4_PROTOCOL         *IpInstance,
  IN IP4_ADDR             Address
  );

EFI_STATUS
Ip4IgmpHandle (
  IN IP4_SERVICE          *IpService,
  IN IP4_HEAD             *Head,
  IN NET_BUF              *Packet
  );

VOID
Ip4IgmpTicking (
  IN IP4_SERVICE          *IpService
  );

IP4_ADDR *
Ip4CombineGroups (
  IN  IP4_ADDR            *SourceGroups,
  IN  UINT32              Count,
  IN  IP4_ADDR            Addr
  );

INTN
Ip4RemoveGroupAddr (
  IN IP4_ADDR             *Group,
  IN UINT32               GroupCnt,
  IN IP4_ADDR             Addr
  );

IGMP_GROUP *
Ip4FindGroup (
  IN IGMP_SERVICE_DATA    *IgmpCtrl,
  IN IP4_ADDR             Address
  );
#endif
