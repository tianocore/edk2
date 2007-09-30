/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _DHCP_H
#define _DHCP_H

//
// Definitions for DHCP version 4 UDP packet.
// The field names in this structure are defined and described in RFC 2131.
//
#pragma pack(1)

typedef struct {
  UINT8   op;
#define BOOTP_REQUEST 1
#define BOOTP_REPLY   2

  UINT8   htype;
  UINT8   hlen;
  UINT8   hops;
  UINT32  xid;
  UINT16  secs;
  UINT16  flags;
#define DHCP_BROADCAST_FLAG 0x8000

  UINT32  ciaddr;
  UINT32  yiaddr;
  UINT32  siaddr;
  UINT32  giaddr;
  UINT8   chaddr[16];
  UINT8   sname[64];
  UINT8   file[128];
  UINT8   options[312];
#define OP_PAD                              0
#define OP_END                              255
#define OP_SUBNET_MASK                      1
#define OP_TIME_OFFSET                      2
#define OP_ROUTER_LIST                      3
#define OP_TIME_SERVERS                     4
#define OP_NAME_SERVERS                     5
#define OP_DNS_SERVERS                      6
#define OP_LOG_SERVERS                      7
#define OP_COOKIE_SERVERS                   8
#define OP_LPR_SREVERS                      9
#define OP_IMPRESS_SERVERS                  10
#define OP_RES_LOC_SERVERS                  11
#define OP_HOST_NAME                        12
#define OP_BOOT_FILE_SZ                     13
#define OP_DUMP_FILE                        14
#define OP_DOMAIN_NAME                      15
#define OP_SWAP_SERVER                      16
#define OP_ROOT_PATH                        17
#define OP_EXTENSION_PATH                   18
#define OP_IP_FORWARDING                    19
#define OP_NON_LOCAL_SRC_RTE                20
#define OP_POLICY_FILTER                    21
#define OP_MAX_DATAGRAM_SZ                  22
#define OP_DEFAULT_TTL                      23
#define OP_MTU_AGING_TIMEOUT                24
#define OP_MTU_SIZES                        25
#define OP_MTU_TO_USE                       26
#define OP_ALL_SUBNETS_LOCAL                27
#define OP_BROADCAST_ADD                    28
#define OP_PERFORM_MASK_DISCOVERY           29
#define OP_RESPOND_TO_MASK_REQ              30
#define OP_PERFORM_ROUTER_DISCOVERY         31
#define OP_ROUTER_SOLICIT_ADDRESS           32
#define OP_STATIC_ROUTER_LIST               33
#define OP_USE_ARP_TRAILERS                 34
#define OP_ARP_CACHE_TIMEOUT                35
#define OP_ETHERNET_ENCAPSULATION           36
#define OP_TCP_DEFAULT_TTL                  37
#define OP_TCP_KEEP_ALIVE_INT               38
#define OP_KEEP_ALIVE_GARBAGE               39
#define OP_NIS_DOMAIN_NAME                  40
#define OP_NIS_SERVERS                      41
#define OP_NTP_SERVERS                      42
#define OP_VENDOR_SPECIFIC                  43
#define VEND_PXE_MTFTP_IP                   1
#define VEND_PXE_MTFTP_CPORT                2
#define VEND_PXE_MTFTP_SPORT                3
#define VEND_PXE_MTFTP_TMOUT                4
#define VEND_PXE_MTFTP_DELAY                5
#define VEND_PXE_DISCOVERY_CONTROL          6
#define PXE_DISABLE_BROADCAST_DISCOVERY     (1 << 0)
#define PXE_DISABLE_MULTICAST_DISCOVERY     (1 << 1)
#define PXE_ACCEPT_ONLY_PXE_BOOT_SERVERS    (1 << 2)
#define PXE_DO_NOT_PROMPT                   (1 << 3)
#define VEND_PXE_DISCOVERY_MCAST_ADDR       7
#define VEND_PXE_BOOT_SERVERS               8
#define VEND_PXE_BOOT_MENU                  9
#define VEND_PXE_BOOT_PROMPT                10
#define VEND_PXE_MCAST_ADDRS_ALLOC          11
#define VEND_PXE_CREDENTIAL_TYPES           12
#define VEND_PXE_BOOT_ITEM                  71
#define OP_NBNS_SERVERS                     44
#define OP_NBDD_SERVERS                     45
#define OP_NETBIOS_NODE_TYPE                46
#define OP_NETBIOS_SCOPE                    47
#define OP_XWINDOW_SYSTEM_FONT_SERVERS      48
#define OP_XWINDOW_SYSTEM_DISPLAY_MANAGERS  49
#define OP_DHCP_REQ_IP_ADD                  50
#define OP_DHCP_LEASE_TIME                  51
#define OP_DHCP_OPTION_OVERLOAD             52
#define OVLD_FILE                           1
#define OVLD_SRVR_NAME                      2
#define OP_DHCP_MESSAGE_TYPE                53
#define DHCPDISCOVER                        1
#define DHCPOFFER                           2
#define DHCPREQUEST                         3
#define DHCPDECLINE                         4
#define DHCPACK                             5
#define DHCPNAK                             6
#define DHCPRELEASE                         7
#define DHCPINFORM                          8
#define OP_DHCP_SERVER_IP                   54
#define OP_DHCP_PARM_REQ_LIST               55
#define OP_DHCP_ERROR_MESSAGE               56
#define OP_DHCP_MAX_MESSAGE_SZ              57
#define OP_DHCP_RENEWAL_TIME                58
#define OP_DHCP_REBINDING_TIME              59
#define OP_DHCP_CLASS_IDENTIFIER            60
#define OP_DHCP_CLIENT_IDENTIFIER           61
#define OP_NISPLUS_DOMAIN_NAME              64
#define OP_NISPLUS_SERVERS                  65
#define OP_DHCP_TFTP_SERVER_NAME            66
#define OP_DHCP_BOOTFILE                    67
#define OP_MOBILE_IP_HOME_AGENTS            68
#define OP_SMPT_SERVERS                     69
#define OP_POP3_SERVERS                     70
#define OP_NNTP_SERVERS                     71
#define OP_WWW_SERVERS                      72
#define OP_FINGER_SERVERS                   73
#define OP_IRC_SERVERS                      74
#define OP_STREET_TALK_SERVERS              75
#define OP_STREET_TALK_DIR_ASSIST_SERVERS   76
#define OP_NDS_SERVERS                      85
#define OP_NDS_TREE_NAME                    86
#define OP_NDS_CONTEXT                      87
#define OP_DHCP_SYSTEM_ARCH                 93
#define OP_DHCP_NETWORK_ARCH                94
#define OP_DHCP_PLATFORM_ID                 97
} DHCPV4_STRUCT;

