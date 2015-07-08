/** @file
  Implement the IP4 driver support for the socket layer.

  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include "Socket.h"


/** Get the local socket address.

  This routine returns the IPv4 address associated with the local
  socket.

  This routine is called by ::EslSocketGetLocalAddress to determine the
  network address for the SOCK_RAW socket.

  @param [in] pPort       Address of an ::ESL_PORT structure.
  @param [out] pAddress   Network address to receive the local system address
**/
VOID
EslIp4LocalAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pAddress
  )
{
  struct sockaddr_in * pLocalAddress;
  ESL_IP4_CONTEXT * pIp4;

  DBG_ENTER ( );

  //  Return the local address
  pIp4 = &pPort->Context.Ip4;
  pLocalAddress = (struct sockaddr_in *)pAddress;
  pLocalAddress->sin_family = AF_INET;
  CopyMem ( &pLocalAddress->sin_addr,
            &pIp4->ModeData.ConfigData.StationAddress.Addr[0],
            sizeof ( pLocalAddress->sin_addr ));

  DBG_EXIT ( );
}


/** Set the local port address.

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
EslIp4LocalAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN BOOLEAN bBindTest
  )
{
  EFI_IP4_CONFIG_DATA * pConfig;
  CONST struct sockaddr_in * pIpAddress;
  CONST UINT8 * pIpv4Address;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //  Validate the address
  pIpAddress = (struct sockaddr_in *)pSockAddr;
  if ( INADDR_BROADCAST == pIpAddress->sin_addr.s_addr ) {
    //  The local address must not be the broadcast address
    Status = EFI_INVALID_PARAMETER;
    pPort->pSocket->errno = EADDRNOTAVAIL;
  }
  else {
    Status = EFI_SUCCESS;

    //  Set the local address
    pIpAddress = (struct sockaddr_in *)pSockAddr;
    pIpv4Address = (UINT8 *)&pIpAddress->sin_addr.s_addr;
    pConfig = &pPort->Context.Ip4.ModeData.ConfigData;
    pConfig->StationAddress.Addr[0] = pIpv4Address[0];
    pConfig->StationAddress.Addr[1] = pIpv4Address[1];
    pConfig->StationAddress.Addr[2] = pIpv4Address[2];
    pConfig->StationAddress.Addr[3] = pIpv4Address[3];

    //  Determine if the default address is used
    pConfig->UseDefaultAddress = (BOOLEAN)( 0 == pIpAddress->sin_addr.s_addr );

    //  Display the local address
    DEBUG (( DEBUG_BIND,
              "0x%08x: Port, Local IP4 Address: %d.%d.%d.%d\r\n",
              pPort,
              pConfig->StationAddress.Addr[0],
              pConfig->StationAddress.Addr[1],
              pConfig->StationAddress.Addr[2],
              pConfig->StationAddress.Addr[3]));

    //  Set the subnet mask
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
  }
  //  Return the operation status
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Get the option value.

  This routine handles the IPv4 level options.

  The ::EslSocketOptionGet routine calls this routine to retrieve
  the IPv4 options one at a time by name.

  @param [in] pSocket           Address of an ::ESL_SOCKET structure
  @param [in] OptionName        Name of the option
  @param [out] ppOptionData     Buffer to receive address of option value
  @param [out] pOptionLength    Buffer to receive the option length

  @retval EFI_SUCCESS - Socket data successfully received
 **/
