/** @file
  Implement the recvfrom API.

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
  Receive data from a network connection and return the remote system's address.

  The recvfrom routine waits for receive data from a remote network
  connection.  This routine is typically called for SOCK_DGRAM sockets
  when the socket is being shared by multiple remote systems and it is
  important to get the remote system address for a response.

  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html">POSIX</a>
  documentation is available online.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] buffer    Address of a buffer to receive the data.

  @param [in] length    Length of the buffer in bytes.

  @param [in] flags     Message control flags

  @param [out] address  Network address to receive the remote system address

  @param [in] address_len Length of the remote network address structure

  @return     This routine returns the number of valid bytes in the buffer,
              zero if no data was received, and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
ssize_t
recvfrom (
  int s,
  void * buffer,
  size_t length,
  int flags,
  struct sockaddr * address,
  socklen_t * address_len
  )
{
  socklen_t ByteCount;
  ssize_t LengthInBytes;
  UINT8 * pData;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;
  struct timeval TimeVal;
  EFI_EVENT pTimer;
  UINT64 Timeout;
  ssize_t TotalBytes;

  //
  //  Assume failure
  //
  LengthInBytes = -1;

  //
  //  Locate the context for this socket
  //
  pSocketProtocol = BslFdToSocketProtocol ( s, NULL, &errno );
  if ( NULL != pSocketProtocol ) {
    //
    //  Receive the data from the socket
    //
    Status = pSocketProtocol->pfnReceive ( pSocketProtocol,
                                           flags,
                                           length,
                                           buffer,
                                           (size_t *)&LengthInBytes,
                                           address,
                                           address_len,
                                           &errno );
    if ( EFI_ERROR ( Status )) {
      LengthInBytes = -1;
      if ( EAGAIN == errno ) {
        //
        //  Get the timeout
        //
        ByteCount = sizeof ( TimeVal );
        LengthInBytes = getsockopt ( s,
                                     SOL_SOCKET,
                                     SO_RCVTIMEO,
                                     &TimeVal,
                                     &ByteCount );
        if ( 0 == LengthInBytes ) {
          //
          //  Compute the timeout
          //
          Timeout = TimeVal.tv_sec;
          Timeout *= 1000 * 1000;
          Timeout += TimeVal.tv_usec;
          Timeout *= 10;

          //
          //  The timer is only necessary if a timeout is running
          //
          LengthInBytes = -1;
          Status = EFI_SUCCESS;
          pTimer = NULL;
          if ( 0 != Timeout ) {
            Status = gBS->CreateEvent ( EVT_TIMER,
                                        TPL_NOTIFY,
                                        NULL,
                                        NULL,
                                        &pTimer );
          }
          if ( !EFI_ERROR ( Status )) {
            //
            //  Start the timer
            //
            if ( NULL != pTimer ) {
              Status = gBS->SetTimer ( pTimer,
                                       TimerRelative,
                                       Timeout );
            }
            if ( !EFI_ERROR ( Status )) {
              //
              //  Loop until data is received or the timeout
              //  expires
              //
              TotalBytes = 0;
              pData = (UINT8 *)buffer;
              do {
                //
                //  Determine if the timeout expired
                //
                if ( NULL != pTimer ) {
                  Status = gBS->CheckEvent ( pTimer );
                  if ( EFI_SUCCESS == Status ) {
                    errno = ETIMEDOUT;
                    if ( 0 == TotalBytes ) {
                      TotalBytes = -1;
                    }
                    break;
                  }
                }

                //
                //  Attempt to receive some data
                //
                Status = pSocketProtocol->pfnReceive ( pSocketProtocol,
                                                       flags,
                                                       length,
                                                       pData,
                                                       (size_t *)&LengthInBytes,
                                                       address,
                                                       address_len,
                                                       &errno );
                if ( !EFI_ERROR ( Status )) {
                  //
                  //  Account for the data received
                  //
                  TotalBytes += LengthInBytes;
                  pData += LengthInBytes;
                  length -= LengthInBytes;
                }
              } while ( EFI_NOT_READY == Status );
              LengthInBytes = TotalBytes;

              //
              //  Stop the timer
              //
              if ( NULL != pTimer ) {
                gBS->SetTimer ( pTimer,
                                TimerCancel,
                                0 );
              }
            }

            //
            //  Release the timer
            //
            if ( NULL != pTimer ) {
              gBS->CloseEvent ( pTimer );
            }
          }
        }
      }
    }
  }

  //
  //  Return the receive data length, -1 for errors
  //
  return LengthInBytes;
}
