/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeDhcp4.h

Abstract:
  EFI PXE DHCPv4 protocol definition

--*/

#ifndef _PXEDHCP4_H_
#define _PXEDHCP4_H_


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// PXE DHCPv4 GUID definition
//

#define EFI_PXE_DHCP4_PROTOCOL_GUID \
  { 0x03c4e624, 0xac28, 0x11d3, {0x9a, 0x2d, 0x00, 0x90, 0x29, 0x3f, 0xc1, 0x4d} }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// Interface definition
//

EFI_FORWARD_DECLARATION (EFI_PXE_DHCP4_PROTOCOL);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// Descriptions of the DHCP version 4 header and options can be found
// in RFC-2131 and RFC-2132 at www.ietf.org
//

#pragma pack(1)
typedef struct {

  UINT8 op;
#define BOOTP_REQUEST   1
#define BOOTP_REPLY   2

  UINT8 htype;

  UINT8 hlen;

  UINT8 hops;

  UINT32 xid;

  UINT16 secs;
#define DHCP4_INITIAL_SECONDS 4

  UINT16 flags;
#define DHCP4_BROADCAST_FLAG  0x8000

  UINT32 ciaddr;

  UINT32 yiaddr;

  UINT32 siaddr;

  UINT32 giaddr;

  UINT8 chaddr[16];

  UINT8 sname[64];

  UINT8 fname[128];

//
// This is the minimum option length as specified in RFC-2131.
// The packet must be padded out this far with DHCP4_PAD.
// DHCPv4 packets are usually 576 bytes in length.  This length
// includes the IPv4 and UDPv4 headers but not the media header.
// Note: Not all DHCP relay agents will forward DHCPv4 packets
// if they are less than 384 bytes or exceed 576 bytes.  Even if
// the underlying hardware can handle smaller and larger packets,
// many older relay agents will not accept them.
//
  UINT32 magik;
#define DHCP4_MAGIK_NUMBER  0x63825363

  UINT8 options[308];

} DHCP4_HEADER;
#pragma pack()

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// DHCPv4 packet definition.  Room for 576 bytes including IP and
// UDP header.
//

#define DHCP4_MAX_PACKET_SIZE     576
#define DHCP4_UDP_HEADER_SIZE     8
#define DHCP4_IP_HEADER_SIZE      20

#pragma pack(1)
typedef union _DHCP4_PACKET {
  UINT32 _force_data_alignment;

  UINT8 raw[1500];

  DHCP4_HEADER dhcp4;
} DHCP4_PACKET;
#pragma pack()

#define DHCP4_SERVER_PORT 67
#define DHCP4_CLIENT_PORT 68

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// DHCPv4 and PXE option numbers.
//

