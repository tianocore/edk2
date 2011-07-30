/** @file
  Implement the TCP4 driver support for the socket layer.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Socket.h"


/**
  Accept a network connection.

  The SocketAccept routine waits for a network connection to the socket.
  It is able to return the remote network address to the caller if
  requested.

  @param [in] pSocket   Address of the socket structure.

  @param [in] pSockAddr       Address of a buffer to receive the remote
                              network address.

  @param [in, out] pSockAddrLength  Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.

  @retval EFI_SUCCESS   Remote address is available
  @retval Others        Remote address not available

 **/
EFI_STATUS
EslTcpAccept4 (
  IN DT_SOCKET * pSocket,
  IN struct sockaddr * pSockAddr,
  IN OUT socklen_t * pSockAddrLength
  )
{
  DT_PORT * pPort;
  struct sockaddr_in * pRemoteAddress;
  DT_TCP4_CONTEXT * pTcp4;
  UINT32 RemoteAddress;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Validate the socket length
  //
  pRemoteAddress = (struct sockaddr_in *) pSockAddr;
  if (( NULL == pSockAddrLength )
    || ( sizeof ( *pRemoteAddress ) > *pSockAddrLength )) {
    //
    //  Invalid socket address
    //
    Status = EFI_INVALID_PARAMETER;
    pSocket->errno = EINVAL;
    DEBUG (( DEBUG_ACCEPT,
              "ERROR - Invalid address length\r\n" ));
  }
  else {
    //
    //  Assume success
    //
    Status = EFI_SUCCESS;

    //
    //  Locate the address context
    //
    pPort = pSocket->pPortList;
    pTcp4 = &pPort->Context.Tcp4;

    //
    //  Fill-in the remote address structure
    //
    ZeroMem ( pRemoteAddress, sizeof ( *pRemoteAddress ));
    pRemoteAddress->sin_len = sizeof ( *pRemoteAddress );
    pRemoteAddress->sin_family = AF_INET;
    pRemoteAddress->sin_port = SwapBytes16 ( pTcp4->ConfigData.AccessPoint.RemotePort );
    RemoteAddress = pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[3];
    RemoteAddress <<= 8;
    RemoteAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[2];
    RemoteAddress <<= 8;
    RemoteAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[1];
    RemoteAddress <<= 8;
    RemoteAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[0];
    pRemoteAddress->sin_addr.s_addr = RemoteAddress;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Bind a name to a socket.

  The ::TcpBind4 routine connects a name to a TCP4 stack on the local machine.

  @param [in] pSocket   Address of the socket structure.

  @param [in] pSockAddr Address of a sockaddr structure that contains the
                        connection point on the local machine.  An IPv4 address
                        of INADDR_ANY specifies that the connection is made to
                        all of the network stacks on the platform.  Specifying a
                        specific IPv4 address restricts the connection to the
                        network stack supporting that address.  Specifying zero
                        for the port causes the network layer to assign a port
                        number from the dynamic range.  Specifying a specific
                        port number causes the network layer to use that port.

  @param [in] SockAddrLen   Specifies the length in bytes of the sockaddr structure.

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslTcpBind4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  )
{
  EFI_HANDLE ChildHandle;
  DT_LAYER * pLayer;
  DT_PORT * pPort;
  DT_SERVICE * pService;
  CONST struct sockaddr_in * pIp4Address;
  EFI_SERVICE_BINDING_PROTOCOL * pTcp4Service;
  EFI_STATUS Status;
  EFI_STATUS TempStatus;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Assume success
  //
  pSocket->errno = 0;
  Status = EFI_SUCCESS;

  //
  //  Validate the address length
  //
  pIp4Address = (CONST struct sockaddr_in *) pSockAddr;
  if ( SockAddrLength >= ( sizeof ( *pIp4Address )
                           - sizeof ( pIp4Address->sin_zero ))) {

    //
    //  Walk the list of services
    //
    pLayer = &mEslLayer;
    pService = pLayer->pTcp4List;
    while ( NULL != pService ) {
      //
      //  Create the TCP port
      //
      pTcp4Service = pService->pInterface;
      ChildHandle = NULL;
      Status = pTcp4Service->CreateChild ( pTcp4Service,
                                           &ChildHandle );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_BIND | DEBUG_POOL,
                  "0x%08x: Tcp4 port handle created\r\n",
                  ChildHandle ));

        //
        //  Open the port
        //
        Status = EslTcpPortAllocate4 ( pSocket,
                                       pService,
                                       ChildHandle,
                                       (UINT8 *) &pIp4Address->sin_addr.s_addr,
                                       SwapBytes16 ( pIp4Address->sin_port ),
                                       DEBUG_BIND,
                                       &pPort );
      }
      else {
        DEBUG (( DEBUG_BIND | DEBUG_POOL,
                  "ERROR - Failed to open Tcp4 port handle, Status: %r\r\n",
                  Status ));
        ChildHandle = NULL;
      }

      //
      //  Close the port if necessary
      //
      if (( EFI_ERROR ( Status )) && ( NULL != ChildHandle )) {
        TempStatus = pTcp4Service->DestroyChild ( pTcp4Service,
                                                  ChildHandle );
        if ( !EFI_ERROR ( TempStatus )) {
          DEBUG (( DEBUG_BIND | DEBUG_POOL,
                    "0x%08x: Tcp4 port handle destroyed\r\n",
                    ChildHandle ));
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_BIND | DEBUG_POOL,
                    "ERROR - Failed to destroy the Tcp4 port handle 0x%08x, Status: %r\r\n",
                    ChildHandle,
                    TempStatus ));
          ASSERT ( EFI_SUCCESS == TempStatus );
        }
      }

      //
      //  Set the next service
      //
      pService = pService->pNext;
    }

    //
    //  Verify that at least one network connection was found
    //
    if ( NULL == pSocket->pPortList ) {
      DEBUG (( DEBUG_BIND | DEBUG_POOL | DEBUG_INIT,
                "Socket address %d.%d.%d.%d (0x%08x) is not available!\r\n",
                ( pIp4Address->sin_addr.s_addr >> 24 ) & 0xff,
                ( pIp4Address->sin_addr.s_addr >> 16 ) & 0xff,
                ( pIp4Address->sin_addr.s_addr >> 8 ) & 0xff,
                pIp4Address->sin_addr.s_addr & 0xff,
                pIp4Address->sin_addr.s_addr ));
      pSocket->errno = EADDRNOTAVAIL;
      Status = EFI_INVALID_PARAMETER;
    }
  }
  else {
    DEBUG (( DEBUG_BIND,
              "ERROR - Invalid TCP4 address length: %d\r\n",
              SockAddrLength ));
    Status = EFI_INVALID_PARAMETER;
    pSocket->errno = EINVAL;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Attempt to connect to a remote TCP port

  @param [in] pSocket         Address of the socket structure.

  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslTcpConnectAttempt4 (
  IN DT_SOCKET * pSocket
  )
{
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Determine if any more local adapters are available
  //
  pPort = pSocket->pPortList;
  if ( NULL != pPort ) {
    //
    //  Configure the port
    //
    pTcp4 = &pPort->Context.Tcp4;
    pTcp4->ConfigData.AccessPoint.ActiveFlag = TRUE;
    pTcp4->ConfigData.TimeToLive = 255;
    pTcp4Protocol = pTcp4->pProtocol;
    Status = pTcp4Protocol->Configure ( pTcp4Protocol,
                                        &pTcp4->ConfigData );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_CONNECT,
                "ERROR - Failed to configure the Tcp4 port, Status: %r\r\n",
                Status ));
      switch ( Status ) {
      case EFI_ACCESS_DENIED:
        pSocket->errno = EACCES;
        break;
    
      default:
      case EFI_DEVICE_ERROR:
        pSocket->errno = EIO;
        break;
    
      case EFI_INVALID_PARAMETER:
        pSocket->errno = EADDRNOTAVAIL;
        break;
    
      case EFI_NO_MAPPING:
        pSocket->errno = EAFNOSUPPORT;
        break;
    
      case EFI_OUT_OF_RESOURCES:
        pSocket->errno = ENOBUFS;
        break;
    
      case EFI_UNSUPPORTED:
        pSocket->errno = EOPNOTSUPP;
        break;
      }
    }
    else {
      DEBUG (( DEBUG_CONNECT,
                "0x%08x: Port configured\r\n",
                pPort ));
      pTcp4->bConfigured = TRUE;

      //
      //  Attempt the connection to the remote system
      //
      Status = pTcp4Protocol->Connect ( pTcp4Protocol,
                                        &pTcp4->ConnectToken );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Connection in progress
        //
        pSocket->errno = EINPROGRESS;
        Status = EFI_NOT_READY;
        DEBUG (( DEBUG_CONNECT,
                  "0x%08x: Port attempting connection to %d.%d.%d.%d:%d\r\n",
                  pPort,
                  pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[0],
                  pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[1],
                  pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[2],
                  pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[3],
                  pTcp4->ConfigData.AccessPoint.RemotePort ));
      }
      else {
        //
        //  Connection error
        //
        pSocket->errno = EINVAL;
        DEBUG (( DEBUG_CONNECT,
                  "ERROR - Port 0x%08x not connected, Status: %r\r\n",
                  pPort,
                  Status ));
      }
    }
  }
  else {
    //
    //  No more local adapters available
    //
    pSocket->errno = ENETUNREACH;
    Status = EFI_NO_RESPONSE;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the remote connection attempt

  A connection attempt to a remote system has just completed when
  this routine is invoked.  Release the port in the case of an
  error and start a connection attempt on the next port.  If the
  connection attempt was successful, then release all of the other
  ports.

  @param  Event         The connect completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpConnectComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  BOOLEAN bRemoveFirstPort;
  BOOLEAN bRemovePorts;
  DT_PORT * pNextPort;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Locate the TCP context
  //
  pSocket = pPort->pSocket;
  pTcp4 = &pPort->Context.Tcp4;

  //
  //  Get the connection status
  //
  bRemoveFirstPort = FALSE;
  bRemovePorts = FALSE;
  Status = pTcp4->ConnectToken.CompletionToken.Status;
  pSocket->ConnectStatus = Status;
  if ( !EFI_ERROR ( Status )) {
    //
    //  The connection was successful
    //
    DEBUG (( DEBUG_CONNECT,
              "0x%08x: Port connected to %d.%d.%d.%d:%d\r\n",
              pPort,
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [0],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [1],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [2],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [3],
              pTcp4->ConfigData.AccessPoint.RemotePort ));

    //
    //  Remove the rest of the ports
    //
    bRemovePorts = TRUE;
  }
  else {
    //
    //  The connection failed
    //
    DEBUG (( DEBUG_CONNECT,
              "0x%08x: Port connection to %d.%d.%d.%d:%d failed, Status: %r\r\n",
              pPort,
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [0],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [1],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [2],
              pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr [3],
              pTcp4->ConfigData.AccessPoint.RemotePort,
              Status ));

    //
    //  Close the current port
    //
    Status = EslTcpPortClose4 ( pPort );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_CONNECT,
              "0x%08x: Port closed\r\n",
              pPort ));
    }
    else {
      DEBUG (( DEBUG_CONNECT,
                "ERROR - Failed to close port 0x%08x, Status: %r\r\n",
                pPort,
                Status ));
    }

    //
    //  Try to connect using the next port
    //
    Status = EslTcpConnectAttempt4 ( pSocket );
    if ( EFI_NOT_READY != Status ) {
      pSocket->ConnectStatus = Status;
      bRemoveFirstPort = TRUE;
    }
  }

  //
  //  Remove the ports if necessary
  //
  if ( bRemoveFirstPort || bRemovePorts ) {
    //
    //  Remove the first port if necessary
    //
    pPort = pSocket->pPortList;
    if (( !bRemoveFirstPort ) && ( NULL != pPort )) {
      pPort = pPort->pLinkSocket;
    }

    //
    //  Remove the rest of the list
    //
    while ( NULL != pPort ) {
      pNextPort = pPort->pLinkSocket;
      EslTcpPortClose4 ( pPort );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_CONNECT,
                "0x%08x: Port closed\r\n",
                pPort ));
      }
      else {
        DEBUG (( DEBUG_CONNECT,
                  "ERROR - Failed to close port 0x%08x, Status: %r\r\n",
                  pPort,
                  Status ));
      }
      pPort = pNextPort;
    }

    //
    //  Notify the poll routine
    //
    pSocket->bConnected = TRUE;
  }

  DBG_EXIT ( );
}


