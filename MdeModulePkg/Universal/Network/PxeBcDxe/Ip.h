/** @file

Copyright (c) 2004 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _IP_H_
#define _IP_H_

#include "Hton.h"

//
// portability macros
//
#define UDP_FILTER_MASK  (EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP | \
                         EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT | \
                         EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP | \
                         EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT | \
                         EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER \
          )

#define PXE_BOOT_LAYER_MASK             0x7FFF
#define PXE_BOOT_LAYER_INITIAL          0x0000
#define PXE_BOOT_LAYER_CREDENTIAL_FLAG  0x8000
#define MAX_BOOT_SERVERS                32

//
// macro to evaluate IP address as TRUE if it is a multicast IP address
//
#define IS_MULTICAST(ptr) ((*((UINT8 *) ptr) & 0xf0) == 0xe0)

//
// length macros
//
#define IP_ADDRESS_LENGTH(qp)   (((qp)->UsingIpv6) ? sizeof (EFI_IPv6_ADDRESS) : sizeof (EFI_IPv4_ADDRESS))

#define MAX_FRAME_DATA_SIZE     1488
#define ALLOCATE_SIZE(X)        (((X) + 7) & 0xfff8)
#define MODE_ALLOCATE_SIZE      ALLOCATE_SIZE (sizeof (EFI_PXE_BASE_CODE_MODE))
#define BUFFER_ALLOCATE_SIZE    (8192 + 512)
#define ROUTER_ALLOCATE_SIZE    ALLOCATE_SIZE ((sizeof (EFI_PXE_BASE_CODE_ROUTE_ENTRY) * PXE_ROUTER_TABLE_SIZE))
#define ARP_ALLOCATE_SIZE       ALLOCATE_SIZE ((sizeof (EFI_PXE_BASE_CODE_ARP_ENTRY) * PXE_ARP_CACHE_SIZE))
#define FILTER_ALLOCATE_SIZE    ALLOCATE_SIZE ((sizeof (EFI_IP_ADDRESS) * PXE_IP_FILTER_SIZE))
#define PXE_ARP_CACHE_SIZE      8
#define PXE_ROUTER_TABLE_SIZE   8
#define PXE_IP_FILTER_SIZE      8
#define ICMP_ALLOCATE_SIZE      ALLOCATE_SIZE (sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR))
#define TFTP_ERR_ALLOCATE_SIZE  ALLOCATE_SIZE (sizeof (EFI_PXE_BASE_CODE_TFTP_ERROR))

//
// DHCP discover/request packets are sent to this UDP port.  ProxyDHCP
// servers listen on this port for DHCP discover packets that have a
// class identifier (option 60) with 'PXEClient' in the first 9 bytes.
// Bootservers also listen on this port for PXE broadcast discover
// requests from PXE clients.
//
#define DHCP_SERVER_PORT  67

//
// When DHCP, proxyDHCP and Bootservers respond to DHCP and PXE broadcast
// discover requests by broadcasting the reply packet, the packet is
// broadcast to this port.
//
#define DHCP_CLIENT_PORT  68

//
// TFTP servers listen for TFTP open requests on this port.
//
#define TFTP_OPEN_PORT  69

//
// proxyDHCP and Bootservers listen on this port for a PXE unicast and/or
// multicast discover requests from PXE clients.  A PXE discover request
// looks like a DHCP discover or DHCP request packet.
//
#define PXE_DISCOVERY_PORT  4011

//
// This port is used by the PXE client/server protocol tests.
//
#define PXE_PORT_PXETEST_PORT 0x8080

//
// Definitions for Ethertype protocol numbers and interface types
// Per RFC 1700,
//
#define PXE_PROTOCOL_ETHERNET_IP    0x0800
#define PXE_PROTOCOL_ETHERNET_ARP   0x0806
#define PXE_PROTOCOL_ETHERNET_RARP  0x8035

#define PXE_IFTYPE_ETHERNET         0x01
#define PXE_IFTYPE_TOKENRING        0x04
#define PXE_IFTYPE_FIBRE_CHANNEL    0x12

//
// Definitions for internet protocol version 4 header
// Per RFC 791, September 1981.
//
#define IPVER4  4

#pragma pack(1) // make network structures packed byte alignment
typedef union {
  UINT8   B[4];
  UINT32  L;
} IPV4_ADDR;

#define IPV4_HEADER_LENGTH(IpHeaderPtr) (((IpHeaderPtr)->VersionIhl & 0xf) << 2)

#define SET_IPV4_VER_HDL(IpHeaderPtr, IpHeaderLen) { \
    (IpHeaderPtr)->VersionIhl = (UINT8) ((IPVER4 << 4) | ((IpHeaderLen) >> 2)); \
  }

typedef struct {
  UINT8     VersionIhl;
  UINT8     TypeOfService;
  UINT16    TotalLength;
  UINT16    Id;
  UINT16    FragmentFields;
  UINT8     TimeToLive;
  UINT8     Protocol;
  UINT16    HeaderChecksum;
  IPV4_ADDR SrcAddr;
  IPV4_ADDR DestAddr;
  //
  // options are not implemented
  //
} IPV4_HEADER;

#define IP_FRAG_RSVD    0x8000  // reserved bit - must be zero
#define IP_NO_FRAG      0x4000  // do not fragment bit
#define IP_MORE_FRAG    0x2000  // not last fragment
#define IP_FRAG_OFF_MSK 0x1fff  // fragment offset in 8 byte chunks
#define DEFAULT_RFC_TTL 64

#define PROT_ICMP       1
#define PROT_IGMP       2
#define PROT_TCP        6
#define PROT_UDP        17

/*
 * Definitions for internet control message protocol version 4 message
 * structure.  Per RFC 792, September 1981.
 */

