/**
  IScsi Tcp4 IO related definitions.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiTcp4Io.h

Abstract:

  IScsi Tcp4 IO related definitions.

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

  @param  Image[in]      The handle of the driver image.

  @param  Controller[in] The handle of the controller.

  @param  ConfigData[in] The Tcp4 configuration data.

  @param  Tcp4Io[in]     The Tcp4Io.
  
  @retval EFI_SUCCESS    The TCP socket is created and configured.

  @retval Other          Failed to create the TCP socket or configure it.

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

  @retval     None.

**/
VOID
Tcp4IoDestroySocket (
  IN TCP4_IO  *Tcp4Io
  );

/**
  Connect to the other endpoint of the TCP socket.

  @param  Tcp4Io[in]  The Tcp4Io wrapping the TCP socket.

  @param  Timeout[in] The time to wait for connection done.

  @retval None.

**/
EFI_STATUS
Tcp4IoConnect (
  IN TCP4_IO    *Tcp4Io,
  IN EFI_EVENT  Timeout
  );

/**
  Reset the socket.

  @param  Tcp4Io[in] The Tcp4Io wrapping the TCP socket.

  @retval None.

**/
VOID
Tcp4IoReset (
  IN TCP4_IO  *Tcp4Io
  );

/**
  Transmit the Packet to the other endpoint of the socket.

  @param  Tcp4Io[in]           The Tcp4Io wrapping the TCP socket.

  @param  Packet[in]           The packet to transmit

  @retval EFI_SUCCESS          The packet is trasmitted.

  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.

**/
EFI_STATUS
Tcp4IoTransmit (
  IN TCP4_IO  *Tcp4Io,
  IN NET_BUF  *Packet
  );

/**
  Receive data from the socket.

  @param  Tcp4Io[in]           The Tcp4Io which wraps the socket to be destroyeds.

  @param  Packet[in]           The buffer to hold the data copy from the soket rx buffer.

  @param  AsyncMode[in]        Is this receive asyncronous or not.

  @param  Timeout[in]          The time to wait for receiving the amount of data the Packet
                               can hold.

  @retval EFI_SUCCESS          The required amount of data is received from the socket.

  @retval EFI_OUT_OF_RESOURCES Failed to allocate momery.

  @retval EFI_TIMEOUT          Failed to receive the required amount of data in the
                               specified time period.

**/
EFI_STATUS
Tcp4IoReceive (
  IN TCP4_IO    *Tcp4Io,
  IN NET_BUF    *Packet,
  IN BOOLEAN    AsyncMode,
  IN EFI_EVENT  Timeout
  );

#endif