/**
  Poll for completion of the connection attempt.

  The ::TcpConnectPoll4 routine determines when the connection
  attempt transitions from being in process to being complete.

  @param [in] pSocket         Address of the socket structure.

  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslTcpConnectPoll4 (
  IN DT_SOCKET * pSocket
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Determine if the connection is complete
  //
  if ( !pSocket->bConnected ) {
    //
    //  Not connected
    //
    pSocket->errno = EAGAIN;
    Status = EFI_NOT_READY;
  }
  else {
    //
    //  The connection processing is complete
    //
    pSocket->bConnected = FALSE;

    //
    //  Translate the connection status
    //
    Status = pSocket->ConnectStatus;
    switch ( Status ) {
    default:
    case EFI_DEVICE_ERROR:
      pSocket->errno = EIO;
      break;

    case EFI_ABORTED:
      pSocket->errno = ECONNREFUSED;
      break;

    case EFI_INVALID_PARAMETER:
      pSocket->errno = EINVAL;
      break;

    case EFI_NO_MAPPING:
    case EFI_NO_RESPONSE:
      pSocket->errno = EHOSTUNREACH;
      break;

    case EFI_NO_MEDIA:
      pSocket->errno = ENETDOWN;
      break;

    case EFI_OUT_OF_RESOURCES:
      pSocket->errno = ENOMEM;
      break;

    case EFI_SUCCESS:
      pSocket->errno = 0;
      pSocket->bConfigured = TRUE;
      break;

    case EFI_TIMEOUT:
      pSocket->errno = ETIMEDOUT;
      break;

    case EFI_UNSUPPORTED:
      pSocket->errno = ENOTSUP;
      break;

    case 0x80000069:
      pSocket->errno = ECONNRESET;
      break;
    }
  }

  //
  //  Return the initialization status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Connect to a remote system via the network.

  The ::TcpConnectStart4= routine starts the connection processing
  for a TCP4 port.

  @param [in] pSocket         Address of the socket structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslTcpConnectStart4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  )
{
  struct sockaddr_in LocalAddress;
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  CONST struct sockaddr_in * pIp4Address;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Validate the address length
  //
  Status = EFI_SUCCESS;
  pIp4Address = (CONST struct sockaddr_in *) pSockAddr;
  if ( SockAddrLength >= ( sizeof ( *pIp4Address )
                           - sizeof ( pIp4Address->sin_zero ))) {
    //
    //  Determine if BIND was already called
    //
    if ( NULL == pSocket->pPortList ) {
      //
      //  Allow any local port
      //
      ZeroMem ( &LocalAddress, sizeof ( LocalAddress ));
      LocalAddress.sin_len = sizeof ( LocalAddress );
      LocalAddress.sin_family = AF_INET;
      Status = EslSocketBind ( &pSocket->SocketProtocol,
                               (struct sockaddr *)&LocalAddress,
                               LocalAddress.sin_len,
                               &pSocket->errno );
    }
    if ( NULL != pSocket->pPortList ) {
      //
      //  Walk the list of ports
      //
      pPort = pSocket->pPortList;
      while ( NULL != pPort ) {
        //
        //  Set the remote address
        //
        pTcp4 = &pPort->Context.Tcp4;
        *(UINT32 *)&pTcp4->ConfigData.AccessPoint.RemoteAddress = pIp4Address->sin_addr.s_addr;
        pTcp4->ConfigData.AccessPoint.RemotePort = SwapBytes16 ( pIp4Address->sin_port );

        //
        //  Set the next port
        //
        pPort = pPort->pLinkSocket;
      }
      
      //
      //  Attempt a connection using the first adapter
      //
      Status = EslTcpConnectAttempt4 ( pSocket );
    }
  }
  else {
    DEBUG (( DEBUG_CONNECT,
              "ERROR - Invalid TCP4 address length: %d\r\n",
              SockAddrLength ));
    Status = EFI_INVALID_PARAMETER;
    pSocket->errno = EINVAL;
  }

  //
  //  Return the initialization status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Initialize the TCP4 service.

  This routine initializes the TCP4 service after its service binding
  protocol was located on a controller.

  @param [in] pService        DT_SERVICE structure address

  @retval EFI_SUCCESS         The service was properly initialized
  @retval other               A failure occurred during the service initialization

**/
EFI_STATUS
EFIAPI
EslTcpInitialize4 (
  IN DT_SERVICE * pService
  )
{
  DT_LAYER * pLayer;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Identify the service
  //
  pService->NetworkType = NETWORK_TYPE_TCP4;

  //
  //  Connect this service to the service list
  //
  pLayer = &mEslLayer;
  pService->pNext = pLayer->pTcp4List;
  pLayer->pTcp4List = pService;

  //
  //  Assume the list is empty
  //
  Status = EFI_SUCCESS;

  //
  //  Return the initialization status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Get the local socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslTcpGetLocalAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  )
{
  socklen_t LengthInBytes;
  DT_PORT * pPort;
  struct sockaddr_in * pLocalAddress;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Verify that there is just a single connection
  //
  pPort = pSocket->pPortList;
  if (( NULL != pPort ) && ( NULL == pPort->pLinkSocket )) {
    //
    //  Verify the address length
    //
    LengthInBytes = sizeof ( struct sockaddr_in );
    if ( LengthInBytes <= * pAddressLength ) {
      //
      //  Return the local address
      //
      pTcp4 = &pPort->Context.Tcp4;
      pLocalAddress = (struct sockaddr_in *)pAddress;
      ZeroMem ( pLocalAddress, LengthInBytes );
      pLocalAddress->sin_family = AF_INET;
      pLocalAddress->sin_len = (uint8_t)LengthInBytes;
      pLocalAddress->sin_port = SwapBytes16 ( pTcp4->ConfigData.AccessPoint.StationPort );
      CopyMem ( &pLocalAddress->sin_addr,
                &pTcp4->ConfigData.AccessPoint.StationAddress.Addr[0],
                sizeof ( pLocalAddress->sin_addr ));
      pSocket->errno = 0;
      Status = EFI_SUCCESS;
    }
    else {
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }
  }
  else {
    pSocket->errno = ENOTCONN;
    Status = EFI_NOT_STARTED;
  }
  
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Get the remote socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslTcpGetRemoteAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  )
{
  socklen_t LengthInBytes;
  DT_PORT * pPort;
  struct sockaddr_in * pRemoteAddress;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Verify that there is just a single connection
  //
  pPort = pSocket->pPortList;
  if (( NULL != pPort ) && ( NULL == pPort->pLinkSocket )) {
    //
    //  Verify the address length
    //
    LengthInBytes = sizeof ( struct sockaddr_in );
    if ( LengthInBytes <= * pAddressLength ) {
      //
      //  Return the local address
      //
      pTcp4 = &pPort->Context.Tcp4;
      pRemoteAddress = (struct sockaddr_in *)pAddress;
      ZeroMem ( pRemoteAddress, LengthInBytes );
      pRemoteAddress->sin_family = AF_INET;
      pRemoteAddress->sin_len = (uint8_t)LengthInBytes;
      pRemoteAddress->sin_port = SwapBytes16 ( pTcp4->ConfigData.AccessPoint.RemotePort );
      CopyMem ( &pRemoteAddress->sin_addr,
                &pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[0],
                sizeof ( pRemoteAddress->sin_addr ));
      pSocket->errno = 0;
      Status = EFI_SUCCESS;
    }
    else {
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }
  }
  else {
    pSocket->errno = ENOTCONN;
    Status = EFI_NOT_STARTED;
  }
  
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Establish the known port to listen for network connections.

  The ::Tcp4Listen routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::Tcp4Accept routine to remove
  the next connection from the queue and get the associated socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation for the listen routine is available online for reference.

  @param [in] pSocket     Address of the socket structure.

  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen

**/
EFI_STATUS
EslTcpListen4 (
  IN DT_SOCKET * pSocket
  )
{
  DT_PORT * pNextPort;
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Assume no ports are available
    //
    pSocket->errno = EOPNOTSUPP;
    Status = EFI_NOT_READY;

    //
    //  Walk the list of ports
    //
    pPort = pSocket->pPortList;
    while ( NULL != pPort ) {
      //
      //  Assume success
      //
      pSocket->errno = 0;

      //
      //  Use for/break insteak of goto
      //
      for ( ; ; ) {
        //
        //  Create the listen completion event
        //
        pTcp4 = &pPort->Context.Tcp4;
        Status = gBS->CreateEvent ( EVT_NOTIFY_SIGNAL,
                                    TPL_SOCKETS,
                                    (EFI_EVENT_NOTIFY)EslTcpListenComplete4,
                                    pPort,
                                    &pTcp4->ListenToken.CompletionToken.Event );
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_ERROR | DEBUG_LISTEN,
                    "ERROR - Failed to create the listen completion event, Status: %r\r\n",
                    Status ));
          pSocket->errno = ENOMEM;
          break;
        }
        DEBUG (( DEBUG_POOL,
                  "0x%08x: Created listen completion event\r\n",
                  pTcp4->ListenToken.CompletionToken.Event ));

        //
        //  Configure the port
        //
        pTcp4Protocol = pTcp4->pProtocol;
        Status = pTcp4Protocol->Configure ( pTcp4Protocol,
                                            &pTcp4->ConfigData );
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_LISTEN,
                    "ERROR - Failed to configure the Tcp4 port, Status: %r\r\n",
                    Status ));
          switch ( Status ) {
          case EFI_ACCESS_DENIED:
            pSocket->errno = EACCES;
            break;

          default:
          case EFI_DEVICE_ERROR:
            pSocket->errno = EIO;
            break;

          case EFI_INVALID_PARAMETER:
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case EFI_NO_MAPPING:
            pSocket->errno = EAFNOSUPPORT;
            break;

          case EFI_OUT_OF_RESOURCES:
            pSocket->errno = ENOBUFS;
            break;

          case EFI_UNSUPPORTED:
            pSocket->errno = EOPNOTSUPP;
            break;
          }
          break;
        }
        DEBUG (( DEBUG_LISTEN,
                  "0x%08x: Port configured\r\n",
                  pPort ));
        pTcp4->bConfigured = TRUE;

        //
        //  Start the listen operation on the port
        //
        Status = pTcp4Protocol->Accept ( pTcp4Protocol,
                                         &pTcp4->ListenToken );
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_LISTEN,
                    "ERROR - Failed Tcp4 accept, Status: %r\r\n",
                    Status ));
          switch ( Status ) {
          case EFI_ACCESS_DENIED:
            pSocket->errno = EACCES;
            break;

          default:
          case EFI_DEVICE_ERROR:
            pSocket->errno = EIO;
            break;

          case EFI_INVALID_PARAMETER:
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case EFI_NOT_STARTED:
            pSocket->errno = ENETDOWN;
            break;

          case EFI_OUT_OF_RESOURCES:
            pSocket->errno = ENOBUFS;
            break;
          }
          break;
        }
        DEBUG (( DEBUG_LISTEN,
                  "0x%08x: Listen pending on Port\r\n",
                  pPort ));

        //
        //  Listen is pending on this port
        //
        break;
      }

      //
      //  Get the next port
      //
      pNextPort = pPort->pLinkSocket;

      //
      //  Close the port upon error
      //
      if ( EFI_ERROR ( Status ))
      {
        EslTcpPortCloseStart4 ( pPort, TRUE, DEBUG_LISTEN );
      }

      //
      //  Set the next port
      //
      pPort = pNextPort;
    }
    
    //
    //  Determine if any ports are in the listen state
    //
    if ( NULL == pSocket->pPortList ) {
      //
      //  No ports in the listen state
      //
      pSocket->MaxFifoDepth = 0;

      //
      //  Return the last error detected
      //
      break;
    }

    //
    //  Mark the socket as configured
    //
    pSocket->bConfigured = TRUE;

    //
    //  All done
    //
    DEBUG (( DEBUG_LISTEN,
              "0x%08x: pSocket - Listen pending on socket\r\n",
              pSocket ));
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the connection attempt

  A system has initiated a connection attempt with a socket in the
  listen state.  Attempt to complete the connection.

  @param  Event         The listen completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpListenComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  EFI_HANDLE ChildHandle;
  EFI_TCP4_CONFIG_DATA * pConfigData;
  DT_LAYER * pLayer;
  DT_PORT * pNewPort;
  DT_SOCKET * pNewSocket;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;
  EFI_HANDLE TcpPortHandle;
  EFI_STATUS TempStatus;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Determine if this connection fits into the connection FIFO
  //
  pSocket = pPort->pSocket;
  TcpPortHandle = pPort->Context.Tcp4.ListenToken.NewChildHandle;
  if (( SOCKET_STATE_LISTENING == pSocket->State )
    && ( pSocket->MaxFifoDepth > pSocket->FifoDepth )) {
    //
    //  Allocate a socket for this connection
    //
    ChildHandle = NULL;
    pLayer = &mEslLayer;
    Status = EslSocketAllocate ( &ChildHandle,
                                 DEBUG_CONNECTION,
                                 &pNewSocket );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Clone the socket parameters
      //
      pNewSocket->Domain = pSocket->Domain;
      pNewSocket->Protocol = pSocket->Protocol;
      pNewSocket->Type = pSocket->Type;

      //
      //  Allocate a port for this connection
      //
      pTcp4 = &pPort->Context.Tcp4;
      Status = EslTcpPortAllocate4 ( pNewSocket,
                                     pPort->pService,
                                     TcpPortHandle,
                                     &pTcp4->ConfigData.AccessPoint.StationAddress.Addr[0],
                                     0,
                                     DEBUG_CONNECTION,
                                     &pNewPort );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Restart the listen operation on the port
        //
        pTcp4Protocol = pTcp4->pProtocol;
        Status = pTcp4Protocol->Accept ( pTcp4Protocol,
                                         &pTcp4->ListenToken );

        //
        //  Close the TCP port using SocketClose
        //
        TcpPortHandle = NULL;
        pTcp4 = &pNewPort->Context.Tcp4;
        pTcp4->bConfigured = TRUE;

        //
        //  Check for an accept call error
        //
        if ( !EFI_ERROR ( Status )) {
          //
          //  Get the port configuration
          //
          pConfigData = &pTcp4->ConfigData;
          pConfigData->ControlOption = &pTcp4->Option;
          pTcp4Protocol = pTcp4->pProtocol;
          Status = pTcp4Protocol->GetModeData ( pTcp4Protocol,
                                                NULL,
                                                pConfigData,
                                                NULL,
                                                NULL,
                                                NULL );
          if ( !EFI_ERROR ( Status )) {
            //
            //  Add the new socket to the connection FIFO
            //
            if ( NULL == pSocket->pFifoTail ) {
              //
              //  First connection
              //
              pSocket->pFifoHead = pNewSocket;
            }
            else {
              //
              //  Add to end of list.
              //
              pSocket->pFifoTail->pNextConnection = pNewSocket;
            }
            pSocket->pFifoTail = pNewSocket;
            pSocket->FifoDepth += 1;

            //
            //  Update the socket state
            //
            pNewSocket->State = SOCKET_STATE_IN_FIFO;

            //
            //  Log the connection
            //
            DEBUG (( DEBUG_CONNECTION | DEBUG_INFO,
                      "0x%08x: Socket on port %d.%d.%d.%d:%d connected to %d.%d.%d.%d:%d\r\n",
                      pNewSocket,
                      pConfigData->AccessPoint.StationAddress.Addr[0],
                      pConfigData->AccessPoint.StationAddress.Addr[1],
                      pConfigData->AccessPoint.StationAddress.Addr[2],
                      pConfigData->AccessPoint.StationAddress.Addr[3],
                      pConfigData->AccessPoint.StationPort,
                      pConfigData->AccessPoint.RemoteAddress.Addr[0],
                      pConfigData->AccessPoint.RemoteAddress.Addr[1],
                      pConfigData->AccessPoint.RemoteAddress.Addr[2],
                      pConfigData->AccessPoint.RemoteAddress.Addr[3],
                      pConfigData->AccessPoint.RemotePort ));
            DEBUG (( DEBUG_CONNECTION | DEBUG_INFO,
                      "0x%08x: Listen socket adding socket 0x%08x to FIFO, depth: %d\r\n",
                      pSocket,
                      pNewSocket,
                      pSocket->FifoDepth ));

            //
            //  Start the receive operation
            //
            EslTcpRxStart4 ( pNewPort );
          }
          else {
            DEBUG (( DEBUG_ERROR | DEBUG_CONNECTION | DEBUG_INFO,
                      "ERROR - GetModeData failed on port 0x%08x, Status: %r\r\n",
                      pNewPort,
                      Status ));
          }
        }
        else {
          //
          //  The listen failed on this port
          //
          DEBUG (( DEBUG_LISTEN | DEBUG_INFO,
                    "ERROR - Listen failed on port 0x%08x, Status: %r\r\n",
                    pPort,
                    Status ));

          //
          //  Close the listening port
          //
          EslTcpPortCloseStart4 ( pPort, TRUE, DEBUG_LISTEN );
        }
      }

      //
      //  Done with the socket if necessary
      //
      if ( EFI_ERROR ( Status )) {
        TempStatus = EslSocketCloseStart ( &pNewSocket->SocketProtocol,
                                           TRUE,
                                           &pSocket->errno );
        ASSERT ( EFI_SUCCESS == TempStatus );
      }
    }
  }
  else {
    DEBUG (( DEBUG_CONNECTION,
              "0x%08x: Socket FIFO full, connection refused\r\n",
              pSocket ));

    //
    //  The FIFO is full or the socket is in the wrong state
    //
    Status = EFI_BUFFER_TOO_SMALL;
  }

  //
  //  Close the connection if necessary
  //
  if (( EFI_ERROR ( Status ))
    && ( NULL == TcpPortHandle )) {
    //
    // TODO: Finish this code path
    //  The new connection does not fit into the connection FIFO
    //
    //  Process:
    //    Call close
    //    Release the resources
    
  }

  DBG_EXIT ( );
}


