/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcDhcp.h

Abstract:

  Dhcp and Discover routines for PxeBc


**/

#ifndef __EFI_PXEBC_DHCP_H__
#define __EFI_PXEBC_DHCP_H__

enum {
  PXEBC_DHCP4_MAX_OPTION_NUM        = 16,
  PXEBC_DHCP4_MAX_OPTION_SIZE       = 312,
  PXEBC_DHCP4_MAX_PACKET_SIZE       = 1472,

  PXEBC_DHCP4_S_PORT                = 67,
  PXEBC_DHCP4_C_PORT                = 68,
  PXEBC_BS_DOWNLOAD_PORT            = 69,
  PXEBC_BS_DISCOVER_PORT            = 4011,

  PXEBC_DHCP4_OPCODE_REQUEST        = 1,
  PXEBC_DHCP4_OPCODE_REPLY          = 2,
  PXEBC_DHCP4_MSG_TYPE_REQUEST      = 3,
  PXEBC_DHCP4_MAGIC                 = 0x63538263, // network byte order
  //
  // Dhcp Options
  //
  PXEBC_DHCP4_TAG_PAD               = 0,    // Pad Option
  PXEBC_DHCP4_TAG_EOP               = 255,  // End Option
  PXEBC_DHCP4_TAG_NETMASK           = 1,    // Subnet Mask
  PXEBC_DHCP4_TAG_TIME_OFFSET       = 2,    // Time Offset from UTC
  PXEBC_DHCP4_TAG_ROUTER            = 3,    // Router option,
  PXEBC_DHCP4_TAG_TIME_SERVER       = 4,    // Time Server
  PXEBC_DHCP4_TAG_NAME_SERVER       = 5,    // Name Server
  PXEBC_DHCP4_TAG_DNS_SERVER        = 6,    // Domain Name Server
  PXEBC_DHCP4_TAG_HOSTNAME          = 12,   // Host Name
  PXEBC_DHCP4_TAG_BOOTFILE_LEN      = 13,   // Boot File Size
  PXEBC_DHCP4_TAG_DUMP              = 14,   // Merit Dump File
  PXEBC_DHCP4_TAG_DOMAINNAME        = 15,   // Domain Name
  PXEBC_DHCP4_TAG_ROOTPATH          = 17,   // Root path
  PXEBC_DHCP4_TAG_EXTEND_PATH       = 18,   // Extensions Path
  PXEBC_DHCP4_TAG_EMTU              = 22,   // Maximum Datagram Reassembly Size
  PXEBC_DHCP4_TAG_TTL               = 23,   // Default IP Time-to-live
  PXEBC_DHCP4_TAG_BROADCAST         = 28,   // Broadcast Address
  PXEBC_DHCP4_TAG_NIS_DOMAIN        = 40,   // Network Information Service Domain
  PXEBC_DHCP4_TAG_NIS_SERVER        = 41,   // Network Information Servers
  PXEBC_DHCP4_TAG_NTP_SERVER        = 42,   // Network Time Protocol Servers
  PXEBC_DHCP4_TAG_VENDOR            = 43,   // Vendor Specific Information
  PXEBC_DHCP4_TAG_REQUEST_IP        = 50,   // Requested IP Address
  PXEBC_DHCP4_TAG_LEASE             = 51,   // IP Address Lease Time
  PXEBC_DHCP4_TAG_OVERLOAD          = 52,   // Option Overload
  PXEBC_DHCP4_TAG_MSG_TYPE          = 53,   // DHCP Message Type
  PXEBC_DHCP4_TAG_SERVER_ID         = 54,   // Server Identifier
  PXEBC_DHCP4_TAG_PARA_LIST         = 55,   // Parameter Request List
  PXEBC_DHCP4_TAG_MAXMSG            = 57,   // Maximum DHCP Message Size
  PXEBC_DHCP4_TAG_T1                = 58,   // Renewal (T1) Time Value
  PXEBC_DHCP4_TAG_T2                = 59,   // Rebinding (T2) Time Value
  PXEBC_DHCP4_TAG_CLASS_ID          = 60,   // Vendor class identifier
  PXEBC_DHCP4_TAG_CLIENT_ID         = 61,   // Client-identifier
  PXEBC_DHCP4_TAG_TFTP              = 66,   // TFTP server name
  PXEBC_DHCP4_TAG_BOOTFILE          = 67,   // Bootfile name
  PXEBC_PXE_DHCP4_TAG_ARCH          = 93,
  PXEBC_PXE_DHCP4_TAG_UNDI          = 94,
  PXEBC_PXE_DHCP4_TAG_UUID          = 97,
  //
  // Sub-Options in Dhcp Vendor Option
  //
  PXEBC_VENDOR_TAG_MTFTP_IP         = 1,
  PXEBC_VENDOR_TAG_MTFTP_CPORT      = 2,
  PXEBC_VENDOR_TAG_MTFTP_SPORT      = 3,
  PXEBC_VENDOR_TAG_MTFTP_TIMEOUT    = 4,
  PXEBC_VENDOR_TAG_MTFTP_DELAY      = 5,
  PXEBC_VENDOR_TAG_DISCOVER_CTRL    = 6,
  PXEBC_VENDOR_TAG_DISCOVER_MCAST   = 7,
  PXEBC_VENDOR_TAG_BOOT_SERVERS     = 8,
  PXEBC_VENDOR_TAG_BOOT_MENU        = 9,
  PXEBC_VENDOR_TAG_MENU_PROMPT      = 10,
  PXEBC_VENDOR_TAG_MCAST_ALLOC      = 11,
  PXEBC_VENDOR_TAG_CREDENTIAL_TYPES = 12,
  PXEBC_VENDOR_TAG_BOOT_ITEM        = 71,

