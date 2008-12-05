/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Driver.h

Abstract:


**/

#ifndef _TCP4_DRIVER_H_
#define _TCP4_DRIVER_H_

#include <Protocol/ServiceBinding.h>
#include <Library/IpIoLib.h>

#define TCP4_DRIVER_SIGNATURE   EFI_SIGNATURE_32 ('T', 'C', 'P', '4')

#define TCP4_PORT_KNOWN         1024
#define TCP4_PORT_USER_RESERVED 65535

typedef struct _TCP4_HEARTBEAT_TIMER {
  EFI_EVENT  TimerEvent;
  INTN       RefCnt;
} TCP4_HEARTBEAT_TIMER;

typedef struct _TCP4_SERVICE_DATA {
  UINT32                        Signature;
  EFI_HANDLE                    ControllerHandle;
  IP_IO                         *IpIo;  // IP Io consumed by TCP4
  EFI_SERVICE_BINDING_PROTOCOL  Tcp4ServiceBinding;
  EFI_HANDLE                    DriverBindingHandle;
  CHAR16                        *MacString;
  LIST_ENTRY                    SocketList;
} TCP4_SERVICE_DATA;

//
// Prototype for TCP4 driver Rcv callback function registered to IP_IO
//
VOID
Tcp4RxCallback (
  IN EFI_STATUS                       Status,
  IN ICMP_ERROR                       IcmpErr,
  IN EFI_NET_SESSION_DATA             *NetSession,
  IN NET_BUF                          *Pkt,
  IN VOID                             *Context    OPTIONAL
  );

INTN
TcpSendIpPacket (
  IN TCP_CB    *Tcb,
  IN NET_BUF   *Nbuf,
  IN UINT32    Src,
  IN UINT32    Dest
  );

EFI_STATUS
Tcp4Dispatcher (
  IN SOCKET                  *Sock,
  IN SOCK_REQUEST            Request,
  IN VOID                    *Data   OPTIONAL
  );

typedef struct _TCP4_PROTO_DATA {
  TCP4_SERVICE_DATA *TcpService;
  TCP_CB            *TcpPcb;
} TCP4_PROTO_DATA;

#define TCP4_FROM_THIS(a) \
  CR ( \
  (a), \
  TCP4_SERVICE_DATA, \
  Tcp4ServiceBinding, \
  TCP4_DRIVER_SIGNATURE \
  )

//
// Function prototype for the driver's entry point
//
EFI_STATUS
EFIAPI
Tcp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

//
// Function prototypes for the Drivr Binding Protocol
//
EFI_STATUS
EFIAPI
Tcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
Tcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
Tcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

EFI_STATUS
Tcp4CreateSocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

VOID
Tcp4DestroySocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

//
// Function ptototypes for the ServiceBinding Prococol
//
EFI_STATUS
EFIAPI
Tcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  );

EFI_STATUS
EFIAPI
Tcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
