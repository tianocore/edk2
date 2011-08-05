/** @file
  Definitions for the socket library.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SOCKET_INTERNALS_H_
#define _SOCKET_INTERNALS_H_

#include <Uefi.h>

//----------------------------------------------------------------------
//
//  The following private files are required to support file descriptors
//

#include <kfile.h>
#include <MainData.h>

#include <Efi/SysEfi.h>

//
//  End of private files
//
//----------------------------------------------------------------------

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiSocket.h>
#include <Protocol/ServiceBinding.h>

#include <sys/errno.h>
#include <sys/poll.h>
#include <sys/EfiSysCall.h>
#include <sys/socket.h>

//------------------------------------------------------------------------------
//  Support Routines
//------------------------------------------------------------------------------

/**
  Translate from the socket file descriptor to the socket protocol.

  @param [in] s             Socket file descriptor returned from ::socket.

  @param [in] ppDescriptor  Address to receive the descriptor structure
                            address for the file
  @param [in] pErrno        Address of the errno variable

  @return   A pointer to the socket protocol structure or NULL if
            an invalid file descriptor was passed in.

 **/
EFI_SOCKET_PROTOCOL *
BslFdToSocketProtocol (
  int s,
  struct __filedes ** ppDescriptor,
  int * pErrno
  );

/**
  Close the socket

  @param [in] pDescriptor Descriptor address for the file

  @return   This routine returns 0 upon success and -1 upon failure.
            In the case of failure, errno contains more information.

**/
INT32
BslSocketClose (
  struct __filedes * pDescriptor
  );

/**
  Worker routine to close the socket.

  @param [in] pSocketProtocol   Socket protocol structure address

  @param [in] pErrno            Address of the errno variable

  @retval EFI_SUCCESS   Successfully closed the socket

**/
EFI_STATUS
BslSocketCloseWork (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  );

/**
  Poll the socket for activity

  @param [in] pDescriptor Descriptor address for the file

  @param [in] Events      Mask of events to detect

  @return     Detected events for the socket

 **/
short
BslSocketPoll (
  IN struct __filedes * pDescriptor,
  IN short Events
  );

/**
  Build a file descriptor for a socket.

  @param [in] pSocketProtocol   Socket protocol structure address

  @param [in] pErrno            Address of the errno variable

  @return  The file descriptor for the socket or -1 if an error occurs.

 **/
int
BslSocketProtocolToFd (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  );

/**
  Read support routine for sockets

  @param [in] pDescriptor   Descriptor address for the file
  @param [in] pOffset       File offset
  @param [in] LengthInBytes Number of bytes to read
  @param [in] pBuffer       Address of the buffer to receive the data

  @return   The number of bytes read or -1 if an error occurs.

**/
ssize_t
BslSocketRead (
  struct __filedes *pDescriptor,
  off_t * pOffset,
  size_t LengthInBytes,
  void * pBuffer
  );

/**
  Write support routine for sockets

  @param [in] pDescriptor   Descriptor address for the file
  @param [in] pOffset       File offset
  @param [in] LengthInBytes Number of bytes to write
  @param [in] pBuffer       Address of the data

  @return   The number of bytes written or -1 if an error occurs.

**/
ssize_t
BslSocketWrite (
  struct __filedes *pDescriptor,
  off_t * pOffset,
  size_t LengthInBytes,
  const void * pBuffer
  );

/**
  Validate the socket's file descriptor

  @param [in] pDescriptor Descriptor for the file

  @param [in] pErrno      Address of the errno variable

  @return   A pointer to the socket protocol structure or NULL if
            an invalid file descriptor was passed in.

 **/
EFI_SOCKET_PROTOCOL *
BslValidateSocketFd (
  struct __filedes * pDescriptor,
  int * pErrno
  );

//------------------------------------------------------------------------------

#endif  //  _SOCKET_INTERNALS_H_
