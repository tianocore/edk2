/** @file
  Test the getnameinfo API

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <Uefi.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/socket.h>

char mBuffer[65536];
char mHostName[256];
char mServiceName[256];

/**
  Test the getnameinfo API

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
  struct addrinfo * pAddrInfo;
  char * pHostName;
  struct addrinfo * pInfo;
  char * pServerName;

  //
  //  Determine if the host name is specified
  //
  AppStatus = 0;
  if ( 1 == Argc ) {
    printf ( "%s  <host address>  <server name>\r\n", Argv[0]);
  }
  else {
    //
    //  Translate the host name
    //
    pHostName = Argv[1];
    pServerName = NULL;
    if ( 2 < Argc ) {
      pServerName = Argv[2];
    }
    AppStatus = getaddrinfo ( pHostName,
                              pServerName,
                              NULL,
                              &pAddrInfo );
    if ( 0 != AppStatus ) {
      printf ( "ERROR - address info not found, errno: %d\r\n", AppStatus );
    }
    if ( NULL == pAddrInfo ) {
      printf ( "ERROR - No address info structure allocated\r\n" );
    }
    else {
      //
      //  Walk the list of names
      //
      pInfo = pAddrInfo;
      while ( NULL != pInfo ) {
        //
        //  Get the name info
        //
        AppStatus = getnameinfo ((struct sockaddr *)pInfo->ai_addr,
                                  pInfo->ai_addrlen,
                                  &mHostName[0],
                                  sizeof ( mHostName ),
                                  &mServiceName[0],
                                  sizeof ( mServiceName ),
                                  0 );
        if ( 0 != AppStatus ) {
          break;
        }

        //
        //  Display this entry
        //
        printf ( "%s: HostName\r\n", &mHostName[0]);
        printf ( "%s: Service Name\r\n", &mServiceName[0]);

        //
        //  Set the next entry
        //
        pInfo = pInfo->ai_next;
      }

      //
      //  Done with this structures
      //
      freeaddrinfo ( pAddrInfo );
    }
  }

  //
  //  All done
  //
  return AppStatus;
}