//
// icmp header for all icmp messages
//
typedef struct {
  UINT8   Type;     // message type
  UINT8   Code;     // type specific - 0 for types we implement
  UINT16  Checksum; // ones complement of ones complement sum of 16 bit words of message
} ICMPV4_HEADER;

#define ICMP_DEST_UNREACHABLE   3
#define ICMP_SOURCE_QUENCH      4
#define ICMP_REDIRECT           5
#define ICMP_ECHO               8
#define ICMP_ECHO_REPLY         0
#define ICMP_ROUTER_ADV         9
#define ICMP_ROUTER_SOLICIT     10
#define ICMP_TIME_EXCEEDED      11
#define ICMP_PARAMETER_PROBLEM  12
#define ICMP_TIMESTAMP          13
#define ICMP_TIMESTAMP_REPLY    14
#define ICMP_INFO_REQ           15
#define ICMP_INFO_REQ_REPLY     16
#define ICMP_SUBNET_MASK_REQ    17
#define ICMP_SUBNET_MASK_REPLY  18
//
// other ICMP message types ignored in this implementation
//
// icmp general messages
//
typedef struct {
  ICMPV4_HEADER Header;
  //
  // generally unused except byte [0] for
  // parameter problem message
  //
  UINT8         GenerallyUnused[4];
  //
  // original message ip header of plus 64
  // bits of data
  //
  IPV4_HEADER   IpHeader;
} ICMPV4_GENERAL_MESSAGE;

//
// icmp req/rply message header
//
typedef struct {
  ICMPV4_HEADER Header;
  UINT16        Id;
  UINT16        SequenceNumber;
} ICMPV4_REQUEST_REPLY_HEADER;

//
// icmp echo message
//
typedef struct {
  ICMPV4_REQUEST_REPLY_HEADER Header;
  UINT8                       EchoData[1];  // variable length data to be echoed
} ICMPV4_ECHO_MESSAGE;

//
// icmp timestamp message - times are milliseconds since midnight UT -
// if non std, set high order bit
//
typedef struct {
  ICMPV4_REQUEST_REPLY_HEADER Header;
  UINT32                      OriginalTime; // originating timestamp
  UINT32                      ReceiveTime;  // receiving timestamp
  UINT32                      TransmitTime; // transmitting timestamp
} ICMPV4_TIMESTAMP_MESSAGE;

//
// icmp info request structure - fill in source and dest net ip address on reply
//
typedef struct {
  ICMPV4_REQUEST_REPLY_HEADER Header;
} ICMPV4_INFO_MESSAGE;

