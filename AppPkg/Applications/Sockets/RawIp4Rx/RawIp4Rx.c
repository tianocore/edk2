/** @file
  Raw IP4 receive application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RawIp4Rx.h"

UINT8 mBuffer[65536];


/**
  Run the raw IP4 receive application

  @param [in] ArgC      Argument count
  @param [in] ArgV      Argument value array

  @retval 0             Successfully operation
 **/
int
RawIp4Rx (
  IN int ArgC,
  IN char **ArgV
  )
{
  ssize_t BytesReceived;
  struct sockaddr_in LocalPort;
  socklen_t LocalPortLength;
  struct sockaddr_in RemotePort;
  socklen_t RemotePortLength;
  int RetVal;
  SOCKET s;
  UINT64 TotalBytesReceived;

  //
  //  Create the socket
  //
  s = socket ( AF_INET, SOCK_RAW, RAW_PROTOCOL );
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
      memset ( &LocalPort, 0, sizeof ( LocalPort ));
      SIN_LEN ( LocalPort ) = sizeof ( LocalPort );
      SIN_FAMILY ( LocalPort ) = AF_INET;
      SIN_ADDR ( LocalPort ) = 0;
      SIN_PORT ( LocalPort ) = 0;
      RetVal = bind ( s,
                      (struct sockaddr *)&LocalPort,
                      sizeof ( LocalPort ));
      if ( -1 == RetVal ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - bind error, errno: %d\r\n", RetVal );
          break;
      }

      //
      //  Display the local address and protocol
      //
      LocalPortLength = sizeof ( LocalPort );
      RetVal = getsockname ( s, (struct sockaddr *)&LocalPort, &LocalPortLength );
      if ( 0 != RetVal ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - getsockname error, errno: %d\r\n", RetVal );
          break;
      }
      printf ( "Local Address: %d.%d.%d.%d, Protocol: %d\r\n",
               (UINT8)SIN_ADDR ( LocalPort ),
               (UINT8)( SIN_ADDR ( LocalPort ) >> 8 ),
               (UINT8)( SIN_ADDR ( LocalPort ) >> 16 ),
               (UINT8)( SIN_ADDR ( LocalPort ) >> 24 ),
               RAW_PROTOCOL );

      //
      //  Use for/break instead of goto
      //
      TotalBytesReceived = 0;
      for ( ; ; ) {
        //
        //  Receive data from the remote system
        //
        do {
          //
          //  Attempt to receive a packet
          //
          RemotePortLength = sizeof ( RemotePort );
          BytesReceived = recvfrom ( s,
                                     &mBuffer[0],
                                     sizeof ( mBuffer ),
                                     0,
                                     (struct sockaddr *)&RemotePort,
                                     &RemotePortLength );
          RetVal = (UINT32)BytesReceived;
          if ( 0 < BytesReceived ) {
            //
            //  Display the received data
            //
            printf ( "%4d bytes received from %d.%d.%d.%d:%d\r\n"
                     "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n"
                     "%02x %02x %02x %02x\r\n",
                     (UINT32)BytesReceived,
                     (UINT8)SIN_ADDR ( RemotePort ),
                     (UINT8)( SIN_ADDR ( RemotePort ) >> 8 ),
                     (UINT8)( SIN_ADDR ( RemotePort ) >> 16 ),
                     (UINT8)( SIN_ADDR ( RemotePort ) >> 24 ),
                     SIN_PORT ( RemotePort ),
                     mBuffer[0],
                     mBuffer[1],
                     mBuffer[2],
                     mBuffer[3],
                     mBuffer[4],
                     mBuffer[5],
                     mBuffer[6],
                     mBuffer[7],
                     mBuffer[8],
                     mBuffer[9],
                     mBuffer[10],
                     mBuffer[11],
                     mBuffer[12],
                     mBuffer[13],
                     mBuffer[14],
                     mBuffer[15],
                     mBuffer[16],
                     mBuffer[17],
                     mBuffer[18],
                     mBuffer[19]);
            TotalBytesReceived += BytesReceived;

            //
            //  All done when the correct packet is received
            //
            if ( mBuffer[9] == RAW_PROTOCOL ) {
              break;
            }
          }
          else if ( -1 == BytesReceived ) {
            //
            //  Check for a timeout
            //
            RetVal = GET_ERRNO;
            printf ( "ERROR - recv error, errno: %d\r\n", RetVal );
            break;
          }
        } while ( 0 != RetVal );

        //
        //  Display the bytes received
        //
        if ( 0 == RetVal ) {
          printf ( "Total Bytes Received:  %Ld\r\n", TotalBytesReceived );
        }

        //
        //  Test complete
        //
        break;
      }
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
