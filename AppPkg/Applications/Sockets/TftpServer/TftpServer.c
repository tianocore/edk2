/** @file
  This is a simple TFTP server application

  Copyright (c) 2011, 2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <TftpServer.h>

TSDT_TFTP_SERVER mTftpServer;       ///<  TFTP server's control structure
volatile BOOLEAN mbTftpServerExit;  ///<  Set TRUE to cause TFTP server to exit


/**
  Read file data into a buffer

  @param [in] pContext    Connection context structure address

  @retval TRUE if a read error occurred

**/
BOOLEAN
BufferFill (
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  BOOLEAN bReadError;
  size_t BytesRead;
  UINT64 LengthInBytes;

  DBG_ENTER ( );

  //
  //  Use break instead of goto
  //
  bReadError = FALSE;
  for ( ; ; ) {
    //
    //  Determine if there is any work to do
    //
    LengthInBytes = DIM ( pContext->FileData ) >> 1;
    if (( pContext->ValidBytes > LengthInBytes )
      || ( 0 == pContext->BytesRemaining )) {
      break;
    }

    //
    //  Determine the number of bytes to read
    //
    if ( LengthInBytes > pContext->BytesRemaining ) {
      LengthInBytes = pContext->BytesRemaining;
    }

    //
    //  Read in the next portion of the file
    //
    BytesRead = fread ( pContext->pFill,
                        1,
                        (size_t)LengthInBytes,
                        pContext->File );
    if ( -1 == BytesRead ) {
      bReadError = TRUE;
      break;
    }

    //
    //  Account for the file data read
    //
    pContext->BytesRemaining -= BytesRead;
    pContext->ValidBytes += BytesRead;
    DEBUG (( DEBUG_FILE_BUFFER,
              "0x%08x: Buffer filled with %Ld bytes, %Ld bytes ramaining\r\n",
              pContext->pFill,
              BytesRead,
              pContext->BytesRemaining ));

    //
    //  Set the next buffer location
    //
    pContext->pFill += BytesRead;
    if ( pContext->pEnd <= pContext->pFill ) {
      pContext->pFill = &pContext->FileData[ 0 ];
    }

    //
    //  Verify that the end of the buffer is reached
    //
    ASSERT ( 0 == ( DIM ( pContext->FileData ) & 1 ));
    break;
  }

  //
  //  Return the read status
  //
  DBG_EXIT ( );
  return bReadError;
}


/**
  Add a connection context to the list of connection contexts.

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] SocketFd      Socket file descriptor

  @retval Context structure address, NULL if allocation fails

**/
TSDT_CONNECTION_CONTEXT *
ContextAdd (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN int SocketFd
  )
{
  TSDT_CONNECTION_CONTEXT * pContext;
  TFTP_PACKET * pEnd;
  TFTP_PACKET * pPacket;

  DBG_ENTER ( );

  //
  //  Allocate a new context
  //
  pContext = (TSDT_CONNECTION_CONTEXT *)AllocateZeroPool ( sizeof ( *pContext ));
  if ( NULL != pContext ) {
    //
    //  Initialize the context
    //
    pContext->SocketFd = SocketFd;
    CopyMem ( &pContext->RemoteAddress,
              &pTftpServer->RemoteAddress,
              sizeof ( pContext->RemoteAddress ));
    pContext->BlockSize = 512;

    //
    //  Buffer management
    //
    pContext->pFill = &pContext->FileData[ 0 ];
    pContext->pEnd = &pContext->FileData[ sizeof ( pContext->FileData )];
    pContext->pBuffer = pContext->pFill;

    //
    //  Window management
    //
    pContext->MaxTimeout = MultU64x32 ( PcdGet32 ( Tftp_MaxTimeoutInSec ),
                                        2 * 1000 * 1000 * 1000 );
    pContext->Rtt2x = pContext->MaxTimeout;
    pContext->WindowSize = MAX_PACKETS;
    WindowTimeout ( pContext );

    //
    //  Place the packets on the free list
    //
    pPacket = &pContext->Tx[ 0 ];
    pEnd = &pPacket[ DIM ( pContext->Tx )];
    while ( pEnd > pPacket ) {
      PacketFree ( pContext, pPacket );
      pPacket += 1;
    }

    //
    //  Display the new context
    //
    if ( AF_INET == pTftpServer->RemoteAddress.v4.sin_family ) {
      DEBUG (( DEBUG_PORT_WORK,
                "0x%08x: Context for %d.%d.%d.%d:%d\r\n",
                pContext,
                (UINT8)pTftpServer->RemoteAddress.v4.sin_addr.s_addr,
                (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 8 ),
                (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 16 ),
                (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 24 ),
                htons ( pTftpServer->RemoteAddress.v4.sin_port )));
    }
    else {
      DEBUG (( DEBUG_PORT_WORK,
                "0x%08x: Context for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                pContext,
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 0 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 1 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 2 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 3 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 4 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 5 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 6 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 7 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 8 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 9 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 10 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 11 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 12 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 13 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 14 ],
                pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 15 ],
                htons ( pTftpServer->RemoteAddress.v6.sin6_port )));
    }

    //
    //  Add the context to the context list
    //
    pContext->pNext = pTftpServer->pContextList;
    pTftpServer->pContextList = pContext;
  }

  //
  //  Return the connection context
  //
  DBG_EXIT_STATUS ( pContext );
  return pContext;
}


/**
  Locate a remote connection context.

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pIpAddress    The start of the remote IP address in network order
  @param [in] Port          The remote port number

  @retval Context structure address, NULL if not found

**/
TSDT_CONNECTION_CONTEXT *
ContextFind (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  TSDT_CONNECTION_CONTEXT * pContext;

  DBG_ENTER ( );

  //
  //  Walk the list of connection contexts
  //
  pContext = pTftpServer->pContextList;
  while ( NULL != pContext ) {
    //
    //  Attempt to locate the remote network connection
    //
    if ( 0 == memcmp ( &pTftpServer->RemoteAddress,
                       &pContext->RemoteAddress,
                       pTftpServer->RemoteAddress.v6.sin6_len )) {
      //
      //  The connection was found
      //
      DEBUG (( DEBUG_TFTP_REQUEST,
                "0x%08x: pContext found\r\n",
                pContext ));
      break;
    }

    //
    //  Set the next context
    //
    pContext = pContext->pNext;
  }

  //
  //  Return the connection context structure address
  //
  DBG_EXIT_HEX ( pContext );
  return pContext;
}


/**
  Remove a context from the list.

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext      Address of a ::TSDT_CONNECTION_CONTEXT structure

**/
VOID
ContextRemove (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  TSDT_CONNECTION_CONTEXT * pNextContext;
  TSDT_CONNECTION_CONTEXT * pPreviousContext;

  DBG_ENTER ( );

  //
  //  Attempt to locate the context in the list
  //
  pPreviousContext = NULL;
  pNextContext = pTftpServer->pContextList;
  while ( NULL != pNextContext ) {
    //
    //  Determine if the context was found
    //
    if ( pNextContext == pContext ) {
      //
      //  Remove the context from the list
      //
      if ( NULL == pPreviousContext ) {
        pTftpServer->pContextList = pContext->pNext;
      }
      else {
        pPreviousContext->pNext = pContext->pNext;
      }
      break;
    }

    //
    //  Set the next context
    //
    pPreviousContext = pNextContext;
    pNextContext = pNextContext->pNext;
  }

  //
  //  Determine if the context was found
  //
  if ( NULL != pContext ) {
    //
    //  Return the resources
    //
    gBS->FreePool ( pContext );
  }

  DBG_EXIT ( );
}


