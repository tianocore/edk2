/** @file
  Translate the port number into a service name

  Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <sys/socket.h>

/**
  Translate the IP address into a host name

  @param[in] Argc   The number of arguments
  @param[in] Argv   The argument value array

  @retval  0        The application exited normally.
  @retval  Other    An error occurred.
**/
int
main (
  IN int Argc,
  IN char **Argv
  )
{
  UINTN Index;
  UINT8 IpAddress[4];
  struct hostent * pHost;
  UINT8 * pIpAddress;
  char ** ppName;
  UINT32 RemoteAddress[4];

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
    //  Translate the address into a host name
    //
    IpAddress[0] = (UINT8)RemoteAddress[0];
    IpAddress[1] = (UINT8)RemoteAddress[1];
    IpAddress[2] = (UINT8)RemoteAddress[2];
    IpAddress[3] = (UINT8)RemoteAddress[3];
    pHost = gethostbyaddr ( (const char *)&IpAddress[0], INADDRSZ, AF_INET );
    if ( NULL == pHost ) {
      Print ( L"ERROR - host not found, h_errno: %d\r\n", h_errno );
    }
    else {
      pIpAddress = (UINT8 *)pHost->h_addr_list[ 0 ];
      Print ( L"%d.%d.%d.%d, %a\r\n",
              pIpAddress[0],
              pIpAddress[1],
              pIpAddress[2],
              pIpAddress[3],
              pHost->h_name );

      //
      //  Display the other addresses
      //
      for ( Index = 1; NULL != pHost->h_addr_list[Index]; Index++ ) {
        pIpAddress = (UINT8 *)pHost->h_addr_list[Index];
        Print ( L"%d.%d.%d.%d\r\n",
                pIpAddress[0],
                pIpAddress[1],
                pIpAddress[2],
                pIpAddress[3]);
      }

      //
      //  Display the list of aliases
      //
      ppName = pHost->h_aliases;
      if (( NULL == ppName ) || ( NULL == *ppName )) {
        Print ( L"No aliases\r\n" );
      }
      else {
        Print ( L"Aliases: " );
        while ( NULL != *ppName ) {
          //
          //  Display the alias
          //
          Print ( L"%a", *ppName );

          //
          //  Set the next alias
          //
          ppName += 1;
          if ( NULL != *ppName ) {
            Print ( L", " );
          }
        }
        Print ( L"\r\n" );
      }
    }
  }

  //
  //  All done
  //
  return errno;
}