//
// DHCPv4 option header
//
typedef struct {
  UINT8 OpCode;
  UINT8 Length;
  //
  // followed by Data[]
  //
} DHCPV4_OP_HEADER;

//
// Generic DHCPv4 option (header followed by data)
//
typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Data[1];
} DHCPV4_OP_STRUCT;

//
// Maximum DHCP packet size on ethernet
//
#define MAX_DHCP_MSG_SZ (MAX_ENET_DATA_SIZE - sizeof (IPV4_HEADER) - sizeof (UDPV4_HEADER))

//
// Macros used in pxe_bc_dhcp.c and pxe_loadfile.c
//
#define DHCPV4_TRANSMIT_BUFFER  (*(DHCPV4_STRUCT *) (Private->TransmitBuffer))
#define DHCPV4_OPTIONS_BUFFER   (*(struct optionsstr *) DHCPV4_TRANSMIT_BUFFER.options)

#define DHCPV4_ACK_INDEX        0
#define PXE_BINL_INDEX          1
#define PXE_OFFER_INDEX         1
#define PXE_ACK_INDEX           2
#define PXE_BIS_INDEX           3

#define DHCPV4_ACK_BUFFER       ((struct DhcpReceiveBufferStruct *) Private->DhcpPacketBuffer)[DHCPV4_ACK_INDEX]
#define PXE_BINL_BUFFER         ((struct DhcpReceiveBufferStruct *) Private->DhcpPacketBuffer)[PXE_BINL_INDEX]
#define PXE_OFFER_BUFFER        ((struct DhcpReceiveBufferStruct *) Private->DhcpPacketBuffer)[PXE_OFFER_INDEX]
#define PXE_ACK_BUFFER          ((struct DhcpReceiveBufferStruct *) Private->DhcpPacketBuffer)[PXE_ACK_INDEX]
#define PXE_BIS_BUFFER          ((struct DhcpReceiveBufferStruct *) Private->DhcpPacketBuffer)[PXE_BIS_INDEX]

