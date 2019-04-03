/** @file
  Translate the port number into a service name

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <sys/socket.h>

char mBuffer[65536];


/** Translate the port number into a service name

  @param[in]  Argc  The number of arguments
  @param[in]  Argv  The argument value array

  @retval  0        The application exited normally.
  @retval  Other    An error occurred.
**/
int
main (
  IN int Argc,
  IN char **Argv
  )
{
  int PortNumber;
  struct servent * pService;

  //  Determine if the service name is specified
  if (( 2 != Argc )
    || ( 1 != sscanf ( Argv[1], "%d", &PortNumber ))) {
    Print ( L"%a  <port number>\r\n", Argv[0]);
  }
  else {
    //  Translate the port number
    pService = getservbyport ( htons ( PortNumber ), NULL );
    if ( NULL == pService ) {
      Print ( L"ERROR - service not found, errno: %d\r\n", errno );
    }
    else {
      Print ( L"%a: %d, %a\r\n",
              pService->s_name,
              PortNumber,
              pService->s_proto );
    }
  }
  //  All done
  return errno;
}
