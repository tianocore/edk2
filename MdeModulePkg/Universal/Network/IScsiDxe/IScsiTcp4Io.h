/** @file
  iSCSI Tcp4 IO related definitions.

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_TCP4_IO_H_
#define _ISCSI_TCP4_IO_H_

#include <Library/NetLib.h>
#include <Protocol/Tcp4.h>

typedef struct _TCP4_IO_CONFIG_DATA {
  EFI_IPv4_ADDRESS  LocalIp;
  EFI_IPv4_ADDRESS  SubnetMask;
  EFI_IPv4_ADDRESS  Gateway;

  EFI_IPv4_ADDRESS  RemoteIp;
  UINT16            RemotePort;
} TCP4_IO_CONFIG_DATA;

typedef struct _TCP4_IO {
  EFI_HANDLE                Image;
  EFI_HANDLE                Controller;

  EFI_HANDLE                Handle;
  EFI_TCP4_PROTOCOL         *Tcp4;

  EFI_TCP4_CONNECTION_TOKEN ConnToken;
  EFI_TCP4_IO_TOKEN         TxToken;
  EFI_TCP4_IO_TOKEN         RxToken;
  EFI_TCP4_CLOSE_TOKEN      CloseToken;

  BOOLEAN                   IsConnDone;
  BOOLEAN                   IsTxDone;
  BOOLEAN                   IsRxDone;
  BOOLEAN                   IsCloseDone;
} TCP4_IO;

/**
  Create a TCP socket with the specified configuration data. 

  @param[in]  Image      The handle of the driver image.
  @param[in]  Controller The handle of the controller.
  @param[in]  ConfigData The Tcp4 configuration data.
  @param[in]  Tcp4Io     The Tcp4Io.
  
  @retval EFI_SUCCESS    The TCP socket is created and configured.
  @retval Others         Failed to create the TCP socket or configure it.
**/
EFI_STATUS
Tcp4IoCreateSocket (
  IN EFI_HANDLE           Image,
  IN EFI_HANDLE           Controller,
  IN TCP4_IO_CONFIG_DATA  *ConfigData,
  IN TCP4_IO              *Tcp4Io
  );

/**
  Destroy the socket. 

  @param[in]  Tcp4Io The Tcp4Io which wraps the socket to be destroyeds.
**/
VOID
Tcp4IoDestroySocket (
  IN TCP4_IO  *Tcp4Io
  );

/**
  Connect to the other endpoint of the TCP socket.

  @param[in, out]  Tcp4Io    The Tcp4Io wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.
  
  @retval EFI_SUCCESS          Connect to the other endpoint of the TCP socket successfully.
  @retval EFI_TIMEOUT          Failed to connect to the other endpoint of the TCP socket in the                               specified time period.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoConnect (
  IN OUT TCP4_IO    *Tcp4Io,
  IN EFI_EVENT      Timeout
  );

/**
  Reset the socket.

  @param[in, out]  Tcp4Io The Tcp4Io wrapping the TCP socket.
**/
VOID
Tcp4IoReset (
  IN OUT TCP4_IO  *Tcp4Io
  );

/**
  Transmit the Packet to the other endpoint of the socket.

  @param[in]   Tcp4Io          The Tcp4Io wrapping the TCP socket.
  @param[in]   Packet          The packet to transmit.
  
  @retval EFI_SUCCESS          The packet is trasmitted.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoTransmit (
  IN TCP4_IO  *Tcp4Io,
  IN NET_BUF  *Packet
  );

/**
  Receive data from the socket.

  @param[in]  Tcp4Io           The Tcp4Io which wraps the socket to be destroyed.
  @param[in]  Packet           The buffer to hold the data copy from the soket rx buffer.
  @param[in]  AsyncMode        Is this receive asyncronous or not.
  @param[in]  Timeout          The time to wait for receiving the amount of data the Packet
                               can hold.

  @retval EFI_SUCCESS          The required amount of data is received from the socket.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate momery.
  @retval EFI_TIMEOUT          Failed to receive the required amount of data in the
                               specified time period.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoReceive (
  IN TCP4_IO    *Tcp4Io,
  IN NET_BUF    *Packet,
  IN BOOLEAN    AsyncMode,
  IN EFI_EVENT  Timeout
  );

#endif
