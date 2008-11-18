/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4Icmp.h

Abstract:

  Header file for ICMP protocol.


**/

#ifndef __EFI_IP4_ICMP_H__
#define __EFI_IP4_ICMP_H__

typedef enum {
  //
  // ICMP type definations
  //
  ICMP_ECHO_REPLY           = 0,
  ICMP_DEST_UNREACHABLE     = 3,
  ICMP_SOURCE_QUENCH        = 4,
  ICMP_REDIRECT             = 5,
  ICMP_ECHO_REQUEST         = 8,
  ICMP_TIME_EXCEEDED        = 11,
  ICMP_PARAMETER_PROBLEM    = 12,
  ICMP_TIMESTAMP            = 13,
  ICMP_INFO_REQUEST         = 15,
  ICMP_INFO_REPLY           = 16,
  ICMP_TYPE_MAX             = ICMP_INFO_REPLY,

  ICMP_DEFAULT_CODE         = 0,

  //
  // ICMP code definations for ICMP_DEST_UNREACHABLE
  //
  ICMP_NET_UNREACHABLE      = 0,
  ICMP_HOST_UNREACHABLE     = 1,
  ICMP_PROTO_UNREACHABLE    = 2,  // Host may generate
  ICMP_PORT_UNREACHABLE     = 3,  // Host may generate
  ICMP_FRAGMENT_FAILED      = 4,
  ICMP_SOURCEROUTE_FAILED   = 5,  // Host may generate
  ICMP_NET_UNKNOWN          = 6,
  ICMP_HOST_UNKNOWN         = 7,
  ICMP_SOURCE_ISOLATED      = 8,
  ICMP_NET_PROHIBITED       = 9,
  ICMP_HOST_PROHIBITED      = 10,
  ICMP_NET_UNREACHABLE_TOS  = 11,
  ICMP_HOST_UNREACHABLE_TOS = 12,

  //
  // ICMP code definations for ICMP_TIME_EXCEEDED
  //
  ICMP_TIMEOUT_IN_TRANSIT   = 0,
  ICMp_TIMEOUT_REASSEMBLE   = 1,  // Host may generate

  //
  // ICMP code definations for ICMP_TIME_EXCEEDED
  //
  ICMP_NET_REDIRECT         = 0,
  ICMP_HOST_REDIRECT        = 1,
  ICMP_NET_TOS_REDIRECT     = 2,
  ICMP_HOST_TOS_REDIRECT    = 3,

  //
  // ICMP message classes, each class of ICMP message shares
  // a common message format. INVALID_MESSAGE is only a flag.
  //
  ICMP_INVALID_MESSAGE      = 0,
  ICMP_ERROR_MESSAGE        = 1,
  ICMP_QUERY_MESSAGE        = 2
} ICMP_ENUM_TYPES;

typedef struct {
  UINT8                   IcmpType;
  UINT8                   IcmpClass;
} IP4_ICMP_CLASS;

extern IP4_ICMP_CLASS     mIcmpClass[];
extern EFI_IP4_ICMP_TYPE  mIp4SupportedIcmp[];

/**
  Handle the ICMP packet. First validate the message format,
  then according to the message types, process it as query or
  error packet.

  @param  IpSb                   The IP service that receivd the packet
  @param  Head                   The IP head of the ICMP query packet
  @param  Packet                 The content of the ICMP query with IP head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is malformated.
  @retval EFI_SUCCESS            The ICMP message is successfully processed.
  @retval Others                 Failed to handle ICMP packet.

**/
EFI_STATUS
Ip4IcmpHandle (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  );
#endif