/**
  Allocate and initialize a DT_PORT structure.

  @param [in] pSocket     Address of the socket structure.
  @param [in] pService    Address of the DT_SERVICE structure.
  @param [in] ChildHandle TCP4 child handle
  @param [in] pIpAddress  Buffer containing IP4 network address of the local host
  @param [in] PortNumber  Tcp4 port number
  @param [in] DebugFlags  Flags for debug messages
  @param [out] ppPort     Buffer to receive new DT_PORT structure address

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslTcpPortAllocate4 (
  IN DT_SOCKET * pSocket,
  IN DT_SERVICE * pService,
  IN EFI_HANDLE ChildHandle,
  IN CONST UINT8 * pIpAddress,
  IN UINT16 PortNumber,
  IN UINTN DebugFlags,
  OUT DT_PORT ** ppPort
  )
{
  UINTN LengthInBytes;
  EFI_TCP4_ACCESS_POINT * pAccessPoint;
  DT_LAYER * pLayer;
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  for ( ; ; ) {
    //
    //  Allocate a port structure
    //
    pLayer = &mEslLayer;
    LengthInBytes = sizeof ( *pPort );
    Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                 LengthInBytes,
                                 (VOID **)&pPort );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL | DEBUG_INIT,
                "ERROR - Failed to allocate the port structure, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      pPort = NULL;
      break;
    }
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pPort, %d bytes\r\n",
              pPort,
              LengthInBytes ));

    //
    //  Initialize the port
    //
    ZeroMem ( pPort, LengthInBytes );
    pPort->Signature = PORT_SIGNATURE;
    pPort->pService = pService;
    pPort->pSocket = pSocket;
    pPort->pfnCloseStart = EslTcpPortCloseStart4;
    pPort->DebugFlags = DebugFlags;

    //
    //  Allocate the receive event
    //
    pTcp4 = &pPort->Context.Tcp4;
    Status = gBS->CreateEvent (  EVT_NOTIFY_SIGNAL,
                                 TPL_SOCKETS,
                                 (EFI_EVENT_NOTIFY)EslTcpRxComplete4,
                                 pPort,
                                 &pTcp4->RxToken.CompletionToken.Event);
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the receive event, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DEBUG_RX | DEBUG_POOL,
              "0x%08x: Created receive event\r\n",
              pTcp4->RxToken.CompletionToken.Event ));

    //
    //  Allocate the urgent transmit event
    //
    Status = gBS->CreateEvent (  EVT_NOTIFY_SIGNAL,
                                 TPL_SOCKETS,
                                 (EFI_EVENT_NOTIFY)EslTcpTxOobComplete4,
                                 pPort,
                                 &pTcp4->TxOobToken.CompletionToken.Event);
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the urgent transmit event, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DEBUG_CLOSE | DEBUG_POOL,
              "0x%08x: Created urgent transmit event\r\n",
              pTcp4->TxOobToken.CompletionToken.Event ));

    //
    //  Allocate the normal transmit event
    //
    Status = gBS->CreateEvent (  EVT_NOTIFY_SIGNAL,
                                 TPL_SOCKETS,
                                 (EFI_EVENT_NOTIFY)EslTcpTxComplete4,
                                 pPort,
                                 &pTcp4->TxToken.CompletionToken.Event);
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the normal transmit event, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DEBUG_CLOSE | DEBUG_POOL,
              "0x%08x: Created normal transmit event\r\n",
              pTcp4->TxToken.CompletionToken.Event ));

    //
    //  Allocate the close event
    //
    Status = gBS->CreateEvent (  EVT_NOTIFY_SIGNAL,
                                 TPL_SOCKETS,
                                 (EFI_EVENT_NOTIFY)EslTcpPortCloseComplete4,
                                 pPort,
                                 &pTcp4->CloseToken.CompletionToken.Event);
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the close event, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DEBUG_CLOSE | DEBUG_POOL,
              "0x%08x: Created close event\r\n",
              pTcp4->CloseToken.CompletionToken.Event ));

    //
    //  Allocate the connection event
    //
    Status = gBS->CreateEvent (  EVT_NOTIFY_SIGNAL,
                                 TPL_SOCKETS,
                                 (EFI_EVENT_NOTIFY)EslTcpConnectComplete4,
                                 pPort,
                                 &pTcp4->ConnectToken.CompletionToken.Event);
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to create the connect event, Status: %r\r\n",
                Status ));
      pSocket->errno = ENOMEM;
      break;
    }
    DEBUG (( DEBUG_CLOSE | DEBUG_POOL,
              "0x%08x: Created connect event\r\n",
              pTcp4->ConnectToken.CompletionToken.Event ));

    //
    //  Open the port protocol
    //
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiTcp4ProtocolGuid,
                    (VOID **) &pTcp4->pProtocol,
                    pLayer->ImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to open gEfiTcp4ProtocolGuid on controller 0x%08x\r\n",
                pTcp4->Handle ));
      pSocket->errno = EEXIST;
      break;
    }
    DEBUG (( DebugFlags,
              "0x%08x: gEfiTcp4ProtocolGuid opened on controller 0x%08x\r\n",
              pTcp4->pProtocol,
              ChildHandle ));

    //
    //  Set the port address
    //
    pTcp4->Handle = ChildHandle;
    pAccessPoint = &pPort->Context.Tcp4.ConfigData.AccessPoint;
    pAccessPoint->StationPort = PortNumber;
    if (( 0 == pIpAddress[0])
      && ( 0 == pIpAddress[1])
      && ( 0 == pIpAddress[2])
      && ( 0 == pIpAddress[3])) {
      pAccessPoint->UseDefaultAddress = TRUE;
    }
    else {
      pAccessPoint->StationAddress.Addr[0] = pIpAddress[0];
      pAccessPoint->StationAddress.Addr[1] = pIpAddress[1];
      pAccessPoint->StationAddress.Addr[2] = pIpAddress[2];
      pAccessPoint->StationAddress.Addr[3] = pIpAddress[3];
      pAccessPoint->SubnetMask.Addr[0] = 0xff;
      pAccessPoint->SubnetMask.Addr[1] = 0xff;
      pAccessPoint->SubnetMask.Addr[2] = 0xff;
      pAccessPoint->SubnetMask.Addr[3] = 0xff;
    }
    pAccessPoint->ActiveFlag = FALSE;
    pTcp4->ConfigData.TimeToLive = 255;

    //
    //  Verify the socket layer synchronization
    //
    VERIFY_TPL ( TPL_SOCKETS );

    //
    //  Add this port to the socket
    //
    pPort->pLinkSocket = pSocket->pPortList;
    pSocket->pPortList = pPort;
    DEBUG (( DebugFlags,
              "0x%08x: Socket adding port: 0x%08x\r\n",
              pSocket,
              pPort ));

    //
    //  Add this port to the service
    //
    pPort->pLinkService = pService->pPortList;
    pService->pPortList = pPort;

    //
    //  Return the port
    //
    *ppPort = pPort;
    break;
  }

  //
  //  Clean up after the error if necessary
  //
  if (( EFI_ERROR ( Status )) && ( NULL != pPort )) {
    //
    //  Close the port
    //
    EslTcpPortClose4 ( pPort );
  }
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Close a TCP4 port.

  This routine releases the resources allocated by
  ::TcpPortAllocate4().
  
  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS     The port is closed
  @retval other           Port close error

**/
EFI_STATUS
EslTcpPortClose4 (
  IN DT_PORT * pPort
  )
{
  UINTN DebugFlags;
  DT_LAYER * pLayer;
  DT_PACKET * pPacket;
  DT_PORT * pPreviousPort;
  DT_SERVICE * pService;
  DT_SOCKET * pSocket;
  EFI_SERVICE_BINDING_PROTOCOL * pTcp4Service;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;
  
  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Locate the port in the socket list
  //
  Status = EFI_SUCCESS;
  pLayer = &mEslLayer;
  DebugFlags = pPort->DebugFlags;
  pSocket = pPort->pSocket;
  pPreviousPort = pSocket->pPortList;
  if ( pPreviousPort == pPort ) {
    //
    //  Remove this port from the head of the socket list
    //
    pSocket->pPortList = pPort->pLinkSocket;
  }
  else {
    //
    //  Locate the port in the middle of the socket list
    //
    while (( NULL != pPreviousPort )
      && ( pPreviousPort->pLinkSocket != pPort )) {
      pPreviousPort = pPreviousPort->pLinkSocket;
    }
    if ( NULL != pPreviousPort ) {
      //
      //  Remove the port from the middle of the socket list
      //
      pPreviousPort->pLinkSocket = pPort->pLinkSocket;
    }
  }

  //
  //  Locate the port in the service list
  //
  pService = pPort->pService;
  pPreviousPort = pService->pPortList;
  if ( pPreviousPort == pPort ) {
    //
    //  Remove this port from the head of the service list
    //
    pService->pPortList = pPort->pLinkService;
  }
  else {
    //
    //  Locate the port in the middle of the service list
    //
    while (( NULL != pPreviousPort )
      && ( pPreviousPort->pLinkService != pPort )) {
      pPreviousPort = pPreviousPort->pLinkService;
    }
    if ( NULL != pPreviousPort ) {
      //
      //  Remove the port from the middle of the service list
      //
      pPreviousPort->pLinkService = pPort->pLinkService;
    }
  }

  //
  //  Empty the urgent receive queue
  //
  pTcp4 = &pPort->Context.Tcp4;
  while ( NULL != pSocket->pRxOobPacketListHead ) {
    pPacket = pSocket->pRxOobPacketListHead;
    pSocket->pRxOobPacketListHead = pPacket->pNext;
    pSocket->RxOobBytes -= pPacket->Op.Tcp4Rx.ValidBytes;
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }
  pSocket->pRxOobPacketListTail = NULL;
  ASSERT ( 0 == pSocket->RxOobBytes );

  //
  //  Empty the receive queue
  //
  while ( NULL != pSocket->pRxPacketListHead ) {
    pPacket = pSocket->pRxPacketListHead;
    pSocket->pRxPacketListHead = pPacket->pNext;
    pSocket->RxBytes -= pPacket->Op.Tcp4Rx.ValidBytes;
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }
  pSocket->pRxPacketListTail = NULL;
  ASSERT ( 0 == pSocket->RxBytes );

  //
  //  Empty the receive free queue
  //
  while ( NULL != pSocket->pRxFree ) {
    pPacket = pSocket->pRxFree;
    pSocket->pRxFree = pPacket->pNext;
    EslSocketPacketFree ( pPacket, DEBUG_RX );
  }

  //
  //  Done with the connect event
  //
  if ( NULL != pTcp4->ConnectToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->ConnectToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed connect event\r\n",
                pTcp4->ConnectToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the connect event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the close event
  //
  if ( NULL != pTcp4->CloseToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->CloseToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed close event\r\n",
                pTcp4->CloseToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the close event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the listen completion event
  //
  if ( NULL != pTcp4->ListenToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->ListenToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed listen completion event\r\n",
                pTcp4->ListenToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the listen completion event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the receive event
  //
  if ( NULL != pTcp4->RxToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->RxToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed receive event\r\n",
                pTcp4->RxToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the receive event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the normal transmit event
  //
  if ( NULL != pTcp4->TxToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->TxToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed normal transmit event\r\n",
                pTcp4->TxToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the normal transmit event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the urgent transmit event
  //
  if ( NULL != pTcp4->TxOobToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( pTcp4->TxOobToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Closed urgent transmit event\r\n",
                pTcp4->TxOobToken.CompletionToken.Event ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close the urgent transmit event, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the TCP protocol
  //
  pTcp4Service = pService->pInterface;
  if ( NULL != pTcp4->pProtocol ) {
    Status = gBS->CloseProtocol ( pTcp4->Handle,
                                  &gEfiTcp4ProtocolGuid,
                                  pLayer->ImageHandle,
                                  NULL );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags,
                "0x%08x: gEfiTcp4ProtocolGuid closed on controller 0x%08x\r\n",
                pTcp4->pProtocol,
                pTcp4->Handle ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags,
                "ERROR - Failed to close gEfiTcp4ProtocolGuid opened on controller 0x%08x, Status: %r\r\n",
                pTcp4->Handle,
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Done with the TCP port
  //
  if ( NULL != pTcp4->Handle ) {
    Status = pTcp4Service->DestroyChild ( pTcp4Service,
                                          pTcp4->Handle );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL,
                "0x%08x: Tcp4 port handle destroyed\r\n",
                pTcp4->Handle ));
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL,
                "ERROR - Failed to destroy the Tcp4 port handle, Status: %r\r\n",
                Status ));
      ASSERT ( EFI_SUCCESS == Status );
    }
  }

  //
  //  Release the port structure
  //
  Status = gBS->FreePool ( pPort );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL,
              "0x%08x: Free pPort, %d bytes\r\n",
              pPort,
              sizeof ( *pPort )));
  }
  else {
    DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL,
              "ERROR - Failed to free pPort: 0x%08x, Status: %r\r\n",
              pPort,
              Status ));
    ASSERT ( EFI_SUCCESS == Status );
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the port close completion

  @param  Event         The close completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpPortCloseComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Update the port state
  //
  pPort->State = PORT_STATE_CLOSE_DONE;

  //
  //  Release the resources once the receive operation completes
  //
  Status = EslTcpPortCloseRxDone4 ( pPort );
  DBG_EXIT_STATUS ( Status );
}


