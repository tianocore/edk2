/** @file
  Implement the listen API.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <SocketInternals.h>


/** Establish the known port to listen for network connections.

  The listen routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::accept routine to remove
  the next connection from the queue and get the associated socket.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] backlog   backlog specifies the maximum FIFO depth for the connections
                        waiting for the application to call ::accept.  Connection attempts
                        received while the queue is full are refused.

  @return     This routine returns zero (0) if successful or -1 when an error occurs.
              In the case of an error, ::errno contains more details.
 **/
int
listen (
  IN int s,
  IN int backlog
  )
{
  int ListenStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //  Locate the context for this socket
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //  Enable connections on the known port
    (void) pSocketProtocol->pfnListen ( pSocketProtocol,
                                          backlog,
                                          &errno );
  }
  //  Return the operation stauts
  ListenStatus = ( 0 == errno ) ? 0 : -1;
  return ListenStatus;
}
