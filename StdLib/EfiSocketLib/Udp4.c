/** @file
  Implement the UDP4 driver support for the socket layer.

  Copyright (c) 2011 - 2015, Intel Corporation
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

  This routine returns the IPv4 address and UDP port number associated
  with the local socket.

  This routine is called by ::EslSocketGetLocalAddress to determine the
  network address for the SOCK_DGRAM socket.

  @param [in] pPort       Address of an ::ESL_PORT structure.

  @param [out] pSockAddr  Network address to receive the local system address

**/
VOID
EslUdp4LocalAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pSockAddr
  )
{
  struct sockaddr_in * pLocalAddress;
  ESL_UDP4_CONTEXT * pUdp4;

  DBG_ENTER ( );

  //
  //  Return the local address
  //
  pUdp4 = &pPort->Context.Udp4;
  pLocalAddress = (struct sockaddr_in *)pSockAddr;
  pLocalAddress->sin_family = AF_INET;
  pLocalAddress->sin_port = SwapBytes16 ( pUdp4->ConfigData.StationPort );
  CopyMem ( &pLocalAddress->sin_addr,
            &pUdp4->ConfigData.StationAddress.Addr[0],
            sizeof ( pLocalAddress->sin_addr ));

  DBG_EXIT ( );
}


/**
  Set the local port address.

  This routine sets the local port address.

  This support routine is called by ::EslSocketPortAllocate.

  @param [in] pPort       Address of an ESL_PORT structure
  @param [in] pSockAddr   Address of a sockaddr structure that contains the
                          connection point on the local machine.  An IPv4 address
                          of INADDR_ANY specifies that the connection is made to
                          all of the network stacks on the platform.  Specifying a
                          specific IPv4 address restricts the connection to the
                          network stack supporting that address.  Specifying zero
                          for the port causes the network layer to assign a port
                          number from the dynamic range.  Specifying a specific
                          port number causes the network layer to use that port.

  @param [in] bBindTest   TRUE = run bind testing

  @retval EFI_SUCCESS     The operation was successful

 **/