/**
  Queue data packets for transmission

  @param [in] pContext    Connection context structure address

  @retval TRUE if a read error occurred

**/
BOOLEAN
PacketFill (
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  BOOLEAN bReadError;
  UINT64 LengthInBytes;
  UINT8 * pBuffer;
  TFTP_PACKET * pPacket;

  DBG_ENTER ( );

  //
  //  Use break instead of goto
  //
  bReadError = FALSE;
  for ( ; ; ) {
    //
    //  Fill the buffer if necessary
    //
    bReadError = BufferFill ( pContext );
    if ( bReadError ) {
      //
      //  File access mode not supported
      //
      DEBUG (( DEBUG_ERROR | DEBUG_TFTP_REQUEST,
                "ERROR - File read failure!\r\n" ));

      //
      //  Tell the client of the error
      //
      SendError ( pContext,
                  TFTP_ERROR_SEE_MSG,
                  (UINT8 *)"Read failure" );
      break;
    }

    //
    //  Determine if any packets can be filled
    //
    if ( pContext->bEofSent
      || ( NULL == pContext->pFreeList )) {
      //
      //  All of the packets are filled
      //
      break;
    }

    //
    //  Set the TFTP opcode and block number
    //
    pPacket = PacketGet ( pContext );
    pBuffer = &pPacket->TxBuffer[ 0 ];
    *pBuffer++ = 0;
    *pBuffer++ = TFTP_OP_DATA;
    *pBuffer++ = (UINT8)( pContext->BlockNumber >> 8 );
    *pBuffer++ = (UINT8)pContext->BlockNumber;

    //
    //  Determine how much data needs to be sent
    //
    LengthInBytes = pContext->BlockSize;
    if (( pContext->BytesToSend < TFTP_MAX_BLOCK_SIZE )
      && ( LengthInBytes > pContext->BytesToSend )) {
      LengthInBytes = pContext->BytesToSend;
      pContext->bEofSent = TRUE;
    }
    DEBUG (( DEBUG_TX_PACKET,
              "0x%08x: Packet, Block %d filled with %d bytes\r\n",
              pPacket,
              pContext->BlockNumber,
              (UINT32)LengthInBytes ));
    
    //
    //  Copy the file data into the packet
    //
    pPacket->TxBytes = (ssize_t)( 2 + 2 + LengthInBytes );
    if ( 0 < LengthInBytes ) {
      CopyMem ( pBuffer,
                pContext->pBuffer,
                (UINTN)LengthInBytes );
      DEBUG (( DEBUG_FILE_BUFFER,
                "0x%08x: Buffer consumed %d bytes of file data\r\n",
                pContext->pBuffer,
                LengthInBytes ));

      //
      //  Account for the file data consumed
      //
      pContext->ValidBytes -= LengthInBytes;
      pContext->BytesToSend -= LengthInBytes;
      pContext->pBuffer += LengthInBytes;
      if ( pContext->pEnd <= pContext->pBuffer ) {
        pContext->pBuffer = &pContext->FileData[ 0 ];
      }
    }
    
    //
    //  Queue the packet for transmission
    //
    PacketQueue ( pContext, pPacket );
  }

  //
  //  Return the read status
  //
  DBG_EXIT ( );
  return bReadError;
}


/**
  Free the packet

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

**/
VOID
PacketFree(
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  )
{
  DBG_ENTER ( );

  //
  //  Don't free the error packet
  //
  if ( pPacket != &pContext->ErrorPacket ) {
    //
    //  Place the packet on the free list
    //
    pPacket->pNext = pContext->pFreeList;
    pContext->pFreeList = pPacket;
    DEBUG (( DEBUG_TX_PACKET,
              "0x%08x: Packet queued to free list\r\n",
              pPacket ));
  }

  DBG_EXIT ( );
}


/**
  Get a packet from the free list for transmission

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure

  @retval Address of a ::TFTP_PACKET structure

**/
TFTP_PACKET *
PacketGet (
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  TFTP_PACKET * pPacket;

  DBG_ENTER ( );

  //
  //  Get the next packet from the free list
  //
  pPacket = pContext->pFreeList;
  if ( NULL != pPacket ) {
    pContext->pFreeList = pPacket->pNext;
    pPacket->RetryCount = 0;
    DEBUG (( DEBUG_TX_PACKET,
              "0x%08x: Packet removed from free list\r\n",
              pPacket ));
  }

  //
  //  Return the packet
  //
  DBG_EXIT_HEX ( pPacket );
  return pPacket;
}


/**
  Queue the packet for transmission

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

  @retval TRUE if a transmission error has occurred

**/
BOOLEAN
PacketQueue (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  )
{
  BOOLEAN bTransmitError;
  TFTP_PACKET * pTail;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Account for this data block
  //
  pPacket->BlockNumber = pContext->BlockNumber;
  pContext->BlockNumber += 1;

  //
  //  Queue the packet for transmission
  //
  pTail = pContext->pTxTail;
  if ( NULL == pTail ) {
    pContext->pTxHead = pPacket;
  }
  else {
    pTail->pNext = pPacket;
  }
  pContext->pTxTail = pPacket;
  pPacket->pNext = NULL;
  DEBUG (( DEBUG_TX_PACKET,
            "0x%08x: Packet queued to TX list\r\n",
            pPacket ));

  //
  //  Start the transmission if necessary
  //
  bTransmitError = FALSE;
  if ( pContext->PacketsInWindow < pContext->WindowSize ) {
    Status = PacketTx ( pContext, pPacket );
    bTransmitError = (BOOLEAN)( EFI_ERROR ( Status ));
  }

  //
  //  Return the transmit status
  //
  DBG_EXIT_TF ( bTransmitError );
  return bTransmitError;
}


/**
  Remove a packet from the transmit queue

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure

**/
TFTP_PACKET *
PacketRemove(
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  TFTP_PACKET * pNext;
  TFTP_PACKET * pPacket;

  DBG_ENTER ( );

  //
  //  Remove a packet from the transmit queue
  //
  //
  pPacket = pContext->pTxHead;
  if ( NULL != pPacket ) {
    pNext = pPacket->pNext;
    pContext->pTxHead = pNext;
    if ( NULL == pNext ) {
      pContext->pTxTail = NULL;
    }
    DEBUG (( DEBUG_TX_PACKET,
              "0x%08x: Packet removed from TX list\r\n",
              pPacket ));

    //
    //  Remove this packet from the window
    //
    pContext->PacketsInWindow -= 1;
  }

  //
  //  Return the packet
  //
  DBG_EXIT_HEX ( pPacket );
  return pPacket;
}