  PXEBC_DHCP4_DISCOVER_INIT_TIMEOUT = 4,
  PXEBC_DHCP4_DISCOVER_RETRIES      = 4,

  PXEBC_MAX_MENU_NUM                = 24,
  PXEBC_MAX_OFFER_NUM               = 16,

  PXEBC_BOOT_REQUEST_TIMEOUT        = 1,
  PXEBC_BOOT_REQUEST_RETRIES        = 4,

  PXEBC_DHCP4_OVERLOAD_FILE         = 1,
  PXEBC_DHCP4_OVERLOAD_SERVER_NAME  = 2
};

//
// The array index of the DHCP4 option tag interested
//
enum {
  PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN,
  PXEBC_DHCP4_TAG_INDEX_VENDOR,
  PXEBC_DHCP4_TAG_INDEX_OVERLOAD,
  PXEBC_DHCP4_TAG_INDEX_MSG_TYPE,
  PXEBC_DHCP4_TAG_INDEX_SERVER_ID,
  PXEBC_DHCP4_TAG_INDEX_CLASS_ID,
  PXEBC_DHCP4_TAG_INDEX_BOOTFILE,
  PXEBC_DHCP4_TAG_INDEX_MAX
};

//
// The type of DHCP OFFER, arranged by priority, PXE10 has the highest priority.
//
enum {
  DHCP4_PACKET_TYPE_PXE10,
  DHCP4_PACKET_TYPE_WFM11A,
  DHCP4_PACKET_TYPE_BINL,
  DHCP4_PACKET_TYPE_DHCP_ONLY,
  DHCP4_PACKET_TYPE_MAX,
  DHCP4_PACKET_TYPE_BOOTP           = DHCP4_PACKET_TYPE_MAX
};

#define BIT(x)  (1 << x)
#define CTRL(x) (0x1F & (x))

//
// WfM11a options
//
#define MTFTP_VENDOR_OPTION_BIT_MAP (BIT (PXEBC_VENDOR_TAG_MTFTP_IP) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_CPORT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_SPORT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_TIMEOUT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_DELAY))
//
// Discoverty options
//
#define DISCOVER_VENDOR_OPTION_BIT_MAP  (BIT (PXEBC_VENDOR_TAG_DISCOVER_CTRL) | \
                                         BIT (PXEBC_VENDOR_TAG_DISCOVER_MCAST) | \
                                         BIT (PXEBC_VENDOR_TAG_BOOT_SERVERS) | \
                                         BIT (PXEBC_VENDOR_TAG_BOOT_MENU) | \
                                         BIT (PXEBC_VENDOR_TAG_MENU_PROMPT))

