/** @file
  Implement the socket support for the socket layer.

  Socket States:
  * Bound - pSocket->PortList is not NULL
  * Listen - AcceptWait event is not NULL

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
  Socket driver connection points

  List the network stack connection points for the socket driver.
**/
CONST DT_SOCKET_BINDING cEslSocketBinding [] = {
  { L"Tcp4",
    &gEfiTcp4ServiceBindingProtocolGuid,
    &mEslTcp4ServiceGuid,
    EslTcpInitialize4,
    EslTcpShutdown4 },
  { L"Udp4",
    &gEfiUdp4ServiceBindingProtocolGuid,
    &mEslUdp4ServiceGuid,
    EslUdpInitialize4,
    EslUdpShutdown4 }
};

CONST UINTN cEslSocketBindingEntries = DIM ( cEslSocketBinding );

DT_LAYER mEslLayer;


/**
  Initialize an endpoint for network communication.

  The ::Socket routine initializes the communication endpoint by providing
  the support for the socket library function ::socket.  The
  <a href="http://www.linuxhowtos.org/manpages/2/socket.htm">Linux</a>,
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html">POSIX</a>
  and <a href="http://msdn.microsoft.com/en-us/library/ms740506(v=VS.85).aspx">Windows</a>
  documentation for the socket routine are available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] domain    Select the family of protocols for the client or server
                        application.

  @param [in] type      Specifies how to make the network connection.  The following values
                        are supported:
                        <ul>
                          <li>
                            SOCK_STREAM - Connect to TCP, provides a byte stream
                            that is manipluated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_SEQPACKET - Connect to TCP, provides sequenced packet stream
                            that is manipulated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_DGRAM - Connect to UDP, provides a datagram service that is
                            manipulated by recvfrom and sendto.
                          </li>
                        </ul>

  @param [in] protocol  Specifies the lower layer protocol to use.  The following
                        values are supported:
                        <ul>
                          <li>IPPROTO_TCP</li> - This value must be combined with SOCK_STREAM.</li>
                          <li>IPPROTO_UDP</li> - This value must be combined with SOCK_DGRAM.</li>
                        </ul>

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval EFI_INVALID_PARAMETER - Invalid domain value, errno = EAFNOSUPPORT
  @retval EFI_INVALID_PARAMETER - Invalid type value, errno = EINVAL
  @retval EFI_INVALID_PARAMETER - Invalid protocol value, errno = EINVAL

 **/