/**
  Transmit the packet

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
PacketTx (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  )
{
  ssize_t LengthInBytes;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Determine if this packet should be transmitted
  //
  if ( PcdGet32 ( Tftp_MaxRetry ) >= pPacket->RetryCount ) {
    pPacket->RetryCount += 1;

    //
    //  Display the operation
    //
    DEBUG (( DEBUG_TX_PACKET,
              "0x%08x: Packet transmiting\r\n",
              pPacket ));
    DEBUG (( DEBUG_TX,
              "0x%08x: pContext sending 0x%08x bytes\r\n",
              pContext,
              pPacket->TxBytes ));

    //
    //  Keep track of when the packet was transmitted
    //
    if ( PcdGetBool ( Tftp_HighSpeed )) {
      pPacket->TxTime = GetPerformanceCounter ( );
    }

    //
    //  Send the TFTP packet
    //
    pContext->PacketsInWindow += 1;
    LengthInBytes = sendto ( pContext->SocketFd,
                             &pPacket->TxBuffer[ 0 ],
                             pPacket->TxBytes,
                             0,
                             (struct sockaddr *)&pContext->RemoteAddress,
                             pContext->RemoteAddress.sin6_len );
    if ( -1 == LengthInBytes ) {
      DEBUG (( DEBUG_ERROR | DEBUG_TX,
                "ERROR - Transmit failure, errno: 0x%08x\r\n",
                errno ));
      pContext->PacketsInWindow -= 1;
      Status = EFI_DEVICE_ERROR;
    }
  }
  else {
    //
    //  Too many retries
    //
    Status = EFI_NO_RESPONSE;
    DEBUG (( DEBUG_WARN | DEBUG_WINDOW,
              "WARNING - No response from TFTP client\r\n" ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Process the work for the sockets.

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pIndex        Address of an index into the pollfd array

**/
VOID
PortWork (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN int * pIndex
  )
{
  int Index;
  TSDT_CONNECTION_CONTEXT * pContext;
  struct pollfd * pTftpPort;
  socklen_t RemoteAddressLength;
  int revents;

  DBG_ENTER ( );

  //
  //  Locate the port
  //
  Index = *pIndex;
  if ( -1 != Index ) {
    pTftpPort = &pTftpServer->TftpPort[ *pIndex ];

    //
    //  Handle input events
    //
    revents = pTftpPort->revents;
    pTftpPort->revents = 0;
    if ( 0 != ( revents & POLLRDNORM )) {
      //
      //  Receive the message from the remote system
      //
      RemoteAddressLength = sizeof ( pTftpServer->RemoteAddress );
      pTftpServer->RxBytes = recvfrom ( pTftpPort->fd,
                                        &pTftpServer->RxBuffer[ 0 ],
                                        sizeof ( pTftpServer->RxBuffer ),
                                        0,
                                        (struct sockaddr *) &pTftpServer->RemoteAddress,
                                        &RemoteAddressLength );
      if ( -1 != pTftpServer->RxBytes ) {
        if ( PcdGetBool ( Tftp_HighSpeed )) {
          pTftpServer->RxTime = GetPerformanceCounter ( );
        }
        if ( AF_INET == pTftpServer->RemoteAddress.v4.sin_family ) {
          DEBUG (( DEBUG_TFTP_PORT,
                   "Received %d bytes from %d.%d.%d.%d:%d\r\n",
                   pTftpServer->RxBytes,
                   pTftpServer->RemoteAddress.v4.sin_addr.s_addr & 0xff,
                   ( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 8 ) & 0xff,
                   ( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 16 ) & 0xff,
                   ( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 24 ) & 0xff,
                   htons ( pTftpServer->RemoteAddress.v4.sin_port )));
        }
        else {
          DEBUG (( DEBUG_TFTP_PORT,
                   "Received %d bytes from [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                   pTftpServer->RxBytes,
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 0 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 1 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 2 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 3 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 4 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 5 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 6 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 7 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 8 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 9 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 10 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 11 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 12 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 13 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 14 ],
                   pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 15 ],
                   htons ( pTftpServer->RemoteAddress.v6.sin6_port )));
        }

        //
        //  Lookup connection context using the remote system address and port
        //  to determine if an existing connection to this remote
        //  system exists
        //
        pContext = ContextFind ( pTftpServer );

        //
        //  Process the received message
        //
        TftpProcessRequest ( pTftpServer, pContext, pTftpPort->fd );
      }
      else {
        //
        //  Receive error on the TFTP server port
        //  Close the server socket
        //
        DEBUG (( DEBUG_ERROR,
                  "ERROR - Failed receive on TFTP server port, errno: 0x%08x\r\n",
                  errno ));
        revents |= POLLHUP;
      }
    }

    //
    //  Handle the close event
    //
    if ( 0 != ( revents & POLLHUP )) {
      //
      //  Close the port
      //
      close ( pTftpPort->fd );
      pTftpPort->fd = -1;
      *pIndex = -1;
      pTftpServer->Entries -= 1;
      ASSERT ( 0 <= pTftpServer->Entries );
    }
  }

  DBG_EXIT ( );
}