EFI_STATUS
EslIp4OptionGet (
  IN ESL_SOCKET * pSocket,
  IN int OptionName,
  OUT CONST void ** __restrict ppOptionData,
  OUT socklen_t * __restrict pOptionLength
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //  Assume success
  pSocket->errno = 0;
  Status = EFI_SUCCESS;

  //  Attempt to get the option
  switch ( OptionName ) {
  default:
    //  Option not supported
    pSocket->errno = ENOPROTOOPT;
    Status = EFI_INVALID_PARAMETER;
    break;

  case IP_HDRINCL:
    *ppOptionData = (void *)&pSocket->bIncludeHeader;
    *pOptionLength = sizeof ( pSocket->bIncludeHeader );
    break;
  }
  //  Return the operation status
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/** Set the option value.

  This routine handles the IPv4 level options.

  The ::EslSocketOptionSet routine calls this routine to adjust
  the IPv4 options one at a time by name.

  @param [in] pSocket         Address of an ::ESL_SOCKET structure
  @param [in] OptionName      Name of the option
  @param [in] pOptionValue    Buffer containing the option value
  @param [in] OptionLength    Length of the buffer in bytes

  @retval EFI_SUCCESS - Option successfully set
 **/
EFI_STATUS
EslIp4OptionSet (
  IN ESL_SOCKET * pSocket,
  IN int OptionName,
  IN CONST void * pOptionValue,
  IN socklen_t OptionLength
  )
{
  BOOLEAN bTrueFalse;
  //socklen_t LengthInBytes;
  //UINT8 * pOptionData;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //  Assume success
  pSocket->errno = 0;
  Status = EFI_SUCCESS;

  //  Determine if the option protocol matches
  //LengthInBytes = 0;
  //pOptionData = NULL;
  switch ( OptionName ) {
  default:
    //  Protocol level not supported
    DEBUG (( DEBUG_INFO | DEBUG_OPTION, "ERROR - Invalid protocol option\r\n" ));
    pSocket->errno = ENOTSUP;
    Status = EFI_UNSUPPORTED;
    break;

  case IP_HDRINCL:

    //  Validate the option length
    if ( sizeof ( UINT32 ) == OptionLength ) {
      //  Restrict the input to TRUE or FALSE
      bTrueFalse = TRUE;
      if ( 0 == *(UINT32 *)pOptionValue ) {
        bTrueFalse = FALSE;
      }
      pOptionValue = &bTrueFalse;

      //  Set the option value
      //pOptionData = (UINT8 *)&pSocket->bIncludeHeader;
      //LengthInBytes = sizeof ( pSocket->bIncludeHeader );
    }
    break;
  }
  //  Return the operation status
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
EslIp4PacketFree (
  IN ESL_PACKET * pPacket,
  IN OUT size_t * pRxBytes
  )
{
  EFI_IP4_RECEIVE_DATA * pRxData;
  DBG_ENTER ( );

  //
  //  Account for the receive bytes
  //
  pRxData = pPacket->Op.Ip4Rx.pRxData;
  *pRxBytes -= pRxData->HeaderLength + pRxData->DataLength;

  //
  //  Disconnect the buffer from the packet
  //
  pPacket->Op.Ip4Rx.pRxData = NULL;

  //
  //  Return the buffer to the IP4 driver
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
  running the IPv4 protocol.

  @param [in] pPort       Address of an ESL_PORT structure
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslIp4PortAllocate (
  IN ESL_PORT * pPort,
  IN UINTN DebugFlags
  )
{
  EFI_IP4_CONFIG_DATA * pConfig;
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Initialize the port
  //
  pSocket = pPort->pSocket;
  pSocket->TxPacketOffset = OFFSET_OF ( ESL_PACKET, Op.Ip4Tx.TxData );
  pSocket->TxTokenEventOffset = OFFSET_OF ( ESL_IO_MGMT, Token.Ip4Tx.Event );
  pSocket->TxTokenOffset = OFFSET_OF ( EFI_IP4_COMPLETION_TOKEN, Packet.TxData );

  //
  //  Save the cancel, receive and transmit addresses
  //
  pPort->pfnConfigure = (PFN_NET_CONFIGURE)pPort->pProtocol.IPv4->Configure;
  pPort->pfnRxCancel = (PFN_NET_IO_START)pPort->pProtocol.IPv4->Cancel;
  pPort->pfnRxPoll = (PFN_NET_POLL)pPort->pProtocol.IPv4->Poll;
  pPort->pfnRxStart = (PFN_NET_IO_START)pPort->pProtocol.IPv4->Receive;
  pPort->pfnTxStart = (PFN_NET_IO_START)pPort->pProtocol.IPv4->Transmit;

  //
  //  Set the configuration flags
  //
  pConfig = &pPort->Context.Ip4.ModeData.ConfigData;
  pConfig->AcceptIcmpErrors = FALSE;
  pConfig->AcceptBroadcast = FALSE;
  pConfig->AcceptPromiscuous = FALSE;
  pConfig->TypeOfService = 0;
  pConfig->TimeToLive = 255;
  pConfig->DoNotFragment = FALSE;
  pConfig->RawData = FALSE;
  pConfig->ReceiveTimeout = 0;
  pConfig->TransmitTimeout = 0;

  //
  //  Set the default protocol
  //
  pConfig->DefaultProtocol = (UINT8)pSocket->Protocol;
  pConfig->AcceptAnyProtocol = (BOOLEAN)( 0 == pConfig->DefaultProtocol );
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
  specific receive operation to support SOCK_RAW sockets.

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
EslIp4Receive (
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
  size_t HeaderBytes;
  size_t LengthInBytes;
  struct sockaddr_in * pRemoteAddress;
  EFI_IP4_RECEIVE_DATA * pRxData;

  DBG_ENTER ( );

  //
  //  Return the remote system address if requested
  //
  pRxData = pPacket->Op.Ip4Rx.pRxData;
  if ( NULL != pAddress ) {
    //
    //  Build the remote address
    //
    DEBUG (( DEBUG_RX,
              "Getting packet remote address: %d.%d.%d.%d\r\n",
              pRxData->Header->SourceAddress.Addr[0],
              pRxData->Header->SourceAddress.Addr[1],
              pRxData->Header->SourceAddress.Addr[2],
              pRxData->Header->SourceAddress.Addr[3]));
    pRemoteAddress = (struct sockaddr_in *)pAddress;
    CopyMem ( &pRemoteAddress->sin_addr,
              &pRxData->Header->SourceAddress.Addr[0],
              sizeof ( pRemoteAddress->sin_addr ));
  }

  //
  //  Copy the IP header
  //
  HeaderBytes = pRxData->HeaderLength;
  if ( HeaderBytes > BufferLength ) {
    HeaderBytes = BufferLength;
  }
  DEBUG (( DEBUG_RX,
            "0x%08x --> 0x%08x: Copy header 0x%08x bytes\r\n",
            pRxData->Header,
            pBuffer,
            HeaderBytes ));
  CopyMem ( pBuffer, pRxData->Header, HeaderBytes );
  pBuffer += HeaderBytes;
  LengthInBytes = HeaderBytes;

  //
  //  Copy the received data
  //
  if ( 0 < ( BufferLength - LengthInBytes )) {
    pBuffer = EslSocketCopyFragmentedBuffer ( pRxData->FragmentCount,
                                              &pRxData->FragmentTable[0],
                                              BufferLength - LengthInBytes,
                                              pBuffer,
                                              &DataBytes );
    LengthInBytes += DataBytes;
  }

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
              LengthInBytes ));

    //
    //  Account for any discarded data
    //
    *pSkipBytes = pRxData->HeaderLength + pRxData->DataLength - LengthInBytes;
  }

  //
  //  Return the data length and the buffer address
  //
  *pDataLength = LengthInBytes;
  DBG_EXIT_HEX ( pBuffer );
  return pBuffer;
}


/**
  Get the remote socket address

  This routine returns the address of the remote connection point
  associated with the SOCK_RAW socket.

  This routine is called by ::EslSocketGetPeerAddress to detemine
  the IPv4 address associated with the network adapter.

  @param [in] pPort       Address of an ::ESL_PORT structure.

  @param [out] pAddress   Network address to receive the remote system address

**/
VOID
EslIp4RemoteAddressGet (
  IN ESL_PORT * pPort,
  OUT struct sockaddr * pAddress
  )
{
  struct sockaddr_in * pRemoteAddress;
  ESL_IP4_CONTEXT * pIp4;

  DBG_ENTER ( );

  //
  //  Return the remote address
  //
  pIp4 = &pPort->Context.Ip4;
  pRemoteAddress = (struct sockaddr_in *)pAddress;
  pRemoteAddress->sin_family = AF_INET;
  CopyMem ( &pRemoteAddress->sin_addr,
            &pIp4->DestinationAddress.Addr[0],
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
EslIp4RemoteAddressSet (
  IN ESL_PORT * pPort,
  IN CONST struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  )
{
  ESL_IP4_CONTEXT * pIp4;
  CONST struct sockaddr_in * pRemoteAddress;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Set the remote address
  //
  pIp4 = &pPort->Context.Ip4;
  pRemoteAddress = (struct sockaddr_in *)pSockAddr;
  pIp4->DestinationAddress.Addr[0] = (UINT8)( pRemoteAddress->sin_addr.s_addr );
  pIp4->DestinationAddress.Addr[1] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 8 );
  pIp4->DestinationAddress.Addr[2] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 16 );
  pIp4->DestinationAddress.Addr[3] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 24 );
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

  This routine keeps the IPv4 driver's buffer and queues it in
  in FIFO order to the data queue.  The IP4 driver's buffer will
  be returned by either ::EslIp4Receive or ::EslSocketPortCloseTxDone.
  See the \ref ReceiveEngine section.

  This routine is called by the IPv4 driver when data is
  received.

  @param [in] Event     The receive completion event

  @param [in] pIo       The address of an ::ESL_IO_MGMT structure

**/
VOID
EslIp4RxComplete (
  IN EFI_EVENT Event,
  IN ESL_IO_MGMT * pIo
  )
{
  size_t LengthInBytes;
  ESL_PACKET * pPacket;
  EFI_IP4_RECEIVE_DATA * pRxData;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Get the operation status.
  //
  Status = pIo->Token.Ip4Rx.Status;

  //
  //  Get the packet length
  //
  pRxData = pIo->Token.Ip4Rx.Packet.RxData;
  LengthInBytes = pRxData->HeaderLength + pRxData->DataLength;

  //{{
  //      +--------------------+   +----------------------+
  //      | ESL_IO_MGMT        |   |      Data Buffer     |
  //      |                    |   |     (Driver owned)   |
  //      |    +---------------+   +----------------------+
  //      |    | Token         |               ^
  //      |    |      Rx Event |               |
  //      |    |               |   +----------------------+
  //      |    |        RxData --> | EFI_IP4_RECEIVE_DATA |
  //      +----+---------------+   |    (Driver owned)    |
  //                               +----------------------+
  //      +--------------------+               ^
  //      | ESL_PACKET         |               .
  //      |                    |               .
  //      |    +---------------+               .
  //      |    |       pRxData --> NULL  .......
  //      +----+---------------+
  //
  //
  //  Save the data in the packet
  //}}
  pPacket = pIo->pPacket;
  pPacket->Op.Ip4Rx.pRxData = pRxData;

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
  This routine calls the ::EslSocketBind and configuration routines
  if they were not already called.  After the port is configured,
  the \ref ReceiveEngine is started.

  This routine is called by EslSocketIsConfigured to verify
  that the socket is configured.

  @param [in] pSocket         Address of an ::ESL_SOCKET structure

  @retval EFI_SUCCESS - The port is connected
  @retval EFI_NOT_STARTED - The port is not connected

 **/
 EFI_STATUS
 EslIp4SocketIsConfigured (
  IN ESL_SOCKET * pSocket
  )
{
  UINTN Index;
  ESL_PORT * pPort;
  ESL_PORT * pNextPort;
  ESL_IP4_CONTEXT * pIp4;
  EFI_IP4_PROTOCOL * pIp4Protocol;
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
      //  Update the raw setting
      //
      pIp4 = &pPort->Context.Ip4;
      if ( pSocket->bIncludeHeader ) {
        //
        //  IP header will be included with the data on transmit
        //
        pIp4->ModeData.ConfigData.RawData = TRUE;
      }

      //
      //  Attempt to configure the port
      //
      pNextPort = pPort->pLinkSocket;
      pIp4Protocol = pPort->pProtocol.IPv4;
      DEBUG (( DEBUG_TX,
                "0x%08x: pPort Configuring for %d.%d.%d.%d --> %d.%d.%d.%d\r\n",
                          pPort,
                          pIp4->ModeData.ConfigData.StationAddress.Addr[0],
                          pIp4->ModeData.ConfigData.StationAddress.Addr[1],
                          pIp4->ModeData.ConfigData.StationAddress.Addr[2],
                          pIp4->ModeData.ConfigData.StationAddress.Addr[3],
                          pIp4->DestinationAddress.Addr[0],
                          pIp4->DestinationAddress.Addr[1],
                          pIp4->DestinationAddress.Addr[2],
                          pIp4->DestinationAddress.Addr[3]));
      Status = pIp4Protocol->Configure ( pIp4Protocol,
                                          &pIp4->ModeData.ConfigData );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Update the configuration data
        //
        Status = pIp4Protocol->GetModeData ( pIp4Protocol,
                                             &pIp4->ModeData,
                                             NULL,
                                             NULL );
      }
      if ( EFI_ERROR ( Status )) {
        if ( !pSocket->bConfigured ) {
          DEBUG (( DEBUG_LISTEN,
                    "ERROR - Failed to configure the Ip4 port, Status: %r\r\n",
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
                  "0x%08x: pPort Configured for %d.%d.%d.%d --> %d.%d.%d.%d\r\n",
                  pPort,
                  pIp4->ModeData.ConfigData.StationAddress.Addr[0],
                  pIp4->ModeData.ConfigData.StationAddress.Addr[1],
                  pIp4->ModeData.ConfigData.StationAddress.Addr[2],
                  pIp4->ModeData.ConfigData.StationAddress.Addr[3],
                  pIp4->DestinationAddress.Addr[0],
                  pIp4->DestinationAddress.Addr[1],
                  pIp4->DestinationAddress.Addr[2],
                  pIp4->DestinationAddress.Addr[3]));
        DEBUG (( DEBUG_TX,
                  "Subnet Mask: %d.%d.%d.%d\r\n",
                  pIp4->ModeData.ConfigData.SubnetMask.Addr[0],
                  pIp4->ModeData.ConfigData.SubnetMask.Addr[1],
                  pIp4->ModeData.ConfigData.SubnetMask.Addr[2],
                  pIp4->ModeData.ConfigData.SubnetMask.Addr[3]));
        DEBUG (( DEBUG_TX,
                  "Route Count: %d\r\n",
                  pIp4->ModeData.RouteCount ));
        for ( Index = 0; pIp4->ModeData.RouteCount > Index; Index++ ) {
          if ( 0 == Index ) {
            DEBUG (( DEBUG_TX, "Route Table:\r\n" ));
          }
          DEBUG (( DEBUG_TX,
                    "%5d: %d.%d.%d.%d, %d.%d.%d.%d ==> %d.%d.%d.%d\r\n",
                    Index,
                    pIp4->ModeData.RouteTable[Index].SubnetAddress.Addr[0],
                    pIp4->ModeData.RouteTable[Index].SubnetAddress.Addr[1],
                    pIp4->ModeData.RouteTable[Index].SubnetAddress.Addr[2],
                    pIp4->ModeData.RouteTable[Index].SubnetAddress.Addr[3],
                    pIp4->ModeData.RouteTable[Index].SubnetMask.Addr[0],
                    pIp4->ModeData.RouteTable[Index].SubnetMask.Addr[1],
                    pIp4->ModeData.RouteTable[Index].SubnetMask.Addr[2],
                    pIp4->ModeData.RouteTable[Index].SubnetMask.Addr[3],
                    pIp4->ModeData.RouteTable[Index].GatewayAddress.Addr[0],
                    pIp4->ModeData.RouteTable[Index].GatewayAddress.Addr[1],
                    pIp4->ModeData.RouteTable[Index].GatewayAddress.Addr[2],
                    pIp4->ModeData.RouteTable[Index].GatewayAddress.Addr[3]));
        }
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
EslIp4TxBuffer (
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
  ESL_IP4_CONTEXT * pIp4;
  size_t * pTxBytes;
  ESL_IP4_TX_DATA * pTxData;
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
        pIp4 = &pPort->Context.Ip4;

        //
        //  Attempt to allocate the packet
        //
        Status = EslSocketPacketAllocate ( &pPacket,
                                           sizeof ( pPacket->Op.Ip4Tx )
                                           - sizeof ( pPacket->Op.Ip4Tx.Buffer )
                                           + BufferLength,
                                           0,
                                           DEBUG_TX );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Initialize the transmit operation
          //
          pTxData = &pPacket->Op.Ip4Tx;
          pTxData->TxData.DestinationAddress.Addr[0] = pIp4->DestinationAddress.Addr[0];
          pTxData->TxData.DestinationAddress.Addr[1] = pIp4->DestinationAddress.Addr[1];
          pTxData->TxData.DestinationAddress.Addr[2] = pIp4->DestinationAddress.Addr[2];
          pTxData->TxData.DestinationAddress.Addr[3] = pIp4->DestinationAddress.Addr[3];
          pTxData->TxData.OverrideData = NULL;
          pTxData->TxData.OptionsLength = 0;
          pTxData->TxData.OptionsBuffer = NULL;
          pTxData->TxData.TotalDataLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentCount = 1;
          pTxData->TxData.FragmentTable[0].FragmentLength = (UINT32) BufferLength;
          pTxData->TxData.FragmentTable[0].FragmentBuffer = &pPacket->Op.Ip4Tx.Buffer[0];

          //
          //  Set the remote system address if necessary
          //
          if ( NULL != pAddress ) {
            pRemoteAddress = (const struct sockaddr_in *)pAddress;
            pTxData->Override.SourceAddress.Addr[0] = pIp4->ModeData.ConfigData.StationAddress.Addr[0];
            pTxData->Override.SourceAddress.Addr[1] = pIp4->ModeData.ConfigData.StationAddress.Addr[1];
            pTxData->Override.SourceAddress.Addr[2] = pIp4->ModeData.ConfigData.StationAddress.Addr[2];
            pTxData->Override.SourceAddress.Addr[3] = pIp4->ModeData.ConfigData.StationAddress.Addr[3];
            pTxData->TxData.DestinationAddress.Addr[0] = (UINT8)pRemoteAddress->sin_addr.s_addr;
            pTxData->TxData.DestinationAddress.Addr[1] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 8 );
            pTxData->TxData.DestinationAddress.Addr[2] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 16 );
            pTxData->TxData.DestinationAddress.Addr[3] = (UINT8)( pRemoteAddress->sin_addr.s_addr >> 24 );
            pTxData->Override.GatewayAddress.Addr[0] = 0;
            pTxData->Override.GatewayAddress.Addr[1] = 0;
            pTxData->Override.GatewayAddress.Addr[2] = 0;
            pTxData->Override.GatewayAddress.Addr[3] = 0;
            pTxData->Override.Protocol = (UINT8)pSocket->Protocol;
            pTxData->Override.TypeOfService = 0;
            pTxData->Override.TimeToLive = 255;
            pTxData->Override.DoNotFragment = FALSE;

            //
            //  Use the remote system address when sending this packet
            //
            pTxData->TxData.OverrideData = &pTxData->Override;
          }

          //
          //  Copy the data into the buffer
          //
          CopyMem ( &pPacket->Op.Ip4Tx.Buffer[0],
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
                    "Send %d bytes from 0x%08x, %d.%d.%d.%d --> %d.%d.%d.%d\r\n",
                    BufferLength,
                    pBuffer,
                    pIp4->ModeData.ConfigData.StationAddress.Addr[0],
                    pIp4->ModeData.ConfigData.StationAddress.Addr[1],
                    pIp4->ModeData.ConfigData.StationAddress.Addr[2],
                    pIp4->ModeData.ConfigData.StationAddress.Addr[3],
                    pTxData->TxData.DestinationAddress.Addr[0],
                    pTxData->TxData.DestinationAddress.Addr[1],
                    pTxData->TxData.DestinationAddress.Addr[2],
                    pTxData->TxData.DestinationAddress.Addr[3]));

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

  This routine is called by the IPv4 network layer when a data
  transmit request completes.

  @param [in] Event     The normal transmit completion event

  @param [in] pIo       The address of an ::ESL_IO_MGMT structure

**/
VOID
EslIp4TxComplete (
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
  LengthInBytes = pPacket->Op.Ip4Tx.TxData.TotalDataLength;
  pSocket->TxBytes -= LengthInBytes;
  Status = pIo->Token.Ip4Tx.Status;

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
                        "Raw ",
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
EslIp4VerifyLocalIpAddress (
  IN ESL_PORT * pPort,
  IN EFI_IP4_CONFIG_DATA * pConfigData
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
    // Get the interface information size.
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
  code that supports SOCK_RAW sockets over IPv4.
**/
CONST ESL_PROTOCOL_API cEslIp4Api = {
  "IPv4",
    IPPROTO_IP,
  OFFSET_OF ( ESL_PORT, Context.Ip4.ModeData.ConfigData ),
  OFFSET_OF ( ESL_LAYER, pIp4List ),
  OFFSET_OF ( struct sockaddr_in, sin_zero ),
  sizeof ( struct sockaddr_in ),
  AF_INET,
  sizeof (((ESL_PACKET *)0 )->Op.Ip4Rx ),
  sizeof (((ESL_PACKET *)0 )->Op.Ip4Rx ),
  OFFSET_OF ( ESL_IO_MGMT, Token.Ip4Rx.Packet.RxData ),
  FALSE,
  EADDRNOTAVAIL,
  NULL,   //  Accept
  NULL,   //  ConnectPoll
  NULL,   //  ConnectStart
  EslIp4SocketIsConfigured,
  EslIp4LocalAddressGet,
  EslIp4LocalAddressSet,
  NULL,   //  Listen
  EslIp4OptionGet,
  EslIp4OptionSet,
  EslIp4PacketFree,
  EslIp4PortAllocate,
  NULL,   //  PortClose
  NULL,   //  PortCloseOp
  TRUE,
  EslIp4Receive,
  EslIp4RemoteAddressGet,
  EslIp4RemoteAddressSet,
  EslIp4RxComplete,
  NULL,   //  RxStart
  EslIp4TxBuffer,
  EslIp4TxComplete,
  NULL,   //  TxOobComplete
  (PFN_API_VERIFY_LOCAL_IP_ADDRESS)EslIp4VerifyLocalIpAddress
};
