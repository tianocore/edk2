/**
  @file
  Web server application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <WebServer.h>

DT_WEB_SERVER mWebServer;   ///<  Web server's control structure


/**
  Add a port to the list of ports to be polled.

  @param [in] pWebServer    The web server control structure address.

  @param [in] SocketFD      The socket's file descriptor to add to the list.

  @retval EFI_SUCCESS       The port was successfully added
  @retval EFI_NO_RESOURCES  Insufficient memory to add the port

**/
EFI_STATUS
PortAdd (
  IN DT_WEB_SERVER * pWebServer,
  IN int SocketFD
  )
{
  nfds_t Index;
  size_t LengthInBytes;
  nfds_t MaxEntries;
  nfds_t MaxEntriesNew;
  struct pollfd * pFdList;
  struct pollfd * pFdListNew;
  WSDT_PORT ** ppPortListNew;
  WSDT_PORT * pPort;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Assume success
    //
    Status = EFI_SUCCESS;

    //
    //  Create a new list if necessary
    //
    pFdList = pWebServer->pFdList;
    MaxEntries = pWebServer->MaxEntries;
    if ( pWebServer->Entries >= MaxEntries ) {
      MaxEntriesNew = 16 + MaxEntries;

      //
      //  The current FD list is full
      //  Allocate a new FD list
      //
      LengthInBytes = sizeof ( *pFdList ) * MaxEntriesNew;
      Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   LengthInBytes,
                                   (VOID **)&pFdListNew );
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                  "ERROR - Failed to allocate the FD list, Status: %r\r\n",
                  Status ));
        break;
      }

      //
      //  Allocate a new port list
      //
      LengthInBytes = sizeof ( *ppPortListNew ) * MaxEntriesNew;
      Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   LengthInBytes,
                                   (VOID **) &ppPortListNew );
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                  "ERROR - Failed to allocate the port list, Status: %r\r\n",
                  Status ));

        //
        //  Free the new FD list
        //
        gBS->FreePool ( pFdListNew );
        break;
      }

      //
      //  Duplicate the FD list
      //
      Index = MaxEntries;
      if ( NULL != pFdList ) {
        CopyMem ( pFdListNew,
                  pFdList,
                  Index * sizeof ( *pFdList ));
      }

      //
      //  Initialize the new entries in the FD list
      //
      for ( ; MaxEntriesNew > Index; Index++ ) {
        pFdListNew[ Index ].fd = -1;
        pFdListNew[ Index ].events = 0;
        pFdListNew[ Index ].revents = 0;
      }

      //
      //  Free the old FD list
      //
      if ( NULL != pFdList ) {
        gBS->FreePool ( pFdList );
      }

      //
      //  Switch to the new FD list
      //
      pWebServer->pFdList = pFdListNew;
      pFdList = pWebServer->pFdList;

      //
      //  Duplicate the port list
      //
      Index = MaxEntries;
      if ( NULL != pWebServer->ppPortList ) {
        CopyMem ( ppPortListNew,
                  pWebServer->ppPortList,
                  Index * sizeof ( *ppPortListNew ));
      }
      
      //
      //  Initialize the new entries in the port list
      //
      for ( ; MaxEntriesNew > Index; Index++ ) {
        ppPortListNew[ Index ] = NULL;
      }
      
      //
      //  Free the old port list
      //
      if ( NULL != pWebServer->ppPortList ) {
        gBS->FreePool ( pWebServer->ppPortList );
      }
      
      //
      //  Switch to the new port list
      //
      pWebServer->ppPortList = ppPortListNew;
      
      //
      //  Update the list size
      //
      pWebServer->MaxEntries = MaxEntriesNew;
    }

    //
    //  Allocate a new port
    //
    LengthInBytes = sizeof ( *pPort );
    Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                 LengthInBytes,
                                 (VOID **)&pPort );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                "ERROR - Failed to allocate the port, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Initialize the port
    //
    pPort->RequestLength = 0;
    pPort->TxBytes = 0;

    //
    //  Add the socket to the FD list
    //
    pFdList[ pWebServer->Entries ].fd = SocketFD;
    pFdList[ pWebServer->Entries ].events = POLLRDNORM
                                             | POLLHUP;
    pFdList[ pWebServer->Entries ].revents = 0;

    //
    //  Add the port to the port list
    //
    pWebServer->ppPortList[ pWebServer->Entries ] = pPort;

    //
    //  Account for the new entry
    //
    pWebServer->Entries += 1;
    DEBUG (( DEBUG_PORT_WORK | DEBUG_INFO,
              "WebServer handling %d ports\r\n",
              pWebServer->Entries ));

    //
    //  All done
    //
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Remove a port from the list of ports to be polled.

  @param [in] pWebServer    The web server control structure address.

  @param [in] SocketFD      The socket's file descriptor to add to the list.