#define IS_VALID_BOOT_PROMPT(x) \
  ((((x)[0]) & BIT (PXEBC_VENDOR_TAG_MENU_PROMPT)) == BIT (PXEBC_VENDOR_TAG_MENU_PROMPT))

#define IS_VALID_BOOT_MENU(x) \
  ((((x)[0]) & BIT (PXEBC_VENDOR_TAG_BOOT_MENU)) == BIT (PXEBC_VENDOR_TAG_BOOT_MENU))

#define IS_VALID_MTFTP_VENDOR_OPTION(x) \
    (((UINT32) ((x)[0]) & MTFTP_VENDOR_OPTION_BIT_MAP) == MTFTP_VENDOR_OPTION_BIT_MAP)

#define IS_VALID_DISCOVER_VENDOR_OPTION(x)  (((UINT32) ((x)[0]) & DISCOVER_VENDOR_OPTION_BIT_MAP) != 0)

#define IS_VALID_CREDENTIAL_VENDOR_OPTION(x) \
    (((UINT32) ((x)[0]) & BIT (PXEBC_VENDOR_TAG_CREDENTIAL_TYPES)) == BIT (PXEBC_VENDOR_TAG_CREDENTIAL_TYPES))

#define IS_VALID_BOOTITEM_VENDOR_OPTION(x) \
    (((UINT32) ((x)[PXEBC_VENDOR_TAG_BOOT_ITEM / 32]) & BIT (PXEBC_VENDOR_TAG_BOOT_ITEM % 32)) \
      == BIT (PXEBC_VENDOR_TAG_BOOT_ITEM % 32))

#define IS_DISABLE_BCAST_DISCOVER(x)    (((x) & BIT (0)) == BIT (0))
#define IS_DISABLE_MCAST_DISCOVER(x)    (((x) & BIT (1)) == BIT (1))
#define IS_ENABLE_USE_SERVER_LIST(x)    (((x) & BIT (2)) == BIT (2))
#define IS_ENABLE_BOOT_FILE_NAME(x)     (((x) & BIT (3)) == BIT (3))

#define SET_VENDOR_OPTION_BIT_MAP(x, y) (((x)[(y) / 32]) = (UINT32) ((x)[(y) / 32]) | BIT ((y) % 32))

#pragma pack(1)
typedef struct {
  UINT8 ParaList[135];
} PXEBC_DHCP4_OPTION_PARA;

typedef struct {
  UINT16  Size;
} PXEBC_DHCP4_OPTION_MAX_MESG_SIZE;

typedef struct {
  UINT8 Type;
  UINT8 MajorVer;
  UINT8 MinorVer;
} PXEBC_DHCP4_OPTION_UNDI;

typedef struct {
  UINT8 Type;
} PXEBC_DHCP4_OPTION_MESG;

typedef struct {
  UINT16  Type;
} PXEBC_DHCP4_OPTION_ARCH;

#define DEFAULT_CLASS_ID_DATA "PXEClient:Arch:?????:????:??????"

typedef struct {
  UINT8 ClassIdentifier[10];
  UINT8 ArchitecturePrefix[5];
  UINT8 ArchitectureType[5];
  UINT8 Lit3[1];
  UINT8 InterfaceName[4];
  UINT8 Lit4[1];
  UINT8 UndiMajor[3];
  UINT8 UndiMinor[3];
} PXEBC_DHCP4_OPTION_CLID;

typedef struct {
  UINT8 Type;
  UINT8 Guid[16];
} PXEBC_DHCP4_OPTION_UUID;

typedef struct {
  UINT16  Type;
  UINT16  Layer;
} PXEBC_OPTION_BOOT_ITEM;

#pragma pack()

