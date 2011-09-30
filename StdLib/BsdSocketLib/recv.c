/** @file
  Implement the recv API.

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
  Receive data from a network connection.

  The recv routine waits for receive data from a remote network
  connection.  This routine is typically used for SOCK_STREAM
  because it waits for receive data from the target system specified
  in the ::connect call.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] buffer    Address of a buffer to receive the data.

  @param [in] length    Length of the buffer in bytes.

  @param [in] flags     Message control flags

  @return     This routine returns the number of valid bytes in the buffer,
              zero if no data was received, and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
ssize_t
recv (
  int s,
  void * buffer,
  size_t length,
  int flags
  )
{
  ssize_t BytesRead;

  //
  //  Receive the data from the remote system
  //
  BytesRead = recvfrom ( s,
                         buffer,
                         length,
                         flags,
                         NULL,
                         NULL );

  //
  //  Return the number of bytes read
  //
  return BytesRead;
}