/**
  Start the close operation on a TCP4 port, state 1.

  Closing a port goes through the following states:
  1. Port close starting - Mark the port as closing and wait for transmission to complete
  2. Port TX close done - Transmissions complete, close the port and abort the receives
  3. Port RX close done - Receive operations complete, close the port
  4. Port closed - Release the port resources
  
  @param [in] pPort       Address of the port structure.
  @param [in] bCloseNow   Set TRUE to abort active transfers
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port has started the closing process
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslTcpPortCloseStart4 (
  IN DT_PORT * pPort,
  IN BOOLEAN bCloseNow,
  IN UINTN DebugFlags
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Mark the port as closing
  //
  Status = EFI_ALREADY_STARTED;
  pSocket = pPort->pSocket;
  pSocket->errno = EALREADY;
  if ( PORT_STATE_CLOSE_STARTED > pPort->State ) {

    //
    //  Update the port state
    //
    pPort->State = PORT_STATE_CLOSE_STARTED;
    DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
              "0x%08x: Port Close State: PORT_STATE_CLOSE_STARTED\r\n",
              pPort ));
    pPort->bCloseNow = bCloseNow;
    pPort->DebugFlags = DebugFlags;

    //
    //  Determine if transmits are complete
    //
    Status = EslTcpPortCloseTxDone4 ( pPort );
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Port close state 3

  Continue the close operation after the receive is complete.

  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS         The port is closed
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslTcpPortCloseRxDone4 (
  IN DT_PORT * pPort
  )
{
  PORT_STATE PortState;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Verify that the port is closing
  //
  Status = EFI_ALREADY_STARTED;
  PortState = pPort->State;
  if (( PORT_STATE_CLOSE_TX_DONE == PortState )
    || ( PORT_STATE_CLOSE_DONE == PortState )) {
    //
    //  Determine if the receive operation is pending
    //
    Status = EFI_NOT_READY;
    pTcp4 = &pPort->Context.Tcp4;
    if ( NULL == pTcp4->pReceivePending ) {
      //
      //  The receive operation is complete
      //  Update the port state
      //
      pPort->State = PORT_STATE_CLOSE_RX_DONE;
      DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                "0x%08x: Port Close State: PORT_STATE_CLOSE_RX_DONE\r\n",
                pPort ));

      //
      //  Determine if the close operation has completed
      //
      if ( PORT_STATE_CLOSE_DONE == PortState ) {
        //
        //  The close operation has completed
        //  Release the port resources
        //
        Status = EslTcpPortClose4 ( pPort );
      }
      else
      {
        DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                  "0x%08x: Port Close: Close operation still pending!\r\n",
                  pPort ));
      }
    }
    else {
      DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                "0x%08x: Port Close: Receive still pending!\r\n",
                pPort ));
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Port close state 2

  Continue the close operation after the transmission is complete.

  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslTcpPortCloseTxDone4 (
  IN DT_PORT * pPort
  )
{
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  All transmissions are complete or must be stopped
  //  Mark the port as TX complete
  //
  Status = EFI_ALREADY_STARTED;
  if ( PORT_STATE_CLOSE_STARTED == pPort->State ) {
    //
    //  Verify that the transmissions are complete
    //
    pSocket = pPort->pSocket;
    if ( pPort->bCloseNow
         || ( EFI_SUCCESS != pSocket->TxError )
         || (( 0 == pSocket->TxOobBytes )
                && ( 0 == pSocket->TxBytes ))) {
      //
      //  Start the close operation on the port
      //
      pTcp4 = &pPort->Context.Tcp4;
      pTcp4->CloseToken.AbortOnClose = FALSE;
      pTcp4Protocol = pTcp4->pProtocol;
      if ( !pTcp4->bConfigured ) {
        //
        //  Skip the close operation since the port is not
        //  configured
        //
        //  Update the port state
        //
        pPort->State = PORT_STATE_CLOSE_DONE;
        DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                  "0x%08x: Port Close State: PORT_STATE_CLOSE_DONE\r\n",
                  pPort ));
        Status = EFI_SUCCESS;
      }
      else {
        //
        //  Close the configured port
        //
        Status = pTcp4Protocol->Close ( pTcp4Protocol,
                                        &pTcp4->CloseToken );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( pPort->DebugFlags | DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Port close started\r\n",
                    pPort ));

          //
          //  Update the port state
          //
          pPort->State = PORT_STATE_CLOSE_TX_DONE;
          DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                    "0x%08x: Port Close State: PORT_STATE_CLOSE_TX_DONE\r\n",
                    pPort ));
        }
        else {
          DEBUG (( DEBUG_ERROR | pPort->DebugFlags | DEBUG_CLOSE | DEBUG_INFO,
                   "ERROR - Close failed on port 0x%08x, Status: %r\r\n",
                   pPort,
                   Status ));
          ASSERT ( EFI_SUCCESS == Status );
        }
      }

      //
      //  Determine if the receive operation is pending
      //
      if ( !EFI_ERROR ( Status )) {
        Status = EslTcpPortCloseRxDone4 ( pPort );
      }
    }
    else {
      //
      //  Transmissions are still active, exit
      //
      DEBUG (( DEBUG_CLOSE | DEBUG_INFO,
                "0x%08x: Port Close: Transmits are still pending!\r\n",
                pPort ));
      Status = EFI_NOT_READY;
      pSocket->errno = EAGAIN;
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Receive data from a network connection.


  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslTcpReceive4 (
  IN DT_SOCKET * pSocket,
  IN INT32 Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  )
{
  socklen_t AddressLength;
  size_t BytesToCopy;
  in_addr_t IpAddress;
  size_t LengthInBytes;
  DT_PACKET * pPacket;
  DT_PORT * pPort;
  DT_PACKET ** ppQueueHead;
  DT_PACKET ** ppQueueTail;
  struct sockaddr_in * pRemoteAddress;
  size_t * pRxDataBytes;
  DT_TCP4_CONTEXT * pTcp4;
  struct sockaddr_in RemoteAddress;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  Status = EFI_UNSUPPORTED;
  pSocket->errno = ENOTCONN;

  //
  //  Verify that the socket is connected
  //
  if (( SOCKET_STATE_CONNECTED == pSocket->State )
    || ( PORT_STATE_RX_ERROR == pSocket->State )) {
    //
    //  Locate the port
    //
    pPort = pSocket->pPortList;
    if ( NULL != pPort ) {
      //
      //  Determine the queue head
      //
      pTcp4 = &pPort->Context.Tcp4;
      if ( 0 != ( Flags & MSG_OOB )) {
        ppQueueHead = &pSocket->pRxOobPacketListHead;
        ppQueueTail = &pSocket->pRxOobPacketListTail;
        pRxDataBytes = &pSocket->RxOobBytes;
      }
      else {
        ppQueueHead = &pSocket->pRxPacketListHead;
        ppQueueTail = &pSocket->pRxPacketListTail;
        pRxDataBytes = &pSocket->RxBytes;
      }

      //
      //  Determine if there is any data on the queue
      //
      pPacket = *ppQueueHead;
      if ( NULL != pPacket ) {
        //
        //  Validate the return address parameters
        //
        if (( NULL == pAddress ) || ( NULL != pAddressLength )) {
          //
          //  Return the remote system address if requested
          //
          if ( NULL != pAddress ) {
            //
            //  Build the remote address
            //
            ZeroMem ( &RemoteAddress, sizeof ( RemoteAddress ));
            RemoteAddress.sin_len = sizeof ( RemoteAddress );
            RemoteAddress.sin_family = AF_INET;
            IpAddress = pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[3];
            IpAddress <<= 8;
            IpAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[2];
            IpAddress <<= 8;
            IpAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[1];
            IpAddress <<= 8;
            IpAddress |= pTcp4->ConfigData.AccessPoint.RemoteAddress.Addr[0];
            RemoteAddress.sin_addr.s_addr = IpAddress;
            RemoteAddress.sin_port = SwapBytes16 ( pTcp4->ConfigData.AccessPoint.RemotePort );

            //
            //  Copy the address
            //
            pRemoteAddress = (struct sockaddr_in *)pAddress;
            AddressLength = sizeof ( *pRemoteAddress );
            if ( AddressLength > *pAddressLength ) {
              AddressLength = *pAddressLength;
            }
            CopyMem ( pRemoteAddress,
                      &RemoteAddress,
                      AddressLength );

            //
            //  Update the address length
            //
            *pAddressLength = AddressLength;
          }

          //
          //  Copy the received data
          //
          LengthInBytes = 0;
          do {
            //
            //  Determine the amount of received data
            //
            BytesToCopy = pPacket->Op.Tcp4Rx.ValidBytes;
            if (( BufferLength - LengthInBytes ) < BytesToCopy ) {
              BytesToCopy = BufferLength - LengthInBytes;
            }
            LengthInBytes += BytesToCopy;

            //
            //  Move the data into the buffer
            //
            DEBUG (( DEBUG_RX,
                      "0x%08x: Port copy packet 0x%08x data into 0x%08x, 0x%08x bytes\r\n",
                      pPort,
                      pPacket,
                      pBuffer,
                      BytesToCopy ));
            CopyMem ( pBuffer, pPacket->Op.Tcp4Rx.pBuffer, BytesToCopy );

            //
            //  Determine if the data is being read
            //
            if ( 0 == ( Flags & MSG_PEEK )) {
              //
              //  Account for the bytes consumed
              //
              pPacket->Op.Tcp4Rx.pBuffer += BytesToCopy;
              pPacket->Op.Tcp4Rx.ValidBytes -= BytesToCopy;
              *pRxDataBytes -= BytesToCopy;
              DEBUG (( DEBUG_RX,
                        "0x%08x: Port account for 0x%08x bytes\r\n",
                        pPort,
                        BytesToCopy ));

              //
              //  Determine if the entire packet was consumed
              //
              if (( 0 == pPacket->Op.Tcp4Rx.ValidBytes )
                || ( SOCK_STREAM != pSocket->Type )) {
                //
                //  All done with this packet
                //  Account for any discarded data
                //
                *pRxDataBytes -= pPacket->Op.Tcp4Rx.ValidBytes;
                if ( 0 != pPacket->Op.Tcp4Rx.ValidBytes ) {
                  DEBUG (( DEBUG_RX,
                            "0x%08x: Port, packet read, skipping over 0x%08x bytes\r\n",
                            pPort,
                            pPacket->Op.Tcp4Rx.ValidBytes ));
                }

                //
                //  Remove this packet from the queue
                //
                *ppQueueHead = pPacket->pNext;
                if ( NULL == *ppQueueHead ) {
                  *ppQueueTail = NULL;
                }

                //
                //  Move the packet to the free queue
                //
                pPacket->pNext = pSocket->pRxFree;
                pSocket->pRxFree = pPacket;
                DEBUG (( DEBUG_RX,
                          "0x%08x: Port freeing packet 0x%08x\r\n",
                          pPort,
                          pPacket ));

                //
                //  Restart this receive operation if necessary
                //
                if (( NULL == pTcp4->pReceivePending )
                  && ( MAX_RX_DATA > pSocket->RxBytes )) {
                    EslTcpRxStart4 ( pPort );
                }
              }
            }

            //
            //  Get the next packet
            //
            pPacket = *ppQueueHead;
          } while (( SOCK_STREAM == pSocket->Type )
                && ( NULL != pPacket )
                && ( 0 == ( Flags & MSG_PEEK ))
                && ( BufferLength > LengthInBytes ));

          //
          //  Return the data length
          //
          *pDataLength = LengthInBytes;

          //
          //  Successful operation
          //
          Status = EFI_SUCCESS;
          pSocket->errno = 0;
        }
        else {
          //
          //  Bad return address pointer and length
          //
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EINVAL;
        }
      }
      else {
        //
        //  The queue is empty
        //  Determine if it is time to return the receive error
        //
        if ( EFI_ERROR ( pSocket->RxError )
          && ( NULL == pSocket->pRxPacketListHead )
          && ( NULL == pSocket->pRxOobPacketListHead )) {
          Status = pSocket->RxError;
          switch ( Status ) {
          default:
            pSocket->errno = EIO;
            break;
          
          case EFI_CONNECTION_FIN:
            pSocket->errno = ESHUTDOWN;
            break;

          case EFI_CONNECTION_REFUSED:
            pSocket->errno = ECONNREFUSED;
            break;

          case EFI_CONNECTION_RESET:
            pSocket->errno = ECONNRESET;
            break;

          case EFI_HOST_UNREACHABLE:
            pSocket->errno = EHOSTUNREACH;
            break;
          
          case EFI_NETWORK_UNREACHABLE:
            pSocket->errno = ENETUNREACH;
            break;
          
          case EFI_PORT_UNREACHABLE:
            pSocket->errno = EPROTONOSUPPORT;
            break;
          
          case EFI_PROTOCOL_UNREACHABLE:
            pSocket->errno = ENOPROTOOPT;
            break;
          }
          pSocket->RxError = EFI_SUCCESS;
        }
        else {
          Status = EFI_NOT_READY;
          pSocket->errno = EAGAIN;
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Cancel the receive operations

  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The cancel was successful

 **/
