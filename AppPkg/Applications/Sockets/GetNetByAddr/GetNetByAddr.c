/** @file
  Translate the IPv4 address into a network name

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
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

/**
  Translate the IPv4 address into a network name

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
  UINT32 RemoteAddress[4];
  UINT8 IpAddress[4];
  struct netent * pNetwork;
  
  //
  //  Determine if the IPv4 address is specified
  //
  if (( 2 != Argc )
    || ( 4 != sscanf ( Argv[1],
                       "%d.%d.%d.%d",
                       &RemoteAddress[0],
                       &RemoteAddress[1],
                       &RemoteAddress[2],
                       &RemoteAddress[3]))
    || ( 255 < RemoteAddress[0])
    || ( 255 < RemoteAddress[1])
    || ( 255 < RemoteAddress[2])
    || ( 255 < RemoteAddress[3])) {
    Print ( L"%a  <IPv4 Address>\r\n", Argv[0]);
  }
  else {
    //
    //  Translate the address into a network name
    //
    IpAddress[0] = (UINT8)RemoteAddress[0];
    IpAddress[1] = (UINT8)RemoteAddress[1];
    IpAddress[2] = (UINT8)RemoteAddress[2];
    IpAddress[3] = (UINT8)RemoteAddress[3];
    pNetwork = getnetbyaddr ( *(uint32_t *)&IpAddress[0], AF_INET );
    if ( NULL == pNetwork ) {
      Print ( L"ERROR - network not found, errno: %d\r\n", errno );
    }
    else {
      Print ( L"%a: %d.%d.%d.%d, 0x%08x\r\n",
              pNetwork->n_name,
              IpAddress[0],
              IpAddress[1],
              IpAddress[2],
              IpAddress[3],
              pNetwork->n_net );
    }
  }
  
  //
  //  All done
  //
  return errno;
}
