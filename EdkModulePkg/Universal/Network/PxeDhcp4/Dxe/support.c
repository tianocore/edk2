/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  support.c

Abstract:
  Miscellaneous support routines for PxeDhcp4 protocol.

--*/


#include "PxeDhcp4.h"

#define DebugPrint(x)
//
// #define DebugPrint(x) Aprint x
//

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
UINT16
htons (
  UINTN n
  )
{
  return (UINT16) ((n >> 8) | (n << 8));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
UINT32
htonl (
  UINTN n
  )
{
  return (UINT32) ((n >> 24) | ((n >> 8) & 0xFF00) | ((n & 0xFF00) << 8) | (n << 24));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
EFIAPI
timeout_notify (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  ASSERT (Context);

  if (Context != NULL) {
    ((PXE_DHCP4_PRIVATE_DATA *) Context)->TimeoutOccurred = TRUE;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
EFIAPI
periodic_notify (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  ASSERT (Context);

  if (Context != NULL) {
    ((PXE_DHCP4_PRIVATE_DATA *) Context)->PeriodicOccurred = TRUE;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
find_opt (
  IN DHCP4_PACKET *Packet,
  IN UINT8        OpCode,
  IN UINTN        Skip,
  OUT DHCP4_OP    **OpPtr
  )
/*++
Routine description:
  Locate option inside DHCP packet.

Parameters:
  Packet := Pointer to DHCP packet structure.
  OpCode := Option op-code to find.
  Skip := Number of found op-codes to skip.
  OpPtr := Pointer to found op-code pointer.

Returns:
  EFI_SUCCESS := Option was found
  EFI_INVALID_PARAMETER := Packet == NULL || OpPtr == NULL
  EFI_INVALID_PARAMETER := OpCode == DHCP4_PAD
  EFI_INVALID_PARAMETER := OpCode == DHCP4_END && Skip != 0
  EFI_INVALID_PARAMETER := DHCP magik number in Packet is not valid
  EFI_NOT_FOUND := op-code was not found in packet
  EFI_INVALID_PARAMETER := If present, DHCP_MAX_MESSAGE_SIZE option
    does not have a valid value.
--*/
{
  UINTN msg_size;
  UINTN buf_len;
  UINTN n;
  UINT8 *buf;
  UINT8 *end_ptr;
  UINT8 overload;

  //
  // Verify parameters.
  //
  if (Packet == NULL || OpPtr == NULL || OpCode == DHCP4_PAD || (OpCode == DHCP4_END && Skip != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packet->dhcp4.magik != htonl (DHCP4_MAGIK_NUMBER)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize search variables.
  //
  *OpPtr    = NULL;

  msg_size  = DHCP4_MAX_PACKET_SIZE - (DHCP4_UDP_HEADER_SIZE + DHCP4_IP_HEADER_SIZE);

  overload  = 0;
  end_ptr   = NULL;

  buf       = Packet->dhcp4.options;
  buf_len   = msg_size - (Packet->dhcp4.options - Packet->raw);

  //
  // Start searching for requested option.
  //
  for (n = 0;;) {
    //
    // If match is found, decrement skip count and return
    // when desired match is found.
    //
    if (buf[n] == OpCode) {
      *OpPtr = (DHCP4_OP *) &buf[n];

      if (Skip-- == 0) {
        return EFI_SUCCESS;
      }
    }
    //
    // Skip past current option.  Check for option overload
    // and message size options since these will affect the
    // amount of data to be searched.
    //
    switch (buf[n]) {
    case DHCP4_PAD:
      //
      // Remember the first pad byte of a group.  This
      // could be the end of a badly formed packet.
      //
      if (end_ptr == NULL) {
        end_ptr = &buf[n];
      }

      ++n;
      break;

    case DHCP4_END:
      //
      // If we reach the end we are done.
      //
      end_ptr = NULL;
      return EFI_NOT_FOUND;

    case DHCP4_OPTION_OVERLOAD:
      //
      // Remember the option overload value since it
      // could cause the search to continue into
      // the fname and sname fields.
      //
      end_ptr = NULL;

      if (buf[n + 1] == 1) {
        overload = buf[n + 2];
      }

      n += 2 + buf[n + 1];
      break;

    case DHCP4_MAX_MESSAGE_SIZE:
      //
      // Remember the message size value since it could
      // change the amount of option buffer to search.
      //
      end_ptr = NULL;

      if (buf[n + 1] == 2 && buf == Packet->dhcp4.options) {
        msg_size = ((buf[n + 2] << 8) | buf[n + 3]) - (DHCP4_UDP_HEADER_SIZE + DHCP4_IP_HEADER_SIZE);

        if (msg_size < 328) {
          return EFI_INVALID_PARAMETER;
        }

        buf_len = msg_size - (Packet->dhcp4.options - Packet->raw);

        if (n + 2 + buf[n + 1] > buf_len) {
          return EFI_INVALID_PARAMETER;
        }
      }

    /* fall thru */
    default:
      end_ptr = NULL;

      n += 2 + buf[n + 1];
    }
    //
    // Keep searching until the end of the buffer is reached.
    //
    if (n < buf_len) {
      continue;
    }
    //
    // Reached end of current buffer.  Check if we are supposed
    // to search the fname and sname buffers.
    //
    if (buf == Packet->dhcp4.options &&
        (overload == DHCP4_OVERLOAD_FNAME || overload == DHCP4_OVERLOAD_FNAME_AND_SNAME)
        ) {
      buf     = Packet->dhcp4.fname;
      buf_len = 128;
      n       = 0;
      continue;
    }

    if (buf != Packet->dhcp4.sname && (overload == DHCP4_OVERLOAD_SNAME || overload == DHCP4_OVERLOAD_FNAME_AND_SNAME)) {
      buf     = Packet->dhcp4.sname;
      buf_len = 64;
      n       = 0;
      continue;
    }
    //
    // End of last buffer reached.  If this was a search
    // for the end of the options, go back to the start
    // of the current pad block.
    //
    if (OpCode == DHCP4_END && end_ptr != NULL) {
      *OpPtr = (DHCP4_OP *) end_ptr;
      return EFI_SUCCESS;
    }

    return EFI_NOT_FOUND;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
add_opt (
  IN DHCP4_PACKET *Packet,
  IN DHCP4_OP     *OpPtr
  )
/*++
Routine description:
  Add option to DHCP packet.

Parameters:
  Packet := Pointer to DHCP packet structure.
  OpPtr := Pointer to DHCP option.

Returns:
  EFI_INVALID_PARAMETER := Packet == NULL || OpPtr == NULL
  EFI_INVALID_PARAMETER := OpPtr->op == DHCP4_PAD || OpPtr->op == DHCP4_END
  EFI_INVALID_PARAMETER := DHCP magik number in DHCP packet is not valid
  EFI_INVALID_PARAMETER := If DHCP_MAX_MESSAGE_SIZE option is present and
    is not valid
  EFI_INVALID_PARAMETER := If DHCP_OPTION_OVERLOAD option is present and
    is not valid
  EFI_DEVICE_ERROR := Cannot determine end of packet
  EFI_BUFFER_TOO_SMALL := Not enough room in packet to add option
  EFI_SUCCESS := Option added to DHCP packet
--*/
{
  EFI_STATUS  efi_status;
  DHCP4_OP    *msg_size_op;
  DHCP4_OP    *overload_op;
  DHCP4_OP    *op;
  UINTN       msg_size;
  UINTN       buf_len;
  UINT32      magik;
  UINT8       *buf;

  //
  // Verify parameters.
  //
  ASSERT (Packet);
  ASSERT (OpPtr);

  if (Packet == NULL || OpPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (OpPtr->op) {
  case DHCP4_PAD:
  case DHCP4_END:
    //
    // No adding PAD or END.
    //
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check the DHCP magik number.
  //
  CopyMem (&magik, &Packet->dhcp4.magik, 4);

  if (magik != htonl (DHCP4_MAGIK_NUMBER)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the DHCP message size option.
  //
  msg_size = DHCP4_DEFAULT_MAX_MESSAGE_SIZE;

  efi_status = find_opt (
                Packet,
                DHCP4_MAX_MESSAGE_SIZE,
                0,
                &msg_size_op
                );

  if (EFI_ERROR (efi_status)) {
    if (efi_status != EFI_NOT_FOUND) {
      DebugPrint (
        ("%s:%d:%r\n",
        __FILE__,
        __LINE__,
        efi_status)
        );
      return efi_status;
    }

    msg_size_op = NULL;
  } else {
    CopyMem (&msg_size, msg_size_op->data, 2);
    msg_size = htons (msg_size);

    if (msg_size < DHCP4_DEFAULT_MAX_MESSAGE_SIZE) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Find the DHCP option overload option.
  //
  efi_status = find_opt (
                Packet,
                DHCP4_OPTION_OVERLOAD,
                0,
                &overload_op
                );

  if (EFI_ERROR (efi_status)) {
    if (efi_status != EFI_NOT_FOUND) {
      DebugPrint (
        ("%s:%d:%r\n",
        __FILE__,
        __LINE__,
        efi_status)
        );
      return efi_status;
    }

    overload_op = NULL;
  } else {
    if (overload_op->len != 1) {
      return EFI_INVALID_PARAMETER;
    }

    switch (overload_op->data[0]) {
    case 1:
    case 2:
    case 3:
      break;

    default:
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Find the end of the packet.
  //
  efi_status = find_opt (Packet, DHCP4_END, 0, &op);

  if (EFI_ERROR (efi_status)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find which buffer the end is in.
  //
  if ((UINTN) op >= (UINTN) (buf = Packet->dhcp4.options)) {
    buf_len = (msg_size - ((UINT8 *) &Packet->dhcp4.options - (UINT8 *) &Packet->raw)) - (DHCP4_UDP_HEADER_SIZE + DHCP4_IP_HEADER_SIZE);
  } else if ((UINTN) op >= (UINTN) (buf = Packet->dhcp4.fname)) {
    buf_len = 128;
  } else if ((UINTN) op >= (UINTN) (buf = Packet->dhcp4.sname)) {
    buf_len = 64;
  } else {
    return EFI_DEVICE_ERROR;
  }
  //
  // Add option to current buffer if there is no overlow.
  //
  if ((UINTN) ((&op->op - buf) + 3 + op->len) < buf_len) {
    CopyMem (op, OpPtr, OpPtr->len + 2);

    op->data[op->len] = DHCP4_END;

    return EFI_SUCCESS;
  }
  //
  // Error if there is no space for option.
  //
  return EFI_BUFFER_TOO_SMALL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
start_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN OPTIONAL EFI_IP_ADDRESS         *StationIp,
  IN OPTIONAL EFI_IP_ADDRESS         *SubnetMask
  )
/*++
Routine description:
  Setup PXE BaseCode UDP stack.

Parameters:
  Private := Pointer to PxeDhcp4 private data.
  StationIp := Pointer to IP address or NULL if not known.
  SubnetMask := Pointer to subnet mask or NULL if not known.

Returns:
  EFI_INVALID_PARAMETER := Private == NULL || Private->PxeBc == NULL
  EFI_INVALID_PARAMETER := Only one of StationIp and SubnetMask is given
  EFI_SUCCESS := UDP stack is ready
  other := Error from PxeBc->SetIpFilter() or PxeBc->SetStationIp()
--*/
{
  EFI_PXE_BASE_CODE_IP_FILTER bcast_filter;
  EFI_STATUS                  efi_status;

  //
  //
  //
  ASSERT (Private);
  ASSERT (Private->PxeBc);

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->PxeBc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (StationIp != NULL && SubnetMask == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (StationIp == NULL && SubnetMask != NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Setup broadcast receive filter...
  //
  ZeroMem (&bcast_filter, sizeof (EFI_PXE_BASE_CODE_IP_FILTER));

  bcast_filter.Filters  = EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST;
  bcast_filter.IpCnt    = 0;

  efi_status = Private->PxeBc->SetIpFilter (
                                Private->PxeBc,
                                &bcast_filter
                                );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    return efi_status;
  }
  //
  // Configure station IP address and subnet mask...
  //
  efi_status = Private->PxeBc->SetStationIp (
                                Private->PxeBc,
                                StationIp,
                                SubnetMask
                                );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
  }

  return efi_status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
stop_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private
  )
{
  //
  //
  //
  ASSERT (Private);
  ASSERT (Private->PxeBc);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
start_receive_events (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN UINTN                  SecondsTimeout
  )
/*++
Routine description:
  Create periodic and timeout receive events.

Parameters:
  Private := Pointer to PxeDhcp4 private data.
  SecondsTimeout := Number of seconds to wait before timeout.

Returns:
--*/
{
  EFI_STATUS  efi_status;
  UINTN       random;

  //
  //
  //
  ASSERT (Private);
  ASSERT (SecondsTimeout);

  if (Private == NULL || SecondsTimeout == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Need a bettern randomizer...
  // For now adjust the timeout value by the least significant
  // digit in the MAC address.
  //
  random = 0;

  if (Private->PxeDhcp4.Data != NULL) {
    if (Private->PxeDhcp4.Data->Discover.dhcp4.hlen != 0 && Private->PxeDhcp4.Data->Discover.dhcp4.hlen <= 16) {
      random = 0xFFF & Private->PxeDhcp4.Data->Discover.dhcp4.chaddr[Private->PxeDhcp4.Data->Discover.dhcp4.hlen - 1];
    }
  }
  //
  // Setup timeout event and start timer.
  //
  efi_status = gBS->CreateEvent (
                      EVT_TIMER | EVT_NOTIFY_SIGNAL,
                      TPL_NOTIFY,
                      &timeout_notify,
                      Private,
                      &Private->TimeoutEvent
                      );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    return efi_status;
  }

  efi_status = gBS->SetTimer (
                      Private->TimeoutEvent,
                      TimerRelative,
                      SecondsTimeout * 10000000 + random
                      );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    gBS->CloseEvent (Private->TimeoutEvent);
    return efi_status;
  }

  Private->TimeoutOccurred = FALSE;

  //
  // Setup periodic event for callbacks
  //
  efi_status = gBS->CreateEvent (
                      EVT_TIMER | EVT_NOTIFY_SIGNAL,
                      TPL_NOTIFY,
                      &periodic_notify,
                      Private,
                      &Private->PeriodicEvent
                      );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    gBS->CloseEvent (Private->TimeoutEvent);
    return efi_status;
  }

  efi_status = gBS->SetTimer (
                      Private->PeriodicEvent,
                      TimerPeriodic,
                      1000000
                      );  /* 1/10th second */

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    gBS->CloseEvent (Private->TimeoutEvent);
    gBS->CloseEvent (Private->PeriodicEvent);
    return efi_status;
  }

  Private->PeriodicOccurred = FALSE;

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
stop_receive_events (
  IN PXE_DHCP4_PRIVATE_DATA *Private
  )
{
  //
  //
  //
  ASSERT (Private);

  if (Private == NULL) {
    return ;
  }
  //
  //
  //
  gBS->CloseEvent (Private->TimeoutEvent);
  Private->TimeoutOccurred = FALSE;

  //
  //
  //
  gBS->CloseEvent (Private->PeriodicEvent);
  Private->PeriodicOccurred = FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
tx_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN EFI_IP_ADDRESS         *dest_ip,
  IN OPTIONAL EFI_IP_ADDRESS         *gateway_ip,
  IN EFI_IP_ADDRESS         *src_ip,
  IN VOID                   *buffer,
  IN UINTN                  BufferSize
  )
/*++
Routine description:
  Transmit DHCP packet.

Parameters:
  Private := Pointer to PxeDhcp4 private data
  dest_ip := Pointer to destination IP address
  gateway_ip := Pointer to gateway IP address or NULL
  src_ip := Pointer to source IP address or NULL
  buffer := Pointer to buffer to transmit
  BufferSize := Size of buffer in bytes

Returns:
  EFI_INVALID_PARAMETER := Private == NULL || dest_ip == NULL ||
    buffer == NULL || BufferSize < 300 || Private->PxeBc == NULL
  EFI_SUCCESS := Buffer was transmitted
  other := Return from PxeBc->UdpWrite()
--*/
{
  EFI_PXE_BASE_CODE_UDP_PORT  dest_port;
  EFI_PXE_BASE_CODE_UDP_PORT  src_port;
  EFI_IP_ADDRESS              zero_ip;

  //
  //
  //
  ASSERT (Private);
  ASSERT (dest_ip);
  ASSERT (buffer);
  ASSERT (BufferSize >= 300);

  if (Private == NULL || dest_ip == NULL || buffer == NULL || BufferSize < 300) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (Private->PxeBc);

  if (Private->PxeBc == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Transmit DHCP discover packet...
  //
  ZeroMem (&zero_ip, sizeof (EFI_IP_ADDRESS));

  if (src_ip == NULL) {
    src_ip = &zero_ip;
  }

  dest_port = DHCP4_SERVER_PORT;
  src_port  = DHCP4_CLIENT_PORT;

  return Private->PxeBc->UdpWrite (
                          Private->PxeBc,
                          EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
                          dest_ip,
                          &dest_port,
                          gateway_ip,
                          src_ip,
                          &src_port,
                          NULL,
                          NULL,
                          &BufferSize,
                          buffer
                          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
rx_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  OUT VOID                  *buffer,
  IN OUT UINTN              *BufferSize,
  IN OUT EFI_IP_ADDRESS     *dest_ip,
  IN OUT EFI_IP_ADDRESS     *src_ip,
  IN UINT16                 op_flags
  )
/*++
Routine description:
  Receive DHCP packet.

Parameters:
  Private := Pointer to PxeDhcp4 private data
  buffer := Pointer to buffer to receive DHCP packet
  BufferSize := Pointer to buffer size in bytes
  dest_ip := Pointer to destination IP address
  src_ip := Pointer to source IP address
  op_flags := UDP receive operation flags

Returns:
  EFI_INVALID_PARAMETER :=
  EFI_SUCCESS := Packet received
  other := Return from PxeBc->UdpRead()
--*/
{
  EFI_PXE_BASE_CODE_UDP_PORT  dest_port;
  EFI_PXE_BASE_CODE_UDP_PORT  src_port;

  //
  //
  //
  ASSERT (Private);
  ASSERT (buffer);
  ASSERT (dest_ip);
  ASSERT (src_ip);

  if (Private == NULL || buffer == NULL || dest_ip == NULL || src_ip == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (Private->PxeBc);

  if (Private->PxeBc == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check for packet
  //
  *BufferSize = sizeof (DHCP4_PACKET);

  dest_port   = DHCP4_CLIENT_PORT;
  src_port    = DHCP4_SERVER_PORT;

  return Private->PxeBc->UdpRead (
                          Private->PxeBc,
                          op_flags,
                          dest_ip,
                          &dest_port,
                          src_ip,
                          &src_port,
                          NULL,
                          NULL,
                          BufferSize,
                          buffer
                          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
tx_rx_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN OUT EFI_IP_ADDRESS     *ServerIp,
  IN OPTIONAL EFI_IP_ADDRESS         *gateway_ip,
  IN OPTIONAL EFI_IP_ADDRESS         *client_ip,
  IN OPTIONAL EFI_IP_ADDRESS         *SubnetMask,
  IN DHCP4_PACKET           *tx_pkt,
  OUT DHCP4_PACKET          *rx_pkt,
  IN INTN (*rx_vfy)(
      IN PXE_DHCP4_PRIVATE_DATA *Private,
      IN DHCP4_PACKET *tx_pkt,
      IN DHCP4_PACKET *rx_pkt,
      IN UINTN rx_pkt_size
    ),
  IN UINTN SecondsTimeout
  )
/*++
Routine description:
  Transmit DHCP packet and wait for replies.

Parameters:
  Private := Pointer to PxeDhcp4 private data
  ServerIp := Pointer to server IP address
  gateway_ip := Pointer to gateway IP address or NULL
  client_ip := Pointer to client IP address or NULL
  SubnetMask := Pointer to subnet mask or NULL
  tx_pkt := Pointer to DHCP packet to transmit
  rx_pkt := Pointer to DHCP packet receive buffer
  rx_vfy := Pointer to DHCP packet receive verification routine
  SecondsTimeout := Number of seconds until timeout

Returns:
  EFI_INVALID_PARAMETER := Private == NULL || ServerIp == NULL ||
    tx_pkt == NULL || rx_pkt == NULL || rx_vfy == NULL || Private->PxeBc == NULL
  EFI_ABORTED := Receive aborted
  EFI_TIMEOUT := No packets received
  EFI_SUCCESS := Packet(s) received
  other := Returns from other PxeDhcp4 support routines
--*/
{
  EFI_PXE_DHCP4_CALLBACK_STATUS CallbackStatus;
  EFI_IP_ADDRESS                dest_ip;
  EFI_IP_ADDRESS                src_ip;
  EFI_STATUS                    efi_status;
  DHCP4_OP                      *msg_size_op;
  UINTN                         pkt_size;
  UINTN                         n;
  UINT16                        msg_size;
  UINT16                        op_flags;
  BOOLEAN                       done_flag;
  BOOLEAN                       got_packet;

  //
  // Bad programmer check...
  //
  ASSERT (Private);
  ASSERT (ServerIp);
  ASSERT (tx_pkt);
  ASSERT (rx_pkt);
  ASSERT (rx_vfy);

  if (Private == NULL || ServerIp == NULL || tx_pkt == NULL || rx_pkt == NULL || rx_vfy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (Private->PxeBc);

  if (Private->PxeBc == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Enable UDP...
  //
  efi_status = start_udp (Private, client_ip, SubnetMask);

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    return efi_status;
  }
  //
  // Get length of transmit packet...
  //
  msg_size = DHCP4_DEFAULT_MAX_MESSAGE_SIZE;

  efi_status = find_opt (
                tx_pkt,
                DHCP4_MAX_MESSAGE_SIZE,
                0,
                &msg_size_op
                );

  if (!EFI_ERROR (efi_status)) {
    CopyMem (&msg_size, msg_size_op->data, 2);

    if ((msg_size = htons (msg_size)) < 328) {
      msg_size = 328;
    }
  }
  //
  // Transmit packet...
  //
  efi_status = tx_udp (
                Private,
                ServerIp,
                gateway_ip,
                client_ip,
                tx_pkt,
                msg_size - (DHCP4_UDP_HEADER_SIZE + DHCP4_IP_HEADER_SIZE)
                );

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    stop_udp (Private);
    return efi_status;
  }
  //
  // Enable periodic and timeout events...
  //
  efi_status = start_receive_events (Private, SecondsTimeout);

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    stop_udp (Private);
    return efi_status;
  }
  //
  // Wait for packet(s)...
  //

  done_flag   = FALSE;
  got_packet  = FALSE;

  while (!done_flag) {
    //
    // Check for timeout event...
    //
    if (Private->TimeoutOccurred) {
      efi_status = EFI_SUCCESS;
      break;
    }
    //
    // Check for periodic event...
    //
    if (Private->PeriodicOccurred && Private->callback != NULL) {
      CallbackStatus = EFI_PXE_DHCP4_CALLBACK_STATUS_CONTINUE;

      if (Private->callback->Callback != NULL) {
        CallbackStatus = (Private->callback->Callback) (&Private->PxeDhcp4, Private->function, 0, NULL);
      }

      switch (CallbackStatus) {
      case EFI_PXE_DHCP4_CALLBACK_STATUS_CONTINUE:
        break;

      case EFI_PXE_DHCP4_CALLBACK_STATUS_ABORT:
      default:
        stop_receive_events (Private);
        stop_udp (Private);
        return EFI_ABORTED;
      }

      Private->PeriodicOccurred = FALSE;
    }
    //
    // Check for packet...
    //
    if (client_ip == NULL) {
      SetMem (&dest_ip, sizeof (EFI_IP_ADDRESS), 0xFF);
    } else {
      CopyMem (&dest_ip, client_ip, sizeof (EFI_IP_ADDRESS));
    }

    SetMem (&src_ip, sizeof (EFI_IP_ADDRESS), 0xFF);

    if (CompareMem (&src_ip, &ServerIp, sizeof (EFI_IP_ADDRESS))) {
      ZeroMem (&src_ip, sizeof (EFI_IP_ADDRESS));
      op_flags = EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP;
    } else {
      op_flags = 0;
    }

    efi_status = rx_udp (
                  Private,
                  rx_pkt,
                  &pkt_size,
                  &dest_ip,
                  &src_ip,
                  op_flags
                  );

    if (efi_status == EFI_TIMEOUT) {
      efi_status = EFI_SUCCESS;
      continue;
    }

    if (EFI_ERROR (efi_status)) {
      break;
    }
    //
    // Some basic packet sanity checks..
    //
    if (pkt_size < 300) {
      continue;
    }

    if (rx_pkt->dhcp4.op != BOOTP_REPLY) {
      continue;
    }

    if (tx_pkt->dhcp4.htype != rx_pkt->dhcp4.htype) {
      continue;
    }

    if ((n = tx_pkt->dhcp4.hlen) != rx_pkt->dhcp4.hlen) {
      continue;
    }

    if (CompareMem (&tx_pkt->dhcp4.xid, &rx_pkt->dhcp4.xid, 4)) {
      continue;
    }

    if (n != 0) {
      if (n >= 16) {
        n = 16;
      }

      if (CompareMem (tx_pkt->dhcp4.chaddr, rx_pkt->dhcp4.chaddr, n)) {
        continue;
      }
    }
    //
    // Internal callback packet verification...
    //
    switch ((*rx_vfy) (Private, tx_pkt, rx_pkt, pkt_size)) {
    case -2:  /* ignore and stop */
      stop_receive_events (Private);
      stop_udp (Private);
      return EFI_ABORTED;

    case -1:  /* ignore and wait */
      continue;

    case 0:   /* accept and wait */
      break;

    case 1:   /* accept and stop */
      done_flag = TRUE;
      break;

    default:
      ASSERT (0);
    }
    //
    // External callback packet verification...
    //
    CallbackStatus = EFI_PXE_DHCP4_CALLBACK_STATUS_KEEP_CONTINUE;

    if (Private->callback != NULL) {
      if (Private->callback->Callback != NULL) {
        CallbackStatus = (Private->callback->Callback) (&Private->PxeDhcp4, Private->function, (UINT32) pkt_size, rx_pkt);
      }
    }

    switch (CallbackStatus) {
    case EFI_PXE_DHCP4_CALLBACK_STATUS_IGNORE_CONTINUE:
      continue;

    case EFI_PXE_DHCP4_CALLBACK_STATUS_KEEP_ABORT:
      done_flag = TRUE;
      break;

    case EFI_PXE_DHCP4_CALLBACK_STATUS_IGNORE_ABORT:
      stop_receive_events (Private);
      stop_udp (Private);
      return EFI_ABORTED;

    case EFI_PXE_DHCP4_CALLBACK_STATUS_KEEP_CONTINUE:
    default:
      break;
    }
    //
    // We did!  We did get a packet!
    //
    got_packet = TRUE;
  }
  //
  //
  //
  stop_receive_events (Private);
  stop_udp (Private);

  if (EFI_ERROR (efi_status)) {
    DebugPrint (("%s:%d:%r\n", __FILE__, __LINE__, efi_status));
    return efi_status;
  }

  if (got_packet) {
    return EFI_SUCCESS;
  } else {
    return EFI_TIMEOUT;
  }
}

/* eof - support.c */