#define DHCPV4_ACK_PACKET       DHCPV4_ACK_BUFFER.u.Dhcpv4
#define PXE_BINL_PACKET         PXE_BINL_BUFFER.u.Dhcpv4
#define PXE_OFFER_PACKET        PXE_OFFER_BUFFER.u.Dhcpv4
#define PXE_ACK_PACKET          PXE_ACK_BUFFER.u.Dhcpv4
#define PXE_BIS_PACKET          PXE_BIS_BUFFER.u.Dhcpv4

//
// network structure definitions
//
//
// some option definitions
//
#define DHCPV4_OPTION_LENGTH(type)  (sizeof (type) - sizeof (DHCPV4_OP_HEADER))

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Type;
} DHCPV4_OP_MESSAGE_TYPE;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Overload;
} DHCPV4_OP_OVERLOAD;

//
// boot server list structure
// one or more contained in a pxe boot servers structure
//
typedef struct {
  UINT8             IpCount;
  EFI_IPv4_ADDRESS  IpList[1];  // IP count of IPs
} PXEV4_SERVER_LIST;

typedef struct {
  UINT8             IpCount;
  EFI_IPv6_ADDRESS  IpList[1];  // IP count of IPs
} PXEV6_SERVER_LIST;

typedef union {
  PXEV4_SERVER_LIST Ipv4List;
  PXEV6_SERVER_LIST Ipv6List;
} PXE_SERVER_LISTS;

typedef struct {
  UINT16            Type;
  PXE_SERVER_LISTS  u;
} PXE_SERVER_LIST;

//
// pxe boot servers structure
//
typedef struct {
  DHCPV4_OP_HEADER  Header;
  PXE_SERVER_LIST   ServerList[1];  // one or more
} PXE_OP_SERVER_LIST;

//
// pxe boot item structure
//
typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT16            Type;
  UINT16            Layer;
} PXE_OP_BOOT_ITEM;

//
// pxe boot menu item structure
//
typedef struct {
  UINT16  Type;
  UINT8   DataLen;
  UINT8   Data[1];
} PXE_BOOT_MENU_ENTRY;

//
// pxe boot menu structure
//
typedef struct {
  DHCPV4_OP_HEADER    Header;
  PXE_BOOT_MENU_ENTRY MenuItem[1];
} PXE_OP_BOOT_MENU;

//
// pxe boot prompt structure
//
typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Timeout;
  UINT8             Prompt[1];
} PXE_OP_BOOT_PROMPT;

#define PXE_BOOT_PROMPT_AUTO_SELECT 0
#define PXE_BOOT_PROMPT_NO_TIMEOUT  255

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Class[1];
} DHCPV4_OP_CLASS;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             File[1];
} DHCPV4_OP_BOOTFILE;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             VendorOptions[1];
} DHCPV4_OP_VENDOR_OPTIONS;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             MaxSize[2];
} DHCPV4_OP_MAX_MESSAGE_SIZE;

