/** @file
  To validate, parse and process the DHCP options.
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_DHCP4_OPTION_H__
#define __EFI_DHCP4_OPTION_H__

///
/// DHCP option tags (types)
///

//
// RFC1497 vendor extensions
//
#define DHCP_TAG_PAD              0    // Pad Option
#define DHCP_TAG_EOP              255  // End Option
#define DHCP_TAG_NETMASK          1    // Subnet Mask
#define DHCP_TAG_TIME_OFFSET      2    // Time Offset from UTC
#define DHCP_TAG_ROUTER           3    // Router option,
#define DHCP_TAG_TIME_SERVER      4    // Time Server
#define DHCP_TAG_NAME_SERVER      5    // Name Server
#define DHCP_TAG_DNS_SERVER       6    // Domain Name Server
#define DHCP_TAG_LOG_SERVER       7    // Log Server
#define DHCP_TAG_COOKIE_SERVER    8    // Cookie Server
#define DHCP_TAG_LPR_SERVER       9    // LPR Print Server
#define DHCP_TAG_IMPRESS_SERVER   10   // Impress Server
#define DHCP_TAG_RL_SERVER        11   // Resource Location Server
#define DHCP_TAG_HOSTNAME         12   // Host Name
#define DHCP_TAG_BOOTFILE_LEN     13   // Boot File Size
#define DHCP_TAG_DUMP             14   // Merit Dump File
#define DHCP_TAG_DOMAINNAME       15   // Domain Name
#define DHCP_TAG_SWAP_SERVER      16   // Swap Server
#define DHCP_TAG_ROOTPATH         17   // Root path
#define DHCP_TAG_EXTEND_PATH      18   // Extensions Path

//
// IP Layer Parameters per Host
//
#define DHCP_TAG_IPFORWARD        19 // IP Forwarding Enable/Disable
#define DHCP_TAG_NONLOCAL_SRR     20 // on-Local Source Routing Enable/Disable
#define DHCP_TAG_POLICY_SRR       21 // Policy Filter
#define DHCP_TAG_EMTU             22 // Maximum Datagram Reassembly Size
#define DHCP_TAG_TTL              23 // Default IP Time-to-live
#define DHCP_TAG_PATHMTU_AGE      24 // Path MTU Aging Timeout
#define DHCP_TAG_PATHMTU_PLATEAU  25 // Path MTU Plateau Table

//
// IP Layer Parameters per Interface
//
#define DHCP_TAG_IFMTU            26 // Interface MTU
#define DHCP_TAG_SUBNET_LOCAL     27 // All Subnets are Local
#define DHCP_TAG_BROADCAST        28 // Broadcast Address
#define DHCP_TAG_DISCOVER_MASK    29 // Perform Mask Discovery
#define DHCP_TAG_SUPPLY_MASK      30 // Mask Supplier
#define DHCP_TAG_DISCOVER_ROUTE   31 // Perform Router Discovery
#define DHCP_TAG_ROUTER_SOLICIT   32 // Router Solicitation Address
#define DHCP_TAG_STATIC_ROUTE     33 // Static Route

//
// Link Layer Parameters per Interface
//
#define DHCP_TAG_TRAILER          34 // Trailer Encapsulation
#define DHCP_TAG_ARPAGE           35 // ARP Cache Timeout
#define DHCP_TAG_ETHER_ENCAP      36 // Ethernet Encapsulation

//
// TCP Parameters
//
#define DHCP_TAG_TCP_TTL          37 // TCP Default TTL
#define DHCP_TAG_KEEP_INTERVAL    38 // TCP Keepalive Interval
#define DHCP_TAG_KEEP_GARBAGE     39 // TCP Keepalive Garbage

//
// Application and Service Parameters
//
#define DHCP_TAG_NIS_DOMAIN       40   // Network Information Service Domain
#define DHCP_TAG_NIS_SERVER       41   // Network Information Servers
#define DHCP_TAG_NTP_SERVER       42   // Network Time Protocol Servers
#define DHCP_TAG_VENDOR           43   // Vendor Specific Information
#define DHCP_TAG_NBNS             44   // NetBIOS over TCP/IP Name Server
#define DHCP_TAG_NBDD             45   // NetBIOS Datagram Distribution Server
#define DHCP_TAG_NBTYPE           46   // NetBIOS over TCP/IP Node Type
#define DHCP_TAG_NBSCOPE          47   // NetBIOS over TCP/IP Scope
#define DHCP_TAG_XFONT            48   // X Window System Font Server
#define DHCP_TAG_XDM              49   // X Window System Display Manager
#define DHCP_TAG_NISPLUS          64   // Network Information Service+ Domain
#define DHCP_TAG_NISPLUS_SERVER   65   // Network Information Service+ Servers
#define DHCP_TAG_MOBILEIP         68   // Mobile IP Home Agent
#define DHCP_TAG_SMTP             69   // Simple Mail Transport Protocol Server
#define DHCP_TAG_POP3             70   // Post Office Protocol (POP3) Server
#define DHCP_TAG_NNTP             71   // Network News Transport Protocol Server
#define DHCP_TAG_WWW              72   // Default World Wide Web (WWW) Server
#define DHCP_TAG_FINGER           73   // Default Finger Server
#define DHCP_TAG_IRC              74   // Default Internet Relay Chat (IRC) Server
#define DHCP_TAG_STTALK           75   // StreetTalk Server
#define DHCP_TAG_STDA             76   // StreetTalk Directory Assistance Server
#define DHCP_TAG_CLASSLESS_ROUTE  121  // Classless Route

//
// DHCP Extensions
//
#define DHCP_TAG_REQUEST_IP       50         // Requested IP Address
#define DHCP_TAG_LEASE            51         // IP Address Lease Time
#define DHCP_TAG_OVERLOAD         52         // Option Overload
#define DHCP_TAG_TFTP             66         // TFTP server name
#define DHCP_TAG_BOOTFILE         67         // Bootfile name
#define DHCP_TAG_TYPE             53         // DHCP Message Type
#define DHCP_TAG_SERVER_ID        54         // Server Identifier
#define DHCP_TAG_PARA_LIST        55         // Parameter Request List
#define DHCP_TAG_MESSAGE          56         // Message
#define DHCP_TAG_MAXMSG           57         // Maximum DHCP Message Size
#define DHCP_TAG_T1               58         // Renewal (T1) Time Value
#define DHCP_TAG_T2               59         // Rebinding (T2) Time Value
#define DHCP_TAG_VENDOR_CLASS     60         // Vendor class identifier
#define DHCP_TAG_CLIENT_ID        61         // Client-identifier


#define DHCP_OPTION_MAGIC         0x63538263 // Network byte order
#define DHCP_MAX_OPTIONS          256

 
//
// DHCP option types, this is used to validate the DHCP options.
//
#define DHCP_OPTION_SWITCH        1
#define DHCP_OPTION_INT8          2
#define DHCP_OPTION_INT16         3
#define DHCP_OPTION_INT32         4
#define DHCP_OPTION_IP            5
#define DHCP_OPTION_IPPAIR        6

//
// Value of DHCP overload option
//
#define DHCP_OVERLOAD_FILENAME    1
#define DHCP_OVERLOAD_SVRNAME     2
#define DHCP_OVERLOAD_BOTH        3

///
/// The DHCP option structure. This structure extends the EFI_DHCP_OPTION
/// structure to support options longer than 255 bytes, such as classless route.
///
typedef struct {
  UINT8                     Tag;
  UINT16                    Len;
  UINT8                     *Data;
} DHCP_OPTION;

///
/// Structures used to parse the DHCP options with RFC3396 support.
///
typedef struct {
  UINT8                     Index;
  UINT16                    Offset;
} DHCP_OPTION_COUNT;

typedef struct {
  DHCP_OPTION_COUNT         *OpCount;
  DHCP_OPTION               *Options;
  UINT8                     *Buf;
} DHCP_OPTION_CONTEXT;

///
/// The options that matters to DHCP driver itself. The user of
/// DHCP clients may be interested in other options, such as
/// classless route, who can parse the DHCP offer to get them.
///
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

///
/// Structure used to describe and validate the format of DHCP options.
/// Type is the options' data type, such as DHCP_OPTION_INT8. MinOccur
/// is the minium occurance of this data type. MaxOccur is defined
/// similarly. If MaxOccur is -1, it means that there is no limit on the
/// maximum occurance. Alert tells whether DHCP client should further
/// inspect the option to parse DHCP_PARAMETER.
///
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

/**
  Iterate through a DHCP message to visit each option. First inspect
  all the options in the OPTION field. Then if overloaded, inspect
  the options in FILENAME and SERVERNAME fields. One option may be
  encoded in several places. See RFC 3396 Encoding Long Options in DHCP

  @param[in]  Packet                 The DHCP packet to check the options for
  @param[in]  Check                  The callback function to be called for each option
                                     found
  @param[in]  Context                The opaque parameter for Check

  @retval EFI_SUCCESS            The DHCP packet's options are well formated
  @retval EFI_INVALID_PARAMETER  The DHCP packet's options are not well formated

**/
EFI_STATUS
DhcpIterateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  IN  DHCP_CHECK_OPTION     Check         OPTIONAL,
  IN  VOID                  *Context
  );

