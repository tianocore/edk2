/** @file
Functions implementation related with DHCPv4/v6 for DNS driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DNS_DHCP_H_
#define _DNS_DHCP_H_

//
// DHCP DNS related
//
#pragma pack(1)

#define IP4_ETHER_PROTO  0x0800

#define DHCP4_OPCODE_REQUEST  1
#define DHCP4_MAGIC           0x63538263  /// network byte order
#define DHCP4_TAG_EOP         255         /// End Option

#define DHCP4_TAG_TYPE     53
#define DHCP4_MSG_REQUEST  3
#define DHCP4_MSG_INFORM   8

#define DHCP4_TAG_PARA_LIST   55
#define DHCP4_TAG_DNS_SERVER  6

#define DHCP6_TAG_DNS_REQUEST  6
#define DHCP6_TAG_DNS_SERVER   23

#define DNS_CHECK_MEDIA_GET_DHCP_WAITING_TIME  EFI_TIMER_PERIOD_SECONDS(20)

//
// The required Dns4 server information.
//
typedef struct {
  UINT32              *ServerCount;
  EFI_IPv4_ADDRESS    *ServerList;
} DNS4_SERVER_INFOR;

//
// The required Dns6 server information.
//
typedef struct {
  UINT32              *ServerCount;
  EFI_IPv6_ADDRESS    *ServerList;
} DNS6_SERVER_INFOR;

#pragma pack()

/**
  Parse the ACK to get required information

  @param  Dhcp4            The DHCP4 protocol.
  @param  Packet           Packet waiting for parse.
  @param  DnsServerInfor   The required Dns4 server information.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_NO_MAPPING        DHCP failed to acquire address and other information.
  @retval EFI_DEVICE_ERROR      Other errors as indicated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.

**/
EFI_STATUS
ParseDhcp4Ack (
  IN EFI_DHCP4_PROTOCOL  *Dhcp4,
  IN EFI_DHCP4_PACKET    *Packet,
  IN DNS4_SERVER_INFOR   *DnsServerInfor
  );

/**
  EFI_DHCP6_INFO_CALLBACK is provided by the consumer of the EFI DHCPv6 Protocol
  instance to intercept events that occurs in the DHCPv6 Information Request
  exchange process.

  @param  This                  Pointer to the EFI_DHCP6_PROTOCOL instance that
                                is used to configure this  callback function.
  @param  Context               Pointer to the context that is initialized in
                                the EFI_DHCP6_PROTOCOL.InfoRequest().
  @param  Packet                Pointer to Reply packet that has been received.
                                The EFI DHCPv6 Protocol instance is responsible
                                for freeing the buffer.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_DEVICE_ERROR      Other errors as indicated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
ParseDhcp6Ack (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN VOID                *Context,
  IN EFI_DHCP6_PACKET    *Packet
  );

/**
  Parse the DHCP ACK to get Dns4 server information.

  @param  Instance         The DNS instance.
  @param  DnsServerCount   Retrieved Dns4 server Ip count.
  @param  DnsServerList    Retrieved Dns4 server Ip list.

  @retval EFI_SUCCESS           The Dns4 information is got from the DHCP ACK.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
GetDns4ServerFromDhcp4 (
  IN  DNS_INSTANCE      *Instance,
  OUT UINT32            *DnsServerCount,
  OUT EFI_IPv4_ADDRESS  **DnsServerList
  );

/**
  Parse the DHCP ACK to get Dns6 server information.

  @param  Image            The handle of the driver image.
  @param  Controller       The handle of the controller.
  @param  DnsServerCount   Retrieved Dns6 server Ip count.
  @param  DnsServerList    Retrieved Dns6 server Ip list.

  @retval EFI_SUCCESS           The Dns6 information is got from the DHCP ACK.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
GetDns6ServerFromDhcp6 (
  IN  EFI_HANDLE        Image,
  IN  EFI_HANDLE        Controller,
  OUT UINT32            *DnsServerCount,
  OUT EFI_IPv6_ADDRESS  **DnsServerList
  );

#endif