EFI_STATUS
EslSocket (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int domain,
  IN int type,
  IN int protocol,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  int errno;

  DBG_ENTER ( );

  //
  //  Locate the socket
  //
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

  //
  //  Set the default domain if necessary
  //
  if ( AF_UNSPEC == domain ) {
    domain = AF_INET;
  }

  //
  //  Assume success
  //
  errno = 0;
  Status = EFI_SUCCESS;

  //
  //  Use break instead of goto
  //
  for ( ; ; ) {
    //
    //  Validate the domain value
    //
    if (( AF_INET != domain )
      && ( AF_LOCAL != domain ))
    {
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - Invalid domain value" ));
      Status = EFI_INVALID_PARAMETER;
      errno = EAFNOSUPPORT;
      break;
    }

    //
    //  Set the default type if necessary
    //
    if ( 0 == type ) {
      type = SOCK_STREAM;
    }

    //
    //  Validate the type value
    //
    if (( SOCK_STREAM == type )
      || ( SOCK_SEQPACKET == type )) {
      //
      //  Set the default protocol if necessary
      //
      if ( 0 == protocol ) {
        protocol = IPPROTO_TCP;
      }
    }
    else if ( SOCK_DGRAM == type ) {
      //
      //  Set the default protocol if necessary
      //
      if ( 0 == protocol ) {
        protocol = IPPROTO_UDP;
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - Invalid type value" ));
      Status = EFI_INVALID_PARAMETER;
      errno = EINVAL;
      break;
    }

    //
    //  Validate the protocol value
    //
    if (( IPPROTO_TCP != protocol )
      && ( IPPROTO_UDP != protocol )) {
      DEBUG (( DEBUG_ERROR | DEBUG_SOCKET,
                "ERROR - Invalid protocol value" ));
      Status = EFI_INVALID_PARAMETER;
      errno = EINVAL;
      break;
    }

    //
    //  Save the socket attributes
    //
    pSocket->Domain = domain;
    pSocket->Type = type;
    pSocket->Protocol = protocol;

    //
    //  Done
    //
    break;
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Accept a network connection.

  The SocketAccept routine waits for a network connection to the socket.
  It is able to return the remote network address to the caller if
  requested.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] pSockAddr       Address of a buffer to receive the remote
                              network address.

  @param [in, out] pSockAddrLength  Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.

  @param [out] ppSocketProtocol Address of a buffer to receive the socket protocol
                                instance associated with the new socket.

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS   New connection successfully created
  @retval EFI_NOT_READY No connection is available

 **/
EFI_STATUS
EslSocketAccept (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN struct sockaddr * pSockAddr,
  IN OUT socklen_t * pSockAddrLength,
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol,
  IN int * pErrno
  )
{
  DT_SOCKET * pNewSocket;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  pNewSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the sockaddr
    //
    if (( NULL != pSockAddr )
      && ( NULL == pSockAddrLength )) {
      DEBUG (( DEBUG_ACCEPT,
                "ERROR - pSockAddr is NULL!\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EFAULT;
    }
    else {
      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Verify that the socket is in the listen state
      //
      if ( SOCKET_STATE_LISTENING != pSocket->State ) {
        DEBUG (( DEBUG_ACCEPT,
                  "ERROR - Socket is not listening!\r\n" ));
        Status = EFI_NOT_STARTED;
        pSocket->errno = EOPNOTSUPP;
      }
      else {
        //
        //  Determine if a socket is available
        //
        if ( 0 == pSocket->FifoDepth ) {
          //
          //  No connections available
          //  Determine if any ports are available
          //
          if ( NULL == pSocket->pPortList ) {
            //
            //  No ports available
            //
            Status = EFI_DEVICE_ERROR;
            pSocket->errno = EINVAL;

            //
            //  Update the socket state
            //
            pSocket->State = SOCKET_STATE_NO_PORTS;
          }
          else {
            //
            //  Ports are available
            //  No connection requests at this time
            //
            Status = EFI_NOT_READY;
            pSocket->errno = EAGAIN;
          }
        }
        else {
  
          //
          //  Get the remote network address
          //
          pNewSocket = pSocket->pFifoHead;
          ASSERT ( NULL != pNewSocket );
          switch ( pSocket->Domain ) {
          default:
            DEBUG (( DEBUG_ACCEPT,
                      "ERROR - Invalid socket address family: %d\r\n",
                      pSocket->Domain ));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case AF_INET:
            //
            //  Determine the connection point within the network stack
            //
            switch ( pSocket->Type ) {
            default:
              DEBUG (( DEBUG_ACCEPT,
                        "ERROR - Invalid socket type: %d\r\n",
                        pSocket->Type));
              Status = EFI_INVALID_PARAMETER;
              pSocket->errno = EADDRNOTAVAIL;
              break;

            case SOCK_STREAM:
            case SOCK_SEQPACKET:
              Status = EslTcpAccept4 ( pNewSocket,
                                       pSockAddr,
                                       pSockAddrLength );
              break;

  /*
            case SOCK_DGRAM:
              Status = UdpAccept4 ( pSocket );
              break;
  */
            }
            break;
          }
          if ( !EFI_ERROR ( Status )) {
            //
            //  Remove the new socket from the list
            //
            pSocket->pFifoHead = pNewSocket->pNextConnection;
            if ( NULL == pSocket->pFifoHead ) {
              pSocket->pFifoTail = NULL;
            }

            //
            //  Account for this socket
            //
            pSocket->FifoDepth -= 1;

            //
            //  Update the new socket's state
            //
            pNewSocket->State = SOCKET_STATE_CONNECTED;
            pNewSocket->bConfigured = TRUE;
            DEBUG (( DEBUG_ACCEPT,
                      "0x%08x: Socket connected\r\n",
                      pNewSocket ));
          }
        }
      }

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );
    }
  }

  //
  //  Return the new socket
  //
  if (( NULL != ppSocketProtocol )
    && ( NULL != pNewSocket )) {
    *ppSocketProtocol = &pNewSocket->SocketProtocol;
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Allocate and initialize a DT_SOCKET structure.
  
  The ::SocketAllocate() function allocates a DT_SOCKET structure
  and installs a protocol on ChildHandle.  If pChildHandle is a
  pointer to NULL, then a new handle is created and returned in
  pChildHandle.  If pChildHandle is not a pointer to NULL, then
  the protocol installs on the existing pChildHandle.

  @param [in, out] pChildHandle Pointer to the handle of the child to create.
                                If it is NULL, then a new handle is created.
                                If it is a pointer to an existing UEFI handle, 
                                then the protocol is added to the existing UEFI
                                handle.
  @param [in] DebugFlags        Flags for debug messages
  @param [in, out] ppSocket     The buffer to receive the DT_SOCKET structure address.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created
  
**/
EFI_STATUS
EFIAPI
EslSocketAllocate (
  IN OUT EFI_HANDLE * pChildHandle,
  IN     UINTN DebugFlags,
  IN OUT DT_SOCKET ** ppSocket
  )
{
  UINTN LengthInBytes;
  DT_LAYER * pLayer;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Create a socket structure
  //
  LengthInBytes = sizeof ( *pSocket );
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  LengthInBytes,
                  (VOID **) &pSocket
                  );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pSocket, %d bytes\r\n",
              pSocket,
              LengthInBytes ));

    //
    //  Initialize the socket protocol
    //
    ZeroMem ( pSocket, LengthInBytes );

    pSocket->Signature = SOCKET_SIGNATURE;
    pSocket->SocketProtocol.pfnAccept = EslSocketAccept;
    pSocket->SocketProtocol.pfnBind = EslSocketBind;
    pSocket->SocketProtocol.pfnClosePoll = EslSocketClosePoll;
    pSocket->SocketProtocol.pfnCloseStart = EslSocketCloseStart;
    pSocket->SocketProtocol.pfnConnect = EslSocketConnect;
    pSocket->SocketProtocol.pfnGetLocal = EslSocketGetLocalAddress;
    pSocket->SocketProtocol.pfnGetPeer = EslSocketGetPeerAddress;
    pSocket->SocketProtocol.pfnListen = EslSocketListen;
    pSocket->SocketProtocol.pfnOptionGet = EslSocketOptionGet;
    pSocket->SocketProtocol.pfnOptionSet = EslSocketOptionSet;
    pSocket->SocketProtocol.pfnPoll = EslSocketPoll;
    pSocket->SocketProtocol.pfnReceive = EslSocketReceive;
    pSocket->SocketProtocol.pfnSend = EslSocketTransmit;
    pSocket->SocketProtocol.pfnShutdown = EslSocketShutdown;
    pSocket->SocketProtocol.pfnSocket = EslSocket;

    pSocket->MaxRxBuf = MAX_RX_DATA;
    pSocket->MaxTxBuf = MAX_TX_DATA;

    //
    //  Install the socket protocol on the specified handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    pChildHandle,
                    &gEfiSocketProtocolGuid,
                    &pSocket->SocketProtocol,
                    NULL
                    );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                "Installed: gEfiSocketProtocolGuid on   0x%08x\r\n",
                *pChildHandle ));
      pSocket->SocketProtocol.SocketHandle = *pChildHandle;

      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Add this socket to the list
      //
      pLayer = &mEslLayer;
      pSocket->pNext = pLayer->pSocketList;
      pLayer->pSocketList = pSocket;

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );

      //
      //  Return the socket structure address
      //
      *ppSocket = pSocket;
    }
    else {
      DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL | DEBUG_INIT,
                "ERROR - Failed to install gEfiSocketProtocolGuid on 0x%08x, Status: %r\r\n",
                *pChildHandle,
                Status ));
    }

    //
    //  Release the socket if necessary
    //
    if ( EFI_ERROR ( Status )) {
      gBS->FreePool ( pSocket );
      DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
                "0x%08x: Free pSocket, %d bytes\r\n",
                pSocket,
                sizeof ( *pSocket )));
      pSocket = NULL;
    }
  }
  else {
    DEBUG (( DEBUG_ERROR | DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "ERROR - Failed socket allocation, Status: %r\r\n",
              Status ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Bind a name to a socket.

  The ::SocketBind routine connects a name to a socket on the local machine.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html">POSIX</a>
  documentation for the bind routine is available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.

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

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslSocketBind (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  OUT int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the structure pointer
    //
    if ( NULL == pSockAddr ) {
      DEBUG (( DEBUG_BIND,
                "ERROR - pSockAddr is NULL!\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EFAULT;
    }
    else{
      //
      //  Validate the name length
      //
      if (( SockAddrLength < ( sizeof ( struct sockaddr ) - sizeof ( pSockAddr->sa_data )))
        || ( pSockAddr->sa_len < ( sizeof ( struct sockaddr ) - sizeof ( pSockAddr->sa_data )))) {
        DEBUG (( DEBUG_BIND,
                  "ERROR - Invalid bind name length: %d, sa_len: %d\r\n",
                  SockAddrLength,
                  pSockAddr->sa_len ));
        Status = EFI_INVALID_PARAMETER;
        pSocket->errno = EINVAL;
      }
      else {
        //
        //  Set the socket address length
        //
        if ( SockAddrLength > pSockAddr->sa_len ) {
          SockAddrLength = pSockAddr->sa_len;
        }

        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Validate the local address
        //
        switch ( pSockAddr->sa_family ) {
        default:
          DEBUG (( DEBUG_BIND,
                    "ERROR - Invalid bind address family: %d\r\n",
                    pSockAddr->sa_family ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_BIND,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            Status = EslTcpBind4 ( pSocket,
                                   pSockAddr,
                                   SockAddrLength );
            break;

          case SOCK_DGRAM:
            Status = EslUdpBind4 ( pSocket,
                                   pSockAddr,
                                   SockAddrLength );
            break;
          }
          break;
        }

        //
        //  Mark this socket as bound if successful
        //
        if ( !EFI_ERROR ( Status )) {
          pSocket->State = SOCKET_STATE_BOUND;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Determine if the socket is closed

  Reverses the operations of the ::SocketAllocate() routine.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
EFI_STATUS
EslSocketClosePoll (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  )
{
  int errno;
  DT_LAYER * pLayer;
  DT_SOCKET * pNextSocket;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  errno = 0;
  Status = EFI_SUCCESS;

  //
  //  Synchronize with the socket layer
  //
  RAISE_TPL ( TplPrevious, TPL_SOCKETS );

  //
  //  Locate the socket
  //
  pLayer = &mEslLayer;
  pNextSocket = pLayer->pSocketList;
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  while ( NULL != pNextSocket ) {
    if ( pNextSocket == pSocket ) {
      //
      //  Determine if the socket is in the closing state
      //
      if ( SOCKET_STATE_CLOSED == pSocket->State ) {
        //
        //  Walk the list of ports
        //
        if ( NULL == pSocket->pPortList ) {
          //
          //  All the ports are closed
          //  Close the WaitAccept event if necessary
          //
          if ( NULL != pSocket->WaitAccept ) {
            Status = gBS->CloseEvent ( pSocket->WaitAccept );
            if ( !EFI_ERROR ( Status )) {
              DEBUG (( DEBUG_SOCKET | DEBUG_CLOSE | DEBUG_POOL,
                        "0x%08x: Closed WaitAccept event\r\n",
                        pSocket->WaitAccept ));
              //
              //  Return the transmit status
              //
              Status = pSocket->TxError;
              if ( EFI_ERROR ( Status )) {
                pSocket->errno = EIO;
              }
            }
            else {
              DEBUG (( DEBUG_ERROR | DEBUG_SOCKET | DEBUG_CLOSE | DEBUG_POOL,
                        "ERROR - Failed to close the WaitAccept event, Status: %r\r\n",
                        Status ));
              ASSERT ( EFI_SUCCESS == Status );
            }
          }
        }
        else {
          //
          //  At least one port is still open
          //
          Status = EFI_NOT_READY;
          errno = EAGAIN;
        }
      }
      else {
        //
        //  SocketCloseStart was not called
        //
        Status = EFI_NOT_STARTED;
        errno = EPERM;
      }
      break;
    }

    //
    //  Set the next socket
    //
    pNextSocket = pNextSocket->pNext;
  }

  //
  //  Handle the error case where the socket was already closed
  //
  if ( NULL == pSocket ) {
    //
    //  Socket not found
    //
    Status = EFI_NOT_FOUND;
    errno = ENOTSOCK;
  }

  //
  //  Release the socket layer synchronization
  //
  RESTORE_TPL ( TplPrevious );

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Start the close operation on the socket

  Start closing the socket by closing all of the ports.  Upon
  completion, the ::SocketPoll() routine finishes closing the
  socket.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] bCloseNow       Boolean to control close behavior
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
EFI_STATUS
EslSocketCloseStart (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN BOOLEAN bCloseNow,
  IN int * pErrno
  )
{
  int errno;
  DT_PORT * pNextPort;
  DT_PORT * pPort;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  errno = 0;

  //
  //  Synchronize with the socket layer
  //
  RAISE_TPL ( TplPrevious, TPL_SOCKETS );

  //
  //  Determine if the socket is already closed
  //
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  if ( SOCKET_STATE_CLOSED > pSocket->State ) {
    //
    //  Update the socket state
    //
    pSocket->State = SOCKET_STATE_CLOSED;

    //
    //  Walk the list of ports
    //
    pPort = pSocket->pPortList;
    while ( NULL != pPort ) {
      //
      //  Start closing the ports
      //
      pNextPort = pPort->pLinkSocket;
      Status = pPort->pfnCloseStart ( pPort,
                                      bCloseNow,
                                      DEBUG_CLOSE | DEBUG_LISTEN | DEBUG_CONNECTION );
      if (( EFI_SUCCESS != Status )
        && ( EFI_NOT_READY != Status )) {
        errno = EIO;
        break;
      }

      //
      //  Set the next port
      //
      pPort = pNextPort;
    }

    //
    //  Attempt to finish closing the socket
    //
    if ( NULL == pPort ) {
      Status = EslSocketClosePoll ( pSocketProtocol, &errno );
    }
  }
  else {
    Status = EFI_ALREADY_STARTED;
    errno = EALREADY;
  }

  //
  //  Release the socket layer synchronization
  //
  RESTORE_TPL ( TplPrevious );

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Connect to a remote system via the network.

  The ::SocketConnect routine attempts to establish a connection to a
  socket on the local or remote system using the specified address.
  The POSIX
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html">connect</a>
  documentation is available online.

  There are three states associated with a connection:
  <ul>
    <li>Not connected</li>
    <li>Connection in progress</li>
    <li>Connected</li>
  </ul>
  In the "Not connected" state, calls to ::connect start the connection
  processing and update the state to "Connection in progress".  During
  the "Connection in progress" state, connect polls for connection completion
  and moves the state to "Connected" after the connection is established.
  Note that these states are only visible when the file descriptor is marked
  with O_NONBLOCK.  Also, the POLL_WRITE bit is set when the connection
  completes and may be used by poll or select as an indicator to call
  connect again.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @param [out] pErrno   Address to receive the errno value upon completion.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslSocketConnect (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  
  DEBUG (( DEBUG_CONNECT, "Entering SocketConnect\r\n" ));

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Validate the name length
    //
    if (( SockAddrLength < ( sizeof ( struct sockaddr ) - sizeof ( pSockAddr->sa_data )))
      || ( pSockAddr->sa_len < ( sizeof ( struct sockaddr ) - sizeof ( pSockAddr->sa_data )))) {
      DEBUG (( DEBUG_CONNECT,
                "ERROR - Invalid bind name length: %d, sa_len: %d\r\n",
                SockAddrLength,
                pSockAddr->sa_len ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EINVAL;
    }
    else {
      //
      //  Assume success
      //
      pSocket->errno = 0;

      //
      //  Set the socket address length
      //
      if ( SockAddrLength > pSockAddr->sa_len ) {
        SockAddrLength = pSockAddr->sa_len;
      }

      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Validate the socket state
      //
      switch ( pSocket->State ) {
      default:
        //
        //  Wrong socket state
        //
        pSocket->errno = EIO;
        Status = EFI_DEVICE_ERROR;
        break;

      case SOCKET_STATE_NOT_CONFIGURED:
      case SOCKET_STATE_BOUND:
        //
        //  Validate the local address
        //
        switch ( pSockAddr->sa_family ) {
        default:
          DEBUG (( DEBUG_CONNECT,
                    "ERROR - Invalid bind address family: %d\r\n",
                    pSockAddr->sa_family ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_CONNECT,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Start the connection processing
            //
            Status = EslTcpConnectStart4 ( pSocket,
                                           pSockAddr,
                                           SockAddrLength );

            //
            //  Set the next state if connecting
            //
            if ( EFI_NOT_READY == Status ) {
              pSocket->State = SOCKET_STATE_CONNECTING;
            }
            break;

          case SOCK_DGRAM:
            Status = EslUdpConnect4 ( pSocket,
                                      pSockAddr,
                                      SockAddrLength );
            break;
          }
          break;
        }
        break;

      case SOCKET_STATE_CONNECTING:
        //
        //  Validate the local address
        //
        switch ( pSockAddr->sa_family ) {
        default:
          DEBUG (( DEBUG_CONNECT,
                    "ERROR - Invalid bind address family: %d\r\n",
                    pSockAddr->sa_family ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_CONNECT,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Determine if the connection processing is completed
            //
            Status = EslTcpConnectPoll4 ( pSocket );

            //
            //  Set the next state if connected
            //
            if ( EFI_NOT_READY != Status ) {
              if ( !EFI_ERROR ( Status )) {
                pSocket->State = SOCKET_STATE_CONNECTED;
              }
              else {
                pSocket->State = SOCKET_STATE_BOUND;
              }
            }
            break;

          case SOCK_DGRAM:
            //
            //  Already connected
            //
            pSocket->errno = EISCONN;
            Status = EFI_ALREADY_STARTED;
            break;
          }
          break;
        }
        break;

      case SOCKET_STATE_CONNECTED:
        //
        //  Already connected
        //
        pSocket->errno = EISCONN;
        Status = EFI_ALREADY_STARTED;
        break;
      }

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      //
      //  Bad socket protocol
      //
      DEBUG (( DEBUG_ERROR | DEBUG_CONNECT,
                "ERROR - pSocketProtocol invalid!\r\n" ));
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }

  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_CONNECT, "Exiting SocketConnect, Status: %r\r\n", Status ));
  return Status;
}


/**
  Creates a child handle and installs a protocol.
  
  The CreateChild() function installs a protocol on ChildHandle. 
  If pChildHandle is a pointer to NULL, then a new handle is created and returned in pChildHandle. 
  If pChildHandle is not a pointer to NULL, then the protocol installs on the existing pChildHandle.

  @param [in] pThis        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] pChildHandle Pointer to the handle of the child to create. If it is NULL,
                           then a new handle is created. If it is a pointer to an existing UEFI handle, 
                           then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
EslSocketCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN OUT EFI_HANDLE * pChildHandle
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Create a socket structure
  //
  Status = EslSocketAllocate ( pChildHandle,
                               DEBUG_SOCKET,
                               &pSocket );

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Destroys a child handle with a protocol installed on it.
  
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol 
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the 
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param [in] pThis       Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is not a valid UEFI Handle.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
EslSocketDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE ChildHandle
  )
{
  DT_LAYER * pLayer;
  DT_SOCKET * pSocket;
  DT_SOCKET * pSocketPrevious;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Locate the socket control structure
  //
  pLayer = &mEslLayer;
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiSocketProtocolGuid,
                  (VOID **)&pSocketProtocol,
                  pLayer->ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if ( !EFI_ERROR ( Status )) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Synchronize with the socket layer
    //
    RAISE_TPL ( TplPrevious, TPL_SOCKETS );

    //
    //  Walk the socket list
    //
    pSocketPrevious = pLayer->pSocketList;
    if ( NULL != pSocketPrevious ) {
      if ( pSocket == pSocketPrevious ) {
        //
        //  Remove the socket from the head of the list
        //
        pLayer->pSocketList = pSocket->pNext;
      }
      else {
        //
        //  Find the socket in the middle of the list
        //
        while (( NULL != pSocketPrevious )
          && ( pSocket != pSocketPrevious->pNext )) {
          //
          //  Set the next socket
          //
          pSocketPrevious = pSocketPrevious->pNext;
        }
        if ( NULL != pSocketPrevious ) {
          //
          //  Remove the socket from the middle of the list
          //
          pSocketPrevious = pSocket->pNext;
        }
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                "ERROR - Socket list is empty!\r\n" ));
    }

    //
    //  Release the socket layer synchronization
    //
    RESTORE_TPL ( TplPrevious );

    //
    //  Determine if the socket was found
    //
    if ( NULL != pSocketPrevious ) {
      pSocket->pNext = NULL;

      //
      //  Remove the socket protocol
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                ChildHandle,
                &gEfiSocketProtocolGuid,
                &pSocket->SocketProtocol,
                NULL );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL | DEBUG_INFO,
                    "Removed:   gEfiSocketProtocolGuid from 0x%08x\r\n",
                    ChildHandle ));

        //
        //  Free the socket structure
        //
        Status = gBS->FreePool ( pSocket );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_POOL,
                    "0x%08x: Free pSocket, %d bytes\r\n",
                    pSocket,
                    sizeof ( *pSocket )));
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                    "ERROR - Failed to free pSocket 0x%08x, Status: %r\r\n",
                    pSocket,
                    Status ));
        }
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INFO,
                    "ERROR - Failed to remove gEfiSocketProtocolGuid from 0x%08x, Status: %r\r\n",
                    ChildHandle,
                    Status ));
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                "ERROR - The socket was not in the socket list!\r\n" ));
      Status = EFI_NOT_FOUND;
    }
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Failed to open socket protocol on 0x%08x, Status; %r\r\n",
              ChildHandle,
              Status ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Get the local address.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [out] pAddress       Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Local address successfully returned

 **/
EFI_STATUS
EslSocketGetLocalAddress (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  
  DBG_ENTER ( );
  
  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  
  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the address buffer and length address
    //
    if (( NULL != pAddress ) && ( NULL != pAddressLength )) {
      //
      //  Verify the socket state
      //
      if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_RX,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Get the local address
            //
            Status = EslTcpGetLocalAddress4 ( pSocket,
                                              pAddress,
                                              pAddressLength );
            break;

          case SOCK_DGRAM:
            //
            //  Get the local address
            //
            Status = EslUdpGetLocalAddress4 ( pSocket,
                                              pAddress,
                                              pAddressLength );
            break;
          }
          break;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
      else {
        pSocket->errno = ENOTCONN;
        Status = EFI_NOT_STARTED;
      }
    }
    else {
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }
  }
  
  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Get the peer address.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Remote address successfully returned

 **/
EFI_STATUS
EslSocketGetPeerAddress (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  
  DBG_ENTER ( );
  
  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  
  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify the address buffer and length address
    //
    if (( NULL != pAddress ) && ( NULL != pAddressLength )) {
      //
      //  Verify the socket state
      //
      if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_RX,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Verify the port state
            //
            Status = EslTcpGetRemoteAddress4 ( pSocket,
                                               pAddress,
                                               pAddressLength );
            break;

          case SOCK_DGRAM:
            //
            //  Verify the port state
            //
            Status = EslUdpGetRemoteAddress4 ( pSocket,
                                               pAddress,
                                               pAddressLength );
            break;
          }
          break;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
      else {
        pSocket->errno = ENOTCONN;
        Status = EFI_NOT_STARTED;
      }
    }
    else {
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }
  }
  
  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Establish the known port to listen for network connections.

  The ::SocketListen routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::SocketAccept routine to remove
  the next connection from the queue and get the associated socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation for the listen routine is available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] Backlog         Backlog specifies the maximum FIFO depth for
                              the connections waiting for the application
                              to call accept.  Connection attempts received
                              while the queue is full are refused.

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen

**/
EFI_STATUS
EslSocketListen (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Backlog,
  OUT int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_STATUS TempStatus;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Assume success
    //
    pSocket->Status = EFI_SUCCESS;
    pSocket->errno = 0;

    //
    //  Verify that the bind operation was successful
    //
    if ( SOCKET_STATE_BOUND == pSocket->State ) {
      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Create the event for SocketAccept completion
      //
      Status = gBS->CreateEvent ( 0,
                                  TplPrevious,
                                  NULL,
                                  NULL,
                                  &pSocket->WaitAccept );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL,
                  "0x%08x: Created WaitAccept event\r\n",
                  pSocket->WaitAccept ));
        //
        //  Set the maximum FIFO depth
        //
        if ( 0 >= Backlog ) {
          Backlog = MAX_PENDING_CONNECTIONS;
        }
        else {
          if ( SOMAXCONN < Backlog ) {
            Backlog = SOMAXCONN;
          }
          else {
            pSocket->MaxFifoDepth = Backlog;
          }
        }

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_BIND,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_BIND,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            Status = EslTcpListen4 ( pSocket );
            break;

/*
          case SOCK_DGRAM:
            Status = UdpListen4 ( pSocket );
            break;
*/
          }
          break;
        }

        //
        //  Place the socket in the listen state if successful
        //
        if ( !EFI_ERROR ( Status )) {
          pSocket->State = SOCKET_STATE_LISTENING;
        }
        else {
          //
          //  Not waiting for SocketAccept to complete
          //
          TempStatus = gBS->CloseEvent ( pSocket->WaitAccept );
          if ( !EFI_ERROR ( TempStatus )) {
            DEBUG (( DEBUG_POOL,
                      "0x%08x: Closed WaitAccept event\r\n",
                      pSocket->WaitAccept ));
            pSocket->WaitAccept = NULL;
          }
          else {
            DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                      "ERROR - Failed to close WaitAccept event, Status: %r\r\n",
                      TempStatus ));
            ASSERT ( EFI_SUCCESS == TempStatus );
          }
        }
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_LISTEN,
                  "ERROR - Failed to create the WaitAccept event, Status: %r\r\n",
                  Status ));
        pSocket->errno = ENOMEM;
      }

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_LISTEN,
                "ERROR - Bind operation must be performed first!\r\n" ));
      pSocket->errno = EDESTADDRREQ;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Get the socket options

  Retrieve the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol   Address of the socket protocol structure.
  @param [in] level             Option protocol level
  @param [in] OptionName        Name of the option
  @param [out] pOptionValue     Buffer to receive the option value
  @param [in,out] pOptionLength Length of the buffer in bytes,
                                upon return length of the option value in bytes
  @param [out] pErrno           Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslSocketOptionGet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  OUT void * __restrict pOptionValue,
  IN OUT socklen_t * __restrict pOptionLength,
  IN int * pErrno
  )
{
  int errno;
  socklen_t LengthInBytes;
  socklen_t MaxBytes;
  UINT8 * pOptionData;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  errno = EINVAL;
  Status = EFI_INVALID_PARAMETER;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if (( NULL != pSocketProtocol )
    && ( NULL != pOptionValue )
    && ( NULL != pOptionLength )) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
    LengthInBytes = 0;
    MaxBytes = *pOptionLength;
    pOptionData = NULL;
    switch ( level ) {
    default:
      //
      //  Protocol level not supported
      //
      errno = ENOTSUP;
      Status = EFI_UNSUPPORTED;
      break;

    case SOL_SOCKET:
      switch ( OptionName ) {
      default:
        //
        //  Option not supported
        //
        errno = ENOTSUP;
        Status = EFI_UNSUPPORTED;
        break;

      case SO_RCVTIMEO:
        //
        //  Return the receive timeout
        //
        pOptionData = (UINT8 *)&pSocket->RxTimeout;
        LengthInBytes = sizeof ( pSocket->RxTimeout );
        break;
        
      case SO_RCVBUF:
        //
        //  Return the maximum transmit buffer size
        //
        pOptionData = (UINT8 *)&pSocket->MaxRxBuf;
        LengthInBytes = sizeof ( pSocket->MaxRxBuf );
        break;

      case SO_SNDBUF:
        //
        //  Return the maximum transmit buffer size
        //
        pOptionData = (UINT8 *)&pSocket->MaxTxBuf;
        LengthInBytes = sizeof ( pSocket->MaxTxBuf );
        break;

      case SO_TYPE:
        //
        //  Return the socket type
        //
        pOptionData = (UINT8 *)&pSocket->Type;
        LengthInBytes = sizeof ( pSocket->Type );
        break;
      }
      break;
    }

    //
    //  Return the option length
    //
    *pOptionLength = LengthInBytes;

    //
    //  Return the option value
    //
    if ( NULL != pOptionData ) {
      //
      //  Silently truncate the value length
      //
      if ( LengthInBytes > MaxBytes ) {
        LengthInBytes = MaxBytes;
      }
      CopyMem ( pOptionValue, pOptionData, LengthInBytes );
      errno = 0;
      Status = EFI_SUCCESS;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Set the socket options

  Adjust the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] level           Option protocol level
  @param [in] OptionName      Name of the option
  @param [in] pOptionValue    Buffer containing the option value
  @param [in] OptionLength    Length of the buffer in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslSocketOptionSet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  IN CONST void * pOptionValue,
  IN socklen_t OptionLength,
  IN int * pErrno
  )
{
  int errno;
  socklen_t LengthInBytes;
  UINT8 * pOptionData;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Assume failure
  //
  errno = EINVAL;
  Status = EFI_INVALID_PARAMETER;
  
  //
  //  Validate the socket
  //
  pSocket = NULL;
  if (( NULL != pSocketProtocol )
    && ( NULL != pOptionValue )) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
    LengthInBytes = 0;
    pOptionData = NULL;
    switch ( level ) {
    default:
      //
      //  Protocol level not supported
      //
      errno = ENOTSUP;
      Status = EFI_UNSUPPORTED;
      break;
  
    case SOL_SOCKET:
      switch ( OptionName ) {
      default:
        //
        //  Option not supported
        //
        errno = ENOTSUP;
        Status = EFI_UNSUPPORTED;
        break;
  
      case SO_RCVTIMEO:
        //
        //  Return the receive timeout
        //
        pOptionData = (UINT8 *)&pSocket->RxTimeout;
        LengthInBytes = sizeof ( pSocket->RxTimeout );
        break;

      case SO_RCVBUF:
        //
        //  Return the maximum transmit buffer size
        //
        pOptionData = (UINT8 *)&pSocket->MaxRxBuf;
        LengthInBytes = sizeof ( pSocket->MaxRxBuf );
        break;

      case SO_SNDBUF:
        //
        //  Send buffer size
        //
        //
        //  Return the maximum transmit buffer size
        //
        pOptionData = (UINT8 *)&pSocket->MaxTxBuf;
        LengthInBytes = sizeof ( pSocket->MaxTxBuf );
        break;
      }
      break;
    }

    //
    //  Validate the option length
    //
    if ( LengthInBytes <= OptionLength ) {
      //
      //  Set the option value
      //
      if ( NULL != pOptionData ) {
        CopyMem ( pOptionData, pOptionValue, LengthInBytes );
        errno = 0;
        Status = EFI_SUCCESS;
      }
    }
  }
  
  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    *pErrno = errno;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Allocate a packet for a receive or transmit operation

  @param [in] ppPacket      Address to receive the DT_PACKET structure
  @param [in] LengthInBytes Length of the packet structure
  @param [in] DebugFlags    Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully

 **/
