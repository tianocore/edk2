/** @file
  Functions declaration related with DHCPv6 for HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __EFI_HTTP_BOOT_DHCP6_H__
#define __EFI_HTTP_BOOT_DHCP6_H__

#define HTTP_BOOT_OFFER_MAX_NUM                16
#define HTTP_BOOT_DHCP6_OPTION_MAX_NUM         16
#define HTTP_BOOT_DHCP6_OPTION_MAX_SIZE        312
#define HTTP_BOOT_DHCP6_PACKET_MAX_SIZE        1472
#define HTTP_BOOT_IP6_ROUTE_TABLE_TIMEOUT      10
#define HTTP_BOOT_DEFAULT_HOPLIMIT             64
#define HTTP_BOOT_DEFAULT_LIFETIME             50000

#define HTTP_BOOT_DHCP6_ENTERPRISE_NUM        343   // TODO: IANA TBD: temporarily using Intel's
#define HTTP_BOOT_DHCP6_MAX_BOOT_FILE_SIZE    65535 //   It's a limitation of bit length, 65535*512 bytes.

#define HTTP_BOOT_DHCP6_IDX_IA_NA             0
#define HTTP_BOOT_DHCP6_IDX_BOOT_FILE_URL     1
#define HTTP_BOOT_DHCP6_IDX_BOOT_FILE_PARAM   2
#define HTTP_BOOT_DHCP6_IDX_VENDOR_CLASS      3
#define HTTP_BOOT_DHCP6_IDX_DNS_SERVER        4
#define HTTP_BOOT_DHCP6_IDX_MAX               5

#pragma pack(1)
typedef struct {
  UINT16 OpCode[256];
} HTTP_BOOT_DHCP6_OPTION_ORO;

typedef struct {
  UINT8 Type;
  UINT8 MajorVer;
  UINT8 MinorVer;
} HTTP_BOOT_DHCP6_OPTION_UNDI;

typedef struct {
  UINT16 Type;
} HTTP_BOOT_DHCP6_OPTION_ARCH;

typedef struct {
  UINT8 ClassIdentifier[11];
  UINT8 ArchitecturePrefix[5];
  UINT8 ArchitectureType[5];
  UINT8 Lit3[1];
  UINT8 InterfaceName[4];
  UINT8 Lit4[1];
  UINT8 UndiMajor[3];
  UINT8 UndiMinor[3];
} HTTP_BOOT_CLASS_ID;

typedef struct {
  UINT32             Vendor;
  UINT16             ClassLen;
  HTTP_BOOT_CLASS_ID ClassId;
} HTTP_BOOT_DHCP6_OPTION_VENDOR_CLASS;

#pragma pack()

typedef union {
  HTTP_BOOT_DHCP6_OPTION_ORO            *Oro;
  HTTP_BOOT_DHCP6_OPTION_UNDI           *Undi;
  HTTP_BOOT_DHCP6_OPTION_ARCH           *Arch;
  HTTP_BOOT_DHCP6_OPTION_VENDOR_CLASS   *VendorClass;
} HTTP_BOOT_DHCP6_OPTION_ENTRY;

#define HTTP_CACHED_DHCP6_PACKET_MAX_SIZE  (OFFSET_OF (EFI_DHCP6_PACKET, Dhcp6) + HTTP_BOOT_DHCP6_PACKET_MAX_SIZE)

typedef union {
  EFI_DHCP6_PACKET        Offer;
  EFI_DHCP6_PACKET        Ack;
  UINT8                   Buffer[HTTP_CACHED_DHCP6_PACKET_MAX_SIZE];
} HTTP_BOOT_DHCP6_PACKET;

typedef struct {
  HTTP_BOOT_DHCP6_PACKET      Packet;
  HTTP_BOOT_OFFER_TYPE        OfferType;
  EFI_DHCP6_PACKET_OPTION     *OptList[HTTP_BOOT_DHCP6_IDX_MAX];
  VOID                        *UriParser;
} HTTP_BOOT_DHCP6_PACKET_CACHE;

#define GET_NEXT_DHCP6_OPTION(Opt) \
  (EFI_DHCP6_PACKET_OPTION *) ((UINT8 *) (Opt) + \
  sizeof (EFI_DHCP6_PACKET_OPTION) + (NTOHS ((Opt)->OpLen)) - 1)

#define GET_DHCP6_OPTION_SIZE(Pkt)  \
  ((Pkt)->Length - sizeof (EFI_DHCP6_HEADER))

/**
  Start the S.A.R.R DHCPv6 process to acquire the IPv6 address and other Http boot information.

  @param[in]  Private           Pointer to HTTP_BOOT private data.

  @retval EFI_SUCCESS           The S.A.R.R process successfully finished.
  @retval Others                Failed to finish the S.A.R.R process.

**/
EFI_STATUS
HttpBootDhcp6Sarr (
  IN HTTP_BOOT_PRIVATE_DATA         *Private
  );

/**
  Set the IP6 policy to Automatic.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.

  @retval     EFI_SUCCESS         Switch the IP policy successfully.
  @retval     Others              Unexpected error happened.

**/
EFI_STATUS
HttpBootSetIp6Policy (
  IN HTTP_BOOT_PRIVATE_DATA        *Private
  );

/**
  This function will register the default DNS addresses to the network device.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.
  @param[in]  DataLength          Size of the buffer pointed to by DnsServerData in bytes.
  @param[in]  DnsServerData       Point a list of DNS server address in an array
                                  of EFI_IPv6_ADDRESS instances.

  @retval     EFI_SUCCESS         The DNS configuration has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootSetIp6Dns (
  IN HTTP_BOOT_PRIVATE_DATA         *Private,
  IN UINTN                          DataLength,
  IN VOID                           *DnsServerData
  );

/**
  This function will register the IPv6 gateway address to the network device.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The new IP configuration has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootSetIp6Gateway (
  IN HTTP_BOOT_PRIVATE_DATA         *Private
  );

/**
  This function will register the station IP address.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The new IP address has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootSetIp6Address (
  IN HTTP_BOOT_PRIVATE_DATA         *Private
  );

#endif
