/** @file
  Translate the port number into a service name

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
