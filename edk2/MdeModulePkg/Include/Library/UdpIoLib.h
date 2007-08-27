/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Udp4Io.h

Abstract:

  The helper routines to access UDP service. It is used by both
  DHCP and MTFTP.


**/

#ifndef _UDP4IO_H_
#define _UDP4IO_H_

#include <PiDxe.h>

#include <Protocol/Udp4.h>

#include <Library/UdpIoLib.h>
#include <Library/NetLib.h>

typedef struct _UDP_IO_PORT UDP_IO_PORT;

enum {
  UDP_IO_RX_SIGNATURE = EFI_SIGNATURE_32 ('U', 'D', 'P', 'R'),
  UDP_IO_TX_SIGNATURE = EFI_SIGNATURE_32 ('U', 'D', 'P', 'T'),
  UDP_IO_SIGNATURE    = EFI_SIGNATURE_32 ('U', 'D', 'P', 'I'),
};

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
  NET_LIST_ENTRY            Link;
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
  NET_LIST_ENTRY            Link;
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

  NET_LIST_ENTRY            SentDatagram;
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

UDP_IO_PORT *
UdpIoCreatePort (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            ImageHandle,
  IN  UDP_IO_CONFIG         Configure,
  IN  VOID                  *Context
  );

EFI_STATUS
UdpIoFreePort (
  IN  UDP_IO_PORT           *UdpIo
  );

VOID
UdpIoCleanPort (
  IN  UDP_IO_PORT           *UdpIo
  );

EFI_STATUS
UdpIoSendDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet,
  IN  UDP_POINTS            *EndPoint, OPTIONAL
  IN  IP4_ADDR              Gateway,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context
  );

VOID
UdpIoCancelSentDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet
  );

EFI_STATUS
UdpIoRecvDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context,
  IN  UINT32                HeadLen
  );
#endif
