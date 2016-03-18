/** @file
  Set the host name

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

#include <sys/socket.h>

char mBuffer[65536];


/**
  Set the host name

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
  int AppStatus;

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //
  //  Determine if the host name is specified
  //
  AppStatus = 0;
  if ( 1 < Argc ) {
    //
    //  Set the host name
    //
    AppStatus = sethostname ( Argv[1], strlen ( Argv[1]));
    if ( -1 == AppStatus ) {
      switch ( errno ) {
      default:
        Print ( L"ERROR - errno: %d\r\n", errno );
        break;

      case ENODEV:
        Print ( L"WARNING - Plarform does not support permanent storage!\r\n" );
        break;

      case ENOMEM:
        Print ( L"ERROR - Insufficient storage to save host name!\r\n" );
        break;

      case ENOTSUP:
        Print ( L"ERROR - Platform does not support environment variable storage!\r\n" );
        break;
      }
    }
  }
  else {
    //
    //  Display the current host name
    //
    AppStatus = gethostname ( &mBuffer[0], sizeof ( mBuffer ));
    if ( -1 == AppStatus ) {
      Print ( L"ERROR - Unable to get host name, errno: %d\r\n", errno );
    }
    else {
      if ( 0 == mBuffer[0]) {
        Print ( L"Host name is not set!\r\n" );
      }
      else {
        Print ( L"Host name: %a", &mBuffer[0]);
      }
    }
  }

  //
  //  All done
  //
  return errno;
}
