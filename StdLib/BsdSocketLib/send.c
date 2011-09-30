/** @file
  Implement the send API.

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
  Send data using a network connection.

  The send routine queues data to the network for transmission.
  This routine is typically used for SOCK_STREAM sockets where the target
  system was specified in the ::connect call.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/send.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] buffer    Address of a buffer containing the data to send.

  @param [in] length    Length of the buffer in bytes.

  @param [in] flags     Message control flags

  @return     This routine returns the number of data bytes that were
              sent and -1 when an error occurs.  In the case of
              an error, ::errno contains more details.

 **/
ssize_t
send (
  int s,
  CONST void * buffer,
  size_t length,
  int flags
  )
{
  //
  //  Send the data
  //
  return sendto ( s, buffer, length, flags, NULL, 0 );
}
