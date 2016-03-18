/** @file
  Implement the shutdown API.

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
  Shutdown the socket receive and transmit operations

  The shutdown routine stops socket receive and transmit operations.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/shutdown.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] how       Which operations to shutdown

  @return     This routine returns the zero (0) if successful or -1 when an
              error occurs.  In the latter case, ::errno contains more details.

 **/
int
shutdown (
  int s,
  int how
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
    //  Receive the data from the socket
    //
    Status = pSocketProtocol->pfnShutdown ( pSocketProtocol,
                                            how,
                                            &errno );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Success
      //
      RetVal = 0;
    }
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