typedef struct {
  UINT8 _OP_SUBNET_MASK;            /* 1 */
  UINT8 _OP_TIME_OFFSET;            /* 2 */
  UINT8 _OP_ROUTER_LIST;            /* 3 */
  UINT8 _OP_TIME_SERVERS;           /* 4 */
  UINT8 _OP_NAME_SERVERS;           /* 5 */
  UINT8 _OP_DNS_SERVERS;            /* 6 */
  UINT8 _OP_HOST_NAME;              /* 12 */
  UINT8 _OP_BOOT_FILE_SZ;           /* 13 */
  UINT8 _OP_DOMAIN_NAME;            /* 15 */
  UINT8 _OP_ROOT_PATH;              /* 17 */
  UINT8 _OP_EXTENSION_PATH;         /* 18 */
  UINT8 _OP_MAX_DATAGRAM_SZ;        /* 22 */
  UINT8 _OP_DEFAULT_TTL;            /* 23 */
  UINT8 _OP_BROADCAST_ADD;          /* 28 */
  UINT8 _OP_NIS_DOMAIN_NAME;        /* 40 */
  UINT8 _OP_NIS_SERVERS;            /* 41 */
  UINT8 _OP_NTP_SERVERS;            /* 42 */
  UINT8 _OP_VENDOR_SPECIFIC;        /* 43 */
  UINT8 _OP_DHCP_REQ_IP_ADD;        /* 50 */
  UINT8 _OP_DHCP_LEASE_TIME;        /* 51 */
  UINT8 _OP_DHCP_SERVER_IP;         /* 54 */
  UINT8 _OP_DHCP_RENEWAL_TIME;      /* 58 */
  UINT8 _OP_DHCP_REBINDING_TIME;    /* 59 */
  UINT8 _OP_DHCP_CLASS_IDENTIFIER;  /* 60 */
  UINT8 _OP_DHCP_TFTP_SERVER_NAME;  /* 66 */
  UINT8 _OP_DHCP_BOOTFILE;          /* 67 */
  UINT8 _OP_DHCP_PLATFORM_ID;       /* 97 */
  UINT8 VendorOption128;            //      vendor option 128
  UINT8 VendorOption129;            //      vendor option 129
  UINT8 VendorOption130;            //      vendor option 130
  UINT8 VendorOption131;            //      vendor option 131
  UINT8 VendorOption132;            //      vendor option 132
  UINT8 VendorOption133;            //      vendor option 133
  UINT8 VendorOption134;            //      vendor option 134
  UINT8 VendorOption135;            //      vendor option 135
} DHCPV4_REQUESTED_OPTIONS_DATA;

typedef struct {
  DHCPV4_OP_HEADER              Header;
  DHCPV4_REQUESTED_OPTIONS_DATA Data;
} DHCPV4_OP_REQUESTED_OPTIONS;

typedef struct opipstr {
  DHCPV4_OP_HEADER  Header;
  EFI_IPv4_ADDRESS  Ip;
} DHCPV4_OP_IP_ADDRESS;

//
// ip list structure - e.g. router list
//
typedef struct {
  DHCPV4_OP_HEADER  Header;
  EFI_IPv4_ADDRESS  IpList[1];
} DHCPV4_OP_IP_LIST;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Type;
  UINT8             Guid[sizeof (EFI_GUID)];
} DHCPV4_OP_CLIENT_ID;

//
// special options start - someday obsolete ???
//
#define DHCPV4_OP_PLATFORM_ID DHCPV4_OP_CLIENT_ID

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT8             Type; // SNP = 2
  UINT8             MajorVersion;
  UINT8             MinorVersion;
} DHCPV4_OP_NETWORK_INTERFACE;

#define UNDI_TYPE 1
#define SNP_TYPE  2

typedef struct {
  DHCPV4_OP_HEADER  Header;
  UINT16            Type;
} DHCPV4_OP_ARCHITECTURE_TYPE;
//
// special options end - someday obsolete ???
//
typedef struct {
  UINT8 ClassIdentifier[10];  // PXEClient:
  UINT8 Lit2[5];              // Arch:
  UINT8 ArchitectureType[5];  // 00000 - 65536
  UINT8 Lit3[1];              // :
  UINT8 InterfaceName[4];     // e.g. UNDI
  UINT8 Lit4[1];              // :
  UINT8 UndiMajor[3];         // 000 - 255
  UINT8 UndiMinor[3];         // 000 - 255
} DHCPV4_CLASS_ID_DATA;

typedef struct {
  DHCPV4_OP_HEADER      Header;
  DHCPV4_CLASS_ID_DATA  Data;
} DHCPV4_OP_CLASS_ID;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  EFI_IPv4_ADDRESS  Ip;
} DHCPV4_OP_REQUESTED_IP;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  EFI_IPv4_ADDRESS  Ip;
} DHCPV4_OP_SERVER_IP;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  EFI_IPv4_ADDRESS  Ip;
} DHCPV4_OP_SUBNET_MASK;

typedef struct {              // oppxedisctlstr {
  DHCPV4_OP_HEADER  Header;
  UINT8             ControlBits;
} PXE_OP_DISCOVERY_CONTROL;

#define DISABLE_BCAST   (1 << 0)
#define DISABLE_MCAST   (1 << 1)
#define USE_ACCEPT_LIST (1 << 2)
#define USE_BOOTFILE    (1 << 3)