//
// Definitions for internet control message protocol version 4 message structure
// Router discovery
// Per RFC 1256, September 1991.
//
//
// icmp router advertisement message
//
typedef struct {
  ICMPV4_HEADER Header;
  UINT8         NumberEntries;  // number of address entries
  UINT8         EntrySize;      // number of 32 bit words per address entry
  UINT16        Lifetime;       // seconds to consider info valid
  UINT32        RouterIp;
  UINT32        Preferance;
} ICMPV4_ROUTER_ADVERTISE_MESSAGE;

//
// icmp router solicitation message
//
typedef struct {
  ICMPV4_HEADER Header;
  UINT32        Reserved;
} ICMPV4_ROUTER_SOLICIT_MESSAGE;

#define MAX_SOLICITATION_DELAY      1   //  1 second
#define SOLICITATION_INTERVAL       3   //  3 seconds
#define MAX_SOLICITATIONS           3   //  3 transmissions
#define V1ROUTER_PRESENT_TIMEOUT    400 // 400 second timeout until v2 reports can be sent
#define UNSOLICITED_REPORT_INTERVAL 10  // 10 seconds between unsolicited reports
#define BROADCAST_IPv4              0xffffffff

//
// Definitions for address resolution protocol message structure
// Per RFC 826, November 1982
//
typedef struct {
  UINT16  HwType;     // hardware type - e.g. ethernet (1)
  UINT16  ProtType;   // protocol type - for ethernet, 0x800 for IP
  UINT8   HwAddLen;   // byte length of a hardware address (e.g. 6 for ethernet)
  UINT8   ProtAddLen; // byte length of a protocol address (e.g. 4 for ipv4)
  UINT16  OpCode;
  //
  // source and dest hw and prot addresses follow - see example below
  //
} ARP_HEADER;

#define ETHERNET_ADD_SPC  1

#define ETHER_TYPE_IP     0x800

#define ARP_REQUEST       1
#define ARP_REPLY         2

//
// generic ARP packet
//
typedef struct {
  ARP_HEADER      ArpHeader;
  EFI_MAC_ADDRESS SrcHardwareAddr;
  EFI_IP_ADDRESS  SrcProtocolAddr;
  EFI_MAC_ADDRESS DestHardwareAddr;
  EFI_IP_ADDRESS  DestProtocolAddr;
} ARP_PACKET;

#define ENET_HWADDLEN   6
#define IPV4_PROTADDLEN 4

//
// Definitions for user datagram protocol version 4 pseudo header & header
// Per RFC 768, 28 August 1980
//
typedef struct {
  IPV4_ADDR SrcAddr;      // source ip address
  IPV4_ADDR DestAddr;     // dest ip address
  UINT8     Zero;         // 0
  UINT8     Protocol;     // protocol
  UINT16    TotalLength;  // UDP length - sizeof udpv4hdr + data length
} UDPV4_PSEUDO_HEADER;

typedef struct {
  UINT16  SrcPort;        // source port identifier
  UINT16  DestPort;       // destination port identifier
  UINT16  TotalLength;    // total length header plus data
  //
  // ones complement of ones complement sum of 16 bit
  // words of pseudo header, UDP header, and data
  // zero checksum is transmitted as -0 (ones comp)
  // zero transmitted means checksum not computed
  // data follows
  //
  UINT16  Checksum;
} UDPV4_HEADER;

typedef struct {
  UDPV4_PSEUDO_HEADER Udpv4PseudoHeader;
  UDPV4_HEADER        Udpv4Header;
} UDPV4_HEADERS;

//
// Definitions for transmission control protocol header
// Per RFC 793, September, 1981
//
typedef struct {
  IPV4_ADDR SrcAddr;      // source ip address
  IPV4_ADDR DestAddr;     // dest ip address
  UINT8     Zero;         // 0
  UINT8     Protocol;     // protocol
  UINT16    TotalLength;  // TCP length - TCP header length + data length
} TCPV4_PSEUDO_HEADER;

