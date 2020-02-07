/** @file
  Definition for IP6 pseudo interface structure.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP6_IF_H__
#define __EFI_IP6_IF_H__

#define IP6_LINK_RX_SIGNATURE   SIGNATURE_32 ('I', 'P', '6', 'R')
#define IP6_LINK_TX_SIGNATURE   SIGNATURE_32 ('I', 'P', '6', 'T')
#define IP6_INTERFACE_SIGNATURE SIGNATURE_32 ('I', 'P', '6', 'I')
#define IP6_ADDR_INFO_SIGNATURE SIGNATURE_32 ('I', 'P', 'A', 'I')

//
// This prototype is used by both receive and transmission.
// When receiving Netbuf is allocated by IP6_INTERFACE, and
// released by IP6. Flag shows whether the frame is received
// as unicast/multicast/anycast...
//
// When transmitting, the Netbuf is from IP6, and provided
// to the callback as a reference. Flag isn't used.
//
// IpInstance can be NULL which means that it is the IP6 driver
// itself sending the packets. IP6 driver may send packets that
// don't belong to any instance, such as ICMP errors, ICMP
// informational packets. IpInstance is used as a tag in
// this module.
//
typedef
VOID
(*IP6_FRAME_CALLBACK) (
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    LinkFlag,
  VOID                      *Context
  );

//
// Each receive request is wrapped in an IP6_LINK_RX_TOKEN.
// Upon completion, the Callback will be called. Only one
// receive request is send to MNP. IpInstance is always NULL.
// Reference MNP's spec for information.
//
typedef struct {
  UINT32                                Signature;
  IP6_FRAME_CALLBACK                    CallBack;
  VOID                                  *Context;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  MnpToken;
} IP6_LINK_RX_TOKEN;

//
// Each transmit request is wrapped in an IP6_LINK_TX_TOKEN.
// Upon completion, the Callback will be called.
//
typedef struct {
  UINT32                                Signature;
  LIST_ENTRY                            Link;

  IP6_PROTOCOL                          *IpInstance;
  IP6_FRAME_CALLBACK                    CallBack;
  NET_BUF                               *Packet;
  VOID                                  *Context;

  EFI_MAC_ADDRESS                       DstMac;
  EFI_MAC_ADDRESS                       SrcMac;

  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  MnpToken;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     MnpTxData;
} IP6_LINK_TX_TOKEN;

struct _IP6_ADDRESS_INFO {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  EFI_IPv6_ADDRESS        Address;
  BOOLEAN                 IsAnycast;
  UINT8                   PrefixLength;
  UINT32                  ValidLifetime;
  UINT32                  PreferredLifetime;
};

//
// Callback to select which frame to cancel. Caller can cancel a
// single frame, or all the frame from an IP instance.
//
typedef
BOOLEAN
(*IP6_FRAME_TO_CANCEL) (
  IP6_LINK_TX_TOKEN       *Frame,
  VOID                    *Context
  );

struct _IP6_INTERFACE {
  UINT32                        Signature;
  LIST_ENTRY                    Link;
  INTN                          RefCnt;

  //
  // IP address and prefix length of the interface.  The fileds
  // are invalid if (Configured == FALSE)
  //
  LIST_ENTRY                    AddressList;
  UINT32                        AddressCount;
  BOOLEAN                       Configured;

  IP6_SERVICE                   *Service;

  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;


  //
  // Queues to keep the frames sent and waiting ARP request.
  //
  LIST_ENTRY                    ArpQues;
  LIST_ENTRY                    SentFrames;


  //
  // The interface's configuration variables
  //
  UINT32                        DupAddrDetect;
  LIST_ENTRY                    DupAddrDetectList;
  LIST_ENTRY                    DelayJoinList;

  //
  // All the IP instances that have the same IP/SubnetMask are linked
  // together through IpInstances. If any of the instance enables
  // promiscuous receive, PromiscRecv is true.
  //
  LIST_ENTRY                    IpInstances;
  BOOLEAN                       PromiscRecv;
};

/**
  Create an IP6_INTERFACE.

  @param[in]  IpSb                  The IP6 service binding instance.
  @param[in]  LinkLocal             If TRUE, the instance is created for link-local address.
                                    Otherwise, it is not for a link-local address.

  @return Point to the created IP6_INTERFACE, otherwise NULL.

**/
IP6_INTERFACE *
Ip6CreateInterface (
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                LinkLocal
  );

/**
  Free the interface used by IpInstance. All the IP instance with
  the same Ip/prefix pair share the same interface. It is reference
  counted. All the frames that haven't been sent will be cancelled.
  Because the IpInstance is optional, the caller must remove
  IpInstance from the interface's instance list.

  @param[in]  Interface         The interface used by the IpInstance.
  @param[in]  IpInstance        The IP instance that free the interface. NULL if
                                the IP driver is releasing the default interface.

**/
VOID
Ip6CleanInterface (
  IN  IP6_INTERFACE         *Interface,
  IN  IP6_PROTOCOL          *IpInstance           OPTIONAL
  );

/**
  Free the link layer transmit token. It will close the event
  then free the memory used.

  @param[in]  Token                 Token to free.

**/
VOID
Ip6FreeLinkTxToken (
  IN IP6_LINK_TX_TOKEN      *Token
  );

/**
  Request Ip6OnFrameReceivedDpc as a DPC at TPL_CALLBACK

  @param  Event                 The receive event delivered to MNP for receive.
  @param  Context               Context for the callback.

**/
VOID
EFIAPI
Ip6OnFrameReceived (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  );

/**
  Request to receive the packet from the interface.

  @param[in]  CallBack          Function to call when the receive finished.
  @param[in]  IpSb              Points to the IP6 service binding instance.

  @retval EFI_ALREADY_STARTED   There is already a pending receive request.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to receive.
  @retval EFI_SUCCESS           The receive request has been started.

**/
EFI_STATUS
Ip6ReceiveFrame (
  IN  IP6_FRAME_CALLBACK    CallBack,
  IN  IP6_SERVICE           *IpSb
  );

/**
  Send a frame from the interface. If the next hop is multicast address,
  it is transmitted immediately. If the next hop is a unicast,
  and the NextHop's MAC is not known, it will perform address resolution.
  If some error happened, the CallBack won't be called. So, the caller
  must test the return value, and take action when there is an error.

  @param[in]  Interface         The interface to send the frame from
  @param[in]  IpInstance        The IP child that request the transmission.
                                NULL if it is the IP6 driver itself.
  @param[in]  Packet            The packet to transmit.
  @param[in]  NextHop           The immediate destination to transmit the packet to.
  @param[in]  CallBack          Function to call back when transmit finished.
  @param[in]  Context           Opaque parameter to the call back.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to send the frame.
  @retval EFI_NO_MAPPING        Can't resolve the MAC for the nexthop.
  @retval EFI_SUCCESS           The packet successfully transmitted.

**/
EFI_STATUS
Ip6SendFrame (
  IN  IP6_INTERFACE         *Interface,
  IN  IP6_PROTOCOL          *IpInstance      OPTIONAL,
  IN  NET_BUF               *Packet,
  IN  EFI_IPv6_ADDRESS      *NextHop,
  IN  IP6_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context
  );

/**
  The heartbeat timer of IP6 service instance. It times out
  all of its IP6 children's received-but-not-delivered and
  transmitted-but-not-recycle packets.

  @param[in]  Event                 The IP6 service instance's heart beat timer.
  @param[in]  Context               The IP6 service instance.

**/
VOID
EFIAPI
Ip6TimerTicking (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

#endif