#define DHCP4_PAD                             0
#define DHCP4_END                             255
#define DHCP4_SUBNET_MASK                     1
#define DHCP4_TIME_OFFSET                     2
#define DHCP4_ROUTER_LIST                     3
#define DHCP4_TIME_SERVERS                    4
#define DHCP4_NAME_SERVERS                    5
#define DHCP4_DNS_SERVERS                     6
#define DHCP4_LOG_SERVERS                     7
#define DHCP4_COOKIE_SERVERS                  8
#define DHCP4_LPR_SREVERS                     9
#define DHCP4_IMPRESS_SERVERS                 10
#define DHCP4_RESOURCE_LOCATION_SERVERS       11
#define DHCP4_HOST_NAME                       12
#define DHCP4_BOOT_FILE_SIZE                  13
#define DHCP4_DUMP_FILE                       14
#define DHCP4_DOMAIN_NAME                     15
#define DHCP4_SWAP_SERVER                     16
#define DHCP4_ROOT_PATH                       17
#define DHCP4_EXTENSION_PATH                  18
#define DHCP4_IP_FORWARDING                   19
#define DHCP4_NON_LOCAL_SOURCE_ROUTE          20
#define DHCP4_POLICY_FILTER                   21
#define DHCP4_MAX_DATAGRAM_SIZE               22
#define DHCP4_DEFAULT_TTL                     23
#define DHCP4_MTU_AGING_TIMEOUT               24
#define DHCP4_MTU_SIZES                       25
#define DHCP4_MTU_TO_USE                      26
#define DHCP4_ALL_SUBNETS_LOCAL               27
#define DHCP4_BROADCAST_ADDRESS               28
#define DHCP4_PERFORM_MASK_DISCOVERY          29
#define DHCP4_RESPOND_TO_MASK_REQ             30
#define DHCP4_PERFORM_ROUTER_DISCOVERY        31
#define DHCP4_ROUTER_SOLICIT_ADDRESS          32
#define DHCP4_STATIC_ROUTER_LIST              33
#define DHCP4_USE_ARP_TRAILERS                34
#define DHCP4_ARP_CACHE_TIMEOUT               35
#define DHCP4_ETHERNET_ENCAPSULATION          36
#define DHCP4_TCP_DEFAULT_TTL                 37
#define DHCP4_TCP_KEEP_ALIVE_INT              38
#define DHCP4_KEEP_ALIVE_GARBAGE              39
#define DHCP4_NIS_DOMAIN_NAME                 40
#define DHCP4_NIS_SERVERS                     41
#define DHCP4_NTP_SERVERS                     42
#define DHCP4_VENDOR_SPECIFIC                 43
# define PXE_MTFTP_IP                         1
# define PXE_MTFTP_CPORT                      2
# define PXE_MTFTP_SPORT                      3
# define PXE_MTFTP_TMOUT                      4
# define PXE_MTFTP_DELAY                      5
# define PXE_DISCOVERY_CONTROL                6
#  define PXE_DISABLE_BROADCAST_DISCOVERY     0x01
#  define PXE_DISABLE_MULTICAST_DISCOVERY     0x02
#  define PXE_ACCEPT_ONLY_PXE_BOOT_SERVERS    0x04
#  define PXE_DO_NOT_PROMPT                   0x08
# define PXE_DISCOVERY_MCAST_ADDR             7
# define PXE_BOOT_SERVERS                     8
# define PXE_BOOT_MENU                        9
# define PXE_BOOT_PROMPT                      10
# define PXE_MCAST_ADDRS_ALLOC                11
# define PXE_CREDENTIAL_TYPES                 12
# define PXE_BOOT_ITEM                        71
#define DHCP4_NBNS_SERVERS                    44
#define DHCP4_NBDD_SERVERS                    45
#define DHCP4_NETBIOS_NODE_TYPE               46
#define DHCP4_NETBIOS_SCOPE                   47
#define DHCP4_XWINDOW_SYSTEM_FONT_SERVERS     48
#define DHCP4_XWINDOW_SYSTEM_DISPLAY_MANAGERS 49
#define DHCP4_REQUESTED_IP_ADDRESS            50
#define DHCP4_LEASE_TIME                      51
#define DHCP4_OPTION_OVERLOAD                 52
# define DHCP4_OVERLOAD_FNAME                 1
# define DHCP4_OVERLOAD_SNAME                 2
# define DHCP4_OVERLOAD_FNAME_AND_SNAME       3
#define DHCP4_MESSAGE_TYPE                    53
# define DHCP4_MESSAGE_TYPE_DISCOVER          1
# define DHCP4_MESSAGE_TYPE_OFFER             2
# define DHCP4_MESSAGE_TYPE_REQUEST           3
# define DHCP4_MESSAGE_TYPE_DECLINE           4
# define DHCP4_MESSAGE_TYPE_ACK               5
# define DHCP4_MESSAGE_TYPE_NAK               6
# define DHCP4_MESSAGE_TYPE_RELEASE           7
# define DHCP4_MESSAGE_TYPE_INFORM            8
#define DHCP4_SERVER_IDENTIFIER               54
#define DHCP4_PARAMETER_REQUEST_LIST          55
#define DHCP4_ERROR_MESSAGE                   56
#define DHCP4_MAX_MESSAGE_SIZE                57
# define DHCP4_DEFAULT_MAX_MESSAGE_SIZE       576
#define DHCP4_RENEWAL_TIME                    58
#define DHCP4_REBINDING_TIME                  59
#define DHCP4_CLASS_IDENTIFIER                60
#define DHCP4_CLIENT_IDENTIFIER               61
#define DHCP4_NISPLUS_DOMAIN_NAME             64
#define DHCP4_NISPLUS_SERVERS                 65
#define DHCP4_TFTP_SERVER_NAME                66
#define DHCP4_BOOTFILE                        67
#define DHCP4_MOBILE_IP_HOME_AGENTS           68
#define DHCP4_SMPT_SERVERS                    69
#define DHCP4_POP3_SERVERS                    70
#define DHCP4_NNTP_SERVERS                    71
#define DHCP4_WWW_SERVERS                     72
#define DHCP4_FINGER_SERVERS                  73
#define DHCP4_IRC_SERVERS                     74
#define DHCP4_STREET_TALK_SERVERS             75
#define DHCP4_STREET_TALK_DIR_ASSIST_SERVERS  76
#define DHCP4_NDS_SERVERS                     85
#define DHCP4_NDS_TREE_NAME                   86
#define DHCP4_NDS_CONTEXT                     87
#define DHCP4_SYSTEM_ARCHITECTURE             93
#define DHCP4_NETWORK_ARCHITECTURE            94
#define DHCP4_PLATFORM_ID                     97

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//
// DHCP4 option format.
//

