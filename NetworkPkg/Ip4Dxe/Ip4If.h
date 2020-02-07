/** @file
  Definition for IP4 pseudo interface structure.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP4_IF_H__
#define __EFI_IP4_IF_H__

#define IP4_FRAME_RX_SIGNATURE  SIGNATURE_32 ('I', 'P', 'F', 'R')
#define IP4_FRAME_TX_SIGNATURE  SIGNATURE_32 ('I', 'P', 'F', 'T')
#define IP4_FRAME_ARP_SIGNATURE SIGNATURE_32 ('I', 'P', 'F', 'A')
#define IP4_INTERFACE_SIGNATURE SIGNATURE_32 ('I', 'P', 'I', 'F')

/**
  This prototype is used by both receive and transmission.
  When receiving Netbuf is allocated by IP4_INTERFACE, and
  released by IP4. Flag shows whether the frame is received
  as link broadcast/multicast...

  When transmitting, the Netbuf is from IP4, and provided
  to the callback as a reference. Flag isn't used.

  @param[in] IpInstance The instance that sent or received the packet.
                        IpInstance can be NULL which means that it is the IP4 driver
                        itself sending the packets. IP4 driver may send packets that
                        don't belong to any instance, such as ICMP errors, ICMP echo
                        responses, or IGMP packets. IpInstance is used as a tag in
                        this module.
  @param[in] Packet     The sent or received packet.
  @param[in] IoStatus   Status of sending or receiving.
  @param[in] LinkFlag   Indicate if the frame is received as link broadcast/multicast.
                        When transmitting, it is not used.
  @param[in] Context    Additional data for callback.

  @retval None.
**/
typedef
VOID
(*IP4_FRAME_CALLBACK)(
  IN IP4_PROTOCOL              *IpInstance       OPTIONAL,
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
  IP4_SERVICE                           *IpSb;

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
  // the subnet/net broadcast address for quick access. The fields
  // are invalid if (Configured == FALSE)
  //
  IP4_ADDR                      Ip;
  IP4_ADDR                      SubnetMask;
  IP4_ADDR                      SubnetBrdcast;
  IP4_ADDR                      NetBrdcast;
  BOOLEAN                       Configured;

  //
  // Handle used to create/destroy ARP child. All the IP children
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

/**
  Create an IP4_INTERFACE. Delay the creation of ARP instance until
  the interface is configured.

  @param[in]  Mnp               The shared MNP child of this IP4 service binding
                                instance.
  @param[in]  Controller        The controller this IP4 service binding instance
                                is installed. Most like the UNDI handle.
  @param[in]  ImageHandle       This driver's image handle.

  @return Point to the created IP4_INTERFACE, otherwise NULL.

**/
IP4_INTERFACE *
Ip4CreateInterface (
  IN  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp,
  IN  EFI_HANDLE                    Controller,
  IN  EFI_HANDLE                    ImageHandle
  );

/**
  Set the interface's address, create and configure
  the ARP child if necessary.

  @param  Interface         The interface to set the address.
  @param  IpAddr            The interface's IP address.
  @param  SubnetMask        The interface's netmask.

  @retval EFI_SUCCESS           The interface is configured with Ip/netmask pair,
                                and a ARP is created for it.
  @retval Others                Failed to set the interface's address.

**/
EFI_STATUS
Ip4SetAddress (
  IN OUT IP4_INTERFACE      *Interface,
  IN     IP4_ADDR           IpAddr,
  IN     IP4_ADDR           SubnetMask
  );

/**
  Free the interface used by IpInstance. All the IP instance with
  the same Ip/Netmask pair share the same interface. It is reference
  counted. All the frames haven't been sent will be cancelled.
  Because the IpInstance is optional, the caller must remove
  IpInstance from the interface's instance list itself.

  @param[in]  Interface         The interface used by the IpInstance.
  @param[in]  IpInstance        The Ip instance that free the interface. NULL if
                                the Ip driver is releasing the default interface.

  @retval EFI_SUCCESS           The interface use IpInstance is freed.

**/
EFI_STATUS
Ip4FreeInterface (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance           OPTIONAL
  );

/**
  Send a frame from the interface. If the next hop is broadcast or
  multicast address, it is transmitted immediately. If the next hop
  is a unicast, it will consult ARP to resolve the NextHop's MAC.
  If some error happened, the CallBack won't be called. So, the caller
  must test the return value, and take action when there is an error.

  @param[in]  Interface         The interface to send the frame from
  @param[in]  IpInstance        The IP child that request the transmission.  NULL
                                if it is the IP4 driver itself.
  @param[in]  Packet            The packet to transmit.
  @param[in]  NextHop           The immediate destination to transmit the packet
                                to.
  @param[in]  CallBack          Function to call back when transmit finished.
  @param[in]  Context           Opaque parameter to the call back.
  @param[in]  IpSb              The pointer to the IP4 service binding instance.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to send the frame
  @retval EFI_NO_MAPPING        Can't resolve the MAC for the nexthop
  @retval EFI_SUCCESS           The packet is successfully transmitted.
  @retval other                 Other error occurs.

**/
EFI_STATUS
Ip4SendFrame (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance       OPTIONAL,
  IN  NET_BUF               *Packet,
  IN  IP4_ADDR              NextHop,
  IN  IP4_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context,
  IN IP4_SERVICE            *IpSb
  );

/**
  Remove all the frames on the interface that pass the FrameToCancel,
  either queued on ARP queues or that have already been delivered to
  MNP and not yet recycled.

  @param[in]  Interface         Interface to remove the frames from.
  @param[in]  IoStatus          The transmit status returned to the frames'
                                callback.
  @param[in]  FrameToCancel     Function to select the frame to cancel, NULL to
                                select all.
  @param[in]  Context           Opaque parameters passed to FrameToCancel.

**/
VOID
Ip4CancelFrames (
  IN IP4_INTERFACE          *Interface,
  IN EFI_STATUS             IoStatus,
  IN IP4_FRAME_TO_CANCEL    FrameToCancel    OPTIONAL,
  IN VOID                   *Context
  );

/**
  If there is a pending receive request, cancel it. Don't call
  the receive request's callback because this function can be only
  called if the instance or driver is tearing itself down. It
  doesn't make sense to call it back. But it is necessary to call
  the transmit token's callback to give it a chance to free the
  packet and update the upper layer's transmit request status, say
  that from the UDP.

  @param[in]  Interface         The interface used by the IpInstance

**/
VOID
Ip4CancelReceive (
  IN IP4_INTERFACE          *Interface
  );

/**
  Request to receive the packet from the interface.

  @param[in]  Interface         The interface to receive the frames from.
  @param[in]  IpInstance        The instance that requests the receive. NULL for
                                the driver itself.
  @param[in]  CallBack          Function to call when receive finished.
  @param[in]  Context           Opaque parameter to the callback.

  @retval EFI_ALREADY_STARTED   There is already a pending receive request.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to receive.
  @retval EFI_SUCCESS           The receive request has been started.
  @retval other                 Other error occurs.

**/
EFI_STATUS
Ip4ReceiveFrame (
  IN  IP4_INTERFACE         *Interface,
  IN  IP4_PROTOCOL          *IpInstance       OPTIONAL,
  IN  IP4_FRAME_CALLBACK    CallBack,
  IN  VOID                  *Context
  );

#endif
