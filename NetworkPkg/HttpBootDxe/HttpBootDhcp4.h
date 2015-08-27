/** @file
  Functions declaration related with DHCPv4 for HTTP boot driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

//
// Dhcp Options
//
#define HTTP_BOOT_DHCP4_TAG_PAD                0    // Pad Option
#define HTTP_BOOT_DHCP4_TAG_EOP                255  // End Option
#define HTTP_BOOT_DHCP4_TAG_NETMASK            1    // Subnet Mask
#define HTTP_BOOT_DHCP4_TAG_TIME_OFFSET        2    // Time Offset from UTC
#define HTTP_BOOT_DHCP4_TAG_ROUTER             3    // Router option,
#define HTTP_BOOT_DHCP4_TAG_TIME_SERVER        4    // Time Server
#define HTTP_BOOT_DHCP4_TAG_NAME_SERVER        5    // Name Server
#define HTTP_BOOT_DHCP4_TAG_DNS_SERVER         6    // Domain Name Server
#define HTTP_BOOT_DHCP4_TAG_HOSTNAME           12   // Host Name
#define HTTP_BOOT_DHCP4_TAG_BOOTFILE_LEN       13   // Boot File Size
#define HTTP_BOOT_DHCP4_TAG_DUMP               14   // Merit Dump File
#define HTTP_BOOT_DHCP4_TAG_DOMAINNAME         15   // Domain Name
#define HTTP_BOOT_DHCP4_TAG_ROOTPATH           17   // Root path
#define HTTP_BOOT_DHCP4_TAG_EXTEND_PATH        18   // Extensions Path
#define HTTP_BOOT_DHCP4_TAG_EMTU               22   // Maximum Datagram Reassembly Size
#define HTTP_BOOT_DHCP4_TAG_TTL                23   // Default IP Time-to-live
#define HTTP_BOOT_DHCP4_TAG_BROADCAST          28   // Broadcast Address
#define HTTP_BOOT_DHCP4_TAG_NIS_DOMAIN         40   // Network Information Service Domain
#define HTTP_BOOT_DHCP4_TAG_NIS_SERVER         41   // Network Information Servers
#define HTTP_BOOT_DHCP4_TAG_NTP_SERVER         42   // Network Time Protocol Servers
#define HTTP_BOOT_DHCP4_TAG_VENDOR             43   // Vendor Specific Information
#define HTTP_BOOT_DHCP4_TAG_REQUEST_IP         50   // Requested IP Address
#define HTTP_BOOT_DHCP4_TAG_LEASE              51   // IP Address Lease Time
#define HTTP_BOOT_DHCP4_TAG_OVERLOAD           52   // Option Overload
#define HTTP_BOOT_DHCP4_TAG_MSG_TYPE           53   // DHCP Message Type
#define HTTP_BOOT_DHCP4_TAG_SERVER_ID          54   // Server Identifier
#define HTTP_BOOT_DHCP4_TAG_PARA_LIST          55   // Parameter Request List
#define HTTP_BOOT_DHCP4_TAG_MAXMSG             57   // Maximum DHCP Message Size
#define HTTP_BOOT_DHCP4_TAG_T1                 58   // Renewal (T1) Time Value
#define HTTP_BOOT_DHCP4_TAG_T2                 59   // Rebinding (T2) Time Value
#define HTTP_BOOT_DHCP4_TAG_CLASS_ID           60   // Vendor class identifier
#define HTTP_BOOT_DHCP4_TAG_CLIENT_ID          61   // Client-identifier
#define HTTP_BOOT_DHCP4_TAG_TFTP               66   // TFTP server name
#define HTTP_BOOT_DHCP4_TAG_BOOTFILE           67   // Bootfile name
#define HTTP_BOOT_DHCP4_TAG_ARCH               93
#define HTTP_BOOT_DHCP4_TAG_UNDI               94
#define HTTP_BOOT_DHCP4_TAG_UUID               97

#define HTTP_BOOT_DHCP4_OVERLOAD_FILE          1
#define HTTP_BOOT_DHCP4_OVERLOAD_SERVER_NAME   2

///
/// HTTP Tag definition that identifies the processor 
/// and programming environment of the client system.
/// These identifiers are defined by IETF:
/// http://www.ietf.org/assignments/dhcpv6-parameters/dhcpv6-parameters.xml
///
#if defined (MDE_CPU_IA32)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    0x000F
#elif defined (MDE_CPU_X64)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    0x0010
#elif defined (MDE_CPU_ARM)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    0x0012
#elif defined (MDE_CPU_AARCH64)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    0x0013
#elif defined (MDE_CPU_EBC)
#define EFI_HTTP_BOOT_CLIENT_SYSTEM_ARCHITECTURE    0x0011
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
  // <IP address, IP expressed URI> or
  // <IP address, IP expressed URI, Name-server (will be ignored)>
  //
  HttpOfferTypeDhcpIpUri,
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

typedef union {
  EFI_DHCP4_PACKET        Offer;
  EFI_DHCP4_PACKET        Ack;
  UINT8                   Buffer[HTTP_BOOT_DHCP4_PACKET_MAX_SIZE];
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