**/
VOID
PortRemove (
  IN DT_WEB_SERVER * pWebServer,
  IN int SocketFD
  )
{
  nfds_t Entries;
  nfds_t Index;
  struct pollfd * pFdList;
  WSDT_PORT ** ppPortList;

  DBG_ENTER ( );

  //
  //  Attempt to remove the entry from the list
  //
  Entries = pWebServer->Entries;
  pFdList = pWebServer->pFdList;
  ppPortList = pWebServer->ppPortList;
  for ( Index = 0; Entries > Index; Index++ ) {
    //
    //  Locate the specified socket file descriptor
    //
    if ( SocketFD == pFdList[ Index ].fd ) {
      //
      //  Determine if this is the listen port
      //
      if ( SocketFD == pWebServer->HttpListenPort ) {
        pWebServer->HttpListenPort = -1;
      }

      //
      //  Close the socket
      //
      close ( SocketFD );

      //
      //  Free the port structure
      //
      gBS->FreePool ( ppPortList[ Index ]);

      //
      //  Remove this port from the list by copying
      //  the rest of the list down one entry
      //
      Entries -= 1;
      for ( ; Entries > Index; Index++ ) {
        pFdList[ Index ] = pFdList[ Index + 1 ];
        ppPortList[ Index ] = ppPortList[ Index + 1 ];
      }
      pFdList[ Index ].fd = -1;
      pFdList[ Index ].events = 0;
      pFdList[ Index ].revents = 0;
      ppPortList[ Index ] = NULL;

      //
      //  Update the number of entries in the list
      //
      pWebServer->Entries = Entries;
      DEBUG (( DEBUG_PORT_WORK | DEBUG_INFO,
                "WebServer handling %d ports\r\n",
                pWebServer->Entries ));
      break;
    }
  }

  DBG_EXIT ( );
}