EFI_STATUS
EslSocketPacketAllocate (
  IN DT_PACKET ** ppPacket,
  IN size_t LengthInBytes,
  IN UINTN DebugFlags
  )
{
  DT_PACKET * pPacket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Allocate a packet structure
  //
  LengthInBytes += sizeof ( *pPacket )
                - sizeof ( pPacket->Op );
  Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                               LengthInBytes,
                               (VOID **)&pPacket );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pPacket, %d bytes\r\n",
              pPacket,
              LengthInBytes ));
    pPacket->PacketSize = LengthInBytes;
  }
  else {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INFO,
              "ERROR - Packet allocation failed for %d bytes, Status: %r\r\n",
              LengthInBytes,
              Status ));
    pPacket = NULL;
  }

  //
  //  Return the packet
  //
  *ppPacket = pPacket;

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Free a packet used for receive or transmit operation

  @param [in] pPacket     Address of the DT_PACKET structure
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully

 **/
EFI_STATUS
EslSocketPacketFree (
  IN DT_PACKET * pPacket,
  IN UINTN DebugFlags
  )
{
  UINTN LengthInBytes;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Allocate a packet structure
  //
  LengthInBytes = pPacket->PacketSize;
  Status = gBS->FreePool ( pPacket );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DebugFlags | DEBUG_POOL,
              "0x%08x: Free pPacket, %d bytes\r\n",
              pPacket,
              LengthInBytes ));
  }
  else {
    DEBUG (( DebugFlags | DEBUG_POOL | DEBUG_INFO,
              "ERROR - Failed to free packet 0x%08x, Status: %r\r\n",
              pPacket,
              Status ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Poll a socket for pending activity.

  The SocketPoll routine checks a socket for pending activity associated
  with the event mask.  Activity is returned in the detected event buffer.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] Events    Events of interest for this socket

  @param [in] pEvents   Address to receive the detected events

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully polled
  @retval EFI_INVALID_PARAMETER - When pEvents is NULL

 **/
EFI_STATUS
EslSocketPoll (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN short Events,
  IN short * pEvents,
  IN int * pErrno
  )
{
  short DetectedEvents;
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  short ValidEvents;

  DEBUG (( DEBUG_POLL, "Entering SocketPoll\r\n" ));

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  DetectedEvents = 0;
  pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );
  pSocket->errno = 0;

  //
  //  Verify the socket state
  //
  if ( !pSocket->bConfigured ) {
    //
    //  Synchronize with the socket layer
    //
    RAISE_TPL ( TplPrevious, TPL_SOCKETS );

    //
    //  Validate the local address
    //
    switch ( pSocket->Domain ) {
    default:
      DEBUG (( DEBUG_RX,
                "ERROR - Invalid socket address family: %d\r\n",
                pSocket->Domain ));
      Status = EFI_INVALID_PARAMETER;
      pSocket->errno = EADDRNOTAVAIL;
      break;

    case AF_INET:
      //
      //  Determine the connection point within the network stack
      //
      switch ( pSocket->Type ) {
      default:
        DEBUG (( DEBUG_RX,
                  "ERROR - Invalid socket type: %d\r\n",
                  pSocket->Type));
        Status = EFI_INVALID_PARAMETER;
        pSocket->errno = EADDRNOTAVAIL;
        break;

      case SOCK_STREAM:
      case SOCK_SEQPACKET:
        //
        //  Verify the port state
        //
        Status = EslTcpSocketIsConfigured4 ( pSocket );
        break;

      case SOCK_DGRAM:
        //
        //  Verify the port state
        //
        Status = EslUdpSocketIsConfigured4 ( pSocket );
        break;
      }
      break;
    }

    //
    //  Release the socket layer synchronization
    //
    RESTORE_TPL ( TplPrevious );
  }
  if ( !EFI_ERROR ( Status )) {
    //
    //  Check for invalid events
    //
    ValidEvents = POLLIN
                | POLLPRI
                | POLLOUT | POLLWRNORM
                | POLLERR
                | POLLHUP
                | POLLNVAL
                | POLLRDNORM
                | POLLRDBAND
                | POLLWRBAND ;
    if ( 0 != ( Events & ( ~ValidEvents ))) {
      DetectedEvents |= POLLNVAL;
      DEBUG (( DEBUG_INFO | DEBUG_POLL,
                "ERROR - Invalid event mask, Valid Events: 0x%04x, Invalid Events: 0x%04x\r\n",
                Events & ValidEvents,
                Events & ( ~ValidEvents )));
    }
    else {
      //
      //  Check for pending connections
      //
      if ( 0 != pSocket->FifoDepth ) {
        //
        //  A connection is waiting for an accept call
        //  See posix connect documentation at
        //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.htm
        //
        DetectedEvents |= POLLIN | POLLRDNORM;
      }
      if ( pSocket->bConnected ) {
        //
        //  A connection is present
        //  See posix connect documentation at
        //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.htm
        //
        DetectedEvents |= POLLOUT | POLLWRNORM;
      }

      //
      //  The following bits are set based upon the POSIX poll documentation at
      //  http://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html
      //

      //
      //  Check for urgent receive data
      //
      if ( 0 < pSocket->RxOobBytes ) {
        DetectedEvents |= POLLRDBAND | POLLPRI | POLLIN;
      }

      //
      //  Check for normal receive data
      //
      if (( 0 < pSocket->RxBytes )
        || ( EFI_SUCCESS != pSocket->RxError )) {
        DetectedEvents |= POLLRDNORM | POLLIN;
      }

      //
      //  Handle the receive errors
      //
      if (( EFI_SUCCESS != pSocket->RxError )
        && ( 0 == ( DetectedEvents & POLLIN ))) {
        DetectedEvents |= POLLERR | POLLIN | POLLRDNORM | POLLRDBAND;
      }

      //
      //  Check for urgent transmit data buffer space
      //
      if (( MAX_TX_DATA > pSocket->TxOobBytes )
        || ( EFI_SUCCESS != pSocket->TxError )) {
        DetectedEvents |= POLLWRBAND;
      }

      //
      //  Check for normal transmit data buffer space
      //
      if (( MAX_TX_DATA > pSocket->TxBytes )
        || ( EFI_SUCCESS != pSocket->TxError )) {
        DetectedEvents |= POLLWRNORM;
      }

      //
      //  Handle the transmit error
      //
      if ( EFI_ERROR ( pSocket->TxError )) {
        DetectedEvents |= POLLERR;
      }
    }
  }

  //
  //  Return the detected events
  //
  *pEvents = DetectedEvents & ( Events
                              | POLLERR
                              | POLLHUP
                              | POLLNVAL );

  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_POLL, "Exiting SocketPoll, Status: %r\r\n", Status ));
  return Status;
}