typedef struct {
  UINT16  SrcPort;        // source port identifier
  UINT16  DestPort;       // destination port identifier
  UINT32  SeqNumber;      // Sequence number
  UINT32  AckNumber;      // Acknowledgement Number
  //
  // Nibble of HLEN (length of header in 32-bit multiples)
  // 6bits of RESERVED
  // Nibble of Code Bits
  //
  UINT16  HlenResCode;
  UINT16  Window;   // Software buffer size (sliding window size) in network-standard byte order
  //
  // ones complement of ones complement sum of 16 bit words of
  // pseudo header, TCP header, and data
  // zero checksum is transmitted as -0 (ones comp)
  // zero transmitted means checksum not computed
  //
  UINT16  Checksum;
  UINT16  UrgentPointer;                // pointer to urgent data (allows sender to specify urgent data)
} TCPV4_HEADER;

typedef struct {
  TCPV4_PSEUDO_HEADER Tcpv4PseudoHeader;
  TCPV4_HEADER        Tcpv4Header;
} TCPV4_HEADERS;

typedef struct {
  UINT8 Kind;                           // one of the following:
  UINT8 Length;                         // total option length including Kind and Lth
  UINT8 Data[1];                        // length = Lth - 2
} TCPV4_OPTION;

#define TCP_OP_END                0     // only used to pad to end of TCP header
#define TCP_NOP                   1     // optional - may be used to pad between options to get alignment
#define TCP_MAX_SEG               2     // maximum receive segment size - only send at initial connection request
#define MAX_MEDIA_HDR_SIZE        64
#define MIN_ENET_DATA_SIZE        64
#define MAX_ENET_DATA_SIZE        1500  // temp def - make a network based var
#define MAX_IPV4_PKT_SIZE         65535 // maximum IP packet size
#define MAX_IPV4_DATA_SIZE        (MAX_IPV4_PKT_SIZE - sizeof (IPV4_HEADER))
#define MAX_IPV4_FRAME_DATA_SIZE  (MAX_FRAME_DATA_SIZE - sizeof (IPV4_HEADER))
#define REAS_IPV4_PKT_SIZE        576   // minimum IP packet size all IP host can handle
#define REAS_IPV4_DATA_SIZE       (REAS_IPV4_PKT_SIZE - sizeof (IPV4_HEADER))

//
//
//
typedef union {
  UINT8           Data[MAX_ENET_DATA_SIZE];
  ICMPV4_HEADER   IcmpHeader;
  IGMPV2_MESSAGE  IgmpMessage;
  struct {
    UDPV4_HEADER  UdpHeader;
    UINT8         Data[1];
  } Udp;
  struct {
    TCPV4_HEADER  TcpHeader;
    UINT8         Data[1];
  } Tcp;
} PROTOCOL_UNION;

//
// out buffer structure
//
typedef struct {
  UINT8           MediaHeader[MAX_MEDIA_HDR_SIZE];
  IPV4_HEADER     IpHeader;
  //
  // following union placement only valid if no option IP header
  //
  PROTOCOL_UNION  u;
} IPV4_BUFFER;

typedef struct {
  IPV4_HEADER     IpHeader;
  //
  // following union placement only valid if no option IP header
  //
  PROTOCOL_UNION  u;
} IPV4_STRUCT;

#pragma pack()  // reset to default

  ////////////////////////////////////////////////////////////
//
//  BC IP Filter Routine
//
EFI_STATUS
IpFilter (
  PXE_BASECODE_DEVICE            *Private,
  IN EFI_PXE_BASE_CODE_IP_FILTER *Filter
  )
;

//
// //////////////////////////////////////////////////////////////////////
//
//  Udp Write Routine - called by base code - e.g. TFTP - already locked
//
EFI_STATUS
UdpWrite (
  IN PXE_BASECODE_DEVICE                      *Private,
  IN UINT16                                   OpFlags,
  IN EFI_IP_ADDRESS                           *DestIpPtr,
  IN EFI_PXE_BASE_CODE_UDP_PORT               *DestPortptr,
  IN EFI_IP_ADDRESS                           *GatewayIpPtr, OPTIONAL
  IN EFI_IP_ADDRESS                           *SrcIpPtr, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT           *SrcPortPtr, OPTIONAL
  IN UINTN                                    *HeaderSizePtr, OPTIONAL
  IN VOID                                     *HeaderPtr, OPTIONAL
  IN UINTN                                    *BufferSizePtr,
  IN VOID                                     *BufferPtr
  )