EFI_STATUS
EslTcpRxCancel4 (
  IN DT_SOCKET * pSocket
  )
{
  DT_PACKET * pPacket;
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  Status = EFI_NOT_FOUND;

  //
  //  Locate the port
  //
  pPort = pSocket->pPortList;
  if ( NULL != pPort ) {
    //
    //  Determine if a receive is pending
    //
    pTcp4 = &pPort->Context.Tcp4;
    pPacket = pTcp4->pReceivePending;
    if ( NULL != pPacket ) {
      //
      //  Attempt to cancel the receive operation
      //
      pTcp4Protocol = pTcp4->pProtocol;
      Status = pTcp4Protocol->Cancel ( pTcp4Protocol,
                                       &pTcp4->RxToken.CompletionToken );
      if ( EFI_NOT_FOUND == Status ) {
        //
        //  The receive is complete
        //
        Status = EFI_SUCCESS;
      }
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the receive completion

  Buffer the data that was just received.

  @param  Event         The receive completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpRxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  BOOLEAN bUrgent;
  size_t LengthInBytes;
  DT_PACKET * pPacket;
  DT_PACKET * pPrevious;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Mark this receive complete
  //
  pTcp4 = &pPort->Context.Tcp4;
  pPacket = pTcp4->pReceivePending;
  pTcp4->pReceivePending = NULL;

  //
  //  Determine if this receive was successful
  //
  pSocket = pPort->pSocket;
  Status = pTcp4->RxToken.CompletionToken.Status;
  if (( !EFI_ERROR ( Status )) && ( !pSocket->bRxDisable )) {
    //
    //  Set the buffer size and address
    //
    pPacket->Op.Tcp4Rx.pBuffer = pPacket->Op.Tcp4Rx.RxData.FragmentTable[0].FragmentBuffer;
    LengthInBytes = pPacket->Op.Tcp4Rx.RxData.DataLength;
    pPacket->Op.Tcp4Rx.ValidBytes = LengthInBytes;
    pPacket->pNext = NULL;

    //
    //  Queue this packet
    //
    bUrgent = pPacket->Op.Tcp4Rx.RxData.UrgentFlag;
    if ( bUrgent ) {
      //
      //  Add packet to the urgent list
      //
      pPrevious = pSocket->pRxOobPacketListTail;
      if ( NULL == pPrevious ) {
        pSocket->pRxOobPacketListHead = pPacket;
      }
      else {
        pPrevious->pNext = pPacket;
      }
      pSocket->pRxOobPacketListTail = pPacket;

      //
      //  Account for the urgent data
      //
      pSocket->RxOobBytes += LengthInBytes;
    }
    else {
      //
      //  Add packet to the normal list
      //
      pPrevious = pSocket->pRxPacketListTail;
      if ( NULL == pPrevious ) {
        pSocket->pRxPacketListHead = pPacket;
      }
      else {
        pPrevious->pNext = pPacket;
      }
      pSocket->pRxPacketListTail = pPacket;

      //
      //  Account for the normal data
      //
      pSocket->RxBytes += LengthInBytes;
    }

    //
    //  Log the received data
    //
    DEBUG (( DEBUG_RX | DEBUG_INFO,
              "0x%08x: Packet queued on port 0x%08x with 0x%08x bytes of %s data\r\n",
              pPacket,
              pPort,
              LengthInBytes,
              bUrgent ? L"urgent" : L"normal" ));

    //
    //  Attempt to restart this receive operation
    //
    if ( pSocket->MaxRxBuf > pSocket->RxBytes ) {
      EslTcpRxStart4 ( pPort );
    }
    else {
      DEBUG (( DEBUG_RX,
                "0x%08x: Port RX suspended, 0x%08x bytes queued\r\n",
                pPort,
                pSocket->RxBytes ));
    }
  }
  else
  {
    DEBUG (( DEBUG_RX | DEBUG_INFO,
              "ERROR - Receiving packet 0x%08x, on port 0x%08x, Status:%r\r\n",
              pPacket,
              pPort,
              Status ));

    //
    //  Receive error, free the packet save the error
    //
    EslSocketPacketFree ( pPacket, DEBUG_RX );
    if ( !EFI_ERROR ( pSocket->RxError )) {
      pSocket->RxError = Status;
    }

    //
    //  Update the port state
    //
    if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
      EslTcpPortCloseRxDone4 ( pPort );
    }
    else {
      if ( EFI_ERROR ( Status )) {
        pPort->State = PORT_STATE_RX_ERROR;
      }
    }
  }

  DBG_EXIT ( );
}


/**
  Start a receive operation

  @param [in] pPort       Address of the DT_PORT structure.

 **/
VOID
EslTcpRxStart4 (
  IN DT_PORT * pPort
  )
{
  size_t LengthInBytes;
  DT_PACKET * pPacket;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Determine if a receive is already pending
  //
  Status = EFI_SUCCESS;
  pPacket = NULL;
  pSocket = pPort->pSocket;
  pTcp4 = &pPort->Context.Tcp4;
  if ( !EFI_ERROR ( pPort->pSocket->RxError )) {
    if (( NULL == pTcp4->pReceivePending )
      && ( PORT_STATE_CLOSE_STARTED > pPort->State )) {
      //
      //  Determine if there are any free packets
      //
      pPacket = pSocket->pRxFree;
      LengthInBytes = sizeof ( pPacket->Op.Tcp4Rx.Buffer );
      if ( NULL != pPacket ) {
        //
        //  Remove this packet from the free list
        //
        pSocket->pRxFree = pPacket->pNext;
        DEBUG (( DEBUG_RX,
                  "0x%08x: Port removed packet 0x%08x from free list\r\n",
                  pPort,
                  pPacket ));
      }
      else {
        //
        //  Allocate a packet structure
        //
        Status = EslSocketPacketAllocate ( &pPacket,
                                           sizeof ( pPacket->Op.Tcp4Rx ),
                                           DEBUG_RX );
        if ( EFI_ERROR ( Status )) {
          pPacket = NULL;
          DEBUG (( DEBUG_ERROR | DEBUG_RX,
                    "0x%08x: Port failed to allocate RX packet, Status: %r\r\n",
                    pPort,
                    Status ));
        }
      }

      //
      //  Determine if a packet is available
      //
      if ( NULL != pPacket ) {
        //
        //  Initialize the buffer for receive
        //
        pTcp4->RxToken.Packet.RxData = &pPacket->Op.Tcp4Rx.RxData;
        pPacket->Op.Tcp4Rx.RxData.DataLength = (UINT32) LengthInBytes;
        pPacket->Op.Tcp4Rx.RxData.FragmentCount = 1;
        pPacket->Op.Tcp4Rx.RxData.FragmentTable [0].FragmentLength = (UINT32) LengthInBytes;
        pPacket->Op.Tcp4Rx.RxData.FragmentTable [0].FragmentBuffer = &pPacket->Op.Tcp4Rx.Buffer [0];
        pTcp4->pReceivePending = pPacket;

        //
        //  Start the receive on the packet
        //
        pTcp4Protocol = pTcp4->pProtocol;
        Status = pTcp4Protocol->Receive ( pTcp4Protocol,
                                          &pTcp4->RxToken );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_RX | DEBUG_INFO,
                    "0x%08x: Packet receive pending on port 0x%08x\r\n",
                    pPacket,
                    pPort ));
        }
        else {
          DEBUG (( DEBUG_RX | DEBUG_INFO,
                    "ERROR - Failed to post a receive on port 0x%08x, Status: %r\r\n",
                    pPort,
                    Status ));
          pTcp4->pReceivePending = NULL;
          if ( !EFI_ERROR ( pSocket->RxError )) {
            //
            //  Save the error status
            //
            pSocket->RxError = Status;
          }
        }
      }
    }
  }

  DBG_EXIT ( );
}