/**
  Process the work for the sockets.

  @param [in] pWebServer    The web server control structure address.

  @param [in] SocketFD      The socket's file descriptor to add to the list.

  @param [in] events        everts is a bitmask of the work to be done

  @param [in] pPort         The address of a WSDT_PORT structure

  @retval EFI_SUCCESS       The operation was successful
  @retval EFI_DEVICE_ERROR  Error, close the port

**/
EFI_STATUS
PortWork (
  IN DT_WEB_SERVER * pWebServer,
  IN int SocketFD,
  IN INTN events,
  IN WSDT_PORT * pPort
  )
{
  BOOLEAN bDone;
  size_t LengthInBytes;
  int NewSocket;
  EFI_STATUS OpStatus;
  struct sockaddr_in6 RemoteAddress;
  socklen_t RemoteAddressLength;
  EFI_STATUS Status;

  DEBUG (( DEBUG_PORT_WORK, "Entering PortWork\r\n" ));

  //
  //  Assume success
  //
  OpStatus = EFI_SUCCESS;

  //
  //  Handle input events
  //
  if ( 0 != ( events & POLLRDNORM )) {
    //
    //  Determine if this is a connection attempt
    //
    if (( SocketFD == pWebServer->HttpListenPort )
      || ( SocketFD == pWebServer->HttpListenPort6 )) {
      //
      //  Handle connection attempts
      //  Accepts arrive as read events
      //
      RemoteAddressLength = sizeof ( RemoteAddress );
      NewSocket = accept ( SocketFD,
                           (struct sockaddr *)&RemoteAddress,
                           &RemoteAddressLength );
      if ( -1 != NewSocket ) {
        if ( 0 != NewSocket ) {
          //
          //  Add this port to the list monitored by the web server
          //
          Status = PortAdd ( pWebServer, NewSocket );
          if ( EFI_ERROR ( Status )) {
            DEBUG (( DEBUG_ERROR,
                      "ERROR - Failed to add the port 0x%08x, Status: %r\r\n",
                      NewSocket,
                      Status ));

            //
            //  Done with the new socket
            //
            close ( NewSocket );
          }
        }
        else {
          DEBUG (( DEBUG_ERROR,
                    "ERROR - Socket not available!\r\n" ));
        }

        //
        //  Leave the listen port open
        //
      }
      else {
        //
        //  Listen port error
        //  Close the listen port by returning error status
        //
        OpStatus = EFI_DEVICE_ERROR;
        DEBUG (( DEBUG_ERROR,
                  "ERROR - Failed to accept new connection, errno: 0x%08x\r\n",
                  errno ));
      }
    }
    else {
      //
      //  Handle the data received event
      //
      if ( 0 == pPort->RequestLength ) {
        //
        //  Receive the page request
        //
        pPort->RequestLength = recv ( SocketFD,
                                      &pPort->Request[0],
                                      DIM ( pPort->Request ),
                                      0 );
        if ( -1 == pPort->RequestLength ) {
          //
          //  Receive error detected
          //  Close the port
          //
          OpStatus = EFI_DEVICE_ERROR;
        }
        else {
          DEBUG (( DEBUG_REQUEST,
                    "0x%08x: Socket - Received %d bytes of HTTP request\r\n",
                    SocketFD,
                    pPort->RequestLength ));

          //
          //  Process the request
          //
          OpStatus = HttpRequest ( SocketFD, pPort, &bDone );
          if ( bDone ) {
            //
            //  Notify the upper layer to close the socket
            //
            OpStatus = EFI_DEVICE_ERROR;
          }
        }
      }
      else {
        //
        //  Receive the file data
        //
        LengthInBytes = recv ( SocketFD,
                               &pPort->RxBuffer[0],
                               DIM ( pPort->RxBuffer ),
                               0 );
        if ( -1 == LengthInBytes ) {
          //
          //  Receive error detected
          //  Close the port
          //
          OpStatus = EFI_DEVICE_ERROR;
        }
        else {
          DEBUG (( DEBUG_REQUEST,
                    "0x%08x: Socket - Received %d bytes of file data\r\n",
                    SocketFD,
                    LengthInBytes ));

          //
          // TODO: Process the file data
          //
        }
      }
    }
  }

  //
  //  Handle the close event
  //
  if ( 0 != ( events & POLLHUP )) {
    //
    //  Close the port
    //
    OpStatus = EFI_DEVICE_ERROR;
  }

  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_PORT_WORK,
            "Exiting PortWork, Status: %r\r\n",
            OpStatus ));
  return OpStatus;
}