/**
  Build and send an error packet

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] Error       Error number for the packet
  @param [in] pError      Zero terminated error string address

  @retval EFI_SUCCESS     Message processed successfully

**/
EFI_STATUS
SendError (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN UINT16 Error,
  IN UINT8 * pError
  )
{
  UINT8 Character;
  UINT8 * pBuffer;
  TFTP_PACKET * pPacket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Build the error packet
  //
  pPacket = &pContext->ErrorPacket;
  pBuffer = &pPacket->TxBuffer[ 0 ];
  pBuffer[ 0 ] = 0;
  pBuffer[ 1 ] = TFTP_OP_ERROR;
  pBuffer[ 2 ] = (UINT8)( Error >> 8 );
  pBuffer[ 3 ] = (UINT8)Error;

  //
  //  Copy the zero terminated string into the buffer
  //
  pBuffer += 4;
  do {
    Character = *pError++;
    *pBuffer++ = Character;
  } while ( 0 != Character );

  //
  //  Send the error message
  //
  pPacket->TxBytes = pBuffer - &pPacket->TxBuffer[ 0 ];
  Status = PacketTx ( pContext, pPacket );

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Scan the list of sockets and process any pending work

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure

**/
VOID
SocketPoll (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  int FDCount;

  DEBUG (( DEBUG_SOCKET_POLL, "Entering SocketPoll\r\n" ));

  //
  //  Determine if any ports are active
  //
  if ( 0 != pTftpServer->Entries ) {
    FDCount = poll ( &pTftpServer->TftpPort[ 0 ],
                     pTftpServer->Entries,
                     CLIENT_POLL_DELAY );
    if ( 0 < FDCount ) {
      //
      //  Process this port
      //
      PortWork ( pTftpServer, &pTftpServer->Udpv4Index );
      PortWork ( pTftpServer, &pTftpServer->Udpv6Index );
    }
  }

  DEBUG (( DEBUG_SOCKET_POLL, "Exiting SocketPoll\r\n" ));
}


/**
  Process the ACK

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Connection context structure address

  @retval TRUE if the context should be closed

**/
BOOLEAN
TftpAck (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  INTN AckNumber;
  BOOLEAN bCloseContext;
  UINT16 BlockNumber;
  UINT8 * pBuffer;
  TFTP_PACKET * pPacket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use break instead of goto
  //
  bCloseContext = FALSE;
  for ( ; ; ) {
    //
    //  Validate the parameters
    //
    if ( NULL == pContext ) {
      if ( AF_INET == pTftpServer->RemoteAddress.v4.sin_family ) {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                  (UINT8)pTftpServer->RemoteAddress.v4.sin_addr.s_addr,
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 8 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 16 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 24 ),
                  htons ( pTftpServer->RemoteAddress.v4.sin_port )));
      }
      else {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 0 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 1 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 2 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 3 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 4 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 5 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 6 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 7 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 8 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 9 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 10 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 11 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 12 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 13 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 14 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 15 ],
                  htons ( pTftpServer->RemoteAddress.v6.sin6_port )));
      }
      break;
    }

    //
    //  Verify that the ACK was expected
    //
    pPacket = pContext->pTxHead;
    if ( NULL == pPacket ) {
      //
      //  ACK not expected!
      //
      DEBUG (( DEBUG_ERROR,
                "ERROR - Expecting data not ACKs for pContext 0x%08x\r\n",
                pContext ));
      break;
    }

    //
    //  Get the ACKed block number
    //
    pBuffer = &pTftpServer->RxBuffer[ 0 ];
    BlockNumber = HTONS ( *(UINT16 *)&pBuffer[ 2 ]);

    //
    //  Determine if this is the correct ACK
    //
    DEBUG (( DEBUG_TFTP_ACK,
              "ACK for block 0x%04x received\r\n",
              BlockNumber ));
    AckNumber = BlockNumber - pPacket->BlockNumber;
    if (( 0 > AckNumber ) || ( AckNumber >= (INTN)pContext->PacketsInWindow )){
      DEBUG (( DEBUG_WARN | DEBUG_TFTP_ACK,
                "WARNING - Expecting ACK 0x%0x4 not received ACK 0x%08x\r\n",
                pPacket->BlockNumber,
                BlockNumber ));
      break;
    }

    //
    //  Release the ACKed packets
    //
    do {
      //
      //  Remove the packet from the transmit list and window
      //
      pPacket = PacketRemove ( pContext );

      //
      //  Get the block number of this packet
      //
      AckNumber = pPacket->BlockNumber;

      //
      //  Increase the size of the transmit window
      //
      if ( PcdGetBool ( Tftp_HighSpeed )
        && ( AckNumber == BlockNumber )) {
        WindowAck ( pTftpServer, pContext, pPacket );
      }

      //
      //  Free this packet
      //
      PacketFree ( pContext, pPacket );
    } while (( NULL != pContext->pTxHead ) && ( AckNumber != BlockNumber ));

    //
    //  Fill the window with packets
    //
    pPacket = pContext->pTxHead;
    while (( NULL != pPacket )
      && ( pContext->PacketsInWindow < pContext->WindowSize )
      && ( !bCloseContext )) {
      Status = PacketTx ( pContext, pPacket );
      bCloseContext = (BOOLEAN)( EFI_ERROR ( Status ));
      pPacket = pPacket->pNext;
    }
    
    //
    //  Get more packets ready for transmission
    //
    PacketFill ( pContext );

    //
    //  Close the context when the last packet is ACKed
    //
    if ( 0 == pContext->PacketsInWindow ) {
      bCloseContext = TRUE;

      //
      //  Display the bandwidth
      //
      if ( PcdGetBool ( Tftp_Bandwidth )) {
        UINT64 Bandwidth;
        UINT64 DeltaTime;
        UINT64 NanoSeconds;
        UINT32 Value;

        //
        //  Compute the download time
        //
        DeltaTime = GetPerformanceCounter ( );
        if ( pTftpServer->Time2 > pTftpServer->Time1 ) {
          DeltaTime = DeltaTime - pContext->TimeStart;
        }
        else {
          DeltaTime = pContext->TimeStart - DeltaTime;
        }
        NanoSeconds = GetTimeInNanoSecond ( DeltaTime );
        Bandwidth = pContext->LengthInBytes;
        DEBUG (( DEBUG_WINDOW,
                  "File Length %Ld, Transfer Time: %d.%03d Sec\r\n",
                  Bandwidth,
                  DivU64x32 ( NanoSeconds, 1000 * 1000 * 1000 ),
                  ((UINT32)DivU64x32 ( NanoSeconds, 1000 * 1000 )) % 1000 ));

        //
        //  Display the round trip time
        //
        Bandwidth = MultU64x32 ( Bandwidth, 8 * 1000 * 1000 );
        Bandwidth /= NanoSeconds;
        if ( 1000 > Bandwidth ) {
          Value = (UINT32)Bandwidth;
          Print ( L"Bandwidth: %d Kbits/Sec\r\n",
                  Value );
        }
        else if (( 1000 * 1000 ) > Bandwidth ) {
          Value = (UINT32)Bandwidth;
          Print ( L"Bandwidth: %d.%03d Mbits/Sec\r\n",
                  Value / 1000,
                  Value % 1000 );
        }
        else {
          Value = (UINT32)DivU64x32 ( Bandwidth, 1000 );
          Print ( L"Bandwidth: %d.%03d Gbits/Sec\r\n",
                  Value / 1000,
                  Value % 1000 );
        }
      }
    }
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT ( );
  return bCloseContext;
}


/**
  Get the next TFTP option

  @param [in] pOption       Address of a zero terminated option string
  @param [in] pEnd          End of buffer address
  @param [in] ppNextOption  Address to receive the address of the next
                            zero terminated option string

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
TftpOptionGet (
  IN UINT8 * pOption,
  IN UINT8 * pEnd,
  IN UINT8 ** ppNextOption
  )
{
  UINT8 * pNextOption;
  EFI_STATUS Status;

  //
  //  Locate the end of the option
  //
  pNextOption = pOption;
  while (( pEnd > pNextOption ) && ( 0 != *pNextOption )) {
    pNextOption += 1;
  }
  if ( pEnd <= pNextOption ) {
    //
    //  Error - end of buffer reached
    //
    DEBUG (( DEBUG_ERROR | DEBUG_TFTP_REQUEST,
              "ERROR - Option without zero termination received!\r\n" ));
    Status = EFI_INVALID_PARAMETER;
  }
  else {
    //
    //  Zero terminated option found
    //
    pNextOption += 1;

    //
    //  Display the zero terminated ASCII option string
    //
    DEBUG (( DEBUG_TFTP_REQUEST,
              "Option: %a\r\n",
              pOption ));
    Status = EFI_SUCCESS;
  }

  //
  //  Return the next option address
  //
  *ppNextOption = pNextOption;

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Place an option value into the option acknowledgement

  @param [in] pOack     Option acknowledgement address
  @param [in] Value     Value to translate into ASCII decimal

  @return               Option acknowledgement address

**/
UINT8 *
TftpOptionSet (
  IN UINT8 * pOack,
  IN UINT64 Value
  )
{
  UINT64 NextValue;

  //
  //  Determine the next value
  //
  NextValue = Value / 10;

  //
  //  Supress leading zeros
  //
  if ( 0 != NextValue ) {
    pOack = TftpOptionSet ( pOack, NextValue );
  }

  //
  //  Output this digit
  //
  *pOack++ = (UINT8)( Value - ( NextValue * 10 ) + '0' );

  //
  //  Return the next option acknowledgement location
  //
  return pOack;
}


