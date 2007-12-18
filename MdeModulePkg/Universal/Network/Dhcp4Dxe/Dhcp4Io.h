/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Dhcp4Io.h

Abstract:

  The DHCP4 protocol implementation.


**/

#ifndef __EFI_DHCP4_IO_H__
#define __EFI_DHCP4_IO_H__

#include <PiDxe.h>

#include <Protocol/ServiceBinding.h>

#include <Library/NetLib.h>
#include <Library/UdpIoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>


enum {
  DHCP_WAIT_OFFER         = 3,              // Time to wait the offers
  DHCP_DEFAULT_LEASE      = 7 *24 *60 *60,  // Seven days as default.
  DHCP_SERVER_PORT        = 67,
  DHCP_CLIENT_PORT        = 68,

  //
  // BOOTP header "op" field
  //
  BOOTP_REQUEST           = 1,
  BOOTP_REPLY             = 2,

  //
  // DHCP message types
  //
  DHCP_MSG_DISCOVER       = 1,
  DHCP_MSG_OFFER          = 2,
  DHCP_MSG_REQUEST        = 3,
  DHCP_MSG_DECLINE        = 4,
  DHCP_MSG_ACK            = 5,
  DHCP_MSG_NAK            = 6,
  DHCP_MSG_RELEASE        = 7,
  DHCP_MSG_INFORM         = 8,

  //
  // DHCP notify user type
  //
  DHCP_NOTIFY_COMPLETION  = 1,
  DHCP_NOTIFY_RENEWREBIND,
  DHCP_NOTIFY_ALL
};

#define DHCP_IS_BOOTP(Parameter)  (((Parameter) == NULL) || ((Parameter)->DhcpType == 0))

#define DHCP_CONNECTED(State)     \
  (((State) == Dhcp4Bound) || ((State) == (Dhcp4Renewing)) || ((State) == Dhcp4Rebinding))

EFI_STATUS
DhcpSetState (
  IN DHCP_SERVICE           *DhcpSb,
  IN INTN                   State,
  IN BOOLEAN                CallUser
  );

EFI_STATUS
DhcpSendMessage (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Seed,
  IN DHCP_PARAMETER         *Para,
  IN UINT8                  Type,
  IN UINT8                  *Msg
  );

VOID
EFIAPI
DhcpOnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

VOID
DhcpInput (
  NET_BUF                   *UdpPacket,
  UDP_POINTS                *Points,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

EFI_STATUS
DhcpInitRequest (
  IN DHCP_SERVICE           *DhcpSb
  );

VOID
DhcpCleanLease (
  IN DHCP_SERVICE           *DhcpSb
  );

VOID
DhcpOnPacketSent (
  NET_BUF                   *Packet,
  UDP_POINTS                *Points,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

#endif
