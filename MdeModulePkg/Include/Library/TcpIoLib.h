/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to access TCP service.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP_IO_H_
#define _TCP_IO_H_


#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>

#include <Library/NetLib.h>

#define TCP_VERSION_4 IP_VERSION_4
#define TCP_VERSION_6 IP_VERSION_6

///
/// 10 seconds
///
#define TCP_GET_MAPPING_TIMEOUT 100000000U


typedef struct {
  EFI_IPv4_ADDRESS          LocalIp;
  EFI_IPv4_ADDRESS          SubnetMask;
  EFI_IPv4_ADDRESS          Gateway;

  UINT16                    StationPort;
  EFI_IPv4_ADDRESS          RemoteIp;
  UINT16                    RemotePort;
  BOOLEAN                   ActiveFlag;
} TCP4_IO_CONFIG_DATA;

typedef struct {
  UINT16                    StationPort;
  EFI_IPv6_ADDRESS          RemoteIp;
  UINT16                    RemotePort;
  BOOLEAN                   ActiveFlag;
} TCP6_IO_CONFIG_DATA;

typedef union {
  TCP4_IO_CONFIG_DATA       Tcp4IoConfigData;
  TCP6_IO_CONFIG_DATA       Tcp6IoConfigData;
} TCP_IO_CONFIG_DATA;

typedef union {
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
} TCP_IO_PROTOCOL;

typedef union {
  EFI_TCP4_CONNECTION_TOKEN Tcp4Token;
  EFI_TCP6_CONNECTION_TOKEN Tcp6Token;
} TCP_IO_CONNECTION_TOKEN;

typedef union {
  EFI_TCP4_IO_TOKEN         Tcp4Token;
  EFI_TCP6_IO_TOKEN         Tcp6Token;
} TCP_IO_IO_TOKEN;

typedef union {
  EFI_TCP4_CLOSE_TOKEN      Tcp4Token;
  EFI_TCP6_CLOSE_TOKEN      Tcp6Token;
} TCP_IO_CLOSE_TOKEN;

typedef union {
  EFI_TCP4_LISTEN_TOKEN     Tcp4Token;
  EFI_TCP6_LISTEN_TOKEN     Tcp6Token;
} TCP_IO_LISTEN_TOKEN;


typedef struct {
  UINT8                     TcpVersion;
  EFI_HANDLE                Image;
  EFI_HANDLE                Controller;
  EFI_HANDLE                Handle;
  
  TCP_IO_PROTOCOL           Tcp;
  TCP_IO_PROTOCOL           NewTcp;
  TCP_IO_CONNECTION_TOKEN   ConnToken;
  TCP_IO_IO_TOKEN           TxToken;
  TCP_IO_IO_TOKEN           RxToken;
  TCP_IO_CLOSE_TOKEN        CloseToken;
  TCP_IO_LISTEN_TOKEN       ListenToken;
  
  BOOLEAN                   IsConnDone;
  BOOLEAN                   IsTxDone;
  BOOLEAN                   IsRxDone;
  BOOLEAN                   IsCloseDone;
  BOOLEAN                   IsListenDone;
} TCP_IO;

/**
  Create a TCP socket with the specified configuration data. 

  @param[in]  Image      The handle of the driver image.
  @param[in]  Controller The handle of the controller.
  @param[in]  TcpVersion The version of Tcp, TCP_VERSION_4 or TCP_VERSION_6.
  @param[in]  ConfigData The Tcp configuration data.
  @param[out] TcpIo      The TcpIo.
  
  @retval EFI_SUCCESS            The TCP socket is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the TCP socket or configure it.

**/
EFI_STATUS
EFIAPI
TcpIoCreateSocket (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  TcpVersion,
  IN TCP_IO_CONFIG_DATA     *ConfigData,
  OUT TCP_IO                *TcpIo
  );

/**
  Destroy the socket. 

  @param[in]  TcpIo The TcpIo which wraps the socket to be destroyed.

**/
VOID
EFIAPI
TcpIoDestroySocket (
  IN TCP_IO                 *TcpIo
  );

/**
  Connect to the other endpoint of the TCP socket.

  @param[in, out]  TcpIo     The TcpIo wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.
  
  @retval EFI_SUCCESS            Connect to the other endpoint of the TCP socket
                                 successfully.
  @retval EFI_TIMEOUT            Failed to connect to the other endpoint of the
                                 TCP socket in the specified time period.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoConnect (
  IN OUT TCP_IO             *TcpIo,
  IN     EFI_EVENT          Timeout
  );

/**
  Accept the incomding request from the other endpoint of the TCP socket.

  @param[in, out]  TcpIo     The TcpIo wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.

  
  @retval EFI_SUCCESS            Connect to the other endpoint of the TCP socket
                                 successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.

  @retval EFI_TIMEOUT            Failed to connect to the other endpoint of the
                                 TCP socket in the specified time period.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoAccept (
  IN OUT TCP_IO             *TcpIo,
  IN     EFI_EVENT          Timeout
  );
  
/**
  Reset the socket.

  @param[in, out]  TcpIo The TcpIo wrapping the TCP socket.

**/
VOID
EFIAPI
TcpIoReset (
  IN OUT TCP_IO             *TcpIo
  );

/**
  Transmit the Packet to the other endpoint of the socket.

  @param[in]   TcpIo           The TcpIo wrapping the TCP socket.
  @param[in]   Packet          The packet to transmit.
  
  @retval EFI_SUCCESS            The packet is trasmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoTransmit (
  IN TCP_IO                 *TcpIo,
  IN NET_BUF                *Packet
  );

/**
  Receive data from the socket.

  @param[in, out]  TcpIo       The TcpIo which wraps the socket to be destroyed.
  @param[in]       Packet      The buffer to hold the data copy from the socket rx buffer.
  @param[in]       AsyncMode   Is this receive asyncronous or not.
  @param[in]       Timeout     The time to wait for receiving the amount of data the Packet
                               can hold.

  @retval EFI_SUCCESS            The required amount of data is received from the socket.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate momery.
  @retval EFI_TIMEOUT            Failed to receive the required amount of data in the
                                 specified time period.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoReceive (
  IN OUT TCP_IO             *TcpIo,
  IN     NET_BUF            *Packet,
  IN     BOOLEAN            AsyncMode,
  IN     EFI_EVENT          Timeout
  );

#endif