/**
  Scan the list of sockets and process any pending work

  @param [in] pWebServer    The web server control structure address.

**/
VOID
SocketPoll (
  IN DT_WEB_SERVER * pWebServer
  )
{
  int FDCount;
  struct pollfd * pPoll;
  WSDT_PORT ** ppPort;
  EFI_STATUS Status;

  DEBUG (( DEBUG_SOCKET_POLL, "Entering SocketPoll\r\n" ));

  //
  //  Determine if any ports are active
  //
  FDCount = poll ( pWebServer->pFdList,
                   pWebServer->Entries,
                   CLIENT_POLL_DELAY );
  if ( -1 == FDCount ) {
    DEBUG (( DEBUG_ERROR | DEBUG_SOCKET_POLL,
              "ERROR - errno: %d\r\n",
              errno ));
  }

  pPoll = pWebServer->pFdList;
  ppPort = pWebServer->ppPortList;
  while ( 0 < FDCount ) {
    //
    //  Walk the list of ports to determine what work needs to be done
    //
    if ( 0 != pPoll->revents ) {
      //
      //  Process this port
      //
      Status = PortWork ( pWebServer,
                          pPoll->fd,
                          pPoll->revents,
                          *ppPort );
      pPoll->revents = 0;

      //
      //  Close the port if necessary
      //
      if ( EFI_ERROR ( Status )) {
        PortRemove ( pWebServer, pPoll->fd );
        pPoll -= 1;
        ppPort -= 1;
      }

      //
      //  Account for this file descriptor
      //
      FDCount -= 1;
    }

    //
    //  Set the next port
    //
    pPoll += 1;
    ppPort += 1;
  }

  DEBUG (( DEBUG_SOCKET_POLL, "Exiting SocketPoll\r\n" ));
}


/**
  Create an HTTP port for the web server

  This routine polls the network layer to create an HTTP port for the
  web server.  More than one attempt may be necessary since it may take
  some time to get the IP address and initialize the upper layers of
  the network stack.

  After the HTTP port is created, the socket layer will manage the
  coming and going of the network connections until the last network
  connection is broken.

  @param [in] pWebServer    The web server control structure address.
  @param [in] AddressFamily Address family for the network connection
  @param [in] Protocol      Protocol to use for the network connection
  @param [in] HttpPort      Port number for the HTTP connection
  @param [out] pPort        Address of the port

**/
VOID
WebServerListen (
  IN DT_WEB_SERVER * pWebServer,
  IN sa_family_t AddressFamily,
  IN int Protocol,
  IN UINT16 HttpPort,
  OUT int * pPort
  )
{
  union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
  } WebServerAddress;
  int SocketStatus;
  EFI_STATUS Status;

  DEBUG (( DEBUG_SERVER_LISTEN, "Entering WebServerListen\r\n" ));

  //
  //  Attempt to create the socket for the web server
  //
  * pPort = socket ( AddressFamily, SOCK_STREAM, Protocol );
  if ( -1 != *pPort ) {
    //
    //  Build the socket address
    //
    ZeroMem ( &WebServerAddress, sizeof ( WebServerAddress ));
    if ( AF_INET == AddressFamily ) {
      WebServerAddress.v4.sin_len = sizeof ( WebServerAddress.v4 );
      WebServerAddress.v4.sin_family = AddressFamily;
      WebServerAddress.v4.sin_port = htons ( HttpPort );
    }
    else {
      WebServerAddress.v6.sin6_len = sizeof ( WebServerAddress.v6 );
      WebServerAddress.v6.sin6_family = AddressFamily;
      WebServerAddress.v6.sin6_port = htons ( HttpPort );
      WebServerAddress.v6.sin6_scope_id = __IPV6_ADDR_SCOPE_GLOBAL;
    }

    //
    //  Bind the socket to the HTTP port
    //
    SocketStatus = bind ( *pPort,
                          (struct sockaddr *) &WebServerAddress,
                          WebServerAddress.v4.sin_len );
    if ( -1 != SocketStatus ) {
      //
      //  Enable connections to the HTTP port
      //
      SocketStatus = listen ( *pPort, SOMAXCONN );
      if ( -1 != SocketStatus ) {
        //
        //  Add the HTTP port to the list of ports to poll
        //
        Status = PortAdd ( pWebServer, *pPort );
        if ( EFI_ERROR ( Status )) {
          SocketStatus = -1;
        }
        else {
          DEBUG (( DEBUG_PORT_WORK,
                    "Listening on Tcp%d:%d\r\n",
                    ( AF_INET == AddressFamily ) ? 4 : 6,
                    HttpPort ));
        }
      }
    }

    //
    //  Release the socket if necessary
    //
    if ( -1 == SocketStatus ) {
      close ( *pPort );
      *pPort = -1;
    }
  }

  DEBUG (( DEBUG_SERVER_LISTEN, "Exiting WebServerListen\r\n" ));
}