/**
  Validate the packet's options. If necessary, allocate
  and fill in the interested parameters.

  @param[in]  Packet                 The packet to validate the options
  @param[out] Para                   The variable to save the DHCP parameters.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to validate the packet.
  @retval EFI_INVALID_PARAMETER  The options are mal-formated
  @retval EFI_SUCCESS            The options are parsed into OptionPoint

**/
EFI_STATUS
DhcpValidateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT DHCP_PARAMETER        **Para       OPTIONAL
  );

/**
  Parse the options of a DHCP packet. It supports RFC 3396: Encoding
  Long Options in DHCP. That is, it will combine all the option value
  of all the occurances of each option.
  A little bit of implemenation:
  It adopts the "Key indexed counting" algorithm. First, it allocates
  an array of 256 DHCP_OPTION_COUNTs because DHCP option tag is encoded
  as a UINT8. It then iterates the DHCP packet to get data length of
  each option by calling DhcpIterOptions with DhcpGetOptionLen. Now, it
  knows the number of present options and their length. It allocates a
  array of DHCP_OPTION and a continuous buffer after the array to put
  all the options' data. Each option's data is pointed to by the Data
  field in DHCP_OPTION structure. At last, it call DhcpIterateOptions
  with DhcpFillOption to fill each option's data to its position in the
  buffer.

  @param[in]  Packet                 The DHCP packet to parse the options
  @param[out] Count                  The number of valid dhcp options present in the
                                     packet
  @param[out] OptionPoint            The array that contains the DHCP options. Caller
                                     should free it.

  @retval EFI_NOT_FOUND          Cannot find any option.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to parse the packet.
  @retval EFI_INVALID_PARAMETER  The options are mal-formated
  @retval EFI_SUCCESS            The options are parsed into OptionPoint

**/
EFI_STATUS
DhcpParseOption (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT INTN                  *Count,
  OUT DHCP_OPTION           **OptionPoint
  );

