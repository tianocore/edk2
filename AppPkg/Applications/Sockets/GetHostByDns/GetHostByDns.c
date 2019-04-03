/** @file
  Translate the host name into an IP address

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <sys/socket.h>

struct hostent * _gethostbydnsname (const char *, int);

char mBuffer[65536];


/** Translate the host name into an IP address

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
  UINT8 * pIpAddress;
  struct hostent * pHost;

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //  Determine if the host name is specified
  if ( 1 == Argc ) {
    Print ( L"%a  <host name>\r\n", Argv[0]);
  }
  else {
    //  Translate the host name
    pHost = _gethostbydnsname ( Argv[1], AF_INET );
    if ( NULL == pHost ) {
      Print ( L"ERROR - host not found, h_errno: %d\r\n", h_errno );
    }
    else {
      pIpAddress = (UINT8 *)pHost->h_addr;
      Print ( L"%a: Type %d, %d.%d.%d.%d\r\n",
              pHost->h_name,
              pHost->h_addrtype,
              pIpAddress[0],
              pIpAddress[1],
              pIpAddress[2],
              pIpAddress[3]);
    }
  }
  //  All done
  return errno;
}
