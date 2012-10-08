/** @file
  Implement the close API.

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
  Worker routine to close the socket.

  @param[in] pSocketProtocol   Socket protocol structure address

  @param[in] pErrno            Address of the ::errno variable

  @retval EFI_SUCCESS   Successfully closed the socket

**/
EFI_STATUS
BslSocketCloseWork (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  )
{
  EFI_STATUS Status;

  //
  //  Start closing the socket
  //
  Status = pSocketProtocol->pfnCloseStart ( pSocketProtocol,
                                            FALSE,
                                            pErrno );

  //
  //  Wait for the socket to close or an error
  //
  while ( EFI_NOT_READY == Status ) {
    Status = pSocketProtocol->pfnClosePoll ( pSocketProtocol,
                                             pErrno );
  }
  if ( !EFI_ERROR ( Status )) {
    //
    //  Release the socket resources
    //
    *pErrno = EslServiceFreeProtocol ( pSocketProtocol );
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Failed to close the socket: %r\r\n",
              Status ));
    *pErrno = EIO;
  }

  //
  //  Return the close status
  //
  return Status;
}


/**
  Close the socket

  The BslSocketClose routine is called indirectly from the close file
  system routine.  This routine closes the socket and returns the
  status to the caller.

  @param[in] pDescriptor Descriptor address for the file

  @return   This routine returns 0 upon success and -1 upon failure.
            In the case of failure, ::errno contains more information.

**/
int
EFIAPI
BslSocketClose (
  struct __filedes * pDescriptor
  )
{
  int CloseStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //
  //  Locate the socket protocol
  //
  pSocketProtocol = BslValidateSocketFd ( pDescriptor, &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Close the socket
    //
    BslSocketCloseWork ( pSocketProtocol, &errno );
  }

  //
  //  Return the close status
  //
  CloseStatus = ( errno == 0 ) ? 0 : -1;
  return CloseStatus;
}