/**
  Append an option to the memory, if the option is longer than
  255 bytes, splits it into several options.

  @param[out] Buf                    The buffer to append the option to
  @param[in]  Tag                    The option's tag
  @param[in]  DataLen                The length of the option's data
  @param[in]  Data                   The option's data

  @return The position to append the next option

**/
UINT8 *
DhcpAppendOption (
  OUT UINT8                  *Buf,
  IN  UINT8                  Tag,
  IN  UINT16                 DataLen,
  IN  UINT8                  *Data
  );

/**
  Build a new DHCP packet from a seed packet. Options may be deleted or
  appended. The caller should free the NewPacket when finished using it.

  @param[in]  SeedPacket             The seed packet to start with
  @param[in]  DeleteCount            The number of options to delete
  @param[in]  DeleteList             The options to delete from the packet
  @param[in]  AppendCount            The number of options to append
  @param[in]  AppendList             The options to append to the packet
  @param[out] NewPacket              The new packet, allocated and built by this
                                     function.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory
  @retval EFI_INVALID_PARAMETER  The options in SeekPacket are mal-formated
  @retval EFI_SUCCESS            The packet is build.

**/
EFI_STATUS
DhcpBuild (
  IN  EFI_DHCP4_PACKET        *SeedPacket,
  IN  UINT32                  DeleteCount,
  IN  UINT8                   *DeleteList     OPTIONAL,
  IN  UINT32                  AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION *AppendList[]   OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  );

#endif