/**
  Process the TFTP request

  @param [in] pContext  Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pOption   Address of the first zero terminated option string
  @param [in] pEnd      End of buffer address

**/
VOID
TftpOptions (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN UINT8 * pOption,
  IN UINT8 * pEnd
  )
{
  UINT8 * pNextOption;
  UINT8 * pOack;
  TFTP_PACKET * pPacket;
  UINT8 * pTemp;
  UINT8 * pValue;
  EFI_STATUS Status;
  INT32 Value;

  //
  //  Get a packet
  //
  pPacket = PacketGet ( pContext );

  //
  //  Start the OACK packet
  //  Let the OACK handle the parsing errors
  //  See http://tools.ietf.org/html/rfc2347
  //
  pOack = &pPacket->TxBuffer[ 0 ];
  *pOack++ = 0;
  *pOack++ = TFTP_OP_OACK;
  pPacket->TxBytes = 2;
  pPacket->BlockNumber = 0;

  //
  //  Walk the list of options
  //
  do {
    //
    //  Get the next option, skip junk at end of message
    //
    Status = TftpOptionGet ( pOption, pEnd, &pNextOption );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Process the option
      //

      //
      //  blksize - See http://tools.ietf.org/html/rfc2348
      //
      pValue = pNextOption;
      if ( 0 == strcasecmp ((char *)pOption, "blksize" )) {
        //
        //  Get the value
        //
        Status = TftpOptionGet ( pValue, pEnd, &pNextOption );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Validate the block size, skip non-numeric block sizes
          //
          Status = TftpOptionValue ( pValue, &Value );
          if ( !EFI_ERROR ( Status )) {
            //
            //  Propose a smaller block size if necessary
            //
            if ( Value > TFTP_MAX_BLOCK_SIZE ) {
              Value = TFTP_MAX_BLOCK_SIZE;
            }

            //
            //  Set the new block size
            //
            pContext->BlockSize = Value;
            DEBUG (( DEBUG_TFTP_REQUEST,
                      "Using block size of %d bytes\r\n",
                      pContext->BlockSize ));

            //
            //  Update the OACK
            //
            pTemp = pOack;
            *pOack++ = 'b';
            *pOack++ = 'l';
            *pOack++ = 'k';
            *pOack++ = 's';
            *pOack++ = 'i';
            *pOack++ = 'z';
            *pOack++ = 'e';
            *pOack++ = 0;
            pOack = TftpOptionSet ( pOack, pContext->BlockSize );
            *pOack++ = 0;
            pPacket->TxBytes += pOack - pTemp;
          }
        }
      }

      //
      //  timeout - See http://tools.ietf.org/html/rfc2349
      //
      else if ( 0 == strcasecmp ((char *)pOption, "timeout" )) {
        //
        //  Get the value
        //
        Status = TftpOptionGet ( pValue, pEnd, &pNextOption );
        if ( !EFI_ERROR ( Status )) {
          Status = TftpOptionValue ( pValue, &Value );
          if ( !EFI_ERROR ( Status )) {
            //
            //  Set the timeout value
            //
            pContext->MaxTimeout = Value;
            DEBUG (( DEBUG_TFTP_REQUEST,
                      "Using timeout of %d seconds\r\n",
                      pContext->MaxTimeout ));

            //
            //  Update the OACK
            //
            pTemp = pOack;
            *pOack++ = 't';
            *pOack++ = 'i';
            *pOack++ = 'm';
            *pOack++ = 'e';
            *pOack++ = 'o';
            *pOack++ = 'u';
            *pOack++ = 't';
            *pOack++ = 0;
            pOack = TftpOptionSet ( pOack, pContext->MaxTimeout );
            *pOack++ = 0;
            pPacket->TxBytes += pOack - pTemp;
          }
        }
      }

      //
      //  tsize - See http://tools.ietf.org/html/rfc2349
      //
      else if ( 0 == strcasecmp ((char *)pOption, "tsize" )) {
        //
        //  Get the value
        //
        Status = TftpOptionGet ( pValue, pEnd, &pNextOption );
        if ( !EFI_ERROR ( Status )) {
          Status = TftpOptionValue ( pValue, &Value );
          if ( !EFI_ERROR ( Status )) {
            //
            //  Return the file size
            //
            DEBUG (( DEBUG_TFTP_REQUEST,
                      "Returning file size of %Ld bytes\r\n",
                      pContext->LengthInBytes ));

            //
            //  Update the OACK
            //
            pTemp = pOack;
            *pOack++ = 't';
            *pOack++ = 's';
            *pOack++ = 'i';
            *pOack++ = 'z';
            *pOack++ = 'e';
            *pOack++ = 0;
            pOack = TftpOptionSet ( pOack, pContext->LengthInBytes );
            *pOack++ = 0;
            pPacket->TxBytes += pOack - pTemp;
          }
        }
      }
      else {
        //
        //  Unknown option - Ignore it
        //
        DEBUG (( DEBUG_WARN | DEBUG_TFTP_REQUEST,
                  "WARNING - Skipping unknown option: %a\r\n",
                  pOption ));
      }
    }

    //
    //  Set the next option
    //
    pOption = pNextOption;
  } while ( pEnd > pOption );

  //
  //  Transmit the OACK if necessary
  //
  if ( 2 < pPacket->TxBytes ) {
    PacketQueue ( pContext, pPacket );
  }
  else {
    PacketFree ( pContext, pPacket );
  }
}


/**
  Process the TFTP request

  @param [in] pOption   Address of the first zero terminated option string
  @param [in] pValue    Address to receive the value

  @retval EFI_SUCCESS   Option translated into a value

**/
EFI_STATUS
TftpOptionValue (
  IN UINT8 * pOption,
  IN INT32 * pValue
  )
{
  UINT8 Digit;
  EFI_STATUS Status;
  INT32 Value;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the characters in the option
  //
  Value = 0;
  while ( 0 != *pOption ) {
    //
    //  Convert the next digit to binary
    //
    Digit = *pOption++;
    if (( '0' <= Digit ) && ( '9' >= Digit )) {
      Value *= 10;
      Value += Digit - '0';
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_TFTP_REQUEST,
                "ERROR - Invalid character '0x%02x' in the value\r\n",
                Digit ));
      Status = EFI_INVALID_PARAMETER;
      break;
    }
  }

  //
  //  Return the value
  //
  *pValue = Value;

  //
  //  Return the conversion status
  //
  return Status;
}


