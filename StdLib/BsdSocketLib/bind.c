/** @file
  Implement the bind API.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <SocketInternals.h>


/** Bind a name to a socket.

  The bind routine connects a name (network address) to a socket on the local machine.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html">POSIX</a>
  documentation is available online.

  @param[in] s         Socket file descriptor returned from ::socket.

  @param[in] name      Address of a sockaddr structure that contains the
                        connection point on the local machine.  An IPv4 address
                        of INADDR_ANY specifies that the connection is made to
                        all of the network stacks on the platform.  Specifying a
                        specific IPv4 address restricts the connection to the
                        network stack supporting that address.  Specifying zero
                        for the port causes the network layer to assign a port
                        number from the dynamic range.  Specifying a specific
                        port number causes the network layer to use that port.

  @param[in] namelen   Specifies the length in bytes of the sockaddr structure.

  @return     The bind routine returns zero (0) if successful and -1 upon failure.
              In the case of an error, ::errno contains more information.
 **/
int
bind (
  IN int s,
  IN const struct sockaddr * name,
  IN socklen_t namelen
  )
{
  int BindStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //  Locate the context for this socket
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {

    //  Bind the socket
    (void) pSocketProtocol->pfnBind ( pSocketProtocol,
                                        name,
                                        namelen,
                                        &errno );
  }

  //  Return the operation stauts
  BindStatus = ( 0 == errno ) ? 0 : -1;
  return BindStatus;
}