/**
  Entry point for the web server application.

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
  UINT16 HttpPort;
  UINTN Index;
  DT_WEB_SERVER * pWebServer;
  EFI_STATUS Status;
  UINT64 TriggerTime;

  //
  //  Get the HTTP port
  //
  HttpPort = PcdGet16 ( WebServer_HttpPort );
  DEBUG (( DEBUG_HTTP_PORT,
            "HTTP Port: %d\r\n",
            HttpPort ));

  //
  //  Create a timer event to start HTTP port
  //
  pWebServer = &mWebServer;
  Status = gBS->CreateEvent ( EVT_TIMER,
                              TPL_WEB_SERVER,
                              NULL,
                              NULL,
                              &pWebServer->TimerEvent );
  if ( !EFI_ERROR ( Status )) {
    TriggerTime = HTTP_PORT_POLL_DELAY * ( 1000 * 10 );
    Status = gBS->SetTimer ( pWebServer->TimerEvent,
                             TimerPeriodic,
                             TriggerTime );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Run the web server forever
      //
      pWebServer->HttpListenPort = -1;
      pWebServer->HttpListenPort6 = -1;
      pWebServer->bRunning = TRUE;
      do {
        //
        //  Poll the network layer to create the HTTP port
        //  for the web server.  More than one attempt may
        //  be necessary since it may take some time to get
        //  the IP address and initialize the upper layers
        //  of the network stack.
        //
        if (( -1 == pWebServer->HttpListenPort )
          || ( -1 == pWebServer->HttpListenPort6 )) {
          do {
            //
            //  Wait a while before polling for a connection
            //
            if ( EFI_SUCCESS != gBS->CheckEvent ( pWebServer->TimerEvent )) {
              if ( 0 != pWebServer->Entries ) {
                  break;
              }
              gBS->WaitForEvent ( 1, &pWebServer->TimerEvent, &Index );
            }

            //
            //  Poll for a network connection
            //
            if ( -1 == pWebServer->HttpListenPort ) {
              WebServerListen ( pWebServer,
                                AF_INET,
                                IPPROTO_TCP,
                                HttpPort,
                                &pWebServer->HttpListenPort );
            }
            if ( -1 == pWebServer->HttpListenPort6 ) {
              WebServerListen ( pWebServer,
                                AF_INET6,
                                IPPROTO_TCP,
                                HttpPort,
                                &pWebServer->HttpListenPort6 );
            }

            //
            //  Continue polling while both network connections are
            //  not present
            //
          } while ( 0 == pWebServer->Entries );
        }

        //
        //  Poll the sockets for activity while both network
        //  connections are connected
        //
        do {
          SocketPoll ( pWebServer );
        } while ( pWebServer->bRunning
                && ( -1 != pWebServer->HttpListenPort )
                && ( -1 != pWebServer->HttpListenPort6 ));

        //
        //  Continue polling the network connections until both
        //  TCP4 and TCP6 are connected
        //
      } while ( pWebServer->bRunning );

      //
      //  Stop the timer
      //
      gBS->SetTimer ( pWebServer->TimerEvent,
                      TimerCancel,
                      0 );
    }

    //
    //  Done with the timer event
    //
    gBS->CloseEvent ( pWebServer->TimerEvent );
  }

  //
  //  Return the final status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
