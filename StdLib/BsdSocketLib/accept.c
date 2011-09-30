/** @file
  Implement the accept API.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SocketInternals.h"


/**
  Worker routine for ::accept and ::AcceptNB

  @param [in] s                 Socket file descriptor returned from ::socket.

  @param [in] bBlockingAllowed  TRUE if this is a blocking call
  @param [in] address           Address of a buffer to receive the remote network address.

  @param [in, out] address_len  Address of a buffer containing the Length in bytes
                                of the remote network address buffer.  Upon return,
                                contains the length of the remote network address.

  @return     AcceptWork returns zero if successful and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
AcceptWork (
  int s,
  BOOLEAN bBlockingAllowed,
  struct sockaddr * address,
  socklen_t * address_len
  )
{
  BOOLEAN bBlocking;
  INT32 NewSocketFd;
  struct __filedes * pDescriptor;
  EFI_SOCKET_PROTOCOL * pNewSocket;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Assume failure
  //
  NewSocketFd = -1;

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
    bBlocking &= bBlockingAllowed;

    //
    //  Attempt to accept a new network connection
    //
    do {
      Status = pSocketProtocol->pfnAccept ( pSocketProtocol,
                                            address,
                                            address_len,
                                            &pNewSocket,
                                            &errno );
    } while ( bBlocking && ( EFI_NOT_READY == Status ));

    //
    //  Convert the protocol to a socket
    //
    if ( !EFI_ERROR ( Status )) {
      NewSocketFd = BslSocketProtocolToFd ( pNewSocket, &errno );
      if ( -1 == NewSocketFd ) {
        //
        //  Close the socket
        //
        BslSocketCloseWork ( pNewSocket, NULL );
      }
    }
  }

  //
  //  Return the new socket file descriptor
  //
  return NewSocketFd;
}


/**
  Accept a network connection.

  The accept routine waits for a network connection to the socket.
  It returns the remote network address to the caller if requested.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] address   Address of a buffer to receive the remote network address.

  @param [in, out] address_len  Address of a buffer containing the Length in bytes
                                of the remote network address buffer.  Upon return,
                                contains the length of the remote network address.

  @return     The accept routine returns zero if successful and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
accept (
  int s,
  struct sockaddr * address,
  socklen_t * address_len
  )
{
  //
  //  Wait for the accept call to complete
  //
  return AcceptWork ( s, TRUE, address, address_len );
}


/**
  Non blocking version of ::accept.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] address   Address of a buffer to receive the remote network address.

  @param [in, out] address_len  Address of a buffer containing the Length in bytes
                                of the remote network address buffer.  Upon return,
                                contains the length of the remote network address.

  @return     This routine returns zero if successful and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
AcceptNB (
  int s,
  struct sockaddr * address,
  socklen_t * address_len
  )
{
  //
  //  Attempt to accept a network connection
  //
  return AcceptWork ( s, FALSE, address, address_len );
}
