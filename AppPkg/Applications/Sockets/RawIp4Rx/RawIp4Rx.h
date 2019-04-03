/** @file
  Definitions for the raw IP4 receive application

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RAW_IP4_RX_H_
#define _RAW_IP4_RX_H_

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

#ifdef  BUILD_FOR_WINDOWS
//
//  Build for Windows environment
//

#include <winsock2.h>

#define CLOSE_SOCKET      closesocket
#define SIN_ADDR(port)    port.sin_addr.S_un.S_addr
#define SIN_FAMILY(port)  port.sin_family
#define SIN_LEN(port)     port.sin_family
#define SIN_PORT(port)    port.sin_port
#define GET_ERRNO         WSAGetLastError ( )

#define ssize_t           int
#define socklen_t         int

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
#include <sys/time.h>

#define CLOSE_SOCKET      close
#define SIN_ADDR(port)    port.sin_addr.s_addr
#define SIN_FAMILY(port)  port.sin_family
#define SIN_LEN(port)     port.sin_len
#define SIN_PORT(port)    port.sin_port
#define SOCKET            int
#define GET_ERRNO         errno

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
  Run the raw IP4 receive application

  @param [in] ArgC      Argument count
  @param [in] ArgV      Argument value array

  @retval 0             Successfully operation
 **/

int
RawIp4Rx (
  IN int ArgC,
  IN char **ArgV
  );

//------------------------------------------------------------------------------

#endif  //  _RAW_IP4_RX_H_
