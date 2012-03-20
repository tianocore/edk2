/** @file
  Receive a datagram

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/time.h>

UINT8 mBuffer[ 65536 ];

/**
  Receive a datagram

  @param [in] Argc  The number of arguments
  @param [in] Argv  The argument value array

  @retval  0        The application exited normally.
  @retval  Other    An error occurred.
**/
int
main (
  IN int Argc,
  IN char **Argv
  )
{
  struct sockaddr_in Address;
  socklen_t AddressLength;
  ssize_t LengthInBytes;
  int s;
  int Status;
  struct timeval Timeout;
  

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //
  //  Get the socket
  //
  s = socket ( AF_INET, SOCK_DGRAM, 0 );
  if ( -1 == s ) {
    Print ( L"ERROR - Unable to open the socket, errno: %d\r\n", errno );
  }
  else {
    Timeout.tv_sec = 5;
    Timeout.tv_usec = 0;
    Status = setsockopt ( s,
                          SOL_SOCKET,
                          SO_RCVTIMEO,
                          &Timeout,
                          sizeof ( Timeout ));
    if ( -1 == Status ) {
      Print ( L"ERROR - Unable to set the receive timeout, errno: %d\r\n", errno );
    }
    else {
      AddressLength = sizeof ( Address );
      LengthInBytes = recvfrom ( s,
                                 &mBuffer[0],
                                 sizeof ( mBuffer[0]),
                                 0,
                                 (struct sockaddr *)&Address,
                                 &AddressLength );
      if ( -1 == LengthInBytes ) {
        if ( ETIMEDOUT == errno ) {
          Print ( L"No datagram received\r\n" );
        }
        else {
          Print ( L"ERROR - No datagram received, errno: %d\r\n", errno );
        }
      }
      else {
        Print ( L"Received %d bytes from %d.%d.%d.%d:%d\r\n",
                LengthInBytes,
                (UINT8)Address.sin_addr.s_addr,
                (UINT8)( Address.sin_addr.s_addr >> 8 ),
                (UINT8)( Address.sin_addr.s_addr >> 16 ),
                (UINT8)( Address.sin_addr.s_addr >> 24 ),
                htons ( Address.sin_port ));
      }
    }

    //
    //  Done with the socket
    //
    close ( s );
  }

  //
  //  All done
  //
  DEBUG (( DEBUG_INFO,
            "%a exiting, errno: %d\r\n",
            Argv[0],
            errno ));
  return errno;
}
