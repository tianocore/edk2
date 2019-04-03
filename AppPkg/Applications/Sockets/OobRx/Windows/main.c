/** @file
  Windows version of the OOB Receive application

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <OobRx.h>


/**
  Receive out-of-band messages from the remote system.

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
    //
    RetVal = OobRx ( argc, argv );

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
