/** @file
  IP6 internal functions and definitions to process the incoming packets.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP6_INPUT_H__
#define __EFI_IP6_INPUT_H__

#define IP6_MIN_HEADLEN  40
#define IP6_MAX_HEADLEN  120
///
/// 8(ESP header) + 16(max IV) + 16(max padding) + 2(ESP tail) + 12(max ICV) = 54
///
#define IP6_MAX_IPSEC_HEADLEN  54

#define IP6_ASSEMLE_HASH_SIZE  127
///
/// Lift time in seconds.
///
#define IP6_FRAGMENT_LIFE    60
#define IP6_MAX_PACKET_SIZE  65535

#define IP6_GET_CLIP_INFO(Packet)  ((IP6_CLIP_INFO *) ((Packet)->ProtoData))

#define IP6_ASSEMBLE_HASH(Dst, Src, Id)  \
          ((*((UINT32 *) (Dst)) + *((UINT32 *) (Src)) + (Id)) % IP6_ASSEMLE_HASH_SIZE)

#define IP6_RXDATA_WRAP_SIZE(NumFrag) \
          (sizeof (IP6_RXDATA_WRAP) + sizeof (EFI_IP6_FRAGMENT_DATA) * ((NumFrag) - 1))

//
// Per packet information for input process. LinkFlag specifies whether
// the packet is received as Link layer unicast, multicast or broadcast.
// The CastType is the IP layer cast type, such as IP multicast or unicast.
// Start, End and Length are staffs used to assemble the packets. Start
// is the sequence number of the first byte of data in the packet. Length
// is the number of bytes of data. End = Start + Length, that is, the
// sequence number of last byte + 1. Each assembled packet has a count down
// life. If it isn't consumed before Life reaches zero, the packet is released.
//
typedef struct {
  UINT32        LinkFlag;
  INT32         CastType;
  INT32         Start;
  INT32         End;
  INT32         Length;
  UINT32        Life;
  EFI_STATUS    Status;
  UINT32        Id;
  UINT16        HeadLen;
  UINT8         NextHeader;
  UINT8         LastFrag;
  UINT32        FormerNextHeader;
} IP6_CLIP_INFO;

//
// Structure used to assemble IP packets.
//
typedef struct {
  LIST_ENTRY          Link;
  LIST_ENTRY          Fragments;        // List of all the fragments of this packet

  //
  // Identity of one IP6 packet. Each fragment of a packet has
  // the same (Dst, Src, Id).
  //
  EFI_IPv6_ADDRESS    Dst;
  EFI_IPv6_ADDRESS    Src;
  UINT32              Id;

  UINT32              TotalLen;
  UINT32              CurLen;
  UINT32              Life;             // Count down life for the packet.

  EFI_IP6_HEADER      *Head;            // IP head of the first fragment
  IP6_CLIP_INFO       *Info;            // Per packet information of the first fragment
  NET_BUF             *Packet;          // The first fragment of the packet
} IP6_ASSEMBLE_ENTRY;

//
// Each Ip service instance has an assemble table to reassemble
// the packets before delivery to its children. It is organized
// as hash table.
//
typedef struct {
  LIST_ENTRY    Bucket[IP6_ASSEMLE_HASH_SIZE];
} IP6_ASSEMBLE_TABLE;

/**
  The IP6 input routine. It is called by the IP6_INTERFACE when an
  IP6 fragment is received from MNP.

  @param[in]  Packet             The IP6 packet received.
  @param[in]  IoStatus           The return status of receive request.
  @param[in]  Flag               The link layer flag for the packet received, such
                                 as multicast.
  @param[in]  Context            The IP6 service instance that own the MNP.

**/
VOID
Ip6AcceptFrame (
  IN NET_BUF     *Packet,
  IN EFI_STATUS  IoStatus,
  IN UINT32      Flag,
  IN VOID        *Context
  );

