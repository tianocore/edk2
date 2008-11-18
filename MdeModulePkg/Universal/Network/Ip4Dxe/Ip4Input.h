/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Ip4Input.h

Abstract:


**/

#ifndef __EFI_IP4_INPUT_H__
#define __EFI_IP4_INPUT_H__

typedef enum {
  IP4_MIN_HEADLEN       = 20,
  IP4_MAX_HEADLEN       = 60,

  IP4_ASSEMLE_HASH_SIZE = 31,
  IP4_FRAGMENT_LIFE     = 120,
  IP4_MAX_PACKET_SIZE   = 65535
} IP4_INPUT_ENUM_TYPES;

///
/// Per packet information for input process. LinkFlag specifies whether
/// the packet is received as Link layer unicast, multicast or broadcast.
/// The CastType is the IP layer cast type, such as IP multicast or unicast.
/// Start, End and Length are staffs used to assemble the packets. Start
/// is the sequence number of the first byte of data in the packet. Length
/// is the number of bytes of data. End = Start + Length, that is, the
/// sequence number of last byte + 1. Each assembled packet has a count down
/// life. If it isn't consumed before Life reaches zero, the packet is released.
///
typedef struct {
  UINTN                     LinkFlag;
  INTN                      CastType;
  INTN                      Start;
  INTN                      End;
  INTN                      Length;
  UINT32                    Life;
  EFI_STATUS                Status;
} IP4_CLIP_INFO;

///
/// Structure used to assemble IP packets.
///
typedef struct {
  LIST_ENTRY                Link;

  //
  // Identity of one IP4 packet. Each fragment of a packet has
  // the same (Dst, Src, Id, Protocol).
  //
  IP4_ADDR                  Dst;
  IP4_ADDR                  Src;
  UINT16                    Id;
  UINT8                     Protocol;

  INTN                      TotalLen;
  INTN                      CurLen;
  LIST_ENTRY                Fragments;  // List of all the fragments of this packet

  IP4_HEAD                  *Head;      // IP head of the first fragment
  IP4_CLIP_INFO             *Info;      // Per packet info of the first fragment
  INTN                      Life;       // Count down life for the packet.
} IP4_ASSEMBLE_ENTRY;

///
/// Each Ip service instance has an assemble table to reassemble
/// the packets before delivery to its children. It is organized
/// as hash table.
///
typedef struct {
  LIST_ENTRY      Bucket[IP4_ASSEMLE_HASH_SIZE];
} IP4_ASSEMBLE_TABLE;

#define IP4_GET_CLIP_INFO(Packet) ((IP4_CLIP_INFO *) ((Packet)->ProtoData))

#define IP4_ASSEMBLE_HASH(Dst, Src, Id, Proto)  \
          (((Dst) + (Src) + ((Id) << 16) + (Proto)) % IP4_ASSEMLE_HASH_SIZE)

#define IP4_RXDATA_WRAP_SIZE(NumFrag) \
          (sizeof (IP4_RXDATA_WRAP) + sizeof (EFI_IP4_FRAGMENT_DATA) * ((NumFrag) - 1))

/**
  Initialize an already allocated assemble table. This is generally
  the assemble table embedded in the IP4 service instance.

  @param  Table                  The assemble table to initialize.

  @return NONE

**/
VOID
Ip4InitAssembleTable (
  IN OUT IP4_ASSEMBLE_TABLE     *Table
  );

/**
  Clean up the assemble table: remove all the fragments
  and assemble entries.

  @param  Table                  The assemble table to clean up

  @return None

**/
VOID
Ip4CleanAssembleTable (
  IN IP4_ASSEMBLE_TABLE     *Table
  );

/**
  The IP4 input routine. It is called by the IP4_INTERFACE when a
  IP4 fragment is received from MNP.

  @param  Ip4Instance            The IP4 child that request the receive, most like
                                 it is NULL.
  @param  Packet                 The IP4 packet received.
  @param  IoStatus               The return status of receive request.
  @param  Flag                   The link layer flag for the packet received, such
                                 as multicast.
  @param  Context                The IP4 service instance that own the MNP.

  @return None

**/
VOID
Ip4AccpetFrame (
  IN IP4_PROTOCOL           *Ip4Instance,
  IN NET_BUF                *Packet,
  IN EFI_STATUS             IoStatus,
  IN UINT32                 Flag,
  IN VOID                   *Context
  );

/**
  Demultiple the packet. the packet delivery is processed in two
  passes. The first pass will enque a shared copy of the packet
  to each IP4 child that accepts the packet. The second pass will
  deliver a non-shared copy of the packet to each IP4 child that
  has pending receive requests. Data is copied if more than one
  child wants to consume the packet because each IP child needs
  its own copy of the packet to make changes.

  @param  IpSb                   The IP4 service instance that received the packet
  @param  Head                   The header of the received packet
  @param  Packet                 The data of the received packet

  @retval EFI_NOT_FOUND          No IP child accepts the packet
  @retval EFI_SUCCESS            The packet is enqueued or delivered to some IP
                                 children.

**/
EFI_STATUS
Ip4Demultiplex (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  );

/**
  Enqueue a received packet to all the IP children that share
  the same interface.

  @param  IpSb                   The IP4 service instance that receive the packet
  @param  Head                   The header of the received packet
  @param  Packet                 The data of the received packet
  @param  IpIf                   The interface to enqueue the packet to

  @return The number of the IP4 children that accepts the packet

**/
INTN
Ip4InterfaceEnquePacket (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet,
  IN IP4_INTERFACE          *IpIf
  );

/**
  Deliver the received packets to upper layer if there are both received
  requests and enqueued packets. If the enqueued packet is shared, it will
  duplicate it to a non-shared packet, release the shared packet, then
  deliver the non-shared packet up.

  @param  IpInstance             The IP child to deliver the packet up.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources to deliver the
                                 packets.
  @retval EFI_SUCCESS            All the enqueued packets that can be delivered
                                 are delivered up.

**/
EFI_STATUS
Ip4InstanceDeliverPacket (
  IN IP4_PROTOCOL           *IpInstance
  );

/**
  Timeout the fragment and enqueued packets.

  @param  IpSb                   The IP4 service instance to timeout

  @return None

**/
VOID
Ip4PacketTimerTicking (
  IN IP4_SERVICE            *IpSb
  );

#endif