/**
  Shutdown the TCP4 service.

  This routine undoes the work performed by ::TcpInitialize4.

  @param [in] pService        DT_SERVICE structure address

**/
VOID
EFIAPI
EslTcpShutdown4 (
  IN DT_SERVICE * pService
  )
{
  DT_LAYER * pLayer;
  DT_PORT * pPort;
  DT_SERVICE * pPreviousService;

  DBG_ENTER ( );

  //
  //  Verify the socket layer synchronization
  //
  VERIFY_TPL ( TPL_SOCKETS );

  //
  //  Walk the list of ports
  //
  do {
    pPort = pService->pPortList;
    if ( NULL != pPort ) {
      //
      //  Remove the port from the port list
      //
      pService->pPortList = pPort->pLinkService;

      //
      //  Close the port
      // TODO: Fix this
      //
//      pPort->pfnClosePort ( pPort, DEBUG_LISTEN | DEBUG_CONNECTION );
    }
  } while ( NULL != pPort );

  //
  //  Remove the service from the service list
  //
  pLayer = &mEslLayer;
  pPreviousService = pLayer->pTcp4List;
  if ( pService == pPreviousService ) {
    //
    //  Remove the service from the beginning of the list
    //
    pLayer->pTcp4List = pService->pNext;
  }
  else {
    //
    //  Remove the service from the middle of the list
    //
    while ( NULL != pPreviousService ) {
      if ( pService == pPreviousService->pNext ) {
        pPreviousService->pNext = pService->pNext;
        break;
      }
    }
  }

  DBG_EXIT ( );
}


