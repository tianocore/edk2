/** @file
  Header file for ICMP protocol.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP4_ICMP_H__
#define __EFI_IP4_ICMP_H__

//
// ICMP type definitions
//
#define ICMP_ECHO_REPLY         0
#define ICMP_DEST_UNREACHABLE   3
#define ICMP_SOURCE_QUENCH      4
#define ICMP_REDIRECT           5
#define ICMP_ECHO_REQUEST       8
#define ICMP_TIME_EXCEEDED      11
#define ICMP_PARAMETER_PROBLEM  12
#define ICMP_TIMESTAMP          13
#define ICMP_INFO_REQUEST       15
#define ICMP_INFO_REPLY         16
#define ICMP_TYPE_MAX           ICMP_INFO_REPLY

#define ICMP_DEFAULT_CODE  0

//
// ICMP code definitions for ICMP_DEST_UNREACHABLE
//
#define ICMP_NET_UNREACHABLE       0
#define ICMP_HOST_UNREACHABLE      1
#define ICMP_PROTO_UNREACHABLE     2  // Host may generate
#define ICMP_PORT_UNREACHABLE      3  // Host may generate
#define ICMP_FRAGMENT_FAILED       4
#define ICMP_SOURCEROUTE_FAILED    5  // Host may generate
#define ICMP_NET_UNKNOWN           6
#define ICMP_HOST_UNKNOWN          7
#define ICMP_SOURCE_ISOLATED       8
#define ICMP_NET_PROHIBITED        9
#define ICMP_HOST_PROHIBITED       10
#define ICMP_NET_UNREACHABLE_TOS   11
#define ICMP_HOST_UNREACHABLE_TOS  12

//
// ICMP code definitions for ICMP_TIME_EXCEEDED
//
#define ICMP_TIMEOUT_IN_TRANSIT  0
#define ICMP_TIMEOUT_REASSEMBLE  1    // Host may generate

//
// ICMP code definitions for ICMP_TIME_EXCEEDED
//
#define ICMP_NET_REDIRECT       0
#define ICMP_HOST_REDIRECT      1
#define ICMP_NET_TOS_REDIRECT   2
#define ICMP_HOST_TOS_REDIRECT  3

//
// ICMP message classes, each class of ICMP message shares
// a common message format. INVALID_MESSAGE is only a flag.
//
#define ICMP_INVALID_MESSAGE  0
#define ICMP_ERROR_MESSAGE    1
#define ICMP_QUERY_MESSAGE    2

typedef struct {
  UINT8    IcmpType;
  UINT8    IcmpClass;
} IP4_ICMP_CLASS;

extern IP4_ICMP_CLASS     mIcmpClass[];
extern EFI_IP4_ICMP_TYPE  mIp4SupportedIcmp[];

/**
  Handle the ICMP packet. First validate the message format,
  then according to the message types, process it as query or
  error packet.

  @param[in]  IpSb               The IP4 service that receivd the packet.
  @param[in]  Head               The IP4 head of the ICMP query packet.
  @param[in]  Packet             The content of the ICMP query with IP4 head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is malformatted.
  @retval EFI_SUCCESS            The ICMP message is successfully processed.
  @retval Others                 Failed to handle ICMP packet.

**/
EFI_STATUS
Ip4IcmpHandle (
  IN IP4_SERVICE  *IpSb,
  IN IP4_HEAD     *Head,
  IN NET_BUF      *Packet
  );

#endif
