/** @file
  Implement the read API.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <SocketInternals.h>


/**
  Read support routine for sockets

  @param [in] pDescriptor   Descriptor address for the file
  @param [in] pOffset       File offset
  @param [in] LengthInBytes Number of bytes to read
  @param [in] pBuffer       Address of the buffer to receive the data

  @returns  The number of bytes read or -1 if an error occurs.

**/
ssize_t
BslSocketRead (
  struct __filedes *pDescriptor,
  off_t * pOffset,
  size_t LengthInBytes,
  void * pBuffer
  )
{
  ssize_t BytesRead;

  //
  //  Receive the data from the remote system
  //
  BytesRead = recvfrom ( pDescriptor->MyFD,
                         pBuffer,
                         LengthInBytes,
                         0,
                         NULL,
                         NULL );

  //
  //  Return the number of bytes read
  //
  return BytesRead;
}
