/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Dhcp4Option.h

Abstract:

  To validate, parse and process the DHCP options


**/

#ifndef __EFI_DHCP4_OPTION_H__
#define __EFI_DHCP4_OPTION_H__

//
// DHCP option tags (types)
//
enum {
  //
  // RFC1497 vendor extensions
  //
  DHCP_TAG_PAD             = 0,    // Pad Option
  DHCP_TAG_EOP             = 255,  // End Option
  DHCP_TAG_NETMASK         = 1,    // Subnet Mask
  DHCP_TAG_TIME_OFFSET     = 2,    // Time Offset from UTC
  DHCP_TAG_ROUTER          = 3,    // Router option,
  DHCP_TAG_TIME_SERVER     = 4,    // Time Server
  DHCP_TAG_NAME_SERVER     = 5,    // Name Server
  DHCP_TAG_DNS_SERVER      = 6,    // Domain Name Server
  DHCP_TAG_LOG_SERVER      = 7,    // Log Server
  DHCP_TAG_COOKIE_SERVER   = 8,    // Cookie Server
  DHCP_TAG_LPR_SERVER      = 9,    // LPR Print Server
  DHCP_TAG_IMPRESS_SERVER  = 10,   // Impress Server
  DHCP_TAG_RL_SERVER       = 11,   // Resource Location Server
  DHCP_TAG_HOSTNAME        = 12,   // Host Name
  DHCP_TAG_BOOTFILE_LEN    = 13,   // Boot File Size
  DHCP_TAG_DUMP            = 14,   // Merit Dump File
  DHCP_TAG_DOMAINNAME      = 15,   // Domain Name
  DHCP_TAG_SWAP_SERVER     = 16,   // Swap Server
  DHCP_TAG_ROOTPATH        = 17,   // Root path
  DHCP_TAG_EXTEND_PATH     = 18,   // Extensions Path

  //
  // IP Layer Parameters per Host
  //
  DHCP_TAG_IPFORWARD       = 19, // IP Forwarding Enable/Disable
  DHCP_TAG_NONLOCAL_SRR    = 20, // on-Local Source Routing Enable/Disable
  DHCP_TAG_POLICY_SRR      = 21, // Policy Filter
  DHCP_TAG_EMTU            = 22, // Maximum Datagram Reassembly Size
  DHCP_TAG_TTL             = 23, // Default IP Time-to-live
  DHCP_TAG_PATHMTU_AGE     = 24, // Path MTU Aging Timeout
  DHCP_TAG_PATHMTU_PLATEAU = 25, // Path MTU Plateau Table

  //
  // IP Layer Parameters per Interface
  //
  DHCP_TAG_IFMTU           = 26, // Interface MTU
  DHCP_TAG_SUBNET_LOCAL    = 27, // All Subnets are Local
  DHCP_TAG_BROADCAST       = 28, // Broadcast Address
  DHCP_TAG_DISCOVER_MASK   = 29, // Perform Mask Discovery
  DHCP_TAG_SUPPLY_MASK     = 30, // Mask Supplier
  DHCP_TAG_DISCOVER_ROUTE  = 31, // Perform Router Discovery
  DHCP_TAG_ROUTER_SOLICIT  = 32, // Router Solicitation Address
  DHCP_TAG_STATIC_ROUTE    = 33, // Static Route

  //
  // Link Layer Parameters per Interface
  //
  DHCP_TAG_TRAILER         = 34, // Trailer Encapsulation
  DHCP_TAG_ARPAGE          = 35, // ARP Cache Timeout
  DHCP_TAG_ETHER_ENCAP     = 36, // Ethernet Encapsulation

  //
  // TCP Parameters
  //
  DHCP_TAG_TCP_TTL         = 37, // TCP Default TTL
  DHCP_TAG_KEEP_INTERVAL   = 38, // TCP Keepalive Interval
  DHCP_TAG_KEEP_GARBAGE    = 39, // TCP Keepalive Garbage

  //
  // Application and Service Parameters
  //
  DHCP_TAG_NIS_DOMAIN      = 40,   // Network Information Service Domain
  DHCP_TAG_NIS_SERVER      = 41,   // Network Information Servers
  DHCP_TAG_NTP_SERVER      = 42,   // Network Time Protocol Servers
  DHCP_TAG_VENDOR          = 43,   // Vendor Specific Information
  DHCP_TAG_NBNS            = 44,   // NetBIOS over TCP/IP Name Server
  DHCP_TAG_NBDD            = 45,   // NetBIOS Datagram Distribution Server
  DHCP_TAG_NBTYPE          = 46,   // NetBIOS over TCP/IP Node Type
  DHCP_TAG_NBSCOPE         = 47,   // NetBIOS over TCP/IP Scope
  DHCP_TAG_XFONT           = 48,   // X Window System Font Server
  DHCP_TAG_XDM             = 49,   // X Window System Display Manager
  DHCP_TAG_NISPLUS         = 64,   // Network Information Service+ Domain
  DHCP_TAG_NISPLUS_SERVER  = 65,   // Network Information Service+ Servers
  DHCP_TAG_MOBILEIP        = 68,   // Mobile IP Home Agent
  DHCP_TAG_SMTP            = 69,   // Simple Mail Transport Protocol Server
  DHCP_TAG_POP3            = 70,   // Post Office Protocol (POP3) Server
  DHCP_TAG_NNTP            = 71,   // Network News Transport Protocol Server
  DHCP_TAG_WWW             = 72,   // Default World Wide Web (WWW) Server
  DHCP_TAG_FINGER          = 73,   // Default Finger Server
  DHCP_TAG_IRC             = 74,   // Default Internet Relay Chat (IRC) Server
  DHCP_TAG_STTALK          = 75,   // StreetTalk Server
  DHCP_TAG_STDA            = 76,   // StreetTalk Directory Assistance Server
  DHCP_TAG_CLASSLESS_ROUTE = 121,  // Classless Route

