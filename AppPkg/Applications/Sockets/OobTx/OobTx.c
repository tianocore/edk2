/** @file
  Windows version of the OOB Transmit application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OobTx.h>

UINT8 mBuffer[8192];
UINT8 mOob[512];

/**
  Transmit out-of-band messages to the remote system.

  @param [in] ArgC        Argument count
  @param [in] ArgV        Argument value array

  @retval 0               Successfully operation
 **/

int
OobTx (
  IN int ArgC,
  IN char **ArgV
  )
{
  UINT32 BytesSent;
  ssize_t BytesTransmitted;
  UINT32 Index;
  struct sockaddr_in LocalPort;
  UINT32 OobInLine;
  UINT16 PortNumber;
  UINT32 RemoteAddress[4];
  struct sockaddr_in RemotePort;
  int RetVal;
  UINT32 TransmittedAfter;
  UINT32 TransmittedBefore;
  UINT32 TransmittedOob;
  SOCKET s;

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
      //  Validate the arguments
      //
      if (( 2 > ArgC )
        || ( 4 != sscanf ( ArgV[1],
                           "%d.%d.%d.%d",
                           &RemoteAddress[0],
                           &RemoteAddress[1],
                           &RemoteAddress[2],
                           &RemoteAddress[3]))
          || ( 224 < RemoteAddress[0])
          || ( 255 < RemoteAddress[1])
          || ( 255 < RemoteAddress[2])
          || ( 255 < RemoteAddress[3])
          || (( 0 == RemoteAddress[0])
              && ( 0 == RemoteAddress[1])
              && ( 0 == RemoteAddress[2])
              && ( 0 == RemoteAddress[3]))) {
        printf ( "%s  <remote IP address>  [optional: enables in-line OOB]\r\n", ArgV[0]);
        RetVal = EINVAL;
        break;
      }

      //
      //  Bind the socket to a local port
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
      //  Specify the remote port
      //
      PortNumber = OOB_RX_PORT;
      memset ( &RemotePort, 0, sizeof ( RemotePort ));
      SIN_LEN ( RemotePort ) = sizeof ( RemotePort );
      SIN_FAMILY ( RemotePort ) = AF_INET;
      SIN_ADDR ( RemotePort ) = ( RemoteAddress[3] << 24 )
                              | ( RemoteAddress[2] << 16 )
                              | ( RemoteAddress[1] << 8 )
                              | RemoteAddress[0];
      SIN_PORT ( RemotePort ) = htons ( PortNumber );

      //
      //  Connect to the remote server
      //
      RetVal = connect ( s, (struct sockaddr *)&RemotePort, sizeof ( RemotePort ));
      if ( -1 == RetVal ) {
        RetVal = GET_ERRNO;
        printf ( "ERROR - connect error, errno: %d\r\n", RetVal );
        break;
      }

      //
      //  Select the OOB processing
      //
      OobInLine = ( 2 < ArgC );
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
      //  Initialize the messages
      //
      memset ( &mBuffer[0], 0, sizeof ( mBuffer ));
      memset ( &mOob[0], 0x11, sizeof ( mOob ));

      //
      //  Send the data before the out-of-band message
      //
      TransmittedBefore = 0;
      for ( Index = 0; TX_MSGS_BEFORE > Index; Index++ ) {
        BytesSent = 0;
        do {
          BytesTransmitted = send ( s,
                                    &mBuffer[BytesSent],
                                    sizeof ( mBuffer ) - BytesSent,
                                    0 );
          if ( -1 == BytesTransmitted ) {
            RetVal = GET_ERRNO;
            printf ( "ERROR - send before error, errno: %d\r\n", RetVal );
            break;
          }
          BytesSent += (UINT32)BytesTransmitted;
          RetVal = 0;
        } while ( sizeof ( mBuffer ) > BytesSent );
        if ( 0 != RetVal ) {
          break;
        }
        TransmittedBefore += BytesSent;
      }
      if ( 0 != RetVal ) {
        break;
      }

      //
      //  Send the out-of-band message
      //
      BytesSent = 0;
      do {
        BytesTransmitted = send ( s,
                                  &mOob[BytesSent],
                                  sizeof ( mOob ) - BytesSent,
                                  MSG_OOB );
        if ( -1 == BytesTransmitted ) {
          RetVal = GET_ERRNO;
          printf ( "ERROR - send OOB error, errno: %d\r\n", RetVal );
          break;
        }
        BytesSent += (UINT32)BytesTransmitted;
        RetVal = 0;
      } while ( sizeof ( mOob ) > BytesSent );
      if ( 0 != RetVal ) {
        break;
      }
      TransmittedOob = BytesSent;

      //
      //  Send the data after the out-of-band message
      //
      TransmittedAfter = 0;
      for ( Index = 0; TX_MSGS_AFTER > Index; Index++ ) {
        BytesSent = 0;
        do {
          BytesTransmitted = send ( s,
                                    &mBuffer[BytesSent],
                                    sizeof ( mBuffer ) - BytesSent,
                                    0 );
          if ( -1 == BytesTransmitted ) {
            RetVal = GET_ERRNO;
            printf ( "ERROR - send after error, errno: %d\r\n", RetVal );
            break;
          }
          BytesSent += (UINT32)BytesTransmitted;
          RetVal = 0;
        } while ( sizeof ( mBuffer ) > BytesSent );
        if ( 0 != RetVal ) {
          break;
        }
        TransmittedAfter += BytesSent;
      }

      //
      //  Test completed successfully
      //
      if ( 0 == RetVal ) {
        printf ( "Bytes before OOB:  %8d\r\n", TransmittedBefore );
        printf ( "Out-of-band bytes: %8d\r\n", TransmittedOob );
        printf ( "Bytes after OOB:   %8d\r\n", TransmittedAfter );
        printf ( "                   --------\r\n" );
        printf ( "Total Bytes:       %8d\r\n", TransmittedBefore
                                               + TransmittedOob
                                               + TransmittedAfter );
      }
      break;
    }

    //
    //  Close the socket
    //
    CLOSE_SOCKET ( s );
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
