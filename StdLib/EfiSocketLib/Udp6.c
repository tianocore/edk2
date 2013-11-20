/** @file
  Implement the UDP4 driver support for the socket layer.

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
  Get the local socket address

  This routine returns the IPv6 address and UDP port number associated
  with the local socket.

  This routine is called by ::EslSocketGetLocalAddress to determine the
  network address for the SOCK_DGRAM socket.

  @param [in] pPort       Address of an ::ESL_PORT structure.

  @param [out] pSockAddr  Network address to receive the local system address

**/
VOID
EslUdp6LocalAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pSockAddr
  )
{
  struct sockaddr_in6 * pLocalAddress;
  ESL_UDP6_CONTEXT * pUdp6;

  DBG_ENTER ( );

  //
  //  Return the local address
  //
  pUdp6 = &pPort->Context.Udp6;
  pLocalAddress = (struct sockaddr_in6 *)pSockAddr;
  pLocalAddress->sin6_family = AF_INET6;
  pLocalAddress->sin6_port = SwapBytes16 ( pUdp6->ConfigData.StationPort );
  CopyMem ( &pLocalAddress->sin6_addr,
            &pUdp6->ConfigData.StationAddress.Addr[0],
            sizeof ( pLocalAddress->sin6_addr ));

  DBG_EXIT ( );
}


/**
  Set the local port address.

  This routine sets the local port address.

  This support routine is called by ::EslSocketPortAllocate.

  @param [in] pPort       Address of an ESL_PORT structure
  @param [in] pSockAddr   Address of a sockaddr structure that contains the
                          connection point on the local machine.  An IPv6 address
                          of INADDR_ANY specifies that the connection is made to
                          all of the network stacks on the platform.  Specifying a
                          specific IPv6 address restricts the connection to the
                          network stack supporting that address.  Specifying zero
                          for the port causes the network layer to assign a port
                          number from the dynamic range.  Specifying a specific
                          port number causes the network layer to use that port.

  @param [in] bBindTest   TRUE = run bind testing

  @retval EFI_SUCCESS     The operation was successful

 **/
