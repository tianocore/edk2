/** @file
  Test the getaddrinfo API

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

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


/**
  Test the getaddrinfo API

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
  int Index;
  int MaxLen;
  struct addrinfo * pAddrInfo;
  char * pHostName;
  struct addrinfo * pInfo;
  char * pServerName;

  //
  //  Determine if the host name is specified
  //
  AppStatus = 0;
  if ( 1 == Argc ) {
    printf ( "%s  <host name>  <server name>\r\n", Argv[0]);
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
      //  Walk the list of addresses
      //
      pInfo = pAddrInfo;
      while ( NULL != pInfo ) {
        //
        //  Display this entry
        //
        printf ( "0x%08x: ai_flags\r\n", pInfo->ai_flags );
        printf ( "0x%08x: ai_family\r\n", pInfo->ai_family );
        printf ( "0x%08x: ai_socktype\r\n", pInfo->ai_socktype );
        printf ( "0x%08x: ai_protocol\r\n", pInfo->ai_protocol );
        printf ( "0x%08x: ai_addrlen\r\n", pInfo->ai_addrlen );
        printf ( "%s: ai_canonname\r\n", pInfo->ai_canonname );
        printf ( "      0x%02x: ai_addr->sa_len\r\n", (UINT8)pInfo->ai_addr->sa_len );
        printf ( "      0x%02x: ai_addr->sa_family\r\n", (UINT8)pInfo->ai_addr->sa_family );
        MaxLen = pInfo->ai_addr->sa_len;
        if ( sizeof ( struct sockaddr_in6 ) < MaxLen ) {
          MaxLen = sizeof ( struct sockaddr_in6 );
        }
        for ( Index = 0; ( MaxLen - 2 ) > Index; Index++ ) {
          printf ( "      0x%02x: ai_addr->sa_data[%02d]\r\n", (UINT8)pInfo->ai_addr->sa_data [ Index ], Index );
        }

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
