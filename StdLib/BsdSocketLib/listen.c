/** @file
  Implement the listen API.

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
  Establish the known port to listen for network connections.

  The ::listen routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::accept routine to remove
  the next connection from the queue and get the associated socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation for the bind routine is available online for reference.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] backlog   backlog specifies the maximum FIFO depth for the connections
                        waiting for the application to call accept.  Connection attempts
                        received while the queue is full are refused.

  @returns    The listen routine returns zero (0) if successful and -1 upon failure.

 **/
int
listen (
  IN int s,
  IN int backlog
  )
{
  int ListenStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Locate the context for this socket
  //
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Enable connections on the known port
    //
    Status = pSocketProtocol->pfnListen ( pSocketProtocol,
                                          backlog,
                                          &errno );
  }

  //
  //  Return the operation stauts
  //
  ListenStatus = ( 0 == errno ) ? 0 : -1;
  return ListenStatus;
}
