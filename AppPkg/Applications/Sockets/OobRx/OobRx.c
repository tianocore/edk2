/** @file
  Windows version of the OOB Receive application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OobRx.h>

UINT8 mBuffer[65536];


/**
  Run the OOB receive application

  @param [in] ArgC      Argument count
  @param [in] ArgV      Argument value array

  @retval 0             Successfully operation
 **/
int
OobRx (
  IN int ArgC,
  IN char **ArgV
  )
{
  SOCKET a;
  ssize_t BytesReceived;
  struct sockaddr_in LocalPort;
  UINT32 OobInLine;
  UINT16 PortNumber;
  struct timeval ReceiveTimeout;
  struct sockaddr_in RemotePort;
  socklen_t RemotePortLength;
  int RetVal;
  SOCKET s;
  UINT32 TransmittedBefore;
  UINT32 TransmittedDuring;
  UINT32 TransmittedOob;
  UINT32 TransmittedAfter;
  UINT32 * pTransmittedBytes;

  //
  //  Create the socket
  //
  s = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  if ( -1 == s ) {
    RetVal = GET_ERRNO;
    printf ( "ERROR - socket error, errno: %d\r\n", RetVal );
  }
  else {
    //
    //  Use for/break; instead of goto
    //
    for ( ; ; ) {
      //
      //  Bind the socket to a known port
      //
      PortNumber = OOB_RX_PORT;
      memset ( &LocalPort, 0, sizeof ( LocalPort ));
      SIN_LEN ( LocalPort ) = sizeof ( LocalPort );
      SIN_FAMILY ( LocalPort ) = AF_INET;
      SIN_ADDR ( LocalPort ) = 0;
      SIN_PORT ( LocalPort ) = htons ( PortNumber );
      RetVal = bind ( s,
                      (struct sockaddr *)&LocalPort,
                      sizeof ( LocalPort ));
      if ( -1 == RetVal ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - bind error, errno: %d\r\n", RetVal );
          break;
      }

      //
      //  Make the port available on the server
      //
      RetVal = listen ( s, 2 );
      if ( -1 == RetVal ) {
        RetVal = GET_ERRNO;
        printf ( "ERROR - listen error, errno: %d\r\n", RetVal );
        break;
      }

      //
      //  Wait for a connection to the known port
      //
      RemotePortLength = sizeof ( RemotePort );
      a = accept ( s,
                   (struct sockaddr *)&RemotePort,
                   &RemotePortLength );
      if ( -1 == a ) {
        RetVal = GET_ERRNO;
        printf ( "ERROR - accept error, errno: %d\r\n", RetVal );
        break;
      }

      //
      //  Use for/break instead of goto
      //
      for ( ; ; ) {
        //
        //  Set the receive timeout
        //
        ReceiveTimeout.tv_sec = 0;
        ReceiveTimeout.tv_usec = 20 * 1000;
        RetVal = setsockopt ( a,
                              SOL_SOCKET,
                              SO_RCVTIMEO,
                              (char *)&ReceiveTimeout,
                              sizeof ( ReceiveTimeout ));
        if ( -1 == RetVal ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - setsockopt RCVTIMEO error, errno: %d\r\n", RetVal );
          break;
        }

        //
        //  Select the OOB processing
        //
        OobInLine = ( 1 < ArgC );
        RetVal = setsockopt ( s,
                              SOL_SOCKET,
                              SO_OOBINLINE,
                              (char *)&OobInLine,
                              sizeof ( OobInLine ));
        if ( -1 == RetVal ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - setsockopt OOBINLINE error, errno: %d\r\n", RetVal );
          break;
        }
        printf ( "%s\r\n", ( 0 != OobInLine ) ? "OOB messages are in-line"
                                              : "OOB messages move to the head of the queue" );

        //
        //  Receive data from the remote system
        //
        TransmittedBefore = 0;
        TransmittedOob = 0;
        TransmittedDuring = 0;
        TransmittedAfter = 0;
        pTransmittedBytes = &TransmittedBefore;
        do {
          //
          //  Attempt to receive OOB data
          //
          BytesReceived = recv ( a, &mBuffer[0], sizeof ( mBuffer ), MSG_OOB );
          RetVal = (UINT32)BytesReceived;
          if ( 0 < BytesReceived ) {
            //
            //  Display the received OOB data
            //
            printf ( "%5Ld OOB bytes received\r\n", (UINT64)BytesReceived );

            //
            //  Account for the bytes received
            //
            TransmittedOob += RetVal;
            *pTransmittedBytes += TransmittedAfter;
            TransmittedAfter = 0;
            pTransmittedBytes = &TransmittedDuring;
          }
          else if ( -1 == BytesReceived ) {
            //
            //  Check for connection timeout
            //
            RetVal = GET_ERRNO;
            if ( RX_TIMEOUT_ERROR != RetVal ) {
              //
              //  Receive error
              //
              printf ( "ERROR - recv OOB error, errno: %d\r\n", RetVal );
              break;
            }

            //
            //  Ignore the timeout
            //  Try to receive normal data instead
            //
            BytesReceived = recv ( a, &mBuffer[0], sizeof ( mBuffer ), 0 );
            RetVal = (UINT32)BytesReceived;
            if ( 0 < BytesReceived ) {
              //
              //  Display the received data
              //
              printf ( "%4Ld bytes received\r\n", (UINT64)BytesReceived );

              //
              //  Account for the bytes received
              //
              TransmittedAfter += RetVal;
            }
            else if ( -1 == BytesReceived ) {
              //
              //  Check for a timeout
              //
              RetVal = GET_ERRNO;
              if ( RX_TIMEOUT_ERROR != RetVal ) {
                printf ( "ERROR - recv error, errno: %d\r\n", RetVal );
                break;
              }
            }
          }
        } while ( 0 != RetVal );

        //
        //  Display the bytes received
        //
        if ( 0 == RetVal ) {
          printf ( "Bytes before OOB:  %8d\r\n", TransmittedBefore );
          if ( 0 != TransmittedDuring ) {
            printf ( "Bytes during OOB:  %8d\r\n", TransmittedDuring );
          }
          printf ( "Out-of-band bytes: %8d\r\n", TransmittedOob );
          printf ( "Bytes after OOB:   %8d\r\n", TransmittedAfter );
          printf ( "                   --------\r\n" );
          printf ( "Total Bytes:       %8d\r\n", TransmittedBefore
                                                 + TransmittedDuring
                                                 + TransmittedOob
                                                 + TransmittedAfter );
        }

        //
        //  Test complete
        //
        break;
      }

      //
      //  Close the test socket
      //
      CLOSE_SOCKET ( a );
      break;
    }

    //
    //  Close the socket
    //
    CLOSE_SOCKET ( s );
    printf ( "Socket closed\r\n" );
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
