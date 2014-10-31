/** @file
  Translate the network name into an IP address

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
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <sys/socket.h>

char mBuffer[65536];


/** Translate the network name into an IP address

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
  UINT8 * pIpAddress;
  struct netent * pNetwork;

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //  Determine if the network name is specified
  if ( 1 == Argc ) {
    Print ( L"%a  <network name>\r\n", Argv[0]);
  }
  else {
    //  Translate the net name
    pNetwork = getnetbyname ( Argv[1]);
    if ( NULL == pNetwork ) {
      Print ( L"ERROR - network not found, errno: %d\r\n", errno );
    }
    else {
      pIpAddress = (UINT8 *)(UINTN)&pNetwork->n_net;
      Print ( L"%a: Type %d, %d.%d.%d.%d\r\n",
              pNetwork->n_name,
              pNetwork->n_addrtype,
              pIpAddress[0],
              pIpAddress[1],
              pIpAddress[2],
              pIpAddress[3]);
    }
  }
  //  All done
  return errno;
}
