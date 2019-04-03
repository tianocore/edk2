/** @file
  The DHCP4 protocol implementation.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP4_IO_H__
#define __EFI_DHCP4_IO_H__

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>

#include <Library/NetLib.h>
#include <Library/UdpIoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>



#define DHCP_WAIT_OFFER                    3  // Time to wait the offers
#define DHCP_DEFAULT_LEASE  7 * 24 * 60 * 60  // Seven days as default.
#define DHCP_SERVER_PORT                  67
#define DHCP_CLIENT_PORT                  68

//
// BOOTP header "op" field
//
#define BOOTP_REQUEST           1
#define BOOTP_REPLY             2

//
// DHCP message types
//
#define DHCP_MSG_DISCOVER       1
#define DHCP_MSG_OFFER          2
#define DHCP_MSG_REQUEST        3
#define DHCP_MSG_DECLINE        4
#define DHCP_MSG_ACK            5
#define DHCP_MSG_NAK            6
#define DHCP_MSG_RELEASE        7
#define DHCP_MSG_INFORM         8

//
// DHCP notify user type
//
#define DHCP_NOTIFY_COMPLETION  1
#define DHCP_NOTIFY_RENEWREBIND 2
#define DHCP_NOTIFY_ALL         3

#define DHCP_IS_BOOTP(Parameter)  (((Parameter) == NULL) || ((Parameter)->DhcpType == 0))

#define DHCP_CONNECTED(State)     \
  (((State) == Dhcp4Bound) || ((State) == (Dhcp4Renewing)) || ((State) == Dhcp4Rebinding))

/**
  Set the DHCP state. If CallUser is true, it will try to notify
  the user before change the state by DhcpNotifyUser. It returns
  EFI_ABORTED if the user return EFI_ABORTED, otherwise, it returns
  EFI_SUCCESS. If CallUser is FALSE, it isn't necessary to test
  the return value of this function.

  @param  DhcpSb                The DHCP service instance
  @param  State                 The new DHCP state to change to
  @param  CallUser              Whether we need to call user

  @retval EFI_SUCCESS           The state is changed
  @retval EFI_ABORTED           The user asks to abort the DHCP process.

**/
EFI_STATUS
DhcpSetState (
  IN OUT DHCP_SERVICE           *DhcpSb,
  IN     INTN                   State,
  IN     BOOLEAN                CallUser
  );

/**
  Build and transmit a DHCP message according to the current states.
  This function implement the Table 5. of RFC 2131. Always transits
  the state (as defined in Figure 5. of the same RFC) before sending
  a DHCP message. The table is adjusted accordingly.

  @param[in]  DhcpSb                The DHCP service instance
  @param[in]  Seed                  The seed packet which the new packet is based on
  @param[in]  Para                  The DHCP parameter of the Seed packet
  @param[in]  Type                  The message type to send
  @param[in]  Msg                   The human readable message to include in the packet
                                    sent.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources for the packet
  @retval EFI_ACCESS_DENIED     Failed to transmit the packet through UDP
  @retval EFI_SUCCESS           The message is sent
  @retval other                 Other error occurs

**/
EFI_STATUS
DhcpSendMessage (
  IN DHCP_SERVICE           *DhcpSb,
  IN EFI_DHCP4_PACKET       *Seed,
  IN DHCP_PARAMETER         *Para,
  IN UINT8                  Type,
  IN UINT8                  *Msg
  );

/**
  Each DHCP service has three timer. Two of them are count down timer.
  One for the packet retransmission. The other is to collect the offers.
  The third timer increaments the lease life which is compared to T1, T2,
  and lease to determine the time to renew and rebind the lease.
  DhcpOnTimerTick will be called once every second.

  @param[in]  Event                 The timer event
  @param[in]  Context               The context, which is the DHCP service instance.

**/
VOID
EFIAPI
DhcpOnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  Handle the received DHCP packets. This function drives the DHCP
  state machine.

  @param  UdpPacket             The UDP packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DhcpInput (
  NET_BUF                   *UdpPacket,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

/**
  Send an initial DISCOVER or REQUEST message according to the
  DHCP service's current state.

  @param[in]  DhcpSb                The DHCP service instance

  @retval EFI_SUCCESS           The request has been sent
  @retval other                 Some error occurs when sending the request.

**/
EFI_STATUS
DhcpInitRequest (
  IN DHCP_SERVICE           *DhcpSb
  );

/**
  Clean up the DHCP related states, IoStatus isn't reset.

  @param  DhcpSb                The DHCP instance service.

**/
VOID
DhcpCleanLease (
  IN DHCP_SERVICE           *DhcpSb
  );

/**
  Release the net buffer when packet is sent.

  @param  UdpPacket             The UDP packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DhcpOnPacketSent (
  NET_BUF                   *Packet,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

#endif
