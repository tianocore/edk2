/** @file
  Windows version of the raw IP4 transmit application

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <RawIp4Tx.h>


/**
  Transmit raw IP4 packets to the remote system.

  Please note that this program must be run with administrator privileges!

  @param [in] argc  The number of arguments
  @param [in] argv  The argument value array

  @retval  0        The application exited normally.
  @retval  Other    An error occurred.
**/
int
main(
  int argc,
  char ** argv
  )
{
  int RetVal;
  WSADATA WsaData;

  //
  //  Initialize the WinSock layer
  //
  RetVal = WSAStartup ( MAKEWORD ( 2, 2 ), &WsaData );
  if ( 0 == RetVal ) {
    //
    //  Start the application
    //  See http://msdn.microsoft.com/en-us/library/ms740548(v=vs.85).aspx
    //
    RetVal = RawIp4Tx ( argc, argv );
    if ( WSAEACCES == RetVal ) {
      printf ( "Requires administrator privileges to run!\r\n" );
    }

    //
    //  Done with the WinSock layer
    //
    WSACleanup ( );
  }

  //
  //  Return the final result
  //
  return RetVal;  
}
