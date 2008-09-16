/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  pxe_bc_dhcp.c

Abstract:
  DHCP and PXE discovery protocol implementations.


**/

#include "Bc.h"

#include "PxeArch.h"

STATIC EFI_PXE_BASE_CODE_UDP_PORT DhcpServerPort        = DHCP_SERVER_PORT;
STATIC EFI_PXE_BASE_CODE_UDP_PORT DHCPClientPort        = DHCP_CLIENT_PORT;
STATIC EFI_PXE_BASE_CODE_UDP_PORT PseudoDhcpServerPort  = PXE_DISCOVERY_PORT;
#define PSEUDO_DHCP_CLIENT_PORT PseudoDhcpServerPort
STATIC EFI_IP_ADDRESS             BroadcastIP       = {{0xffffffff}};
STATIC EFI_IP_ADDRESS             DefaultSubnetMask = {{0xffffff00}};

typedef union {
  DHCPV4_OP_STRUCT          *OpPtr;
  PXE_OP_SERVER_LIST        *BootServersStr;
  PXE_SERVER_LIST           *BootServerList;
  PXE_BOOT_MENU_ENTRY       *BootMenuItem;
  PXE_OP_DISCOVERY_CONTROL  *DiscoveryControl;
  PXE_OP_BOOT_MENU          *BootMenu;
  PXE_OP_BOOT_ITEM          *BootItem;
  DHCPV4_OP_VENDOR_OPTIONS  *VendorOptions;
  DHCPV4_OP_OVERLOAD        *Overload;
  DHCPV4_OP_CLASS           *PxeClassStr;
  DHCPV4_OP_SUBNET_MASK     *SubnetMaskStr;
  DHCPV4_OP_MESSAGE_TYPE    *MessageType;
  UINT8                     *BytePtr;
} UNION_PTR;

#pragma pack(1)
//
// option structure for DHCPREQUEST at end of DISCOVER options
// and for DHCPDECLINE
//
STATIC const struct requestopendstr {
  DHCPV4_OP_REQUESTED_IP  OpReqIP;
  DHCPV4_OP_SERVER_IP     DhcServerIpPtr;
  UINT8                   End[1];
}
RequestOpEndStr = {
  {
    {
      OP_DHCP_REQ_IP_ADD,
      DHCPV4_OPTION_LENGTH(DHCPV4_OP_REQUESTED_IP)
    }
  },
  {
    {
      OP_DHCP_SERVER_IP,
      DHCPV4_OPTION_LENGTH(DHCPV4_OP_SERVER_IP)
    }
  },
  {
    OP_END
  }
};

#define DHCP_REQ_OPTIONS  (*(struct requestopendstr *) DHCPV4_OPTIONS_BUFFER.End)

PXE_OP_BOOT_ITEM                DefaultBootItem = {
  {
    VEND_PXE_BOOT_ITEM,
    DHCPV4_OPTION_LENGTH(PXE_OP_BOOT_ITEM)
  },
  0,
  0
};

//
// PXE discovery control default structure
//
STATIC PXE_OP_DISCOVERY_CONTROL DefaultDisCtl = {
  { VEND_PXE_DISCOVERY_CONTROL, DHCPV4_OPTION_LENGTH(PXE_OP_DISCOVERY_CONTROL) },
  0
};

//
// PXE credentials option structure
//
typedef struct {
  UINT8 c[4];
} PXE_CREDENTIAL;

typedef struct {
  DHCPV4_OP_HEADER  Header;
  PXE_CREDENTIAL    Credentials[1];
} PXE_OP_CREDENTIAL_TYPES;

//
// option structure for PXE discover (without credentials)
//
typedef struct {            // discoveropendstr {
  DHCPV4_OP_HEADER  Header; // vendor options
  PXE_OP_BOOT_ITEM  BootItem;
  UINT8             End[1]; // if credentials option, it starts here
} PXE_DISCOVER_OPTIONS;

#define DISCOVERoptions (*(PXE_DISCOVER_OPTIONS *) DHCPV4_OPTIONS_BUFFER.End)
#define DISCREDoptions  (*(PXE_OP_CREDENTIAL_TYPES *) DISCOVERoptions.End)

//
// common option beginning for all our DHCP messages except
// DHCPDECLINE and DHCPRELEASE
//
STATIC struct optionsstr {
  UINT8                       DhcpCookie[4];
  DHCPV4_OP_MESSAGE_TYPE      DhcpMessageType;
  DHCPV4_OP_MAX_MESSAGE_SIZE  DhcpMaxMessageSize;
  DHCPV4_OP_REQUESTED_OPTIONS DhcpRequestedOptions;
  DHCPV4_OP_PLATFORM_ID       DhcpPlatformId;
  DHCPV4_OP_NETWORK_INTERFACE DhcpNetworkInterface;
  DHCPV4_OP_ARCHITECTURE_TYPE DhcpClientArchitecture;
  DHCPV4_OP_CLASS_ID          DhcpClassIdentifier;
  UINT8                       End[1];
} DHCPOpStart;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
OptionsStrucInit (
  VOID
  )
{
  DHCPOpStart.DhcpCookie[0] = 99;
  DHCPOpStart.DhcpCookie[1] = 130;
  DHCPOpStart.DhcpCookie[2] = 83;
  DHCPOpStart.DhcpCookie[3] = 99;
  DHCPOpStart.DhcpMessageType.Header.OpCode = OP_DHCP_MESSAGE_TYPE;
  DHCPOpStart.DhcpMessageType.Header.Length = 1;
  DHCPOpStart.DhcpMessageType.Type = DHCPDISCOVER;
  DHCPOpStart.DhcpMaxMessageSize.Header.OpCode = OP_DHCP_MAX_MESSAGE_SZ;
  DHCPOpStart.DhcpMaxMessageSize.Header.Length = 2;
  DHCPOpStart.DhcpMaxMessageSize.MaxSize[0] = MAX_DHCP_MSG_SZ >> 8;
  DHCPOpStart.DhcpMaxMessageSize.MaxSize[1] = MAX_DHCP_MSG_SZ & 0xff;
  DHCPOpStart.DhcpRequestedOptions.Header.OpCode = OP_DHCP_PARM_REQ_LIST;
  DHCPOpStart.DhcpRequestedOptions.Header.Length = sizeof (DHCPV4_REQUESTED_OPTIONS_DATA);
  DHCPOpStart.DhcpRequestedOptions.Data._OP_SUBNET_MASK = OP_SUBNET_MASK;                     /* 1 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_TIME_OFFSET = OP_TIME_OFFSET;                     /* 2 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_ROUTER_LIST = OP_ROUTER_LIST;                     /* 3 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_TIME_SERVERS = OP_TIME_SERVERS;                   /* 4 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_NAME_SERVERS = OP_NAME_SERVERS;                   /* 5 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DNS_SERVERS = OP_DNS_SERVERS;                     /* 6 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_HOST_NAME = OP_HOST_NAME;                         /* 12 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_BOOT_FILE_SZ = OP_BOOT_FILE_SZ;                   /* 13 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DOMAIN_NAME = OP_DOMAIN_NAME;                     /* 15 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_ROOT_PATH = OP_ROOT_PATH;                         /* 17 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_EXTENSION_PATH = OP_EXTENSION_PATH;               /* 18 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_MAX_DATAGRAM_SZ = OP_MAX_DATAGRAM_SZ;             /* 22 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DEFAULT_TTL = OP_DEFAULT_TTL;                     /* 23 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_BROADCAST_ADD = OP_BROADCAST_ADD;                 /* 28 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_NIS_DOMAIN_NAME = OP_NIS_DOMAIN_NAME;             /* 40 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_NIS_SERVERS = OP_NIS_SERVERS;                     /* 41 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_NTP_SERVERS = OP_NTP_SERVERS;                     /* 42 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_VENDOR_SPECIFIC = OP_VENDOR_SPECIFIC;             /* 43 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_REQ_IP_ADD = OP_DHCP_REQ_IP_ADD;             /* 50 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_LEASE_TIME = OP_DHCP_LEASE_TIME;             /* 51 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_SERVER_IP = OP_DHCP_SERVER_IP;               /* 54 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_RENEWAL_TIME = OP_DHCP_RENEWAL_TIME;         /* 58 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_REBINDING_TIME = OP_DHCP_REBINDING_TIME;     /* 59 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_CLASS_IDENTIFIER = OP_DHCP_CLASS_IDENTIFIER; /* 60 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_TFTP_SERVER_NAME = OP_DHCP_TFTP_SERVER_NAME; /* 66 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_BOOTFILE = OP_DHCP_BOOTFILE;                 /* 67 */
  DHCPOpStart.DhcpRequestedOptions.Data._OP_DHCP_PLATFORM_ID = OP_DHCP_PLATFORM_ID;           /* 97 */
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption128 = 128;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption129 = 129;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption130 = 130;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption131 = 131;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption132 = 132;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption133 = 133, DHCPOpStart.DhcpRequestedOptions.Data.VendorOption134 = 134;
  DHCPOpStart.DhcpRequestedOptions.Data.VendorOption135 = 135;
  DHCPOpStart.DhcpPlatformId.Header.OpCode              = OP_DHCP_PLATFORM_ID;
  DHCPOpStart.DhcpPlatformId.Header.Length              = DHCPV4_OPTION_LENGTH (DHCPV4_OP_PLATFORM_ID);
  DHCPOpStart.DhcpNetworkInterface.Header.OpCode        = OP_DHCP_NETWORK_ARCH;
  DHCPOpStart.DhcpNetworkInterface.Header.Length        = DHCPV4_OPTION_LENGTH (DHCPV4_OP_NETWORK_INTERFACE);
  DHCPOpStart.DhcpNetworkInterface.Type                 = 0;
  DHCPOpStart.DhcpNetworkInterface.MajorVersion         = 0;
  DHCPOpStart.DhcpNetworkInterface.MinorVersion         = 0;
  DHCPOpStart.DhcpClientArchitecture.Header.OpCode      = OP_DHCP_SYSTEM_ARCH;
  DHCPOpStart.DhcpClientArchitecture.Header.Length      = DHCPV4_OPTION_LENGTH (DHCPV4_OP_ARCHITECTURE_TYPE);
  DHCPOpStart.DhcpClientArchitecture.Type               = HTONS (SYS_ARCH);
  DHCPOpStart.DhcpClassIdentifier.Header.OpCode         = OP_DHCP_CLASS_IDENTIFIER;
  DHCPOpStart.DhcpClassIdentifier.Header.Length         = sizeof (DHCPV4_CLASS_ID_DATA);
  CopyMem (
    DHCPOpStart.DhcpClassIdentifier.Data.ClassIdentifier,
    "PXEClient:",
    sizeof ("PXEClient:")
    );
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.Lit2, "Arch:", sizeof ("Arch:"));
  CopyMem (
    DHCPOpStart.DhcpClassIdentifier.Data.ArchitectureType,
    "xxxxx",
    sizeof ("xxxxx")
    );
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.Lit3, ":", sizeof (":"));
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.InterfaceName, "XXXX", sizeof ("XXXX"));
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.Lit4, ":", sizeof (":"));
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.UndiMajor, "yyy", sizeof ("yyy"));
  CopyMem (DHCPOpStart.DhcpClassIdentifier.Data.UndiMinor, "xxx", sizeof ("xxx"));
  DHCPOpStart.End[0] = OP_END;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// DHCPDECLINE option structure
//
struct opdeclinestr {
  UINT8                   DhcpCookie[4];
  DHCPV4_OP_MESSAGE_TYPE  DhcpMessageType;
  struct requestopendstr  OpDeclineEnd;
};

#define DHCPDECLINEoptions  (*(struct opdeclinestr *) DHCPV4_TRANSMIT_BUFFER.options)

//
// DHCPRELEASE option structure
//
struct opreleasestr {
  UINT8                   DhcpCookie[4];
  DHCPV4_OP_MESSAGE_TYPE  DhcpMessageType;
  DHCPV4_OP_SERVER_IP     DhcServerIpPtr;
  UINT8                   End[1];
};

#define DHCPRELEASEoptions  (*(struct opreleasestr *) DHCPV4_TRANSMIT_BUFFER.options)

