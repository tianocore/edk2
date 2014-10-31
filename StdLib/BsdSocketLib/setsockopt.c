/** @file
  Implement the setsockopt API.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <SocketInternals.h>


/** Set the socket options

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] s               Socket file descriptor returned from ::socket.
  @param [in] level           Option protocol level
  @param [in] option_name     Name of the option
  @param [in] option_value    Buffer containing the option value
  @param [in] option_len      Length of the value in bytes

  @return   This routine returns zero (0) upon success and -1 when an error occurs.
            In the case of an error, ::errno contains more details.
**/
int
setsockopt (
  IN int s,
  IN int level,
  IN int option_name,
  IN CONST void * option_value,
  IN socklen_t option_len
  )
{
  int OptionStatus;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //  Locate the context for this socket
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //  Set the socket option
    (void) pSocketProtocol->pfnOptionSet (pSocketProtocol,
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
