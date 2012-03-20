/** @file
  Definitions for the raw IP4 transmit application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _RAW_IP4_TX_H_
#define _RAW_IP4_TX_H_

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

#ifdef  BUILD_FOR_WINDOWS
//
//  Build for Windows environment
//

#include <winsock2.h>

#define CHAR8             char
#define CLOSE_SOCKET      closesocket
#define EINVAL            22    //  Invalid argument
#define GET_ERRNO         WSAGetLastError ( )
#define SIN_ADDR(port)    port.sin_addr.S_un.S_addr
#define SIN_FAMILY(port)  port.sin_family
#define SIN_LEN(port)     port.sin_family
#define SIN_PORT(port)    port.sin_port
#define socklen_t         int
#define ssize_t           int

#else   //  BUILD_FOR_WINDOWS
//
//  Build for EFI environment
//

#include <Uefi.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include <sys/EfiSysCall.h>
#include <sys/endian.h>
#include <sys/socket.h>

#define CLOSE_SOCKET      close
#define GET_ERRNO         errno
#define SIN_ADDR(port)    port.sin_addr.s_addr
#define SIN_FAMILY(port)  port.sin_family
#define SIN_LEN(port)     port.sin_len
#define SIN_PORT(port)    port.sin_port
#define SOCKET            int

#endif  //  BUILD_FOR_WINDOWS

#include <stdio.h>

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

//
//  See http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xml
//  and http://tools.ietf.org/html/rfc3692
//
#define RAW_PROTOCOL      253

//------------------------------------------------------------------------------
//  API
//------------------------------------------------------------------------------

/**
  Transmit raw IP4 packets to the remote system.

  @param [in] ArgC        Argument count
  @param [in] ArgV        Argument value array

  @retval 0               Successfully operation
 **/

int
RawIp4Tx (
  IN int ArgC,
  IN char **ArgV
  );

//------------------------------------------------------------------------------

#endif  //  _RAW_IP4_TX_H_