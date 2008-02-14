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

enum {
  IP4_MIN_HEADLEN       = 20,
  IP4_MAX_HEADLEN       = 60,

  IP4_ASSEMLE_HASH_SIZE = 31,
  IP4_FRAGMENT_LIFE     = 120,
  IP4_MAX_PACKET_SIZE   = 65535
};

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
  UINTN                     LinkFlag;
  INTN                      CastType;
  INTN                      Start;
  INTN                      End;
  INTN                      Length;
  UINT32                    Life;
  EFI_STATUS                Status;
} IP4_CLIP_INFO;

//
// Structure used to assemble IP packets.
//
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

//
// Each Ip service instance has an assemble table to reassemble
// the packets before delivery to its children. It is organized
// as hash table.
//
typedef struct {
  LIST_ENTRY      Bucket[IP4_ASSEMLE_HASH_SIZE];
} IP4_ASSEMBLE_TABLE;

#define IP4_GET_CLIP_INFO(Packet) ((IP4_CLIP_INFO *) ((Packet)->ProtoData))

#define IP4_ASSEMBLE_HASH(Dst, Src, Id, Proto)  \
          (((Dst) + (Src) + ((Id) << 16) + (Proto)) % IP4_ASSEMLE_HASH_SIZE)

#define IP4_RXDATA_WRAP_SIZE(NumFrag) \
          (sizeof (IP4_RXDATA_WRAP) + sizeof (EFI_IP4_FRAGMENT_DATA) * ((NumFrag) - 1))

VOID
Ip4InitAssembleTable (
  IN IP4_ASSEMBLE_TABLE     *Table
  );

VOID
Ip4CleanAssembleTable (
  IN IP4_ASSEMBLE_TABLE     *Table
  );

VOID
Ip4AccpetFrame (
  IN IP4_PROTOCOL           *Ip4Instance,
  IN NET_BUF                *Packet,
  IN EFI_STATUS             IoStatus,
  IN UINT32                 Flag,
  IN VOID                   *Context
  );

EFI_STATUS
Ip4Demultiplex (
  IN IP4_SERVICE            *SbInstance,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  );

INTN
Ip4InterfaceEnquePacket (
  IN IP4_SERVICE            *SbInstance,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet,
  IN IP4_INTERFACE          *Interface
  );

EFI_STATUS
Ip4InstanceDeliverPacket (
  IN IP4_PROTOCOL           *Ip4Instance
  );

VOID
Ip4PacketTimerTicking (
  IN IP4_SERVICE            *IpSb
  );

#endif