  //
  // DHCP Extensions
  //
  DHCP_TAG_REQUEST_IP      = 50,         // Requested IP Address
  DHCP_TAG_LEASE           = 51,         // IP Address Lease Time
  DHCP_TAG_OVERLOAD        = 52,         // Option Overload
  DHCP_TAG_TFTP            = 66,         // TFTP server name
  DHCP_TAG_BOOTFILE        = 67,         // Bootfile name
  DHCP_TAG_TYPE            = 53,         // DHCP Message Type
  DHCP_TAG_SERVER_ID       = 54,         // Server Identifier
  DHCP_TAG_PARA_LIST       = 55,         // Parameter Request List
  DHCP_TAG_MESSAGE         = 56,         // Message
  DHCP_TAG_MAXMSG          = 57,         // Maximum DHCP Message Size
  DHCP_TAG_T1              = 58,         // Renewal (T1) Time Value
  DHCP_TAG_T2              = 59,         // Rebinding (T2) Time Value
  DHCP_TAG_VENDOR_CLASS    = 60,         // Vendor class identifier
  DHCP_TAG_CLIENT_ID       = 61          // Client-identifier
};

enum {
  DHCP_OPTION_MAGIC        = 0x63538263, // Network byte order
  DHCP_MAX_OPTIONS         = 256,

  //
  // DHCP option types, this is used to validate the DHCP options.
  //
  DHCP_OPTION_SWITCH       = 1,
  DHCP_OPTION_INT8,
  DHCP_OPTION_INT16,
  DHCP_OPTION_INT32,
  DHCP_OPTION_IP,
  DHCP_OPTION_IPPAIR,

  //
  // Value of DHCP overload option
  //
  DHCP_OVERLOAD_FILENAME   = 1,
  DHCP_OVERLOAD_SVRNAME    = 2,
  DHCP_OVERLOAD_BOTH       = 3
};

//
// The DHCP option structure. This structure extends the EFI_DHCP_OPTION
// structure to support options longer than 255 bytes, such as classless route.
//
typedef struct {
  UINT8                     Tag;
  UINT16                    Len;
  UINT8                     *Data;
} DHCP_OPTION;

//
// Structures used to parse the DHCP options with RFC3396 support.
//
typedef struct {
  UINT8                     Index;
  UINT16                    Offset;
} DHCP_OPTION_COUNT;

typedef struct {
  DHCP_OPTION_COUNT         *OpCount;
  DHCP_OPTION               *Options;
  UINT8                     *Buf;
} DHCP_OPTION_CONTEXT;

//
// The options that matters to DHCP driver itself. The user of
// DHCP clients may be interested in other options, such as
// classless route, who can parse the DHCP offer to get them.
//
typedef struct {
  IP4_ADDR                  NetMask;  // DHCP_TAG_NETMASK
  IP4_ADDR                  Router;   // DHCP_TAG_ROUTER, only the first router is used

  //
  // DHCP specific options
  //
  UINT8                     DhcpType; // DHCP_TAG_TYPE
  UINT8                     Overload; // DHCP_TAG_OVERLOAD
  IP4_ADDR                  ServerId; // DHCP_TAG_SERVER_ID
  UINT32                    Lease;    // DHCP_TAG_LEASE
  UINT32                    T1;       // DHCP_TAG_T1
  UINT32                    T2;       // DHCP_TAG_T2
} DHCP_PARAMETER;

//
// Structure used to describe and validate the format of DHCP options.
// Type is the options' data type, such as DHCP_OPTION_INT8. MinOccur
// is the minium occurance of this data type. MaxOccur is defined
// similarly. If MaxOccur is -1, it means that there is no limit on the
// maximum occurance. Alert tells whether DHCP client should further
// inspect the option to parse DHCP_PARAMETER.
//
typedef struct {
  UINT8                     Tag;
  INTN                      Type;
  INTN                      MinOccur;
  INTN                      MaxOccur;
  BOOLEAN                   Alert;
} DHCP_OPTION_FORMAT;

typedef
EFI_STATUS
(*DHCP_CHECK_OPTION) (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  );

EFI_STATUS
DhcpIterateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  IN  DHCP_CHECK_OPTION     Check,          OPTIONAL
  IN  VOID                  *Context
  );

EFI_STATUS
DhcpValidateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT DHCP_PARAMETER        **Para          OPTIONAL
  );

EFI_STATUS
DhcpParseOption (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT INTN                  *Count,
  OUT DHCP_OPTION           **OptionPoint
  );

UINT8 *
DhcpAppendOption (
  IN UINT8                  *Buf,
  IN UINT8                  Tag,
  IN UINT16                 DataLen,
  IN UINT8                  *Data
  );

EFI_STATUS
DhcpBuild (
  IN  EFI_DHCP4_PACKET         *SeedPacket,
  IN  UINT32                   DeleteCount,
  IN  UINT8                    *DeleteList OPTIONAL,
  IN  UINT32                   AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION  *AppendList[] OPTIONAL,
  OUT EFI_DHCP4_PACKET         **NewPacket
  );

#endif