/**
  Determine if the socket is configured.


  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The port is connected
  @retval EFI_NOT_STARTED - The port is not connected

 **/
 EFI_STATUS
 EslTcpSocketIsConfigured4 (
  IN DT_SOCKET * pSocket
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Determine the socket configuration status
  //
  Status = pSocket->bConfigured ? EFI_SUCCESS : EFI_NOT_STARTED;

  //
  //  Return the port connected state.
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Buffer data for transmission over a network connection.

  This routine is called by the socket layer API to buffer
  data for transmission.  When necessary, this routine will
  start the transmit engine that performs the data transmission
  on the network connection.

  The transmit engine uses two queues, one for urgent (out-of-band)
  data and the other for normal data.  The urgent data is provided
  to TCP as soon as it is available, allowing the TCP layer to
  schedule transmission of the urgent data between packets of normal
  data.

  Transmission errors are returned during the next transmission or
  during the close operation.  Only buffering errors are returned
  during the current transmission attempt.

  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @retval EFI_SUCCESS - Socket data successfully buffered

 **/
EFI_STATUS
EslTcpTxBuffer4 (
  IN DT_SOCKET * pSocket,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength
  )
{
  BOOLEAN bUrgent;
  DT_PACKET * pPacket;
  DT_PACKET * pPreviousPacket;
  DT_PACKET ** ppPacket;
  DT_PACKET ** ppQueueHead;
  DT_PACKET ** ppQueueTail;
  DT_PORT * pPort;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_TCP4_IO_TOKEN * pToken;
  size_t * pTxBytes;
  EFI_TCP4_TRANSMIT_DATA * pTxData;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  Status = EFI_UNSUPPORTED;
  pSocket->errno = ENOTCONN;
  * pDataLength = 0;

  //
  //  Verify that the socket is connected
  //
  if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
    //
    //  Locate the port
    //
    pPort = pSocket->pPortList;
    if ( NULL != pPort ) {
      //
      //  Determine the queue head
      //
      pTcp4 = &pPort->Context.Tcp4;
      bUrgent = (BOOLEAN)( 0 != ( Flags & MSG_OOB ));
      if ( bUrgent ) {
        ppQueueHead = &pSocket->pTxOobPacketListHead;
        ppQueueTail = &pSocket->pTxOobPacketListTail;
        ppPacket = &pTcp4->pTxOobPacket;
        pToken = &pTcp4->TxOobToken;
        pTxBytes = &pSocket->TxOobBytes;
      }
      else {
        ppQueueHead = &pSocket->pTxPacketListHead;
        ppQueueTail = &pSocket->pTxPacketListTail;
        ppPacket = &pTcp4->pTxPacket;
        pToken = &pTcp4->TxToken;
        pTxBytes = &pSocket->TxBytes;
      }

      //
      //  Verify that there is enough room to buffer another
      //  transmit operation
      //
      if ( pSocket->MaxTxBuf > *pTxBytes ) {
        //
        //  Attempt to allocate the packet
        //
        Status = EslSocketPacketAllocate ( &pPacket,
                                           sizeof ( pPacket->Op.Tcp4Tx )
                                           - sizeof ( pPacket->Op.Tcp4Tx.Buffer )
                                           + BufferLength,
                                           DEBUG_TX );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Initialize the transmit operation
          //
          pTxData = &pPacket->Op.Tcp4Tx.TxData;
          pTxData->Push = TRUE;
          pTxData->Urgent = bUrgent;
          pTxData->DataLength = (UINT32) BufferLength;
          pTxData->FragmentCount = 1;
          pTxData->FragmentTable[0].FragmentLength = (UINT32) BufferLength;
          pTxData->FragmentTable[0].FragmentBuffer = &pPacket->Op.Tcp4Tx.Buffer[0];

          //
          //  Copy the data into the buffer
          //
          CopyMem ( &pPacket->Op.Tcp4Tx.Buffer[0],
                    pBuffer,
                    BufferLength );

          //
          //  Synchronize with the socket layer
          //
          RAISE_TPL ( TplPrevious, TPL_SOCKETS );

          //
          //  Stop transmission after an error
          //
          if ( !EFI_ERROR ( pSocket->TxError )) {
            //
            //  Display the request
            //
            DEBUG (( DEBUG_TX,
                      "Send %d %s bytes from 0x%08x\r\n",
                      BufferLength,
                      bUrgent ? L"urgent" : L"normal",
                      pBuffer ));

            //
            //  Queue the data for transmission
            //
            pPacket->pNext = NULL;
            pPreviousPacket = *ppQueueTail;
            if ( NULL == pPreviousPacket ) {
              *ppQueueHead = pPacket;
            }
            else {
              pPreviousPacket->pNext = pPacket;
            }
            *ppQueueTail = pPacket;
            DEBUG (( DEBUG_TX,
                      "0x%08x: Packet on %s transmit list\r\n",
                      pPacket,
                      bUrgent ? L"urgent" : L"normal" ));

            //
            //  Account for the buffered data
            //
            *pTxBytes += BufferLength;
            *pDataLength = BufferLength;

            //
            //  Start the transmit engine if it is idle
            //
            if ( NULL == *ppPacket ) {
              EslTcpTxStart4 ( pSocket->pPortList,
                               pToken,
                               ppQueueHead,
                               ppQueueTail,
                               ppPacket );
            }
          }
          else {
            //
            //  Previous transmit error
            //  Stop transmission
            //
            Status = pSocket->TxError;
            pSocket->errno = EIO;

            //
            //  Free the packet
            //
            EslSocketPacketFree ( pPacket, DEBUG_TX );
          }

          //
          //  Release the socket layer synchronization
          //
          RESTORE_TPL ( TplPrevious );
        }
        else {
          //
          //  Packet allocation failed
          //
          pSocket->errno = ENOMEM;
        }
      }
      else {
        //
        //  Not enough buffer space available
        //
        pSocket->errno = EAGAIN;
        Status = EFI_NOT_READY;
      }
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the normal data transmit completion

  @param  Event         The normal transmit completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpTxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  UINT32 LengthInBytes;
  DT_PACKET * pCurrentPacket;
  DT_PACKET * pNextPacket;
  DT_PACKET * pPacket;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Locate the active transmit packet
  //
  pSocket = pPort->pSocket;
  pTcp4 = &pPort->Context.Tcp4;
  pPacket = pTcp4->pTxPacket;
  
  //
  //  Mark this packet as complete
  //
  pTcp4->pTxPacket = NULL;
  LengthInBytes = pPacket->Op.Tcp4Tx.TxData.DataLength;
  pSocket->TxBytes -= LengthInBytes;
  
  //
  //  Save any transmit error
  //
  Status = pTcp4->TxToken.CompletionToken.Status;
  if ( EFI_ERROR ( Status )) {
    if ( !EFI_ERROR ( pSocket->TxError )) {
      pSocket->TxError = Status;
    }
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "ERROR - Transmit failure for packet 0x%08x, Status: %r\r\n",
              pPacket,
              Status ));

    //
    //  Empty the normal transmit list
    //
    pCurrentPacket = pPacket;
    pNextPacket = pSocket->pTxPacketListHead;
    while ( NULL != pNextPacket ) {
      pPacket = pNextPacket;
      pNextPacket = pPacket->pNext;
      EslSocketPacketFree ( pPacket, DEBUG_TX );
    }
    pSocket->pTxPacketListHead = NULL;
    pSocket->pTxPacketListTail = NULL;
    pPacket = pCurrentPacket;
  }
  else
  {
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "0x%08x: Packet transmitted %d bytes successfully\r\n",
              pPacket,
              LengthInBytes ));

    //
    //  Verify the transmit engine is still running
    //
    if ( !pPort->bCloseNow ) {
      //
      //  Start the next packet transmission
      //
      EslTcpTxStart4 ( pPort,
                       &pTcp4->TxToken,
                       &pSocket->pTxPacketListHead,
                       &pSocket->pTxPacketListTail,
                       &pTcp4->pTxPacket );
    }
  }

  //
  //  Release this packet
  //
  EslSocketPacketFree ( pPacket, DEBUG_TX );

  //
  //  Finish the close operation if necessary
  //
  if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
    //
    //  Indicate that the transmit is complete
    //
    EslTcpPortCloseTxDone4 ( pPort );
  }
  DBG_EXIT ( );
}