/**
  Process the TFTP request

  @param [in] pTftpServer Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] SocketFd    Socket file descriptor

**/
VOID
TftpProcessRequest (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN int SocketFd
  )
{
  BOOLEAN bCloseContext;
  UINT16 Opcode;

  DBG_ENTER ( );

  //
  //  Get the opcode
  //
  Opcode = HTONS ( *(UINT16 *)&pTftpServer->RxBuffer[ 0 ]);
  DEBUG (( DEBUG_TFTP_REQUEST,
            "TFTP Opcode: 0x%08x\r\n",
            Opcode ));

  //
  //  Validate the parameters
  //
  bCloseContext = FALSE;
  switch ( Opcode ) {
  default:
    DEBUG (( DEBUG_TFTP_REQUEST,
              "ERROR - Unknown TFTP opcode: %d\r\n",
              Opcode ));
    break;

  case TFTP_OP_ACK:
    bCloseContext = TftpAck ( pTftpServer, pContext );
    break;

  case TFTP_OP_READ_REQUEST:
    bCloseContext = TftpRead ( pTftpServer, pContext, SocketFd );
    break;



  
  case TFTP_OP_DATA:
    if ( NULL == pContext ) {
      if ( AF_INET == pTftpServer->RemoteAddress.v4.sin_family ) {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                  (UINT8)pTftpServer->RemoteAddress.v4.sin_addr.s_addr,
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 8 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 16 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 24 ),
                  htons ( pTftpServer->RemoteAddress.v4.sin_port )));
      }
      else {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 0 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 1 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 2 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 3 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 4 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 5 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 6 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 7 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 8 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 9 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 10 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 11 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 12 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 13 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 14 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 15 ],
                  htons ( pTftpServer->RemoteAddress.v6.sin6_port )));
      }
      break;
    }
    if ( 0 != pContext->PacketsInWindow ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Expecting ACKs not data for pContext 0x%08x\r\n",
                pContext ));
      break;
    }
    if ( pTftpServer->RxBytes > (ssize_t)( pContext->BlockSize + 2 + 2 )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Receive data length of %d > %d bytes (maximum block size) for pContext 0x%08x\r\n",
                pTftpServer->RxBytes - 2 - 2,
                pContext->BlockSize,
                pContext ));
      break;
    }
    break;

  case TFTP_OP_ERROR:
    if ( NULL == pContext ) {
      if ( AF_INET == pTftpServer->RemoteAddress.v4.sin_family ) {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                  (UINT8)pTftpServer->RemoteAddress.v4.sin_addr.s_addr,
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 8 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 16 ),
                  (UINT8)( pTftpServer->RemoteAddress.v4.sin_addr.s_addr >> 24 ),
                  htons ( pTftpServer->RemoteAddress.v4.sin_port )));
      }
      else {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - File not open for [%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d\r\n",
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 0 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 1 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 2 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 3 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 4 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 5 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 6 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 7 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 8 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 9 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 10 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 11 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 12 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 13 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 14 ],
                  pTftpServer->RemoteAddress.v6.sin6_addr.__u6_addr.__u6_addr8[ 15 ],
                  htons ( pTftpServer->RemoteAddress.v6.sin6_port )));
      }
    }
    break;
  }

  //
  //  Determine if the context should be closed
  //
  if ( bCloseContext ) {
    ContextRemove ( pTftpServer, pContext );
  }

  DBG_EXIT ( );
}


/**
  Process the read request

  @param [in] pTftpServer Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] SocketFd    Socket file descriptor

  @retval TRUE if the context should be closed

**/
BOOLEAN
TftpRead (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN int SocketFd
  )
{
  BOOLEAN bCloseContext;
  struct stat FileStatus;
  UINT8 * pBuffer;
  UINT8 * pEnd;
  UINT8 * pFileName;
  UINT8 * pMode;
  UINT8 * pOption;
  CHAR8 * pReadMode;
  UINT64 TimeStart;

  DBG_ENTER ( );

  //
  //  Log the receive time
  //
  TimeStart = 0;
  if ( PcdGetBool ( Tftp_Bandwidth )) {
    TimeStart = GetPerformanceCounter ( );
  }

  //
  //  Close the context if necessary
  //
  bCloseContext = FALSE;
  if ( NULL != pContext ) {
    ContextRemove ( pTftpServer, pContext );
  }

  //
  //  Use break instead of goto
  //
  for ( ; ; ) {
    //
    //  Create the connection context
    //
    pContext = ContextAdd ( pTftpServer, SocketFd );
    if ( NULL == pContext ) {
      break;
    }

    //
    //  Set the start time
    //
    if ( PcdGetBool ( Tftp_Bandwidth )) {
      pContext->TimeStart = TimeStart;
    }

    //
    //  Locate the mode
    //
    pBuffer = &pTftpServer->RxBuffer[ 0 ];
    pEnd = &pBuffer[ pTftpServer->RxBytes ];
    pFileName = &pBuffer[ 2 ];
    pMode = pFileName;
    while (( pEnd > pMode ) && ( 0 != *pMode )) {
      pMode += 1;
    }
    if ( pEnd <= pMode ) {
      //
      //  Mode not found
      //
      DEBUG (( DEBUG_ERROR | DEBUG_RX,
                "ERROR - File mode not found\r\n" ));
      //
      //  Tell the client of the error
      //
      SendError ( pContext,
                  TFTP_ERROR_SEE_MSG,
                  (UINT8 *)"File open mode not found" );
      break;
    }
    pMode += 1;
    DEBUG (( DEBUG_TFTP_REQUEST,
              "TFTP - FileName: %a\r\n",
              pFileName ));

    //
    //  Locate the options
    //
    pOption = pMode;
    while (( pEnd > pOption ) && ( 0 != *pOption )) {
      pOption += 1;
    }
    if ( pEnd <= pOption ) {
      //
      //  End of mode not found
      //
      DEBUG (( DEBUG_ERROR | DEBUG_RX,
                "ERROR - File mode not valid\r\n" ));
      //
      //  Tell the client of the error
      //
      SendError ( pContext,
                  TFTP_ERROR_SEE_MSG,
                  (UINT8 *)"File open mode not valid" );
      break;
    }
    pOption += 1;
    DEBUG (( DEBUG_TFTP_REQUEST,
              "TFTP - Mode: %a\r\n",
              pMode ));

    //
    //  Verify the mode is supported
    //
    pReadMode = "r";
    if ( 0 == strcasecmp ((char *)pMode, "octet" )) {
      //
      //  Read the file as binary input
      //
      pReadMode = "rb";
    }

    //
    //  Determine the file length
    //
    pContext->File = fopen ((const char *)pFileName, pReadMode );
    if (( NULL == pContext->File )
        || ( -1 == stat ((const char *)pFileName, &FileStatus ))) {
      //
      //  File not found
      //
      DEBUG (( DEBUG_ERROR | DEBUG_TFTP_REQUEST,
                ( NULL == pContext->File )
                ? "ERROR - File not found!\r\n"
                : "ERROR - Unable to determine file %a size!\r\n",
                pFileName ));

      //
      //  Tell the client of the error
      //
      SendError ( pContext,
                  TFTP_ERROR_NOT_FOUND,
                  (UINT8 *)"File not found" );
      break;
    }
    pContext->LengthInBytes = FileStatus.st_size;
    pContext->BytesRemaining = pContext->LengthInBytes;
    pContext->BytesToSend = pContext->LengthInBytes;

    //
    //  Display the file size
    //
    DEBUG_CODE_BEGIN ( );
    UINT32 Value;

    if ( 1024 > pContext->LengthInBytes ) {
      Value = (UINT32)pContext->LengthInBytes;
      DEBUG (( DEBUG_FILE_BUFFER,
                "%a size: %d Bytes\r\n",
                pFileName,
                Value ));
    }
    else if (( 1024 * 1024 ) > pContext->LengthInBytes ) {
      Value = (UINT32)pContext->LengthInBytes;
      DEBUG (( DEBUG_FILE_BUFFER,
                "%a size: %d.%03d KiBytes (%Ld Bytes)\r\n",
                pFileName,
                Value / 1024,
                (( Value % 1024 ) * 1000 ) / 1024,
                pContext->LengthInBytes ));
    }
    else if (( 1024 * 1024 * 1024 ) > pContext->LengthInBytes ) {
      Value = (UINT32)DivU64x32 ( pContext->LengthInBytes, 1024 );
      DEBUG (( DEBUG_FILE_BUFFER,
                "%a size: %d.%03d MiBytes (%Ld Bytes)\r\n",
                pFileName,
                Value / 1024,
                (( Value % 1024 ) * 1000 ) / 1024,
                pContext->LengthInBytes ));
    }
    else {
      Value = (UINT32)DivU64x32 ( pContext->LengthInBytes, 1024 * 1024 );
      DEBUG (( DEBUG_FILE_BUFFER,
                "%a size: %d.%03d GiBytes (%Ld Bytes)\r\n",
                pFileName,
                Value / 1024,
                (( Value % 1024 ) * 1000 ) / 1024,
                pContext->LengthInBytes ));
    }
    DEBUG_CODE_END ( );

    //
    //  Process the options
    //
    if ( pEnd > pOption ) {
      TftpOptions ( pContext, pOption, pEnd );
    }
    else {
      //
      //  Skip the open ACK
      //
      pContext->BlockNumber = 1;
    }

    //
    //  Send the first packet (OACK or data block)
    //
    bCloseContext = PacketFill ( pContext );
    break;
  }

  //
  //  Return the close status
  //
  DBG_EXIT ( );
  return bCloseContext;
}


