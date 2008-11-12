/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4If.h

Abstract:

  Definition for IP4 pesudo interface structure.


**/

#ifndef __EFI_IP4_IF_H__
#define __EFI_IP4_IF_H__

typedef enum {
  IP4_FRAME_RX_SIGNATURE  = EFI_SIGNATURE_32 ('I', 'P', 'F', 'R'),
  IP4_FRAME_TX_SIGNATURE  = EFI_SIGNATURE_32 ('I', 'P', 'F', 'T'),
  IP4_FRAME_ARP_SIGNATURE = EFI_SIGNATURE_32 ('I', 'P', 'F', 'A'),
  IP4_INTERFACE_SIGNATURE = EFI_SIGNATURE_32 ('I', 'P', 'I', 'F')
} IP4_IF_ENUM_TYPES;

/**
  This prototype is used by both receive and transmission.
  When receiving Netbuf is allocated by IP4_INTERFACE, and
  released by IP4. Flag shows whether the frame is received
  as link broadcast/multicast...

  When transmitting, the Netbuf is from IP4, and provided
  to the callback as a reference. Flag isn't used.

  @param IpInstance The instance that sent or received the packet.
                    IpInstance can be NULL which means that it is the IP4 driver
                    itself sending the packets. IP4 driver may send packets that
                    don't belong to any instance, such as ICMP errors, ICMP echo
                    responses, or IGMP packets. IpInstance is used as a tag in
                    this module.
  @param Packet     The sent or received packet.
  @param IoStatus   Status of sending or receiving.
  @param LinkFlag   Indicate if the frame is received as link broadcast/multicast.
                    When transmitting, it is not used.
  @param Context    Additional data for callback.

  @return None.
**/
typedef
VOID
(*IP4_FRAME_CALLBACK)(
  IN IP4_PROTOCOL              *IpInstance,       OPTIONAL
  IN NET_BUF                   *Packet,
  IN EFI_STATUS                IoStatus,
  IN UINT32                    LinkFlag,
  IN VOID                      *Context
  );

///
/// Each receive request is wrapped in an IP4_LINK_RX_TOKEN.
/// Upon completion, the Callback will be called. Only one
/// receive request is send to MNP. IpInstance is always NULL.
/// Reference MNP's spec for information.
///
typedef struct {
  UINT32                                Signature;
  IP4_INTERFACE                         *Interface;

  IP4_PROTOCOL                          *IpInstance;
  IP4_FRAME_CALLBACK                    CallBack;
  VOID                                  *Context;

  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  MnpToken;
} IP4_LINK_RX_TOKEN;

///
/// Each transmit request is wrapped in an IP4_LINK_TX_TOKEN.
/// Upon completion, the Callback will be called.
///
typedef struct {
  UINT32                                Signature;
  LIST_ENTRY                            Link;

  IP4_INTERFACE                         *Interface;

  IP4_PROTOCOL                          *IpInstance;
  IP4_FRAME_CALLBACK                    CallBack;
  NET_BUF                               *Packet;
  VOID                                  *Context;

  EFI_MAC_ADDRESS                       DstMac;
  EFI_MAC_ADDRESS                       SrcMac;

  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  MnpToken;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     MnpTxData;
} IP4_LINK_TX_TOKEN;

///
/// Only one ARP request is requested for all the frames in
/// a time. It is started for the first frames to the Ip. Any
/// subsequent transmission frame will be linked to Frames, and
/// be sent all at once the ARP requests succeed.
///
typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;

  LIST_ENTRY              Frames;
  IP4_INTERFACE           *Interface;

  //
  // ARP requesting staffs
  //
  EFI_EVENT               OnResolved;
  IP4_ADDR                Ip;
  EFI_MAC_ADDRESS         Mac;
} IP4_ARP_QUE;

/**
  Callback to select which frame to cancel. Caller can cancel a
  single frame, or all the frame from an IP instance.

  @param Frame      The sending frame to check for cancellation.
  @param Context    Additional data for callback.

  @retval TRUE      The sending of the frame should be cancelled.
  @retval FALSE     Do not cancel the frame sending.
**/
typedef
BOOLEAN
(*IP4_FRAME_TO_CANCEL)(
  IP4_LINK_TX_TOKEN       *Frame,
  VOID                    *Context
  );

//
// Each IP4 instance has its own station address. All the instances
// with the same station address share a single interface structure.
// Each interface has its own ARP child, and shares one MNP child.
// Notice the special cases that DHCP can configure the interface
// with 0.0.0.0/0.0.0.0.
//
struct _IP4_INTERFACE {
  UINT32                        Signature;
  LIST_ENTRY                    Link;
  INTN                          RefCnt;

  //
  // IP address and subnet mask of the interface. It also contains
  // the subnet/net broadcast address for quick access. The fileds
  // are invalid if (Configured == FALSE)
  //
  IP4_ADDR                      Ip;
  IP4_ADDR                      SubnetMask;
  IP4_ADDR                      SubnetBrdcast;
  IP4_ADDR                      NetBrdcast;
  BOOLEAN                       Configured;

  //
  // Handle used to create/destory ARP child. All the IP children
  // share one MNP which is owned by IP service binding.
  //
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  EFI_ARP_PROTOCOL              *Arp;
  EFI_HANDLE                    ArpHandle;

  //
  // Queues to keep the frames sent and waiting ARP request.
  //
  LIST_ENTRY                    ArpQues;
  LIST_ENTRY                    SentFrames;
  IP4_LINK_RX_TOKEN             *RecvRequest;

  //
  // The interface's MAC and broadcast MAC address.
  //
  EFI_MAC_ADDRESS               Mac;
  EFI_MAC_ADDRESS               BroadcastMac;
  UINT32                        HwaddrLen;

  //
  // All the IP instances that have the same IP/SubnetMask are linked
  // together through IpInstances. If any of the instance enables
  // promiscuous receive, PromiscRecv is true.
  //
  LIST_ENTRY                    IpInstances;
  BOOLEAN                       PromiscRecv;
};

IP4_INTERFACE *
Ip4CreateInterface (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp,
  IN  EFI_HANDLE                    Controller,
  IN  EFI_HANDLE                    ImageHandle
  );

EFI_STATUS
Ip4SetAddress (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_ADDR              IpAddr,
  IN  IP4_ADDR              SubnetMask
  );

EFI_STATUS
Ip4FreeInterface (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance       OPTIONAL
  );

EFI_STATUS
Ip4SendFrame (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance,      OPTIONAL
  IN  NET_BUF               *Packet,
  IN  IP4_ADDR              NextHop,
  IN  IP4_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context
  );

VOID
Ip4CancelFrames (
  IN IP4_INTERFACE          *Interface,
  IN EFI_STATUS             IoStatus,
  IN IP4_FRAME_TO_CANCEL    FrameToCancel,   OPTIONAL
  IN VOID                   *Context
  );

VOID
Ip4CancelReceive (
  IN IP4_INTERFACE          *Interface
  );

EFI_STATUS
Ip4ReceiveFrame (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance,      OPTIONAL
  IN  IP4_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context
  );

#endif