#pragma pack()
//
// definitions of indices to populate option interest array
//
#define VEND_PXE_MTFTP_IP_IX              1                     // multicast IP address of bootfile for MTFTP listen
#define VEND_PXE_MTFTP_CPORT_IX           2                     // UDP Port to monitor for MTFTP responses - Intel order
#define VEND_PXE_MTFTP_SPORT_IX           3                     // Server UDP Port for MTFTP open - Intel order
#define VEND_PXE_MTFTP_TMOUT_IX           4                     // Listen timeout - secs
#define VEND_PXE_MTFTP_DELAY_IX           5                     // Transmission timeout - secs
#define VEND_PXE_DISCOVERY_CONTROL_IX     6                     // bit field
#define VEND_PXE_DISCOVERY_MCAST_ADDR_IX  7                     // boot server discovery multicast address
#define VEND_PXE_BOOT_SERVERS_IX          8                     // list of boot servers of form tp(2) cnt(1) ips[cnt]
#define VEND_PXE_BOOT_MENU_IX             9
#define VEND_PXE_BOOT_PROMPT_IX           10
#define VEND_PXE_MCAST_ADDRS_ALLOC_IX     0                     // not used by PXE client
#define VEND_PXE_CREDENTIAL_TYPES_IX      11
#define VEND_13_IX                        0                     // not used by PXE client
#define VEND_14_IX                        0                     // not used by PXE client
#define VEND_15_IX                        0                     // not used by PXE client
#define VEND_16_IX                        0                     // not used by PXE client
#define VEND_17_IX                        0                     // not used by PXE client
#define VEND_18_IX                        0                     // not used by PXE client
#define VEND_19_IX                        0                     // not used by PXE client
#define VEND_20_IX                        0                     // not used by PXE client
#define VEND_21_IX                        0                     // not used by PXE client
#define VEND_22_IX                        0                     // not used by PXE client
#define VEND_23_IX                        0                     // not used by PXE client
#define VEND_24_IX                        0                     // not used by PXE client
#define VEND_25_IX                        0                     // not used by PXE client
#define VEND_26_IX                        0                     // not used by PXE client
#define VEND_27_IX                        0                     // not used by PXE client
#define VEND_28_IX                        0                     // not used by PXE client
#define VEND_29_IX                        0                     // not used by PXE client
#define VEND_30_IX                        0                     // not used by PXE client
#define VEND_31_IX                        0                     // not used by PXE client
#define VEND_32_IX                        0                     // not used by PXE client
#define VEND_33_IX                        0                     // not used by PXE client
#define VEND_34_IX                        0                     // not used by PXE client
#define VEND_35_IX                        0                     // not used by PXE client
#define VEND_36_IX                        0                     // not used by PXE client
#define VEND_37_IX                        0                     // not used by PXE client
#define VEND_38_IX                        0                     // not used by PXE client
#define VEND_39_IX                        0                     // not used by PXE client
#define VEND_40_IX                        0                     // not used by PXE client
#define VEND_41_IX                        0                     // not used by PXE client
#define VEND_42_IX                        0                     // not used by PXE client
#define VEND_43_IX                        0                     // not used by PXE client
#define VEND_44_IX                        0                     // not used by PXE client
#define VEND_45_IX                        0                     // not used by PXE client
#define VEND_46_IX                        0                     // not used by PXE client
#define VEND_47_IX                        0                     // not used by PXE client
#define VEND_48_IX                        0                     // not used by PXE client
#define VEND_49_IX                        0                     // not used by PXE client
#define VEND_50_IX                        0                     // not used by PXE client
#define VEND_51_IX                        0                     // not used by PXE client
#define VEND_52_IX                        0                     // not used by PXE client
#define VEND_53_IX                        0                     // not used by PXE client
#define VEND_54_IX                        0                     // not used by PXE client
#define VEND_55_IX                        0                     // not used by PXE client
#define VEND_56_IX                        0                     // not used by PXE client
#define VEND_57_IX                        0                     // not used by PXE client
#define VEND_58_IX                        0                     // not used by PXE client
#define VEND_59_IX                        0                     // not used by PXE client
#define VEND_60_IX                        0                     // not used by PXE client
#define VEND_61_IX                        0                     // not used by PXE client
#define VEND_62_IX                        0                     // not used by PXE client
#define VEND_63_IX                        0                     // not used by PXE client
#define VEND_64_IX                        0                     // not used by PXE client
#define VEND_65_IX                        0                     // not used by PXE client
#define VEND_66_IX                        0                     // not used by PXE client
#define VEND_67_IX                        0                     // not used by PXE client
#define VEND_68_IX                        0                     // not used by PXE client
#define VEND_69_IX                        0                     // not used by PXE client
#define VEND_70_IX                        0                     // not used by PXE client
#define VEND_PXE_BOOT_ITEM_IX             12