/**
  Receive data from a network connection.


  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslSocketReceive (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Return the transmit error if necessary
    //
    if ( EFI_SUCCESS != pSocket->TxError ) {
      pSocket->errno = EIO;
      Status = pSocket->TxError;
      pSocket->TxError = EFI_SUCCESS;
    }
    else {
      //
      //  Verify the socket state
      //
      if ( !pSocket->bConfigured ) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_RX,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Verify the port state
            //
            Status = EslTcpSocketIsConfigured4 ( pSocket );
            break;

          case SOCK_DGRAM:
            //
            //  Verify the port state
            //
            Status = EslUdpSocketIsConfigured4 ( pSocket );
            break;
          }
          break;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );

        //
        //  Set errno if a failure occurs
        //
        if ( EFI_ERROR ( Status )) {
          pSocket->errno = EADDRNOTAVAIL;
        }
      }
      if ( !EFI_ERROR ( Status )) {
        //
        //  Validate the buffer length
        //
        if (( NULL == pDataLength )
          && ( 0 > pDataLength )
          && ( NULL == pBuffer )) {
          if ( NULL == pDataLength ) {
            DEBUG (( DEBUG_RX,
                      "ERROR - pDataLength is NULL!\r\n" ));
          }
          else if ( NULL == pBuffer ) {
            DEBUG (( DEBUG_RX,
                      "ERROR - pBuffer is NULL!\r\n" ));
          }
          else {
            DEBUG (( DEBUG_RX,
                      "ERROR - Data length < 0!\r\n" ));
          }
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EFAULT;
        }
        else{
          //
          //  Synchronize with the socket layer
          //
          RAISE_TPL ( TplPrevious, TPL_SOCKETS );

          //
          //  Validate the local address
          //
          switch ( pSocket->Domain ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket address family: %d\r\n",
                      pSocket->Domain ));
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EADDRNOTAVAIL;
            break;

          case AF_INET:
            //
            //  Determine the connection point within the network stack
            //
            switch ( pSocket->Type ) {
            default:
              DEBUG (( DEBUG_RX,
                        "ERROR - Invalid socket type: %d\r\n",
                        pSocket->Type));
              Status = EFI_INVALID_PARAMETER;
              pSocket->errno = EADDRNOTAVAIL;
              break;

            case SOCK_STREAM:
            case SOCK_SEQPACKET:
              Status = EslTcpReceive4 ( pSocket,
                                        Flags,
                                        BufferLength,
                                        pBuffer,
                                        pDataLength,
                                        pAddress,
                                        pAddressLength );
              break;

            case SOCK_DGRAM:
              Status = EslUdpReceive4 ( pSocket,
                                        Flags,
                                        BufferLength,
                                        pBuffer,
                                        pDataLength,
                                        pAddress,
                                        pAddressLength);
              break;
            }
            break;
          }

          //
          //  Release the socket layer synchronization
          //
          RESTORE_TPL ( TplPrevious );
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Shutdown the socket receive and transmit operations

  The SocketShutdown routine stops the socket receive and transmit
  operations.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] How             Which operations to stop
  
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket operations successfully shutdown

 **/