typedef union {
  PXEBC_DHCP4_OPTION_PARA           *Para;
  PXEBC_DHCP4_OPTION_UNDI           *Undi;
  PXEBC_DHCP4_OPTION_ARCH           *Arch;
  PXEBC_DHCP4_OPTION_CLID           *Clid;
  PXEBC_DHCP4_OPTION_UUID           *Uuid;
  PXEBC_DHCP4_OPTION_MESG           *Mesg;
  PXEBC_DHCP4_OPTION_MAX_MESG_SIZE  *MaxMesgSize;
} PXEBC_DHCP4_OPTION_ENTRY;

typedef struct {
  UINT16            Type;
  UINT8             IpCnt;
  EFI_IPv4_ADDRESS  IpAddr[1];
} PXEBC_BOOT_SVR_ENTRY;

typedef struct {
  UINT16  Type;
  UINT8   DescLen;
  UINT8   DescStr[1];
} PXEBC_BOOT_MENU_ENTRY;

typedef struct {
  UINT8 Timeout;
  UINT8 Prompt[1];
} PXEBC_MENU_PROMPT;

typedef struct {
  UINT32                BitMap[8];
  EFI_IPv4_ADDRESS      MtftpIp;
  UINT16                MtftpCPort;
  UINT16                MtftpSPort;
  UINT8                 MtftpTimeout;
  UINT8                 MtftpDelay;
  UINT8                 DiscoverCtrl;
  EFI_IPv4_ADDRESS      DiscoverMcastIp;
  EFI_IPv4_ADDRESS      McastIpBase;
  UINT16                McastIpBlock;
  UINT16                McastIpRange;
  UINT16                BootSrvType;
  UINT16                BootSrvLayer;
  PXEBC_BOOT_SVR_ENTRY  *BootSvr;
  UINT8                 BootSvrLen;
  PXEBC_BOOT_MENU_ENTRY *BootMenu;
  UINT8                 BootMenuLen;
  PXEBC_MENU_PROMPT     *MenuPrompt;
  UINT8                 MenuPromptLen;
  UINT32                *CredType;
  UINT8                 CredTypeLen;
} PXEBC_VENDOR_OPTION;

#define PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE  (EFI_FIELD_OFFSET (EFI_DHCP4_PACKET, Dhcp4) + PXEBC_DHCP4_MAX_PACKET_SIZE)

typedef struct {
  union {
    EFI_DHCP4_PACKET  Offer;
    EFI_DHCP4_PACKET  Ack;
    UINT8             Buffer[PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE];
  } Packet;

  BOOLEAN                 IsPxeOffer;
  UINT8                   OfferType;
  EFI_DHCP4_PACKET_OPTION *Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_MAX];
  PXEBC_VENDOR_OPTION    PxeVendorOption;
} PXEBC_CACHED_DHCP4_PACKET;

#define GET_NEXT_DHCP_OPTION(Opt) \
  (EFI_DHCP4_PACKET_OPTION *) ((UINT8 *) (Opt) + sizeof (EFI_DHCP4_PACKET_OPTION) + (Opt)->Length - 1)

#define GET_OPTION_BUFFER_LEN(Pkt)  ((Pkt)->Length - sizeof (EFI_DHCP4_HEADER) - 4)
#define IS_PROXY_DHCP_OFFER(Offer)  EFI_IP4_EQUAL (&((Offer)->Dhcp4.Header.YourAddr), &mZeroIp4Addr)

#define GET_NEXT_BOOT_SVR_ENTRY(Ent) \
  (PXEBC_BOOT_SVR_ENTRY *) ((UINT8 *) Ent + sizeof (*(Ent)) + ((Ent)->IpCnt - 1) * sizeof (EFI_IPv4_ADDRESS))

VOID
PxeBcInitSeedPacket (
  IN EFI_DHCP4_PACKET  *Seed,
  IN EFI_UDP4_PROTOCOL *Udp4
  )
/*++

Routine Description:

  GC_NOTO: Add function description

Arguments:

  Seed  - GC_NOTO: add argument description
  Udp4  - GC_NOTO: add argument description

Returns:

  GC_NOTO: add return values

--*/
;