#define MAX_OUR_PXE_OPT                   VEND_PXE_BOOT_ITEM    // largest PXE option in which we are interested
#define MAX_OUR_PXE_IX                    VEND_PXE_BOOT_ITEM_IX // largest PXE option index
//
// define various types by options that are sent
//
#define WfM11a_OPTS   ((1<<VEND_PXE_MTFTP_IP_IX) | \
                      (1<<VEND_PXE_MTFTP_CPORT_IX) | \
                      (1<<VEND_PXE_MTFTP_SPORT_IX) | \
                      (1<<VEND_PXE_MTFTP_TMOUT_IX) | \
                      (1<<VEND_PXE_MTFTP_DELAY_IX))

#define DISCOVER_OPTS ((1<<VEND_PXE_DISCOVERY_CONTROL_IX) | \
                      (1<<VEND_PXE_DISCOVERY_MCAST_ADDR_IX) | \
                      (1<<VEND_PXE_BOOT_SERVERS_IX) | \
                      (1<<VEND_PXE_BOOT_MENU_IX) | \
                      (1<<VEND_PXE_BOOT_PROMPT_IX) | \
                      (1<<VEND_PXE_BOOT_ITEM_IX))

#define CREDENTIALS_OPT (1 << VEND_PXE_CREDENTIAL_TYPES_IX)

