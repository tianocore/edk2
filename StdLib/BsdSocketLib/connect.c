/** @file
  Implement the connect API.

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
  Connect to a remote system via the network.

  The connect routine attempts to establish a connection to a
  socket on the local or remote system using the specified address.

  There are three states associated with a connection:
  <ul>
    <li>Not connected</li>
    <li>Connection in progress</li>
    <li>Connected</li>
  </ul>
  In the initial "Not connected" state, calls to connect start the connection
  processing and update the state to "Connection in progress".  During
  the "Connection in progress" state, connect polls for connection completion
  and moves the state to "Connected" after the connection is established.
  Note that these states are only visible when the file descriptor is marked
  with O_NONBLOCK.  Also, the POLLOUT bit is set when the connection
  completes and may be used by poll or select as an indicator to call
  connect again.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html">POSIX</a>
  documentation is available online.
  
  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] address   Network address of the remote system

  @param [in] address_len Length of the remote network address

  @return     This routine returns zero if successful and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
connect (
  int s,
  const struct sockaddr * address,
  socklen_t address_len
  )
{
  BOOLEAN bBlocking;
  int ConnectStatus;
  struct __filedes * pDescriptor;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Locate the context for this socket
  //
  pSocketProtocol = BslFdToSocketProtocol ( s,
                                            &pDescriptor,
                                            &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Determine if the operation is blocking
    //
    bBlocking = (BOOLEAN)( 0 == ( pDescriptor->Oflags & O_NONBLOCK ));

    //
    //  Attempt to connect to a remote system
    //
    do {
      errno = 0;
      Status = pSocketProtocol->pfnConnect ( pSocketProtocol,
                                             address,
                                             address_len,
                                             &errno );
    } while ( bBlocking && ( EFI_NOT_READY == Status ));
  }

  //
  //  Return the new socket file descriptor
  //
  ConnectStatus = (0 == errno) ? 0 : -1;
  return ConnectStatus;
}
