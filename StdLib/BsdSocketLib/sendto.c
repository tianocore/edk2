/** @file
  Implement the sendto API.

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

  The ::send routine queues data to the network for transmission.
  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/send.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] buffer    Address of a buffer containing the data to send.
    
  @param [in] length    Length of the buffer in bytes.

  @param [in] flags     Message control flags

  @param [in] to        Remote system address

  @param [in] tolen     Length of remote system address structure

  @returns    ::send returns the number of data bytes that were
              sent and -1 when an error occurs.  In the case of
              an error, errno contains more details.

 **/
ssize_t
sendto (
  int s,
  const void * buffer,
  size_t length,
  int flags,
  const struct sockaddr * to,
  socklen_t tolen
  )
{
  ssize_t LengthInBytes;
  CONST UINT8 * pData;
  struct __filedes * pDescriptor;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Assume failure
  //
  LengthInBytes = -1;

  //
  //  Locate the context for this socket
  //
  pSocketProtocol = BslFdToSocketProtocol ( s,
                                            &pDescriptor,
                                            &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Send the data using the socket
    //
    pData = buffer;
    do {
      errno = 0;
      Status = pSocketProtocol->pfnSend ( pSocketProtocol,
                                          flags,
                                          length,
                                          pData,
                                          (size_t *)&LengthInBytes,
                                          to,
                                          tolen,
                                          &errno );
      if ( EFI_ERROR ( Status ) && ( EFI_NOT_READY != Status )) {
        LengthInBytes = -1;
        break;
      }

      //
      //  Account for the data sent
      //
      pData += LengthInBytes;
      length -= LengthInBytes;
      // TODO: Add non-blocking check
    } while (( 0 != length ) && ( EFI_NOT_READY == Status ));
  }

  //
  //  Return the number of data bytes sent, -1 for errors
  //
  return (INT32)LengthInBytes;
}
