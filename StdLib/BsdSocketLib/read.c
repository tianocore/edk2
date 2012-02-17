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

  The BslSocketRead routine is called indirectly by the read file
  system routine.  This routine is typically used for SOCK_STREAM
  because it waits for receive data from the target system specified
  in the ::connect call.

  @param [in] pDescriptor   Descriptor address for the file
  @param [in] pOffset       File offset
  @param [in] LengthInBytes Number of bytes to read
  @param [in] pBuffer       Address of the buffer to receive the data

  @return   The number of bytes read or -1 if an error occurs.
            In the case of an error, ::errno contains more details.

**/
ssize_t
EFIAPI
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
