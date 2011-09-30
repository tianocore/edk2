/** @file
  Implement the getsockname API.

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
  Get the local socket address.

  The getsockname routine retrieves the local system address from the socket.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockname.html#">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [out] address  Network address to receive the local system address

  @param [in] address_len Length of the local network address structure

  @return     This routine returns zero (0) if successful or -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
getsockname (
  int s,
  struct sockaddr * address,
  socklen_t * address_len
  )
{
  int RetVal;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Assume failure
  //
  RetVal = -1;

  //
  //  Locate the context for this socket
  //
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Get the local socket address
    //
    Status = pSocketProtocol->pfnGetLocal ( pSocketProtocol,
                                            address,
                                            address_len,
                                            &errno );
    if ( !EFI_ERROR ( Status )) {
      RetVal = 0;
    }
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