//
// definitions of indices to populate option interest array
//
#define OP_SUBNET_MASK_IX                     1
#define OP_TIME_OFFSET_IX                     0 // not used by PXE client
#define OP_ROUTER_LIST_IX                     2
#define OP_TIME_SERVERS_IX                    0 // not used by PXE client
#define OP_NAME_SERVERS_IX                    0 // not used by PXE client
#define OP_DNS_SERVERS_IX                     0 // not used by PXE client
#define OP_LOG_SERVERS_IX                     0 // not used by PXE client
#define OP_COOKIE_SERVERS_IX                  0 // not used by PXE client
#define OP_LPR_SREVERS_IX                     0 // not used by PXE client
#define OP_IMPRESS_SERVERS_IX                 0 // not used by PXE client
#define OP_RES_LOC_SERVERS_IX                 0 // not used by PXE client
#define OP_HOST_NAME_IX                       0 // not used by PXE client
#define OP_BOOT_FILE_SZ_IX                    9
#define OP_DUMP_FILE_IX                       0 // not used by PXE client
#define OP_DOMAIN_NAME_IX                     0 // not used by PXE client
#define OP_SWAP_SERVER_IX                     0 // not used by PXE client
#define OP_ROOT_PATH_IX                       0 // not used by PXE client
#define OP_EXTENSION_PATH_IX                  0 // not used by PXE client
#define OP_IP_FORWARDING_IX                   0 // not used by PXE client
#define OP_NON_LOCAL_SRC_RTE_IX               0 // not used by PXE client
#define OP_POLICY_FILTER_IX                   0 // not used by PXE client
#define OP_MAX_DATAGRAM_SZ_IX                 0 // not used by PXE client
#define OP_DEFAULT_TTL_IX                     0 // not used by PXE client
#define OP_MTU_AGING_TIMEOUT_IX               0 // not used by PXE client
#define OP_MTU_SIZES_IX                       0 // not used by PXE client
#define OP_MTU_TO_USE_IX                      0 // not used by PXE client
#define OP_ALL_SUBNETS_LOCAL_IX               0 // not used by PXE client
#define OP_BROADCAST_ADD_IX                   0 // not used by PXE client
#define OP_PERFORM_MASK_DISCOVERY_IX          0 // not used by PXE client
#define OP_RESPOND_TO_MASK_REQ_IX             0 // not used by PXE client
#define OP_PERFORM_ROUTER_DISCOVERY_IX        0 // not used by PXE client
#define OP_ROUTER_SOLICIT_ADDRESS_IX          0 // not used by PXE client
#define OP_STATIC_ROUTER_LIST_IX              0 // not used by PXE client
#define OP_USE_ARP_TRAILERS_IX                0 // not used by PXE client
#define OP_ARP_CACHE_TIMEOUT_IX               0 // not used by PXE client
#define OP_ETHERNET_ENCAPSULATION_IX          0 // not used by PXE client
#define OP_TCP_DEFAULT_TTL_IX                 0 // not used by PXE client
#define OP_TCP_KEEP_ALIVE_INT_IX              0 // not used by PXE client
#define OP_KEEP_ALIVE_GARBAGE_IX              0 // not used by PXE client
#define OP_NIS_DOMAIN_NAME_IX                 0 // not used by PXE client
#define OP_NIS_SERVERS_IX                     0 // not used by PXE client
#define OP_NTP_SERVERS_IX                     0 // not used by PXE client
#define OP_VENDOR_SPECIFIC_IX                 3
#define OP_NBNS_SERVERS_IX                    0 // not used by PXE client
#define OP_NBDD_SERVERS_IX                    0 // not used by PXE client
#define OP_NETBIOS_NODE_TYPE_IX               0 // not used by PXE client
#define OP_NETBIOS_SCOPE_IX                   0 // not used by PXE client
#define OP_XWINDOW_SYSTEM_FONT_SERVERS_IX     0 // not used by PXE client
#define OP_XWINDOW_SYSTEM_DISPLAY_MANAGERS_IX 0 // not used by PXE client
// DHCP option indices
//
#define OP_DHCP_REQ_IP_ADD_IX         0                 // not used by PXE client
#define OP_DHCP_LEASE_TIME_IX         0                 // not used by PXE client
#define OP_DHCP_OPTION_OVERLOAD_IX    4
#define OP_DHCP_MESSAGE_TYPE_IX       5
#define OP_DHCP_SERVER_IP_IX          6
#define OP_DHCP_PARM_REQ_LIST_IX      0                 // not used by PXE client
#define OP_DHCP_ERROR_MESSAGE_IX      0                 // not used by PXE client
#define OP_DHCP_MAX_MESSAGE_SZ_IX     0                 // not used by PXE client
#define OP_DHCP_RENEWAL_TIME_IX       0                 // not used by PXE client
#define OP_DHCP_REBINDING_TIME_IX     0                 // not used by PXE client
#define OP_DHCP_CLASS_IDENTIFIER_IX   7
#define OP_DHCP_CLIENT_IDENTIFIER_IX  0                 // not used by PXE client
#define OP_RESERVED62_IX              0                 // not used by PXE client
#define OP_RESERVED63_IX              0                 // not used by PXE client
#define OP_NISPLUS_DOMAIN_NAME_IX     0                 // not used by PXE client
#define OP_NISPLUS_SERVERS_IX         0                 // not used by PXE client
#define OP_DHCP_TFTP_SERVER_NAME_IX   0                 // not used by PXE client
#define OP_DHCP_BOOTFILE_IX           8

#define MAX_OUR_OPT                   OP_DHCP_BOOTFILE  // largest option in which we are interested
#define MAX_OUR_IX                    OP_BOOT_FILE_SZ_IX

typedef struct {
  DHCPV4_OP_STRUCT  *PktOptAdds[MAX_OUR_IX];
  DHCPV4_OP_STRUCT  *PxeOptAdds[MAX_OUR_PXE_IX];
  UINT8             Status;
} OPTION_POINTERS;

typedef struct DhcpReceiveBufferStruct {
  union {
    UINT8         ReceiveBuffer[MAX_DHCP_MSG_SZ];
    DHCPV4_STRUCT Dhcpv4;
  } u;

  OPTION_POINTERS OpAdds;
} DHCP_RECEIVE_BUFFER;

#define PXE_TYPE          (1 << 0)
#define WfM11a_TYPE       (1 << 1)
#define DISCOVER_TYPE     (1 << 2)
#define CREDENTIALS_TYPE  (1 << 3)
#define USE_THREE_BYTE    (1 << 4)

#endif // _DHCP_H

/* EOF - dhcp.h */