;

//
// /////////////////////////////////////////////////////////////////////
//
//  Udp Read Routine - called by base code - e.g. TFTP - already locked
//
EFI_STATUS
UdpRead (
  IN PXE_BASECODE_DEVICE            *Private,
  IN UINT16                         OpFlags,
  IN OUT EFI_IP_ADDRESS             *DestIpPtr, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT *DestPorPtrt, OPTIONAL
  IN OUT EFI_IP_ADDRESS             *SrcIpPtr, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT *SrcPortPtr, OPTIONAL
  IN UINTN                          *HeaderSizePtr, OPTIONAL
  IN VOID                           *HeaderPtr, OPTIONAL
  IN OUT UINTN                      *BufferSizePtr,
  IN VOID                           *BufferPtr,
  IN EFI_EVENT                      TimeoutEvent
  )
;

VOID
IgmpLeaveGroup (
  PXE_BASECODE_DEVICE *Private,
  EFI_IP_ADDRESS      *
  )
;

VOID
IgmpJoinGroup (
  PXE_BASECODE_DEVICE *Private,
  EFI_IP_ADDRESS      *
  )
;

//
// convert number to zero filled ascii value of length lth
//
VOID
CvtNum (
  UINTN Number,
  UINT8 *BufferPtr,
  INTN  BufferLen
  )
;

//
// convert number to ascii string at ptr
//
VOID
UtoA10 (
  UINTN Number,
  UINT8 *BufferPtr
  )
;

//
// convert ascii numeric string to UINTN
//
UINTN
AtoU (
  UINT8 *BufferPtr
  )
;

UINT64
AtoU64 (
  UINT8 *BufferPtr
  )
;

//
// calculate the internet checksum (RFC 1071)
// return 16 bit ones complement of ones complement sum of 16 bit words
//
UINT16
IpChecksum (
  UINT16 *MessagePtr,
  UINTN  ByteLength
  )
;

//
// do checksum on non contiguous header and data
//
UINT16
IpChecksum2 (
  UINT16 *Header,
  UINTN  HeaderLength,
  UINT16 *Message,
  UINTN  MessageLength
  )
;

//
// update checksum when only a single word changes
//
UINT16
UpdateChecksum (
  UINT16 OldChecksum,
  UINT16 OldWord,
  UINT16 NewWord
  )
;

VOID
SeedRandom (
  IN PXE_BASECODE_DEVICE  *Private,
  IN UINT16               InitialSeed
  )
;

UINT16
Random (
  IN PXE_BASECODE_DEVICE  *Private
  )
;

EFI_STATUS
SendPacket (
  PXE_BASECODE_DEVICE           *Private,
  VOID                          *HeaderPtr,
  VOID                          *PacketPtr,
  INTN                          PacketLength,
  VOID                          *HardwareAddress,
  UINT16                        MediaProtocol,
  IN EFI_PXE_BASE_CODE_FUNCTION Function
  )
;

VOID
HandleArpReceive (
  PXE_BASECODE_DEVICE *Private,
  ARP_PACKET          *ArpPacketPtr,
  VOID                *HeaderPtr
  )
;

VOID
HandleIgmp (
  PXE_BASECODE_DEVICE *Private,
  IGMPV2_MESSAGE      *IgmpMessageptr,
  UINTN               IgmpMessageLen
  )
;

VOID
IgmpCheckTimers (
  PXE_BASECODE_DEVICE *Private
  )
;  // poll when doing a receive
// return hw add of IP and TRUE if available, otherwise FALSE
//
BOOLEAN
GetHwAddr (
  IN PXE_BASECODE_DEVICE  *Private,
  EFI_IP_ADDRESS          *ProtocolAddressPtr,
  EFI_MAC_ADDRESS         *HardwareAddressPtr
  )
;

EFI_STATUS
DoArp (
  IN PXE_BASECODE_DEVICE  *Private,
  IN EFI_IP_ADDRESS       *ProtocolAddressPtr,
  OUT EFI_MAC_ADDRESS     *HardwareAddressptr
  )
