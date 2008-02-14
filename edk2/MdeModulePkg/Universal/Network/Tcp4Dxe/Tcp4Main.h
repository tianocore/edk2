/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Main.h

Abstract:


**/

#ifndef _TCP4_MAIN_H_
#define _TCP4_MAIN_H_

#include "Socket.h"

#include "Tcp4Proto.h"
#include "Tcp4Driver.h"


extern UINT16  mTcp4RandomPort;

//
// Driver Produced Protocol Prototypes
//

//
// Function prototype for the Tcp4 socket request handler
//
EFI_STATUS
Tcp4Dispatcher (
  IN SOCKET                  *Sock,
  IN SOCK_REQUEST            Request,
  IN VOID                    *Data    OPTIONAL
  );

typedef struct _TCP4_MODE_DATA {
  EFI_TCP4_CONNECTION_STATE       *Tcp4State;
  EFI_TCP4_CONFIG_DATA            *Tcp4ConfigData;
  EFI_IP4_MODE_DATA               *Ip4ModeData;
  EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE         *SnpModeData;
} TCP4_MODE_DATA;

typedef struct _TCP4_ROUTE_INFO {
  BOOLEAN           DeleteRoute;
  EFI_IPv4_ADDRESS  *SubnetAddress;
  EFI_IPv4_ADDRESS  *SubnetMask;
  EFI_IPv4_ADDRESS  *GatewayAddress;
} TCP4_ROUTE_INFO;

//
// Get the mode data of a TCP instance
//
EFI_STATUS
EFIAPI
Tcp4GetModeData (
  IN  CONST EFI_TCP4_PROTOCOL                  * This,
  OUT       EFI_TCP4_CONNECTION_STATE          * Tcp4State OPTIONAL,
  OUT       EFI_TCP4_CONFIG_DATA               * Tcp4ConfigData OPTIONAL,
  OUT       EFI_IP4_MODE_DATA                  * Ip4ModeData OPTIONAL,
  OUT       EFI_MANAGED_NETWORK_CONFIG_DATA    * MnpConfigData OPTIONAL,
  OUT       EFI_SIMPLE_NETWORK_MODE            * SnpModeData OPTIONAL
  );

//
// Initialize or reset a TCP instance
//
EFI_STATUS
EFIAPI
Tcp4Configure (
  IN EFI_TCP4_PROTOCOL        * This,
  IN EFI_TCP4_CONFIG_DATA     * TcpConfigData OPTIONAL
  );

//
// Add a route entry to the route table
//
EFI_STATUS
EFIAPI
Tcp4Routes (
  IN EFI_TCP4_PROTOCOL           *This,
  IN BOOLEAN                     DeleteRoute,
  IN EFI_IPv4_ADDRESS            *SubnetAddress,
  IN EFI_IPv4_ADDRESS            *SubnetMask,
  IN EFI_IPv4_ADDRESS            *GatewayAddress
  );

//
// Issue an asynchronous connection establishment
// request to the peer
//
EFI_STATUS
EFIAPI
Tcp4Connect (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CONNECTION_TOKEN   *ConnectionToken
  );

//
// Issue an asynchronous listent token to accept an
// incoming connection reques
//
EFI_STATUS
EFIAPI
Tcp4Accept (
  IN EFI_TCP4_PROTOCOL             *This,
  IN EFI_TCP4_LISTEN_TOKEN         *ListenToken
  );

//
// Issue an asynchronous IO token to transmit some data
// through this TCP instance
//
EFI_STATUS
EFIAPI
Tcp4Transmit (
  IN EFI_TCP4_PROTOCOL            *This,
  IN EFI_TCP4_IO_TOKEN            *Token
  );

//
// Issue an asynchronous IO token to receive some data
// through this TCP instance
//
EFI_STATUS
Tcp4Receive (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_IO_TOKEN           *Token
  );

//
// Issue an asynchronous CloseToken to close a TCP
// connection represented by instance
//
EFI_STATUS
EFIAPI
Tcp4Close (
  IN EFI_TCP4_PROTOCOL           *This,
  IN EFI_TCP4_CLOSE_TOKEN        *CloseToken
  );

//
// cancle an connect, listent or IO token
//
EFI_STATUS
EFIAPI
Tcp4Cancel (
  IN EFI_TCP4_PROTOCOL           * This,
  IN EFI_TCP4_COMPLETION_TOKEN   * Token OPTIONAL
  );

//
// poll data from NIC for receive
//
EFI_STATUS
EFIAPI
Tcp4Poll (
  IN EFI_TCP4_PROTOCOL        *This
  );

#endif