#pragma pack(1)
typedef struct {
  UINT8 op;
  UINT8 len;
  UINT8 data[1];
} DHCP4_OP;
#pragma pack()

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct {
  DHCP4_PACKET Discover;
  DHCP4_PACKET Offer;
  DHCP4_PACKET Request;
  DHCP4_PACKET AckNak;
  BOOLEAN SetupCompleted;
  BOOLEAN InitCompleted;
  BOOLEAN SelectCompleted;
  BOOLEAN IsBootp;
  BOOLEAN IsAck;
} EFI_PXE_DHCP4_DATA;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_RUN) (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN OPTIONAL UINTN         OpLen,
  IN OPTIONAL VOID          *OpList
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_SETUP) (
  IN EFI_PXE_DHCP4_PROTOCOL          *This,
  IN OPTIONAL EFI_PXE_DHCP4_DATA     * NewData
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_INIT) (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  SecondsTimeout,
  OUT UINTN                 *Offers,
  OUT DHCP4_PACKET          **OfferList
  );

#define DHCP4_MIN_SECONDS   1
#define DHCP4_MAX_SECONDS   60

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_SELECT) (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  SecondsTimeout,
  IN DHCP4_PACKET           * offer
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_RENEW) (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  UINTN                     seconds_timeout
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_REBIND) (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  UINTN                     seconds_timeout
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_DHCP4_RELEASE) (
  IN EFI_PXE_DHCP4_PROTOCOL * This
  );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define EFI_PXE_DHCP4_PROTOCOL_REVISION    0x00010000

struct _EFI_PXE_DHCP4_PROTOCOL {
  UINT64 Revision;
  EFI_PXE_DHCP4_RUN Run;
  EFI_PXE_DHCP4_SETUP Setup;
  EFI_PXE_DHCP4_INIT Init;
  EFI_PXE_DHCP4_SELECT Select;
  EFI_PXE_DHCP4_RENEW Renew;
  EFI_PXE_DHCP4_REBIND Rebind;
  EFI_PXE_DHCP4_RELEASE Release;
  EFI_PXE_DHCP4_DATA *Data;
};

//
//
//

extern EFI_GUID gEfiPxeDhcp4ProtocolGuid;

#endif /* _PXEDHCP4_H_ */
/* EOF - PxeDhcp4.h */
