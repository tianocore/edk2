/** @file
  The helper routines to access UDP service. It is used by both
  DHCP and MTFTP.

Copyright (c) 2006 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UDP4IO_H_
#define _UDP4IO_H_

#include <Protocol/Udp4.h>

#include <Library/UdpIoLib.h>
#include <Library/NetLib.h>

typedef struct _UDP_IO_PORT UDP_IO_PORT;

///
/// Signatures used by UdpIo Library.
///
typedef enum {
  UDP_IO_RX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'R'),
  UDP_IO_TX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'T'),
  UDP_IO_SIGNATURE    = SIGNATURE_32 ('U', 'D', 'P', 'I')
} UDP_IO_SIGNATURE_TYPE;

///
/// The Udp4 address pair.
///
typedef struct {
  IP4_ADDR                  LocalAddr;
  UINT16                    LocalPort;
  IP4_ADDR                  RemoteAddr;
  UINT16                    RemotePort;
} UDP_POINTS;

/**
  Prototype called when receiving or sending packets from/to a UDP point.

  This prototype is used by both receive and sending when calling
  UdpIoRecvDatagram or UdpIoSendDatagram. When receiving, Netbuf is allocated by
  UDP access point, and released by user. When sending, the NetBuf is from user,
  and provided to the callback as a reference.
  
  @param Packet       Packet received or sent
  @param Points       The Udp4 address pair corresponds to the Udp4 IO
  @param IoStatus     Packet receiving or sending status
  @param Context      User-defined data when calling UdpIoRecvDatagram or
                      UdpIoSendDatagram

  @return None
**/
typedef
VOID
(EFIAPI *UDP_IO_CALLBACK) (
  IN NET_BUF                *Packet,
  IN UDP_POINTS             *Points,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );

///
/// This structure is used internally by UdpIo Library.
///
/// Each receive request is wrapped in an UDP_RX_TOKEN. Upon completion,
/// the CallBack will be called. Only one receive request is sent to UDP at a
/// time. HeadLen gives the length of the application's header. UDP_IO will
/// make the application's header continuous before delivering up.
///
typedef struct {
  UINT32                    Signature;
  UDP_IO_PORT               *UdpIo;

  UDP_IO_CALLBACK           CallBack;
  VOID                      *Context;

  UINT32                    HeadLen;
  EFI_UDP4_COMPLETION_TOKEN UdpToken;
} UDP_RX_TOKEN;

///
/// This structure is used internally by UdpIo Library.
///
/// Each transmit request is wrapped in an UDP_TX_TOKEN. Upon completion,
/// the CallBack will be called. There can be several transmit requests and they
/// are linked in a list.
///
typedef struct {
  UINT32                    Signature;
  LIST_ENTRY                Link;
  UDP_IO_PORT               *UdpIo;

  UDP_IO_CALLBACK           CallBack;
  NET_BUF                   *Packet;
  VOID                      *Context;

  EFI_UDP4_SESSION_DATA     UdpSession;
  EFI_IPv4_ADDRESS          Gateway;

  EFI_UDP4_COMPLETION_TOKEN UdpToken;
  EFI_UDP4_TRANSMIT_DATA    UdpTxData;
} UDP_TX_TOKEN;

///
/// Type defined as UDP_IO_PORT.
///
/// The data structure wraps Udp4 instance and its configuration. It is used by
/// UdpIo Library to do all Udp4 operations.
///
struct _UDP_IO_PORT {
  UINT32                    Signature;
  LIST_ENTRY                Link;
  INTN                      RefCnt;

  //
  // Handle used to create/destory UDP child
  //
  EFI_HANDLE                Controller;
  EFI_HANDLE                Image;
  EFI_HANDLE                UdpHandle;

  EFI_UDP4_PROTOCOL         *Udp;           ///< The wrapped Udp4 instance.
  EFI_UDP4_CONFIG_DATA      UdpConfig;
  EFI_SIMPLE_NETWORK_MODE   SnpMode;

  LIST_ENTRY                SentDatagram;   ///< A list of UDP_TX_TOKEN.
  UDP_RX_TOKEN              *RecvRequest;
};

