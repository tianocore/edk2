/** @file
  Translate the host name into an IP address

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


/** Translate the host name into an IP address

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
  UINTN Index;
  struct hostent * pHost;
  UINT8 * pIpAddress;
  char ** ppName;

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //  Determine if the host name is specified
  if ( 1 == Argc ) {
    Print ( L"%a  <host name>\r\n", Argv[0]);
  }
  else {
    //  Translate the host name
    pHost = gethostbyname ( Argv[1]);
    if ( NULL == pHost ) {
      Print ( L"ERROR - host not found, h_errno: %d\r\n", h_errno );
    }
    else {
      pIpAddress = (UINT8 *)pHost->h_addr;
      Print ( L"%d.%d.%d.%d, Type %d, %a\r\n",
              pIpAddress[0],
              pIpAddress[1],
              pIpAddress[2],
              pIpAddress[3],
              pHost->h_addrtype,
              pHost->h_name );

      //  Display the other addresses
      for ( Index = 1; NULL != pHost->h_addr_list[Index]; Index++ ) {
        pIpAddress = (UINT8 *)pHost->h_addr_list[Index];
        Print ( L"%d.%d.%d.%d\r\n",
                pIpAddress[0],
                pIpAddress[1],
                pIpAddress[2],
                pIpAddress[3]);
      }

      //  Display the list of aliases
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
  //  All done
  return errno;
}
