/** @file
  Functions declaration related with DHCPv4 for HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_UEFI_HTTP_BOOT_DHCP4_H__
#define __EFI_UEFI_HTTP_BOOT_DHCP4_H__

#define HTTP_BOOT_DHCP4_OPTION_MAX_NUM         16
#define HTTP_BOOT_DHCP4_OPTION_MAX_SIZE        312
#define HTTP_BOOT_DHCP4_PACKET_MAX_SIZE        1472

#define HTTP_BOOT_DHCP4_OPCODE_REQUEST         1
#define HTTP_BOOT_DHCP4_OPCODE_REPLY           2
#define HTTP_BOOT_DHCP4_MSG_TYPE_REQUEST       3
#define HTTP_BOOT_DHCP4_MAGIC                  0x63538263 // network byte order

#define HTTP_BOOT_DHCP4_OVERLOAD_FILE          1
#define HTTP_BOOT_DHCP4_OVERLOAD_SERVER_NAME   2

///
/// HTTP Tag definition that identifies the processor
/// and programming environment of the client system.
/// These identifiers are defined by IETF:
/// http://www.ietf.org/assignments/dhcpv6-parameters/dhcpv6-parameters.xml
///
#if defined (MDE_CPU_IA32)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    HTTP_CLIENT_ARCH_IA32
#elif defined (MDE_CPU_X64)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    HTTP_CLIENT_ARCH_X64
#elif defined (MDE_CPU_ARM)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    HTTP_CLIENT_ARCH_ARM
#elif defined (MDE_CPU_AARCH64)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    HTTP_CLIENT_ARCH_AARCH64
#elif defined (MDE_CPU_EBC)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    HTTP_CLIENT_ARCH_EBC
#endif

/// DHCP offer types among HTTP boot.
/// Dhcp4 and Dhcp6 share this definition, and corresponding
/// relatioinship is as follows:
///   Dhcp4Discover <> Dhcp6Solicit
///   Dhcp4Offer    <> Dhcp6Advertise
///   Dhcp4Request  <> Dhcp6Request
///   Dhcp4Ack      <> DHcp6Reply
///
typedef enum {
  //
  // <IP address, IP expressed URI>
  //
  HttpOfferTypeDhcpIpUri,
  //
  // <IP address, IP expressed URI, Name-server>
  //
  HttpOfferTypeDhcpIpUriDns,
  //
  // <IP address, Domain-name expressed URI, Name-server>
  //
  HttpOfferTypeDhcpNameUriDns,
  //
  // <IP address, Name-server>
  //
  HttpOfferTypeDhcpDns,
  //
  // <IP address>
  //
  HttpOfferTypeDhcpOnly,
  //
  // <Domain-name expressed URI> or
  // <Domain-name expressed URI, Name-server (will be ignored)>
  //
  HttpOfferTypeProxyNameUri,
  //
  // <IP expressed URI> or
  // <IP expressed URI, Name-server (will be ignored)>
  //
  HttpOfferTypeProxyIpUri,
  //
  // <IP address, Domain-name expressed URI>
  //
  HttpOfferTypeDhcpNameUri,
  HttpOfferTypeMax
} HTTP_BOOT_OFFER_TYPE;

#define HTTP_BOOT_DHCP_RETRIES            4
#define HTTP_BOOT_OFFER_MAX_NUM           16

// The array index of the DHCP4 option tag interested
//
#define HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE_LEN 0
#define HTTP_BOOT_DHCP4_TAG_INDEX_OVERLOAD     1
#define HTTP_BOOT_DHCP4_TAG_INDEX_MSG_TYPE     2
#define HTTP_BOOT_DHCP4_TAG_INDEX_SERVER_ID    3
#define HTTP_BOOT_DHCP4_TAG_INDEX_CLASS_ID     4
#define HTTP_BOOT_DHCP4_TAG_INDEX_BOOTFILE     5
#define HTTP_BOOT_DHCP4_TAG_INDEX_DNS_SERVER   6
#define HTTP_BOOT_DHCP4_TAG_INDEX_MAX          7

#pragma pack(1)

typedef struct {
  UINT8 ParaList[135];
} HTTP_BOOT_DHCP4_OPTION_PARA;

typedef struct {
  UINT16  Size;
} HTTP_BOOT_DHCP4_OPTION_MAX_MESG_SIZE;

typedef struct {
  UINT8 Type;
  UINT8 MajorVer;
  UINT8 MinorVer;
} HTTP_BOOT_DHCP4_OPTION_UNDI;

typedef struct {
  UINT8 Type;
} HTTP_BOOT_DHCP4_OPTION_MESG;

typedef struct {
  UINT16 Type;
} HTTP_BOOT_DHCP4_OPTION_ARCH;

typedef struct {
  UINT8 ClassIdentifier[11];
  UINT8 ArchitecturePrefix[5];
  UINT8 ArchitectureType[5];
  UINT8 Lit3[1];
  UINT8 InterfaceName[4];
  UINT8 Lit4[1];
  UINT8 UndiMajor[3];
  UINT8 UndiMinor[3];
} HTTP_BOOT_DHCP4_OPTION_CLID;

typedef struct {
  UINT8 Type;
  UINT8 Guid[16];
} HTTP_BOOT_DHCP4_OPTION_UUID;

typedef struct {
  UINT16 Type;
  UINT16 Layer;
} HTTP_BOOT_OPTION_BOOT_ITEM;

#pragma pack()

typedef union {
  HTTP_BOOT_DHCP4_OPTION_PARA           *Para;
  HTTP_BOOT_DHCP4_OPTION_UNDI           *Undi;
  HTTP_BOOT_DHCP4_OPTION_ARCH           *Arch;
  HTTP_BOOT_DHCP4_OPTION_CLID           *Clid;
  HTTP_BOOT_DHCP4_OPTION_UUID           *Uuid;
  HTTP_BOOT_DHCP4_OPTION_MESG           *Mesg;
  HTTP_BOOT_DHCP4_OPTION_MAX_MESG_SIZE  *MaxMesgSize;
} HTTP_BOOT_DHCP4_OPTION_ENTRY;

#define GET_NEXT_DHCP_OPTION(Opt) \
  (EFI_DHCP4_PACKET_OPTION *) ((UINT8 *) (Opt) + \
   sizeof (EFI_DHCP4_PACKET_OPTION) + (Opt)->Length - 1)

#define GET_OPTION_BUFFER_LEN(Pkt) \
  ((Pkt)->Length - sizeof (EFI_DHCP4_HEADER) - 4)

#define DEFAULT_CLASS_ID_DATA "HTTPClient:Arch:xxxxx:UNDI:003000"
#define DEFAULT_UNDI_TYPE     1
#define DEFAULT_UNDI_MAJOR    3
#define DEFAULT_UNDI_MINOR    0

typedef struct {
  UINT32         Reserved;
} HTTP_BOOT_VENDOR_OPTION;

#define HTTP_CACHED_DHCP4_PACKET_MAX_SIZE  (OFFSET_OF (EFI_DHCP4_PACKET, Dhcp4) + HTTP_BOOT_DHCP4_PACKET_MAX_SIZE)

typedef union {
  EFI_DHCP4_PACKET        Offer;
  EFI_DHCP4_PACKET        Ack;
  UINT8                   Buffer[HTTP_CACHED_DHCP4_PACKET_MAX_SIZE];
} HTTP_BOOT_DHCP4_PACKET;

typedef struct {
  //
  // URI component
  //
  CHAR8                   *Scheme;
  CHAR8                   *Authority;
  CHAR8                   *Path;
  CHAR8                   *Query;
  CHAR8                   *Fragment; /// TODO: may not required in HTTP URL

  CHAR8                   *RegName; /// Point to somewhere in Authority
  BOOLEAN                 AddrIsOk;
  EFI_IP_ADDRESS          Address;
  UINT16                  Port;
} HTTP_BOOT_URI_CONTENT;

typedef struct {
  HTTP_BOOT_DHCP4_PACKET      Packet;
  HTTP_BOOT_OFFER_TYPE        OfferType;
  VOID                        *UriParser;
  EFI_DHCP4_PACKET_OPTION     *OptList[HTTP_BOOT_DHCP4_TAG_INDEX_MAX];
} HTTP_BOOT_DHCP4_PACKET_CACHE;

/**
  Select an DHCPv4 or DHCP6 offer, and record SelectIndex and SelectProxyType.

  @param[in]  Private             Pointer to HTTP boot driver private data.

**/
VOID
HttpBootSelectDhcpOffer (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  );

/**
  Start the D.O.R.A DHCPv4 process to acquire the IPv4 address and other Http boot information.

  @param[in]  Private           Pointer to HTTP_BOOT private data.

  @retval EFI_SUCCESS           The D.O.R.A process successfully finished.
  @retval Others                Failed to finish the D.O.R.A process.

**/
EFI_STATUS
HttpBootDhcp4Dora (
  IN HTTP_BOOT_PRIVATE_DATA         *Private
  );

/**
  This function will register the default DNS addresses to the network device.

  @param[in]  Private             The pointer to HTTP_BOOT_PRIVATE_DATA.
  @param[in]  DataLength          Size of the buffer pointed to by DnsServerData in bytes.
  @param[in]  DnsServerData       Point a list of DNS server address in an array
                                  of EFI_IPv4_ADDRESS instances.

  @retval     EFI_SUCCESS         The DNS configuration has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
HttpBootRegisterIp4Dns (
  IN HTTP_BOOT_PRIVATE_DATA         *Private,
  IN UINTN                          DataLength,
  IN VOID                           *DnsServerData
  );

#endif
