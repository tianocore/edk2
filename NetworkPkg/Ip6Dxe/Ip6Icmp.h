/** @file
  Header file for ICMPv6 protocol.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP6_ICMP_H__
#define __EFI_IP6_ICMP_H__

#define ICMP_V6_DEFAULT_CODE          0

#define ICMP_V6_ERROR_MAX             127

//
// ICMPv6 message classes, each class of ICMPv6 message shares
// a common message format. INVALID_MESSAGE is only a flag.
//
#define ICMP_V6_INVALID_MESSAGE       0
#define ICMP_V6_ERROR_MESSAGE         1
#define ICMP_V6_INFORMATION_MESSAGE   2


extern EFI_IP6_ICMP_TYPE  mIp6SupportedIcmp[];

/**
  Handle the ICMPv6 packet. First validate the message format,
  then, according to the message types, process it as an informational packet or
  an error packet.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Head               The IP head of the ICMPv6 packet.
  @param[in]  Packet             The content of the ICMPv6 packet with IP head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is malformatted.
  @retval EFI_SUCCESS            The ICMPv6 message successfully processed.
  @retval Others                 Failed to handle the ICMPv6 packet.

**/
EFI_STATUS
Ip6IcmpHandle (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_HEADER         *Head,
  IN NET_BUF                *Packet
  );

/**
  Check whether the DestinationAddress is an anycast address.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  DestinationAddress Points to the Destination Address of the packet.

  @retval TRUE                   The DestinationAddress is anycast address.
  @retval FALSE                  The DestinationAddress is not anycast address.

**/
BOOLEAN
Ip6IsAnycast (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *DestinationAddress
  );

/**
  Generate ICMPv6 error message and send it out to DestinationAddress. Currently
  Destination Unreachable message, Time Exceeded message and Parameter Problem
  message are supported.

  @param[in]  IpSb               The IP service that received the packet.
  @param[in]  Packet             The packet which invoking ICMPv6 error.
  @param[in]  SourceAddress      If not NULL, points to the SourceAddress.
                                 Otherwise, the IP layer will select a source address
                                 according to the DestinationAddress.
  @param[in]  DestinationAddress Points to the Destination Address of the ICMPv6
                                 error message.
  @param[in]  Type               The type of the ICMPv6 message.
  @param[in]  Code               The additional level of the ICMPv6 message.
  @param[in]  Pointer            If not NULL, identifies the octet offset within
                                 the invoking packet where the error was detected.

  @retval EFI_INVALID_PARAMETER  The packet is malformatted.
  @retval EFI_OUT_OF_RESOURCES   There is no sufficient resource to complete the
                                 operation.
  @retval EFI_SUCCESS            The ICMPv6 message was successfully sent out.
  @retval Others                 Failed to generate the ICMPv6 packet.

**/
EFI_STATUS
Ip6SendIcmpError (
  IN IP6_SERVICE            *IpSb,
  IN NET_BUF                *Packet,
  IN EFI_IPv6_ADDRESS       *SourceAddress       OPTIONAL,
  IN EFI_IPv6_ADDRESS       *DestinationAddress,
  IN UINT8                  Type,
  IN UINT8                  Code,
  IN UINT32                 *Pointer             OPTIONAL
  );

#endif

