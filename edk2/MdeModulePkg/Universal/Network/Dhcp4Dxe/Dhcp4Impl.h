/** @file

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Dhcp4Impl.h

Abstract:

  EFI DHCP protocol implementation
  RFCs supported are:
  RFC 2131: Dynamic Host Configuration Protocol
  RFC 2132: DHCP Options and BOOTP Vendor Extensions
  RFC 1534: Interoperation Between DHCP and BOOTP
  RFC 3396: Encoding Long Options in DHCP


**/

#ifndef __EFI_DHCP4_IMPL_H__
#define __EFI_DHCP4_IMPL_H__



#include <PiDxe.h>

#include <Protocol/Dhcp4.h>
#include <Protocol/Udp4.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>

typedef struct _DHCP_SERVICE  DHCP_SERVICE;
typedef struct _DHCP_PROTOCOL DHCP_PROTOCOL;

#include "Dhcp4Option.h"
#include "Dhcp4Io.h"

#define DHCP_SERVICE_SIGNATURE   EFI_SIGNATURE_32 ('D', 'H', 'C', 'P')
#define DHCP_PROTOCOL_SIGNATURE  EFI_SIGNATURE_32 ('d', 'h', 'c', 'p')

typedef enum {
  //
  // The state of the DHCP service. It starts as UNCONFIGED. If
  // and active child configures the service successfully, it
  // goes to CONFIGED. If the active child configures NULL, it
  // goes back to UNCONFIGED. It becomes DESTORY if it is (partly)
  // destoried.
  //
  DHCP_UNCONFIGED         = 0,
  DHCP_CONFIGED,
  DHCP_DESTORY
} DHCP_STATE;

struct _DHCP_PROTOCOL {
  UINT32                            Signature;
  EFI_DHCP4_PROTOCOL                Dhcp4Protocol;
  LIST_ENTRY                        Link;
  EFI_HANDLE                        Handle;
  DHCP_SERVICE                      *Service;

  BOOLEAN                           InDestory;

  EFI_EVENT                         CompletionEvent;
  EFI_EVENT                         RenewRebindEvent;

  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token;
  UDP_IO_PORT                       *UdpIo; // The UDP IO used for TransmitReceive.
  UINT32                            Timeout;
  NET_BUF_QUEUE                     ResponseQueue;
};

//
// DHCP driver is specical in that it is a singleton. Although it
// has a service binding, there can be only one active child.
//
struct _DHCP_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;

  INTN                          ServiceState; // CONFIGED, UNCONFIGED, and DESTORY
  BOOLEAN                       InDestory;

  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  LIST_ENTRY                    Children;
  UINTN                         NumChildren;

  INTN                          DhcpState;
  EFI_STATUS                    IoStatus;     // the result of last user operation
  UINT32                        Xid;

  IP4_ADDR                      ClientAddr;   // lease IP or configured client address
  IP4_ADDR                      Netmask;
  IP4_ADDR                      ServerAddr;

  EFI_DHCP4_PACKET              *LastOffer;   // The last received offer
  EFI_DHCP4_PACKET              *Selected;
  DHCP_PARAMETER                *Para;

  UINT32                        Lease;
  UINT32                        T1;
  UINT32                        T2;
  INTN                          ExtraRefresh; // This refresh is reqested by user

  UDP_IO_PORT                   *UdpIo;       // Udp child receiving all DHCP message
  UDP_IO_PORT                   *LeaseIoPort; // Udp child with lease IP
  NET_BUF                       *LastPacket;  // The last sent packet for retransmission
  EFI_MAC_ADDRESS               Mac;
  UINT8                         HwType;
  UINT8                         HwLen;
  UINT8                         ClientAddressSendOut[16];

  DHCP_PROTOCOL                 *ActiveChild;
  EFI_DHCP4_CONFIG_DATA         ActiveConfig;
  UINT32                        UserOptionLen;

  //
  // Timer event and various timer
  //
  EFI_EVENT                     Timer;

  UINT32                        PacketToLive; // Retransmission timer for our packets
  INTN                          CurRetry;
  INTN                          MaxRetries;
  UINT32                        LeaseLife;
};

typedef struct {
  EFI_DHCP4_PACKET_OPTION       **Option;
  UINT32                        OptionCount;
  UINT32                        Index;
} DHCP_PARSE_CONTEXT;

#define DHCP_INSTANCE_FROM_THIS(Proto)  \
  CR ((Proto), DHCP_PROTOCOL, Dhcp4Protocol, DHCP_PROTOCOL_SIGNATURE)

#define DHCP_SERVICE_FROM_THIS(Sb)      \
  CR ((Sb), DHCP_SERVICE, ServiceBinding, DHCP_SERVICE_SIGNATURE)

extern EFI_DHCP4_PROTOCOL mDhcp4ProtocolTemplate;

/**
  Give up the control of the DHCP service to let other child
  resume. Don't change the service's DHCP state and the Client
  address and option list configure as required by RFC2131.

  @param  DhcpSb                 The DHCP service instance.

  @return None

**/
VOID
DhcpYieldControl (
  IN DHCP_SERVICE         *DhcpSb
  );

VOID
PxeDhcpDone (
  IN DHCP_PROTOCOL  *Instance
  );

#endif