/**
  Deliver the received packets to upper layer if there are both received
  requests and enqueued packets. If the enqueued packet is shared, it will
  duplicate it to a non-shared packet, release the shared packet, then
  deliver the non-shared packet up.

  @param[in]  IpInstance         The IP child to deliver the packet up.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources to deliver the
                                 packets.
  @retval EFI_SUCCESS            All the enqueued packets that can be delivered
                                 are delivered up.

**/
EFI_STATUS
Ip6InstanceDeliverPacket (
  IN IP6_PROTOCOL  *IpInstance
  );

/**
  The work function to locate the IPsec protocol to process the inbound or
  outbound IP packets. The process routine handles the packet with the following
  actions: bypass the packet, discard the packet, or protect the packet.

  @param[in]       IpSb          The IP6 service instance.
  @param[in, out]  Head          The caller-supplied IP6 header.
  @param[in, out]  LastHead      The next header field of last IP header.
  @param[in, out]  Netbuf        The IP6 packet to be processed by IPsec.
  @param[in, out]  ExtHdrs       The caller-supplied options.
  @param[in, out]  ExtHdrsLen    The length of the option.
  @param[in]       Direction     The directionality in an SPD entry,
                                 EfiIPsecInBound, or EfiIPsecOutBound.
  @param[in]       Context       The token's wrap.

  @retval EFI_SUCCESS            The IPsec protocol is not available or disabled.
  @retval EFI_SUCCESS            The packet was bypassed, and all buffers remain the same.
  @retval EFI_SUCCESS            The packet was protected.
  @retval EFI_ACCESS_DENIED      The packet was discarded.
  @retval EFI_OUT_OF_RESOURCES   There are not sufficient resources to complete the operation.
  @retval EFI_BUFFER_TOO_SMALL   The number of non-empty blocks is bigger than the
                                 number of input data blocks when building a fragment table.

**/
EFI_STATUS
Ip6IpSecProcessPacket (
  IN     IP6_SERVICE            *IpSb,
  IN OUT EFI_IP6_HEADER         **Head,
  IN OUT UINT8                  *LastHead,
  IN OUT NET_BUF                **Netbuf,
  IN OUT UINT8                  **ExtHdrs,
  IN OUT UINT32                 *ExtHdrsLen,
  IN     EFI_IPSEC_TRAFFIC_DIR  Direction,
  IN     VOID                   *Context
  );

/**
  Initialize an already allocated assemble table. This is generally
  the assemble table embedded in the IP6 service instance.

  @param[in, out]  Table    The assemble table to initialize.

**/
VOID
Ip6CreateAssembleTable (
  IN OUT IP6_ASSEMBLE_TABLE  *Table
  );

/**
  Clean up the assemble table: remove all the fragments
  and assemble entries.

  @param[in, out]  Table    The assemble table to clean up.

**/
VOID
Ip6CleanAssembleTable (
  IN OUT IP6_ASSEMBLE_TABLE  *Table
  );

/**
  Demultiple the packet. the packet delivery is processed in two
  passes. The first pass will enqueue a shared copy of the packet
  to each IP6 child that accepts the packet. The second pass will
  deliver a non-shared copy of the packet to each IP6 child that
  has pending receive requests. Data is copied if more than one
  child wants to consume the packet because each IP child need
  its own copy of the packet to make changes.

  @param[in]  IpSb          The IP6 service instance that received the packet.
  @param[in]  Head          The header of the received packet.
  @param[in]  Packet        The data of the received packet.

  @retval EFI_NOT_FOUND     No IP child accepts the packet.
  @retval EFI_SUCCESS       The packet is enqueued or delivered to some IP
                            children.

**/
EFI_STATUS
Ip6Demultiplex (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  );

/**
  Timeout the fragmented, enqueued, and transmitted packets.

  @param[in]  IpSb          The IP6 service instance to timeout.

**/
VOID
Ip6PacketTimerTicking (
  IN IP6_SERVICE  *IpSb
  );

#endif
