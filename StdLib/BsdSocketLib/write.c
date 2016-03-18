/** @file
  Implement the write API.

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
  Write support routine for sockets

  @param [in] pDescriptor   Descriptor address for the file
  @param [in] pOffset       File offset
  @param [in] LengthInBytes Number of bytes to write
  @param [in] pBuffer       Address of the data

  @return   The number of bytes written or -1 if an error occurs.
            In the case of an error, ::errno contains more details.

**/
ssize_t
EFIAPI
BslSocketWrite (
  struct __filedes *pDescriptor,
  off_t * pOffset,
  size_t LengthInBytes,
  const void * pBuffer
  )
{
  ssize_t BytesWritten;

  //
  //  Send the data using the socket
  //
  BytesWritten = send ( pDescriptor->MyFD,
                        pBuffer,
                        LengthInBytes,
                        0 );

  //
  //  Return the number of bytes written
  //
  return BytesWritten;
}