//
// array of PXE vendor options in which we are interested
// value 0 -> not of interest, else value is index into PXE OPTION array
// option values from 1 to MAX_OUR_PXE_OPT
//
STATIC UINT8  ourPXEopts[MAX_OUR_PXE_OPT] = {
  VEND_PXE_MTFTP_IP_IX,             // multicast IP address of bootfile for MTFTP listen
  VEND_PXE_MTFTP_CPORT_IX,          // UDP Port to monitor for MTFTP responses - Intel order
  VEND_PXE_MTFTP_SPORT_IX,          // Server UDP Port for MTFTP open - Intel order
  VEND_PXE_MTFTP_TMOUT_IX,          // Listen timeout - secs
  VEND_PXE_MTFTP_DELAY_IX,          // Transmission timeout - secs
  VEND_PXE_DISCOVERY_CONTROL_IX,    // bit field
  VEND_PXE_DISCOVERY_MCAST_ADDR_IX, // boot server discovery multicast address
  VEND_PXE_BOOT_SERVERS_IX,         // list of boot servers of form tp(2) cnt(1) ips[cnt]
  VEND_PXE_BOOT_MENU_IX,
  VEND_PXE_BOOT_PROMPT_IX,
  VEND_PXE_MCAST_ADDRS_ALLOC_IX,    // not used by client
  VEND_PXE_CREDENTIAL_TYPES_IX,
  VEND_13_IX,                       // not used by client
  VEND_14_IX,                       // not used by client
  VEND_15_IX,                       // not used by client
  VEND_16_IX,                       // not used by client
  VEND_17_IX,                       // not used by client
  VEND_18_IX,                       // not used by client
  VEND_19_IX,                       // not used by client
  VEND_20_IX,                       // not used by client
  VEND_21_IX,                       // not used by client
  VEND_22_IX,                       // not used by client
  VEND_23_IX,                       // not used by client
  VEND_24_IX,                       // not used by client
  VEND_25_IX,                       // not used by client
  VEND_26_IX,                       // not used by client
  VEND_27_IX,                       // not used by client
  VEND_28_IX,                       // not used by client
  VEND_29_IX,                       // not used by client
  VEND_30_IX,                       // not used by client
  VEND_31_IX,                       // not used by client
  VEND_32_IX,                       // not used by client
  VEND_33_IX,                       // not used by client
  VEND_34_IX,                       // not used by client
  VEND_35_IX,                       // not used by client
  VEND_36_IX,                       // not used by client
  VEND_37_IX,                       // not used by client
  VEND_38_IX,                       // not used by client
  VEND_39_IX,                       // not used by client
  VEND_40_IX,                       // not used by client
  VEND_41_IX,                       // not used by client
  VEND_42_IX,                       // not used by client
  VEND_43_IX,                       // not used by client
  VEND_44_IX,                       // not used by client
  VEND_45_IX,                       // not used by client
  VEND_46_IX,                       // not used by client
  VEND_47_IX,                       // not used by client
  VEND_48_IX,                       // not used by client
  VEND_49_IX,                       // not used by client
  VEND_50_IX,                       // not used by client
  VEND_51_IX,                       // not used by client
  VEND_52_IX,                       // not used by client
  VEND_53_IX,                       // not used by client
  VEND_54_IX,                       // not used by client
  VEND_55_IX,                       // not used by client
  VEND_56_IX,                       // not used by client
  VEND_57_IX,                       // not used by client
  VEND_58_IX,                       // not used by client
  VEND_59_IX,                       // not used by client
  VEND_60_IX,                       // not used by client
  VEND_61_IX,                       // not used by client
  VEND_62_IX,                       // not used by client
  VEND_63_IX,                       // not used by client
  VEND_64_IX,                       // not used by client
  VEND_65_IX,                       // not used by client
  VEND_66_IX,                       // not used by client
  VEND_67_IX,                       // not used by client
  VEND_68_IX,                       // not used by client
  VEND_69_IX,                       // not used by client
  VEND_70_IX,                       // not used by client
  VEND_PXE_BOOT_ITEM_IX
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// array of options in which we are interested
// value 0 -> not of interest, else value is index into OPTION array
// option values from 1 to MAX_OUR_OPT
//
STATIC UINT8  OurDhcpOptions[MAX_OUR_OPT] = {
  OP_SUBNET_MASK_IX,                      // OP_SUBNET_MASK   1   // data is the subnet mask
  OP_TIME_OFFSET_IX,                      // OP_TIME_OFFSET   2   // data is the time offset of subnet to UTC in seconds
  OP_ROUTER_LIST_IX,                      // OP_ROUTER_LIST   3   // list of routers on subnet
  OP_TIME_SERVERS_IX,                     // OP_TIME_SERVERS  4   // list of time servers available
  OP_NAME_SERVERS_IX,                     // OP_NAME_SERVERS  5   // list of name servers available
  OP_DNS_SERVERS_IX,                      // OP_DNS_SERVERS   6   // list of DNS servers available
  OP_LOG_SERVERS_IX,                      // OP_LOG_SERVERS   7
  OP_COOKIE_SERVERS_IX,                   // OP_COOKIE_SERVERS    8
  OP_LPR_SREVERS_IX,                      // OP_LPR_SREVERS   9
  OP_IMPRESS_SERVERS_IX,                  // OP_IMPRESS_SERVERS   10
  OP_RES_LOC_SERVERS_IX,                  // OP_RES_LOC_SERVERS   11
  OP_HOST_NAME_IX,                        // OP_HOST_NAME 12  // client name
  OP_BOOT_FILE_SZ_IX,                     // OP_BOOT_FILE_SZ  13  // number of 512 blocks of boot file
  OP_DUMP_FILE_IX,                        // OP_DUMP_FILE 14  // path name of dump file if client crashes
  OP_DOMAIN_NAME_IX,                      // OP_DOMAIN_NAME   15  // domain name to use
  OP_SWAP_SERVER_IX,                      // OP_SWAP_SERVER   16
  OP_ROOT_PATH_IX,                        // OP_ROOT_PATH 17  // path name containing root disk
  OP_EXTENSION_PATH_IX,                   // OP_EXTENSION_PATH    18  // name of TFTP downloadable file of form of OP
  OP_IP_FORWARDING_IX,                    // OP_IP_FORWARDING 19  // enable/disable IP packet forwarding
  OP_NON_LOCAL_SRC_RTE_IX,                // OP_NON_LOCAL_SRC_RTE 20  // enable/disable non local source routing
  OP_POLICY_FILTER_IX,                    // OP_POLICY_FILTER 21  // policy filters for non local source routing
  OP_MAX_DATAGRAM_SZ_IX,                  // OP_MAX_DATAGRAM_SZ   22  // maximum datagram reassembly size
  OP_DEFAULT_TTL_IX,                      // OP_DEFAULT_TTL   23  // default IP time to live
  OP_MTU_AGING_TIMEOUT_IX,                // OP_MTU_AGING_TIMEOUT 24
  OP_MTU_SIZES_IX,                        // OP_MTU_SIZES 25
  OP_MTU_TO_USE_IX,                       // OP_MTU_TO_USE    26
  OP_ALL_SUBNETS_LOCAL_IX,                // OP_ALL_SUBNETS_LOCAL 27
  OP_BROADCAST_ADD_IX,                    // OP_BROADCAST_ADD 28  // broadcast address used on subnet
  OP_PERFORM_MASK_DISCOVERY_IX,           // OP_PERFORM_MASK_DISCOVERY    29  // perform mask discovery using ICMP
  OP_RESPOND_TO_MASK_REQ_IX,              // OP_RESPOND_TO_MASK_REQ   30  // respond to subnet mask requests using ICMP
  OP_PERFORM_ROUTER_DISCOVERY_IX,         // OP_PERFORM_ROUTER_DISCOVERY  31
  OP_ROUTER_SOLICIT_ADDRESS_IX,           // OP_ROUTER_SOLICIT_ADDRESS    32
  OP_STATIC_ROUTER_LIST_IX,               // OP_STATIC_ROUTER_LIST    33  // list of dest/route pairs
  OP_USE_ARP_TRAILERS_IX,                 // OP_USE_ARP_TRAILERS      34
  OP_ARP_CACHE_TIMEOUT_IX,                // OP_ARP_CACHE_TIMEOUT 35
  OP_ETHERNET_ENCAPSULATION_IX,           // OP_ETHERNET_ENCAPSULATION    36  // 0 -> RFC 894, 1 -> IEEE 802.3 (RFC 1042)
  OP_TCP_DEFAULT_TTL_IX,                  // OP_TCP_DEFAULT_TTL   37  // default time to live when sending TCP segments
  OP_TCP_KEEP_ALIVE_INT_IX,               // OP_TCP_KEEP_ALIVE_INT    38  // keep alive interval in seconds
  OP_KEEP_ALIVE_GARBAGE_IX,               // OP_KEEP_ALIVE_GARBAGE    39
  OP_NIS_DOMAIN_NAME_IX,                  // OP_NIS_DOMAIN_NAME   40
  OP_NIS_SERVERS_IX,                      // OP_NIS_SERVERS   41
  OP_NTP_SERVERS_IX,                      // OP_NTP_SERVERS   42
  OP_VENDOR_SPECIFIC_IX,                  // OP_VENDOR_SPECIFIC   43
  OP_NBNS_SERVERS_IX,                     // OP_NBNS_SERVERS  44
  OP_NBDD_SERVERS_IX,                     // OP_NBDD_SERVERS  45
  OP_NETBIOS_NODE_TYPE_IX,                // OP_NETBIOS_NODE_TYPE 46
  OP_NETBIOS_SCOPE_IX,                    // OP_NETBIOS_SCOPE 47
  OP_XWINDOW_SYSTEM_FONT_SERVERS_IX,      // OP_XWINDOW_SYSTEM_FONT_SERVERS   48
  OP_XWINDOW_SYSTEM_DISPLAY_MANAGERS_IX,  // OP_XWINDOW_SYSTEM_DISPLAY_MANAGERS   49
  OP_DHCP_REQ_IP_ADD_IX,                  // OP_DHCP_REQ_IP_ADD   50  // requested IP address - in DHCPDISCOVER
  OP_DHCP_LEASE_TIME_IX,                  // OP_DHCP_LEASE_TIME   51  // lease time requested/granted
  OP_DHCP_OPTION_OVERLOAD_IX,             // OP_DHCP_OPTION_OVERLOAD  52  // file/server name/both used to hold options
  OP_DHCP_MESSAGE_TYPE_IX,                // OP_DHCP_MESSAGE_TYPE 53  // message type
  OP_DHCP_SERVER_IP_IX,                   // OP_DHCP_SERVER_IP    54      // IP of server
  OP_DHCP_PARM_REQ_LIST_IX,               // OP_DHCP_PARM_REQ_LIST    55  // list of requested parameters
  OP_DHCP_ERROR_MESSAGE_IX,               // OP_DHCP_ERROR_MESSAGE    56  // in DHCPNAK or DECLINE messages
  OP_DHCP_MAX_MESSAGE_SZ_IX,              // OP_DHCP_MAX_MESSAGE_SZ   57  // maximum DHCP message size client will accept
  OP_DHCP_RENEWAL_TIME_IX,                // OP_DHCP_RENEWAL_TIME 58  // time in seconds before transitioning to RENEWING state
  OP_DHCP_REBINDING_TIME_IX,              // OP_DHCP_REBINDING_TIME   59  // time in seconds before transitioning to REBINDING state
  OP_DHCP_CLASS_IDENTIFIER_IX,            // OP_DHCP_CLASS_IDENTIFIER 60
  OP_DHCP_CLIENT_IDENTIFIER_IX,           // OP_DHCP_CLIENT_IDENTIFIER    61
  OP_RESERVED62_IX,                       // OP_RESERVED62
  OP_RESERVED63_IX,                       // OP_RESERVED63
  OP_NISPLUS_DOMAIN_NAME_IX,              // OP_NISPLUS_DOMAIN_NAME   64
  OP_NISPLUS_SERVERS_IX,                  // OP_NISPLUS_SERVERS   65
  OP_DHCP_TFTP_SERVER_NAME_IX,            // OP_DHCP_TFTP_SERVER_NAME 66
  OP_DHCP_BOOTFILE_IX                     // OP_DHCP_BOOTFILE 67
};

#define RxBuf ((DHCP_RECEIVE_BUFFER *) (Private->ReceiveBuffers))

#pragma pack()

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @param  Smbios              Pointer to SMBIOS structure
  @param  StringNumber        String number to return. 0 is used to skip all
                              strings and  point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == 0

**/
CHAR8 *
PxeBcLibGetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER  *Smbios,
  IN  UINT16                    StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;

  //
  // Skip over formatted section
  //
  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber || StringNumber == 0; Index++) {
    if (StringNumber == Index) {
      return String;
    }
    //
    // Skip string
    //
    for (; *String != 0; String++)
      ;
    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Return pointer to next structure in Smbios.
      //  if you pass in a 0 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}


/**
  This function gets system guid and serial number from the smbios table

  @param  SystemGuid          The pointer of returned system guid
  @param  SystemSerialNumber  The pointer of returned system serial number

  @retval EFI_SUCCESS         Successfully get the system guid and system serial
                              number
  @retval EFI_NOT_FOUND       Not find the SMBIOS table

**/
EFI_STATUS
PxeBcLibGetSmbiosSystemGuidAndSerialNumber (
  IN  EFI_GUID  *SystemGuid,
  OUT CHAR8     **SystemSerialNumber
  )
{
  EFI_STATUS                Status;
  SMBIOS_TABLE_ENTRY_POINT  *SmbiosTable;
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT16                    Index;

  Status = EfiGetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID **) &SmbiosTable);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Smbios.Hdr    = (SMBIOS_STRUCTURE *) (UINTN) SmbiosTable->TableAddress;
  SmbiosEnd.Raw = (UINT8 *) (UINTN) (SmbiosTable->TableAddress + SmbiosTable->TableLength);

  for (Index = 0; Index < SmbiosTable->TableLength; Index++) {
    if (Smbios.Hdr->Type == 1) {
      if (Smbios.Hdr->Length < 0x19) {
        //
        // Older version did not support Guid and Serial number
        //
        continue;
      }
      //
      // SMBIOS tables are byte packed so we need to do a byte copy to
      // prevend alignment faults on Itanium-based platform.
      //
      CopyMem (SystemGuid, &Smbios.Type1->Uuid, sizeof (EFI_GUID));
      *SystemSerialNumber = PxeBcLibGetSmbiosString (&Smbios, Smbios.Type1->SerialNumber);

      return EFI_SUCCESS;
    }
    //
    // Make Smbios point to the next record
    //
    PxeBcLibGetSmbiosString (&Smbios, 0);

    if (Smbios.Raw >= SmbiosEnd.Raw) {
      //
      // SMBIOS 2.1 incorrectly stated the length of SmbiosTable as 0x1e.
      // given this we must double check against the lenght of
      // the structure.
      //
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// add router list to list
//
STATIC
VOID
Ip4AddRouterList (
  PXE_BASECODE_DEVICE *Private,
  DHCPV4_OP_IP_LIST   *IpListPtr
  )
{
  EFI_IP_ADDRESS  TmpIp;
  INTN            Index;
  INTN            num;

  if (IpListPtr == NULL) {
    return ;
  }

  for (Index = 0, num = IpListPtr->Header.Length >> 2; Index < num; ++Index) {
    CopyMem (&TmpIp, &IpListPtr->IpList[Index], 4);
    Ip4AddRouter (Private, &TmpIp);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// send ARP for our IP - fail if someone has it
//
STATIC
BOOLEAN
SetStationIP (
  PXE_BASECODE_DEVICE *Private
  )
{
  EFI_MAC_ADDRESS DestMac;
  EFI_STATUS      EfiStatus;

  ZeroMem (&DestMac, sizeof DestMac);

  if (GetHwAddr(Private, (EFI_IP_ADDRESS *)&DHCP_REQ_OPTIONS.OpReqIP.Ip, (EFI_MAC_ADDRESS *)&DestMac)
    || DoArp(Private, (EFI_IP_ADDRESS *)&DHCP_REQ_OPTIONS.OpReqIP.Ip, (EFI_MAC_ADDRESS *)&DestMac) == EFI_SUCCESS) {
    return FALSE;   // somebody else has this IP
  }

  CopyMem (
    (EFI_IPv4_ADDRESS *) &Private->EfiBc.Mode->StationIp,
    &DHCP_REQ_OPTIONS.OpReqIP.Ip,
    sizeof (EFI_IPv4_ADDRESS)
    );

  Private->GoodStationIp = TRUE;

  if (!Private->UseIgmpv1Reporting) {
    return TRUE;
  }

  if (Private->Igmpv1TimeoutEvent != NULL) {
    return TRUE;
  }

  EfiStatus = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &Private->Igmpv1TimeoutEvent
                    );

  if (EFI_ERROR (EfiStatus)) {
    Private->Igmpv1TimeoutEvent = NULL;
    return TRUE;
  }

  EfiStatus = gBS->SetTimer (
                    Private->Igmpv1TimeoutEvent,
                    TimerRelative,
                    (UINT64) V1ROUTER_PRESENT_TIMEOUT * 10000000
                    );  /* 400 seconds */

  if (EFI_ERROR (EfiStatus)) {
    gBS->CloseEvent (Private->Igmpv1TimeoutEvent);
    Private->Igmpv1TimeoutEvent = NULL;
  }

  return TRUE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
AddRouters (
  PXE_BASECODE_DEVICE *Private,
  DHCP_RECEIVE_BUFFER *RxBufPtr
  )
{
  Ip4AddRouterList (
    Private,
    (DHCPV4_OP_IP_LIST *) RxBufPtr->OpAdds.PktOptAdds[OP_ROUTER_LIST_IX - 1]
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
EFI_STATUS
DoUdpWrite (
  PXE_BASECODE_DEVICE         *Private,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_IP_ADDRESS              *ClientIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ClientPortPtr
  )
{
  UINTN Len;

  Len = sizeof DHCPV4_TRANSMIT_BUFFER;

  return UdpWrite (
          Private,
          EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
          ServerIpPtr,
          ServerPortPtr,
          0,
          ClientIpPtr,
          ClientPortPtr,
          0,
          0,
          &Len,
          Private->TransmitBuffer
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// initialize the DHCP structure
//
typedef struct {
  UINT8 x[4];
} C4Str;

STATIC
VOID
InitDhcpv4TxBuf (
  PXE_BASECODE_DEVICE *Private
  )
{
  UINTN                   HwAddrLen;
  UINT8                   *String;
  CHAR8                   *SystemSerialNumber;
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;

  PxebcMode = Private->EfiBc.Mode;

  ZeroMem (&DHCPV4_TRANSMIT_BUFFER, sizeof (DHCPV4_STRUCT));
  DHCPV4_TRANSMIT_BUFFER.op     = BOOTP_REQUEST;
  DHCPV4_TRANSMIT_BUFFER.htype  = Private->SimpleNetwork->Mode->IfType;
  DHCPV4_TRANSMIT_BUFFER.flags  = HTONS (DHCP_BROADCAST_FLAG);
  CopyMem (&DHCPV4_OPTIONS_BUFFER, (VOID *) &DHCPOpStart, sizeof (DHCPOpStart));

  //
  // default to hardware address
  //
  HwAddrLen = Private->SimpleNetwork->Mode->HwAddressSize;

  if (HwAddrLen > sizeof DHCPV4_TRANSMIT_BUFFER.chaddr) {
    HwAddrLen = sizeof DHCPV4_TRANSMIT_BUFFER.chaddr;
  }

  String = (UINT8 *) &Private->SimpleNetwork->Mode->CurrentAddress;

  if (PxeBcLibGetSmbiosSystemGuidAndSerialNumber (
        (EFI_GUID *) DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid,
        &SystemSerialNumber
        ) == EFI_SUCCESS) {
    if (PxebcMode->SendGUID) {
      HwAddrLen = sizeof (EFI_GUID);
      String    = (UINT8 *) DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid;
    }
  } else {
    //
    // GUID not yet set - send all 0xff's to show programable (via SetVariable)
    // SetMem(DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid, sizeof(EFI_GUID), 0xff);
    // GUID not yet set - send all 0's to show not programable
    //
    ZeroMem (DHCPV4_OPTIONS_BUFFER.DhcpPlatformId.Guid, sizeof (EFI_GUID));
  }

  DHCPV4_TRANSMIT_BUFFER.hlen = (UINT8) HwAddrLen;
  CopyMem (DHCPV4_TRANSMIT_BUFFER.chaddr, String, HwAddrLen);

  CvtNum (
    SYS_ARCH,
    (UINT8 *) DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.ArchitectureType,
    sizeof DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.ArchitectureType
    );

  DHCPV4_OPTIONS_BUFFER.DhcpNetworkInterface.Type                         = Private->NiiPtr->Type;
  DHCPV4_OPTIONS_BUFFER.DhcpNetworkInterface.MajorVersion                 = Private->NiiPtr->MajorVer;
  DHCPV4_OPTIONS_BUFFER.DhcpNetworkInterface.MinorVersion                 = Private->NiiPtr->MinorVer;

  *(C4Str *) DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.InterfaceName = *(C4Str *) Private->NiiPtr->StringId;

  CvtNum (
    DHCPV4_OPTIONS_BUFFER.DhcpNetworkInterface.MajorVersion,
    (UINT8 *) DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.UndiMajor,
    sizeof DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.UndiMajor
    );

  CvtNum (
    DHCPV4_OPTIONS_BUFFER.DhcpNetworkInterface.MinorVersion,
    (UINT8 *) DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.UndiMinor,
    sizeof DHCPV4_OPTIONS_BUFFER.DhcpClassIdentifier.Data.UndiMinor
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
UINT32
DecodePxeOptions (
  DHCP_RECEIVE_BUFFER *RxBufPtr,
  UINT8               *ptr,
  INTN                Len
  )
{
  UINT8     Op;
  UINT8     *EndPtr;
  INTN      Index;
  UNION_PTR LocalPtr;
  UINT32    status;

  status = 0;

  for (EndPtr = ptr + Len; ptr < EndPtr; ptr += Len + 2) {
    Op  = ptr[0];
    Len = ptr[1];

    switch (Op) {
    case OP_PAD:
      Len = -1;
      break;

    case OP_END:
      return status;

    default:
      LocalPtr.BytePtr = ptr;
      if (Op <= MAX_OUR_PXE_OPT) {
        Index = ourPXEopts[Op - 1];
        if (Index) {
          RxBufPtr->OpAdds.PxeOptAdds[Index - 1] = LocalPtr.OpPtr;
          status |= 1 << Index;
          if (Index == VEND_PXE_BOOT_ITEM && LocalPtr.BootItem->Header.Length == 3) {
            RxBufPtr->OpAdds.Status |= USE_THREE_BYTE;
          }
        }
      }
      break;
    }
  }

  return status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
DecodeOptions (
  DHCP_RECEIVE_BUFFER *RxBufPtr,
  UINT8               *ptr,
  INTN                Len
  )
{
  UINT8     Op;
  UINT8     *EndPtr;
  INTN      Index;
  UNION_PTR LocalPtr;

  for (EndPtr = ptr + Len; ptr < EndPtr; ptr += Len + 2) {
    Op  = ptr[0];
    Len = ptr[1];

    switch (Op) {
    case OP_PAD:
      Len = -1;
      break;

    case OP_END:
      return ;

    default:
      LocalPtr.BytePtr = ptr;
      if (Op <= MAX_OUR_OPT) {
        Index = OurDhcpOptions[Op - 1];
        if (Index) {
          RxBufPtr->OpAdds.PktOptAdds[Index - 1] = LocalPtr.OpPtr;
          if (Index == OP_VENDOR_SPECIFIC_IX) {
            UINT32  status;
            status = DecodePxeOptions (
                      RxBufPtr,
                      (UINT8 *) LocalPtr.VendorOptions->VendorOptions,
                      LocalPtr.VendorOptions->Header.Length
                      );
            if (status) {
              RxBufPtr->OpAdds.Status |= PXE_TYPE;
              //
              // check for all the MTFTP info options present - any missing is a nogo
              //
              if ((status & WfM11a_OPTS) == WfM11a_OPTS) {
                RxBufPtr->OpAdds.Status |= WfM11a_TYPE;
              }

              if (status & DISCOVER_OPTS) {
                RxBufPtr->OpAdds.Status |= DISCOVER_TYPE;
              }

              if (status & CREDENTIALS_OPT) {
                RxBufPtr->OpAdds.Status |= CREDENTIALS_TYPE;
              }
            }
          }
        }
      }
      break;
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
Parse (
  DHCP_RECEIVE_BUFFER *RxBufPtr,
  INTN                Len
  )
{
  UNION_PTR LocalPtr;

  //
  // initialize
  //
  SetMem (&RxBufPtr->OpAdds, sizeof RxBufPtr->OpAdds, 0);

  DecodeOptions (
    RxBufPtr,
    RxBufPtr->u.Dhcpv4.options + 4,
    Len - (sizeof RxBufPtr->u.Dhcpv4 - sizeof RxBufPtr->u.Dhcpv4.options + 4)
    );

  LocalPtr.OpPtr = RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_OPTION_OVERLOAD_IX - 1];

  if ((LocalPtr.OpPtr) && (LocalPtr.Overload->Overload & OVLD_SRVR_NAME)) {
    DecodeOptions (RxBufPtr, RxBufPtr->u.Dhcpv4.sname, sizeof RxBufPtr->u.Dhcpv4.sname);
  }

  if (LocalPtr.OpPtr && (LocalPtr.Overload->Overload & OVLD_FILE)) {
    DecodeOptions (RxBufPtr, RxBufPtr->u.Dhcpv4.file, sizeof RxBufPtr->u.Dhcpv4.file);
  } else if (!RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1] && RxBufPtr->u.Dhcpv4.file[0]) {
    RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1] = (DHCPV4_OP_STRUCT *) (RxBufPtr->u.Dhcpv4.file - sizeof (DHCPV4_OP_HEADER));

    RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]->Header.Length = (UINT8) AsciiStrLen ((CHAR8 *) RxBufPtr->u.Dhcpv4.file);
  }

  LocalPtr.OpPtr = RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_CLASS_IDENTIFIER_IX - 1];

  if ((LocalPtr.OpPtr) &&
      LocalPtr.PxeClassStr->Header.Length >= 9 &&
      !CompareMem (LocalPtr.PxeClassStr->Class, "PXEClient", 9)
        ) {
    RxBufPtr->OpAdds.Status |= PXE_TYPE;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
CopyParseRxBuf (
  PXE_BASECODE_DEVICE *Private,
  INTN                RxBufIndex,
  INTN                PacketIndex
  )
{
  DHCP_RECEIVE_BUFFER *RxBufPtr;

  RxBufPtr = &((DHCP_RECEIVE_BUFFER *) Private->DhcpPacketBuffer)[PacketIndex];

  CopyMem (
    &RxBufPtr->u.Dhcpv4,
    &RxBuf[RxBufIndex].u.Dhcpv4,
    sizeof (RxBuf[RxBufIndex].u.Dhcpv4)
    );

  Parse (RxBufPtr, sizeof RxBufPtr->u.ReceiveBuffer);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
CopyProxyRxBuf (
  PXE_BASECODE_DEVICE *Private,
  INTN                RxBufIndex
  )
{
  Private->EfiBc.Mode->ProxyOfferReceived = TRUE;
  CopyParseRxBuf (Private, RxBufIndex, PXE_OFFER_INDEX);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
VOID
CopyParse (
  PXE_BASECODE_DEVICE       *Private,
  EFI_PXE_BASE_CODE_PACKET  *PacketPtr,
  EFI_PXE_BASE_CODE_PACKET  *NewPacketPtr,
  INTN                      Index
  )
{
  DHCP_RECEIVE_BUFFER *DhcpRxBuf;

  DhcpRxBuf = &((DHCP_RECEIVE_BUFFER *) Private->DhcpPacketBuffer)[Index];

  CopyMem (
    (EFI_PXE_BASE_CODE_PACKET *) &DhcpRxBuf->u.Dhcpv4,
    NewPacketPtr,
    sizeof (*NewPacketPtr)
    );

  CopyMem (&*PacketPtr, &*NewPacketPtr, sizeof (*NewPacketPtr));

  Parse (DhcpRxBuf, sizeof DhcpRxBuf->u.ReceiveBuffer);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BOOLEAN
AckEdit (
  DHCP_RECEIVE_BUFFER *DhcpRxBuf
  )
{
  UNION_PTR LocalPtr;

  LocalPtr.OpPtr = DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_MESSAGE_TYPE_IX - 1];

  //
  // check that an ACK
  // if a DHCP type, must be DHCPOFFER and must have server id
  //
  return (BOOLEAN)
    (
      (LocalPtr.OpPtr) &&
      (LocalPtr.MessageType->Type == DHCPACK) &&
      DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1]
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// if a discover type packet, make sure all required fields are present
//
BOOLEAN
DHCPOfferAckEdit (
  DHCP_RECEIVE_BUFFER *DhcpRxBuf
  )
{
  PXE_OP_SERVER_LIST  *BootServerOpPtr;
  UNION_PTR           LocalPtr;

  if ((DhcpRxBuf->OpAdds.Status & DISCOVER_TYPE) == 0) {
    return TRUE;
  }

  LocalPtr.OpPtr = DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_CONTROL_IX - 1];

  if (LocalPtr.OpPtr == NULL) {
    LocalPtr.OpPtr  = (DHCPV4_OP_STRUCT *) &DefaultDisCtl;
    DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_CONTROL_IX - 1] = (DHCPV4_OP_STRUCT *) &DefaultDisCtl;
  }
  //
  // make sure all required fields are here
  // if mucticast enabled, need multicast address
  //
  if (!(LocalPtr.DiscoveryControl->ControlBits & DISABLE_MCAST) &&
      (!DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_MCAST_ADDR_IX - 1] || !IS_MULTICAST (((DHCPV4_OP_STRUCT *) DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_MCAST_ADDR_IX - 1])->Data))
      ) {
    return FALSE;
    //
    // missing required field
    //
  }
  //
  // if a list, it better be good
  //
  BootServerOpPtr = (PXE_OP_SERVER_LIST *) DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_SERVERS_IX - 1];

  if (BootServerOpPtr != NULL) {
    PXE_SERVER_LIST *BootServerListPtr;
    INTN            ServerListLen;
    INTN            ServerEntryLen;

    BootServerListPtr = BootServerOpPtr->ServerList;
    ServerListLen     = BootServerOpPtr->Header.Length;

    do {
      EFI_IPv4_ADDRESS  *IpListPtr;
      INTN              IpCnt;

      IpCnt           = BootServerListPtr->u.Ipv4List.IpCount;

      ServerEntryLen  = sizeof (PXEV4_SERVER_LIST) + 2 + (IpCnt - 1) * sizeof (EFI_IPv4_ADDRESS);

      if (ServerListLen < ServerEntryLen) {
        //
        // missing required field
        //
        return FALSE;
      }

      IpListPtr = BootServerListPtr->u.Ipv4List.IpList;

      while (IpCnt--) {
        if (IS_MULTICAST (IpListPtr)) {
          //
          // missing required field
          //
          return FALSE;
        } else {
          ++IpListPtr;
        }
      }

      BootServerListPtr = (PXE_SERVER_LIST *) IpListPtr;
    } while (ServerListLen -= ServerEntryLen);
  }
  //
  // else there must be a list if use list enabled or multicast and
  // broadcast disabled
  //
  else if ((LocalPtr.DiscoveryControl->ControlBits & USE_ACCEPT_LIST) ||
           ((LocalPtr.DiscoveryControl->ControlBits & (DISABLE_MCAST | DISABLE_BCAST)) == (DISABLE_MCAST | DISABLE_BCAST))
          ) {
    //
    // missing required field
    //
    return FALSE;
  }
  //
  // if not USE_BOOTFILE or no bootfile given, must have menu stuff
  //
  if (!(LocalPtr.DiscoveryControl->ControlBits & USE_BOOTFILE) ||
      !DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]
      ) {
    INTN  MenuLth;

    LocalPtr.OpPtr = DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_MENU_IX - 1];

    if (LocalPtr.OpPtr == NULL || !DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_PROMPT_IX - 1]) {
      //
      // missing required field
      //
      return FALSE;
    }
    //
    // make sure menu valid
    //
    MenuLth               = LocalPtr.BootMenu->Header.Length;
    LocalPtr.BootMenuItem = LocalPtr.BootMenu->MenuItem;

    do {
      INTN  MenuItemLen;

      MenuItemLen = LocalPtr.BootMenuItem->DataLen;

      if (MenuItemLen == 0) {
        //
        // missing required field
        //
        return FALSE;
      }

      MenuItemLen += sizeof (*LocalPtr.BootMenuItem) - sizeof (LocalPtr.BootMenuItem->Data);

      MenuLth -= MenuItemLen;
      LocalPtr.BytePtr += MenuItemLen;
    } while (MenuLth > 0);

    if (MenuLth != 0) {
      //
      // missing required field
      //
      return FALSE;
    }
  }

  if (!DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1]) {
    DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1] = (DHCPV4_OP_STRUCT *) &DefaultBootItem;
  }

  return TRUE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BOOLEAN
DHCPAckEdit (
  DHCP_RECEIVE_BUFFER *RxBufPtr
  )
{
  return (BOOLEAN) (DHCPOfferAckEdit (RxBufPtr) ? AckEdit (RxBufPtr) : FALSE);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// get an offer/ack
//
EFI_STATUS
GetOfferAck (
  PXE_BASECODE_DEVICE          *Private,
  BOOLEAN                     (*ExtraEdit)(DHCP_RECEIVE_BUFFER *DhcpRxBuf),
  UINT16 OpFlags, // for Udp read
  EFI_IP_ADDRESS *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT *ServerPortPtr,
  EFI_IP_ADDRESS *ClientIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT *ClientPortPtr,
  DHCP_RECEIVE_BUFFER *DhcpRxBuf,
  EFI_EVENT TimeoutEvent
  )
/*++
Routine description:
  Wait for an OFFER/ACK packet.

Parameters:
  Private := Pointer to PxeBc interface
  ExtraEdit := Pointer to extra option checking function
  OpFlags := UdpRead() option flags
  ServerIpPtr :=
  ServerPortPtr :=
  ClientIpPtr :=
  ClientPortPtr :=
  DhcpRxBuf :=
  TimeoutEvent :=

Returns:
--*/
{
  EFI_IP_ADDRESS  ServerIp;
  EFI_STATUS      StatCode;
  INTN            RxBufLen;

  for (;;) {
    //
    // Wait until we get a UDP packet.
    //
    ZeroMem (&ServerIp, sizeof (EFI_IP_ADDRESS));
    RxBufLen = sizeof RxBuf[0].u.ReceiveBuffer;

    if ((StatCode = UdpRead (
                      Private,
                      OpFlags,
                      ClientIpPtr,
                      ClientPortPtr,
                      ServerIpPtr,
                      ServerPortPtr,
                      0,
                      0,
                      (UINTN *) &RxBufLen,
                      &DhcpRxBuf->u.Dhcpv4,
                      TimeoutEvent
                      )) != EFI_SUCCESS) {
      if (StatCode == EFI_TIMEOUT) {
        StatCode = EFI_NO_RESPONSE;
      }

      break;
    }
    //
    // got a packet - see if a good offer
    //
    if (DhcpRxBuf->u.Dhcpv4.op != BOOTP_REPLY) {
      continue;
    }

    if (DhcpRxBuf->u.Dhcpv4.xid != DHCPV4_TRANSMIT_BUFFER.xid) {
      continue;
    }

    if (*(UINT32 *) DHCPV4_TRANSMIT_BUFFER.options != * (UINT32 *) DhcpRxBuf->u.Dhcpv4.options) {
      continue;
    }

    if (*(UINT8 *) &DhcpRxBuf->u.Dhcpv4.yiaddr > 223) {
      continue;
    }

    if (CompareMem (
          DhcpRxBuf->u.Dhcpv4.chaddr,
          DHCPV4_TRANSMIT_BUFFER.chaddr,
          sizeof DhcpRxBuf->u.Dhcpv4.chaddr
          )) {
      //
      // no good
      //
      continue;
    }

    Parse (DhcpRxBuf, RxBufLen);

    if (!(*ExtraEdit) (DhcpRxBuf)) {
      continue;
    }
    //
    // Good DHCP packet.
    //
    StatCode = EFI_SUCCESS;
    break;
  }

  return StatCode;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// get DHCPOFFER's
//
EFI_STATUS
GetOffers (
  PXE_BASECODE_DEVICE *Private
  )
{
  EFI_IP_ADDRESS  ClientIp;
  EFI_IP_ADDRESS  ServerIp;
  EFI_STATUS      StatCode;
  EFI_EVENT       TimeoutEvent;
  INTN            NumOffers;
  INTN            Index;

  //
  //
  //
  ZeroMem (&ServerIp, sizeof (EFI_IP_ADDRESS));
  NumOffers = 0;

  for (Index = 0; Index < (sizeof Private->ServerCount) / sizeof Private->ServerCount[0]; ++Index) {
    Private->ServerCount[Index] = 0;
    Private->GotProxy[Index]    = 0;
  }

  Private->GotBootp               = 0;
  //
  // these we throw away
  //
  Private->GotProxy[DHCP_ONLY_IX] = 1;
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    return StatCode;
  }

  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerRelative,
                    Private->Timeout * 10000000 + 1000000
                    );

  if (EFI_ERROR (StatCode)) {
    gBS->CloseEvent (TimeoutEvent);
    return StatCode;
  }
  //
  // get offers
  //
  for (;;) {
    DHCP_RECEIVE_BUFFER *DhcpRxBuf;
    UNION_PTR           LocalPtr;

    DhcpRxBuf = &RxBuf[NumOffers];

    if ((
          StatCode = GetOfferAck (
                  Private,
        DHCPOfferAckEdit,
        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP |
        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP |
        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
        &ServerIp,
        &DhcpServerPort,
        &ClientIp,
        &DHCPClientPort,
        DhcpRxBuf,
        TimeoutEvent
        )
) != EFI_SUCCESS
        ) {
      break;
    }

    LocalPtr.OpPtr = DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_MESSAGE_TYPE_IX - 1];

    //
    // check type of offer
    //
    if (LocalPtr.OpPtr == NULL) {
      //
      // bootp - we only need one and make sure has bootfile
      //
      if (Private->GotBootp || !DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]) {
        continue;
      }

      Private->GotBootp = (UINT8) (NumOffers + 1);
    }
    //
    // if a DHCP type, must be DHCPOFFER and must have server id
    //
    else if (LocalPtr.MessageType->Type != DHCPOFFER || !DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1]) {
      continue;
    } else {
      INTN  TypeIx;

      //
      // get type - PXE10, WfM11a, or BINL
      //
      if (DhcpRxBuf->OpAdds.Status & DISCOVER_TYPE) {
        TypeIx = PXE10_IX;
      } else if (DhcpRxBuf->OpAdds.Status & WfM11a_TYPE) {
        //
        // WfM - make sure it has a bootfile
        //
        if (!DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]) {
          continue;
        }

        TypeIx = WfM11a_IX;
      } else {
        TypeIx = (DhcpRxBuf->OpAdds.Status & PXE_TYPE) ? BINL_IX : DHCP_ONLY_IX;
      }
      //
      // check DHCP or proxy
      //
      if (DhcpRxBuf->u.Dhcpv4.yiaddr == 0) {
        //
        // proxy - only need one of each type if not BINL
        // and must have at least PXE_TYPE
        //
        if (TypeIx == BINL_IX) {
          Private->BinlProxies[Private->GotProxy[BINL_IX]++] = (UINT8) NumOffers;
        } else if (Private->GotProxy[TypeIx]) {
          continue;
        } else {
          Private->GotProxy[TypeIx] = (UINT8) (NumOffers + 1);
        }
      } else {
        Private->OfferCount[TypeIx][Private->ServerCount[TypeIx]++] = (UINT8) NumOffers;
      }
    }

    if (++NumOffers == MAX_OFFERS) {
      break;
    }
  }

  gBS->CloseEvent (TimeoutEvent);
  Private->NumOffersReceived = NumOffers;

  return (Private->NumOffersReceived) ? EFI_SUCCESS : EFI_NO_RESPONSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// send DHCPDECLINE
//
STATIC
VOID
DeclineOffer (
  PXE_BASECODE_DEVICE *Private
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;
  UINT16                  SaveSecs;

  PxebcMode                     = Private->EfiBc.Mode;
  SaveSecs                      = DHCPV4_TRANSMIT_BUFFER.secs;

  DHCPV4_TRANSMIT_BUFFER.secs   = 0;
  DHCPV4_TRANSMIT_BUFFER.flags  = 0;
  SetMem (
    DHCPV4_TRANSMIT_BUFFER.options + sizeof (struct opdeclinestr),
    sizeof (DHCPOpStart) - sizeof (struct opdeclinestr),
    OP_PAD
    );
  DHCPDECLINEoptions.DhcpMessageType.Type = DHCPDECLINE;
  CopyMem (&DHCPDECLINEoptions.OpDeclineEnd, &DHCP_REQ_OPTIONS, sizeof (DHCPDECLINEoptions.OpDeclineEnd));

  {
    EFI_IP_ADDRESS  TmpIp;

    CopyMem (&TmpIp, &DHCP_REQ_OPTIONS.DhcServerIpPtr.Ip, sizeof TmpIp);

    DoUdpWrite (
      Private,
      &TmpIp,
      &DhcpServerPort,
      &PxebcMode->StationIp,
      &DHCPClientPort
      );
  }

  InitDhcpv4TxBuf (Private);
  DHCPV4_TRANSMIT_BUFFER.secs = SaveSecs;
  Private->GoodStationIp      = FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// send DHCPRELEASE
//
STATIC
BOOLEAN
Release (
  PXE_BASECODE_DEVICE *Private
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;
  UINT16                  SaveSecs;

  PxebcMode                   = Private->EfiBc.Mode;
  SaveSecs                    = DHCPV4_TRANSMIT_BUFFER.secs;
  DHCPV4_TRANSMIT_BUFFER.secs = 0;

  SetMem (
    DHCPV4_TRANSMIT_BUFFER.options + sizeof (struct opreleasestr),
    sizeof (DHCPOpStart) - sizeof (struct opreleasestr),
    OP_PAD
    );

  DHCPRELEASEoptions.DhcpMessageType.Type = DHCPRELEASE;

  CopyMem (
    &DHCPRELEASEoptions.DhcServerIpPtr,
    (DHCPV4_OP_SERVER_IP *) DHCPV4_ACK_BUFFER.OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1],
    sizeof DHCPRELEASEoptions.DhcServerIpPtr
    );

  DHCPRELEASEoptions.End[0] = OP_END;

  {
    EFI_IP_ADDRESS  TmpIp;

    CopyMem (&TmpIp, &DHCPRELEASEoptions.DhcServerIpPtr.Ip, sizeof TmpIp);

    DoUdpWrite (
      Private,
      &TmpIp,
      &DhcpServerPort,
      &PxebcMode->StationIp,
      &DHCPClientPort
      );
  }

  InitDhcpv4TxBuf (Private);

  DHCPV4_TRANSMIT_BUFFER.secs = SaveSecs;
  Private->GoodStationIp      = FALSE;
  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
BOOLEAN
GetBINLAck (
  PXE_BASECODE_DEVICE *Private,
  EFI_IP_ADDRESS      *ServerIpPtr
  )
{
  DHCP_RECEIVE_BUFFER *DhcpRxBuf;
  EFI_STATUS          StatCode;
  EFI_EVENT           TimeoutEvent;

  //
  //
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    return FALSE;
  }

  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerRelative,
                    Private->Timeout * 10000000 + 1000000
                    );

  if (EFI_ERROR (StatCode)) {
    gBS->CloseEvent (TimeoutEvent);
    return FALSE;
  }
  //
  //
  //
  DhcpRxBuf = &PXE_BINL_BUFFER;

  for (;;) {
    EFI_PXE_BASE_CODE_UDP_PORT  BINLSrvPort;

    BINLSrvPort = 0;

    if (GetOfferAck (
          Private,
          AckEdit,
          EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
          ServerIpPtr,
          &BINLSrvPort,
          &Private->EfiBc.Mode->StationIp,
          &PSEUDO_DHCP_CLIENT_PORT,
          DhcpRxBuf,
          TimeoutEvent
          ) != EFI_SUCCESS) {
      break;
    }
    //
    // make sure from whom we wanted
    //
    if (!DhcpRxBuf->u.Dhcpv4.yiaddr && !CompareMem (
                                          &ServerIpPtr->v4,
                                          &((DHCPV4_OP_SERVER_IP *) DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip,
                                          sizeof (ServerIpPtr->v4)
                                          )) {
      gBS->CloseEvent (TimeoutEvent);
      //
      // got an ACK from server
      //
      return TRUE;
    }
  }

  gBS->CloseEvent (TimeoutEvent);
  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// make sure we can get BINL
// send DHCPREQUEST to PXE server
//
STATIC
BOOLEAN
TryBINL (
  PXE_BASECODE_DEVICE *Private,
  INTN                OfferIx
  )
{
  DHCP_RECEIVE_BUFFER *DhcpRxBuf;
  EFI_IP_ADDRESS      ServerIp;
  UINT16              SaveSecs;
  INTN                Index;

  DhcpRxBuf = &RxBuf[OfferIx];

  //
  // use next server address first.
  //
  ServerIp.Addr[0] = DhcpRxBuf->u.Dhcpv4.siaddr;
  if (ServerIp.Addr[0] == 0) {
    //
    // next server address is NULL, use option 54.
    //
    CopyMem (
      ((EFI_IPv4_ADDRESS *) &ServerIp),
      &((DHCPV4_OP_SERVER_IP *) DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }

  //
  // client IP address - filled in by client if it knows it
  //
  CopyMem (
    ((EFI_IPv4_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr),
    &DHCP_REQ_OPTIONS.OpReqIP.Ip,
    sizeof (EFI_IPv4_ADDRESS)
    );

  SetMem (&DHCP_REQ_OPTIONS, sizeof DHCP_REQ_OPTIONS, OP_PAD);
  DHCPV4_TRANSMIT_BUFFER.flags  = 0;
  DHCPV4_OPTIONS_BUFFER.End[0]  = OP_END;
  AddRouters (Private, DhcpRxBuf);
  SaveSecs = DHCPV4_TRANSMIT_BUFFER.secs;

  for (Index = 0; Index < 3; Private->TotalSeconds = (UINT16) (Private->TotalSeconds + Private->Timeout), ++Index) {
    DHCPV4_TRANSMIT_BUFFER.secs = HTONS (Private->TotalSeconds);

    //
    // unicast DHCPREQUEST to PXE server
    //
    if (DoUdpWrite (
          Private,
          &ServerIp,
          &PseudoDhcpServerPort,
          (EFI_IP_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr,
          &PSEUDO_DHCP_CLIENT_PORT
          ) != EFI_SUCCESS) {
      break;
    }

    if (!GetBINLAck (Private, &ServerIp)) {
      continue;
    }
    //
    // early exit failures
    // make sure a good ACK
    //
    if (!DHCPOfferAckEdit (&PXE_BINL_BUFFER) || (
          !(PXE_BINL_BUFFER.OpAdds.Status & DISCOVER_TYPE) && !PXE_BINL_BUFFER.OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]
      )
        ) {
      break;
    }

    Private->EfiBc.Mode->ProxyOfferReceived = TRUE;
    return TRUE;
  }
  //
  // failed - reset seconds field, etc.
  //
  Private->EfiBc.Mode->RouteTableEntries = 0;
  //
  // reset
  //
  DHCPV4_TRANSMIT_BUFFER.secs = SaveSecs;
  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
BOOLEAN
TryFinishBINL (
  PXE_BASECODE_DEVICE *Private,
  INTN                OfferIx
  )
{
  if (TryBINL (Private, OfferIx)) {
    return TRUE;
  }

  return Release (Private);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
BOOLEAN
TryFinishProxyBINL (
  PXE_BASECODE_DEVICE *Private
  )
{
  INTN  Index;

  for (Index = 0; Index < Private->GotProxy[BINL_IX]; ++Index) {
    if (TryBINL (Private, Private->BinlProxies[Index])) {
      return TRUE;
    }
  }

  return Release (Private);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// try to finish DORA - send DHCP request, wait for ACK, check with ARP
//
STATIC
BOOLEAN
TryFinishDORA (
  PXE_BASECODE_DEVICE *Private,
  INTN                OfferIx
  )
{
  DHCP_RECEIVE_BUFFER *DhcpRxBuf;
  EFI_IP_ADDRESS      ClientIp;
  EFI_IP_ADDRESS      ServerIp;
  EFI_STATUS          StatCode;
  UNION_PTR           LocalPtr;
  EFI_EVENT           TimeoutEvent;

  //
  // send DHCP request
  // if fail return false
  //
  DhcpRxBuf = &DHCPV4_ACK_BUFFER;
  DHCPV4_OPTIONS_BUFFER.DhcpMessageType.Type  = DHCPREQUEST;
  CopyMem (&DHCP_REQ_OPTIONS, &RequestOpEndStr, sizeof (DHCP_REQ_OPTIONS));
  DHCP_REQ_OPTIONS.OpReqIP.Ip = *(EFI_IPv4_ADDRESS *) &RxBuf[OfferIx].u.Dhcpv4.yiaddr;

  CopyMem (
    &DHCP_REQ_OPTIONS.DhcServerIpPtr.Ip,
    &((DHCPV4_OP_SERVER_IP *) RxBuf[OfferIx].OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip,
    sizeof DHCP_REQ_OPTIONS.DhcServerIpPtr.Ip
    );

  CopyMem (
    Private->EfiBc.Mode->SubnetMask.Addr,
    &DefaultSubnetMask,
    4
    );

  //
  // broadcast DHCPREQUEST
  //
  if (DoUdpWrite (
        Private,
        &BroadcastIP,
        &DhcpServerPort,
        (EFI_IP_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr,
        &DHCPClientPort
        ) != EFI_SUCCESS) {
    return FALSE;
  }
  //
  //
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    return FALSE;
  }

  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerPeriodic,
                    Private->Timeout * 10000000 + 1000000
                    );

  if (EFI_ERROR (StatCode)) {
    gBS->CloseEvent (TimeoutEvent);
    return FALSE;
  }
  //
  // wait for ACK
  //
  for (;;) {
    if (GetOfferAck (
          Private,
          DHCPAckEdit,
          EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP,
          &ServerIp,
          &DhcpServerPort,
          &ClientIp,
          &DHCPClientPort,
          DhcpRxBuf,
          TimeoutEvent
          ) != EFI_SUCCESS) {
      break;
    }
    //
    // check type of response - need DHCPACK
    //
    if (CompareMem (
          &DHCP_REQ_OPTIONS.OpReqIP.Ip,
          &DhcpRxBuf->u.Dhcpv4.yiaddr,
          sizeof (EFI_IPv4_ADDRESS)
          ) || CompareMem (
          &DHCP_REQ_OPTIONS.DhcServerIpPtr.Ip,
          &((DHCPV4_OP_SERVER_IP *) DhcpRxBuf->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip,
          sizeof (EFI_IPv4_ADDRESS)
          )) {
      continue;
    }
    //
    // got ACK
    // check with ARP that IP unused - good return true
    //
    if (!SetStationIP (Private)) {
      //
      // fail - send DHCPDECLINE and return false
      //
      DeclineOffer (Private);
      break;
    }

    LocalPtr.OpPtr = DHCPV4_ACK_BUFFER.OpAdds.PktOptAdds[OP_SUBNET_MASK_IX - 1];

    if (LocalPtr.OpPtr != NULL) {
      CopyMem (
        (EFI_IPv4_ADDRESS *) &Private->EfiBc.Mode->SubnetMask,
        &LocalPtr.SubnetMaskStr->Ip,
        sizeof (EFI_IPv4_ADDRESS)
        );
    }

    AddRouters (Private, DhcpRxBuf);
    gBS->CloseEvent (TimeoutEvent);
    return TRUE;
  }

  gBS->CloseEvent (TimeoutEvent);
  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// try a DHCP server of appropriate type
//
STATIC
BOOLEAN
TryDHCPFinishDORA (
  PXE_BASECODE_DEVICE *Private,
  INTN                TypeIx
  )
{
  INTN  Index;

  //
  // go through the DHCP servers of the requested type
  //
  for (Index = 0; Index < Private->ServerCount[TypeIx]; ++Index) {
    if (TryFinishDORA (Private, Index = Private->OfferCount[TypeIx][Index])) {
      if (TypeIx == BINL_IX && !TryFinishBINL (Private, Index)) {
        continue;
      }

      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// try a DHCP only server and a proxy of appropriate type
//
STATIC
BOOLEAN
TryProxyFinishDORA (
  PXE_BASECODE_DEVICE *Private,
  INTN                TypeIx
  )
{
  INTN  Index;

  if (!Private->GotProxy[TypeIx]) {
    //
    // no proxies of the type wanted
    //
    return FALSE;
  }
  //
  // go through the DHCP only servers
  //
  for (Index = 0; Index < Private->ServerCount[DHCP_ONLY_IX]; ++Index) {
    if (TryFinishDORA (Private, Private->OfferCount[DHCP_ONLY_IX][Index])) {
      if (TypeIx != BINL_IX) {
        CopyProxyRxBuf (Private, Private->GotProxy[TypeIx] - 1);
      } else if (!TryFinishProxyBINL (Private)) {
        //
        // if didn't work with this DHCP, won't work with any
        //
        return FALSE;
      }

      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// getting to the bottom of the barrel
//
STATIC
BOOLEAN
TryAnyWithBootfileFinishDORA (
  PXE_BASECODE_DEVICE *Private
  )
{
  //
  // try a DHCP only server who has a bootfile
  //
  UNION_PTR LocalPtr;
  INTN      Index;

  for (Index = 0; Index < Private->ServerCount[DHCP_ONLY_IX]; ++Index) {
    INTN  offer;

    offer = Private->OfferCount[DHCP_ONLY_IX][Index];

    if (RxBuf[offer].OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1] && TryFinishDORA (Private, offer)) {
      return TRUE;
    }
  }
  //
  // really at bottom - see if be have any bootps
  //
  if (!Private->GotBootp) {
    return FALSE;
  }

  DHCP_REQ_OPTIONS.OpReqIP.Ip = *(EFI_IPv4_ADDRESS *) &RxBuf[Private->GotBootp - 1].u.Dhcpv4.yiaddr;

  if (!SetStationIP (Private)) {
    return FALSE;
  }
  //
  // treat BOOTP response as DHCP ACK packet
  //
  CopyParseRxBuf (Private, Private->GotBootp - 1, DHCPV4_ACK_INDEX);

  LocalPtr.OpPtr = RxBuf[Private->GotBootp - 1].OpAdds.PktOptAdds[OP_SUBNET_MASK_IX - 1];

  if (LocalPtr.OpPtr != NULL) {
    *(EFI_IPv4_ADDRESS *) &Private->EfiBc.Mode->SubnetMask = LocalPtr.SubnetMaskStr->Ip;
  }

  return TRUE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* DoDhcpDora()
 */
STATIC
EFI_STATUS
DoDhcpDora (
  PXE_BASECODE_DEVICE *Private,
  BOOLEAN             SortOffers
  )
{
  EFI_PXE_BASE_CODE_IP_FILTER Filter;
  EFI_STATUS                  StatCode;
  INTN                        NumOffers;

  Filter.Filters  = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP | EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST;

  Filter.IpCnt    = 0;
  Filter.reserved = 0;

  //
  // set filter unicast or broadcast
  //
  if ((StatCode = IpFilter (Private, &Filter)) != EFI_SUCCESS) {
    return StatCode;
  }
  //
  // seed random number with hardware address
  //
  SeedRandom (Private, *(UINT16 *) &Private->SimpleNetwork->Mode->CurrentAddress);

  for (Private->Timeout = 1;
       Private->Timeout < 17;
       Private->TotalSeconds = (UINT16) (Private->TotalSeconds + Private->Timeout), Private->Timeout <<= 1
      ) {
    INTN  Index;

    InitDhcpv4TxBuf (Private);
    DHCPV4_TRANSMIT_BUFFER.xid  = Random (Private);
    DHCPV4_TRANSMIT_BUFFER.secs = HTONS (Private->TotalSeconds);

    //
    // broadcast DHCPDISCOVER
    //
    StatCode = DoUdpWrite (
                Private,
                &BroadcastIP,
                &DhcpServerPort,
                (EFI_IP_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr,
                &DHCPClientPort
                );

    if (StatCode != EFI_SUCCESS) {
      return StatCode;
    }

    CopyMem (
      &Private->EfiBc.Mode->DhcpDiscover,
      (EFI_PXE_BASE_CODE_PACKET *) &DHCPV4_TRANSMIT_BUFFER,
      sizeof (EFI_PXE_BASE_CODE_PACKET)
      );

    //
    // get DHCPOFFER's
    //
    if ((StatCode = GetOffers (Private)) != EFI_SUCCESS) {
      if (StatCode != EFI_NO_RESPONSE) {
        return StatCode;
      }

      continue;
    }
    //
    // select offer and reply DHCPREQUEST
    //
    if (SortOffers) {
      if (TryDHCPFinishDORA(Private, PXE10_IX) || // try DHCP with PXE10
        TryDHCPFinishDORA(Private, WfM11a_IX)  || // no - try with WfM
        TryProxyFinishDORA(Private, PXE10_IX)  || // no - try DHCP only and proxy with PXE10
        TryProxyFinishDORA(Private, WfM11a_IX) || // no - try DHCP only and proxy with WfM
        TryDHCPFinishDORA(Private, BINL_IX)    || // no - try with WfM
        TryProxyFinishDORA(Private, BINL_IX)   || // no - try DHCP only and proxy with PXE10
        TryAnyWithBootfileFinishDORA(Private))
      {
        return EFI_SUCCESS;
      }

      continue;
    }
    //
    // FIFO order
    //
    NumOffers = Private->NumOffersReceived;

    for (Index = 0; Index < NumOffers; ++Index) {
      //
      // ignore proxies
      //
      if (!RxBuf[Index].u.Dhcpv4.yiaddr) {
        continue;
      }
      //
      // check if a bootp server
      //
      if (!RxBuf[Index].OpAdds.PktOptAdds[OP_DHCP_MESSAGE_TYPE_IX - 1]) {
        //
        // it is - just check ARP
        //
        if (!SetStationIP (Private)) {
          continue;
        }
      }
      //
      // else check if a DHCP only server
      //
      else if (!(RxBuf[Index].OpAdds.Status & (DISCOVER_TYPE | WfM11a_TYPE | PXE_TYPE))) {
        //
        // it is a normal DHCP offer (without any PXE options), just finish the D.O.R.A by sending DHCP request.
        //
        if (!TryFinishDORA (Private, Index)) {
          continue;
        }
      } else if (TryFinishDORA (Private, Index)) {
        if (!(RxBuf[Index].OpAdds.Status & (DISCOVER_TYPE | WfM11a_TYPE)) && !TryFinishBINL (Private, Index)) {
          continue;
        }
      }

      DEBUG ((DEBUG_WARN, "\nDoDhcpDora()  Got packets.  "));
      return EFI_SUCCESS;
    }
    //
    // now look for DHCP onlys and a Proxy
    //
    for (Index = 0; Index < NumOffers; ++Index) {
      INTN  Index2;

      //
      // ignore proxies, bootps, non DHCP onlys, and bootable DHCPS
      //
      if (!RxBuf[Index].u.Dhcpv4.yiaddr ||
          !RxBuf[Index].OpAdds.PktOptAdds[OP_DHCP_MESSAGE_TYPE_IX - 1] ||
          RxBuf[Index].OpAdds.Status & (DISCOVER_TYPE | WfM11a_TYPE | PXE_TYPE) ||
          RxBuf[Index].OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]
          ) {
        continue;
      }
      //
      // found non bootable DHCP only - try to find a proxy
      //
      for (Index2 = 0; Index2 < NumOffers; ++Index2) {
        if (!RxBuf[Index2].u.Dhcpv4.yiaddr) {
          if (!TryFinishDORA (Private, Index)) {
            //
            // DHCP no ACK
            //
            break;
          }

          if (RxBuf[Index2].OpAdds.Status & (DISCOVER_TYPE | WfM11a_TYPE)) {
            CopyProxyRxBuf (Private, Index2);
          } else if (!TryFinishBINL (Private, Index2)) {
            continue;
          }

          DEBUG ((DEBUG_WARN, "\nDoDhcpDora()  Got packets.  "));
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_NO_RESPONSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// determine if the server ip is in the ip list
//
BOOLEAN
InServerList (
  EFI_IP_ADDRESS    *ServerIpPtr,
  PXE_SERVER_LISTS  *ServerListPtr
  )
{
  UINTN Index;

  if (!ServerListPtr || !ServerListPtr->Ipv4List.IpCount) {
    return TRUE;
  }

  for (Index = 0; Index < ServerListPtr->Ipv4List.IpCount; ++Index) {
    if (!CompareMem (
          ServerIpPtr,
          &ServerListPtr->Ipv4List.IpList[Index],
          sizeof (EFI_IPv4_ADDRESS)
          )) {
      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BOOLEAN
ExtractBootServerList (
  UINT16            Type,
  DHCPV4_OP_STRUCT  *ptr,
  PXE_SERVER_LISTS  **ServerListPtr
  )
{
  UNION_PTR LocalPtr;
  INTN      ServerListLen;

  LocalPtr.OpPtr  = ptr;
  ServerListLen   = LocalPtr.BootServersStr->Header.Length;

  //
  // find type
  //
  LocalPtr.BootServerList = LocalPtr.BootServersStr->ServerList;

  while (ServerListLen) {
    INTN  ServerEntryLen;

    ServerEntryLen = sizeof (PXEV4_SERVER_LIST) + 2 + (LocalPtr.BootServerList->u.Ipv4List.IpCount - 1) *
    sizeof (EFI_IPv4_ADDRESS);

    if (NTOHS (LocalPtr.BootServerList->Type) == Type) {
      *ServerListPtr = &LocalPtr.BootServerList->u;
      return TRUE;
    }

    (LocalPtr.BytePtr) += ServerEntryLen;
    ServerListLen -= ServerEntryLen;
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
FreeMem (
  PXE_BASECODE_DEVICE *Private
  )
{
  if (Private->TransmitBuffer != NULL) {
    gBS->FreePool (Private->TransmitBuffer);
    Private->TransmitBuffer = NULL;
  }

  if (Private->ReceiveBuffers != NULL) {
    gBS->FreePool (Private->ReceiveBuffers);
    Private->ReceiveBuffers = NULL;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BOOLEAN
GetMem (
  PXE_BASECODE_DEVICE *Private
  )
{
  EFI_STATUS  Status;

  if (Private->DhcpPacketBuffer == NULL) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (DHCP_RECEIVE_BUFFER) * (PXE_BIS_INDEX + 1),
                    &Private->DhcpPacketBuffer
                    );

    if (EFI_ERROR (Status) || Private->DhcpPacketBuffer == NULL) {
      Private->DhcpPacketBuffer = NULL;
      FreeMem (Private);
      return FALSE;
    }
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_PXE_BASE_CODE_PACKET),
                  &Private->TransmitBuffer
                  );

  if (EFI_ERROR (Status) || Private->TransmitBuffer == NULL) {
    gBS->FreePool (Private->DhcpPacketBuffer);
    Private->DhcpPacketBuffer = NULL;
    Private->TransmitBuffer   = NULL;
    FreeMem (Private);
    return FALSE;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (DHCP_RECEIVE_BUFFER) * (MAX_OFFERS),
                  &Private->ReceiveBuffers
                  );

  if (EFI_ERROR (Status) || Private->ReceiveBuffers == NULL) {
    gBS->FreePool (Private->TransmitBuffer);
    gBS->FreePool (Private->DhcpPacketBuffer);
    Private->DhcpPacketBuffer = NULL;
    Private->TransmitBuffer   = NULL;
    Private->ReceiveBuffers   = NULL;
    FreeMem (Private);
    return FALSE;
  }

  return TRUE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
EFI_STATUS
EFIAPI
BcDhcp (
  IN EFI_PXE_BASE_CODE_PROTOCOL *This,
  IN BOOLEAN                    SortOffers
  )
{
  EFI_PXE_BASE_CODE_IP_FILTER Filter;
  EFI_PXE_BASE_CODE_MODE      *PxebcMode;
  PXE_BASECODE_DEVICE         *Private;
  EFI_STATUS                  StatCode;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "PXE_BASECODE_DEVICE pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((DEBUG_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  Filter.Filters  = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  Filter.IpCnt    = 0;
  Filter.reserved = 0;

  DEBUG ((DEBUG_INFO, "\nBcDhcp()  Enter.  "));

  PxebcMode = Private->EfiBc.Mode;

  if (!GetMem (Private)) {
    DEBUG ((DEBUG_ERROR, "\nBcDhcp()  GetMem() failed.\n"));
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }

  PxebcMode->DhcpDiscoverValid = FALSE;
  PxebcMode->DhcpAckReceived = FALSE;
  PxebcMode->ProxyOfferReceived = FALSE;

  Private->Function = EFI_PXE_BASE_CODE_FUNCTION_DHCP;

  //
  // Issue BC command
  //
  if (Private->TotalSeconds == 0) {
    //
    // put in seconds field of DHCP send packets
    //
    Private->TotalSeconds = 4;
  }

  if ((StatCode = DoDhcpDora (Private, SortOffers)) == EFI_SUCCESS) {
    //
    // success - copy packets
    //
    PxebcMode->DhcpDiscoverValid = PxebcMode->DhcpAckReceived = TRUE;

    CopyMem (
      &PxebcMode->DhcpAck,
      (EFI_PXE_BASE_CODE_PACKET *) &DHCPV4_ACK_PACKET,
      sizeof (EFI_PXE_BASE_CODE_PACKET)
      );

    if (PxebcMode->ProxyOfferReceived) {
      CopyMem (
        &PxebcMode->ProxyOffer,
        (EFI_PXE_BASE_CODE_PACKET *) &PXE_OFFER_PACKET,
        sizeof (EFI_PXE_BASE_CODE_PACKET)
        );
    }
  }
  //
  // set filter back to unicast
  //
  IpFilter (Private, &Filter);

  FreeMem (Private);

  //
  // Unlock the instance data
  //
  DEBUG ((DEBUG_WARN, "\nBcDhcp()  Exit = %xh  ", StatCode));

  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
BOOLEAN
VerifyCredentialOption (
  UINT8 *tx,
  UINT8 *rx
  )
{
  UINTN n;

  //
  // Fail verification if either pointer is NULL.
  //
  if (tx == NULL || rx == NULL) {
    return FALSE;
  }
  //
  // Fail verification if tx[0] is not a credential type option
  // or if the length is zero or not a multiple of four.
  //
  if (tx[0] != VEND_PXE_CREDENTIAL_TYPES || tx[1] == 0 || tx[1] % 4 != 0) {
    return FALSE;
  }
  //
  // Fail verification if rx[0] is not a credential type option
  // or if the length is not equal to four.
  //
  if (rx[0] != VEND_PXE_CREDENTIAL_TYPES || rx[1] != 4) {
    return FALSE;
  }
  //
  // Look through transmitted credential types for a copy
  // of the received credential type.
  //
  for (n = 0; n < tx[1]; n += 4) {
    if (!CompareMem (&tx[n + 2], &rx[2], 4)) {
      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
EFI_STATUS
DoDiscover (
  PXE_BASECODE_DEVICE *Private,
  UINT16              OpFlags,
  IN UINT16           Type,
  IN UINT16           *LayerPtr,
  IN BOOLEAN          UseBis,
  EFI_IP_ADDRESS      *DestPtr,
  PXE_SERVER_LISTS    *ServerListPtr
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT  ClientPort;
  EFI_PXE_BASE_CODE_UDP_PORT  ServerPort;
  EFI_PXE_BASE_CODE_MODE      *PxebcMode;
  EFI_STATUS                  StatCode;
  EFI_EVENT                   TimeoutEvent;
  UINT8                       OpLen;

  PxebcMode = Private->EfiBc.Mode;

  if (DestPtr->Addr[0] == 0) {
    DEBUG ((DEBUG_WARN, "\nDoDiscover()  !DestPtr->Addr[0]"));
    return EFI_INVALID_PARAMETER;
  }
  //
  // seed random number with hardware address
  //
  SeedRandom (Private, *(UINT16 *) &Private->SimpleNetwork->Mode->CurrentAddress);

  if (DestPtr->Addr[0] == BroadcastIP.Addr[0]) {
    ClientPort  = DHCPClientPort;
    ServerPort  = DhcpServerPort;
  } else {
    ClientPort  = PSEUDO_DHCP_CLIENT_PORT;
    ServerPort  = PseudoDhcpServerPort;
  }

  if (UseBis) {
    *LayerPtr |= PXE_BOOT_LAYER_CREDENTIAL_FLAG;
  } else {
    *LayerPtr &= PXE_BOOT_LAYER_MASK;
  }

  for (Private->Timeout = 1;
       Private->Timeout < 5;
       Private->TotalSeconds = (UINT16) (Private->TotalSeconds + Private->Timeout), ++Private->Timeout
      ) {
    InitDhcpv4TxBuf (Private);
    //
    // initialize DHCP message structure
    //
    DHCPV4_TRANSMIT_BUFFER.xid  = Random (Private);
    DHCPV4_TRANSMIT_BUFFER.secs = HTONS (Private->TotalSeconds);
    CopyMem (
      &DHCPV4_TRANSMIT_BUFFER.ciaddr,
      &PxebcMode->StationIp,
      sizeof DHCPV4_TRANSMIT_BUFFER.ciaddr
      );

    DHCPV4_OPTIONS_BUFFER.DhcpMessageType.Type  = DHCPREQUEST;
    DISCOVERoptions.Header.OpCode               = OP_VENDOR_SPECIFIC;
    DISCOVERoptions.BootItem.Header.OpCode      = VEND_PXE_BOOT_ITEM;
    DISCOVERoptions.BootItem.Header.Length      = DHCPV4_OPTION_LENGTH (PXE_OP_BOOT_ITEM);
    DISCOVERoptions.BootItem.Type               = HTONS (Type);
    DISCOVERoptions.BootItem.Layer              = HTONS (*LayerPtr);

    if (UseBis) {
      EFI_BIS_PROTOCOL        *BisPtr;
      BIS_APPLICATION_HANDLE  BisAppHandle;
      EFI_BIS_DATA            *BisDataSigInfo;
      EFI_BIS_SIGNATURE_INFO  *BisSigInfo;
      UINTN                   Index;
      UINTN                   Index2;

      BisPtr = PxebcBisStart (
                Private,
                &BisAppHandle,
                &BisDataSigInfo
                );

      if (BisPtr == NULL) {
        //
        // %%TBD - In order to get here, BIS must have
        // been present when PXEBC.Start() was called.
        // BIS had to be shutdown/removed/damaged
        // before PXEBC.Discover() was called.
        // Do we need to document a specific error
        // for this case?
        //
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Compute number of credential types.
      //
      Index2                        = BisDataSigInfo->Length / sizeof (EFI_BIS_SIGNATURE_INFO);

      DISCREDoptions.Header.OpCode  = VEND_PXE_CREDENTIAL_TYPES;

      DISCREDoptions.Header.Length  = (UINT8) (Index2 * sizeof (PXE_CREDENTIAL));

      OpLen = (UINT8) (DHCPV4_OPTION_LENGTH (PXE_DISCOVER_OPTIONS) + sizeof (DHCPV4_OP_HEADER) + DISCREDoptions.Header.Length);

      BisSigInfo = (EFI_BIS_SIGNATURE_INFO *) BisDataSigInfo->Data;

      for (Index = 0; Index < Index2; ++Index) {
        UINT32  x;

        CopyMem (&x, &BisSigInfo[Index], sizeof x);
        x = HTONL (x);
        CopyMem (&DISCREDoptions.Credentials[Index], &x, sizeof x);
      }

      PxebcBisStop (BisPtr, BisAppHandle, BisDataSigInfo);
    } else {
      OpLen = DHCPV4_OPTION_LENGTH (PXE_DISCOVER_OPTIONS);
    }

    DISCOVERoptions.Header.Length = OpLen;

    ((UINT8 *) &DISCOVERoptions)[sizeof (DHCPV4_OP_HEADER) + OpLen - 1] = OP_END;
    ((UINT8 *) &DISCOVERoptions)[sizeof (DHCPV4_OP_HEADER) + OpLen]     = OP_END;

    StatCode = DoUdpWrite (
                Private,
                DestPtr,
                &ServerPort,
                (EFI_IP_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr,
                &ClientPort
                );

    if (StatCode != EFI_SUCCESS) {
      return StatCode;
    }
    //
    //
    //
    StatCode = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &TimeoutEvent
                      );

    if (EFI_ERROR (StatCode)) {
      return StatCode;
    }

    StatCode = gBS->SetTimer (
                      TimeoutEvent,
                      TimerRelative,
                      Private->Timeout * 10000000 + 1000000
                      );

    if (EFI_ERROR (StatCode)) {
      gBS->CloseEvent (TimeoutEvent);
      return StatCode;
    }
    //
    // wait for ACK
    //
    for (;;) {
      DHCP_RECEIVE_BUFFER *RxBufPtr;
      UINT16              TmpType;
      UINT16              TmpLayer;

      RxBufPtr = UseBis ? &PXE_BIS_BUFFER : &PXE_ACK_BUFFER;
      ZeroMem (&Private->ServerIp, sizeof (EFI_IP_ADDRESS));

      if (GetOfferAck (
            Private,
            AckEdit,
            OpFlags,
            (EFI_IP_ADDRESS *) &Private->ServerIp,
            0,
            (EFI_IP_ADDRESS *) &DHCPV4_TRANSMIT_BUFFER.ciaddr,
            &ClientPort,
            RxBufPtr,
            TimeoutEvent
            ) != EFI_SUCCESS) {
        break;
      }
      //
      // check type of response - need PXEClient DHCPACK of proper type with bootfile
      //
      if (!(RxBufPtr->OpAdds.Status & PXE_TYPE) ||
          (UseBis && (RxBufPtr->OpAdds.Status & USE_THREE_BYTE)) ||
          !RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1] ||
          !RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1] ||
          !InServerList((EFI_IP_ADDRESS *)&((DHCPV4_OP_SERVER_IP *)RxBufPtr->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX-1])->Ip, ServerListPtr)) {

        continue;
      }

      TmpType = TmpLayer = 0;

      if (RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1]) {
        TmpType = NTOHS (((PXE_OP_BOOT_ITEM *) RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1])->Type);

        if (RxBufPtr->OpAdds.Status & USE_THREE_BYTE) {
          TmpLayer = (UINT16) (((PXE_OP_BOOT_ITEM *) RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1])->Layer >> 8);
        } else {
          TmpLayer = NTOHS (((PXE_OP_BOOT_ITEM *) RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_ITEM_IX - 1])->Layer);
        }
      }

      if (TmpType != Type) {
        continue;
      }

      if (UseBis) {
        if (!RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_CREDENTIAL_TYPES_IX - 1]) {
          continue;
        }

        if (!VerifyCredentialOption (
              (UINT8 *) &DISCREDoptions.Header,
              (UINT8 *) RxBufPtr->OpAdds.PxeOptAdds[VEND_PXE_CREDENTIAL_TYPES_IX - 1]
              )) {
          continue;
        }
      }

      *LayerPtr = TmpLayer;

      if (UseBis) {
        CopyMem (
          &PxebcMode->PxeBisReply,
          &RxBufPtr->u.Dhcpv4,
          sizeof (EFI_PXE_BASE_CODE_PACKET)
          );

        PxebcMode->PxeBisReplyReceived = TRUE;

        StatCode = DoDiscover (
                    Private,
                    EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
                    Type,
                    LayerPtr,
                    FALSE,
                    &Private->ServerIp,
                    0
                    );

        gBS->CloseEvent (TimeoutEvent);
        return StatCode;
      }

      PxebcMode->PxeDiscoverValid = PxebcMode->PxeReplyReceived = TRUE;

      CopyMem (
        &PxebcMode->PxeDiscover,
        &*(EFI_PXE_BASE_CODE_PACKET *) &DHCPV4_TRANSMIT_BUFFER,
        sizeof (*(EFI_PXE_BASE_CODE_PACKET *) &DHCPV4_TRANSMIT_BUFFER)
        );

      CopyMem (
        &PxebcMode->PxeReply,
        &*(EFI_PXE_BASE_CODE_PACKET *) &RxBufPtr->u.Dhcpv4,
        sizeof (*(EFI_PXE_BASE_CODE_PACKET *) &RxBufPtr->u.Dhcpv4)
        );

      AddRouters (Private, RxBufPtr);

      gBS->CloseEvent (TimeoutEvent);
      return EFI_SUCCESS;
    }

    gBS->CloseEvent (TimeoutEvent);
  }
  //
  // end for loop
  //
  return EFI_TIMEOUT;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
  Parameters:
  Private := Pointer to PxeBc interface
  Type :=
  LayerPtr :=
  UseBis :=
  DiscoverInfoPtr :=
  McastServerListPtr :=
  ServerListPtr :=


**/
STATIC
EFI_STATUS
Discover (
  PXE_BASECODE_DEVICE                 *Private,
  IN UINT16                           Type,
  IN UINT16                           *LayerPtr,
  IN BOOLEAN                          UseBis,
  IN EFI_PXE_BASE_CODE_DISCOVER_INFO  *DiscoverInfoPtr,
  PXE_SERVER_LISTS                    *McastServerListPtr,
  PXE_SERVER_LISTS                    *ServerListPtr
  )
{
  EFI_IP_ADDRESS  DestIp;
  EFI_STATUS      StatCode;

  DEBUG ((DEBUG_INFO, "\nDiscover()  Type=%d  Layer=%d  ", Type, *LayerPtr));

  if (UseBis) {
    DEBUG ((DEBUG_INFO, "BIS  "));
  }
  //
  // get dest IP addr - mcast, bcast, or unicast
  //
  if (DiscoverInfoPtr->UseMCast) {
    DestIp.v4 = DiscoverInfoPtr->ServerMCastIp.v4;

    DEBUG (
      (DEBUG_INFO,
      "\nDiscover()  MCast %d.%d.%d.%d  ",
      DestIp.v4.Addr[0],
      DestIp.v4.Addr[1],
      DestIp.v4.Addr[2],
      DestIp.v4.Addr[3])
      );

    if ((StatCode = DoDiscover (
                      Private,
                      EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
                      Type,
                      LayerPtr,
                      UseBis,
                      &DestIp,
                      McastServerListPtr
                      )) != EFI_TIMEOUT) {
      DEBUG (
        (DEBUG_WARN,
        "\nDiscover()  status == %r (%Xh)",
        StatCode,
        StatCode)
        );

      return StatCode;
    }
  }

  if (DiscoverInfoPtr->UseBCast) {
    DEBUG ((DEBUG_INFO, "\nDiscver()  BCast  "));

    if ((StatCode = DoDiscover (
                      Private,
                      EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
                      Type,
                      LayerPtr,
                      UseBis,
                      &BroadcastIP,
                      McastServerListPtr
                      )) != EFI_TIMEOUT) {

      DEBUG ((DEBUG_WARN, "\nDiscover()  status == %r (%Xh)", StatCode, StatCode));

      return StatCode;
    }
  }

  if (DiscoverInfoPtr->UseUCast) {
    UINTN Index;

    DEBUG (
      (DEBUG_INFO,
      "\nDiscover()  UCast  IP#=%d  ",
      ServerListPtr->Ipv4List.IpCount)
      );

    for (Index = 0; Index < ServerListPtr->Ipv4List.IpCount; ++Index) {
      CopyMem (&DestIp, &ServerListPtr->Ipv4List.IpList[Index], 4);

      DEBUG (
        (DEBUG_INFO,
        "\nDiscover()  UCast %d.%d.%d.%d  ",
        DestIp.v4.Addr[0],
        DestIp.v4.Addr[1],
        DestIp.v4.Addr[2],
        DestIp.v4.Addr[3])
        );

      if ((StatCode = DoDiscover (
                        Private,
                        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
                        Type,
                        LayerPtr,
                        UseBis,
                        &DestIp,
                        0
                        )) != EFI_TIMEOUT) {
        DEBUG (
          (DEBUG_WARN,
          "\nDiscover()  status == %r (%Xh)",
          StatCode,
          StatCode)
          );

        return StatCode;
      }
    }
  }

  DEBUG ((DEBUG_WARN, "\nDiscover()  TIMEOUT"));

  return EFI_TIMEOUT;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* BcDiscover()
 */

/**


**/
EFI_STATUS
EFIAPI
BcDiscover (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN UINT16                           Type,
  IN UINT16                           *LayerPtr,
  IN BOOLEAN                          UseBis,
  IN EFI_PXE_BASE_CODE_DISCOVER_INFO  * DiscoverInfoPtr OPTIONAL
  )
{
  EFI_PXE_BASE_CODE_DISCOVER_INFO DefaultInfo;
  EFI_PXE_BASE_CODE_MODE          *PxebcMode;
  DHCP_RECEIVE_BUFFER             *DhcpRxBuf;
  PXE_SERVER_LISTS                DefaultSrvList;
  PXE_SERVER_LISTS                *ServerListPtr;
  PXE_SERVER_LISTS                *McastServerListPtr;
  UNION_PTR                       LocalPtr;
  UINTN                           Index;
  UINTN                           Index2;
  BOOLEAN                         AcquiredSrvList;
  EFI_STATUS                      StatCode;
  PXE_BASECODE_DEVICE             *Private;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((DEBUG_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  ServerListPtr       = NULL;
  McastServerListPtr  = NULL;
  AcquiredSrvList     = FALSE;

  PxebcMode           = Private->EfiBc.Mode;

  if (!GetMem (Private)) {
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }

  if (UseBis) {
    if (!PxebcMode->BisSupported) {
      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }
  }

  Private->Function = EFI_PXE_BASE_CODE_FUNCTION_DISCOVER;

  if (Private->TotalSeconds == 0) {
    //
    // put in seconds field of DHCP send packets
    //
    Private->TotalSeconds = 4;
  }

  ZeroMem (&DefaultInfo, sizeof (EFI_PXE_BASE_CODE_DISCOVER_INFO));

  //
  // if layer number not zero, use previous discover
  //
  if (*LayerPtr != 0) {
    DEBUG ((DEBUG_WARN, "\nBcDiscover()  layer != 0"));

    if (DiscoverInfoPtr != NULL) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  layer != 0 && DiscoverInfoPtr != NULL\n"));

      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    if (!PxebcMode->PxeDiscoverValid) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  layer != 0 && PxeDiscoverValid == 0\n"));

      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    if (!PxebcMode->PxeReplyReceived) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  layer != 0 && PxeReplyReceived == 0\n"));

      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    if (UseBis && !PxebcMode->PxeBisReplyReceived) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  layer != 0 && PxeBisReplyReceived == 0\n"));

      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    DefaultInfo.UseUCast            = TRUE;
    DiscoverInfoPtr                 = &DefaultInfo;

    DefaultSrvList.Ipv4List.IpCount = 1;
    CopyMem (&DefaultSrvList.Ipv4List.IpList[0], &Private->ServerIp, 4);

    ServerListPtr = &DefaultSrvList;
  }
  //
  // layer is zero - see if info is supplied or if we need to use info from a cached offer
  //
  else if (!DiscoverInfoPtr) {
    //
    // not supplied - generate it
    // make sure that there is cached, appropriate information
    // if neither DhcpAck packet nor ProxyOffer packet has pxe info, fail
    //
    DhcpRxBuf = (PxebcMode->ProxyOfferReceived) ? &PXE_OFFER_BUFFER : &DHCPV4_ACK_BUFFER;

    if (!PxebcMode->DhcpAckReceived || !(DhcpRxBuf->OpAdds.Status & DISCOVER_TYPE)) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  !ack && !proxy"));
      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    DiscoverInfoPtr = &DefaultInfo;

    LocalPtr.OpPtr  = DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_CONTROL_IX - 1];

    //
    // if multicast enabled, need multicast address
    //
    if (!(LocalPtr.DiscoveryControl->ControlBits & DISABLE_MCAST)) {
      DefaultInfo.UseMCast = TRUE;

      CopyMem (
        ((EFI_IPv4_ADDRESS *) &DefaultInfo.ServerMCastIp),
        &((DHCPV4_OP_IP_ADDRESS *) DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_MCAST_ADDR_IX - 1])->Ip,
        sizeof (EFI_IPv4_ADDRESS)
        );
    }

    DefaultInfo.UseBCast    = (BOOLEAN) ((LocalPtr.DiscoveryControl->ControlBits & DISABLE_BCAST) == 0);

    DefaultInfo.MustUseList = (BOOLEAN) ((LocalPtr.DiscoveryControl->ControlBits & USE_ACCEPT_LIST) != 0);

    DefaultInfo.UseUCast = (BOOLEAN)
      (
        (DefaultInfo.MustUseList) ||
        ((LocalPtr.DiscoveryControl->ControlBits & (DISABLE_MCAST | DISABLE_BCAST)) == (DISABLE_MCAST | DISABLE_BCAST))
      );

    if ((DefaultInfo.UseUCast | DefaultInfo.MustUseList) && !ExtractBootServerList (
                                                              Type,
                                                              DhcpRxBuf->OpAdds.PxeOptAdds[VEND_PXE_BOOT_SERVERS_IX - 1],
                                                              &ServerListPtr
                                                              )) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  type not in list"));
      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Info supplied - make SrvList if required
  // if we use ucast discovery or must use list, there better be one
  //
  else if (DiscoverInfoPtr->UseUCast || DiscoverInfoPtr->MustUseList) {
    //
    // there better be a list
    //
    if (DiscoverInfoPtr->IpCnt == 0) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  no bootserver list"));
      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }
    //
    // get its size
    //
    for (Index = Index2 = 0; Index < DiscoverInfoPtr->IpCnt; ++Index) {
      if (DiscoverInfoPtr->SrvList[Index].Type == Type) {
        if (DiscoverInfoPtr->SrvList[Index].AcceptAnyResponse) {
          if (Index2 != 0) {
            DEBUG ((DEBUG_WARN, "\nBcDiscover()  accept any?"));
            EfiReleaseLock (&Private->Lock);
            return EFI_INVALID_PARAMETER;
          } else {
            Index2                          = 1;
            DefaultSrvList.Ipv4List.IpCount = 0;
            ServerListPtr                   = &DefaultSrvList;
            break;
          }
        } else {
          ++Index2;
        }
      }
    }

    if (Index2 == 0) {
      DEBUG ((DEBUG_WARN, "\nBcDiscover()  !Index2?"));
      EfiReleaseLock (&Private->Lock);
      return EFI_INVALID_PARAMETER;
    }

    if (ServerListPtr == NULL) {
      ServerListPtr = AllocatePool (
                        sizeof (PXEV4_SERVER_LIST) + (Index2 - 1) * sizeof (EFI_IPv4_ADDRESS)
                      );

      if (ServerListPtr == NULL) {
        EfiReleaseLock (&Private->Lock);
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // build an array of IP addresses from the server list
      //
      AcquiredSrvList                 = TRUE;
      ServerListPtr->Ipv4List.IpCount = (UINT8) Index2;

      for (Index = Index2 = 0; Index < DiscoverInfoPtr->IpCnt; ++Index) {
        if (DiscoverInfoPtr->SrvList[Index].Type == Type) {
          CopyMem (
            &ServerListPtr->Ipv4List.IpList[Index2++],
            &DiscoverInfoPtr->SrvList[Index].IpAddr.v4,
            sizeof ServerListPtr->Ipv4List.IpList[0]
            );
        }
      }
    }
  }

  if (DiscoverInfoPtr->MustUseList) {
    McastServerListPtr = ServerListPtr;
  }

  if (!(DiscoverInfoPtr->UseMCast || DiscoverInfoPtr->UseBCast || DiscoverInfoPtr->UseUCast)) {
    DEBUG ((DEBUG_WARN, "\nBcDiscover()  Nothing to use!\n"));

    EfiReleaseLock (&Private->Lock);
    return EFI_INVALID_PARAMETER;
  }

  PxebcMode->PxeDiscoverValid = PxebcMode->PxeReplyReceived = PxebcMode->PxeBisReplyReceived = FALSE;

  StatCode = Discover (
              Private,
              Type,
              LayerPtr,
              UseBis,
              DiscoverInfoPtr,
              McastServerListPtr,
              ServerListPtr
              );

  if (AcquiredSrvList) {
    gBS->FreePool (ServerListPtr);
  }

  FreeMem (Private);

  //
  // Unlock the instance data
  //
  DEBUG (
    (DEBUG_INFO,
    "\nBcDiscover()  status == %r (%Xh)\n",
    StatCode,
    StatCode)
    );

  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
EFI_STATUS
EFIAPI
BcSetPackets (
  IN EFI_PXE_BASE_CODE_PROTOCOL   * This,
  BOOLEAN                         *NewDhcpDiscoverValid, OPTIONAL
  BOOLEAN                         *NewDhcpAckReceived, OPTIONAL
  BOOLEAN                         *NewProxyOfferReceived, OPTIONAL
  BOOLEAN                         *NewPxeDiscoverValid, OPTIONAL
  BOOLEAN                         *NewPxeReplyReceived, OPTIONAL
  BOOLEAN                         *NewPxeBisReplyReceived, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewDhcpDiscover, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewDhcpAck, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewProxyOffer, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewPxeDiscover, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewPxeReply, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET     * NewPxeBisReply OPTIONAL
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;
  EFI_STATUS              Status;
  PXE_BASECODE_DEVICE     *Private;

  //
  // Lock the instance data and make sure started
  //

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((DEBUG_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  PxebcMode = Private->EfiBc.Mode;

  if (Private->DhcpPacketBuffer == NULL) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (DHCP_RECEIVE_BUFFER) * (PXE_BIS_INDEX + 1),
                    &Private->DhcpPacketBuffer
                    );

    if (EFI_ERROR (Status) || Private->DhcpPacketBuffer == NULL) {
      Private->DhcpPacketBuffer = NULL;
      EfiReleaseLock (&Private->Lock);
      return EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // Issue BC command
  //
  //
  // reset
  //
  Private->FileSize = 0;
  if (NewDhcpDiscoverValid != NULL) {
    PxebcMode->DhcpDiscoverValid = *NewDhcpDiscoverValid;
  }

  if (NewDhcpAckReceived != NULL) {
    PxebcMode->DhcpAckReceived = *NewDhcpAckReceived;
  }

  if (NewProxyOfferReceived != NULL) {
    PxebcMode->ProxyOfferReceived = *NewProxyOfferReceived;
  }

  if (NewPxeDiscoverValid != NULL) {
    PxebcMode->PxeDiscoverValid = *NewPxeDiscoverValid;
  }

  if (NewPxeReplyReceived != NULL) {
    PxebcMode->PxeReplyReceived = *NewPxeReplyReceived;
  }

  if (NewPxeBisReplyReceived != NULL) {
    PxebcMode->PxeBisReplyReceived = *NewPxeBisReplyReceived;
  }

  if (NewDhcpDiscover != NULL) {
    CopyMem (
      &PxebcMode->DhcpDiscover,
      NewDhcpDiscover,
      sizeof *NewDhcpDiscover
      );
  }

  if (NewDhcpAck != NULL) {
    CopyParse (Private, &PxebcMode->DhcpAck, NewDhcpAck, DHCPV4_ACK_INDEX);
  }

  if (NewProxyOffer != NULL) {
    CopyParse (Private, &PxebcMode->ProxyOffer, NewProxyOffer, PXE_OFFER_INDEX);
  }

  if (NewPxeDiscover != NULL) {
    CopyMem (
      &PxebcMode->PxeDiscover,
      NewPxeDiscover,
      sizeof *NewPxeDiscover
      );
  }

  if (NewPxeReply != NULL) {
    CopyParse (Private, &PxebcMode->PxeReply, NewPxeReply, PXE_ACK_INDEX);
  }

  if (NewPxeBisReply != NULL) {
    CopyParse (Private, &PxebcMode->PxeBisReply, NewPxeBisReply, PXE_BIS_INDEX);
  }
  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);
  return EFI_SUCCESS;
}

/* eof - pxe_bc_dhcp.c */