/**
  Create the port for the TFTP server

  This routine polls the network layer to create the TFTP port for the
  TFTP server.  More than one attempt may be necessary since it may take
  some time to get the IP address and initialize the upper layers of
  the network stack.

  @param [in] pTftpServer   Address of the ::TSDT_TFTP_SERVER structure
  @param [in] AddressFamily The address family to use for the conection.
  @param [in] pIndex        Address of the index into the port array

**/
VOID
TftpServerSocket (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN sa_family_t AddressFamily,
  IN int * pIndex
  )
{
  int SocketStatus;
  struct pollfd * pTftpPort;
  UINT16 TftpPort;
  union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
  } TftpServerAddress;

  DEBUG (( DEBUG_SERVER_TIMER, "Entering TftpServerListen\r\n" ));

  //
  //  Determine if the socket is already initialized
  //
  if ( -1 == *pIndex ) {
    //
    //  Attempt to create the socket for the TFTP server
    //
    pTftpPort = &pTftpServer->TftpPort[ pTftpServer->Entries ];
    pTftpPort->fd = socket ( AddressFamily,
                             SOCK_DGRAM,
                             IPPROTO_UDP );
    if ( -1 != pTftpPort->fd ) {
      //
      //  Initialize the poll structure
      //
      pTftpPort->events = POLLRDNORM | POLLHUP;
      pTftpPort->revents = 0;

      //
      //  Set the socket address
      //
      TftpPort = 69;
      ZeroMem ( &TftpServerAddress, sizeof ( TftpServerAddress ));
      TftpServerAddress.v4.sin_port = htons ( TftpPort );
      if ( AF_INET == AddressFamily ) {
        TftpServerAddress.v4.sin_len = sizeof ( TftpServerAddress.v4 );
        TftpServerAddress.v4.sin_family = AF_INET;
      }
      else {
        TftpServerAddress.v6.sin6_len = sizeof ( TftpServerAddress.v6 );
        TftpServerAddress.v6.sin6_family = AF_INET6;
      }

      //
      //  Bind the socket to the TFTP port
      //
      SocketStatus = bind ( pTftpPort->fd,
                            (struct sockaddr *) &TftpServerAddress,
                            TftpServerAddress.v6.sin6_len );
      if ( -1 != SocketStatus ) {
        DEBUG (( DEBUG_TFTP_PORT,
                  "0x%08x: Socket bound to port %d\r\n",
                  pTftpPort->fd,
                  TftpPort ));

        //
        //  Account for this connection
        //
        *pIndex = pTftpServer->Entries;
        pTftpServer->Entries += 1;
        ASSERT ( DIM ( pTftpServer->TftpPort ) >= pTftpServer->Entries );
      }

      //
      //  Release the socket if necessary
      //
      if ( -1 == SocketStatus ) {
        close ( pTftpPort->fd );
        pTftpPort->fd = -1;
      }
    }
  }

  DEBUG (( DEBUG_SERVER_TIMER, "Exiting TftpServerListen\r\n" ));
}


/**
  Update the window due to the ACK

  @param [in] pTftpServer Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

**/
VOID
WindowAck (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  )
{
  if ( PcdGetBool ( Tftp_HighSpeed )) {
    UINT64 DeltaTime;
    UINT64 NanoSeconds;

    DBG_ENTER ( );

    //
    //  Compute the round trip time
    //
    if ( pTftpServer->Time2 > pTftpServer->Time1 ) {
      DeltaTime = pTftpServer->RxTime - pPacket->TxTime;
    }
    else {
      DeltaTime = pPacket->TxTime - pTftpServer->RxTime;
    }

    //
    //  Adjust the round trip time
    //
    NanoSeconds = GetTimeInNanoSecond ( DeltaTime );
    DeltaTime = RShiftU64 ( pContext->Rtt2x, ACK_SHIFT );
    pContext->Rtt2x += NanoSeconds + NanoSeconds - DeltaTime;
    if ( pContext->Rtt2x > pContext->MaxTimeout ) {
      pContext->Rtt2x = pContext->MaxTimeout;
    }

    //
    //  Account for the ACK
    //
    if ( pContext->WindowSize < MAX_PACKETS ) {
      pContext->AckCount -= 1;
      if ( 0 == pContext->AckCount ) {
        //
        //  Increase the window
        //
        pContext->WindowSize += 1;

        //
        //  Set the ACK count
        //
        if ( pContext->WindowSize < pContext->Threshold ) {
          pContext->AckCount = pContext->WindowSize * PcdGet32 ( Tftp_AckMultiplier );
        }
        else {
          pContext->AckCount = PcdGet32 ( Tftp_AckLogBase ) << pContext->WindowSize;
        }

        //
        //  Display the round trip time
        //
        DEBUG_CODE_BEGIN ( );
        UINT32 Value;
        
        DeltaTime = RShiftU64 ( pContext->Rtt2x, 1 );
        if ( 1000 > DeltaTime ) {
          DEBUG (( DEBUG_WINDOW,
                    "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %Ld nSec\r\n",
                    pContext->WindowSize,
                    pContext->Threshold,
                    pContext->AckCount,
                    DeltaTime ));
        }
        else if (( 1000 * 1000 ) > DeltaTime ) {
          Value = (UINT32)DeltaTime;
          DEBUG (( DEBUG_WINDOW,
                    "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d uSec\r\n",
                    pContext->WindowSize,
                    pContext->Threshold,
                    pContext->AckCount,
                    Value / 1000,
                    Value % 1000 ));
        }
        else if (( 1000 * 1000 * 1000 ) > DeltaTime ) {
          Value = (UINT32)DivU64x32 ( DeltaTime, 1000 );
          DEBUG (( DEBUG_WINDOW,
                    "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d mSec\r\n",
                    pContext->WindowSize,
                    pContext->Threshold,
                    pContext->AckCount,
                    Value / 1000,
                    Value % 1000 ));
        }
        else {
          Value = (UINT32)DivU64x32 ( DeltaTime, 1000 * 1000 );
          DEBUG (( DEBUG_WINDOW,
                    "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d Sec\r\n",
                    pContext->WindowSize,
                    pContext->Threshold,
                    pContext->AckCount,
                    Value / 1000,
                    Value % 1000 ));
        }
        DEBUG_CODE_END ( );
      }
    }

    DBG_EXIT ( );
  }
}