EFI_STATUS
EslUdp6LocalAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN BOOLEAN bBindTest
  )
{
  EFI_UDP6_CONFIG_DATA * pConfig;
  CONST struct sockaddr_in6 * pIpAddress;
  CONST UINT8 * pIPv6Address;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Set the local address
  //
  pIpAddress = (struct sockaddr_in6 *)pSockAddr;
  pIPv6Address = (UINT8 *)&pIpAddress->sin6_addr;
  pConfig = &pPort->Context.Udp6.ConfigData;
  CopyMem ( &pConfig->StationAddress,
            pIPv6Address,
            sizeof ( pConfig->StationAddress ));

  //
  //  Validate the IP address
  //
  pConfig->StationPort = 0;
  Status = bBindTest ? EslSocketBindTest ( pPort, EADDRNOTAVAIL )
                     : EFI_SUCCESS;
  if ( !EFI_ERROR ( Status )) {
    //
    //  Set the port number
    //
    pConfig->StationPort = SwapBytes16 ( pIpAddress->sin6_port );
    pPort->pSocket->bAddressSet = TRUE;

    //
    //  Display the local address
    //
    DEBUG (( DEBUG_BIND,
              "0x%08x: Port, Local UDP6 Address: [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
              pPort,
              pConfig->StationAddress.Addr[0],
              pConfig->StationAddress.Addr[1],
              pConfig->StationAddress.Addr[2],
              pConfig->StationAddress.Addr[3],
              pConfig->StationAddress.Addr[4],
              pConfig->StationAddress.Addr[5],
              pConfig->StationAddress.Addr[6],
              pConfig->StationAddress.Addr[7],
              pConfig->StationAddress.Addr[8],
              pConfig->StationAddress.Addr[9],
              pConfig->StationAddress.Addr[10],
              pConfig->StationAddress.Addr[11],
              pConfig->StationAddress.Addr[12],
              pConfig->StationAddress.Addr[13],
              pConfig->StationAddress.Addr[14],
              pConfig->StationAddress.Addr[15],
              pConfig->StationPort ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Free a receive packet

  This routine performs the network specific operations necessary
  to free a receive packet.

  This routine is called by ::EslSocketPortCloseTxDone to free a
  receive packet.

  @param [in] pPacket         Address of an ::ESL_PACKET structure.
  @param [in, out] pRxBytes   Address of the count of RX bytes

**/
VOID
EslUdp6PacketFree (
  IN ESL_PACKET * pPacket,
  IN OUT size_t * pRxBytes
  )
{
  EFI_UDP6_RECEIVE_DATA * pRxData;

  DBG_ENTER ( );

  //
  //  Account for the receive bytes
  //
  pRxData = pPacket->Op.Udp6Rx.pRxData;
  *pRxBytes -= pRxData->DataLength;

  //
  //  Disconnect the buffer from the packet
  //
  pPacket->Op.Udp6Rx.pRxData = NULL;

  //
  //  Return the buffer to the UDP6 driver
  //
  gBS->SignalEvent ( pRxData->RecycleSignal );
  DBG_EXIT ( );
}


/**
  Initialize the network specific portions of an ::ESL_PORT structure.

  This routine initializes the network specific portions of an
  ::ESL_PORT structure for use by the socket.

  This support routine is called by ::EslSocketPortAllocate
  to connect the socket with the underlying network adapter
  running the UDPv4 protocol.

  @param [in] pPort       Address of an ESL_PORT structure
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslUdp6PortAllocate (
  IN ESL_PORT * pPort,
  IN UINTN DebugFlags
  )
{
  EFI_UDP6_CONFIG_DATA * pConfig;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Initialize the port
  //
  pSocket = pPort->pSocket;
  pSocket->TxPacketOffset = OFFSET_OF ( ESL_PACKET, Op.Udp6Tx.TxData );
  pSocket->TxTokenEventOffset = OFFSET_OF ( ESL_IO_MGMT, Token.Udp6Tx.Event );
  pSocket->TxTokenOffset = OFFSET_OF ( EFI_UDP6_COMPLETION_TOKEN, Packet.TxData );

  //
  //  Save the cancel, receive and transmit addresses
  //
  pPort->pfnConfigure = (PFN_NET_CONFIGURE)pPort->pProtocol.UDPv6->Configure;
  pPort->pfnRxCancel = (PFN_NET_IO_START)pPort->pProtocol.UDPv6->Cancel;
  pPort->pfnRxPoll = (PFN_NET_POLL)pPort->pProtocol.UDPv6->Poll;
  pPort->pfnRxStart = (PFN_NET_IO_START)pPort->pProtocol.UDPv6->Receive;
  pPort->pfnTxStart = (PFN_NET_IO_START)pPort->pProtocol.UDPv6->Transmit;

  //
  //  Do not drop packets
  //
  pConfig = &pPort->Context.Udp6.ConfigData;
  pConfig->ReceiveTimeout = 0;
  pConfig->ReceiveTimeout = pConfig->ReceiveTimeout;

  //
  //  Set the configuration flags
  //
  pConfig->AllowDuplicatePort = TRUE;
  pConfig->AcceptAnyPort = FALSE;
  pConfig->AcceptPromiscuous = FALSE;
  pConfig->HopLimit = 255;
  pConfig->TrafficClass = 0;

  Status = EFI_SUCCESS;

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Receive data from a network connection.

  This routine attempts to return buffered data to the caller.  The
  data is removed from the urgent queue if the message flag MSG_OOB
  is specified, otherwise data is removed from the normal queue.
  See the \ref ReceiveEngine section.

  This routine is called by ::EslSocketReceive to handle the network
  specific receive operation to support SOCK_DGRAM sockets.

  @param [in] pPort           Address of an ::ESL_PORT structure.

  @param [in] pPacket         Address of an ::ESL_PACKET structure.
  
  @param [in] pbConsumePacket Address of a BOOLEAN indicating if the packet is to be consumed
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [out] pSkipBytes     Address to receive the number of bytes skipped

  @return   Returns the address of the next free byte in the buffer.

 **/
UINT8 *
EslUdp6Receive (
  IN ESL_PORT * pPort,
  IN ESL_PACKET * pPacket,
  IN BOOLEAN * pbConsumePacket,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  OUT size_t * pSkipBytes
  )
{
  size_t DataBytes;
  struct sockaddr_in6 * pRemoteAddress;
  EFI_UDP6_RECEIVE_DATA * pRxData;

  DBG_ENTER ( );

  pRxData = pPacket->Op.Udp6Rx.pRxData;
  //
  //  Return the remote system address if requested
  //
  if ( NULL != pAddress ) {
    //
    //  Build the remote address
    //
    DEBUG (( DEBUG_RX,
              "Getting packet remote address: [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
              pRxData->UdpSession.SourceAddress.Addr[0],
              pRxData->UdpSession.SourceAddress.Addr[1],
              pRxData->UdpSession.SourceAddress.Addr[2],
              pRxData->UdpSession.SourceAddress.Addr[3],
              pRxData->UdpSession.SourceAddress.Addr[4],
              pRxData->UdpSession.SourceAddress.Addr[5],
              pRxData->UdpSession.SourceAddress.Addr[6],
              pRxData->UdpSession.SourceAddress.Addr[7],
              pRxData->UdpSession.SourceAddress.Addr[8],
              pRxData->UdpSession.SourceAddress.Addr[9],
              pRxData->UdpSession.SourceAddress.Addr[10],
              pRxData->UdpSession.SourceAddress.Addr[11],
              pRxData->UdpSession.SourceAddress.Addr[12],
              pRxData->UdpSession.SourceAddress.Addr[13],
              pRxData->UdpSession.SourceAddress.Addr[14],
              pRxData->UdpSession.SourceAddress.Addr[15],
              pRxData->UdpSession.SourcePort ));
    pRemoteAddress = (struct sockaddr_in6 *)pAddress;
    CopyMem ( &pRemoteAddress->sin6_addr,
              &pRxData->UdpSession.SourceAddress.Addr[0],
              sizeof ( pRemoteAddress->sin6_addr ));
    pRemoteAddress->sin6_port = SwapBytes16 ( pRxData->UdpSession.SourcePort );
  }

  //
  //  Copy the received data
  //
  pBuffer = EslSocketCopyFragmentedBuffer ( pRxData->FragmentCount,
                                            (EFI_IP4_FRAGMENT_DATA *)&pRxData->FragmentTable[0],
                                            BufferLength,
                                            pBuffer,
                                            &DataBytes );

  //
  //  Determine if the data is being read
  //
  if ( *pbConsumePacket ) {
    //
    //  Display for the bytes consumed
    //
    DEBUG (( DEBUG_RX,
              "0x%08x: Port account for 0x%08x bytes\r\n",
              pPort,
              DataBytes ));

    //
    //  Account for any discarded data
    //
    *pSkipBytes = pRxData->DataLength - DataBytes;
  }

  //
  //  Return the data length and the buffer address
  //
  *pDataLength = DataBytes;
  DBG_EXIT_HEX ( pBuffer );
  return pBuffer;
}


/**
  Get the remote socket address

  This routine returns the address of the remote connection point
  associated with the SOCK_DGRAM socket.

  This routine is called by ::EslSocketGetPeerAddress to detemine
  the UDPv4 address and port number associated with the network adapter.

  @param [in] pPort       Address of an ::ESL_PORT structure.

  @param [out] pAddress   Network address to receive the remote system address

**/
VOID
EslUdp6RemoteAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pAddress
  )
{
  struct sockaddr_in6 * pRemoteAddress;
  ESL_UDP6_CONTEXT * pUdp6;

  DBG_ENTER ( );

  //
  //  Return the remote address
  //
  pUdp6 = &pPort->Context.Udp6;
  pRemoteAddress = (struct sockaddr_in6 *)pAddress;
  pRemoteAddress->sin6_family = AF_INET6;
  pRemoteAddress->sin6_port = SwapBytes16 ( pUdp6->ConfigData.RemotePort );
  CopyMem ( &pRemoteAddress->sin6_addr,
            &pUdp6->ConfigData.RemoteAddress.Addr[0],
            sizeof ( pRemoteAddress->sin6_addr ));

  DBG_EXIT ( );
}


/**
  Set the remote address

  This routine sets the remote address in the port.

  This routine is called by ::EslSocketConnect to specify the
  remote network address.

  @param [in] pPort           Address of an ::ESL_PORT structure.

  @param [in] pSockAddr       Network address of the remote system.

  @param [in] SockAddrLength  Length in bytes of the network address.

  @retval EFI_SUCCESS     The operation was successful

 **/
EFI_STATUS
EslUdp6RemoteAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  )
{
  CONST struct sockaddr_in6 * pRemoteAddress;
  ESL_UDP6_CONTEXT * pUdp6;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Set the remote address
  //
  pUdp6 = &pPort->Context.Udp6;
  pRemoteAddress = (struct sockaddr_in6 *)pSockAddr;
  CopyMem ( &pUdp6->ConfigData.RemoteAddress,
            &pRemoteAddress->sin6_addr,
            sizeof ( pUdp6->ConfigData.RemoteAddress ));
  pUdp6->ConfigData.RemotePort = SwapBytes16 ( pRemoteAddress->sin6_port );
  Status = EFI_SUCCESS;

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the receive completion

  This routine keeps the UDPv4 driver's buffer and queues it in
  in FIFO order to the data queue.  The UDP6 driver's buffer will
  be returned by either ::EslUdp6Receive or ::EslSocketPortCloseTxDone.
  See the \ref ReceiveEngine section.

  This routine is called by the UDPv4 driver when data is
  received.

  @param [in] Event     The receive completion event

  @param [in] pIo       Address of an ::ESL_IO_MGMT structure

**/
VOID
EslUdp6RxComplete (
  IN EFI_EVENT Event,
  IN ESL_IO_MGMT * pIo
  )
{
  size_t LengthInBytes;
  ESL_PACKET * pPacket;
  EFI_UDP6_RECEIVE_DATA * pRxData;
  EFI_STATUS Status;
  
  DBG_ENTER ( );

  //
  //  Get the operation status.
  //
  Status = pIo->Token.Udp6Rx.Status;
  
  //
  //  Get the packet length
  //
  pRxData = pIo->Token.Udp6Rx.Packet.RxData;
  LengthInBytes = pRxData->DataLength;

  //
  //      +--------------------+   +-----------------------+
  //      | ESL_IO_MGMT        |   |      Data Buffer      |
  //      |                    |   |     (Driver owned)    |
  //      |    +---------------+   +-----------------------+
  //      |    | Token         |               ^
  //      |    |      Rx Event |               |
  //      |    |               |   +-----------------------+
  //      |    |        RxData --> | EFI_UDP6_RECEIVE_DATA |
  //      +----+---------------+   |     (Driver owned)    |
  //                               +-----------------------+
  //      +--------------------+               ^
  //      | ESL_PACKET         |               .
  //      |                    |               .
  //      |    +---------------+               .
  //      |    |       pRxData --> NULL  .......
  //      +----+---------------+
  //
  //
  //  Save the data in the packet
  //
  pPacket = pIo->pPacket;
  pPacket->Op.Udp6Rx.pRxData = pRxData;

  //
  //  Complete this request
  //
  EslSocketRxComplete ( pIo, Status, LengthInBytes, FALSE );
  DBG_EXIT ( );
}


/**
  Determine if the socket is configured.

  This routine uses the flag ESL_SOCKET::bConfigured to determine
  if the network layer's configuration routine has been called.
  This routine calls the bind and configuration routines if they
  were not already called.  After the port is configured, the
  \ref ReceiveEngine is started.

  This routine is called by EslSocketIsConfigured to verify
  that the socket is configured.

  @param [in] pSocket         Address of an ::ESL_SOCKET structure

  @retval EFI_SUCCESS - The port is connected
  @retval EFI_NOT_STARTED - The port is not connected

 **/
 EFI_STATUS
 EslUdp6SocketIsConfigured (
  IN ESL_SOCKET * pSocket
  )
{
  EFI_UDP6_CONFIG_DATA * pConfigData;
  ESL_PORT * pPort;
  ESL_PORT * pNextPort;
  ESL_UDP6_CONTEXT * pUdp6;
  EFI_UDP6_PROTOCOL * pUdp6Protocol;
  EFI_STATUS Status;
  struct sockaddr_in6 LocalAddress;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Configure the port if necessary
  //
  if ( !pSocket->bConfigured ) {
    //
    //  Fill in the port list if necessary
    //
    pSocket->errno = ENETDOWN;
    if ( NULL == pSocket->pPortList ) {
      ZeroMem ( &LocalAddress, sizeof ( LocalAddress ));
      LocalAddress.sin6_len = sizeof ( LocalAddress );
      LocalAddress.sin6_family = AF_INET6;
      Status = EslSocketBind ( &pSocket->SocketProtocol,
                               (struct sockaddr *)&LocalAddress,
                               LocalAddress.sin6_len,
                               &pSocket->errno );
    }

    //
    //  Walk the port list
    //
    pPort = pSocket->pPortList;
    while ( NULL != pPort ) {
      //
      //  Attempt to configure the port
      //
      pNextPort = pPort->pLinkSocket;
      pUdp6 = &pPort->Context.Udp6;
      pUdp6Protocol = pPort->pProtocol.UDPv6;
      pConfigData = &pUdp6->ConfigData;
      DEBUG (( DEBUG_TX,
                "0x%08x: pPort Configuring for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d --> ",
                pPort,
                pConfigData->StationAddress.Addr[0],
                pConfigData->StationAddress.Addr[1],
                pConfigData->StationAddress.Addr[2],
                pConfigData->StationAddress.Addr[3],
                pConfigData->StationAddress.Addr[4],
                pConfigData->StationAddress.Addr[5],
                pConfigData->StationAddress.Addr[6],
                pConfigData->StationAddress.Addr[7],
                pConfigData->StationAddress.Addr[8],
                pConfigData->StationAddress.Addr[9],
                pConfigData->StationAddress.Addr[10],
                pConfigData->StationAddress.Addr[11],
                pConfigData->StationAddress.Addr[12],
                pConfigData->StationAddress.Addr[13],
                pConfigData->StationAddress.Addr[14],
                pConfigData->StationAddress.Addr[15],
                pConfigData->StationPort ));
      DEBUG (( DEBUG_TX,
                "[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                pConfigData->RemoteAddress.Addr[0],
                pConfigData->RemoteAddress.Addr[1],
                pConfigData->RemoteAddress.Addr[2],
                pConfigData->RemoteAddress.Addr[3],
                pConfigData->RemoteAddress.Addr[4],
                pConfigData->RemoteAddress.Addr[5],
                pConfigData->RemoteAddress.Addr[6],
                pConfigData->RemoteAddress.Addr[7],
                pConfigData->RemoteAddress.Addr[8],
                pConfigData->RemoteAddress.Addr[9],
                pConfigData->RemoteAddress.Addr[10],
                pConfigData->RemoteAddress.Addr[11],
                pConfigData->RemoteAddress.Addr[12],
                pConfigData->RemoteAddress.Addr[13],
                pConfigData->RemoteAddress.Addr[14],
                pConfigData->RemoteAddress.Addr[15],
                pConfigData->RemotePort ));
      Status = pUdp6Protocol->Configure ( pUdp6Protocol,
                                          pConfigData );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Update the configuration data
        //
        Status = pUdp6Protocol->GetModeData ( pUdp6Protocol,
                                              pConfigData,
                                              NULL,
                                              NULL,
                                              NULL );
      }
      if ( EFI_ERROR ( Status )) {
        if ( !pSocket->bConfigured ) {
          DEBUG (( DEBUG_LISTEN,
                    "ERROR - Failed to configure the Udp6 port, Status: %r\r\n",
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
      }
      else {
        DEBUG (( DEBUG_TX,
                  "0x%08x: pPort Configured for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d --> ",
                  pPort,
                  pConfigData->StationAddress.Addr[0],
                  pConfigData->StationAddress.Addr[1],
                  pConfigData->StationAddress.Addr[2],
                  pConfigData->StationAddress.Addr[3],
                  pConfigData->StationAddress.Addr[4],
                  pConfigData->StationAddress.Addr[5],
                  pConfigData->StationAddress.Addr[6],
                  pConfigData->StationAddress.Addr[7],
                  pConfigData->StationAddress.Addr[8],
                  pConfigData->StationAddress.Addr[9],
                  pConfigData->StationAddress.Addr[10],
                  pConfigData->StationAddress.Addr[11],
                  pConfigData->StationAddress.Addr[12],
                  pConfigData->StationAddress.Addr[13],
                  pConfigData->StationAddress.Addr[14],
                  pConfigData->StationAddress.Addr[15],
                  pConfigData->StationPort ));
        DEBUG (( DEBUG_TX,
                  "[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                  pConfigData->RemoteAddress.Addr[0],
                  pConfigData->RemoteAddress.Addr[1],
                  pConfigData->RemoteAddress.Addr[2],
                  pConfigData->RemoteAddress.Addr[3],
                  pConfigData->RemoteAddress.Addr[4],
                  pConfigData->RemoteAddress.Addr[5],
                  pConfigData->RemoteAddress.Addr[6],
                  pConfigData->RemoteAddress.Addr[7],
                  pConfigData->RemoteAddress.Addr[8],
                  pConfigData->RemoteAddress.Addr[9],
                  pConfigData->RemoteAddress.Addr[10],
                  pConfigData->RemoteAddress.Addr[11],
                  pConfigData->RemoteAddress.Addr[12],
                  pConfigData->RemoteAddress.Addr[13],
                  pConfigData->RemoteAddress.Addr[14],
                  pConfigData->RemoteAddress.Addr[15],
                  pConfigData->RemotePort ));
        pPort->bConfigured = TRUE;
        pSocket->bConfigured = TRUE;

        //
        //  Start the first read on the port
        //
        EslSocketRxStart ( pPort );

        //
        //  The socket is connected
        //
        pSocket->State = SOCKET_STATE_CONNECTED;
        pSocket->errno = 0;
      }

      //
      //  Set the next port
      //
      pPort = pNextPort;
    }
  }

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

  This routine buffers data for the transmit engine in the normal
  data queue.  When the \ref TransmitEngine has resources, this
  routine will start the transmission of the next buffer on the
  network connection.

  This routine is called by ::EslSocketTransmit to buffer
  data for transmission.  The data is copied into a local buffer
  freeing the application buffer for reuse upon return.  When
  necessary, this routine starts the transmit engine that
  performs the data transmission on the network connection.  The
  transmit engine transmits the data a packet at a time over the
  network connection.

  Transmission errors are returned during the next transmission or
  during the close operation.  Only buffering errors are returned
  during the current transmission attempt.

  @param [in] pSocket         Address of an ::ESL_SOCKET structure

  @param [in] Flags           Message control flags

  @param [in] BufferLength    Length of the the buffer

  @param [in] pBuffer         Address of a buffer to receive the data.

  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @retval EFI_SUCCESS - Socket data successfully buffered

**/
EFI_STATUS
EslUdp6TxBuffer (
  IN ESL_SOCKET * pSocket,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength
  )
{
  ESL_PACKET * pPacket;
  ESL_PACKET * pPreviousPacket;
  ESL_PORT * pPort;
  const struct sockaddr_in6 * pRemoteAddress;
  ESL_UDP6_CONTEXT * pUdp6;
  size_t * pTxBytes;
  ESL_UDP6_TX_DATA * pTxData;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume failure
  //
  Status = EFI_UNSUPPORTED;
  pSocket->errno = ENOTCONN;
  *pDataLength = 0;

  //
  //  Verify that the socket is connected
  //
  if ( SOCKET_STATE_CONNECTED == pSocket->State ) {
    //
    //  Verify that there is enough room to buffer another
    //  transmit operation
    //
    pTxBytes = &pSocket->TxBytes;
    if ( pSocket->MaxTxBuf > *pTxBytes ) {
      //
      //  Locate the port
      //
      pPort = pSocket->pPortList;
      while ( NULL != pPort ) {
        //
        //  Determine the queue head
        //
        pUdp6 = &pPort->Context.Udp6;

        //
        //  Attempt to allocate the packet
        //
        Status = EslSocketPacketAllocate ( &pPacket,
                                           sizeof ( pPacket->Op.Udp6Tx )
                                           - sizeof ( pPacket->Op.Udp6Tx.Buffer )
                                           + BufferLength,
                                           0,
                                           DEBUG_TX );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Initialize the transmit operation
          //
          pTxData = &pPacket->Op.Udp6Tx;
          pTxData->TxData.UdpSessionData = NULL;
          pTxData->TxData.DataLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentCount = 1;
          pTxData->TxData.FragmentTable[0].FragmentLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentTable[0].FragmentBuffer = &pPacket->Op.Udp6Tx.Buffer[0];

          //
          //  Set the remote system address if necessary
          //
          pTxData->TxData.UdpSessionData = NULL;
          if ( NULL != pAddress ) {
            pRemoteAddress = (const struct sockaddr_in6 *)pAddress;
            CopyMem ( &pTxData->Session.SourceAddress,
                      &pUdp6->ConfigData.StationAddress,
                      sizeof ( pTxData->Session.SourceAddress ));
            pTxData->Session.SourcePort = 0;
            CopyMem ( &pTxData->Session.DestinationAddress,
                      &pRemoteAddress->sin6_addr,
                      sizeof ( pTxData->Session.DestinationAddress ));
            pTxData->Session.DestinationPort = SwapBytes16 ( pRemoteAddress->sin6_port );

            //
            //  Use the remote system address when sending this packet
            //
            pTxData->TxData.UdpSessionData = &pTxData->Session;
          }

          //
          //  Copy the data into the buffer
          //
          CopyMem ( &pPacket->Op.Udp6Tx.Buffer[0],
                    pBuffer,
                    BufferLength );

          //
          //  Synchronize with the socket layer
          //
          RAISE_TPL ( TplPrevious, TPL_SOCKETS );

          //
          //  Display the request
          //
          DEBUG (( DEBUG_TX,
                    "Send %d bytes from 0x%08x to [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                    BufferLength,
                    pBuffer,
                    pTxData->Session.DestinationAddress.Addr[0],
                    pTxData->Session.DestinationAddress.Addr[1],
                    pTxData->Session.DestinationAddress.Addr[2],
                    pTxData->Session.DestinationAddress.Addr[3],
                    pTxData->Session.DestinationAddress.Addr[4],
                    pTxData->Session.DestinationAddress.Addr[5],
                    pTxData->Session.DestinationAddress.Addr[6],
                    pTxData->Session.DestinationAddress.Addr[7],
                    pTxData->Session.DestinationAddress.Addr[8],
                    pTxData->Session.DestinationAddress.Addr[9],
                    pTxData->Session.DestinationAddress.Addr[10],
                    pTxData->Session.DestinationAddress.Addr[11],
                    pTxData->Session.DestinationAddress.Addr[12],
                    pTxData->Session.DestinationAddress.Addr[13],
                    pTxData->Session.DestinationAddress.Addr[14],
                    pTxData->Session.DestinationAddress.Addr[15],
                    pTxData->Session.DestinationPort ));

          //
          //  Queue the data for transmission
          //
          pPacket->pNext = NULL;
          pPreviousPacket = pSocket->pTxPacketListTail;
          if ( NULL == pPreviousPacket ) {
            pSocket->pTxPacketListHead = pPacket;
          }
          else {
            pPreviousPacket->pNext = pPacket;
          }
          pSocket->pTxPacketListTail = pPacket;
          DEBUG (( DEBUG_TX,
                    "0x%08x: Packet on transmit list\r\n",
                    pPacket ));

          //
          //  Account for the buffered data
          //
          *pTxBytes += BufferLength;
          *pDataLength = BufferLength;

          //
          //  Start the transmit engine if it is idle
          //
          if ( NULL != pPort->pTxFree ) {
            EslSocketTxStart ( pPort,
                               &pSocket->pTxPacketListHead,
                               &pSocket->pTxPacketListTail,
                               &pPort->pTxActive,
                               &pPort->pTxFree );

            //
            //  Ignore any transmit error
            //
            if ( EFI_ERROR ( pSocket->TxError )) {
              DEBUG (( DEBUG_TX,
                       "0x%08x: Transmit error, Packet: 0x%08x, Status: %r\r\n",
                       pPort,
                       pPacket,
                       pSocket->TxError ));
            }
            pSocket->TxError = EFI_SUCCESS;
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
          break;
        }

        //
        //  Set the next port
        //
        pPort = pPort->pLinkSocket;
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

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the transmit completion

  This routine use ::EslSocketTxComplete to perform the transmit
  completion processing for data packets.

  This routine is called by the UDPv4 network layer when a data
  transmit request completes.

  @param [in] Event     The normal transmit completion event

  @param [in] pIo       Address of an ::ESL_IO_MGMT structure

**/
VOID
EslUdp6TxComplete (
  IN EFI_EVENT Event,
  IN ESL_IO_MGMT * pIo
  )
{
  UINT32 LengthInBytes;
  ESL_PORT * pPort;
  ESL_PACKET * pPacket;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Locate the active transmit packet
  //
  pPacket = pIo->pPacket;
  pPort = pIo->pPort;
  pSocket = pPort->pSocket;

  //
  //  Get the transmit length and status
  //
  LengthInBytes = pPacket->Op.Udp6Tx.TxData.DataLength;
  pSocket->TxBytes -= LengthInBytes;
  Status = pIo->Token.Udp6Tx.Status;

  //
  //  Ignore the transmit error
  //
  if ( EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_TX,
             "0x%08x: Transmit completion error, Packet: 0x%08x, Status: %r\r\n",
             pPort,
             pPacket,
             Status ));
    Status = EFI_SUCCESS;
  }

  //
  //  Complete the transmit operation
  //
  EslSocketTxComplete ( pIo,
                        LengthInBytes,
                        Status,
                        "UDP ",
                        &pSocket->pTxPacketListHead,
                        &pSocket->pTxPacketListTail,
                        &pPort->pTxActive,
                        &pPort->pTxFree );
  DBG_EXIT ( );
}


/**
  Verify the adapter's IP address

  This support routine is called by EslSocketBindTest.

  @param [in] pPort       Address of an ::ESL_PORT structure.
  @param [in] pConfigData Address of the configuration data

  @retval EFI_SUCCESS - The IP address is valid
  @retval EFI_NOT_STARTED - The IP address is invalid

 **/
EFI_STATUS
EslUdp6VerifyLocalIpAddress (
  IN ESL_PORT * pPort,
  IN EFI_UDP6_CONFIG_DATA * pConfigData
  )
{
  UINTN AddressCount;
  EFI_IP6_ADDRESS_INFO * pAddressInfo;
  UINTN DataSize;
  EFI_IP6_CONFIG_INTERFACE_INFO * pIpConfigData;
  EFI_IP6_CONFIG_PROTOCOL * pIpConfigProtocol;
  ESL_SERVICE * pService;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use break instead of goto
  //
  pIpConfigData = NULL;
  for ( ; ; ) {
    //
    //  Determine if the IP address is specified
    //
    DEBUG (( DEBUG_BIND,
              "Requested IP address: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
              pConfigData->StationAddress.Addr[0],
              pConfigData->StationAddress.Addr[1],
              pConfigData->StationAddress.Addr[2],
              pConfigData->StationAddress.Addr[3],
              pConfigData->StationAddress.Addr[4],
              pConfigData->StationAddress.Addr[5],
              pConfigData->StationAddress.Addr[6],
              pConfigData->StationAddress.Addr[7],
              pConfigData->StationAddress.Addr[8],
              pConfigData->StationAddress.Addr[9],
              pConfigData->StationAddress.Addr[10],
              pConfigData->StationAddress.Addr[11],
              pConfigData->StationAddress.Addr[12],
              pConfigData->StationAddress.Addr[13],
              pConfigData->StationAddress.Addr[14],
              pConfigData->StationAddress.Addr[15]));
    if (( 0 == pConfigData->StationAddress.Addr [ 0 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 1 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 2 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 3 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 4 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 5 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 6 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 7 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 8 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 9 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 10 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 11 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 12 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 13 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 14 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 15 ]))
    {
      Status = EFI_SUCCESS;
      break;
    }

    //
    //  Open the configuration protocol
    //
    pService = pPort->pService;
    Status = gBS->OpenProtocol ( pService->Controller,
                                 &gEfiIp6ConfigProtocolGuid,
                                 (VOID **)&pIpConfigProtocol,
                                 NULL,
                                 NULL,
                                 EFI_OPEN_PROTOCOL_GET_PROTOCOL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - IP Configuration Protocol not available, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Get the IP configuration data size
    //
    DataSize = 0;
    Status = pIpConfigProtocol->GetData ( pIpConfigProtocol,
                                          Ip6ConfigDataTypeInterfaceInfo,
                                          &DataSize,
                                          NULL );
    if ( EFI_BUFFER_TOO_SMALL != Status ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to get IP Configuration data size, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Allocate the configuration data buffer
    //
    pIpConfigData = AllocatePool ( DataSize );
    if ( NULL == pIpConfigData ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Not enough memory to allocate IP Configuration data!\r\n" ));
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    //
    //  Get the IP configuration
    //
    Status = pIpConfigProtocol->GetData ( pIpConfigProtocol,
                                          Ip6ConfigDataTypeInterfaceInfo,
                                          &DataSize,
                                          pIpConfigData );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to return IP Configuration data, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Display the current configuration
    //
    DEBUG (( DEBUG_BIND,
              "Actual adapter IP address: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
              pIpConfigData->HwAddress.Addr [ 0 ],
              pIpConfigData->HwAddress.Addr [ 1 ],
              pIpConfigData->HwAddress.Addr [ 2 ],
              pIpConfigData->HwAddress.Addr [ 3 ],
              pIpConfigData->HwAddress.Addr [ 4 ],
              pIpConfigData->HwAddress.Addr [ 5 ],
              pIpConfigData->HwAddress.Addr [ 6 ],
              pIpConfigData->HwAddress.Addr [ 7 ],
              pIpConfigData->HwAddress.Addr [ 8 ],
              pIpConfigData->HwAddress.Addr [ 9 ],
              pIpConfigData->HwAddress.Addr [ 10 ],
              pIpConfigData->HwAddress.Addr [ 11 ],
              pIpConfigData->HwAddress.Addr [ 12 ],
              pIpConfigData->HwAddress.Addr [ 13 ],
              pIpConfigData->HwAddress.Addr [ 14 ],
              pIpConfigData->HwAddress.Addr [ 15 ]));

    //
    //  Validate the hardware address
    //
    Status = EFI_SUCCESS;
    if (( 16 == pIpConfigData->HwAddressSize )
      && ( pConfigData->StationAddress.Addr [ 0 ] == pIpConfigData->HwAddress.Addr [ 0 ])
      && ( pConfigData->StationAddress.Addr [ 1 ] == pIpConfigData->HwAddress.Addr [ 1 ])
      && ( pConfigData->StationAddress.Addr [ 2 ] == pIpConfigData->HwAddress.Addr [ 2 ])
      && ( pConfigData->StationAddress.Addr [ 3 ] == pIpConfigData->HwAddress.Addr [ 3 ])
      && ( pConfigData->StationAddress.Addr [ 4 ] == pIpConfigData->HwAddress.Addr [ 4 ])
      && ( pConfigData->StationAddress.Addr [ 5 ] == pIpConfigData->HwAddress.Addr [ 5 ])
      && ( pConfigData->StationAddress.Addr [ 6 ] == pIpConfigData->HwAddress.Addr [ 6 ])
      && ( pConfigData->StationAddress.Addr [ 7 ] == pIpConfigData->HwAddress.Addr [ 7 ])
      && ( pConfigData->StationAddress.Addr [ 8 ] == pIpConfigData->HwAddress.Addr [ 8 ])
      && ( pConfigData->StationAddress.Addr [ 9 ] == pIpConfigData->HwAddress.Addr [ 9 ])
      && ( pConfigData->StationAddress.Addr [ 10 ] == pIpConfigData->HwAddress.Addr [ 10 ])
      && ( pConfigData->StationAddress.Addr [ 11 ] == pIpConfigData->HwAddress.Addr [ 11 ])
      && ( pConfigData->StationAddress.Addr [ 12 ] == pIpConfigData->HwAddress.Addr [ 12 ])
      && ( pConfigData->StationAddress.Addr [ 13 ] == pIpConfigData->HwAddress.Addr [ 13 ])
      && ( pConfigData->StationAddress.Addr [ 14 ] == pIpConfigData->HwAddress.Addr [ 14 ])
      && ( pConfigData->StationAddress.Addr [ 15 ] == pIpConfigData->HwAddress.Addr [ 15 ])) {
      break;
    }

    //
    //  Walk the list of other IP addresses assigned to this adapter
    //
    for ( AddressCount = 0; pIpConfigData->AddressInfoCount > AddressCount; AddressCount += 1 ) {
      pAddressInfo = &pIpConfigData->AddressInfo [ AddressCount ];

      //
      //  Display the IP address
      //
      DEBUG (( DEBUG_BIND,
                "Actual adapter IP address: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                pAddressInfo->Address.Addr [ 0 ],
                pAddressInfo->Address.Addr [ 1 ],
                pAddressInfo->Address.Addr [ 2 ],
                pAddressInfo->Address.Addr [ 3 ],
                pAddressInfo->Address.Addr [ 4 ],
                pAddressInfo->Address.Addr [ 5 ],
                pAddressInfo->Address.Addr [ 6 ],
                pAddressInfo->Address.Addr [ 7 ],
                pAddressInfo->Address.Addr [ 8 ],
                pAddressInfo->Address.Addr [ 9 ],
                pAddressInfo->Address.Addr [ 10 ],
                pAddressInfo->Address.Addr [ 11 ],
                pAddressInfo->Address.Addr [ 12 ],
                pAddressInfo->Address.Addr [ 13 ],
                pAddressInfo->Address.Addr [ 14 ],
                pAddressInfo->Address.Addr [ 15 ]));

      //
      //  Validate the IP address
      //
      if (( pConfigData->StationAddress.Addr [ 0 ] == pAddressInfo->Address.Addr [ 0 ])
        && ( pConfigData->StationAddress.Addr [ 1 ] == pAddressInfo->Address.Addr [ 1 ])
        && ( pConfigData->StationAddress.Addr [ 2 ] == pAddressInfo->Address.Addr [ 2 ])
        && ( pConfigData->StationAddress.Addr [ 3 ] == pAddressInfo->Address.Addr [ 3 ])
        && ( pConfigData->StationAddress.Addr [ 4 ] == pAddressInfo->Address.Addr [ 4 ])
        && ( pConfigData->StationAddress.Addr [ 5 ] == pAddressInfo->Address.Addr [ 5 ])
        && ( pConfigData->StationAddress.Addr [ 6 ] == pAddressInfo->Address.Addr [ 6 ])
        && ( pConfigData->StationAddress.Addr [ 7 ] == pAddressInfo->Address.Addr [ 7 ])
        && ( pConfigData->StationAddress.Addr [ 8 ] == pAddressInfo->Address.Addr [ 8 ])
        && ( pConfigData->StationAddress.Addr [ 9 ] == pAddressInfo->Address.Addr [ 9 ])
        && ( pConfigData->StationAddress.Addr [ 10 ] == pAddressInfo->Address.Addr [ 10 ])
        && ( pConfigData->StationAddress.Addr [ 11 ] == pAddressInfo->Address.Addr [ 11 ])
        && ( pConfigData->StationAddress.Addr [ 12 ] == pAddressInfo->Address.Addr [ 12 ])
        && ( pConfigData->StationAddress.Addr [ 13 ] == pAddressInfo->Address.Addr [ 13 ])
        && ( pConfigData->StationAddress.Addr [ 14 ] == pAddressInfo->Address.Addr [ 14 ])
        && ( pConfigData->StationAddress.Addr [ 15 ] == pAddressInfo->Address.Addr [ 15 ])) {
        break;
      }
    }
    if ( pIpConfigData->AddressInfoCount > AddressCount ) {
      break;
    }

    //
    //  The IP address did not match
    //
    Status = EFI_NOT_STARTED;
    break;
  }

  //
  //  Free the buffer if necessary
  //
  if ( NULL != pIpConfigData ) {
    FreePool ( pIpConfigData );
  }

  //
  //  Return the IP address status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Interface between the socket layer and the network specific
  code that supports SOCK_DGRAM sockets over UDPv4.
**/
CONST ESL_PROTOCOL_API cEslUdp6Api = {
  "UDPv6",
  IPPROTO_UDP,
  OFFSET_OF ( ESL_PORT, Context.Udp6.ConfigData ),
  OFFSET_OF ( ESL_LAYER, pUdp6List ),
  sizeof ( struct sockaddr_in6 ),
  sizeof ( struct sockaddr_in6 ),
  AF_INET6,
  sizeof (((ESL_PACKET *)0 )->Op.Udp6Rx ),
  sizeof (((ESL_PACKET *)0 )->Op.Udp6Rx ),
  OFFSET_OF ( ESL_IO_MGMT, Token.Udp6Rx.Packet.RxData ),
  FALSE,
  EADDRINUSE,
  NULL,   //  Accept
  NULL,   //  ConnectPoll
  NULL,   //  ConnectStart
  EslUdp6SocketIsConfigured,
  EslUdp6LocalAddressGet,
  EslUdp6LocalAddressSet,
  NULL,   //  Listen
  NULL,   //  OptionGet
  NULL,   //  OptionSet
  EslUdp6PacketFree,
  EslUdp6PortAllocate,
  NULL,   //  PortClose,
  NULL,   //  PortCloseOp
  TRUE,
  EslUdp6Receive,
  EslUdp6RemoteAddressGet,
  EslUdp6RemoteAddressSet,
  EslUdp6RxComplete,
  NULL,   //  RxStart
  EslUdp6TxBuffer,
  EslUdp6TxComplete,
  NULL,   //  TxOobComplete
  (PFN_API_VERIFY_LOCAL_IP_ADDRESS)EslUdp6VerifyLocalIpAddress
};
