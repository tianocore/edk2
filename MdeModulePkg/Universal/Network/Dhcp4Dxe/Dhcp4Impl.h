/** @file
  EFI DHCP protocol implementation.
  RFCs supported are:
  RFC 2131: Dynamic Host Configuration Protocol
  RFC 2132: DHCP Options and BOOTP Vendor Extensions
  RFC 1534: Interoperation Between DHCP and BOOTP
  RFC 3396: Encoding Long Options in DHCP.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP4_IMPL_H__
#define __EFI_DHCP4_IMPL_H__



#include <Uefi.h>

#include <Protocol/Dhcp4.h>
#include <Protocol/Udp4.h>
#include <IndustryStandard/Dhcp.h>
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

#define DHCP_SERVICE_SIGNATURE   SIGNATURE_32 ('D', 'H', 'C', 'P')
#define DHCP_PROTOCOL_SIGNATURE  SIGNATURE_32 ('d', 'h', 'c', 'p')

#define DHCP_CHECK_MEDIA_WAITING_TIME    EFI_TIMER_PERIOD_SECONDS(20)

//
// The state of the DHCP service. It starts as UNCONFIGED. If
// and active child configures the service successfully, it
// goes to CONFIGED. If the active child configures NULL, it
// goes back to UNCONFIGED. It becomes DESTROY if it is (partly)
// destroyed.
//
#define DHCP_UNCONFIGED          0
#define DHCP_CONFIGED            1
#define DHCP_DESTROY             2


struct _DHCP_PROTOCOL {
  UINT32                            Signature;
  EFI_DHCP4_PROTOCOL                Dhcp4Protocol;
  LIST_ENTRY                        Link;
  EFI_HANDLE                        Handle;
  DHCP_SERVICE                      *Service;

  BOOLEAN                           InDestroy;

  EFI_EVENT                         CompletionEvent;
  EFI_EVENT                         RenewRebindEvent;

  EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token;
  UDP_IO                            *UdpIo; // The UDP IO used for TransmitReceive.
  UINT32                            Timeout;
  UINT16                            ElaspedTime;
  NET_BUF_QUEUE                     ResponseQueue;
};

//
// DHCP driver is specical in that it is a singleton. Although it
// has a service binding, there can be only one active child.
//
struct _DHCP_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;

  INTN                          ServiceState; // CONFIGED, UNCONFIGED, and DESTROY

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

  UDP_IO                        *UdpIo;       // Udp child receiving all DHCP message
  UDP_IO                        *LeaseIoPort; // Udp child with lease IP
  EFI_DHCP4_PACKET              *LastPacket;  // The last sent packet for retransmission
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
  UINT32                        LastTimeout;  // Record the init value of PacketToLive every time
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

**/
VOID
DhcpYieldControl (
  IN DHCP_SERVICE           *DhcpSb
  );

/**
  Complete a Dhcp4 transaction and signal the upper layer.

  @param Instance      Dhcp4 instance.

**/
VOID
PxeDhcpDone (
  IN DHCP_PROTOCOL  *Instance
  );

/**
  Free the resource related to the configure parameters.
  DHCP driver will make a copy of the user's configure
  such as the time out value.

  @param  Config                 The DHCP configure data

**/
VOID
DhcpCleanConfigure (
  IN OUT EFI_DHCP4_CONFIG_DATA  *Config
  );

/**
  Callback of Dhcp packet. Does nothing.

  @param Arg           The context.

**/
VOID
EFIAPI
DhcpDummyExtFree (
  IN VOID                   *Arg
  );

/**
  Set the elapsed time based on the given instance and the pointer to the
  elapsed time option.

  @param[in]      Elapsed       The pointer to the position to append.
  @param[in]      Instance      The pointer to the Dhcp4 instance.
**/
VOID
SetElapsedTime (
  IN     UINT16                 *Elapsed,
  IN     DHCP_PROTOCOL          *Instance
  );

#endif