;

BOOLEAN
OnSameSubnet (
  UINTN           IpAddressLen,
  EFI_IP_ADDRESS  *Ip1,
  EFI_IP_ADDRESS  *Ip2,
  EFI_IP_ADDRESS  *SubnetMask
  )
;

VOID
IpAddRouter (
  PXE_BASECODE_DEVICE *Private,
  EFI_IP_ADDRESS      *RouterIp
  )
;

#define Ip4AddRouter(Private, Ipv4Ptr)  IpAddRouter (Private, (EFI_IP_ADDRESS *) Ipv4Ptr)

//
// routine to send ipv4 packet
// ipv4 + upper protocol header for length TotHdrLth in xmtbuf, ipv4 header length IpHdrLth
// routine fills in ipv4hdr Ver_Hdl, TotLth, and Checksum, moves in Data, and gets dest MAC address
//
EFI_STATUS
Ipv4Xmt (
  PXE_BASECODE_DEVICE         *Private,
  UINT32                      GatewayIP,
  UINTN                       IpHeaderLen,
  UINTN                       TotalHeaderLen,
  VOID                        *Data,
  UINTN                       DataLen,
  EFI_PXE_BASE_CODE_FUNCTION  Function
  )
;

//
// send ipv4 packet with ipv4 option
//
EFI_STATUS
Ipv4SendWOp (
  PXE_BASECODE_DEVICE         *Private,
  UINT32                      GatewayIP,
  UINT8                       *MessagePtr,
  UINTN                       MessageLth,
  UINT8                       Protocol,
  UINT8                       *Option,
  UINTN                       OptionLen,
  UINT32                      DestIp,
  EFI_PXE_BASE_CODE_FUNCTION  Function
  )
;

//
// send MsgLth message at MsgPtr - higher level protocol header already in xmtbuf, length HdrSize
//
EFI_STATUS
Ip4Send (
  IN PXE_BASECODE_DEVICE  *Private,     // pointer to instance data
  IN UINTN                MayFragment,  //
  IN UINT8                Protocol,     // protocol
  IN UINT32               SrcIp,        // Source IP address
  IN UINT32               DestIp,       // Destination IP address
  IN UINT32               GatewayIp,    // used if not NULL and needed
  IN UINTN                HeaderSize,   // protocol header byte length
  IN UINT8                *MsgPtr,      // pointer to data
  IN UINTN                MsgLength
  )
;                                    // data byte length
// receive up to MsgLth message into MsgPtr for protocol Prot
// return message length, src/dest ips if select any, and pointer to protocol header
//
EFI_STATUS
IpReceive (
  IN PXE_BASECODE_DEVICE    *Private,   // pointer to instance data
  UINT16                    OpFlags,    // Flags to determine if filtering on IP addresses
  EFI_IP_ADDRESS            *SrcIpPtr,  // if filtering, O if accept any
  EFI_IP_ADDRESS            *DstIpPtr,  // if filtering, O if accept any
  UINT8                     Protocol,   // protocol
  VOID                      *HeaderPtr, // address of where to put protocol header
  UINTN                     HeaderSize, // protocol header byte length
  UINT8                     *MsgPtr,    // pointer to data buffer
  UINTN                     *MsgLenPtr, // pointer to data buffer length/ O - returned data length
  IN EFI_EVENT              TimeoutEvent
  )
;

#if 0
VOID
WaitForTxComplete (
  IN PXE_BASECODE_DEVICE    *Private
  )
;
#endif
//
// routine to cycle waiting for a receive or timeout
//
EFI_STATUS
WaitForReceive (
  IN PXE_BASECODE_DEVICE        *Private,
  IN EFI_PXE_BASE_CODE_FUNCTION Function,
  IN EFI_EVENT                  TimeoutEvent,
  IN OUT UINTN                  *HeaderSizePtr,
  IN OUT UINTN                  *BufferSizePtr,
  IN OUT UINT16                 *ProtocolPtr
  )
;

#endif /* _IP_H_ */

/* EOF - ip.h */
