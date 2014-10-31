/** @file
  Implement the getsockopt API.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <SocketInternals.h>


/** Get the socket options

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html#">POSIX</a>
  documentation is available online.

  @param [in] s               Socket file descriptor returned from ::socket.
  @param [in] level           Option protocol level
  @param [in] option_name     Name of the option
  @param [out] option_value   Buffer to receive the option value
  @param [in,out] option_len  Length of the buffer in bytes,
                              upon return length of the option value in bytes

  @return     This routine returns zero (0) if successful or -1 when an error occurs.
              In the case of an error, ::errno contains more details.
**/
int
getsockopt (
  IN int s,
  IN int level,
  IN int option_name,
  OUT void * __restrict option_value,
  IN OUT socklen_t * __restrict option_len
  )
{
  int OptionStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //  Locate the context for this socket
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //  Get the socket option
    (void) pSocketProtocol->pfnOptionGet ( pSocketProtocol,
                                             level,
                                             option_name,
                                             option_value,
                                             option_len,
                                             &errno );
  }
  //  Return the operation stauts
  OptionStatus = ( 0 == errno ) ? 0 : -1;
  return OptionStatus;
}