/**
  Process the urgent data transmit completion

  @param  Event         The urgent transmit completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpTxOobComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  )
{
  UINT32 LengthInBytes;
  DT_PACKET * pCurrentPacket;
  DT_PACKET * pNextPacket;
  DT_PACKET * pPacket;
  DT_SOCKET * pSocket;
  DT_TCP4_CONTEXT * pTcp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Locate the active transmit packet
  //
  pSocket = pPort->pSocket;
  pTcp4 = &pPort->Context.Tcp4;
  pPacket = pTcp4->pTxOobPacket;

  //
  //  Mark this packet as complete
  //
  pTcp4->pTxOobPacket = NULL;
  LengthInBytes = pPacket->Op.Tcp4Tx.TxData.DataLength;
  pSocket->TxOobBytes -= LengthInBytes;

  //
  //  Save any transmit error
  //
  Status = pTcp4->TxOobToken.CompletionToken.Status;
  if ( EFI_ERROR ( Status )) {
    if ( !EFI_ERROR ( Status )) {
      pSocket->TxError = Status;
    }
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "ERROR - Transmit failure for urgent packet 0x%08x, Status: %r\r\n",
              pPacket,
              Status ));


    //
    //  Empty the OOB transmit list
    //
    pCurrentPacket = pPacket;
    pNextPacket = pSocket->pTxOobPacketListHead;
    while ( NULL != pNextPacket ) {
      pPacket = pNextPacket;
      pNextPacket = pPacket->pNext;
      EslSocketPacketFree ( pPacket, DEBUG_TX );
    }
    pSocket->pTxOobPacketListHead = NULL;
    pSocket->pTxOobPacketListTail = NULL;
    pPacket = pCurrentPacket;
  }
  else
  {
    DEBUG (( DEBUG_TX | DEBUG_INFO,
              "0x%08x: Urgent packet transmitted %d bytes successfully\r\n",
              pPacket,
              LengthInBytes ));

    //
    //  Verify the transmit engine is still running
    //
    if ( !pPort->bCloseNow ) {
      //
      //  Start the next packet transmission
      //
      EslTcpTxStart4 ( pPort,
                       &pTcp4->TxOobToken,
                       &pSocket->pTxOobPacketListHead,
                       &pSocket->pTxOobPacketListTail,
                       &pTcp4->pTxOobPacket );
    }
  }

  //
  //  Release this packet
  //
  EslSocketPacketFree ( pPacket, DEBUG_TX );

  //
  //  Finish the close operation if necessary
  //
  if ( PORT_STATE_CLOSE_STARTED <= pPort->State ) {
    //
    //  Indicate that the transmit is complete
    //
    EslTcpPortCloseTxDone4 ( pPort );
  }
  DBG_EXIT ( );
}


/**
  Transmit data using a network connection.


  @param [in] pPort           Address of a DT_PORT structure
  @param [in] pToken          Address of either the OOB or normal transmit token
  @param [in] ppQueueHead     Transmit queue head address
  @param [in] ppQueueTail     Transmit queue tail address
  @param [in] ppPacket        Active transmit packet address

 **/
VOID
EslTcpTxStart4 (
  IN DT_PORT * pPort,
  IN EFI_TCP4_IO_TOKEN * pToken,
  IN DT_PACKET ** ppQueueHead,
  IN DT_PACKET ** ppQueueTail,
  IN DT_PACKET ** ppPacket
  )
{
  DT_PACKET * pNextPacket;
  DT_PACKET * pPacket;
  DT_SOCKET * pSocket;
  EFI_TCP4_PROTOCOL * pTcp4Protocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Get the packet from the queue head
  //
  pPacket = *ppQueueHead;
  if ( NULL != pPacket ) {
    //
    //  Remove the packet from the queue
    //
    pNextPacket = pPacket->pNext;
    *ppQueueHead = pNextPacket;
    if ( NULL == pNextPacket ) {
      *ppQueueTail = NULL;
    }

    //
    //  Set the packet as active
    //
    *ppPacket = pPacket;

    //
    //  Start the transmit operation
    //
    pTcp4Protocol = pPort->Context.Tcp4.pProtocol;
    pToken->Packet.TxData = &pPacket->Op.Tcp4Tx.TxData;
    Status = pTcp4Protocol->Transmit ( pTcp4Protocol, pToken );
    if ( EFI_ERROR ( Status )) {
      pSocket = pPort->pSocket;
      if ( EFI_SUCCESS == pSocket->TxError ) {
        pSocket->TxError = Status;
      }
    }
  }

  DBG_EXIT ( );
}