/**
  A timeout has occurred, close the window

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure

**/
VOID
WindowTimeout (
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  if ( PcdGetBool ( Tftp_HighSpeed )) {
    TFTP_PACKET * pPacket;

    DBG_ENTER ( );

    //
    //  Set the threshold at half the previous window size
    //
    pContext->Threshold = ( pContext->WindowSize + 1 ) >> 1;

    //
    //  Close the transmit window
    //
    pContext->WindowSize = 1;
    pContext->PacketsInWindow = 0;

    //
    //  Double the round trip time
    //
    pContext->Rtt2x = LShiftU64 ( pContext->Rtt2x, 1 );
    if ( pContext->Rtt2x > pContext->MaxTimeout ) {
      pContext->Rtt2x = pContext->MaxTimeout;
    }

    //
    //  Set the ACK count
    //
    if ( pContext->WindowSize < pContext->Threshold ) {
      pContext->AckCount = pContext->WindowSize * PcdGet32 ( Tftp_AckMultiplier );
    }
    else {
      pContext->AckCount = PcdGet32 ( Tftp_AckLogBase ) << pContext->WindowSize;
    }

    //
    //  Display the round trip time
    //
    DEBUG_CODE_BEGIN ( );
    UINT64 DeltaTime;
    UINT32 Value;
    
    DeltaTime = RShiftU64 ( pContext->Rtt2x, 1 );
    if ( 1000 > DeltaTime ) {
      DEBUG (( DEBUG_WINDOW,
                "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %Ld nSec\r\n",
                pContext->WindowSize,
                pContext->Threshold,
                pContext->AckCount,
                DeltaTime ));
    }
    else if (( 1000 * 1000 ) > DeltaTime ) {
      Value = (UINT32)DeltaTime;
      DEBUG (( DEBUG_WINDOW,
                "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d uSec\r\n",
                pContext->WindowSize,
                pContext->Threshold,
                pContext->AckCount,
                Value / 1000,
                Value % 1000 ));
    }
    else if (( 1000 * 1000 * 1000 ) > DeltaTime ) {
      Value = (UINT32)DivU64x32 ( DeltaTime, 1000 );
      DEBUG (( DEBUG_WINDOW,
                "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d mSec\r\n",
                pContext->WindowSize,
                pContext->Threshold,
                pContext->AckCount,
                Value / 1000,
                Value % 1000 ));
    }
    else {
      Value = (UINT32)DivU64x32 ( DeltaTime, 1000 * 1000 );
      DEBUG (( DEBUG_WINDOW,
                "WindowSize: %d, Threshold: %d, AckCount: %4d, RTT: %d.%03d Sec\r\n",
                pContext->WindowSize,
                pContext->Threshold,
                pContext->AckCount,
                Value / 1000,
                Value % 1000 ));
    }
    DEBUG_CODE_END ( );

    //
    //  Retransmit the first packet in the window
    //
    pPacket = pContext->pTxHead;
    if ( NULL != pPacket ) {
      PacketTx ( pContext, pPacket );
    }
    
    DBG_EXIT ( );
  }
}


/**
  Entry point for the TFTP server application.

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
  UINTN Index;
  TSDT_TFTP_SERVER * pTftpServer;
  EFI_STATUS Status;
  UINT64 TriggerTime;

  //
  //  Get the performance counter characteristics
  //
  pTftpServer = &mTftpServer;
  if ( PcdGetBool ( Tftp_HighSpeed )
    || PcdGetBool ( Tftp_Bandwidth )) {
    pTftpServer->ClockFrequency = GetPerformanceCounterProperties ( &pTftpServer->Time1,
                                                                  &pTftpServer->Time2 );
  }

  //
  //  Create a timer event to start TFTP port
  //
  Status = gBS->CreateEvent ( EVT_TIMER,
                              TPL_TFTP_SERVER,
                              NULL,
                              NULL,
                              &pTftpServer->TimerEvent );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Compute the poll interval
    //
    TriggerTime = TFTP_PORT_POLL_DELAY * ( 1000 * 10 );
    Status = gBS->SetTimer ( pTftpServer->TimerEvent,
                             TimerPeriodic,
                             TriggerTime );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_TFTP_PORT, "TFTP port timer started\r\n" ));

      //
      //  Run the TFTP server forever
      //
      pTftpServer->Udpv4Index = -1;
      pTftpServer->Udpv6Index = -1;
      do {
        //
        //  Poll the network layer to create the TFTP port
        //  for the tftp server.  More than one attempt may
        //  be necessary since it may take some time to get
        //  the IP address and initialize the upper layers
        //  of the network stack.
        //
        if ( DIM ( pTftpServer->TftpPort ) != pTftpServer->Entries ) {
          do {
            //
            //  Wait a while before polling for a connection
            //
            if ( EFI_SUCCESS != gBS->CheckEvent ( pTftpServer->TimerEvent )) {
              if ( 0 == pTftpServer->Entries ) {
                break;
              }
              gBS->WaitForEvent ( 1, &pTftpServer->TimerEvent, &Index );
            }

            //
            //  Poll for a network connection
            //
            TftpServerSocket ( pTftpServer,
                               AF_INET,
                               &pTftpServer->Udpv4Index );
            TftpServerSocket ( pTftpServer,
                               AF_INET6,
                               &pTftpServer->Udpv6Index );
          } while ( 0 == pTftpServer->Entries );
        }

        //
        //  Poll the socket for activity
        //
        do {
          SocketPoll ( pTftpServer );

          //
          //  Normal TFTP lets the client request the retransmit by
          //  sending another ACK for the previous packet
          //
          if ( PcdGetBool ( Tftp_HighSpeed )) {
            UINT64 CurrentTime;
            UINT64 ElapsedTime;
            TSDT_CONNECTION_CONTEXT * pContext;
            TFTP_PACKET * pPacket;

            //
            //  High speed TFTP uses an agressive retransmit to
            //  get the TFTP client moving again when the ACK or
            //  previous data packet was lost.
            //
            //  Get the current time
            //
            CurrentTime = GetPerformanceCounter ( );

            //
            //  Walk the list of contexts
            //
            pContext = pTftpServer->pContextList;
            while ( NULL != pContext )
            {
              //
              //  Check for a transmit timeout
              //
              pPacket = pContext->pTxHead;
              if ( NULL != pPacket ) {
                //
                //  Compute the elapsed time
                //
                if ( pTftpServer->Time2 > pTftpServer->Time1 ) {
                  ElapsedTime = CurrentTime - pPacket->TxTime;
                }
                else {
                  ElapsedTime = pPacket->TxTime - CurrentTime;
                }
                ElapsedTime = GetTimeInNanoSecond ( ElapsedTime );

                //
                //  Determine if a retransmission is necessary
                //
                if ( ElapsedTime >= pContext->Rtt2x ) {
                  DEBUG (( DEBUG_WINDOW,
                            "0x%08x: Context TX timeout for packet 0x%08x, Window: %d\r\n",
                            pContext,
                            pPacket,
                            pContext->WindowSize ));
                  WindowTimeout ( pContext );
                }
              }

              //
              //  Set the next context
              //
              pContext = pContext->pNext;
            }
          }
        } while ( DIM ( pTftpServer->TftpPort ) == pTftpServer->Entries );
      } while ( !mbTftpServerExit );

      //
      //  Done with the timer event
      //
      gBS->SetTimer ( pTftpServer->TimerEvent,
                      TimerCancel,
                      0 );
    }
    gBS->CloseEvent ( pTftpServer->TimerEvent );
  }

  //
  //  Return the final status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