EFI_STATUS
EslUdp4LocalAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN BOOLEAN bBindTest
  )
{
  EFI_UDP4_CONFIG_DATA * pConfig;
  CONST struct sockaddr_in * pIpAddress;
  CONST UINT8 * pIpv4Address;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Validate the address
  //
  pIpAddress = (struct sockaddr_in *)pSockAddr;
  if ( INADDR_BROADCAST == pIpAddress->sin_addr.s_addr ) {
    //
    //  The local address must not be the broadcast address
    //
    Status = EFI_INVALID_PARAMETER;
    pPort->pSocket->errno = EADDRNOTAVAIL;
  }
  else {
    //
    //  Set the local address
    //
    pIpAddress = (struct sockaddr_in *)pSockAddr;
    pIpv4Address = (UINT8 *)&pIpAddress->sin_addr.s_addr;
    pConfig = &pPort->Context.Udp4.ConfigData;
    pConfig->StationAddress.Addr[0] = pIpv4Address[0];
    pConfig->StationAddress.Addr[1] = pIpv4Address[1];
    pConfig->StationAddress.Addr[2] = pIpv4Address[2];
    pConfig->StationAddress.Addr[3] = pIpv4Address[3];

    //
    //  Determine if the default address is used
    //
    pConfig->UseDefaultAddress = (BOOLEAN)( 0 == pIpAddress->sin_addr.s_addr );
    
    //
    //  Set the subnet mask
    //
    if ( pConfig->UseDefaultAddress ) {
      pConfig->SubnetMask.Addr[0] = 0;
      pConfig->SubnetMask.Addr[1] = 0;
      pConfig->SubnetMask.Addr[2] = 0;
      pConfig->SubnetMask.Addr[3] = 0;
    }
    else {
      pConfig->SubnetMask.Addr[0] = 0xff;
      pConfig->SubnetMask.Addr[1] = ( 128 <= pConfig->StationAddress.Addr[0]) ? 0xff : 0;
      pConfig->SubnetMask.Addr[2] = ( 192 <= pConfig->StationAddress.Addr[0]) ? 0xff : 0;
      pConfig->SubnetMask.Addr[3] = ( 224 <= pConfig->StationAddress.Addr[0]) ? 0xff : 0;
    }

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
      pConfig->StationPort = SwapBytes16 ( pIpAddress->sin_port );

      //
      //  Display the local address
      //
      DEBUG (( DEBUG_BIND,
                "0x%08x: Port, Local UDP4 Address: %d.%d.%d.%d:%d\r\n",
                pPort,
                pConfig->StationAddress.Addr[0],
                pConfig->StationAddress.Addr[1],
                pConfig->StationAddress.Addr[2],
                pConfig->StationAddress.Addr[3],
                pConfig->StationPort ));
    }
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
EslUdp4PacketFree (
  IN ESL_PACKET * pPacket,
  IN OUT size_t * pRxBytes
  )
{
  EFI_UDP4_RECEIVE_DATA * pRxData;

  DBG_ENTER ( );

  //
  //  Account for the receive bytes
  //
  pRxData = pPacket->Op.Udp4Rx.pRxData;
  *pRxBytes -= pRxData->DataLength;

  //
  //  Disconnect the buffer from the packet
  //
  pPacket->Op.Udp4Rx.pRxData = NULL;

  //
  //  Return the buffer to the UDP4 driver
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
EslUdp4PortAllocate (
  IN ESL_PORT * pPort,
  IN UINTN DebugFlags
  )
{
  EFI_UDP4_CONFIG_DATA * pConfig;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Initialize the port
  //
  pSocket = pPort->pSocket;
  pSocket->TxPacketOffset = OFFSET_OF ( ESL_PACKET, Op.Udp4Tx.TxData );
  pSocket->TxTokenEventOffset = OFFSET_OF ( ESL_IO_MGMT, Token.Udp4Tx.Event );
  pSocket->TxTokenOffset = OFFSET_OF ( EFI_UDP4_COMPLETION_TOKEN, Packet.TxData );

  //
  //  Save the cancel, receive and transmit addresses
  //
  pPort->pfnConfigure = (PFN_NET_CONFIGURE)pPort->pProtocol.UDPv4->Configure;
  pPort->pfnRxCancel = (PFN_NET_IO_START)pPort->pProtocol.UDPv4->Cancel;
  pPort->pfnRxPoll = (PFN_NET_POLL)pPort->pProtocol.UDPv4->Poll;
  pPort->pfnRxStart = (PFN_NET_IO_START)pPort->pProtocol.UDPv4->Receive;
  pPort->pfnTxStart = (PFN_NET_IO_START)pPort->pProtocol.UDPv4->Transmit;

  //
  //  Set the configuration flags
  //
  pConfig = &pPort->Context.Udp4.ConfigData;
  pConfig->TimeToLive = 255;
  pConfig->AcceptAnyPort = FALSE;
  pConfig->AcceptBroadcast = FALSE;
  pConfig->AcceptPromiscuous = FALSE;
  pConfig->AllowDuplicatePort = TRUE;
  pConfig->DoNotFragment = FALSE;
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
EslUdp4Receive (
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
  struct sockaddr_in * pRemoteAddress;
  EFI_UDP4_RECEIVE_DATA * pRxData;

  DBG_ENTER ( );

  pRxData = pPacket->Op.Udp4Rx.pRxData;
  //
  //  Return the remote system address if requested
  //
  if ( NULL != pAddress ) {
    //
    //  Build the remote address
    //
    DEBUG (( DEBUG_RX,
              "Getting packet remote address: %d.%d.%d.%d:%d\r\n",
              pRxData->UdpSession.SourceAddress.Addr[0],
              pRxData->UdpSession.SourceAddress.Addr[1],
              pRxData->UdpSession.SourceAddress.Addr[2],
              pRxData->UdpSession.SourceAddress.Addr[3],
              pRxData->UdpSession.SourcePort ));
    pRemoteAddress = (struct sockaddr_in *)pAddress;
    CopyMem ( &pRemoteAddress->sin_addr,
              &pRxData->UdpSession.SourceAddress.Addr[0],
              sizeof ( pRemoteAddress->sin_addr ));
    pRemoteAddress->sin_port = SwapBytes16 ( pRxData->UdpSession.SourcePort );
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
EslUdp4RemoteAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pAddress
  )
{
  struct sockaddr_in * pRemoteAddress;
  ESL_UDP4_CONTEXT * pUdp4;

  DBG_ENTER ( );

  //
  //  Return the remote address
  //
  pUdp4 = &pPort->Context.Udp4;
  pRemoteAddress = (struct sockaddr_in *)pAddress;
  pRemoteAddress->sin_family = AF_INET;
  pRemoteAddress->sin_port = SwapBytes16 ( pUdp4->ConfigData.RemotePort );
  CopyMem ( &pRemoteAddress->sin_addr,
            &pUdp4->ConfigData.RemoteAddress.Addr[0],
            sizeof ( pRemoteAddress->sin_addr ));

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
EslUdp4RemoteAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  )
{
  CONST struct sockaddr_in * pRemoteAddress;
  ESL_UDP4_CONTEXT * pUdp4;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Set the remote address
  //
  pUdp4 = &pPort->Context.Udp4;
  pRemoteAddress = (struct sockaddr_in *)pSockAddr;
  pUdp4->ConfigData.RemoteAddress.Addr[0] = (UINT8)( pRemoteAddress->sin_addr.s_addr );
  pUdp4->ConfigData.RemoteAddress.Addr[1] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 8 );
  pUdp4->ConfigData.RemoteAddress.Addr[2] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 16 );
  pUdp4->ConfigData.RemoteAddress.Addr[3] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 24 );
  pUdp4->ConfigData.RemotePort = SwapBytes16 ( pRemoteAddress->sin_port );
  pPort->pSocket->bAddressSet = TRUE;
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
  in FIFO order to the data queue.  The UDP4 driver's buffer will
  be returned by either ::EslUdp4Receive or ::EslSocketPortCloseTxDone.
  See the \ref ReceiveEngine section.

  This routine is called by the UDPv4 driver when data is
  received.

  @param [in] Event     The receive completion event

  @param [in] pIo       Address of an ::ESL_IO_MGMT structure

**/
VOID
EslUdp4RxComplete (
  IN EFI_EVENT Event,
  IN ESL_IO_MGMT * pIo
  )
{
  size_t LengthInBytes;
  ESL_PACKET * pPacket;
  EFI_UDP4_RECEIVE_DATA * pRxData;
  EFI_STATUS Status;
  
  DBG_ENTER ( );

  //
  //  Get the operation status.
  //
  Status = pIo->Token.Udp4Rx.Status;
  
  //
  //  Get the packet length
  //
  pRxData = pIo->Token.Udp4Rx.Packet.RxData;
  LengthInBytes = pRxData->DataLength;

  //
  //      +--------------------+   +-----------------------+
  //      | ESL_IO_MGMT        |   |      Data Buffer      |
  //      |                    |   |     (Driver owned)    |
  //      |    +---------------+   +-----------------------+
  //      |    | Token         |               ^
  //      |    |      Rx Event |               |
  //      |    |               |   +-----------------------+
  //      |    |        RxData --> | EFI_UDP4_RECEIVE_DATA |
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
  pPacket->Op.Udp4Rx.pRxData = pRxData;

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
 EslUdp4SocketIsConfigured (
  IN ESL_SOCKET * pSocket
  )
{
  EFI_UDP4_CONFIG_DATA * pConfigData;
  ESL_PORT * pPort;
  ESL_PORT * pNextPort;
  ESL_UDP4_CONTEXT * pUdp4;
  EFI_UDP4_PROTOCOL * pUdp4Protocol;
  EFI_STATUS Status;
  struct sockaddr_in LocalAddress;

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
      LocalAddress.sin_len = sizeof ( LocalAddress );
      LocalAddress.sin_family = AF_INET;
      LocalAddress.sin_addr.s_addr = 0;
      LocalAddress.sin_port = 0;
      Status = EslSocketBind ( &pSocket->SocketProtocol,
                               (struct sockaddr *)&LocalAddress,
                               LocalAddress.sin_len,
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
      pUdp4 = &pPort->Context.Udp4;
      pUdp4Protocol = pPort->pProtocol.UDPv4;
      pConfigData = &pUdp4->ConfigData;
      DEBUG (( DEBUG_TX,
                "0x%08x: pPort Configuring for %d.%d.%d.%d:%d --> %d.%d.%d.%d:%d\r\n",
                pPort,
                pConfigData->StationAddress.Addr[0],
                pConfigData->StationAddress.Addr[1],
                pConfigData->StationAddress.Addr[2],
                pConfigData->StationAddress.Addr[3],
                pConfigData->StationPort,
                pConfigData->RemoteAddress.Addr[0],
                pConfigData->RemoteAddress.Addr[1],
                pConfigData->RemoteAddress.Addr[2],
                pConfigData->RemoteAddress.Addr[3],
                pConfigData->RemotePort ));
      Status = pUdp4Protocol->Configure ( pUdp4Protocol,
                                          pConfigData );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Update the configuration data
        //
        Status = pUdp4Protocol->GetModeData ( pUdp4Protocol,
                                              pConfigData,
                                              NULL,
                                              NULL,
                                              NULL );
      }
      if ( EFI_ERROR ( Status )) {
        if ( !pSocket->bConfigured ) {
          DEBUG (( DEBUG_LISTEN,
                    "ERROR - Failed to configure the Udp4 port, Status: %r\r\n",
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
                  "0x%08x: pPort Configured for %d.%d.%d.%d:%d --> %d.%d.%d.%d:%d\r\n",
                  pPort,
                  pConfigData->StationAddress.Addr[0],
                  pConfigData->StationAddress.Addr[1],
                  pConfigData->StationAddress.Addr[2],
                  pConfigData->StationAddress.Addr[3],
                  pConfigData->StationPort,
                  pConfigData->RemoteAddress.Addr[0],
                  pConfigData->RemoteAddress.Addr[1],
                  pConfigData->RemoteAddress.Addr[2],
                  pConfigData->RemoteAddress.Addr[3],
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
EslUdp4TxBuffer (
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
  const struct sockaddr_in * pRemoteAddress;
  ESL_UDP4_CONTEXT * pUdp4;
  size_t * pTxBytes;
  ESL_UDP4_TX_DATA * pTxData;
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
        pUdp4 = &pPort->Context.Udp4;

        //
        //  Attempt to allocate the packet
        //
        Status = EslSocketPacketAllocate ( &pPacket,
                                           sizeof ( pPacket->Op.Udp4Tx )
                                           - sizeof ( pPacket->Op.Udp4Tx.Buffer )
                                           + BufferLength,
                                           0,
                                           DEBUG_TX );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Initialize the transmit operation
          //
          pTxData = &pPacket->Op.Udp4Tx;
          pTxData->TxData.GatewayAddress = NULL;
          pTxData->TxData.UdpSessionData = NULL;
          pTxData->TxData.DataLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentCount = 1;
          pTxData->TxData.FragmentTable[0].FragmentLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentTable[0].FragmentBuffer = &pPacket->Op.Udp4Tx.Buffer[0];
          pTxData->RetransmitCount = 0;

          //
          //  Set the remote system address if necessary
          //
          pTxData->TxData.UdpSessionData = NULL;
          if ( NULL != pAddress ) {
            pRemoteAddress = (const struct sockaddr_in *)pAddress;
            pTxData->Session.SourceAddress.Addr[0] = pUdp4->ConfigData.StationAddress.Addr[0];
            pTxData->Session.SourceAddress.Addr[1] = pUdp4->ConfigData.StationAddress.Addr[1];
            pTxData->Session.SourceAddress.Addr[2] = pUdp4->ConfigData.StationAddress.Addr[2];
            pTxData->Session.SourceAddress.Addr[3] = pUdp4->ConfigData.StationAddress.Addr[3];
            pTxData->Session.SourcePort = 0;
            pTxData->Session.DestinationAddress.Addr[0] = (UINT8)pRemoteAddress->sin_addr.s_addr;
            pTxData->Session.DestinationAddress.Addr[1] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 8 );
            pTxData->Session.DestinationAddress.Addr[2] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 16 );
            pTxData->Session.DestinationAddress.Addr[3] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 24 );
            pTxData->Session.DestinationPort = SwapBytes16 ( pRemoteAddress->sin_port );

            //
            //  Use the remote system address when sending this packet
            //
            pTxData->TxData.UdpSessionData = &pTxData->Session;
          }

          //
          //  Copy the data into the buffer
          //
          CopyMem ( &pPacket->Op.Udp4Tx.Buffer[0],
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
                    "Send %d bytes from 0x%08x to %d.%d.%d.%d:%d\r\n",
                    BufferLength,
                    pBuffer,
                    pTxData->Session.DestinationAddress.Addr[0],
                    pTxData->Session.DestinationAddress.Addr[1],
                    pTxData->Session.DestinationAddress.Addr[2],
                    pTxData->Session.DestinationAddress.Addr[3],
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
            pPacket = pSocket->pTxPacketListHead;
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
EslUdp4TxComplete (
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
  LengthInBytes = pPacket->Op.Udp4Tx.TxData.DataLength;
  pSocket->TxBytes -= LengthInBytes;
  Status = pIo->Token.Udp4Tx.Status;

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
EslUdp4VerifyLocalIpAddress (
  IN ESL_PORT * pPort,
  IN EFI_UDP4_CONFIG_DATA * pConfigData
  )
{
  UINTN DataSize;
  EFI_IP4_CONFIG2_INTERFACE_INFO * pIfInfo;
  EFI_IP4_CONFIG2_PROTOCOL * pIpConfig2Protocol;
  ESL_SERVICE * pService;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use break instead of goto
  //
  pIfInfo = NULL;
  for ( ; ; ) {
    //
    //  Determine if the IP address is specified
    //
    DEBUG (( DEBUG_BIND,
              "UseDefaultAddress: %s\r\n",
              pConfigData->UseDefaultAddress ? L"TRUE" : L"FALSE" ));
    DEBUG (( DEBUG_BIND,
              "Requested IP address: %d.%d.%d.%d\r\n",
              pConfigData->StationAddress.Addr [ 0 ],
              pConfigData->StationAddress.Addr [ 1 ],
              pConfigData->StationAddress.Addr [ 2 ],
              pConfigData->StationAddress.Addr [ 3 ]));
    if ( pConfigData->UseDefaultAddress
      || (( 0 == pConfigData->StationAddress.Addr [ 0 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 1 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 2 ])
      && ( 0 == pConfigData->StationAddress.Addr [ 3 ])))
    {
      Status = EFI_SUCCESS;
      break;
    }

    //
    //  Open the configuration protocol
    //
    pService = pPort->pService;
    Status = gBS->OpenProtocol ( 
                    pService->Controller,
                    &gEfiIp4Config2ProtocolGuid,
                    (VOID **)&pIpConfig2Protocol,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL 
                    );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - IP Configuration Protocol not available, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Get the interface information size
    //
    DataSize = 0;
    Status = pIpConfig2Protocol->GetData ( 
                                   pIpConfig2Protocol,
                                   Ip4Config2DataTypeInterfaceInfo,
                                   &DataSize,
                                   NULL
                                   );
    if ( EFI_BUFFER_TOO_SMALL != Status ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to get the interface information size, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Allocate the interface information buffer
    //
    pIfInfo = AllocatePool ( DataSize );
    if ( NULL == pIfInfo ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Not enough memory to allocate the interface information buffer!\r\n" ));
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    //
    // Get the interface info.
    //
    Status = pIpConfig2Protocol->GetData ( 
                                  pIpConfig2Protocol,
                                  Ip4Config2DataTypeInterfaceInfo,
                                  &DataSize,
                                  pIfInfo
                                  );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to return the interface info, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Display the current configuration
    //
    DEBUG (( DEBUG_BIND,
              "Actual adapter IP address: %d.%d.%d.%d\r\n",
              pIfInfo->StationAddress.Addr [ 0 ],
              pIfInfo->StationAddress.Addr [ 1 ],
              pIfInfo->StationAddress.Addr [ 2 ],
              pIfInfo->StationAddress.Addr [ 3 ]));

    //
    //  Assume the port is not configured
    //
    Status = EFI_SUCCESS;
    if (( pConfigData->StationAddress.Addr [ 0 ] == pIfInfo->StationAddress.Addr [ 0 ])
      && ( pConfigData->StationAddress.Addr [ 1 ] == pIfInfo->StationAddress.Addr [ 1 ])
      && ( pConfigData->StationAddress.Addr [ 2 ] == pIfInfo->StationAddress.Addr [ 2 ])
      && ( pConfigData->StationAddress.Addr [ 3 ] == pIfInfo->StationAddress.Addr [ 3 ])) {
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
  if ( NULL != pIfInfo ) {
    FreePool ( pIfInfo );
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
CONST ESL_PROTOCOL_API cEslUdp4Api = {
  "UDPv4",
  IPPROTO_UDP,
  OFFSET_OF ( ESL_PORT, Context.Udp4.ConfigData ),
  OFFSET_OF ( ESL_LAYER, pUdp4List ),
  OFFSET_OF ( struct sockaddr_in, sin_zero ),
  sizeof ( struct sockaddr_in ),
  AF_INET,
  sizeof (((ESL_PACKET *)0 )->Op.Udp4Rx ),
  sizeof (((ESL_PACKET *)0 )->Op.Udp4Rx ),
  OFFSET_OF ( ESL_IO_MGMT, Token.Udp4Rx.Packet.RxData ),
  FALSE,
  EADDRINUSE,
  NULL,   //  Accept
  NULL,   //  ConnectPoll
  NULL,   //  ConnectStart
  EslUdp4SocketIsConfigured,
  EslUdp4LocalAddressGet,
  EslUdp4LocalAddressSet,
  NULL,   //  Listen
  NULL,   //  OptionGet
  NULL,   //  OptionSet
  EslUdp4PacketFree,
  EslUdp4PortAllocate,
  NULL,   //  PortClose,
  NULL,   //  PortCloseOp
  TRUE,
  EslUdp4Receive,
  EslUdp4RemoteAddressGet,
  EslUdp4RemoteAddressSet,
  EslUdp4RxComplete,
  NULL,   //  RxStart
  EslUdp4TxBuffer,
  EslUdp4TxComplete,
  NULL,   //  TxOobComplete
  (PFN_API_VERIFY_LOCAL_IP_ADDRESS)EslUdp4VerifyLocalIpAddress
};
