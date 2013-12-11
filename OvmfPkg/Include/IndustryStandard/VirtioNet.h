/** @file
  Virtio Network Device specific type and macro definitions corresponding to
  the virtio-0.9.5 specification.

  Copyright (C) 2013, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

#include <IndustryStandard/Virtio.h>

//
// virtio-0.9.5, Appendix C: Network Device
//
#pragma pack(1)
typedef struct {
  UINT8      Mac[6];
  UINT16     LinkStatus;
} VIRTIO_NET_CONFIG;
#pragma pack()

#define OFFSET_OF_VNET(Field) OFFSET_OF (VIRTIO_NET_CONFIG, Field)
#define SIZE_OF_VNET(Field)   (sizeof ((VIRTIO_NET_CONFIG *) 0)->Field)

//
// Queue Identifiers
//
#define VIRTIO_NET_Q_RX 0
#define VIRTIO_NET_Q_TX 1

//
// Feature Bits
//
#define VIRTIO_NET_F_CSUM           BIT0  // host to checksum outgoing packets
#define VIRTIO_NET_F_GUEST_CSUM     BIT1  // guest to checksum incoming packets
#define VIRTIO_NET_F_MAC            BIT5  // MAC available to guest
#define VIRTIO_NET_F_GSO            BIT6  // deprecated
#define VIRTIO_NET_F_GUEST_TSO4     BIT7  // guest can receive TSOv4
#define VIRTIO_NET_F_GUEST_TSO6     BIT8  // guest can receive TSOv6
#define VIRTIO_NET_F_GUEST_ECN      BIT9  // guest can receive TSO with ECN
#define VIRTIO_NET_F_GUEST_UFO      BIT10 // guest can receive UFO
#define VIRTIO_NET_F_HOST_TSO4      BIT11 // host can receive TSOv4
#define VIRTIO_NET_F_HOST_TSO6      BIT12 // host can receive TSOv6
#define VIRTIO_NET_F_HOST_ECN       BIT13 // host can receive TSO with ECN
#define VIRTIO_NET_F_HOST_UFO       BIT14 // host can receive UFO
#define VIRTIO_NET_F_MRG_RXBUF      BIT15 // guest can merge receive buffers
#define VIRTIO_NET_F_STATUS         BIT16 // link status available to guest
#define VIRTIO_NET_F_CTRL_VQ        BIT17 // control channel available
#define VIRTIO_NET_F_CTRL_RX        BIT18 // control channel RX mode support
#define VIRTIO_NET_F_CTRL_VLAN      BIT19 // control channel VLAN filtering
#define VIRTIO_NET_F_GUEST_ANNOUNCE BIT21 // guest can send gratuitous pkts

//
// Packet Header
//
#pragma pack(1)
typedef struct {
  UINT8  Flags;
  UINT8  GsoType;
  UINT16 HdrLen;
  UINT16 GsoSize;
  UINT16 CsumStart;
  UINT16 CsumOffset;
} VIRTIO_NET_REQ;
#pragma pack()

//
// Bits in VIRTIO_NET_REQ.Flags
//
#define VIRTIO_NET_HDR_F_NEEDS_CSUM BIT0

//
// Types/Bits for VIRTIO_NET_REQ.GsoType
//
#define VIRTIO_NET_HDR_GSO_NONE  0x00
#define VIRTIO_NET_HDR_GSO_TCPV4 0x01
#define VIRTIO_NET_HDR_GSO_UDP   0x03
#define VIRTIO_NET_HDR_GSO_TCPV6 0x04
#define VIRTIO_NET_HDR_GSO_ECN   BIT7

//
// Link Status Bits in VIRTIO_NET_CONFIG.LinkStatus
//
#define VIRTIO_NET_S_LINK_UP  BIT0
#define VIRTIO_NET_S_ANNOUNCE BIT1

#endif // _VIRTIO_NET_H_
