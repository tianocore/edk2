/*++
  This file contains an 'Intel UEFI Application' and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/*++

Copyright (c)  2011 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

--*/

/** @file
  This is a simple TFTP server application

**/

#include <TftpServer.h>

TSDT_TFTP_SERVER mTftpServer; ///<  TFTP server's control structure


/**
  Add a connection context to the list of connection contexts.

  @param [in] pTftpServer   The TFTP server control structure address.

  @retval Context structure address, NULL if allocation fails

**/
TSDT_CONNECTION_CONTEXT *
ContextAdd (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  size_t LengthInBytes;
  TSDT_CONNECTION_CONTEXT * pContext;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Allocate a new context
    //
    LengthInBytes = sizeof ( *pContext );
    Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                 LengthInBytes,
                                 (VOID **)&pContext );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR | DEBUG_POOL,
                "ERROR - Failed to allocate the context, Status: %r\r\n",
                Status ));
      pContext = NULL;
      break;
    }

    //
    //  Initialize the context
    //
    ZeroMem ( pContext, LengthInBytes );
    CopyMem ( &pContext->RemoteAddress,
              &pTftpServer->RemoteAddress,
              sizeof ( pContext->RemoteAddress ));
    pContext->BlockSize = TFTP_MAX_BLOCK_SIZE;
    pContext->pBuffer = &pContext->FileData[0];
    pContext->pEnd = &pContext->pBuffer[sizeof ( pContext->pBuffer )];
    pContext->MaxTransferSize = 0;
    pContext->MaxTransferSize -= 1;

    //
    //  Display the new context
    //
    DEBUG (( DEBUG_PORT_WORK | DEBUG_INFO,
              "0x%08x: Context for %d.%d.%d.%d:%d\r\n",
              pContext,
              (UINT8)pContext->RemoteAddress.sin_addr.s_addr,
              (UINT8)( pContext->RemoteAddress.sin_addr.s_addr >> 8 ),
              (UINT8)( pContext->RemoteAddress.sin_addr.s_addr >> 16 ),
              (UINT8)( pContext->RemoteAddress.sin_addr.s_addr >> 24 ),
              htons ( pContext->RemoteAddress.sin_port )));

    //
    //  Add the context to the context list
    //
    pContext->pNext = pTftpServer->pContextList;
    pTftpServer->pContextList = pContext;

    //
    //  All done
    //
    break;
  }

  //
  //  Return the connection context
  //
  DBG_EXIT_STATUS ( pContext );
  return pContext;
}