/**
  GC_NOTO: Add function description

  @param  CachedPacket    GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
BOOLEAN
PxeBcParseCachedDhcpPacket (
  IN PXEBC_CACHED_DHCP4_PACKET  *CachedPacket
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcCheckSelectedOffer (
  IN PXEBC_PRIVATE_DATA  *Private
  )
;


/**
  GC_NOTO: Add function description

  @param  This            GC_NOTO: add argument description
  @param  Context         GC_NOTO: add argument description
  @param  CurrentState    GC_NOTO: add argument description
  @param  Dhcp4Event      GC_NOTO: add argument description
  @param  Packet          GC_NOTO: add argument description
  @param  NewPacket       GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcDhcpCallBack (
  IN EFI_DHCP4_PROTOCOL                * This,
  IN VOID                              *Context,
  IN EFI_DHCP4_STATE                   CurrentState,
  IN EFI_DHCP4_EVENT                   Dhcp4Event,
  IN EFI_DHCP4_PACKET                  * Packet OPTIONAL,
  OUT EFI_DHCP4_PACKET                 **NewPacket OPTIONAL
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description
  @param  Type            GC_NOTO: add argument description
  @param  Layer           GC_NOTO: add argument description
  @param  UseBis          GC_NOTO: add argument description
  @param  DestIp          GC_NOTO: add argument description
  @param  IpCount         GC_NOTO: add argument description
  @param  SrvList         GC_NOTO: add argument description
  @param  IsDiscv         GC_NOTO: add argument description
  @param  Reply           GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcDiscvBootService (
  IN PXEBC_PRIVATE_DATA                * Private,
  IN UINT16                            Type,
  IN UINT16                            *Layer,
  IN BOOLEAN                           UseBis,
  IN EFI_IP_ADDRESS                    * DestIp,
  IN UINT16                            IpCount,
  IN EFI_PXE_BASE_CODE_SRVLIST         * SrvList,
  IN BOOLEAN                           IsDiscv,
  OUT EFI_DHCP4_PACKET                 * Reply OPTIONAL
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description
  @param  OptList         GC_NOTO: add argument description
  @param  IsDhcpDiscover  GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
UINT32
PxeBcBuildDhcpOptions (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_DHCP4_PACKET_OPTION       **OptList,
  IN BOOLEAN                       IsDhcpDiscover
  )
;


/**
  GC_NOTO: Add function description

  @param  OptList         GC_NOTO: add argument description
  @param  Type            GC_NOTO: add argument description
  @param  Layer           GC_NOTO: add argument description
  @param  OptLen          GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
VOID
PxeBcCreateBootOptions (
  IN  EFI_DHCP4_PACKET_OPTION          *OptList,
  IN  UINT16                           Type,
  IN  UINT16                           *Layer,
  OUT UINT32                           *OptLen
  )
;


/**
  GC_NOTO: Add function description

  @param  Buffer          GC_NOTO: add argument description
  @param  Length          GC_NOTO: add argument description
  @param  OptTag          GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_DHCP4_PACKET_OPTION *
PxeBcParseExtendOptions (
  IN UINT8                         *Buffer,
  IN UINT32                        Length,
  IN UINT8                         OptTag
  )
;


/**
  GC_NOTO: Add function description

  @param  Dhcp4Option     GC_NOTO: add argument description
  @param  VendorOption    GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
BOOLEAN
PxeBcParseVendorOptions (
  IN EFI_DHCP4_PACKET_OPTION       *Dhcp4Option,
  IN PXEBC_VENDOR_OPTION          *VendorOption
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description
  @param  Info            GC_NOTO: add argument description
  @param  Type            GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcSelectBootServers (
  IN  PXEBC_PRIVATE_DATA               *Private,
  OUT EFI_PXE_BASE_CODE_DISCOVER_INFO  **Info,
  OUT UINT16                           *Type
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcSelectBootPrompt (
  IN PXEBC_PRIVATE_DATA              *Private
  )
;


/**
  GC_NOTO: Add function description

  @param  Private         GC_NOTO: add argument description
  @param  Type            GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
EFI_STATUS
PxeBcSelectBootMenu (
  IN  PXEBC_PRIVATE_DATA              *Private,
  OUT UINT16                          *Type,
  IN  BOOLEAN                         UseDefaultItem
  )
;

#endif