EFI_STATUS
EslSocketShutdown (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int How,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  
  DBG_ENTER ( );
  
  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Verify that the socket is connected
    //
    if ( pSocket->bConnected ) {
      //
      //  Validate the How value
      //
      if (( SHUT_RD <= How ) && ( SHUT_RDWR >= How )) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Disable the receiver if requested
        //
        if (( SHUT_RD == How ) || ( SHUT_RDWR == How )) {
          pSocket->bRxDisable = TRUE;
        }

        //
        //  Disable the transmitter if requested
        //
        if (( SHUT_WR == How ) || ( SHUT_RDWR == How )) {
          pSocket->bTxDisable = TRUE;
        }

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_RX,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;
        
        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            break;
        
          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Cancel the pending receive operation
            //
            Status = EslTcpRxCancel4 ( pSocket );
            break;
        
          case SOCK_DGRAM:
            //
            //  Cancel the pending receive operation
            //
            Status = EslUdpRxCancel4 ( pSocket );
            break;
          }
          break;
        }
        
        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );
      }
      else {
        //
        //  The socket is not connected
        //
        pSocket->errno = ENOTCONN;
        Status = EFI_NOT_STARTED;
      }
    }
    else {
      //
      //  Invalid How value
      //
      pSocket->errno = EINVAL;
      Status = EFI_INVALID_PARAMETER;
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Send data using a network connection.

  The SocketTransmit routine queues the data for transmission to the
  remote network connection.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer containing the data to send
  
  @param [in] pDataLength     Address to receive the number of data bytes sent

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully queued for transmit

 **/
EFI_STATUS
EslSocketTransmit (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength,
  IN int * pErrno
  )
{
  DT_SOCKET * pSocket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Validate the socket
  //
  pSocket = NULL;
  if ( NULL != pSocketProtocol ) {
    pSocket = SOCKET_FROM_PROTOCOL ( pSocketProtocol );

    //
    //  Return the transmit error if necessary
    //
    if ( EFI_SUCCESS != pSocket->TxError ) {
      pSocket->errno = EIO;
      Status = pSocket->TxError;
      pSocket->TxError = EFI_SUCCESS;
    }
    else {
      //
      //  Verify the socket state
      //
      if ( !pSocket->bConfigured ) {
        //
        //  Synchronize with the socket layer
        //
        RAISE_TPL ( TplPrevious, TPL_SOCKETS );

        //
        //  Validate the local address
        //
        switch ( pSocket->Domain ) {
        default:
          DEBUG (( DEBUG_RX,
                    "ERROR - Invalid socket address family: %d\r\n",
                    pSocket->Domain ));
          Status = EFI_INVALID_PARAMETER;
          pSocket->errno = EADDRNOTAVAIL;
          break;

        case AF_INET:
          //
          //  Determine the connection point within the network stack
          //
          switch ( pSocket->Type ) {
          default:
            DEBUG (( DEBUG_RX,
                      "ERROR - Invalid socket type: %d\r\n",
                      pSocket->Type));
            Status = EFI_INVALID_PARAMETER;
            break;

          case SOCK_STREAM:
          case SOCK_SEQPACKET:
            //
            //  Verify the port state
            //
            Status = EslTcpSocketIsConfigured4 ( pSocket );
            break;

          case SOCK_DGRAM:
            //
            //  Verify the port state
            //
            Status = EslUdpSocketIsConfigured4 ( pSocket );
            break;
          }
          break;
        }

        //
        //  Release the socket layer synchronization
        //
        RESTORE_TPL ( TplPrevious );

        //
        //  Set errno if a failure occurs
        //
        if ( EFI_ERROR ( Status )) {
          pSocket->errno = EADDRNOTAVAIL;
        }
      }
      if ( !EFI_ERROR ( Status )) {
        //
        //  Verify that transmit is still allowed
        //
        if ( !pSocket->bTxDisable ) {
          //
          //  Validate the buffer length
          //
          if (( NULL == pDataLength )
            && ( 0 > pDataLength )
            && ( NULL == pBuffer )) {
            if ( NULL == pDataLength ) {
              DEBUG (( DEBUG_RX,
                        "ERROR - pDataLength is NULL!\r\n" ));
            }
            else if ( NULL == pBuffer ) {
              DEBUG (( DEBUG_RX,
                        "ERROR - pBuffer is NULL!\r\n" ));
            }
            else {
              DEBUG (( DEBUG_RX,
                        "ERROR - Data length < 0!\r\n" ));
            }
            Status = EFI_INVALID_PARAMETER;
            pSocket->errno = EFAULT;
          }
          else {
            //
            //  Validate the remote network address
            //
            if (( NULL != pAddress )
              && ( AddressLength < pAddress->sa_len )) {
              DEBUG (( DEBUG_TX,
                        "ERROR - Invalid sin_len field in address\r\n" ));
              Status = EFI_INVALID_PARAMETER;
              pSocket->errno = EFAULT;
            }
            else {
              //
              //  Synchronize with the socket layer
              //
              RAISE_TPL ( TplPrevious, TPL_SOCKETS );

              //
              //  Validate the local address
              //
              switch ( pSocket->Domain ) {
              default:
                DEBUG (( DEBUG_RX,
                          "ERROR - Invalid socket address family: %d\r\n",
                          pSocket->Domain ));
                Status = EFI_INVALID_PARAMETER;
                pSocket->errno = EADDRNOTAVAIL;
                break;

              case AF_INET:
                //
                //  Determine the connection point within the network stack
                //
                switch ( pSocket->Type ) {
                default:
                  DEBUG (( DEBUG_RX,
                            "ERROR - Invalid socket type: %d\r\n",
                            pSocket->Type));
                  Status = EFI_INVALID_PARAMETER;
                  pSocket->errno = EADDRNOTAVAIL;
                  break;

                case SOCK_STREAM:
                case SOCK_SEQPACKET:
                  Status = EslTcpTxBuffer4 ( pSocket,
                                             Flags,
                                             BufferLength,
                                             pBuffer,
                                             pDataLength );
                  break;

                case SOCK_DGRAM:
                  Status = EslUdpTxBuffer4 ( pSocket,
                                             Flags,
                                             BufferLength,
                                             pBuffer,
                                             pDataLength,
                                             pAddress,
                                             AddressLength );
                  break;
                }
                break;
              }

              //
              //  Release the socket layer synchronization
              //
              RESTORE_TPL ( TplPrevious );
            }
          }
        }
        else {
          //
          //  The transmitter was shutdown
          //
          pSocket->errno = EPIPE;
          Status = EFI_NOT_STARTED;
        }
      }
    }
  }

  //
  //  Return the operation status
  //
  if ( NULL != pErrno ) {
    if ( NULL != pSocket ) {
      *pErrno = pSocket->errno;
    }
    else
    {
      Status = EFI_INVALID_PARAMETER;
      *pErrno = EBADF;
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Socket layer's service binding protocol delcaration.
**/
EFI_SERVICE_BINDING_PROTOCOL mEfiServiceBinding = {
  EslSocketCreateChild,
  EslSocketDestroyChild
};