/**
  Locate a remote connection context.

  @param [in] pTftpServer   The TFTP server control structure address.

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
    if (( pTftpServer->RemoteAddress.sin_addr.s_addr == pContext->RemoteAddress.sin_addr.s_addr )
      && ( pTftpServer->RemoteAddress.sin_port == pContext->RemoteAddress.sin_port )) {
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

  @param [in] pTftpServer    The TFTP server control structure address.

  @param [in] pContext       The context structure address.

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
  Process the work for the sockets.

  @param [in] pTftpServer   The TFTP server control structure address.

**/
VOID
PortWork (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  TSDT_CONNECTION_CONTEXT * pContext;
  socklen_t RemoteAddressLength;

  DBG_ENTER ( );

  //
  //  Handle input events
  //
  if ( 0 != ( pTftpServer->TftpPort.revents & POLLRDNORM )) {
    //
    //  Receive the message from the remote system
    //
    RemoteAddressLength = sizeof ( pTftpServer->RemoteAddress );
    pTftpServer->RxBytes = recvfrom ( pTftpServer->TftpPort.fd,
                                      &pTftpServer->RxBuffer[0],
                                      sizeof ( pTftpServer->RxBuffer ),
                                      0,
                                      (struct sockaddr *) &pTftpServer->RemoteAddress,
                                      &RemoteAddressLength );
    if ( -1 != pTftpServer->RxBytes ) {
      pTftpServer->RemoteAddress.sin_len = (UINT8) RemoteAddressLength;
      DEBUG (( DEBUG_TFTP_PORT,
                 "Received %d bytes from %d.%d.%d.%d:%d\r\n",
                 pTftpServer->RxBytes,
                 pTftpServer->RemoteAddress.sin_addr.s_addr & 0xff,
                 ( pTftpServer->RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                 ( pTftpServer->RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                 ( pTftpServer->RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                 htons ( pTftpServer->RemoteAddress.sin_port )));

      //
      //  Lookup connection context using the remote system address and port
      //  to determine if an existing connection to this remote
      //  system exists
      //
      pContext = ContextFind ( pTftpServer );

      //
      //  Process the received message
      //
      TftpProcessRequest ( pTftpServer, pContext );
    }
    else {
      //
      //  Receive error on the TFTP server port
      //  Close the server socket
      //
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed receive on TFTP server port, errno: 0x%08x\r\n",
                errno ));
      pTftpServer->TftpPort.revents |= POLLHUP;
    }
  }

  //
  //  Handle the close event
  //
  if ( 0 != ( pTftpServer->TftpPort.revents & POLLHUP )) {
    //
    //  Close the port
    //
    close ( pTftpServer->TftpPort.fd );
    pTftpServer->TftpPort.fd = -1;
  }

  DBG_EXIT ( );
}


/**
  Scan the list of sockets and process any pending work

  @param [in] pTftpServer   The TFTP server control structure address.

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
  FDCount = poll ( &pTftpServer->TftpPort,
                   1,
                   CLIENT_POLL_DELAY );
  if ( -1 == FDCount ) {
    DEBUG (( DEBUG_ERROR | DEBUG_SOCKET_POLL,
              "ERROR - errno: %d\r\n",
              errno ));
  }

  if ( 0 < FDCount ) {
    //
    //  Process this port
    //
    PortWork ( pTftpServer );
    pTftpServer->TftpPort.revents = 0;
  }

  DEBUG (( DEBUG_SOCKET_POLL, "Exiting SocketPoll\r\n" ));
}


/**
  Convert a character to lower case

  @param [in] Character The character to convert

  @return   The lower case equivalent of the character

**/
int
tolower (
  int Character
  )
{
  //
  //  Determine if the character is upper case
  //
  if (( 'A' <= Character ) && ( 'Z' >= Character )) {
    //
    //  Convert the character to lower caes
    //
    Character += 'a' - 'A';
  }

  //
  //  Return the converted character
  //
  return Character;
}


/**
  Case independent string comparison

  @param [in] pString1  Zero terminated string address
  @param [in] pString2  Zero terminated string address

  @return     Returns the first character difference between string 1
              and string 2.

**/
int
stricmp (
  char * pString1,
  char * pString2
  )
{
  int Char1;
  int Char2;
  int Difference;

  //
  //  Walk the length of the strings
  //
  do {
    //
    //  Get the next characters
    //
    Char1 = (UINT8)*pString1++;
    Char2 = (UINT8)*pString2++;

    //
    //  Convert them to lower case
    //
    Char1 = tolower ( Char1 );
    Char2 = tolower ( Char2 );

    //
    //  Done when the characters differ
    //
    Difference = Char1 - Char2;
    if ( 0 != Difference ) {
      break;
    }

    //
    //  Done at the end of the string
    //
  } while ( 0 != Char1 );

  //
  //  Return the difference
  //
  return Difference;
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

  @param [in] pContext  The context structure address.
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
  UINT8 * pTemp;
  UINT8 * pValue;
  EFI_STATUS Status;
  INT32 Value;

  //
  //  Start the OACK packet
  //  Let the OACK handle the parsing errors
  //  See http://tools.ietf.org/html/rfc2347
  //
  pOack = &pContext->TxBuffer[0];
  *pOack++ = 0;
  *pOack++ = TFTP_OP_OACK;
  pContext->TxBytes = 2;

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
      if ( 0 == stricmp ((char *)pOption, "blksize" )) {
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
            pContext->TxBytes += pOack - pTemp;
          }
        }
      }

      //
      //  timeout - See http://tools.ietf.org/html/rfc2349
      //
      else if ( 0 == stricmp ((char *)pOption, "timeout" )) {
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
            pContext->Timeout = Value;
            DEBUG (( DEBUG_TFTP_REQUEST,
                      "Using timeout of %d seconds\r\n",
                      pContext->Timeout ));

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
            pOack = TftpOptionSet ( pOack, pContext->Timeout );
            *pOack++ = 0;
            pContext->TxBytes += pOack - pTemp;
          }
        }
      }

      //
      //  tsize - See http://tools.ietf.org/html/rfc2349
      //
      else if ( 0 == stricmp ((char *)pOption, "tsize" )) {
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
            pContext->TxBytes += pOack - pTemp;
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

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    Connection context structure address

**/
VOID
TftpProcessRequest (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  BOOLEAN bCloseContext;
  BOOLEAN bIgnorePacket;
  UINT16 BlockNumber;
  UINT16 Opcode;
  UINT8 * pBuffer;
  UINT8 * pEnd;
  UINT8 * pFileName;
  UINT8 * pMode;
  UINT8 * pOption;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Get the opcode
  //
  pBuffer = &pTftpServer->RxBuffer[0];
  Opcode = HTONS ( *(UINT16 *)&pBuffer[0]);
Print ( L"TFTP Opcode: 0x%08x\r\n", Opcode );

  //
  //  Validate the parameters
  //
  bCloseContext = FALSE;
  bIgnorePacket = FALSE;
  switch ( Opcode ) {
  default:
    DEBUG (( DEBUG_TFTP_REQUEST,
              "ERROR - Unknown TFTP opcode: %d\r\n",
              Opcode ));
    bIgnorePacket = TRUE;
    break;

  case TFTP_OP_READ_REQUEST:
    break;

  case TFTP_OP_DATA:
    if ( NULL == pContext ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                (UINT8)pTftpServer->RemoteAddress.sin_addr.s_addr,
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 8 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 16 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 24 ),
                htons ( pTftpServer->RemoteAddress.sin_port )));
      bIgnorePacket = TRUE;
      break;
    }
    if ( pContext->bExpectAck ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Expecting ACKs not data for pContext 0x%08x\r\n",
                pContext ));
      bIgnorePacket = TRUE;
      break;
    }
    if ( pTftpServer->RxBytes > (ssize_t)( pContext->BlockSize + 2 + 2 )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Receive data length of %d > %d bytes (maximum block size) for pContext 0x%08x\r\n",
                pTftpServer->RxBytes - 2 - 2,
                pContext->BlockSize,
                pContext ));
      bIgnorePacket = TRUE;
      break;
    }
    break;

  case TFTP_OP_ACK:
    if ( NULL == pContext ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                (UINT8)pTftpServer->RemoteAddress.sin_addr.s_addr,
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 8 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 16 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 24 ),
                htons ( pTftpServer->RemoteAddress.sin_port )));
      bIgnorePacket = TRUE;
    }
    if ( !pContext->bExpectAck ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Expecting data not ACKs for pContext 0x%08x\r\n",
                pContext ));
      bIgnorePacket = TRUE;
      break;
    }
    break;

  case TFTP_OP_ERROR:
    if ( NULL == pContext ) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - File not open for %d.%d.%d.%d:%d\r\n",
                (UINT8)pTftpServer->RemoteAddress.sin_addr.s_addr,
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 8 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 16 ),
                (UINT8)( pTftpServer->RemoteAddress.sin_addr.s_addr >> 24 ),
                htons ( pTftpServer->RemoteAddress.sin_port )));
      bIgnorePacket = TRUE;
    }
    break;
  }
  if ( !bIgnorePacket ) {
    //
    //  Process the request
    //
    switch ( Opcode ) {
    default:
      DEBUG (( DEBUG_TFTP_REQUEST,
                "ERROR - Unable to process TFTP opcode: %d\r\n",
                Opcode ));
      break;

    case TFTP_OP_READ_REQUEST:

      //
      //  Close the context if necessary
      //
      if ( NULL != pContext ) {
        ContextRemove ( pTftpServer, pContext );
      }

      //
      //  Create the connection context
      //
      pContext = ContextAdd ( pTftpServer );
      if ( NULL == pContext ) {
        break;
      }

      //
      //  Locate the mode
      //
      pFileName = &pBuffer[2];
      pEnd = &pBuffer[pTftpServer->RxBytes];
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
        TftpSendError ( pTftpServer,
                        pContext,
                        0,
                        (UINT8 *)"File open mode not found" );
        break;
      }
      pMode += 1;
      DEBUG (( DEBUG_TFTP_REQUEST,
                "TFTP - FileName: %a\n",
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
        TftpSendError ( pTftpServer,
                        pContext,
                        0,
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
      if ( 0 != stricmp ((char *)pMode, "octet" )) {
        //
        //  File access mode not supported
        //
        DEBUG (( DEBUG_ERROR | DEBUG_TFTP_REQUEST,
                  "ERROR - File mode %a not supported\r\n",
                  pMode ));

        //
        //  Tell the client of the error
        //
        TftpSendError ( pTftpServer,
                        pContext,
                        0,
                        (UINT8 *)"File open mode not supported" );
        break;
      }

      //
      //  Open the file, close the context on error
      //
// TODO: Remove the following line
pContext->File = (EFI_HANDLE)1;

      //
      //  Determine the file length
      //
//fstat

      //
      //  Process the options
      //
      TftpOptions ( pContext, pOption, pEnd );

      //
      //  Read in the first portion of the file
      //

      //
      //  Send the first block
      //
      pContext->bExpectAck = TRUE;
      if ( 2 < pContext->TxBytes ) {
        //
        //  Send the OACK
        //
        Status = TftpTxPacket ( pTftpServer, pContext );
      }
      else {
        //
        //  Send the first block of data
        //
        Status = TftpSendNextBlock ( pTftpServer, pContext );
      }
      break;

    case TFTP_OP_ACK:
      //
      //  Get the block number that is being ACKed
      //
      BlockNumber = pTftpServer->RxBuffer[2];
      BlockNumber <<= 8;
      BlockNumber |= pTftpServer->RxBuffer[3];

      //
      //  Determine if this is the correct ACK
      //
      DEBUG (( DEBUG_TFTP_ACK,
                "ACK for block 0x%04x received\r\n",
                BlockNumber ));
      if (( !pContext->bExpectAck )
        || ( BlockNumber != pContext->AckNext )) {
        DEBUG (( DEBUG_WARN | DEBUG_TFTP_ACK,
                  "WARNING - Expecting ACK 0x%0x4 not received ACK 0x%08x\r\n",
                  pContext->AckNext,
                  BlockNumber ));
      }
      else {
        //
        //  Process the expected ACK
        //
        if ( pContext->bEofSent ) {
          bCloseContext = TRUE;
        }
        else {
          //
          //  Set the next expected ACK
          //
          pContext->AckNext += 1;

          //
          //  Send the next packet of data
          //
          Status = TftpSendNextBlock ( pTftpServer, pContext );
        }
      }
      break;
    }
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
  Build and send an error packet

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    The context structure address.
  @param [in] Error       Error number for the packet
  @param [in] pError      Zero terminated error string address

  @retval EFI_SUCCESS     Message processed successfully

**/
EFI_STATUS
TftpSendError (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN UINT16 Error,
  IN UINT8 * pError
  )
{
  UINT8 Character;
  UINT8 * pBuffer;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Build the error packet
  //
  pBuffer = &pContext->TxBuffer[0];
  pBuffer[0] = 0;
  pBuffer[1] = TFTP_OP_ERROR;
  pBuffer[2] = (UINT8)( Error >> 8 );
  pBuffer[3] = (UINT8)Error;

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
  pContext->TxBytes = pBuffer - &pContext->TxBuffer[0];
  Status = TftpTxPacket ( pTftpServer, pContext );

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Send the next block of file system data

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    The context structure address.

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
TftpSendNextBlock (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  )
{
  ssize_t LengthInBytes;
  UINT8 * pBuffer;
  EFI_STATUS Status;

  //
  //  Determine how much data needs to be sent
  //
  LengthInBytes = pContext->BlockSize;
  if (( pContext->LengthInBytes < TFTP_MAX_BLOCK_SIZE )
    || ( LengthInBytes > (ssize_t)pContext->LengthInBytes )) {
    LengthInBytes = (ssize_t)pContext->LengthInBytes;
    pContext->bEofSent = TRUE;
  }

  //
  //  Set the TFTP opcode and block number
  //
  pBuffer = &pContext->TxBuffer[0];
  *pBuffer++ = 0;
  *pBuffer++ = TFTP_OP_DATA;
  *pBuffer++ = (UINT8)( pContext->AckNext >> 8 );
  *pBuffer++ = (UINT8)pContext->AckNext;

  //
  //  Copy the file data into the transmit buffer
  //
  pContext->TxBytes = 2 + 2 + LengthInBytes;
  if ( 0 < LengthInBytes ) {
    CopyMem ( &pBuffer,
              pContext->pBuffer,
              LengthInBytes );
  }

  //
  //  Send the next block
  //
  Status = TftpTxPacket ( pTftpServer, pContext );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Create the port for the TFTP server

  This routine polls the network layer to create the TFTP port for the
  TFTP server.  More than one attempt may be necessary since it may take
  some time to get the IP address and initialize the upper layers of
  the network stack.

  @param [in] pTftpServer  The TFTP server control structure address.

**/
VOID
TftpServerTimer (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  UINT16 TftpPort;
  int SocketStatus;
  EFI_STATUS Status;

  DEBUG (( DEBUG_SERVER_TIMER, "Entering TftpServerTimer\r\n" ));

  //
  //  Open the TFTP port on the server
  //
  do {
    do {
      //
      //  Wait for a while
      //
      Status = gBS->CheckEvent ( pTftpServer->TimerEvent );
    } while ( EFI_SUCCESS != Status );

    //
    //  Attempt to create the socket for the TFTP server
    //
    pTftpServer->TftpPort.events = POLLRDNORM | POLLHUP;
    pTftpServer->TftpPort.revents = 0;
    pTftpServer->TftpPort.fd = socket ( AF_INET,
                                        SOCK_DGRAM,
                                        IPPROTO_UDP );
    if ( -1 != pTftpServer->TftpPort.fd ) {
      //
      //  Set the socket address
      //
      ZeroMem ( &pTftpServer->TftpServerAddress,
                sizeof ( pTftpServer->TftpServerAddress ));
      TftpPort = 69;
      DEBUG (( DEBUG_TFTP_PORT,
                "TFTP Port: %d\r\n",
                TftpPort ));
      pTftpServer->TftpServerAddress.sin_len = sizeof ( pTftpServer->TftpServerAddress );
      pTftpServer->TftpServerAddress.sin_family = AF_INET;
      pTftpServer->TftpServerAddress.sin_addr.s_addr = INADDR_ANY;
      pTftpServer->TftpServerAddress.sin_port = htons ( TftpPort );

      //
      //  Bind the socket to the TFTP port
      //
      SocketStatus = bind ( pTftpServer->TftpPort.fd,
                            (struct sockaddr *) &pTftpServer->TftpServerAddress,
                            pTftpServer->TftpServerAddress.sin_len );
      if ( -1 != SocketStatus ) {
        DEBUG (( DEBUG_TFTP_PORT,
                  "0x%08x: Socket bound to port %d\r\n",
                  pTftpServer->TftpPort.fd,
                  TftpPort ));
      }

      //
      //  Release the socket if necessary
      //
      if ( -1 == SocketStatus ) {
        close ( pTftpServer->TftpPort.fd );
        pTftpServer->TftpPort.fd = -1;
      }
    }

    //
    //  Wait until the socket is open
    //
  }while ( -1 == pTftpServer->TftpPort.fd );

  DEBUG (( DEBUG_SERVER_TIMER, "Exiting TftpServerTimer\r\n" ));
}


/**
  Start the TFTP server port creation timer

  @param [in] pTftpServer The TFTP server control structure address.

  @retval EFI_SUCCESS         The timer was successfully started.
  @retval EFI_ALREADY_STARTED The timer is already running.
  @retval Other               The timer failed to start.

**/
EFI_STATUS
TftpServerTimerStart (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  EFI_STATUS Status;
  UINT64 TriggerTime;

  DBG_ENTER ( );

  //
  //  Assume the timer is already running
  //
  Status = EFI_ALREADY_STARTED;
  if ( !pTftpServer->bTimerRunning ) {
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
      //  Mark the timer running
      //
      pTftpServer->bTimerRunning = TRUE;
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_TFTP_PORT,
                "ERROR - Failed to start TFTP port timer, Status: %r\r\n",
                Status ));
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Stop the TFTP server port creation timer

  @param [in] pTftpServer The TFTP server control structure address.

  @retval EFI_SUCCESS   The TFTP port timer is stopped
  @retval Other         Failed to stop the TFTP port timer

**/
EFI_STATUS
TftpServerTimerStop (
  IN TSDT_TFTP_SERVER * pTftpServer
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume the timer is stopped
  //
  Status = EFI_SUCCESS;
  if ( pTftpServer->bTimerRunning ) {
    //
    //  Stop the port creation polling
    //
    Status = gBS->SetTimer ( pTftpServer->TimerEvent,
                             TimerCancel,
                             0 );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_TFTP_PORT, "TFT[ port timer stopped\r\n" ));

      //
      //  Mark the timer stopped
      //
      pTftpServer->bTimerRunning = FALSE;
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_TFTP_PORT,
                "ERROR - Failed to stop TFT[ port timer, Status: %r\r\n",
                Status ));
    }
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}

/**
  Send the next TFTP packet

  @param [in] pTftpServer   The TFTP server control structure address.
  @param [in] pContext      The context structure address.

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
TftpTxPacket (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
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
  //  Send the TFTP packet
  //
  DEBUG (( DEBUG_TX,
            "0x%08x: pContext sending 0x%08x bytes\r\n",
            pContext,
            pContext->TxBytes ));
  LengthInBytes = sendto ( pTftpServer->TftpPort.fd,
                           &pContext->TxBuffer[0],
                           pContext->TxBytes,
                           0,
                           (struct sockaddr *)&pContext->RemoteAddress,
                           pContext->RemoteAddress.sin_len );
  if ( -1 == LengthInBytes ) {
    DEBUG (( DEBUG_ERROR | DEBUG_TX,
              "ERROR - Transmit failure, errno: 0x%08x\r\n",
              errno ));
    Status = EFI_DEVICE_ERROR;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
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
  TSDT_TFTP_SERVER * pTftpServer;
  EFI_STATUS Status;

  //
  //  Create a timer event to start TFTP port
  //
  pTftpServer = &mTftpServer;
  Status = gBS->CreateEvent ( EVT_TIMER,
                              TPL_TFTP_SERVER,
                              NULL,
                              NULL,
                              &pTftpServer->TimerEvent );
  if ( !EFI_ERROR ( Status )) {
    Status = TftpServerTimerStart ( pTftpServer );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Run the TFTP server forever
      //
      for ( ; ; ) {
        //
        //  Poll the network layer to create the TFTP port
        //  for the tftp server.  More than one attempt may
        //  be necessary since it may take some time to get
        //  the IP address and initialize the upper layers
        //  of the network stack.
        //
        TftpServerTimer ( pTftpServer );

        //
        //  Poll the socket for activity
        //
        do {
          SocketPoll ( pTftpServer );
        } while ( -1 != pTftpServer->TftpPort.fd );

//
// TODO: Remove the following test code
//  Exit when the network connection is broken
//
break;
      }

      //
      //  Done with the timer event
      //
      TftpServerTimerStop ( pTftpServer );
      Status = gBS->CloseEvent ( pTftpServer->TimerEvent );
    }
  }

  //
  //  Return the final status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
