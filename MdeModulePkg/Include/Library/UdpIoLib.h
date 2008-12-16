/** @file
  The helper routines to access UDP service. It is used by both
  DHCP and MTFTP.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
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

typedef enum {
  UDP_IO_RX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'R'),
  UDP_IO_TX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'T'),
  UDP_IO_SIGNATURE    = SIGNATURE_32 ('U', 'D', 'P', 'I')
} UDP_IO_SIGNATURE_TYPE;

typedef struct {
  IP4_ADDR                  LocalAddr;
  UINT16                    LocalPort;
  IP4_ADDR                  RemoteAddr;
  UINT16                    RemotePort;
} UDP_POINTS;

//
// This prototype is used by both receive and transmission.
// When receiving Netbuf is allocated by UDP access point, and
// released by user. When transmitting, the NetBuf is from user,
// and provided to the callback as a reference.
//
typedef
VOID
(*UDP_IO_CALLBACK) (
  IN NET_BUF                *Packet,
  IN UDP_POINTS             *Points,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );

//
// Each receive request is wrapped in an UDP_RX_TOKEN. Upon completion,
// the CallBack will be called. Only one receive request is send to UDP.
// HeadLen gives the length of the application's header. UDP_IO will
// make the application's header continous before delivery up.
//
typedef struct {
  UINT32                    Signature;
  UDP_IO_PORT               *UdpIo;

  UDP_IO_CALLBACK           CallBack;
  VOID                      *Context;

  UINT32                    HeadLen;
  EFI_UDP4_COMPLETION_TOKEN UdpToken;
} UDP_RX_TOKEN;

//
// Each transmit request is wrapped in an UDP_TX_TOKEN. Upon completion,
// the CallBack will be called. There can be several transmit requests.
//
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

  EFI_UDP4_PROTOCOL         *Udp;
  EFI_UDP4_CONFIG_DATA      UdpConfig;
  EFI_SIMPLE_NETWORK_MODE   SnpMode;

  LIST_ENTRY                SentDatagram;
  UDP_RX_TOKEN              *RecvRequest;
};

typedef
EFI_STATUS
(*UDP_IO_CONFIG) (
  IN UDP_IO_PORT            *UdpIo,
  IN VOID                   *Context
  );

typedef
BOOLEAN
(*UDP_IO_TO_CANCEL) (
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  );

/**
  Create a UDP IO port to access the UDP service. It will
  create and configure a UDP child.

  @param  Controller            The controller that has the UDP service binding
                                protocol installed.
  @param  ImageHandle           The image handle for the driver.
  @param  Configure             The function to configure the created UDP child
  @param  Context               The opaque parameter for the Configure funtion.

  @return A point to just created UDP IO port or NULL if failed.

**/
UDP_IO_PORT *
EFIAPI
UdpIoCreatePort (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            ImageHandle,
  IN  UDP_IO_CONFIG         Configure,
  IN  VOID                  *Context
  );

/**
  Free the UDP IO port and all its related resources including
  all the transmitted packet.

  @param  UdpIo                 The UDP IO port to free.

  @retval EFI_SUCCESS           The UDP IO port is freed.

**/
EFI_STATUS
EFIAPI
UdpIoFreePort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Clean up the UDP IO port. It will release all the transmitted
  datagrams and receive request. It will also configure NULL the
  UDP child.

  @param  UdpIo                 UDP IO port to clean up.

  @return None

**/
VOID
EFIAPI
UdpIoCleanPort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Send a packet through the UDP IO port.

  @param  UdpIo                 The UDP IO Port to send the packet through
  @param  Packet                The packet to send
  @param  EndPoint              The local and remote access point
  @param  Gateway               The gateway to use
  @param  CallBack              The call back function to call when packet is
                                transmitted or failed.
  @param  Context               The opque parameter to the CallBack

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the packet
  @retval EFI_SUCCESS           The packet is successfully delivered to UDP  for
                                transmission.

**/
EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet,
  IN  UDP_POINTS            *EndPoint, OPTIONAL
  IN  IP4_ADDR              Gateway,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context
  );

/**
  Cancel a single sent datagram.

  @param  UdpIo                 The UDP IO port to cancel the packet from
  @param  Packet                The packet to cancel

  @return None

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet
  );

/**
  Issue a receive request to the UDP IO port.

  @param  UdpIo                 The UDP IO port to recieve the packet from.
  @param  CallBack              The call back function to execute when receive
                                finished.
  @param  Context               The opque context to the call back
  @param  HeadLen               The lenght of the application's header

  @retval EFI_ALREADY_STARTED   There is already a pending receive request. Only
                                one receive request is supported.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate some resource.
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