/**
  Prototype called when UdpIo Library configures a Udp4 instance.
  
  The prototype is set and called when creating a UDP_IO_PORT in UdpIoCreatePort.
  
  @param UdpIo         The UDP_IO_PORT to configure
  @param Context       User-defined data when calling UdpIoCreatePort
  
  @retval EFI_SUCCESS  The configure process succeeds
  @retval Others       The UDP_IO_PORT fails to configure indicating
                       UdpIoCreatePort should fail
**/
typedef
EFI_STATUS
(EFIAPI *UDP_IO_CONFIG) (
  IN UDP_IO_PORT            *UdpIo,
  IN VOID                   *Context
  );

/**
  The select function to decide whether to cancel the UDP_TX_TOKEN. It is used
  
  @param Token        The UDP_TX_TOKEN to decide whether to cancel
  @param Context      User-defined data in UdpIoCancelDgrams
  
  @retval TRUE        To cancel the UDP_TX_TOKEN
  @retval FALSE       Do not cancel this UDP_TX_TOKEN

**/
typedef
BOOLEAN
(*UDP_IO_TO_CANCEL) (
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  );

/**
  Create a UDP_IO_PORT to access the UDP service. It will create and configure
  a UDP child.
  
  The function will locate the UDP service binding prototype on the Controller
  parameter and use it to create a UDP child (aka Udp instance). Then the UDP
  child will be configured by calling Configure function prototype. Any failures
  in creating or configure the UDP child will lead to the failure of UDP_IO_PORT
  creation.

  @param  Controller            The controller that has the UDP service binding
                                protocol installed.
  @param  Image                 The image handle for the driver.
  @param  Configure             The function to configure the created UDP child
  @param  Context               The opaque parameter for the Configure funtion.

  @return Newly-created UDP_IO_PORT or NULL if failed.

**/
UDP_IO_PORT *
EFIAPI
UdpIoCreatePort (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  UDP_IO_CONFIG         Configure,
  IN  VOID                  *Context
  );

/**
  Free the UDP_IO_PORT and all its related resources.
  
  The function will cancel all sent datagram and receive request.

  @param  UdpIo                 The UDP_IO_PORT to free.

  @retval EFI_SUCCESS           The UDP_IO_PORT is freed.

**/
EFI_STATUS
EFIAPI
UdpIoFreePort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Clean up the UDP_IO_PORT without freeing it. The function is called when
  user wants to re-use the UDP_IO_PORT later.
  
  It will release all the transmitted datagrams and receive request. It will
  also configure NULL for the UDP instance.

  @param  UdpIo                 The UDP_IO_PORT to clean up.

**/
VOID
EFIAPI
UdpIoCleanPort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Send a packet through the UDP_IO_PORT.
  
  The packet will be wrapped in UDP_TX_TOKEN. Function Callback will be called
  when the packet is sent. The optional parameter EndPoint overrides the default
  address pair if specified.

  @param  UdpIo                 The UDP_IO_PORT to send the packet through
  @param  Packet                The packet to send
  @param  EndPoint              The local and remote access point. Override the
                                default address pair set during configuration.
  @param  Gateway               The gateway to use
  @param  CallBack              The function being called when packet is
                                transmitted or failed.
  @param  Context               The opaque parameter passed to CallBack

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the packet
  @retval EFI_SUCCESS           The packet is successfully delivered to UDP  for
                                transmission.

**/
EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet,
  IN  UDP_POINTS            *EndPoint  OPTIONAL,
  IN  IP4_ADDR              Gateway,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context
  );

/**
  Cancel a single sent datagram.

  @param  UdpIo                 The UDP_IO_PORT to cancel the packet from
  @param  Packet                The packet to cancel

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet
  );

/**
  Issue a receive request to the UDP_IO_PORT.
  
  This function is called when upper-layer needs packet from UDP for processing.
  Only one receive request is acceptable at a time so a common usage model is
  to invoke this function inside its Callback function when the former packet
  is processed.

  @param  UdpIo                 The UDP_IO_PORT to receive the packet from.
  @param  CallBack              The call back function to execute when the packet
                                is received.
  @param  Context               The opaque context passed to Callback
  @param  HeadLen               The length of the upper-layer's protocol header

  @retval EFI_ALREADY_STARTED   There is already a pending receive request. Only
                                one receive request is supported at a time.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate needed resources.
  @retval EFI_SUCCESS           The receive request is issued successfully.

**/
EFI_STATUS
EFIAPI
UdpIoRecvDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context,
  IN  UINT32                HeadLen
  );
#endif
